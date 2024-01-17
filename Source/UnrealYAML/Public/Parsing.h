#pragma once

#include "CoreMinimal.h"
#include "Node.h"
#include "Parsing.generated.h"


DECLARE_LOG_CATEGORY_EXTERN(LogYamlParsing, Log, All)

/**
 * Controls how UnrealYAML's ParseInto operations behave.
 *
 * The default values here preserve previous behaviour before Options was introduced.
 */
USTRUCT()
struct UNREALYAML_API FYamlParseIntoOptions {
    GENERATED_BODY()

    // Static factory for a set of options that enforces validity of the incoming YAML.
    static FYamlParseIntoOptions Strict();

    /**
     * Ensures that we check that the type of a YAML node matches that which is in
     * the struct, or can be converted to it.
     *
     * For example, without this, if a struct expects a TArray, but the corresponding
     * YAML has a string value, parsing would still be successful. With this enabled,
     * the result will be indicated a failure and a suitable error message provided.
     */
    UPROPERTY()
    bool CheckTypes = false;

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
    UPROPERTY()
    bool CheckEnums = false;

    /**
     * Inspects fields' UPROPERTY meta for a "YamlRequired" specifier on the USTRUCT being parsed in to.
     * If present, and an incoming YAML structure is missing this property, a validation error will occur.
     */
    UPROPERTY()
    bool CheckRequired = false;

    /**
     * If true, validation will fail if the YAML contains properties that do not match with
     * a property in the struct being parsed.
     */
    UPROPERTY()
    bool CheckAdditionalProperties = false;
};

/**
 * State for running ParseInto operations. This provides detailed error information
 * once parsing is complete.
 */
USTRUCT()
struct UNREALYAML_API FYamlParseIntoCtx {
    GENERATED_BODY()

    /**
     * The options this ParseInto operation ran with.
     */
    UPROPERTY()
    FYamlParseIntoOptions Options;

    /**
     * Errors that occurred whilst trying to parse YAML in to a struct.
     */
    UPROPERTY()
    TArray<FString> Errors;

    /**
     * Returns true if there were no errors encountered in a ParseInto operation.
     */
    bool Success() const;

    friend class UYamlParsing;

private:

    // Stack access.
    FYamlParseIntoCtx& PushStack(const TCHAR* Property);
    FYamlParseIntoCtx& PushStack(const FYamlNode& Key);
    FYamlParseIntoCtx& PushStack(const int32 Index);
    void PopStack();

    void AddError(const TCHAR* Err);

    TArray<FString> Stack = {TEXT("")};
    FString StackStr() const;
};

UCLASS(BlueprintType)
class UNREALYAML_API UYamlParsing final : public UBlueprintFunctionLibrary {
    GENERATED_BODY()

    template<typename T>
    friend bool ParseNodeIntoStruct(const FYamlNode&, T&, const FYamlParseIntoOptions&);
    template<typename T>
    friend bool ParseNodeIntoStruct(const FYamlNode&, T&, FYamlParseIntoCtx&, const FYamlParseIntoOptions&);
    friend bool ParseNodeIntoStruct(const FYamlNode&, const UScriptStruct*, void*, const FYamlParseIntoOptions&);
    friend bool ParseNodeIntoStruct(const FYamlNode&, const UScriptStruct*, void*, FYamlParseIntoCtx&, const FYamlParseIntoOptions&);

    // Some conversion are defined in UnrealTypes.h. But FProperty does not provide a way to retrieve the Type, except as
    // an FString. This provides a list of all Types (as String) that can be directly converted via those conversion instead
    // of the Parsing all Fields further (basically a shortcut with neater results). This is only used to determine *if* an
    // FProperty can be converted directly, the if-else chain to check what type it actually is, is in ParseIntoNativeType itself.
    const static TArray<FString> NativeTypes;

    // UPROPERTY(meta=YamlRequired) indicates a USTRUCT's property is required when parsing in to it
    // from a YAML node. This is the string we look for.
    inline const static FName YamlRequiredSpecifier = FName(TEXT("YamlRequired"));

public:
    // Parsing of Strings/Files into Nodes -----------------------------------------------------------------------------
    /** Parses a String into an Yaml-Node.
     *
     * @returns If the Parsing was successful */
    UFUNCTION(BlueprintCallable, Category="YAML")
    static bool ParseYaml(const FString String, FYamlNode& Out);

    /** Opens a Document and Parses the Contents into a Yaml Structure.
     *
     * @returns If the File Exists and the Parsing was successful */
    UFUNCTION(BlueprintCallable, Category="YAML")
    static bool LoadYamlFromFile(const FString Path, FYamlNode& Out);

    /** Writes the Contents of a YAML-Node to an File.
     * This will overwrite the existing File if it exists! */
    UFUNCTION(BlueprintCallable, Category="YAML")
    static void WriteYamlToFile(const FString Path, FYamlNode Node);


    // Parsing into Struct ---------------------------------------------------------------------------------------------

    /** Parse the given Node into the instance of the given Struct. The function will recursively iterate over all
     * Properties in the Struct and try to parse the contents of the corresponding Node-Entry into the Field of the Struct.
     * If there is no corresponding Field in the Node, the entry will be skipped.
     *
     * If you use a C++ USTRUCT, all Properties that you want parsed must be a UPROPERTY for the UHT
     * to register them. For easier c++ access, use the global ParseNodeIntoStruct function
     *
     * NOTE: Key-Comparison is case insensitive!
     * 
     * @param Node The Node we want to Parse
     * @param Struct An instance of the Struct we want to parse into
     * @return Whether all Fields of the Struct were successfully parsed.
     */
    UFUNCTION(BlueprintCallable, CustomThunk, DisplayName="Parse Node into Struct", Category="YAML",
        meta=(CustomStructureParam="Struct"))
    static bool ParseIntoStruct_BP(const FYamlNode& Node, const int32& Struct) {
        checkNoEntry()
        return false;
    }

    // Custom Thunk for ParseIntoStruct_BP
    DECLARE_FUNCTION(execParseIntoStruct_BP) {
        P_GET_STRUCT_REF(FYamlNode, Node)

        // Steps into the stack, walking to the next Object in it
        Stack.Step(Stack.Object, nullptr);

        // Grab the last property found when we walked the stack. This does not contains the property value, only its type information
        const FStructProperty* StructProperty = CastField<FStructProperty>(Stack.MostRecentProperty);
        if (!StructProperty)
            UE_LOG(LogYamlParsing, Error, TEXT("ParseIntoStruct did not receive an Struct Property: '%s'"),
               *Stack.MostRecentProperty->GetName())

        // Grab the base address where the struct actually stores its data. This is where the property value is truly stored
        void* StructPtr = Stack.MostRecentPropertyAddress;

        P_FINISH

        if (StructProperty) {
            FYamlParseIntoCtx Ctx;
            *static_cast<bool*>(RESULT_PARAM) = ParseIntoStruct(Node, StructProperty->Struct, StructPtr, Ctx);
        }
    }

private:
    // Parses a Node into a Single Property. Can be a FStructProperty itself (recursion!)
    static bool ParseIntoProperty(const FYamlNode& Node, const FProperty& Property, void* PropertyValue, FYamlParseIntoCtx& Ctx);

    // Parses a Node into a Single Struct. Calls ParseIntoProperty on all Fields
    static bool ParseIntoStruct(const FYamlNode& Node, const UScriptStruct* Struct, void* StructValue, FYamlParseIntoCtx& Ctx);

    // Parses a Node into a Single UObject. Calls ParseIntoProperty on all Fields
    static bool ParseIntoObject(const FYamlNode& Node, const UClass* Object, void* ObjectValue, FYamlParseIntoCtx& Ctx);

    // Parses a Node via some predefined conversions in UnrealTypes.h via a switch case (see NativeTypes).
    static bool ParseIntoNativeType(const FYamlNode& Node, const UScriptStruct* Struct, void* StructValue, FYamlParseIntoCtx& Ctx);

    template <typename T>
    static bool CheckScalarCanConvert(FYamlParseIntoCtx& Ctx, const TCHAR* TypeName, const FYamlNode& Node) {
        if (!CheckNodeType(Ctx, EYamlNodeType::Scalar, TEXT("scalar"), Node)) {
            return false;
        }

        if (Ctx.Options.CheckTypes && Node.IsDefined() && !Node.CanConvertTo<T>()) {
            Ctx.AddError(*FString::Printf(TEXT("cannot convert \"%s\" to type %s"), *Node.Scalar(), TypeName));
            return false;
        }

        return true;
    }

    static bool CheckNodeType(FYamlParseIntoCtx& Ctx, const EYamlNodeType Expected, const TCHAR* TypeName, const FYamlNode& Node);

    static bool CheckEnumValue(FYamlParseIntoCtx& Ctx, const FYamlNode& Node, const UEnum* Enum);

    // Resolves a UObject by its path, or nullptr if not found.
    template <typename BaseClass>
    static UObject* FindObject(const FString& Path) {
        return StaticLoadObject(BaseClass::StaticClass(), nullptr, *Path);
    }

    // Resolve a class by its path, including blueprint classes. Nullptr if not found.
    static UClass* FindClass(const FString& Path) {
        auto Obj = StaticLoadClass(UObject::StaticClass(), nullptr, *Path);
        if (IsValid(Obj)) {
            return Obj;
        }

        auto Bp = Cast<UBlueprint>(FindObject<UBlueprint>(Path));
        if (!Bp) {
            return nullptr;
        }

        return Cast<UClass>(Bp->GeneratedClass);
    }
};


/** C++ Wrapper for UYamlParsing::ParseIntoStruct */
template<typename T>
FORCENOINLINE bool ParseNodeIntoStruct(const FYamlNode& Node, T& StructIn,
                                       const FYamlParseIntoOptions& Options = FYamlParseIntoOptions()) {
    FYamlParseIntoCtx Result;
    Result.Options = Options;
    return UYamlParsing::ParseIntoStruct(Node, StructIn.StaticStruct(), &StructIn, Result);
}
template<typename T>
FORCENOINLINE bool ParseNodeIntoStruct(const FYamlNode& Node, T& StructIn, FYamlParseIntoCtx& Result,
                                       const FYamlParseIntoOptions& Options = FYamlParseIntoOptions()) {
    Result.Options = Options;
    return UYamlParsing::ParseIntoStruct(Node, StructIn.StaticStruct(), &StructIn, Result);
}

/** Parse the given YAML in to a struct whose type is not known at compile time. This is useful
 * when dealing with struct of an unknown types in C++ contexts, such as interacting with Unreal's
 * data tables.
 */
inline bool ParseNodeIntoStruct(const FYamlNode& Node, const UScriptStruct* Struct, void* StructValue,
                                const FYamlParseIntoOptions& Options = FYamlParseIntoOptions()) {
    FYamlParseIntoCtx Ctx;
    Ctx.Options = Options;
    return UYamlParsing::ParseIntoStruct(Node, Struct, StructValue, Ctx);
}
inline bool ParseNodeIntoStruct(const FYamlNode& Node, const UScriptStruct* Struct, void* StructValue,
                                FYamlParseIntoCtx& Result,
                                const FYamlParseIntoOptions& Options = FYamlParseIntoOptions()) {
    Result.Options = Options;
    return UYamlParsing::ParseIntoStruct(Node, Struct, StructValue, Result);
}