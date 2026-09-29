// Copyright (c) 2021-2026, Forschungszentrum Jülich GmbH. All rights reserved.
// Licensed under the MIT License. See LICENSE file for details.

#include "YamlParsing.h"


DEFINE_LOG_CATEGORY(LogYamlParsing)


// Parsing into/from Files ---------------------------------------------------------------------------------------------
FParseResult UYamlParsing::ParseYaml(const FString String) {
    try {
        const FYamlNode Node(YAML::Load(TCHAR_TO_UTF8(*String)));
        return {.Node = Node, .ParseError = ""};
    } catch (YAML::ParserException e) {
        const FString Error = FString::Printf(
            TEXT("Parsing Error at (%d,%d): %hs"), e.mark.line, e.mark.column, e.msg.c_str());
        return {.Node = FYamlNode{}, .ParseError = Error};
    }
}

FParseResult UYamlParsing::LoadYamlFromFile(const FString Path) {
    FString Contents;
    if (FFileHelper::LoadFileToString(Contents, *Path)) {
        return ParseYaml(Contents);
    }
    return { .Node = FYamlNode{}, .ParseError = "Could not open file" };
}

void UYamlParsing::WriteYamlToFile(const FString Path, const FYamlNode Node) {
    FFileHelper::SaveStringToFile(Node.GetContent(), *Path);
}
