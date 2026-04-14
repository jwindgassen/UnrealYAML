// Copyright (c) 2021-2026, Forschungszentrum Jülich GmbH. All rights reserved.
// Licensed under the MIT License. See LICENSE file for details.

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


FYamlSerializationResult UYamlSerialization::SerializeObject_BP(FYamlNode& Node, const UObject* Object,
                                                                const FYamlSerializeOptions& Options) {
    FYamlSerializationResult Result;
    Node = SerializeObject(Object->GetClass(), Object, Options, Result);
    return Result;
}


DEFINE_FUNCTION(UYamlSerialization::execSerializeStruct_BP) {
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

    P_GET_STRUCT_REF(FYamlSerializeOptions, Options);

    P_FINISH;

    if (!StructProperty || !StructValue) {
        const FString ErrorMessage = FString::Printf(TEXT("SerializeStruct did not receive an Struct Property: '%s'"),
                                                     *Stack.MostRecentProperty->GetName());

        const FBlueprintExceptionInfo ExceptionInfo{EBlueprintExceptionType::AccessViolation,
                                                    FText::FromString(ErrorMessage)};
        FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);

        UE_LOG(LogYamlParsing, Error, TEXT("%s"), *ErrorMessage)
    }

    P_NATIVE_BEGIN;

    FYamlSerializationResult Result;
    Node = SerializeStruct(StructProperty->Struct, StructValue, Options, Result);
    *static_cast<FYamlSerializationResult*>(RESULT_PARAM) = Result;

    P_NATIVE_END;
}


FYamlSerializationResult UYamlSerialization::DeserializeObject_BP(const FYamlNode& Node, UObject* Object,
                                                                  const FYamlDeserializeOptions& Options) {
    FYamlSerializationResult Result;
    DeserializeObject(Node, Object->GetClass(), Object, Options, Result);
    return Result;
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

FYamlNode UYamlSerialization::SerializeProperty(const FProperty& Property, const void* PropertyValue,
                                                const FYamlSerializeOptions& Options,
                                                FYamlSerializationResult& Result) {
    UE_LOG(LogYamlParsing, Verbose, TEXT("SerializeProperty: %s %s"), *Property.GetCPPType(), *Property.GetName())

    if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(&Property)) {
        const int64 Value = EnumProperty->GetUnderlyingProperty()->GetSignedIntPropertyValue(PropertyValue);
        return EnumProperty->GetEnum()->HasAnyEnumFlags(EEnumFlags::Flags) || Options.EnumAsNumber
            ? FYamlNode{Value}
            : FYamlNode{EnumProperty->GetEnum()->GetNameStringByValue(Value)};
    }

    if (const FByteProperty* ByteProperty = CastField<FByteProperty>(&Property);
        ByteProperty && ByteProperty->IsEnum()) {
        const int64 Value = ByteProperty->GetSignedIntPropertyValue(PropertyValue);
        return ByteProperty->GetIntPropertyEnum()->HasAnyEnumFlags(EEnumFlags::Flags) || Options.EnumAsNumber
            ? FYamlNode{Value}
            : FYamlNode{ByteProperty->GetIntPropertyEnum()->GetNameStringByValue(Value)};
    }

    if (const FNumericProperty* NumericProperty = CastField<FNumericProperty>(&Property)) {
        return FYamlNode{NumericProperty->GetNumericPropertyValueToString(PropertyValue)};
    }

    if (const FBoolProperty* BoolProperty = CastField<FBoolProperty>(&Property)) {
        return FYamlNode{BoolProperty->GetPropertyValue(PropertyValue)};
    }

    if (const FStrProperty* StringProperty = CastField<FStrProperty>(&Property)) {
        return FYamlNode{StringProperty->GetPropertyValue(PropertyValue)};
    }

    if (const FNameProperty* NameProperty = CastField<FNameProperty>(&Property)) {
        return FYamlNode{NameProperty->GetPropertyValue(PropertyValue).ToString()};
    }

    if (const FTextProperty* TextProperty = CastField<FTextProperty>(&Property)) {
        return FYamlNode{TextProperty->GetPropertyValue(PropertyValue).ToString()};
    }

    if (const FSoftObjectProperty* SoftObjProperty = CastField<FSoftObjectProperty>(&Property)) {
        const UObject* Object = SoftObjProperty->GetObjectPropertyValue(PropertyValue);
        if (!Object) {
            return FYamlNode{};
        }

        // Recreate FAssetData::GetExportTextName
        // ToDo: Maybe there is a better way to do this?
        FStringBuilderBase Name;
        Object->GetClass()->GetPathName(nullptr, Name);
        Name += "'";
        Object->GetPathName(nullptr, Name);
        Name += "'";

        return FYamlNode{FString{Name.ToString()}};
    }

    if (const FClassProperty* ClassProperty = CastField<FClassProperty>(&Property)) {
        UClass* Class = Cast<UClass>(ClassProperty->GetObjectPropertyValue(PropertyValue));
        if (!Class) {
            return FYamlNode{};
        }

        // Recreate FAssetData::GetExportTextName (but for classes)
        // ToDo: Maybe there is a better way to do this?
        FStringBuilderBase Name;
        Name.Append("/Script/CoreUObject.Class'");
        Class->GetPathName(nullptr, Name);
        Name += "'";

        return FYamlNode{FString{Name.ToString()}};
    }

    if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(&Property)) {
        FYamlNode Sequence{EYamlNodeType::Sequence};

        // We need the helper to get to the items of the array
        FScriptArrayHelper Helper(ArrayProperty, PropertyValue);
        for (int32 i = 0; i < Helper.Num(); ++i) {
            Result.PushStack(i);
            Sequence.Push(SerializeProperty(*ArrayProperty->Inner, Helper.GetRawPtr(i), Options, Result));
            Result.PopStack();
        }

        return Sequence;
    }

    if (const FMapProperty* MapProperty = CastField<FMapProperty>(&Property)) {
        FYamlNode Map{EYamlNodeType::Map};

        if (Options.IncludeTypeInformation) {
            Map.SetTag("TMap");
        }

        FScriptMapHelper Helper(MapProperty, PropertyValue);
        for (int32 i = 0; i < Helper.Num(); ++i) {
            Result.PushStack(i);  // ToDo: Find a better representation

            FYamlNode Key = SerializeProperty(*Helper.KeyProp, Helper.GetKeyPtr(i), Options, Result);
            FYamlNode Value = SerializeProperty(*Helper.ValueProp, Helper.GetValuePtr(i), Options, Result);

            Map[Key] = Value;

            Result.PopStack();
        }

        return Map;
    }

    if (const FObjectProperty* ObjectProperty = CastField<FObjectProperty>(&Property)) {
        return SerializeObject(ObjectProperty->PropertyClass, PropertyValue, Options, Result);
    }

    if (const FStructProperty* StructProperty = CastField<FStructProperty>(&Property)) {
        return SerializeStruct(StructProperty->Struct, PropertyValue, Options, Result);
    }

    return FYamlNode{};
}

FYamlNode UYamlSerialization::SerializeStruct(const UScriptStruct* Struct, const void* StructValue,
                                              const FYamlSerializeOptions& Options, FYamlSerializationResult& Result) {
    UE_LOG(LogYamlParsing, Verbose, TEXT("SerializeStruct: %s"), *Struct->GetName())

    // Custom type handlers provided in options have priority.
    if (const FCustomTypeSerializer* Handler = Options.TypeHandlers.Find(Struct->GetStructCPPName())) {
        return Handler->Execute(Struct, StructValue, Result);
    }

    if (NativeTypes.Contains(Struct->GetStructCPPName())) {
        return SerializeNativeType(Struct, StructValue);
    }

    FYamlNode Node{EYamlNodeType::Map};

    // Set the name of the Struct as a Tag
    if (Options.IncludeTypeInformation) {
        Node.SetTag(FString::Printf(TEXT("F%s"), *Struct->GetName()));
    }

    for (TFieldIterator<FProperty> It(Struct); It; ++It) {
        FString Key = It->GetName();
        CapitalizePropertyName(Key, Options.Capitalization);

        Result.PushStack(Key);
        Node[Key] = SerializeProperty(**It, It->ContainerPtrToValuePtr<void>(StructValue), Options, Result);
        Result.PopStack();
    }

    return Node;
}

FYamlNode UYamlSerialization::SerializeObject(const UClass* Object, const void* ObjectValue,
                                              const FYamlSerializeOptions& Options, FYamlSerializationResult& Result) {
    UE_LOG(LogYamlParsing, Verbose, TEXT("SerializeObject: %s"), *Object->GetName())

    FYamlNode Node{EYamlNodeType::Map};

    // Set the name of the Struct as a Tag
    if (Options.IncludeTypeInformation) {
        Node.SetTag(FString::Printf(TEXT("U%s"), *Object->GetName()));
    }


    for (TFieldIterator<FProperty> It(Object); It; ++It) {
        FString Key = It->GetName();
        CapitalizePropertyName(Key, Options.Capitalization);

        Result.PushStack(Key);
        Node[Key] = SerializeProperty(**It, It->ContainerPtrToValuePtr<void>(ObjectValue), Options, Result);
        Result.PopStack();
    }

    return Node;
}

FYamlNode UYamlSerialization::SerializeNativeType(const UScriptStruct* Struct, const void* StructValue) {
    const FString Type = Struct->GetStructCPPName();

    if (Type == "FString") {
        return FYamlNode{*static_cast<const FString*>(StructValue)};
    }

    if (Type == "FText") {
        return FYamlNode{*static_cast<const FText*>(StructValue)};
    }

    if (Type == "FVector") {
        return FYamlNode{*static_cast<const FVector*>(StructValue)};
    }

    if (Type == "FQuat") {
        return FYamlNode{*static_cast<const FQuat*>(StructValue)};
    }

    if (Type == "FRotator") {
        return FYamlNode{*static_cast<const FRotator*>(StructValue)};
    }

    if (Type == "FTransform") {
        return FYamlNode{*static_cast<const FTransform*>(StructValue)};
    }

    if (Type == "FColor") {
        return FYamlNode{*static_cast<const FColor*>(StructValue)};
    }

    if (Type == "FLinearColor") {
        return FYamlNode{*static_cast<const FLinearColor*>(StructValue)};
    }

    if (Type == "FVector2D") {
        return FYamlNode{*static_cast<const FVector2D*>(StructValue)};
    }

    checkf(false, TEXT("No native type conversion for %s"), *Type);
    return FYamlNode{};
}

void UYamlSerialization::CapitalizePropertyName(FString& Name, EYamlKeyCapitalization Capitalization) {
    // By default, the property name will be generated in PascalCase
    switch (Capitalization) {
        case EYamlKeyCapitalization::PascalCase: break;
        case EYamlKeyCapitalization::CamelCase: {
            Name[0] = FChar::ToLower(Name[0]);
            break;
        }
        case EYamlKeyCapitalization::SnakeCase: {
            // Insert an underscore before each capital letter and digit, except when there was a capital letter
            // before that. Also, no underscore before the first letter.
            TArray Chars = Name.GetCharArray();
            for (int32 i = 1; i < Chars.Num(); i++) {
                if ((FChar::IsUpper(Chars[i]) && FChar::IsLower(Chars[i - 1])) ||
                    (FChar::IsDigit(Chars[i]) && !FChar::IsDigit(Chars[i - 1]))) {
                    Chars.Insert('_', i);
                    i++;
                }
            }
            Name = Chars;

            // Then make the whole string lowercase
            Name.ToLowerInline();

            break;
        }
    }
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
        Result.AddError(TEXT("Value of type '%s' cannot be parsed as an enum value for %s"), *EnumToString(Node.Type()),
                        *Enum->CppType);
    }

    return INDEX_NONE;
}
