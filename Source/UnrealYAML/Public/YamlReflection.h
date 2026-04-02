#pragma once

#include "CoreMinimal.h"
#include "YamlNode.h"
#include "YamlParsing.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include <typeinfo>

#include "YamlReflection.generated.h"

struct FYamlParseResult;

/**
 * Create some parsing logic for custom special types. UnrealYaml provides custom parsing logic for common
 * native Unreal Types, which can be further extended by a custom type handler.
 *
 * See the documentation for `FParseIntoOptions.TypeHandlers` and `FParseFromOptions.TypeHandlers` for more information.
 */
DECLARE_DELEGATE_FourParams(FCustomTypeHandler, const FYamlNode& /* Node */, const UScriptStruct* /* Struct */,
                            void* /* StructValue */, FYamlParseResult& /* Result */);


/**
 * Controls how the `ParseNodeIntoStruct` and `ParseNodeIntoObject` operations behave.
 *
 * Default options:
 *   - No strict type checking
 *   - Do not ensure matching enum values
 *   - Ignore `YamlRequired` metadata on a UPROPERTY
 *   - Ignore unparsed values in the YAML
 */
USTRUCT(Blueprintable)
struct UNREALYAML_API FYamlParseIntoOptions {
    GENERATED_BODY()

    /// Static factory for a set of options that enforces validity of the incoming YAML.
    static FYamlParseIntoOptions Strict();

    /**
     * Ensures that parsed types match what we expect (or can be converted to it).
     *
     * For example, with this false, if a struct expects a TArray, but the corresponding
     * YAML has a string value, parsing would still be successful. With this enabled,
     * the result will be indicated a failure and a suitable error message provided.
     */
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    bool StrictTypes = false;

    /**
     * Ensures that values encountered in YAML which map onto an enum property in a struct
     * are one of the allowed values.
     *
     * Checking is performed case-insensitively on the enum entry, e.g.
     *
     * enum Foo {
     *     AValue1 = 0,
     *     AValue2 = 1,
     * }
     *
     * "avalue1", "avalue2" (and case-variations of) would be the accepted values.
     */
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    bool CheckEnums = false;

    /**
     * Inspects fields' UPROPERTY meta for a "YamlRequired" specifier on the USTRUCT being parsed in to.
     * If present, and an incoming YAML structure is missing this property, a validation error will occur.
     */
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    bool RespectRequiredProperties = false;

    /**
     * If true, validation will fail if the YAML contains properties that do not match with
     * a property in the struct being parsed.
     */
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    bool AllowUnusedValues = true;

    /**
     * Define ParseInto handling for your own types here. By default, ParseIntoStruct will handle
     * some common Unreal types (see ParseIntoNativeType). Adding a Handler here allows defining
     * additional types that require some special handling.
     *
     * The key is the C++ type name, and the associated function is responsible for interpreting the
     * given node, constructing the custom type, then setting it given in StructValue. Any
     * errors encountered can be added to the Result with `FYamlParseResult::AddError`.
     *
     * Example:
     * ```
     * ParseOptions.TypeHandlers.Add(
     *     "FBox",
     *     [](const FYamlNode& Node, const UScriptStruct*, void* Value, struct FYamlParseResult& Result) {
     *         if (!Node.IsSequence() || Node.Size() != 2) {
     *             Result.AddError(TEXT("FBox must be a sequence of 2 Vectors"));
     *             return;
     *         }
     *
     *         FBox& Box = *static_cast<FBox*>(Value);
     *         Box.Min = Node[0].As<FVector>();
     *         Box.Max = Node[1].As<FVector>();
     *     }
     * );
     * ```
     */
    TMap<FString, FCustomTypeHandler> TypeHandlers;
};


/**
 * Controls how the `ParseNodeFromStruct` and `ParseNodeFromObject` operations behave.
 *
 * Default options:
 *   - No strict type checking
 *   - Do not ensure matching enum values
 *   - Ignore `YamlRequired` metadata on a UPROPERTY
 *   - Ignore unparsed values in the YAML
 */
USTRUCT(Blueprintable)
struct UNREALYAML_API FYamlParseFromOptions {
    GENERATED_BODY()
};


/**
 * Result of a parsing operation.
 *
 * Contains a list of errors that might have occured during the parsing. If no errors occured, the parsing
 * is considered successful.
 */
USTRUCT(Blueprintable)
struct FYamlParseResult {
    GENERATED_BODY()

    /// Errors that occurred whilst trying to parse YAML in to a struct.
    UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly)
    TArray<FString> Errors;

    /// Parsing was successful of there were no errors
    FORCEINLINE bool Success() const {
        return Errors.Num() == 0;
    }

    /// Parsing was successful of there were no errors
    FORCEINLINE explicit operator bool() const {
        return Success();
    }

    template<typename FmtType, typename... Types>
    void AddError(const FmtType& Fmt, Types... Args) {
        const FString ErrorMessage = FString::Printf(Fmt, Forward<Types>(Args)...);
        const FString WithScope = FString::Printf(TEXT("%s: %s"), *ScopeName(), *ErrorMessage);

        UE_LOG(LogYamlParsing, Error, TEXT("%s"), *WithScope)
        Errors.Add(WithScope);
    }

private:
    friend class UYamlReflection;

    /// Stack for the Scopes we entered while parsing
    TArray<FString> ScopesStack;

    /// Representation of the current Scope from the Stack
    FString ScopeName() const;

    // Stack manipulation
    void PushStack(const FString& Property);
    void PushStack(const FYamlNode& Key);
    void PushStack(const int32 Index);
    void PopStack();
};



/**
 * Function for interoperation with FYamlNode and Unreal's reflection system (UOBJECTS and USTRUCTS).
 *
 * Adds functionality for parsing an FYamlNode into a UObject/UStruct and vice versa. This BlueprintFunctionLibrary
 * contains the primary logic and bindings for calling from Blueprints. In C++, use the respective wrappers
 * (e.g. `ParseNodeIntoStruct` and `ParseNodeFromStruct`) defined at the end of this file.
 */
UCLASS()
class UNREALYAML_API UYamlReflection : public UBlueprintFunctionLibrary {
    GENERATED_BODY()

#pragma region C++ Wrapper

    template<typename ObjectType>
    friend FYamlParseResult ParseNodeIntoObject(const FYamlNode&, ObjectType*, const FYamlParseIntoOptions&);

    template<typename StructType>
    friend FYamlParseResult ParseNodeIntoStruct(const FYamlNode&, StructType&, const FYamlParseIntoOptions&);

    friend FYamlParseResult ParseNodeIntoStruct(const FYamlNode&, const UScriptStruct*, void*,
                                                const FYamlParseIntoOptions&);


    template<typename ObjectType>
    friend FYamlParseResult ParseNodeFromObject(FYamlNode&, const ObjectType*, const FYamlParseFromOptions&);

    template<typename StructType>
    friend FYamlParseResult ParseNodeFromStruct(FYamlNode&, const StructType&, const FYamlParseFromOptions&);

    friend FYamlParseResult ParseNodeFromStruct(FYamlNode&, const UScriptStruct*, const void*,
                                                const FYamlParseFromOptions&);

#pragma endregion


    /**
     * Some conversion are defined in UnrealTypes.h. But FProperty does not provide a way to retrieve the Type,
     * except as an FString. This provides a list of all Types (as String) that can be directly converted via
     * those conversion instead of the Parsing all Fields further (basically a shortcut with neater results).
     *
     * This is only used to determine *if* an FProperty can be converted directly, the if-else chain to check
     * what type it actually is, is in ParseIntoNativeType itself.
     */
    const static TArray<FString> NativeTypes;

    /**
     * UPROPERTY(meta=YamlRequired) indicates a USTRUCT's property is required when parsing in to it
     * from a YAML node. This is the string we look for.
     */
    inline const static FName YamlRequiredSpecifier{TEXT("YamlRequired")};

public:
    /// Static factory for a set of options that enforces validity of the incoming YAML.
    UFUNCTION(BlueprintPure, meta = (NativeMakeFunc))
    static FYamlParseIntoOptions StrictParseIntoOptions() {
        return FYamlParseIntoOptions::Strict();
    }

    /// Parsing was successful of there were no errors
    UFUNCTION(BlueprintPure, meta = (ExpandBoolAsExecs))
    static bool ParsingSuccessful(const FYamlParseResult& Result) {
        return !!Result;
    }


    /**
     * Parses the given Node into the instance of the given Object. The function will recursively iterate over
     * all Properties in the Object and try to parse the contents of the corresponding Node entry into
     * the field of the Object.
     *
     * If you use a C++ UOBJECT, all Properties that you want parsed must be a UPROPERTY for the UHT
     * to register them. For easier C++ access, use the global ParseNodeIntoStruct function.
     *
     * @param Node The Node that contains the Data
     * @param Object The Object that should receive the data from the Node
     * @param Options Controls the behavior of the Parser
     * @return The Result of the parsing operation
     */
    UFUNCTION(BlueprintCallable, DisplayName = "Parse Node into Object", Category = "YAML")
    static FYamlParseResult ParseNodeIntoObject_BP(const FYamlNode& Node, UObject* Object,
                                                   const FYamlParseIntoOptions& Options);

    /**
     * Parses the given Node into the instance of the given Struct. The function will recursively iterate over
     * all Properties in the Struct and try to parse the contents of the corresponding Node entry into
     * the field of the Struct.
     *
     * If you use a C++ USTRUCT, all Properties that you want parsed must be a UPROPERTY for the UHT
     * to register them. For easier C++ access, use the global ParseNodeIntoStruct function.
     *
     * @param Node The Node that contains the Data
     * @param Struct The Struct that should receive the data from the Node
     * @param Options Controls the behavior of the Parser
     * @return The Result of the parsing operation
     */
    UFUNCTION(BlueprintCallable, CustomThunk, DisplayName = "Parse Node into Struct", Category = "YAML",
              meta = (CustomStructureParam = "Struct"))
    static FYamlParseResult ParseNodeIntoStruct_BP(const FYamlNode& Node, const int32& Struct,
                                                   const FYamlParseIntoOptions& Options) {
        checkNoEntry();
        return {};
    }

    // Custom Thunk for ParseIntoStruct_BP
    DECLARE_FUNCTION(execParseNodeIntoStruct_BP);

private:
#pragma region Parse Node into Property

    // Parses a Node into a Property. Can be a FStructProperty itself (recursion!)
    static void ParseIntoProperty(const FYamlNode& Node, const FProperty& Property, void* PropertyValue,
                                  const FYamlParseIntoOptions& Options, FYamlParseResult& Result);

    // Parses a Node into a Struct. Calls ParseIntoProperty on all Fields
    static void ParseIntoStruct(const FYamlNode& Node, const UScriptStruct* Struct, void* StructValue,
                                const FYamlParseIntoOptions& Options, FYamlParseResult& Result);

    // Parses a Node into a UObject. Calls ParseIntoProperty on all Fields
    static void ParseIntoObject(const FYamlNode& Node, const UClass* Object, void* ObjectValue,
                                const FYamlParseIntoOptions& Options, FYamlParseResult& Result);

    // Parses a Node via some predefined conversions in UnrealTypes.h (see NativeTypes).
    static void ParseIntoNativeType(const FYamlNode& Node, const UScriptStruct* Struct, void* StructValue);


    // Prints error and returns false if the type of the node does not match the type we expect
    static bool EnsureNodeType(const FYamlNode& Node, const EYamlNodeType Expected, const bool Strict,
                               FYamlParseResult& Result);

    // Prints error and returns false if the Node is not a scalar or cannot be converted to the given type
    template<typename ValueType>
    static bool EnsureScalarOfType(const FYamlNode& Node, const FString& TypeName, const bool Strict,
                                   FYamlParseResult& Result) {
        if (!EnsureNodeType(Node, EYamlNodeType::Scalar, Strict, Result)) {
            return false;
        }

        if (Strict && Node.IsDefined() && !Node.CanConvertTo<ValueType>()) {
            Result.AddError(TEXT("Cannot convert \"%s\" to a %s"), *Node.Scalar(), *TypeName);
            return false;
        }

        return true;
    }

    // Parses the Node it into the property if it can be converted correctly
    template<typename ValueType, typename PropertyType>
    static void ParseScalarValue(const FYamlNode& Node, const PropertyType* Property, void* PropertyValue,
                                 const FString& TypeName, const bool Strict, FYamlParseResult& Result) {
        if (!EnsureScalarOfType<ValueType>(Node, TypeName, Strict, Result)) {
            return;
        }

        const auto Value = Node.AsOptional<ValueType>();
        if (Value.IsSet()) {
            Property->SetPropertyValue(PropertyValue, *Value);
        }
    }

    /**
     * Helper for trying to parse the value of an enum from an Integer or String to the corresponding
     * index of the UENUM. This will try to:
     *   (a) Parse the Node to an Integer, potentially doing a bounds check
     *   (b) Parse the Node as a String and checking if the enum has a Value with that name
     */
    static int64 ParseEnumValue(const FYamlNode& Node, const UEnum* Enum, const bool CheckEnums,
                                FYamlParseResult& Result);

#pragma endregion

    // Parses a Property into a Node. Can be a FStructProperty itself (recursion!)
    static void ParseFromProperty(FYamlNode& Node, const FProperty& Property, const void* PropertyValue,
                                  const FYamlParseFromOptions& Options, FYamlParseResult& Result);

    // Parses Struct into a Node. Calls ParseIntoProperty on all Fields
    static void ParseFromStruct(FYamlNode& Node, const UScriptStruct* Struct, const void* StructValue,
                                const FYamlParseFromOptions& Options, FYamlParseResult& Result);

    // Parses UObject into a Node. Calls ParseIntoProperty on all Fields
    static void ParseFromObject(FYamlNode& Node, const UClass* Object, const void* ObjectValue,
                                const FYamlParseFromOptions& Options, FYamlParseResult& Result);

    // Parses some predefined conversions in UnrealTypes.h into a Node (see NativeTypes).
    static void ParseFromNativeType(FYamlNode& Node, const UScriptStruct* Struct, const void* StructValue,
                                    const FYamlParseFromOptions& Options, FYamlParseResult& Result);
};



/**
 * Parses the given Node into the instance of the given Object. The function will recursively iterate over all
 * Properties in the Object and try to parse the contents of the corresponding Node entry into the field of the Object.
 *
 * @tparam ObjectType The Type of Object we are parsing. Must be a UObject
 * @param Node The Node that contains the Data
 * @param Object The Object that should receive the data from the Node
 * @param Options Controls the behavior of the Parser
 * @return The Result of the parsing operation
 */
template<typename ObjectType>
FORCEINLINE FYamlParseResult ParseNodeIntoObject(const FYamlNode& Node, ObjectType* Object,
                                                 const FYamlParseIntoOptions& Options = {}) {
    static_assert(TIsDerivedFrom<ObjectType, UObject>::Value);

    FYamlParseResult Result;
    UYamlReflection::ParseIntoObject(Node, ObjectType::StaticClass(), Object, Options, Result);
    return Result;
}

/**
 * Parses the given Node into the instance of the given Struct. The function will recursively iterate over all
 * Properties in the Struct and try to parse the contents of the corresponding Node entry into the field of the Struct.
 *
 * @tparam StructType The Type of Struct we are parsing. Must be a UStruct
 * @param Node The Node that contains the Data
 * @param Struct The Struct that should receive the data from the Node
 * @param Options Controls the behavior of the Parser
 * @return The Result of the parsing operation
 */
template<typename StructType>
FORCEINLINE FYamlParseResult ParseNodeIntoStruct(const FYamlNode& Node, StructType& Struct,
                                                 const FYamlParseIntoOptions& Options = {}) {
    FYamlParseResult Result;
    UYamlReflection::ParseIntoStruct(Node, Struct.StaticStruct(), &Struct, Options, Result);
    return Result;
}

/**
 * Parses the given Node into the instance of the given Struct. The function will recursively iterate over all
 * Properties in the Struct and try to parse the contents of the corresponding Node entry into the field of the Struct.
 *
 * @param Node The Node that contains the Data
 * @param Struct The Struct class that describes the Struct
 * @param StructValue The pointer to the actual memory location of the Struct
 * @param Options Controls the behavior of the Parser
 * @return The Result of the parsing operation
 */
FORCEINLINE FYamlParseResult ParseNodeIntoStruct(const FYamlNode& Node, const UScriptStruct* Struct, void* StructValue,
                                                 const FYamlParseIntoOptions& Options = {}) {
    FYamlParseResult Result;
    UYamlReflection::ParseIntoStruct(Node, Struct, StructValue, Options, Result);
    return Result;
}


/**
 * Fills the given Node with the data in the given Object. The function will recursively iterate over all Properties
 * in the Object and try to parse the contents in the fields of the Object into an entry in the Node.
 *
 * @tparam ObjectType The Type of Object we are parsing. Must be a UObject
 * @param Node The Node that will receive the data
 * @param Object The Object that contains the data
 * @param Options Controls the behavior of the Parser
 * @return The Result of the parsing operation
 */
template<typename ObjectType>
FORCEINLINE FYamlParseResult ParseNodeFromObject(FYamlNode& Node, const ObjectType* Object,
                                                 const FYamlParseFromOptions& Options = {}) {
    static_assert(TIsDerivedFrom<ObjectType, UObject>::Value);

    FYamlParseResult Result;
    UYamlReflection::ParseFromObject(Node, ObjectType::StaticClass(), Object, Options, Result);
    return Result;
}

/**
 * Fills the given Node with the data in the given Struct. The function will recursively iterate over all Properties
 * in the Struct and try to parse the contents in the fields of the Struct into an entry in the Node.
 *
 * @tparam StructType The Type of Struct we are parsing. Must be a UStruct
 * @param Node The Node that will receive the data
 * @param Struct The Struct that contains the data
 * @param Options Controls the behavior of the Parser
 * @return The Result of the parsing operation
 */
template<typename StructType>
FORCEINLINE FYamlParseResult ParseNodeFromStruct(FYamlNode& Node, const StructType& Struct,
                                                 const FYamlParseFromOptions& Options = {}) {
    FYamlParseResult Result;
    UYamlReflection::ParseFromStruct(Node, Struct.StaticStruct(), &Struct, Options, Result);
    return Result;
}

/**
 * Fills the given Node with the data in the given Struct. The function will recursively iterate over all Properties
 * in the Struct and try to parse the contents in the fields of the Struct into an entry in the Node.
 *
 * @param Node The Node that will receive the data
 * @param Struct The Struct class that describes the Struct
 * @param StructValue The pointer to the actual memory location of the Struct
 * @param Options Controls the behavior of the Parser
 * @return The Result of the parsing operation
 */
FORCEINLINE FYamlParseResult ParseNodeFromStruct(FYamlNode& Node, const UScriptStruct* Struct, const void* StructValue,
                                                 const FYamlParseFromOptions& Options = {}) {
    FYamlParseResult Result;
    UYamlReflection::ParseFromStruct(Node, Struct, StructValue, Options, Result);
    return Result;
}
