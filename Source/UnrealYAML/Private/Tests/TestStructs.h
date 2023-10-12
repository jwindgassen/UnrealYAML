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

// Cannot test for complex yaml, as we can't represent mixed nested types :(
