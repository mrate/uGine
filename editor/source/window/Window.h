#pragma once

#include <ugine/EventEmittor.h>
#include <ugine/String.h>

#include <ugine/engine/gfx/ImGui.h>

namespace ugine::ed {

class EditorContext;

class ModalWindow : public EventEmittor {
public:
    virtual ~ModalWindow() = default;

    virtual const char* Name() const = 0;
    virtual void Init() = 0;
    virtual void Show() = 0;
    virtual int Flags() const { return 0; }
};

class ButtonWindow {
public:
    virtual ~ButtonWindow() = default;

protected:
    struct Button {
        bool enabled{ true };
        String name;
        std::function<void()> func;
    };

    void BeginContent();
    void EndContent();
    void Buttons();

    float RightButtonsWidth() const;
    float ButtonsHeight() const;

    void SetLeftButton(const Button& button);
    void SetLeftButtons(Vector<Button> buttons);
    void SetRightButton(const Button& button);
    void SetRightButtons(Vector<Button> buttons);

    void SetRightButtonEnabled(int i, bool enabled);

private:
    void CalcRightSize();

    Vector<Button> leftButtons_;
    Vector<Button> rightButtons_;

    ImVec2 size_;
    float rightButtonsSize_{};
};

class ModalButtonWindow : public ModalWindow, public ButtonWindow {
public:
    explicit ModalButtonWindow(EditorContext& context);

protected:
};

class MessageWindow : public ModalWindow {
public:
    enum class Type {
        Error = 0,
        Warning,
        Info,
    };

    explicit MessageWindow(EditorContext& context)
        : context_{ context } {}

    void SetType(Type type) { type_ = type; }
    void SetText(StringView text) { text_ = text; }

    int Flags() const { return ImGuiWindowFlags_AlwaysAutoResize; }

    const char* Name() const override { return "Message"; }
    void Init() override {}
    void Show() override;

private:
    EditorContext& context_;
    Type type_{};
    String text_{};
};

} // namespace ugine::ed