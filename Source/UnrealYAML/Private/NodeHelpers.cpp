#include "NodeHelpers.h"

#include "node/parse.h"
#include "HAL/FileManagerGeneric.h"

// At least here we can use macros :)
#define DEFINE_YAML_CONVERSIONS(Type, FancyName) \
    FYamlNode UYamlNodeHelpers::MakeFrom##FancyName(Type Value) { \
        return FYamlNode(Value); \
    } \
    FYamlNode UYamlNodeHelpers::MakeFrom##FancyName##Array(const TArray<Type>& Value) { \
        return FYamlNode(Value); \
    } \
    FYamlNode UYamlNodeHelpers::MakeFromInt##FancyName##Map(const TMap<int32, Type>& Value) { \
        return FYamlNode(Value); \
    } \
    FYamlNode UYamlNodeHelpers::MakeFromString##FancyName##Map(const TMap<FString, Type>& Value) { \
        return FYamlNode(Value); \
    } \
    bool UYamlNodeHelpers::As##FancyName(const FYamlNode& Node, Type Default, Type& Value) { \
        const auto Out = Node.AsOptional<Type>(); \
        const bool Success = Out.IsSet(); \
        Value = Out.Get(Default); \
        return Success; \
    } \
    bool UYamlNodeHelpers::As##FancyName##Array(const FYamlNode& Node, const TArray<Type>& Default, TArray<Type>& Value) { \
        const auto Out = Node.AsOptional<TArray<Type>>(); \
        const bool Success = Out.IsSet(); \
        Value = Out.Get(Default); \
        return Success; \
    } \
    bool UYamlNodeHelpers::AsInt##FancyName##Map(const FYamlNode& Node, const TMap<int32, Type>& Default, TMap<int32, Type>& Value) { \
        const auto Out = Node.AsOptional<TMap<int32, Type>>(); \
        const bool Success = Out.IsSet(); \
        Value = Out.Get(Default); \
        return Success; \
    } \
    bool UYamlNodeHelpers::AsString##FancyName##Map(const FYamlNode& Node, const TMap<FString, Type>& Default, TMap<FString, Type>& Value) { \
        const auto Out = Node.AsOptional<TMap<FString, Type>>(); \
        const bool Success = Out.IsSet(); \
        Value = Out.Get(Default); \
        return Success; \
    }

DEFINE_YAML_CONVERSIONS(int32, Int)
DEFINE_YAML_CONVERSIONS(int64, Long)
DEFINE_YAML_CONVERSIONS(uint8, Byte)
DEFINE_YAML_CONVERSIONS(float, Float)
DEFINE_YAML_CONVERSIONS(bool, Bool)
DEFINE_YAML_CONVERSIONS(FString, String)
DEFINE_YAML_CONVERSIONS(FName, Name)
DEFINE_YAML_CONVERSIONS(FText, Text)
DEFINE_YAML_CONVERSIONS(FVector, Vector)
DEFINE_YAML_CONVERSIONS(FQuat, Quat)
DEFINE_YAML_CONVERSIONS(FTransform, Transform)

#undef DEFINE_YAML_CONVERSIONS
