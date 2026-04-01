#pragma once

#include "TestStructs.generated.h"


// SimpleYaml
USTRUCT()
struct FSimpleStruct {
    GENERATED_BODY()

    UPROPERTY()
    FString Str;

    UPROPERTY()
    int32 Int{};

    UPROPERTY()
    bool Bool{};

    UPROPERTY()
    TArray<int32> Arr;

    UPROPERTY()
    TMap<FString, int32> Map;
};

// SimpleYaml
USTRUCT()
struct FDefaultedStruct {
    GENERATED_BODY()

    UPROPERTY()
    FString Str = TEXT("a string");

    UPROPERTY()
    int32 Int = 1;

    UPROPERTY()
    bool Bool = true;

    UPROPERTY()
    TArray<int32> Arr = {1, 2, 3};

    UPROPERTY()
    TMap<FString, int32> Map = {{"foo", 13}};
};

UENUM()
enum EAnEnum {
    Value1 = 0,
    Value2 = 1,
    Value3 = 2,
};

UENUM()
enum class EAnEnumClass : uint8 {
    Value1 = 0,
    Value2 = 1,
    Value3 = 2,
};

USTRUCT()
struct FChildStruct {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FString> SomeValues;

    UPROPERTY()
    float AFloat;

    UPROPERTY()
    TEnumAsByte<EAnEnum> AnEnum = EAnEnum::Value3;
};

USTRUCT()
struct FParentStruct {
    GENERATED_BODY()

    UPROPERTY()
    FChildStruct Embedded;

    UPROPERTY()
    TArray<FChildStruct> Children;

    UPROPERTY()
    TMap<TEnumAsByte<EAnEnum>, FChildStruct> MappedChildren;
};

USTRUCT()
struct FEnumAsByteStruct {
    GENERATED_BODY()

    UPROPERTY()
    TEnumAsByte<EAnEnum> AnEnum;
};

USTRUCT()
struct FEnumStruct {
    GENERATED_BODY()

    UPROPERTY()
    EAnEnumClass AnEnum;
};

USTRUCT()
struct FDefaultStruct {
    GENERATED_BODY()

    UPROPERTY()
    int AnInt = 13;

    UPROPERTY()
    float AFloat = 13.24;

    UPROPERTY()
    FString AString = "Hello world!";

    UPROPERTY()
    TEnumAsByte<EAnEnum> AnEnum = EAnEnum::Value3;

    UPROPERTY()
    TMap<FString, FString> AMap = {
        {"one", "1"},
        {"two", "2"},
        {"three", "3"},
    };

    UPROPERTY()
    TArray<EAnEnumClass> AnArray = {EAnEnumClass::Value1, EAnEnumClass::Value2};
};

UCLASS()
class USimpleObject : public UObject{
    GENERATED_BODY()

public:
    UPROPERTY()
    FString Str;

    UPROPERTY()
    int32 Int{};

    UPROPERTY()
    bool Bool{};

    UPROPERTY()
    TArray<int32> Arr;

    UPROPERTY()
    TMap<FString, int32> Map;
};

USTRUCT()
struct FUnrealTypeStruct {
    GENERATED_BODY()

    UPROPERTY()
    FTransform Transform;

    UPROPERTY()
    FQuat Quat;

    UPROPERTY()
    FRotator Rotator;

    UPROPERTY()
    FVector Vector;

    UPROPERTY()
    FVector2D Vector2D;

    UPROPERTY()
    TSet<int> Set;

    UPROPERTY()
    FLinearColor LinearColor;

    UPROPERTY()
    FColor Color;

    UPROPERTY()
    FText Text;

    UPROPERTY()
    FName Name;
};

USTRUCT()
struct FUnrealReferenceTypeStruct {
    GENERATED_BODY()

    UPROPERTY()
    TSubclassOf<AActor> SubclassOf;

    UPROPERTY()
    TSoftObjectPtr<UStaticMesh> SoftObjectPtr;
};

USTRUCT()
struct FRequiredFieldsStruct {
    GENERATED_BODY()

    UPROPERTY(meta=(YamlRequired))
    int Required;

    UPROPERTY()
    int Optional;
};

USTRUCT()
struct FCustomType {
    GENERATED_BODY()

    FString Value;
};

USTRUCT()
struct FWithCustomType {
    GENERATED_BODY()

    UPROPERTY()
    FCustomType CustomType;
};

// Cannot test for complex yaml, as we can't represent mixed nested types :(
