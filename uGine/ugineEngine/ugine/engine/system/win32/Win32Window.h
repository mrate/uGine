#pragma once

#ifdef _WIN32

#include <ugine/engine/system/Event.h>

#include <Windows.h>

#include <filesystem>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace ugine::win32 {

class Win32Window {
public:
    using CustomWndProc = std::function<LRESULT(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)>;

    Win32Window();
    ~Win32Window();

    bool Init(HINSTANCE hInstance, const std::wstring& title, const std::wstring& name, LPCWSTR iconName, int width, int height, bool resizable = false,
        bool imgui = false);

    void Show();
    void Close();

    void SetTitle(const std::string_view& title);

    void NewFrame();

    void Show(int nCmdShow);

    std::pair<int, int> GetSize() const;
    bool Closed() const;

    HWND Handle() const { return hwnd_; }

    HINSTANCE Instance() const { return inst_; }

    void SetMouseCapture(bool capture);

    bool Update() {
        if (captureMouse_) {
            return HandleMouseCapture();
        }

        return false;
    }

    std::optional<SystemEvent> PopEvent() { 
        return std::exchange(lastEvent_, std::nullopt);
    }

private:
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    std::vector<std::filesystem::path> OpenFileDialog(HWND hwnd, const std::vector<std::wstring>& filter, bool multipleFiles) const;

    void SetResizing(bool resizing);
    void UpdateSize();
    void HandleResize(unsigned int width, unsigned int height);

    bool HandleMouseCapture();
    void HandleInputEvent(const InputEvent& event);

    void HandleSystemEvent(const SystemEvent& event) { lastEvent_ = event; }

    HINSTANCE inst_{};
    HWND hwnd_{};

    unsigned int width_{};
    unsigned int height_{};

    unsigned int newWidth_{};
    unsigned int newHeight_{};

    bool visible_{};
    bool resizing_{};

    bool imgui_{};
    bool captureMouse_ {};

    int lastMouseX_{};
    int lastMouseY_{};

    std::optional<SystemEvent> lastEvent_;
};

} // namespace ugine::win32

#endif // _WIN32
