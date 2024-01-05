#include "Parsing.h"


DEFINE_LOG_CATEGORY(LogYamlParsing)


// Parsing into/from Files ---------------------------------------------------------------------------------------------
bool UYamlParsing::ParseYaml(const FString String, FYamlNode& Out) {
    try {
        Out = FYamlNode(YAML::Load(TCHAR_TO_UTF8(*String)));
        return true;
    } catch (YAML::ParserException) {
        return false;
    }
}

bool UYamlParsing::LoadYamlFromFile(const FString Path, FYamlNode& Out) {
    FString Contents;
    if (FFileHelper::LoadFileToString(Contents, *Path)) {
        return ParseYaml(Contents, Out);
    }
    return false;
}

void UYamlParsing::WriteYamlToFile(const FString Path, const FYamlNode Node) {
    FFileHelper::SaveStringToFile(Node.GetContent(), *Path);
}


// Parsing into Structs ------------------------------------------------------------------------------------------------
const TArray<FString> UYamlParsing::NativeTypes = {"FString", "FText", "FVector", "FQuat", "FTransform", "FColor",
                                                   "FLinearColor"};

bool UYamlParsing::ParseIntoProperty(const FYamlNode& Node, const FProperty& Property, void* PropertyValue) {
    UE_LOG(LogYamlParsing, Verbose, TEXT("Parsing Node into Property '%s' of type '%s'"), *Property.GetName(),
           *Property.GetCPPType())

    // If we access an invalid sequence or map entry, we will get an Zombie Node, which is invalid
    if (!Node.IsDefined()) {
        return false;
    }

    if (const FNumericProperty* NumericProperty = CastField<FNumericProperty>(&Property)) {
        if (NumericProperty->IsInteger()) {
            const auto Value = Node.AsOptional<uint64>();
            if (Value.IsSet()) {
                NumericProperty->SetIntPropertyValue(PropertyValue, Value.GetValue());
            }
        } else {
            const auto Value = Node.AsOptional<float>();
            if (Value.IsSet()) {
                NumericProperty->SetFloatingPointPropertyValue(PropertyValue, Value.GetValue());
            }
        }
    } else if (const FBoolProperty* BoolProperty = CastField<FBoolProperty>(&Property)) {
        const auto Value = Node.AsOptional<bool>();
        if (Value.IsSet()) {
            BoolProperty->SetPropertyValue(PropertyValue, Value.GetValue());
        }
    } else if (const FStrProperty* StringProperty = CastField<FStrProperty>(&Property)) {
        const auto Value = Node.AsOptional<FString>();
        if (Value.IsSet()) {
            *const_cast<FString*>(&StringProperty->GetPropertyValue(PropertyValue)) = Value.GetValue();
        }
    } else if (const FTextProperty* TextProperty = CastField<FTextProperty>(&Property)) {
        const auto Value = Node.AsOptional<FText>();
        if (Value.IsSet()) {
            *const_cast<FText*>(&TextProperty->GetPropertyValue(PropertyValue)) = Value.GetValue();
        }
    } else if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(&Property)) {
        const int64 Index = EnumProperty->GetEnum()->GetIndexByNameString(Node.As<FString>());
        EnumProperty->GetUnderlyingProperty()->SetIntPropertyValue(PropertyValue, Index);
    } else if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(&Property)) {
        // We need the helper to get to the items of the array            
        FScriptArrayHelper Helper(ArrayProperty, PropertyValue);
        Helper.AddValues(Node.Size());

        bool ParsedAllProperties = true;
        for (int32 i = 0; i < Helper.Num(); ++i) {
            if (!ParseIntoProperty(Node[i], *ArrayProperty->Inner, Helper.GetRawPtr(i))) {
                ParsedAllProperties = false;
            }
        }

        return ParsedAllProperties;
    } else if (const FObjectProperty* ObjectProperty = CastField<FObjectProperty>(&Property)) {
        return ParseIntoObject(Node, ObjectProperty->PropertyClass, PropertyValue);
    } else if (const FStructProperty* StructProperty = CastField<FStructProperty>(&Property)) {
        return ParseIntoStruct(Node, StructProperty->Struct, PropertyValue);
    } else if (const FMapProperty* MapProperty = CastField<FMapProperty>(&Property)) {
        FScriptMapHelper Helper(MapProperty, PropertyValue);

        bool ParsedAllProperties = true;
        for(auto It=Node.begin(); It != Node.end(); ++It) {
            const auto i = Helper.AddDefaultValue_Invalid_NeedsRehash();

            if (!ParseIntoProperty((*It).Key, *Helper.KeyProp, Helper.GetKeyPtr(i))) {
                ParsedAllProperties = false;
            }

            if (!ParseIntoProperty((*It).Value, *Helper.ValueProp, Helper.GetValuePtr(i))) {
                ParsedAllProperties = false;
            }
        }

        Helper.Rehash();

        return ParsedAllProperties;
    }

    return true;
}

bool UYamlParsing::ParseIntoObject(const FYamlNode& Node, const UClass* Object, void* ObjectValue) {
    UE_LOG(LogYamlParsing, Verbose, TEXT("Parsing Node into Object '%s'"), *Object->GetName())

    bool ParsedAllProperties = true;
    for (TFieldIterator<FProperty> It(Object); It; ++It) {
        FString Key = It->GetName();
        if (!ParseIntoProperty(Node[Key], **It, It->ContainerPtrToValuePtr<void>(ObjectValue))) {
            ParsedAllProperties = false;
        }
    }

    return ParsedAllProperties;
}

bool UYamlParsing::ParseIntoStruct(const FYamlNode& Node, const UScriptStruct* Struct, void* StructValue) {
    UE_LOG(LogYamlParsing, Verbose, TEXT("Parsing Node into Struct '%s'"), *Struct->GetName())

    if (NativeTypes.Contains(Struct->GetStructCPPName())) {
        return ParseIntoNativeType(Node, Struct, StructValue);
    }

    bool ParsedAllProperties = true;
    for (TFieldIterator<FProperty> It(Struct); It; ++It) {
        FString Key = It->GetName();
        if (!ParseIntoProperty(Node[Key], **It, It->ContainerPtrToValuePtr<void>(StructValue))) {
            ParsedAllProperties = false;
        }
    }

    return ParsedAllProperties;
}

bool UYamlParsing::ParseIntoNativeType(const FYamlNode& Node, const UScriptStruct* Struct, void* StructValue) {
    const FString Type = Struct->GetStructCPPName();

    if (Type == "FString") {
        *static_cast<FString*>(StructValue) = Node.As<FString>();
    } else if (Type == "FText") {
        *static_cast<FText*>(StructValue) = Node.As<FText>();
    } else if (Type == "FVector") {
        *static_cast<FVector*>(StructValue) = Node.As<FVector>();
    } else if (Type == "FQuat") {
        *static_cast<FQuat*>(StructValue) = Node.As<FQuat>();
    } else if (Type == "FTransform") {
        *static_cast<FTransform*>(StructValue) = Node.As<FTransform>();
    } else if (Type == "FColor") {
        *static_cast<FColor*>(StructValue) = Node.As<FColor>();
    } else if (Type == "FLinearColor") {
        *static_cast<FLinearColor*>(StructValue) = Node.As<FLinearColor>();
    } else {
        checkf(false, TEXT("No native type conversion for %s"), *Type)
        return false;
    }

    return true;
}
