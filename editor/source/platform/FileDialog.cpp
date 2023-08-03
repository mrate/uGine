#include "FileDialog.h"

#include <ugine/File.h>
#include <ugine/FileSystem.h>

#include <Windows.h>

namespace ugine::ed {

Vector<Path> OpenFileDialog(void* nativeWindowHandle, const Vector<String>& filter, bool multipleFiles) {
    auto hwnd{ HWND(nativeWindowHandle) };

    static auto lastDir{ FileSystem::CurrentPath() };

    char szFilter[255];
    char* out = szFilter;
    for (const auto& f : filter) {
        strcpy_s(out, 255 - (out - szFilter), f.Data());
        out += f.Size() + 1;
    }
    *out = '\0';

    OPENFILENAMEA ofn;
    CHAR szFile[4096 + 1] = { 0 };
    ZeroMemory(&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = /*hwnd*/ nullptr;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = 4096;
    ofn.lpstrInitialDir = lastDir.Data();
    ofn.lpstrFilter = szFilter;
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    // TODO: Multiple files.
    // https://docs.microsoft.com/en-us/windows/win32/api/commdlg/ns-commdlg-openfilenamea
    if (multipleFiles) {
        ofn.Flags |= OFN_ALLOWMULTISELECT;
    }

    if (GetOpenFileNameA(&ofn) != TRUE) {
        return {};
    }

    Vector<Path> result;
    if (multipleFiles) {
        auto file{ ofn.lpstrFile };
        Path directory{ file };

        file += strlen(file) + 1;
        while (file[0] != '\0') {
            result.PushBack(directory / file);
            file += strlen(file) + 1;
        }

        if (result.Empty()) {
            result.PushBack(directory);
        }

    } else {
        //std::string file{ ofn.lpstrFile };
        result.PushBack(Path{ ofn.lpstrFile });
    }

    return result;
}

} // namespace ugine::ed