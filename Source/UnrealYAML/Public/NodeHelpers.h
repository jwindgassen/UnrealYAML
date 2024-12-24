#pragma once

#include "CoreMinimal.h"
#include "UnrealYAMLNode.h"
#include "Enums.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "NodeHelpers.generated.h"


// Unfortunately, Macros won't be expanded before the UHT is running, so we need to declare all Functions manually.
// I will leave that here for ease of writing ("Substitute macro call" by IDE) and because it might work in the future
#define DECLARE_YAML_CONVERSION(Type, FancyName) \
    UFUNCTION(BlueprintPure, Category="YAML|Make") static FYamlNode MakeFrom##FancyName(Type Value); \
    UFUNCTION(BlueprintPure, Category="YAML|Make") static FYamlNode MakeFrom##FancyName##Array(const TArray<Type>& Value); \
    UFUNCTION(BlueprintPure, Category="YAML|Make") static FYamlNode MakeFromInt##FancyName##Map(const TMap<int32, Type>& Value); \
    UFUNCTION(BlueprintPure, Category="YAML|Make") static FYamlNode MakeFromString##FancyName##Map(const TMap<FString, Type>& Value); \
    UFUNCTION(BlueprintPure, Category="YAML|Convert") static bool As##FancyName(const FYamlNode& Node, Type Default, Type& Value); \
    UFUNCTION(BlueprintPure, Category="YAML|Convert") static bool As##FancyName##Array(const FYamlNode& Node, const TArray<Type>& Default, TArray<Type>& Value); \
    UFUNCTION(BlueprintPure, Category="YAML|Convert") static bool AsInt##FancyName##Map(const FYamlNode& Node, const TMap<int32, Type>& Default, TMap<int32, Type>& Value); \
    UFUNCTION(BlueprintPure, Category="YAML|Convert") static bool AsString##FancyName##Map(const FYamlNode& Node, const TMap<FString, Type>& Default, TMap<FString, Type>& Value);


DEFINE_LOG_CATEGORY_STATIC(LogYamlHelper, Log, All)


// A Collection of Functions regarding Nodes, primarily for Blueprint-Interfacing
UCLASS(AutoCollapseCategories="Make,Convert")
class UNREALYAML_API UYamlNodeHelpers : public UBlueprintFunctionLibrary {
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, DisplayName="Make YAML Node", Category="YAML|Make")
    static FYamlNode MakeYamlNode() {
        return FYamlNode();
    }

    // Node Properties -------------------------------------------------------------------------------------------------

    /** Returns the Type of the Contained Data */
    UFUNCTION(BlueprintPure, Category="YAML")
    static EYamlNodeType Type(const FYamlNode& Node) {
        return Node.Type();
    }

    /** If the Node has been Defined */
    UFUNCTION(BlueprintPure, Category="YAML")
    static bool IsDefined(const FYamlNode& Node) {
        return Node.IsDefined();
    }

    /** Equivalent to Type() == Null (No Value) */
    UFUNCTION(BlueprintPure, Category="YAML")
    static bool IsNull(const FYamlNode& Node) {
        return Node.IsNull();
    }

    /** Equivalent to Type() == Scalar (Singular Value) */
    UFUNCTION(BlueprintPure, Category="YAML")
    static bool IsScalar(const FYamlNode& Node) {
        return Node.IsScalar();
    }

    /** Equivalent to Type() == Sequence (Multiple Values without Keys) */
    UFUNCTION(BlueprintPure, Category="YAML")
    static bool IsSequence(const FYamlNode& Node) {
        return Node.IsSequence();
    }

    /** Equivalent to Type() == Map (List of Key-Value Pairs) */
    UFUNCTION(BlueprintPure, Category="YAML")
    static bool IsMap(const FYamlNode& Node) {
        return Node.IsMap();
    }

    /** Returns the Style of the Node, mostly relevant for Sequences */
    UFUNCTION(BlueprintPure, Category="YAML")
    static EYamlEmitterStyle Style(const FYamlNode& Node) {
        return Node.Style();
    }

    /** Sets the Style of the Node, mostly relevant for Sequences */
    UFUNCTION(BlueprintPure, Category="YAML")
    static FYamlNode SetStyle(UPARAM(ref) FYamlNode& Node, EYamlEmitterStyle Style) {
        Node.SetStyle(Style);
        return Node;
    }

    /** Test if 2 Nodes are Equal */
    UFUNCTION(BlueprintPure, Category="YAML")
    static bool Equal(const FYamlNode& Node, const FYamlNode Other) {
        return Node.Is(Other);
    }

    /** Overwrite the Contents of this Node with the Content of another Node, or delete them if no Argument is given.
     * @returns If the Operation was successful
     */
    UFUNCTION(BlueprintPure, Category="YAML")
    static bool Reset(FYamlNode& Node) {
        return Node.Reset();
    }

    /** Get Raw String in Node as a single FString */
    UFUNCTION(BlueprintPure, Category="YAML")
    static FString GetContent(const FYamlNode& Node) {
        return Node.GetContent();
    }

    /** Get items size in Node if it is a Sequences or Map, 0 otherwise */
    UFUNCTION(BlueprintPure, Category="YAML")
    static int32 Size(const FYamlNode& Node) {
        return Node.Size();
    }

    // Node Accessing --------------------------------------------------------------------------------------------------

    /** Access the Content of a Node for a given Key and return the Value.
     *  Cannot be used to Access the Index of a Sequence, try using GetIndex instead!
     *
     *  @returns The Value for a given Key. The returned Node is invalid if an invalid Key was used.
     */
    UFUNCTION(BlueprintPure, Category="YAML")
    static FYamlNode Get(const FYamlNode& Node, const FString& Key) {
        return Node[Key];
    }

    /** Access the Content of a Sequence at a given Index
     *
     * @returns The Value at the given Index. The returned Node is invalid if an invalid Key was used.
     */
    UFUNCTION(BlueprintPure, Category="YAML")
    static FYamlNode GetIndex(const FYamlNode& Node, int32 Index) {
        return Node[Index];
    }

    /** Set the Content of a Node for a given Key to the Value.
     *  Cannot be used to Set the Element in a Sequence, try using SetIndex instead!
     */
    UFUNCTION(BlueprintPure, Category="YAML")
    static bool Set(UPARAM(ref) FYamlNode& Node, const FString& Key, const FYamlNode& Value, FYamlNode& Out) {
        try {
            Node[Key] = Value;
        } catch (YAML::InvalidNode) {
            UE_LOG(LogYamlHelper, Warning, TEXT("Could not Set Node Value for Key '%s'"), *Key)
            return false;
        }

        Out = Node;
        return false;
    }

    /** Set the Content of a Sequence at a given Index */
    UFUNCTION(BlueprintPure, Category="YAML")
    static bool SetIndex(UPARAM(ref) FYamlNode& Node, int32 Index, const FYamlNode& Value, FYamlNode& Out) {
        try {
            Node[Index] = Value;
        } catch (YAML::InvalidNode) {
            UE_LOG(LogYamlHelper, Warning, TEXT("Could not Set Node Value at Index %d"), Index)
            return false;
        }

        Out = Node;
        return true;
    }

    /** Push an element to the back of the sequence
     *
     * @returns Push will fail it the Node is not a Sequence or empty
     */
    UFUNCTION(BlueprintPure, Category="YAML")
    static bool Push(UPARAM(ref) FYamlNode& Node, const FYamlNode& Element, FYamlNode& Out) {
        try {
            Node.Push(Element);
        } catch (YAML::BadPushback) {
            UE_LOG(LogYamlHelper, Warning, TEXT("Could not Push onto Node"))
            return false;
        }

        Out = Node;
        return true;
    }


    // Create Constructors and Conversion for all Types ----------------------------------------------------------------
    // Int
    // DECLARE_YAML_CONVERSION(int32, Int)
    UFUNCTION(BlueprintPure, Category="YAML|Make")
    static FYamlNode MakeFromInt(int32 Value);
    UFUNCTION(BlueprintPure, Category="YAML|Make")
    static FYamlNode MakeFromIntArray(const TArray<int32>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Make")
    static FYamlNode MakeFromIntIntMap(const TMap<int32, int32>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Make")
    static FYamlNode MakeFromStringIntMap(const TMap<FString, int32>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Convert")
    static bool AsInt(const FYamlNode& Node, int32 Default, int32& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Convert")
    static bool AsIntArray(const FYamlNode& Node, const TArray<int32>& Default, TArray<int32>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Convert")
    static bool AsIntIntMap(const FYamlNode& Node, const TMap<int32, int32>& Default, TMap<int32, int32>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Convert")
    static bool AsStringIntMap(const FYamlNode& Node, const TMap<FString, int32>& Default, TMap<FString, int32>& Value);

    // Long
    // DECLARE_YAML_CONVERSION(int64, Long)
    UFUNCTION(BlueprintPure, Category="YAML|Make")
    static FYamlNode MakeFromLong(int64 Value);
    UFUNCTION(BlueprintPure, Category="YAML|Make")
    static FYamlNode MakeFromLongArray(const TArray<int64>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Make")
    static FYamlNode MakeFromIntLongMap(const TMap<int32, int64>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Make")
    static FYamlNode MakeFromStringLongMap(const TMap<FString, int64>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Convert")
    static bool AsLong(const FYamlNode& Node, int64 Default, int64& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Convert")
    static bool AsLongArray(const FYamlNode& Node, const TArray<int64>& Default, TArray<int64>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Convert")
    static bool AsIntLongMap(const FYamlNode& Node, const TMap<int32, int64>& Default, TMap<int32, int64>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Convert")
    static bool AsStringLongMap(const FYamlNode& Node, const TMap<FString, int64>& Default,
                                TMap<FString, int64>& Value);

    // Byte
    // DECLARE_YAML_CONVERSION(uint8, Byte)
    UFUNCTION(BlueprintPure, Category="YAML|Make")
    static FYamlNode MakeFromByte(uint8 Value);
    UFUNCTION(BlueprintPure, Category="YAML|Make")
    static FYamlNode MakeFromByteArray(const TArray<uint8>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Make")
    static FYamlNode MakeFromIntByteMap(const TMap<int32, uint8>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Make")
    static FYamlNode MakeFromStringByteMap(const TMap<FString, uint8>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Convert")
    static bool AsByte(const FYamlNode& Node, uint8 Default, uint8& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Convert")
    static bool AsByteArray(const FYamlNode& Node, const TArray<uint8>& Default, TArray<uint8>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Convert")
    static bool AsIntByteMap(const FYamlNode& Node, const TMap<int32, uint8>& Default, TMap<int32, uint8>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Convert")
    static bool AsStringByteMap(const FYamlNode& Node, const TMap<FString, uint8>& Default,
                                TMap<FString, uint8>& Value);

    // Float
    // DECLARE_YAML_CONVERSION(float, Float)
    UFUNCTION(BlueprintPure, Category="YAML|Make")
    static FYamlNode MakeFromFloat(float Value);
    UFUNCTION(BlueprintPure, Category="YAML|Make")
    static FYamlNode MakeFromFloatArray(const TArray<float>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Make")
    static FYamlNode MakeFromIntFloatMap(const TMap<int32, float>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Make")
    static FYamlNode MakeFromStringFloatMap(const TMap<FString, float>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Convert")
    static bool AsFloat(const FYamlNode& Node, float Default, float& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Convert")
    static bool AsFloatArray(const FYamlNode& Node, const TArray<float>& Default, TArray<float>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Convert")
    static bool AsIntFloatMap(const FYamlNode& Node, const TMap<int32, float>& Default, TMap<int32, float>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Convert")
    static bool AsStringFloatMap(const FYamlNode& Node, const TMap<FString, float>& Default,
                                 TMap<FString, float>& Value);

    // Bool
    // DECLARE_YAML_CONVERSION(bool, Bool)
    UFUNCTION(BlueprintPure, Category="YAML|Make")
    static FYamlNode MakeFromBool(bool Value);
    UFUNCTION(BlueprintPure, Category="YAML|Make")
    static FYamlNode MakeFromBoolArray(const TArray<bool>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Make")
    static FYamlNode MakeFromIntBoolMap(const TMap<int32, bool>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Make")
    static FYamlNode MakeFromStringBoolMap(const TMap<FString, bool>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Convert")
    static bool AsBool(const FYamlNode& Node, bool Default, bool& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Convert")
    static bool AsBoolArray(const FYamlNode& Node, const TArray<bool>& Default, TArray<bool>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Convert")
    static bool AsIntBoolMap(const FYamlNode& Node, const TMap<int32, bool>& Default, TMap<int32, bool>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Convert")
    static bool AsStringBoolMap(const FYamlNode& Node, const TMap<FString, bool>& Default, TMap<FString, bool>& Value);

    // String
    // DECLARE_YAML_CONVERSION(FString, String)
    UFUNCTION(BlueprintPure, Category="YAML|Make")
    static FYamlNode MakeFromString(FString Value);
    UFUNCTION(BlueprintPure, Category="YAML|Make")
    static FYamlNode MakeFromStringArray(const TArray<FString>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Make")
    static FYamlNode MakeFromIntStringMap(const TMap<int32, FString>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Make")
    static FYamlNode MakeFromStringStringMap(const TMap<FString, FString>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Convert")
    static bool AsString(const FYamlNode& Node, FString Default, FString& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Convert")
    static bool AsStringArray(const FYamlNode& Node, const TArray<FString>& Default, TArray<FString>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Convert")
    static bool AsIntStringMap(const FYamlNode& Node, const TMap<int32, FString>& Default, TMap<int32, FString>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Convert")
    static bool AsStringStringMap(const FYamlNode& Node, const TMap<FString, FString>& Default,
                                  TMap<FString, FString>& Value);

    // Text
    // DECLARE_YAML_CONVERSION(FText, Text)
    UFUNCTION(BlueprintPure, Category="YAML|Make")
    static FYamlNode MakeFromText(FText Value);
    UFUNCTION(BlueprintPure, Category="YAML|Make")
    static FYamlNode MakeFromTextArray(const TArray<FText>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Make")
    static FYamlNode MakeFromIntTextMap(const TMap<int32, FText>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Make")
    static FYamlNode MakeFromStringTextMap(const TMap<FString, FText>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Convert")
    static bool AsText(const FYamlNode& Node, FText Default, FText& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Convert")
    static bool AsTextArray(const FYamlNode& Node, const TArray<FText>& Default, TArray<FText>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Convert")
    static bool AsIntTextMap(const FYamlNode& Node, const TMap<int32, FText>& Default, TMap<int32, FText>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Convert")
    static bool AsStringTextMap(const FYamlNode& Node, const TMap<FString, FText>& Default,
                                TMap<FString, FText>& Value);

    // Vector
    // DECLARE_YAML_CONVERSION(FVector, Vector)
    UFUNCTION(BlueprintPure, Category="YAML|Make")
    static FYamlNode MakeFromVector(FVector Value);
    UFUNCTION(BlueprintPure, Category="YAML|Make")
    static FYamlNode MakeFromVectorArray(const TArray<FVector>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Make")
    static FYamlNode MakeFromIntVectorMap(const TMap<int32, FVector>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Make")
    static FYamlNode MakeFromStringVectorMap(const TMap<FString, FVector>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Convert")
    static bool AsVector(const FYamlNode& Node, FVector Default, FVector& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Convert")
    static bool AsVectorArray(const FYamlNode& Node, const TArray<FVector>& Default, TArray<FVector>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Convert")
    static bool AsIntVectorMap(const FYamlNode& Node, const TMap<int32, FVector>& Default, TMap<int32, FVector>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Convert")
    static bool AsStringVectorMap(const FYamlNode& Node, const TMap<FString, FVector>& Default,
                                  TMap<FString, FVector>& Value);

    // Quat
    // DECLARE_YAML_CONVERSION(FQuat, Quat)
    UFUNCTION(BlueprintPure, Category="YAML|Make")
    static FYamlNode MakeFromQuat(FQuat Value);
    UFUNCTION(BlueprintPure, Category="YAML|Make")
    static FYamlNode MakeFromQuatArray(const TArray<FQuat>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Make")
    static FYamlNode MakeFromIntQuatMap(const TMap<int32, FQuat>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Make")
    static FYamlNode MakeFromStringQuatMap(const TMap<FString, FQuat>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Convert")
    static bool AsQuat(const FYamlNode& Node, FQuat Default, FQuat& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Convert")
    static bool AsQuatArray(const FYamlNode& Node, const TArray<FQuat>& Default, TArray<FQuat>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Convert")
    static bool AsIntQuatMap(const FYamlNode& Node, const TMap<int32, FQuat>& Default, TMap<int32, FQuat>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Convert")
    static bool AsStringQuatMap(const FYamlNode& Node, const TMap<FString, FQuat>& Default,
                                TMap<FString, FQuat>& Value);

    // Transform
    // DECLARE_YAML_CONVERSION(FTransform, Transform)
    UFUNCTION(BlueprintPure, Category="YAML|Make")
    static FYamlNode MakeFromTransform(FTransform Value);
    UFUNCTION(BlueprintPure, Category="YAML|Make")
    static FYamlNode MakeFromTransformArray(const TArray<FTransform>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Make")
    static FYamlNode MakeFromIntTransformMap(const TMap<int32, FTransform>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Make")
    static FYamlNode MakeFromStringTransformMap(const TMap<FString, FTransform>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Convert")
    static bool AsTransform(const FYamlNode& Node, FTransform Default, FTransform& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Convert")
    static bool AsTransformArray(const FYamlNode& Node, const TArray<FTransform>& Default, TArray<FTransform>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Convert")
    static bool AsIntTransformMap(const FYamlNode& Node, const TMap<int32, FTransform>& Default,
                                  TMap<int32, FTransform>& Value);
    UFUNCTION(BlueprintPure, Category="YAML|Convert")
    static bool AsStringTransformMap(const FYamlNode& Node, const TMap<FString, FTransform>& Default,
                                     TMap<FString, FTransform>& Value);
};

#undef DECLARE_YAML_CONVERSION
