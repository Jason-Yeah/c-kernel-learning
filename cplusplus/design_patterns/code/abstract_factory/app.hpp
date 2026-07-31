#pragma once

#include "button.hpp"
#include "dialog.hpp"
#include "factory.hpp"
#include <memory>

class Application
{
    std::unique_ptr<GUIFactory> factory_;
    std::unique_ptr<Button> btn_;
    std::unique_ptr<Dialog> dialog_;

public:
    using GUIFactory_t = std::unique_ptr<GUIFactory>;

    explicit Application(GUIFactory_t factory) : factory_(std::move(factory)) {}

    void run()
    {
        build_UI();
        btn_->render();
        dialog_->show();
    }

private:
    void build_UI()
    {
        btn_ = factory_->create_button();
        dialog_ = factory_->create_dialog();
    }
};
