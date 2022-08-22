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
const TArray<FString> UYamlParsing::NativeTypes = {"FString", "FText", "FVector", "FQuat", "FTransform", "FColor", "FLinearColor"};

bool UYamlParsing::ParseIntoProperty(const FYamlNode& Node, const FProperty& Property, void* PropertyValue) {
	UE_LOG(LogYamlParsing, Verbose, TEXT("Parsing Node into Property '%s' of type '%s'"), *Property.GetName(), *Property.GetCPPType())
	
	if (const FNumericProperty* NumericProperty = CastField<FNumericProperty>(&Property)) {
		if (NumericProperty->IsInteger()) {
			const auto Value = Node.AsOptional<uint64>();
			if (Value.IsSet()) NumericProperty->SetIntPropertyValue(PropertyValue, Value.GetValue());
		} else {
			const auto Value = Node.AsOptional<float>();
			if (Value.IsSet()) NumericProperty->SetFloatingPointPropertyValue(PropertyValue, Value.GetValue());
		}
	}
	else if (const FBoolProperty* BoolProperty = CastField<FBoolProperty>(&Property)) {
		const auto Value = Node.AsOptional<bool>();
		if (Value.IsSet()) BoolProperty->SetPropertyValue(PropertyValue, Value.GetValue());
	}
	else if (const FStrProperty* StringProperty = CastField<FStrProperty>(&Property)) {
		const auto Value = Node.AsOptional<FString>();
		if (Value.IsSet()) *const_cast<FString*>(&StringProperty->GetPropertyValue(PropertyValue)) = Value.GetValue();
	}
	else if (const FTextProperty* TextProperty = CastField<FTextProperty>(&Property)) {
		const auto Value = Node.AsOptional<FText>();
		if (Value.IsSet()) *const_cast<FText*>(&TextProperty->GetPropertyValue(PropertyValue)) = Value.GetValue();
	}
	else if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(&Property)) {
		// We need the helper to get to the items of the array            
		FScriptArrayHelper Helper(ArrayProperty, PropertyValue);
		Helper.AddValues(Node.Size());
		
		for (int32 i = 0; i < Helper.Num(); ++i) {
			ParseIntoProperty(Node[i], *ArrayProperty->Inner, Helper.GetRawPtr(i));
		}
	}
	else if (const FObjectProperty* ObjectProperty = CastField<FObjectProperty>(&Property)) {
		ParseIntoObject(Node, *ObjectProperty, PropertyValue);
	}
	else if (const FStructProperty* StructProperty = CastField<FStructProperty>(&Property)) {
		ParseIntoStruct(Node, *StructProperty, PropertyValue);
	}
	
	return true;
}

bool UYamlParsing::ParseIntoObject(const FYamlNode& Node, const FObjectProperty& ObjectProperty, void* PropertyValue) {
	UE_LOG(LogYamlParsing, Verbose, TEXT("Parsing Node into Object '%s'"), *ObjectProperty.GetName())
	
	bool ParsedAllProperties = true;
	for (TFieldIterator<FProperty> It(ObjectProperty.PropertyClass); It; ++It) {
		FString Key = It->GetName();
		const bool Success = ParseIntoProperty(Node[Key], **It, It->ContainerPtrToValuePtr<void>(PropertyValue));
			
		if (!Success) ParsedAllProperties = false;
	}
	
	return ParsedAllProperties;
}

bool UYamlParsing::ParseIntoStruct(const FYamlNode& Node, const FStructProperty& StructProperty, void* PropertyValue) {
	UE_LOG(LogYamlParsing, Verbose, TEXT("Parsing Node into Struct '%s'"), *StructProperty.GetName())
	
	if (NativeTypes.Contains(StructProperty.Struct->GetStructCPPName())) {
		return ParseIntoNativeType(Node, StructProperty, PropertyValue);
	}
	
	bool ParsedAllProperties = true;
	for (TFieldIterator<FProperty> It(StructProperty.Struct); It; ++It) {
		FString Key = It->GetName();
		const bool Success = ParseIntoProperty(Node[Key], **It, It->ContainerPtrToValuePtr<void>(PropertyValue));
			
		if (!Success) ParsedAllProperties = false;
	}
	
	return ParsedAllProperties;
}

bool UYamlParsing::ParseIntoNativeType(const FYamlNode& Node, const FStructProperty& StructProperty, void* PropertyValue) {
	const FString Type = StructProperty.Struct->GetStructCPPName();

	if (Type == "FString") {
		*static_cast<FString*>(PropertyValue) = Node.As<FString>();
	} else if (Type == "FText") {
		*static_cast<FText*>(PropertyValue) = Node.As<FText>();
	} else if (Type == "FVector") {
		*static_cast<FVector*>(PropertyValue) = Node.As<FVector>();
	} else if (Type == "FQuat") {
		*static_cast<FQuat*>(PropertyValue) = Node.As<FQuat>();
	} else if (Type == "FTransform") {
		*static_cast<FTransform*>(PropertyValue) = Node.As<FTransform>();
	} else if (Type == "FColor") {
		*static_cast<FColor*>(PropertyValue) = Node.As<FColor>();
	} else if (Type == "FLinearColor") {
		*static_cast<FLinearColor*>(PropertyValue) = Node.As<FLinearColor>();
	} else {
		checkf(false, TEXT("No native type conversion for %s"), *Type)
	}

	return true;
}
