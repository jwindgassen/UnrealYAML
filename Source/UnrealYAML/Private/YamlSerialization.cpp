#include "YamlSerialization.h"

#include "YamlParsing.h"
#include "Blueprint/BlueprintExceptionInfo.h"


FYamlDeserializeOptions FYamlDeserializeOptions::Strict() {
    FYamlDeserializeOptions Ret;
    Ret.StrictTypes = true;
    Ret.CheckEnums = true;
    Ret.RespectRequiredProperties = true;
    Ret.AllowUnusedValues = false;
    return Ret;
}


FString FYamlSerializationResult::ScopeName() const {
    return ScopesStack.Num() > 0 ? FString::Join(ScopesStack, TEXT(".")) : "<root>";
}

void FYamlSerializationResult::PushStack(const FString& Property) {
    ScopesStack.Add(Property);
}

void FYamlSerializationResult::PushStack(const FYamlNode& Key) {
    ScopesStack.Add(Key.Scalar());
}

void FYamlSerializationResult::PushStack(const int32 Index) {
    ScopesStack.Add(FString::Printf(TEXT("[%d]"), Index));
}

void FYamlSerializationResult::PopStack() {
    ScopesStack.RemoveAt(ScopesStack.Num() - 1);
}


const TArray<FString> UYamlSerialization::NativeTypes = {"FString",    "FText",  "FVector",      "FVector2D", "FQuat",
                                                         "FTransform", "FColor", "FLinearColor", "FRotator"};


FYamlSerializationResult UYamlSerialization::DeserializeObject_BP(const FYamlNode& Node, UObject* Object,
                                                                  const FYamlDeserializeOptions& Options) {
    return {};
}


DEFINE_FUNCTION(UYamlSerialization::execDeserializeStruct_BP) {
    P_GET_STRUCT_REF(FYamlNode, Node);

    // Steps into the stack, walking to the next Object in it
    Stack.MostRecentProperty = nullptr;
    Stack.MostRecentPropertyAddress = nullptr;
    Stack.StepCompiledIn<FStructProperty>(nullptr);

    // Grab the last property found when we walked the stack and its address.
    // The former does not contain the property value, only its type information.
    // The latter where the property value is truly stored
    const FStructProperty* StructProperty = CastField<FStructProperty>(Stack.MostRecentProperty);
    void* StructValue = Stack.MostRecentPropertyAddress;

    P_GET_STRUCT_REF(FYamlDeserializeOptions, Options);

    P_FINISH;

    if (!StructProperty || !StructValue) {
        const FString ErrorMessage = FString::Printf(TEXT("DeserializeStruct did not receive an Struct Property: '%s'"),
                                                     *Stack.MostRecentProperty->GetName());

        const FBlueprintExceptionInfo ExceptionInfo{EBlueprintExceptionType::AccessViolation,
                                                    FText::FromString(ErrorMessage)};
        FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);

        UE_LOG(LogYamlParsing, Error, TEXT("%s"), *ErrorMessage)
    }

    P_NATIVE_BEGIN;

    FYamlSerializationResult Result;
    DeserializeStruct(Node, StructProperty->Struct, StructValue, Options, Result);
    *static_cast<FYamlSerializationResult*>(RESULT_PARAM) = Result;

    P_NATIVE_END;
}



void UYamlSerialization::DeserializeProperty(const FYamlNode& Node, const FProperty& Property, void* PropertyValue,
                                             const FYamlDeserializeOptions& Options, FYamlSerializationResult& Result) {
    UE_LOG(LogYamlParsing, Verbose, TEXT("DeserializeProperty: %s %s"), *Property.GetCPPType(), *Property.GetName())

    // If we access an invalid sequence or map entry, we will get a Zombie Node, which is invalid
    if (!Node.IsDefined()) {
        return;
    }

    if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(&Property)) {
        const int64 Index = DeserializeEnumValue(Node, EnumProperty->GetEnum(), Options.CheckEnums, Result);
        if (Index != INDEX_NONE) {
            EnumProperty->GetUnderlyingProperty()->SetIntPropertyValue(PropertyValue, Index);
        }
    } else if (const FByteProperty* ByteProperty = CastField<FByteProperty>(&Property);
               ByteProperty && ByteProperty->IsEnum()) {
        const int64 Index = DeserializeEnumValue(Node, ByteProperty->GetIntPropertyEnum(), Options.CheckEnums, Result);
        if (Index != INDEX_NONE) {
            ByteProperty->SetIntPropertyValue(PropertyValue, Index);
        }
    } else if (const FNumericProperty* NumericProperty = CastField<FNumericProperty>(&Property)) {
        if (!EnsureNodeType(Node, EYamlNodeType::Scalar, Options.StrictTypes, Result)) {
            return;
        }

        // Try conversion to biggest int/float type => any smaller type should also work
        const bool CanConvert = NumericProperty->IsInteger() ? Node.CanConvertTo<int64>() : Node.CanConvertTo<double>();
        if (Options.StrictTypes && Node.IsDefined() && !CanConvert) {
            Result.AddError(TEXT("Cannot convert '%s' to %s"), *Node.Scalar(),
                            NumericProperty->IsInteger() ? TEXT("an Integer") : TEXT("a Float"));
            return;
        }

        const auto ValueAsString = Node.AsOptional<FString>();
        if (ValueAsString.IsSet()) {
            NumericProperty->SetNumericPropertyValueFromString(PropertyValue, **ValueAsString);
        }
    } else if (const FBoolProperty* BoolProperty = CastField<FBoolProperty>(&Property)) {
        DeserializeScalarValue<bool>(Node, BoolProperty, PropertyValue, TEXT("Bool"), Options.StrictTypes, Result);
    } else if (const FStrProperty* StringProperty = CastField<FStrProperty>(&Property)) {
        DeserializeScalarValue<FString>(Node, StringProperty, PropertyValue, TEXT("String"), Options.StrictTypes,
                                        Result);
    } else if (const FNameProperty* NameProperty = CastField<FNameProperty>(&Property)) {
        DeserializeScalarValue<FName>(Node, NameProperty, PropertyValue, TEXT("String"), Options.StrictTypes, Result);
    } else if (const FTextProperty* TextProperty = CastField<FTextProperty>(&Property)) {
        DeserializeScalarValue<FText>(Node, TextProperty, PropertyValue, TEXT("String"), Options.StrictTypes, Result);
    } else if (const FSoftObjectProperty* SoftObjProperty = CastField<FSoftObjectProperty>(&Property)) {
        if (!EnsureScalarOfType<FString>(Node, TEXT("String"), Options.StrictTypes, Result)) {
            return;
        }

        const auto Value = Node.AsOptional<FString>();
        if (Value.IsSet()) {
            const auto Object = StaticLoadObject(UObject::StaticClass(), nullptr, **Value);

            if (!IsValid(Object)) {
                Result.AddError(TEXT("Cannot find Object '%s'"), *Value.GetValue());
                return;
            }

            SoftObjProperty->SetObjectPropertyValue(PropertyValue, Object);
        }
    } else if (const FClassProperty* ClassProperty = CastField<FClassProperty>(&Property)) {
        if (!EnsureScalarOfType<FString>(Node, TEXT("String"), Options.StrictTypes, Result)) {
            return;
        }

        const auto Value = Node.AsOptional<FString>();
        if (Value.IsSet()) {
            auto Class = StaticLoadClass(UObject::StaticClass(), nullptr, **Value);

            if (!Class) {
                if (const auto Bp = Cast<UBlueprint>(StaticLoadObject(UBlueprint::StaticClass(), nullptr, **Value))) {
                    Class = Bp->GeneratedClass;
                }
            }

            if (!Class) {
                Result.AddError(TEXT("Cannot find Class '%s'"), **Value);
                return;
            }

            ClassProperty->SetObjectPropertyValue(PropertyValue, Class);
        }
    } else if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(&Property)) {
        if (!EnsureNodeType(Node, EYamlNodeType::Sequence, Options.StrictTypes, Result)) {
            return;
        }

        // We need the helper to get to the items of the array
        FScriptArrayHelper Helper(ArrayProperty, PropertyValue);
        Helper.EmptyValues();  // Clear existing values
        Helper.AddValues(Node.Size());

        for (int32 i = 0; i < Helper.Num(); ++i) {
            Result.PushStack(i);
            DeserializeProperty(Node[i], *ArrayProperty->Inner, Helper.GetRawPtr(i), Options, Result);
            Result.PopStack();
        }
    } else if (const FMapProperty* MapProperty = CastField<FMapProperty>(&Property)) {
        if (!EnsureNodeType(Node, EYamlNodeType::Map, Options.StrictTypes, Result)) {
            return;
        }

        FScriptMapHelper Helper(MapProperty, PropertyValue);
        Helper.EmptyValues();  // Clear existing values


        for (const auto [Key, Value] : Node) {
            const auto i = Helper.AddDefaultValue_Invalid_NeedsRehash();

            Result.PushStack(Key);

            DeserializeProperty(Key, *Helper.KeyProp, Helper.GetKeyPtr(i), Options, Result);
            DeserializeProperty(Value, *Helper.ValueProp, Helper.GetValuePtr(i), Options, Result);

            Result.PopStack();
        }

        Helper.Rehash();
    } else if (const FObjectProperty* ObjectProperty = CastField<FObjectProperty>(&Property)) {
        return DeserializeObject(Node, ObjectProperty->PropertyClass, PropertyValue, Options, Result);
    } else if (const FStructProperty* StructProperty = CastField<FStructProperty>(&Property)) {
        return DeserializeStruct(Node, StructProperty->Struct, PropertyValue, Options, Result);
    }
}

void UYamlSerialization::DeserializeStruct(const FYamlNode& Node, const UScriptStruct* Struct, void* StructValue,
                                           const FYamlDeserializeOptions& Options, FYamlSerializationResult& Result) {
    UE_LOG(LogYamlParsing, Verbose, TEXT("DeserializeStruct: %s"), *Struct->GetName())

    // Custom type handlers provided in options have priority.
    if (const FCustomTypeDeserializer* Handler = Options.TypeHandlers.Find(Struct->GetStructCPPName())) {
        Handler->Execute(Node, Struct, StructValue, Result);
        return;
    }

    if (NativeTypes.Contains(Struct->GetStructCPPName())) {
        return DeserializeNativeType(Node, Struct, StructValue);
    }

    if (!EnsureNodeType(Node, EYamlNodeType::Map, Options.StrictTypes, Result)) {
        return;
    }

    auto RemainingKeys = Node.Keys<FString>().Array();
    for (TFieldIterator<FProperty> It(Struct); It; ++It) {
        FString Key = It->GetName();

        Result.PushStack(Key);

        // Check if this key is mandatory (Editor-only!)
#if WITH_EDITORONLY_DATA
        if (Options.RespectRequiredProperties && It->HasMetaData(YamlRequiredSpecifier) && !Node[Key].IsDefined()) {
            Result.AddError(TEXT("Missing Required Key: %s"), *Key);
            return;
        }
#endif

        DeserializeProperty(Node[Key], **It, It->ContainerPtrToValuePtr<void>(StructValue), Options, Result);

        RemainingKeys.Remove(Key);

        Result.PopStack();
    }

    if (!Options.AllowUnusedValues && RemainingKeys.Num()) {
        Result.AddError(TEXT("Struct has additional unused Keys: %s"), *FString::Join(RemainingKeys, TEXT(", ")));
    }
}

void UYamlSerialization::DeserializeObject(const FYamlNode& Node, const UClass* Object, void* ObjectValue,
                                           const FYamlDeserializeOptions& Options, FYamlSerializationResult& Result) {
    UE_LOG(LogYamlParsing, Verbose, TEXT("DeserializeObject: %s"), *Object->GetName())

    if (!EnsureNodeType(Node, EYamlNodeType::Map, Options.StrictTypes, Result)) {
        return;
    }

    for (TFieldIterator<FProperty> It(Object); It; ++It) {
        FString Key = It->GetName();

        Result.PushStack(Key);
        DeserializeProperty(Node[Key], **It, It->ContainerPtrToValuePtr<void>(ObjectValue), Options, Result);
        Result.PopStack();
    }
}

void UYamlSerialization::DeserializeNativeType(const FYamlNode& Node, const UScriptStruct* Struct, void* StructValue) {
    const FString Type = Struct->GetStructCPPName();

    if (Type == "FString") {
        *static_cast<FString*>(StructValue) = Node.As<FString>();
    } else if (Type == "FText") {
        *static_cast<FText*>(StructValue) = Node.As<FText>();
    } else if (Type == "FVector") {
        *static_cast<FVector*>(StructValue) = Node.As<FVector>();
    } else if (Type == "FQuat") {
        *static_cast<FQuat*>(StructValue) = Node.As<FQuat>();
    } else if (Type == "FRotator") {
        *static_cast<FRotator*>(StructValue) = Node.As<FQuat>().Rotator();
    } else if (Type == "FTransform") {
        *static_cast<FTransform*>(StructValue) = Node.As<FTransform>();
    } else if (Type == "FColor") {
        *static_cast<FColor*>(StructValue) = Node.As<FColor>();
    } else if (Type == "FLinearColor") {
        *static_cast<FLinearColor*>(StructValue) = Node.As<FLinearColor>();
    } else if (Type == "FVector2D") {
        *static_cast<FVector2D*>(StructValue) = Node.As<FVector2D>();
    } else {
        checkf(false, TEXT("No native type conversion for %s"), *Type);
    }
}

bool UYamlSerialization::EnsureNodeType(const FYamlNode& Node, const EYamlNodeType Expected, const bool Strict,
                                        FYamlSerializationResult& Result) {
    if (Strict && Node.IsDefined() && Node.Type() != Expected) {
        Result.AddError(TEXT("Expected '%s' but found '%s'"), *EnumToString(Expected), *EnumToString(Node.Type()));
        return false;
    }

    return true;
}

int64 UYamlSerialization::DeserializeEnumValue(const FYamlNode& Node, const UEnum* Enum, const bool CheckEnums,
                                               FYamlSerializationResult& Result) {
    if (!Node.IsDefined()) {
        return INDEX_NONE;
    }

    // Try to parse the Value is an Int
    if (Node.CanConvertTo<int>()) {
        const int64 IntValue = Node.As<int64>();

        if (CheckEnums && !Enum->IsValidEnumValueOrBitfield(IntValue)) {
            Result.AddError(TEXT("%d is not an valid enum value of %s"), IntValue, *Enum->CppType);

            return INDEX_NONE;
        }

        return IntValue;
    }

    // Try to parse the Value as a String
    if (Node.CanConvertTo<FString>()) {
        const FString Name = Node.As<FString>();
        const int64 Value = Enum->GetValueByNameString(Name);

        if (CheckEnums && Value == INDEX_NONE) {
            Result.AddError(TEXT("'%s' is not an valid enum value of %s"), *Name, *Enum->CppType);

            return INDEX_NONE;
        }

        return Value;
    }

    // If we arrive here and still have not been able to resolve to an enum, this must be a bad type.
    if (CheckEnums) {
        Result.AddError(TEXT("Value of type '%s' cannot be parsed as an enum value for %s"),
                        *EnumToString(Node.Type()), *Enum->CppType);
    }

    return INDEX_NONE;
}


void UYamlSerialization::SerializeProperty(FYamlNode& Node, const FProperty& Property, const void* PropertyValue,
                                           const FYamlSerializeOptions& Options, FYamlSerializationResult& Result) {}

void UYamlSerialization::SerializeStruct(FYamlNode& Node, const UScriptStruct* Struct, const void* StructValue,
                                         const FYamlSerializeOptions& Options, FYamlSerializationResult& Result) {}

void UYamlSerialization::SerializeObject(FYamlNode& Node, const UClass* Object, const void* ObjectValue,
                                         const FYamlSerializeOptions& Options, FYamlSerializationResult& Result) {}

void UYamlSerialization::SerializeNativeType(FYamlNode& Node, const UScriptStruct* Struct, const void* StructValue,
                                             const FYamlSerializeOptions& Options, FYamlSerializationResult& Result) {}
