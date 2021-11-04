// ReSharper disable CppUnusedIncludeDirective
#pragma once

#include "YAMLAliases.h"
#include "FYamlNode.h"
#include "Parse.h"
#include "Iteration.h"

#include "Modules/ModuleInterface.h"

class FUnrealYAMLModule final : public IModuleInterface {
	virtual bool IsGameModule() const override {
		return false;
	}
};