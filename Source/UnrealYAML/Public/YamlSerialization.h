#pragma once

#include "CoreMinimal.h"
#include "YamlNode.h"
#include "YamlParsing.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "YamlSerialization.generated.h"

struct FYamlSerializationResult;

/**
 * Create some parsing logic for custom special types. UnrealYaml provides custom parsing logic for common
 * native Unreal Types, which can be further extended by a custom type handler.
 *
 * See the documentation for `FYamlDeserializeOptions.TypeHandlers` and `FYamlSerializeOptions.TypeHandlers`.
 *
 * ToDo: Should this be an FProperty and checked in `(De)SerializeNativeType` instead?
 */
DECLARE_DELEGATE_RetVal_ThreeParams(FYamlNode, FCustomTypeSerializer, const UScriptStruct* /* Struct */,
                                    const void* /* StructValue */, FYamlSerializationResult& /* Result */);

DECLARE_DELEGATE_FourParams(FCustomTypeDeserializer, const FYamlNode& /* Node */, const UScriptStruct* /* Struct */,
                            void* /* StructValue */, FYamlSerializationResult& /* Result */);


UENUM()
enum class EYamlKeyCapitalization : uint8 {
    /// MyValue
    PascalCase,

    /// myValue
    CamelCase,

    /// my_value
    SnakeCase
};


/// Controls how the `SerializeStruct` and `SerializeObject` operations behave.
USTRUCT(Blueprintable)
struct UNREALYAML_API FYamlSerializeOptions {
    GENERATED_BODY()

    /**
     * How the keys of map-like objects (classes, structs, TMaps) should be capitalized.
     */
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    EYamlKeyCapitalization Capitalization = EYamlKeyCapitalization::CamelCase;

    /**
     * Whether to include type information in the form of YAML tags when serializing. This can
     * be useful when you need to decide whether a YAML Object used to be a Struct, a Class or a TMap.
     *
     * Example:
     * ```
     * mapValue: !<TMap>
     *   a: 1
     *   b: 2
     * childStruct: !<FChildStruct>
     *   intValue: 42
     * childObject: !<UMyObject>
     *   stringValue: anotherValue
     * ```
     */
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    bool IncludeTypeInformation = false;

    /**
     * How enum values should be serialized. When false (default), enums will be serialized as
     * a string with the name of the option. When true, enum values will instead be represented
     * by the integer of the underlying type.
     *
     * Bitfield integer will always be serialized as integers!
     */
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    bool EnumAsNumber = false;

    /**
     * Define Serialization for your own types here. By default, SerializeStruct will handle
     * some common Unreal types (see SerializeNativeType). Adding a Handler here allows defining
     * additional types that require some special handling.
     *
     * The key is the C++ type name, and the associated function is responsible for getting a value
     * out of the provided field and writing it into the Node. Any errors encountered can
     * be added to the Result with `FYamlSerializationResult::AddError`.
     *
     * Example:
     * ```
     * SerializeOptions.TypeHandlers.Add(
     *     "FBox",
     *     [](FYamlNode& Node, const UScriptStruct*, const void* Value, FYamlSerializationResult& Result) {
     *         FBox& Box = *static_cast<FBox*>(Value);
     *
     *         Node[0] = Box.Min;
     *         Node[1] = Box.Max;
     *     }
     * );
     * ```
     */
    TMap<FString, FCustomTypeSerializer> TypeHandlers;
};


/**
 * Controls how the `DeserializeStruct` and `DeserializeObject` operations behave.
 *
 * Default options:
 *   - No strict type checking
 *   - Do not ensure matching enum values
 *   - Ignore `YamlRequired` metadata on a UPROPERTY
 *   - Ignore unparsed values in the YAML
 */
USTRUCT(Blueprintable)
struct UNREALYAML_API FYamlDeserializeOptions {
    GENERATED_BODY()

    /// Static factory for a set of options that enforces validity of the incoming YAML.
    static FYamlDeserializeOptions Strict();

    /**
     * Ensures that deserialized types are checked before we try to parse them into a value.
     *
     * When false, an entry in the Node that cannot be converted to the desired deserialized type (e.g., an Integer)
     * will just be ignored and skipped during deserialization.
     *
     * When true, deserialization will check both if the format is correct (e.g., only allowing Sequences for Arrays)
     * and if the actual value can correctly be converted.
     * If this is not possible, a corresponding error will be generated.
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
     * Inspects metadata of UPROPERTY fields for a "YamlRequired" specifier.
     * If present and if the incoming YAML Node is missing this property, a validation error will occur.
     */
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    bool RespectRequiredProperties = false;

    /**
     * If true, validation will fail if the YAML Node contains properties that do not match with
     * a property in the struct being deserialized or if not all keys of a Map are deserialized.
     */
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    bool AllowUnusedValues = true;

    /**
     * Define Deserialization for your own types here. By default, DeserializeStruct will handle
     * some common Unreal types (see DeserializeNativeType). Adding a Handler here allows defining
     * additional types that require some special handling.
     *
     * The key is the C++ type name, and the associated function is responsible for interpreting the
     * given node, constructing the custom type, then setting it given in StructValue. Any
     * errors encountered can be added to the Result with `FYamlSerializationResult::AddError`.
     *
     * Example:
     * ```
     * DeserializeOptions.TypeHandlers.Add(
     *     "FBox",
     *     [](const FYamlNode& Node, const UScriptStruct*, void* Value, FYamlSerializationResult& Result) {
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
    TMap<FString, FCustomTypeDeserializer> TypeHandlers;
};


/**
 * Result of a parsing operation.
 *
 * Contains a list of errors that might have occured during the parsing. If no errors occured, the parsing
 * is considered successful.
 */
USTRUCT(Blueprintable)
struct FYamlSerializationResult {
    GENERATED_BODY()

    /// Errors that occurred during (de)serialization
    UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly)
    TArray<FString> Errors;

    /// (De)serialization was successful of there were no errors
    FORCEINLINE bool Success() const {
        return Errors.Num() == 0;
    }

    /// (De)serialization was successful of there were no errors
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
    friend class UYamlSerialization;

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
 * (e.g. `DeserializeStruct` and `SerializeStruct`) defined at the end of this file.
 */
UCLASS()
class UNREALYAML_API UYamlSerialization : public UBlueprintFunctionLibrary {
    GENERATED_BODY()

#pragma region C++ Wrapper

    template<typename ObjectType>
    friend FYamlSerializationResult SerializeObject(FYamlNode&, const ObjectType*, const FYamlSerializeOptions&);

    template<typename StructType>
    friend FYamlSerializationResult SerializeStruct(FYamlNode&, const StructType&, const FYamlSerializeOptions&);

    friend FYamlSerializationResult SerializeStruct(FYamlNode&, const UScriptStruct*, const void*,
                                                    const FYamlSerializeOptions&);


    template<typename ObjectType>
    friend FYamlSerializationResult DeserializeObject(const FYamlNode&, ObjectType*, const FYamlDeserializeOptions&);

    template<typename StructType>
    friend FYamlSerializationResult DeserializeStruct(const FYamlNode&, StructType&, const FYamlDeserializeOptions&);

    friend FYamlSerializationResult DeserializeStruct(const FYamlNode&, const UScriptStruct*, void*,
                                                      const FYamlDeserializeOptions&);

#pragma endregion


    /**
     * Some conversion are defined in UnrealTypes.h. But FProperty does not provide a way to retrieve the Type,
     * except as an FString. This provides a list of all Types (as String) that can be directly converted via
     * those conversion instead of the Parsing all Fields further (basically a shortcut with neater results).
     *
     * This is only used to determine *if* an FProperty can be converted directly, the if-else chain to check
     * what type it actually is, is in DeserializeNativeType itself.
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
    static FYamlDeserializeOptions StrictDeserializeOptions() {
        return FYamlDeserializeOptions::Strict();
    }

    /// Parsing was successful of there were no errors
    UFUNCTION(BlueprintPure, meta = (ExpandBoolAsExecs))
    static bool SerializationSuccessful(const FYamlSerializationResult& Result) {
        return Result.Success();
    }


    /**
     * Serializes the data from the given Object into a Node. The function will recursively iterate over all Properties
     * in the Object and try to parse the contents in the fields of the Object into an entry in the Node.
     *
     * If you use a C++ UCLASS, all Properties that you want deserialized must be a UPROPERTY for the UHT
     * to register them. For easier C++ access, use the global `SerializeObject` function.
     *
     * @param Node The Node that will receive the Data from the Object
     * @param Object The Object that should be serialized
     * @param Options
     * @return The Result of the serialization
     */
    UFUNCTION(BlueprintCallable, DisplayName = "Serialize Object", Category = "YAML")
    static FYamlSerializationResult SerializeObject_BP(FYamlNode& Node, const UObject* Object,
                                                       const FYamlSerializeOptions& Options);


    /**
     * Serializes the data from the given Struct into a Node. The function will recursively iterate over all Properties
     * in the Struct and try to parse the contents in the fields of the Struct into an entry in the Node.
     *
     * If you use a C++ USTRUCT, all Properties that you want deserialized must be a UPROPERTY for the UHT
     * to register them. For easier C++ access, use the global `SerializeStruct` function.
     *
     * @param Node The Node that will receive the Data from the Object
     * @param Struct The Struct that should be serialized
     * @param Options
     * @return The Result of the serialization
     */
    UFUNCTION(BlueprintCallable, CustomThunk, DisplayName = "Serialize Struct", Category = "YAML",
              meta = (CustomStructureParam = "Struct"))
    static FYamlSerializationResult SerializeStruct_BP(FYamlNode& Node, const int32& Struct,
                                                       const FYamlSerializeOptions& Options) {
        checkNoEntry();
        return {};
    }

    // Custom Thunk for SerializeStruct_BP
    DECLARE_FUNCTION(execSerializeStruct_BP);


    /**
     * Deserializes the given Node into the instance of the given Object. The function will recursively iterate over
     * all Properties in the Object and try to parse the contents of the corresponding Node entry into
     * the field of the Object.
     *
     * If you use a C++ UOBJECT, all Properties that you want deserialized must be a UPROPERTY for the UHT
     * to register them. For easier C++ access, use the global `DeserializeStruct` function.
     *
     * @param Node The Node that contains the Data
     * @param Object The Object that should receive the data from the Node
     * @param Options Controls the behavior of the deserialization
     * @return The Result of the parsing operation
     */
    UFUNCTION(BlueprintCallable, DisplayName = "Deserialize Object", Category = "YAML")
    static FYamlSerializationResult DeserializeObject_BP(const FYamlNode& Node, UObject* Object,
                                                         const FYamlDeserializeOptions& Options);

    /**
     * Deserializes the given Node into the instance of the given Struct. The function will recursively iterate over
     * all Properties in the Struct and try to parse the contents of the corresponding Node entry into
     * the field of the Struct.
     *
     * If you use a C++ USTRUCT, all Properties that you want deserialized must be a UPROPERTY for the UHT
     * to register them. For easier C++ access, use the global `DeserializeStruct` function.
     *
     * @param Node The Node that contains the Data
     * @param Struct The Struct that should receive the data from the Node
     * @param Options Controls the behavior of the deserialization
     * @return The Result of the parsing operation
     */
    UFUNCTION(BlueprintCallable, CustomThunk, DisplayName = "Deserialize Struct", Category = "YAML",
              meta = (CustomStructureParam = "Struct"))
    static FYamlSerializationResult DeserializeStruct_BP(const FYamlNode& Node, const int32& Struct,
                                                         const FYamlDeserializeOptions& Options) {
        checkNoEntry();
        return {};
    }

    // Custom Thunk for DeserializeStruct_BP
    DECLARE_FUNCTION(execDeserializeStruct_BP);

private:
#pragma region Serialization
    // Serializes a Property into a Node. Can be a FStructProperty itself (recursion!)
    static FYamlNode SerializeProperty(const FProperty& Property, const void* PropertyValue,
                                       const FYamlSerializeOptions& Options, FYamlSerializationResult& Result);

    // Serializes a Struct into a Node. Calls DeserializeProperty on all Fields
    static FYamlNode SerializeStruct(const UScriptStruct* Struct, const void* StructValue,
                                     const FYamlSerializeOptions& Options, FYamlSerializationResult& Result);

    // Serializes an UObject into a Node. Calls DeserializeProperty on all Fields
    static FYamlNode SerializeObject(const UClass* Object, const void* ObjectValue,
                                     const FYamlSerializeOptions& Options, FYamlSerializationResult& Result);

    // Serializes some predefined conversions in UnrealTypes.h into a Node (see NativeTypes).
    static FYamlNode SerializeNativeType(const UScriptStruct* Struct, const void* StructValue);

    static void CapitalizePropertyName(FString& Name, EYamlKeyCapitalization Capitalization);
#pragma endregion

#pragma region Deserialization

    // Deserializes a Property from a Node. Can be a FStructProperty itself (recursion!)
    static void DeserializeProperty(const FYamlNode& Node, const FProperty& Property, void* PropertyValue,
                                    const FYamlDeserializeOptions& Options, FYamlSerializationResult& Result);

    // Deserializes a Struct from a Node. Calls DeserializeProperty on all Fields
    static void DeserializeStruct(const FYamlNode& Node, const UScriptStruct* Struct, void* StructValue,
                                  const FYamlDeserializeOptions& Options, FYamlSerializationResult& Result);

    // Deserializes an UObject from a Node. Calls DeserializeProperty on all Fields
    static void DeserializeObject(const FYamlNode& Node, const UClass* Object, void* ObjectValue,
                                  const FYamlDeserializeOptions& Options, FYamlSerializationResult& Result);

    // Deserializes some predefined conversions in UnrealTypes.h (see NativeTypes).
    static void DeserializeNativeType(const FYamlNode& Node, const UScriptStruct* Struct, void* StructValue);


    // Prints error and returns false if the type of the node does not match the type we expect
    static bool EnsureNodeType(const FYamlNode& Node, const EYamlNodeType Expected, const bool Strict,
                               FYamlSerializationResult& Result);

    // Prints error and returns false if the Node is not a scalar or cannot be converted to the given type
    template<typename ValueType>
    static bool EnsureScalarOfType(const FYamlNode& Node, const FString& TypeName, const bool Strict,
                                   FYamlSerializationResult& Result) {
        if (!EnsureNodeType(Node, EYamlNodeType::Scalar, Strict, Result)) {
            return false;
        }

        if (Strict && Node.IsDefined() && !Node.CanConvertTo<ValueType>()) {
            Result.AddError(TEXT("Cannot convert \"%s\" to a %s"), *Node.Scalar(), *TypeName);
            return false;
        }

        return true;
    }

    // Deserializes the Node it into the property if it can be converted correctly
    template<typename ValueType, typename PropertyType>
    static void DeserializeScalarValue(const FYamlNode& Node, const PropertyType* Property, void* PropertyValue,
                                       const FString& TypeName, const bool Strict, FYamlSerializationResult& Result) {
        if (!EnsureScalarOfType<ValueType>(Node, TypeName, Strict, Result)) {
            return;
        }

        const auto Value = Node.AsOptional<ValueType>();
        if (Value.IsSet()) {
            Property->SetPropertyValue(PropertyValue, *Value);
        }
    }

    /**
     * Helper for trying to deserialize the value of an enum from an Integer or String to the corresponding
     * index of the UENUM. This will try to deserialize the Node:
     *   (a) to an Integer, potentially doing a bounds check
     *   (b) as a String and checking if the enum has a Value with that name
     */
    static int64 DeserializeEnumValue(const FYamlNode& Node, const UEnum* Enum, const bool CheckEnums,
                                      FYamlSerializationResult& Result);

#pragma endregion
};


/**
 * Serializes the data from the given Object into a Node. The function will recursively iterate over all Properties
 * in the Object and try to parse the contents in the fields of the Object into an entry in the Node.
 *
 * @tparam ObjectType The Type of Object we are parsing. Must be a UObject
 * @param Node The Node that will receive the data
 * @param Object The Object that contains the data
 * @param Options Controls the behavior of the serialization
 * @return The Result of the parsing operation
 */
template<typename ObjectType>
FORCEINLINE FYamlSerializationResult SerializeObject(FYamlNode& Node, const ObjectType* Object,
                                                     const FYamlSerializeOptions& Options = {}) {
    static_assert(TIsDerivedFrom<ObjectType, UObject>::Value);

    FYamlSerializationResult Result;
    Node = UYamlSerialization::SerializeObject(ObjectType::StaticClass(), Object, Options, Result);
    return Result;
}

/**
 * Serializes the data from the given Struct into a Node. The function will recursively iterate over all Properties
 * in the Struct and try to parse the contents in the fields of the Struct into an entry in the Node.
 *
 * @tparam StructType The Type of Struct we are parsing. Must be a UStruct
 * @param Node The Node that will receive the data
 * @param Struct The Struct that contains the data
 * @param Options Controls the behavior of the serialization
 * @return The Result of the parsing operation
 */
template<typename StructType>
FORCEINLINE FYamlSerializationResult SerializeStruct(FYamlNode& Node, const StructType& Struct,
                                                     const FYamlSerializeOptions& Options = {}) {
    FYamlSerializationResult Result;
    Node = UYamlSerialization::SerializeStruct(Struct.StaticStruct(), &Struct, Options, Result);
    return Result;
}

/**
 * Serializes the data from the given Struct into a Node. The function will recursively iterate over all Properties
 * in the Struct and try to parse the contents in the fields of the Struct into an entry in the Node.
 *
 * @param Node The Node that will receive the data
 * @param Struct The Struct class that describes the Struct
 * @param StructValue The pointer to the actual memory location of the Struct
 * @param Options Controls the behavior of the serialization
 * @return The Result of the parsing operation
 */
FORCEINLINE FYamlSerializationResult SerializeStruct(FYamlNode& Node, const UScriptStruct* Struct,
                                                     const void* StructValue,
                                                     const FYamlSerializeOptions& Options = {}) {
    FYamlSerializationResult Result;
    Node = UYamlSerialization::SerializeStruct(Struct, StructValue, Options, Result);
    return Result;
}


/**
 * Deserializes the given Node into the instance of the given Object. The function will recursively iterate over all
 * Properties in the Object and try to parse the contents of the corresponding Node entry into the field of the Object.
 *
 * @tparam ObjectType The Type of Object we are parsing. Must be a UObject
 * @param Node The Node that contains the Data
 * @param Object The Object that should receive the data from the Node
 * @param Options Controls the behavior of the deserialization
 * @return The Result of the parsing operation
 */
template<typename ObjectType>
FORCEINLINE FYamlSerializationResult DeserializeObject(const FYamlNode& Node, ObjectType* Object,
                                                       const FYamlDeserializeOptions& Options = {}) {
    static_assert(TIsDerivedFrom<ObjectType, UObject>::Value);

    FYamlSerializationResult Result;
    UYamlSerialization::DeserializeObject(Node, ObjectType::StaticClass(), Object, Options, Result);
    return Result;
}

/**
 * Deserializes the given Node into the instance of the given Struct. The function will recursively iterate over all
 * Properties in the Struct and try to parse the contents of the corresponding Node entry into the field of the Struct.
 *
 * @tparam StructType The Type of Struct we are parsing. Must be a UStruct
 * @param Node The Node that contains the Data
 * @param Struct The Struct that should receive the data from the Node
 * @param Options Controls the behavior of the deserialization
 * @return The Result of the parsing operation
 */
template<typename StructType>
FORCEINLINE FYamlSerializationResult DeserializeStruct(const FYamlNode& Node, StructType& Struct,
                                                       const FYamlDeserializeOptions& Options = {}) {
    FYamlSerializationResult Result;
    UYamlSerialization::DeserializeStruct(Node, Struct.StaticStruct(), &Struct, Options, Result);
    return Result;
}

/**
 * Deserializes the given Node into the instance of the given Struct. The function will recursively iterate over all
 * Properties in the Struct and try to parse the contents of the corresponding Node entry into the field of the Struct.
 *
 * @param Node The Node that contains the Data
 * @param Struct The Struct class that describes the Struct
 * @param StructValue The pointer to the actual memory location of the Struct
 * @param Options Controls the behavior of the deserialization
 * @return The Result of the parsing operation
 */
FORCEINLINE FYamlSerializationResult DeserializeStruct(const FYamlNode& Node, const UScriptStruct* Struct,
                                                       void* StructValue, const FYamlDeserializeOptions& Options = {}) {
    FYamlSerializationResult Result;
    UYamlSerialization::DeserializeStruct(Node, Struct, StructValue, Options, Result);
    return Result;
}
