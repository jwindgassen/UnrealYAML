#include "Parse.h"

#include "HAL/FileManagerGeneric.h"
#include "FYamlNode.h"

bool ParseYaml(const FString String, FYamlNode& Out) {
	try {
		Out = FYamlNode(YAML::Load(TCHAR_TO_UTF8(*String)));
		return true;
	} catch (YAML::ParserException) {
		return false;
	}
}

bool LoadYamlFromFile(const FString Path, FYamlNode& Out) {
	FString Contents;
	if (FFileHelper::LoadFileToString(Contents, *Path)) {
		return ParseYaml(Contents, Out);
	}

	return false;
}

void WriteYamlToFile(const FString Path, const FYamlNode Node) {
	FFileHelper::SaveStringToFile(Node.GetContent(), *Path);
}
