// Copyright (c) 2021-2026, Forschungszentrum Jülich GmbH. All rights reserved.
// Licensed under the MIT License. See LICENSE file for details.

#include "YamlParsing.h"


DEFINE_LOG_CATEGORY(LogYamlParsing)


// Parsing into/from Files ---------------------------------------------------------------------------------------------
bool UYamlParsing::ParseYaml(const FString String, FYamlNode& Out) {
    try {
        Out = FYamlNode(YAML::Load(TCHAR_TO_UTF8(*String)));
        return true;
    } catch (YAML::ParserException) {
        return false;
    }
}

bool UYamlParsing::LoadYamlFromFile(const FString Path, FYamlNode& Out) {
    FString Contents;
    if (FFileHelper::LoadFileToString(Contents, *Path)) {
        return ParseYaml(Contents, Out);
    }
    return false;
}

void UYamlParsing::WriteYamlToFile(const FString Path, const FYamlNode Node) {
    FFileHelper::SaveStringToFile(Node.GetContent(), *Path);
}
