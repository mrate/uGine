#pragma once

#include <ugine/String.h>

#include "../EditorTool.h"
#include "Window.h"

namespace ugine::ed {

class EditorWindow : public EditorTool {
public:
    EditorWindow(EditorContext& context, bool visible, StringView name, StringView id, int flags = 0);

    void Render() override;

protected:
    virtual void Content() = 0;
    virtual void Reload() = 0;
    virtual bool Save() = 0;
    virtual void Closed() = 0;
    virtual bool Valid() const = 0;
    virtual ImVec2 DefaultSize() const { return ImVec2{ 640, 480 }; }

    void SetModified(bool modified);

private:
    class ConfirmDialog : public ModalWindow {
    public:
        struct YesEvent {};
        struct NoEvent {};

        explicit ConfirmDialog(EditorContext& context)
            : context_{ context } {}

        void SetTitle(StringView title) { title_ = title; }
        void SetText(StringView text) { text_ = text; }

        const char* Name() const override { return title_.Data(); }
        void Init() override {}
        void Show() override;

    private:
        EditorContext& context_;
        String title_;
        String text_;
    };

    class SaveDialog : public ModalWindow {
    public:
        struct SaveEvent {};
        struct CancelEvent {};
        struct CloseEvent {};

        explicit SaveDialog(EditorContext& context)
            : context_{ context } {}

        const char* Name() const override { return "Save"; }
        void Init() override {}
        void Show() override;

    private:
        EditorContext& context_;
    };

    void OnSave();
    void OnClose();
    void OnCancel();

    void OnConfirmYes();
    void OnConfirmNo();

    void Close();
    void Toolbar();

    String name_;

    const int flags_;
    bool modified_{};
    bool closeOnSave_{};
    SaveDialog saveDialog_;
    ConfirmDialog confirmDialog_;
};

} // namespace ugine::ed