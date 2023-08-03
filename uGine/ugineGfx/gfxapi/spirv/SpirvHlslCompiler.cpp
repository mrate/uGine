#include "SpirvHlslCompiler.h"

#include <ugine/Log.h>

#include <Windows.h>

#include <dxc/dxcapi.h>
#include <wrl/client.h>

#include <filesystem>
#include <iostream>
#include <regex>

namespace ugine::gfxapi {

class DxcUtils {
public:
    Microsoft::WRL::ComPtr<IDxcUtils> pUtils{};
    Microsoft::WRL::ComPtr<IDxcIncludeHandler> pDefaultIncludeHandler{};

    DxcUtils() {
        auto hr{ DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&pUtils)) };
        if (SUCCEEDED(hr)) {
            pUtils->CreateDefaultIncludeHandler(&pDefaultIncludeHandler);
        }
    }
};

class CustomIncludeHandler : public IDxcIncludeHandler {
public:
    inline static DxcUtils Utils{};

    CustomIncludeHandler(const std::filesystem::path& shaderPath, const std::vector<std::filesystem::path>& systemDirs)
        : shaderPath_{ shaderPath }
        , systemDirs_{ systemDirs } {}

    HRESULT STDMETHODCALLTYPE LoadSource(_In_ LPCWSTR pFilename, _COM_Outptr_result_maybenull_ IDxcBlob** ppIncludeSource) override {
        std::filesystem::path result{ shaderPath_ / pFilename };
        if (!std::filesystem::exists(result)) {
            for (const auto& sysDir : systemDirs_) {
                result = sysDir / pFilename;
                if (std::filesystem::exists(result)) {
                    break;
                }
            }

            if (!std::filesystem::exists(result)) {
                return E_FAIL;
            }
        }

        const auto can{ std::filesystem::canonical(result.make_preferred()) };
        dependenices_.push_back(can);

        // Return empty string blob if this file has been included before
        Microsoft::WRL::ComPtr<IDxcBlobEncoding> pEncoding;
        HRESULT hr = Utils.pUtils->LoadFile(can.c_str(), nullptr, &pEncoding);
        if (SUCCEEDED(hr)) {
            *ppIncludeSource = pEncoding.Detach();
        } else {
            *ppIncludeSource = nullptr;
        }
        return hr;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) override {
        return Utils.pDefaultIncludeHandler ? Utils.pDefaultIncludeHandler->QueryInterface(riid, ppvObject) : E_FAIL;
    }

    ULONG STDMETHODCALLTYPE AddRef(void) override { return 0; }
    ULONG STDMETHODCALLTYPE Release(void) override { return 0; }

    const std::vector<std::filesystem::path>& Dependenices() const { return dependenices_; }

private:
    std::filesystem::path shaderPath_;
    std::vector<std::filesystem::path> systemDirs_;
    std::vector<std::filesystem::path> dependenices_;
};

inline std::wstring ConvertUTF8(std::string_view input) {
    const auto size{ MultiByteToWideChar(CP_UTF8, 0, input.data(), -1, nullptr, 0) };
    std::wstring result(size, '\0');

    const auto success{ MultiByteToWideChar(CP_UTF8, 0, input.data(), -1, result.data(), static_cast<int>(result.size())) };
    if (!success) {
        throw std::system_error(std::error_code(static_cast<int>(GetLastError()), std::system_category()), "MultiByteToWideChar");
    }

    return result;
}

std::string GetTarget(ShaderStage stage, std::string_view version) {
    switch (stage) {
    case ShaderStage::VertexShader: return std::string{ "vs_" } + version.data();
    case ShaderStage::FragmentShader: return std::string{ "ps_" } + version.data();
    case ShaderStage::DomainShader: return std::string{ "ds_" } + version.data();
    case ShaderStage::HullShader: return std::string{ "hs_" } + version.data();
    case ShaderStage::GeometryShader: return std::string{ "gs_" } + version.data();
    case ShaderStage::ComputeShader: return std::string{ "cs_" } + version.data();
    default:
        // TODO:
        return version.data();
    }
}

class SpirvHlslCompilerImpl {
public:
    SpirvHlslCompilerImpl() {}

    Microsoft::WRL::ComPtr<IDxcResult> Run(IDxcCompiler3* dxcCompiler, std::vector<const wchar_t*> aditionalArgs, const SpirvCompiler::Options& options) {
        std::vector<const wchar_t*> arguments;

        const auto target{ GetTarget(options.stage, "6_0") };

        const auto wEntryPoint{ ConvertUTF8(options.entryPoint) };
        const auto wTarget{ ConvertUTF8(target) };

        // Add full path source name as a first argument for better DebugInfo.
        arguments.push_back(options.sourceFile.c_str());

        // Aditional arguments.
        arguments.insert(std::end(arguments), std::begin(aditionalArgs), std::end(aditionalArgs));

        // Spir-V output.
        arguments.push_back(L"-spirv");
        arguments.push_back(L"-fspv-target-env=vulkan1.3");

        //-E for the entry point (eg. PSMain)
        arguments.push_back(L"-E");
        arguments.push_back(wEntryPoint.c_str());

        //-T for the target profile (eg. ps_6_2)
        arguments.push_back(L"-T");
        arguments.push_back(wTarget.c_str());

        switch (options.optimizationLevel) {
        case SpirvCompiler::Optimization::None: arguments.push_back(DXC_ARG_SKIP_OPTIMIZATIONS); break;
        case SpirvCompiler::Optimization::Size: break;
        case SpirvCompiler::Optimization::Performance: arguments.push_back(DXC_ARG_OPTIMIZATION_LEVEL1); break;
        }

        arguments.push_back(DXC_ARG_ALL_RESOURCES_BOUND);

        if (options.warningsAsErrors) {
            arguments.push_back(DXC_ARG_WARNINGS_ARE_ERRORS); //-WX
        }

        if (options.debugInfo) {
            //arguments.push_back(DXC_ARG_DEBUG); //-Zi
            arguments.push_back(L"-fspv-debug=vulkan-with-source");
        }

        // Add defines -D.
        std::vector<std::wstring> wDefines;
        wDefines.reserve(options.defines.size());
        for (const auto& [name, value] : options.defines) {
            wDefines.push_back(ConvertUTF8(name)); // TODO: value
        }

        for (const auto& wDefine : wDefines) {
            arguments.push_back(L"-D");
            arguments.push_back(wDefine.c_str());
        }

        // Add explicit -I for include dirs.
        for (const auto& include : options.includeSearchDirs) {
            arguments.push_back(L"-I");
            arguments.push_back(include.c_str());
        }

        DxcBuffer sourceBuffer;
        sourceBuffer.Ptr = options.source.c_str();
        sourceBuffer.Size = options.source.size();
        sourceBuffer.Encoding = DXC_CP_ACP;

        CustomIncludeHandler include{ options.sourceFile.filename().parent_path(), options.includeSearchDirs };

        Microsoft::WRL::ComPtr<IDxcResult> pCompileResult;
        auto hr{ dxcCompiler->Compile(&sourceBuffer, arguments.data(), (u32)arguments.size(), &include, IID_PPV_ARGS(&pCompileResult)) };

        Microsoft::WRL::ComPtr<IDxcBlobUtf8> pErrors;
        if (FAILED(hr)) {
            pCompileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr);
            if (pErrors && pErrors->GetStringLength() > 0) {
                error_ = pErrors->GetStringPointer();
                UGINE_ERROR("Compilation failed: {}", pErrors->GetStringPointer());
                return {};
            } else {
                UGINE_ERROR("Compilation failed: unknown error");
                return {};
            }
        }

        return pCompileResult;
    }

    bool Compile(const SpirvCompiler::Options& options) {
        Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler;
        auto hr{ DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler)) };
        if (FAILED(hr)) {
            UGINE_ERROR("Failed to create DXC compiler instance.");
            return false;
        }

        if (options.preprocess) {
            // Preprocess.
            std::vector<const wchar_t*> args{ L"-P" };
            auto pCompileResult{ Run(dxcCompiler.Get(), args, options) };
            if (!pCompileResult) {
                return false;
            }

            Microsoft::WRL::ComPtr<IDxcBlobUtf8> hlsl;
            hr = pCompileResult->GetOutput(DXC_OUT_HLSL, IID_PPV_ARGS(&hlsl), nullptr);
            if (FAILED(hr)) {
                error_ = "Failed to preprocess hlsl";
                UGINE_ERROR("Failed to preprocess hlsl");
                return false;
            }

            preprocessed_ = hlsl->GetStringPointer();
        } else {
            preprocessed_ = options.source;
        }

        auto pCompileResult{ Run(dxcCompiler.Get(), {}, options) };
        if (!pCompileResult) {
            return false;
        }

        Microsoft::WRL::ComPtr<IDxcBlobUtf8> pErrors;
        hr = pCompileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr);
        if (SUCCEEDED(hr) && pErrors != nullptr && pErrors->GetStringLength() != 0) {
            error_ = pErrors->GetStringPointer();
            ParseErrorLine(error_);

            UGINE_WARN("Warnings and errors: {}", error_);
        }

        HRESULT status{ S_OK };
        hr = pCompileResult->GetStatus(&status);
        if (FAILED(hr) || FAILED(status)) {
            UGINE_ERROR("Compilation error: {}", status);
            return false;
        }

        Microsoft::WRL::ComPtr<IDxcBlob> output{};
        hr = pCompileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&output), nullptr);
        if (FAILED(hr)) {
            UGINE_ERROR("Compilation successful, failed to get result");
            return false;
        }

        result_.resize(output->GetBufferSize());
        memcpy(result_.data(), output->GetBufferPointer(), output->GetBufferSize());
        //return binary;

        return true;
    }

    void ParseErrorLine(std::string_view error) {
        std::regex re{ "^(.*):([0-9]+):([0-9]+):" };
        std::smatch match;
        std::string str{ error };

        try {
            if (std::regex_search(str, match, re) && match.size() >= 4) {
                // TODO:
                errorFile_ = match[1].str();
                errorLine_ = std::atoi(match[2].str().c_str());
                errorChar_ = std::atoi(match[3].str().c_str());
            }
        } catch (...) {
        }
    }

    std::vector<u8> result_;

    std::string error_;

    std::string errorFile_;
    int errorLine_{};
    int errorChar_{};

    std::string preprocessed_;
};

SpirvHlslCompiler::SpirvHlslCompiler()
    : impl_{ std::make_unique<SpirvHlslCompilerImpl>() } {
}

SpirvHlslCompiler::~SpirvHlslCompiler() {
}

bool SpirvHlslCompiler::Compile(const Options& options) {
    return impl_->Compile(options);
}

const std::string& SpirvHlslCompiler::Error() const {
    return impl_->error_;
}

const std::string& SpirvHlslCompiler::Preprocessed() const {
    return impl_->preprocessed_;
}

int SpirvHlslCompiler::ErrorLine() const {
    return impl_->errorLine_;
}

int SpirvHlslCompiler::ErrorChar() const {
    return impl_->errorChar_;
}

const std::string& SpirvHlslCompiler::ErrorFile() const {
    return impl_->errorFile_;
}

const std::vector<u8>& SpirvHlslCompiler::Compiled() const {
    return impl_->result_;
}

} // namespace ugine::gfxapi