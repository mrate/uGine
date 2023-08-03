#pragma once

#include <ugine/Path.h>
#include <ugine/String.h>
#include <ugine/Vector.h>

namespace ugine::ed {

Vector<Path> OpenFileDialog(void* nativeWindowHandle, const Vector<String>& filter, bool multipleFiles);

}