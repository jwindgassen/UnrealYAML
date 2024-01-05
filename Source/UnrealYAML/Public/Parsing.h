#pragma once

#include "CoreMinimal.h"
#include "Node.h"
#include "Parsing.generated.h"


DECLARE_LOG_CATEGORY_EXTERN(LogYamlParsing, Log, All)


UCLASS(BlueprintType)
class UNREALYAML_API UYamlParsing final : public UBlueprintFunctionLibrary {
    GENERATED_BODY()

    template<typename T>
    friend bool ParseNodeIntoStruct(const FYamlNode&, T&);

    // Some conversion are defined in UnrealTypes.h. But FProperty does not provide a way to retrieve the Type, except as
    // an FString. This provides a list of all Types (as String) that can be directly converted via those conversion instead
    // of the Parsing all Fields further (basically a shortcut with neater results). This is only used to determine *if* an
    // FProperty can be converted directly, the if-else chain to check what type it actually is, is in ParseNativeType itself.
    const static TArray<FString> NativeTypes;

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
            *static_cast<bool*>(RESULT_PARAM) = ParseIntoStruct(Node, StructProperty->Struct, StructPtr);
        }
    }

    /** Parse the given YAML in to a struct whose type is not known at compile time. This is useful
     * when dealing with struct of an unknown types in CPP contexts, such as interacting with Unreal's
     * data tables.
     */
    static bool ParseIntoStruct(const FYamlNode& Node, const UScriptStruct* Struct, void* StructValue);

private:
    // Parses a Node into a Single Property. Can be a FStructProperty itself (recursion!)
    static bool ParseIntoProperty(const FYamlNode& Node, const FProperty& Property, void* PropertyValue);

    // Parses a Node into a Single UObject. Calls ParseIntoProperty on all Fields
    static bool ParseIntoObject(const FYamlNode& Node, const UClass* Object, void* ObjectValue);

    // Parses a Node via some predefined conversions in UnrealTypes.h via a switch case (see NativeTypes).
    static bool ParseIntoNativeType(const FYamlNode& Node, const UScriptStruct* Struct, void* StructValue);
};


/** C++ Wrapper for UYamlParsing::ParseIntoStruct */
template<typename T>
FORCENOINLINE bool ParseNodeIntoStruct(const FYamlNode& Node, T& StructIn) {
    return UYamlParsing::ParseIntoStruct(Node, StructIn.StaticStruct(), &StructIn);
}
