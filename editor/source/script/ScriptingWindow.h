#pragma once

#include "../EditorTool.h"
#include "ScriptTool.h"

#include "../window/Window.h"

#include <ugine/String.h>
#include <ugine/Vector.h>

namespace ugine::ed {

class EditorContext;

class ScriptingWindow : public ButtonWindow {
public:
    explicit ScriptingWindow(EditorContext& context);
    void Show();

    void Render();

private:
    void Execute();
    
    void AddMacro();
    void RemoveMacro();
    void SaveMacro(int index);
    void SaveMacros();

    void MacroList();
    void MacroContent();
    void SelectMacro(int index);

    EditorContext& context_;
    bool visible_{};
    
    Vector<char> scriptName_;
    Vector<char> scriptCode_;
    
    String error_;

    Vector<ScriptMacro> macros_;
    int selectedMacro_{ -1 };
};

} // namespace ugine::ed