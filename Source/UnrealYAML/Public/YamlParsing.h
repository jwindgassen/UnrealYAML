// Copyright (c) 2021-2026, Forschungszentrum Jülich GmbH. All rights reserved.
// Licensed under the MIT License. See LICENSE file for details.

#pragma once

#include "CoreMinimal.h"
#include "YamlNode.h"
#include "YamlParsing.generated.h"


DECLARE_LOG_CATEGORY_EXTERN(LogYamlParsing, Log, All)

UCLASS(BlueprintType)
class UNREALYAML_API UYamlParsing final : public UBlueprintFunctionLibrary {
    GENERATED_BODY()

public:
    /**
     * Parses a String into a YAML Node.
     *
     * @returns If the Parsing was successful */
    UFUNCTION(BlueprintCallable, Category = "YAML")
    static bool ParseYaml(const FString String, FYamlNode& Out);

    /**
     * Opens a File and Parses the Contents into a YAML Node.
     *
     * @returns If the File Exists and the Parsing was successful */
    UFUNCTION(BlueprintCallable, Category = "YAML")
    static bool LoadYamlFromFile(const FString Path, FYamlNode& Out);

    /**
     * Writes the Contents of a YAML Node to a File.
     *
     * This will overwrite the existing File if it exists!
     */
    UFUNCTION(BlueprintCallable, Category = "YAML")
    static void WriteYamlToFile(const FString Path, FYamlNode Node);
};