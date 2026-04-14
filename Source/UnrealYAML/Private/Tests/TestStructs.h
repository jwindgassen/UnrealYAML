// Copyright (c) 2021-2026, Forschungszentrum Jülich GmbH. All rights reserved.
// Licensed under the MIT License. See LICENSE file for details.

#pragma once

#include "TestStructs.generated.h"

// Simple:

USTRUCT()
struct FSimpleStruct {
    GENERATED_BODY()

    UPROPERTY()
    FString Str = "Hello, World";

    UPROPERTY()
    int32 Int = 42;

    UPROPERTY()
    bool Bool = true;

    UPROPERTY()
    TArray<int32> Arr = {3, 4, 5};

    UPROPERTY()
    TMap<FString, int32> Map = {{"A", 1}, {"B", 2}};
};


UCLASS()
class USimpleObject : public UObject {
    GENERATED_BODY()

public:
    UPROPERTY()
    FString Str = "Hello, World";

    UPROPERTY()
    int32 Int = 42;

    UPROPERTY()
    bool Bool = true;

    UPROPERTY()
    TArray<int32> Arr = {3, 4, 5};

    UPROPERTY()
    TMap<FString, int32> Map = {{"A", 1}, {"B", 2}};
};

// Nested:

USTRUCT()
struct FChildStruct {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FString> Strings;

    UPROPERTY()
    float Float{};
};

USTRUCT()
struct FNestedStruct {
    GENERATED_BODY()

    UPROPERTY()
    FChildStruct Inner;

    UPROPERTY()
    TArray<FChildStruct> ChildArray;

    UPROPERTY()
    TMap<FString, FChildStruct> ChildMap;
};

// Enums:

UENUM()
enum class EEnumClass : uint8 {
    Value1 = 42,
    Value2 = 43,
    Value3 = 44,
};

UENUM()
namespace ENamespaceEnum {
enum Type {
    Value1 = 45,
    Value2 = 46,
    Value3 = 47,
};
}

USTRUCT()
struct FEnumStruct {
    GENERATED_BODY()

    UPROPERTY()
    EEnumClass EnumValue = EEnumClass::Value1;

    UPROPERTY()
    TEnumAsByte<ENamespaceEnum::Type> EnumAsByte = ENamespaceEnum::Value1;
};


USTRUCT()
struct FStructWithVariousKeyNames {
    GENERATED_BODY()

    UPROPERTY()
    int32 AnInteger = 42;

    UPROPERTY()
    FString AnFString = "Foo";

    UPROPERTY()
    int32 notCamelCase = -1;

    UPROPERTY()
    int32 AnIntegerPlus2 = 44;

    UPROPERTY()
    int32 AnIntegerPlus42 = 84;

    UPROPERTY()
    FString CapitalA = "A";
};


USTRUCT()
struct FNativeTypesStruct {
    GENERATED_BODY()

    UPROPERTY()
    FTransform Transform{};

    UPROPERTY()
    FQuat Quat{};

    UPROPERTY()
    FRotator Rotator{};

    UPROPERTY()
    FVector Vector{};

    UPROPERTY()
    FVector2D Vector2D{};

    UPROPERTY()
    TSet<int> Set;

    UPROPERTY()
    FLinearColor LinearColor{};

    UPROPERTY()
    FColor Color{};

    UPROPERTY()
    FText Text;

    UPROPERTY()
    FName Name;
};


USTRUCT()
struct FReferencesStruct {
    GENERATED_BODY()

    UPROPERTY()
    TSubclassOf<AActor> Class;

    UPROPERTY()
    TSoftObjectPtr<UStaticMesh> MeshObject;
};


UCLASS()
class UChildObject : public UObject {
    GENERATED_BODY()

public:
    UPROPERTY()
    TArray<FString> Strings;

    UPROPERTY()
    float Float{};
};


USTRUCT()
struct FObjectTypes {
    GENERATED_BODY()

    UPROPERTY()
    TMap<FString, int32> Values;

    UPROPERTY()
    FChildStruct ChildStruct;

    UPROPERTY()
    UChildObject* ChildObject;
};


USTRUCT()
struct FRequiredFieldsStruct {
    GENERATED_BODY()

    UPROPERTY(meta = (YamlRequired))
    int Required = 42;

    UPROPERTY()
    int Optional = 43;
};


USTRUCT()
struct FCustomType {
    GENERATED_BODY()

    FString InnerValue;
};

USTRUCT()
struct FCustomTypeStruct {
    GENERATED_BODY()

    UPROPERTY()
    FCustomType CustomValue;
};
