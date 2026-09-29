// Copyright (c) 2021-2026, Forschungszentrum Jülich GmbH. All rights reserved.
// Licensed under the MIT License. See LICENSE file for details.

#pragma once

#include "CoreMinimal.h"
#include "YamlNode.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "YamlParsing.generated.h"


DECLARE_LOG_CATEGORY_EXTERN(LogYamlParsing, Log, All)


USTRUCT(Blueprintable)
struct FParseResult {
    GENERATED_BODY()
    
    /// Contains the Result of the parsing if it was successful
    UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
    FYamlNode Node;
    
    /// Non-empty if there occured an error during parsing
    UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
    FString ParseError;
    
    bool Success() const {
        return ParseError.IsEmpty();
    }
    
    operator bool() const {
        return ParseError.IsEmpty();
    }
};


UCLASS(BlueprintType)
class UNREALYAML_API UYamlParsing final : public UBlueprintFunctionLibrary {
    GENERATED_BODY()

public:
    /**
     * Parses a String into a YAML Node.
     *
     * @param String Then string that contains the YAML to parse
     * @returns A Struct with the resulting YAML Node or the Error if one occurred
     */
    UFUNCTION(BlueprintCallable, Category = "YAML")
    static FParseResult ParseYaml(const FString String);

    /**
     * Opens a File and Parses the Contents into a YAML Node.
     *
     * @param Path The path to the file to load
     * @returns A Struct with the resulting YAML Node or the Error if one occurred
     */
    UFUNCTION(BlueprintCallable, Category = "YAML")
    static FParseResult LoadYamlFromFile(const FString Path);

    /**
     * Writes the Contents of a YAML Node to a File.
     *
     * This will overwrite the existing File if it exists!
     * 
     * @param Path The File to write the Node to
     * @param Node The YAML to serialize and wrtie
     * @returns true if successful
     */
    UFUNCTION(BlueprintCallable, Category = "YAML")
    static void WriteYamlToFile(const FString Path, FYamlNode Node);
    
    /// Check whether the Parsing of the YAML was successful
    UFUNCTION(BlueprintCallable, Category = "YAML", meta = (ExpandBoolAsExecs))
    static bool ParseSuccessful(const FParseResult& ParseResult) {
        return ParseResult.Success();
    }
};