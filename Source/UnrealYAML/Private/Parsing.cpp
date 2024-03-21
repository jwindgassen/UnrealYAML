#include "Parsing.h"


DEFINE_LOG_CATEGORY(LogYamlParsing)


FYamlParseIntoOptions FYamlParseIntoOptions::Strict() {
    FYamlParseIntoOptions Ret;
    Ret.CheckTypes = true;
    Ret.CheckEnums = true;
    Ret.CheckRequired = true;
    Ret.CheckAdditionalProperties = true;
    return Ret;
}

FYamlParseIntoCtx& FYamlParseIntoCtx::PushStack(const TCHAR* Property) {
    Stack.Add(Property);
    return *this;
}

FYamlParseIntoCtx& FYamlParseIntoCtx::PushStack(const FYamlNode& Key) {
    Stack.Add(Key.Scalar());
    return *this;
}

FYamlParseIntoCtx& FYamlParseIntoCtx::PushStack(const int32 Index) {
    Stack.Add(FString::Printf(TEXT("[%d]"), Index));
    return *this;
}

void FYamlParseIntoCtx::PopStack() {
    Stack.RemoveAt(Stack.Num() - 1);
}

void FYamlParseIntoCtx::AddError(const TCHAR* Err) {
    Errors.Add(FString::Printf(TEXT("%s: %s"), *StackStr(), Err));
}

bool FYamlParseIntoCtx::Success() const {
    return Errors.Num() == 0;
}

FString FYamlParseIntoCtx::StackStr() const {
    return FString::Join(Stack, TEXT("."));
}

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
const TArray<FString> UYamlParsing::NativeTypes = {"FString", "FText", "FVector", "FVector2D", "FQuat", "FTransform", "FColor",
                                                   "FLinearColor", "FRotator"};

bool UYamlParsing::ParseIntoProperty(const FYamlNode& Node, const FProperty& Property, void* PropertyValue, FYamlParseIntoCtx& Ctx) {
    UE_LOG(LogYamlParsing, Verbose, TEXT("Parsing Node into Property '%s' of type '%s'"), *Property.GetName(),
           *Property.GetCPPType())

    // If we access an invalid sequence or map entry, we will get an Zombie Node, which is invalid
    if (!Node.IsDefined()) {
        return false;
    }

    if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(&Property)) {
        if (CheckEnumValue(Ctx, Node, EnumProperty->GetEnum())) {
            const int64 Index = EnumProperty->GetEnum()->GetIndexByNameString(Node.As<FString>());
            EnumProperty->GetUnderlyingProperty()->SetIntPropertyValue(PropertyValue, Index);
        }
    } else if (const FByteProperty* ByteProperty = CastField<FByteProperty>(&Property); ByteProperty && ByteProperty->GetIntPropertyEnum()) {
        if (CheckEnumValue(Ctx, Node, ByteProperty->GetIntPropertyEnum())) {
            const int64 Index = ByteProperty->GetIntPropertyEnum()->GetIndexByNameString(Node.As<FString>());
            ByteProperty->SetIntPropertyValue(PropertyValue, Index);
        }
    } else if (const FNumericProperty* NumericProperty = CastField<FNumericProperty>(&Property)) {
        if (NumericProperty->IsInteger()) {
            if (!CheckScalarCanConvert<int64>(Ctx, TEXT("integer"), Node)) {
                return false;
            }

            const auto Value = Node.AsOptional<int64>();
            if (Value.IsSet()) {
                NumericProperty->SetIntPropertyValue(PropertyValue, Value.GetValue());
            }
        } else {
            if (!CheckScalarCanConvert<float>(Ctx, TEXT("float"), Node)) {
                return false;
            }

            const auto Value = Node.AsOptional<float>();
            if (Value.IsSet()) {
                NumericProperty->SetFloatingPointPropertyValue(PropertyValue, Value.GetValue());
            }
        }
    } else if (const FBoolProperty* BoolProperty = CastField<FBoolProperty>(&Property)) {
        if (!CheckScalarCanConvert<bool>(Ctx, TEXT("bool"), Node)) {
            return false;
        }

        const auto Value = Node.AsOptional<bool>();
        if (Value.IsSet()) {
            BoolProperty->SetPropertyValue(PropertyValue, Value.GetValue());
        }
    } else if (const FStrProperty* StringProperty = CastField<FStrProperty>(&Property)) {
        if (!CheckScalarCanConvert<FString>(Ctx, TEXT("string"), Node)) {
            return false;
        }

        const auto Value = Node.AsOptional<FString>();
        if (Value.IsSet()) {
            *const_cast<FString*>(&StringProperty->GetPropertyValue(PropertyValue)) = Value.GetValue();
        }
    } else if (const FTextProperty* TextProperty = CastField<FTextProperty>(&Property)) {
        if (!CheckScalarCanConvert<FText>(Ctx, TEXT("string"), Node)) {
            return false;
        }

        const auto Value = Node.AsOptional<FText>();
        if (Value.IsSet()) {
            *const_cast<FText*>(&TextProperty->GetPropertyValue(PropertyValue)) = Value.GetValue();
        }
    } else if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(&Property)) {
        if (!CheckNodeType(Ctx, EYamlNodeType::Sequence, TEXT("sequence"), Node)) {
            return false;
        }

        // We need the helper to get to the items of the array
        FScriptArrayHelper Helper(ArrayProperty, PropertyValue);

        Helper.EmptyValues(); // Remove any values in the array already.

        Helper.AddValues(Node.Size());

        bool ParsedAllProperties = true;
        for (int32 i = 0; i < Helper.Num(); ++i) {
            if (!ParseIntoProperty(Node[i], *ArrayProperty->Inner, Helper.GetRawPtr(i), Ctx.PushStack(i))) {
                ParsedAllProperties = false;
            }

            Ctx.PopStack();
        }

        return ParsedAllProperties;
    } else if (const FSoftObjectProperty* SoftObjProperty = CastField<FSoftObjectProperty>(&Property)) {
        if (!CheckScalarCanConvert<FString>(Ctx, TEXT("string"), Node)) {
            return false;
        }

        const auto Value = Node.AsOptional<FString>();
        if (Value.IsSet()) {
            auto Object = FindObject<UObject>(Value.GetValue());
            if (!IsValid(Object)) {
                Ctx.AddError(*FString::Printf(TEXT("Cannot find object: %s"), *Value.GetValue()));
                return false;
            }

            SoftObjProperty->SetObjectPropertyValue(PropertyValue, Object);
        }
    } else if (const FClassProperty* ClassProperty = CastField<FClassProperty>(&Property)) {
        if (!CheckScalarCanConvert<FString>(Ctx, TEXT("string"), Node)) {
            return false;
        }

        const auto Value = Node.AsOptional<FString>();
        if (Value.IsSet()) {
            auto FoundClass = FindClass(Value.GetValue());
            if (!IsValid(FoundClass)) {
                Ctx.AddError(*FString::Printf(TEXT("Cannot find class: %s"), *Value.GetValue()));
                return false;
            }

            *const_cast<TObjectPtr<UObject>*>(&ClassProperty->GetPropertyValue(PropertyValue)) = FoundClass;
        }
    } else if (const FObjectProperty* ObjectProperty = CastField<FObjectProperty>(&Property)) {
        return ParseIntoObject(Node, ObjectProperty->PropertyClass, PropertyValue, Ctx);
    } else if (const FStructProperty* StructProperty = CastField<FStructProperty>(&Property)) {
        return ParseIntoStruct(Node, StructProperty->Struct, PropertyValue, Ctx);
    } else if (const FMapProperty* MapProperty = CastField<FMapProperty>(&Property)) {
        if (!CheckNodeType(Ctx, EYamlNodeType::Map, TEXT("map"), Node)) {
            return false;
        }

        FScriptMapHelper Helper(MapProperty, PropertyValue);
        Helper.EmptyValues(); // Remove any values in the map already.

        bool ParsedAllProperties = true;
        for(auto It=Node.begin(); It != Node.end(); ++It) {
            const auto i = Helper.AddDefaultValue_Invalid_NeedsRehash();

            Ctx.PushStack((*It).Key);

            if (!ParseIntoProperty((*It).Key, *Helper.KeyProp, Helper.GetKeyPtr(i), Ctx)) {
                ParsedAllProperties = false;
            }

            if (!ParseIntoProperty((*It).Value, *Helper.ValueProp, Helper.GetValuePtr(i), Ctx)) {
                ParsedAllProperties = false;
            }

            Ctx.PopStack();
        }

        Helper.Rehash();

        return ParsedAllProperties;
    }

    return true;
}

bool UYamlParsing::ParseIntoObject(const FYamlNode& Node, const UClass* Object, void* ObjectValue, FYamlParseIntoCtx& Ctx) {
    UE_LOG(LogYamlParsing, Verbose, TEXT("Parsing Node into Object '%s'"), *Object->GetName())

    if (!CheckNodeType(Ctx, EYamlNodeType::Map, TEXT("map"), Node)) {
        return false;
    }

    bool ParsedAllProperties = true;
    for (TFieldIterator<FProperty> It(Object); It; ++It) {
        FString Key = It->GetName();
        if (!ParseIntoProperty(Node[Key], **It, It->ContainerPtrToValuePtr<void>(ObjectValue), Ctx.PushStack(*Key))) {
            ParsedAllProperties = false;
        }

        Ctx.PopStack();
    }

    return ParsedAllProperties;
}

bool UYamlParsing::ParseIntoStruct(const FYamlNode& Node, const UScriptStruct* Struct, void* StructValue, FYamlParseIntoCtx& Ctx) {
    UE_LOG(LogYamlParsing, Verbose, TEXT("Parsing Node into Struct '%s'"), *Struct->GetName())

    // Check for custom handlers provided in options first.
    if (const auto CustomHandler = Ctx.Options.TypeHandlers.Find(Struct->GetStructCPPName()); CustomHandler != nullptr) {
        (*CustomHandler)(Node, Struct, StructValue, Ctx);
        return true;
    }

    if (NativeTypes.Contains(Struct->GetStructCPPName())) {
        return ParseIntoNativeType(Node, Struct, StructValue, Ctx);
    }

    if (!CheckNodeType(Ctx, EYamlNodeType::Map, TEXT("map"), Node)) {
        return false;
    }

    auto RemainingKeys = Node.Keys<FString>().Array();

    bool ParsedAllProperties = true;
    for (TFieldIterator<FProperty> It(Struct); It; ++It) {
        FString Key = It->GetName();

        Ctx.PushStack(*Key);

        if (It->HasMetaData(YamlRequiredSpecifier) && !Node[Key].IsDefined()) {
            Ctx.AddError(TEXT("yaml does not contain this required field"));
        } else if (!ParseIntoProperty(Node[Key], **It, It->ContainerPtrToValuePtr<void>(StructValue), Ctx)) {
            ParsedAllProperties = false;
        }

        const auto MatchingKey = RemainingKeys.IndexOfByPredicate([&Key](const FString& Element) {
            // Note we use == operator here to parallel how yaml-cpp finds keys internally.
            // For FString, this is a case-insensitive match.
            return Element == Key;
        });
        if (MatchingKey != INDEX_NONE) {
            RemainingKeys.RemoveAt(MatchingKey);
        }

        Ctx.PopStack();
    }

    if (Ctx.Options.CheckAdditionalProperties && RemainingKeys.Num()) {
        for (const auto Key : RemainingKeys) {
            Ctx.PushStack(*Key);
            Ctx.AddError(TEXT("additional property does not match a property in USTRUCT"));
            Ctx.PopStack();
        }
    }

    return ParsedAllProperties;
}

bool UYamlParsing::ParseIntoNativeType(const FYamlNode& Node, const UScriptStruct* Struct, void* StructValue, FYamlParseIntoCtx& Ctx) {
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
        checkf(false, TEXT("No native type conversion for %s"), *Type)
        return false;
    }

    return true;
}

bool UYamlParsing::CheckNodeType(FYamlParseIntoCtx& Ctx, const EYamlNodeType Expected, const TCHAR* TypeName,
    const FYamlNode& Node) {
    if (Ctx.Options.CheckTypes && Node.IsDefined() && Node.Type() != Expected) {
        Ctx.AddError(*FString::Printf(TEXT("value is not a %s"), TypeName));
        return false;
    }

    return true;
}

bool UYamlParsing::CheckEnumValue(FYamlParseIntoCtx& Ctx, const FYamlNode& Node, const UEnum* Enum) {
    if (!CheckScalarCanConvert<FString>(Ctx, TEXT("string"), Node)) {
        return false;
    }

    if (Ctx.Options.CheckEnums && Node.IsDefined()) {
        const FString Value(Node.As<FString>());

        if (Enum->GetIndexByNameString(Value) == INDEX_NONE) {
            Ctx.AddError(*FString::Printf(
                TEXT("\"%s\" is not an allowed value for enum %s"),
                *Value,
                *Enum->CppType));
            return false;
        }
    }

    return true;
}
