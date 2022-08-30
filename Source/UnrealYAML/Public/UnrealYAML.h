// ReSharper disable CppUnusedIncludeDirective
#pragma once

#include "Modules/ModuleInterface.h"

class FUnrealYAMLModule final : public IModuleInterface {
    virtual bool IsGameModule() const override {
        return false;
    }
};
