#include "UnrealYAMLNode.h"

EYamlNodeType FYamlNode::Type() const {
    try {
        return static_cast<EYamlNodeType>(Node.Type());
    } catch (YAML::InvalidNode) {
        UE_LOG(LogTemp, Warning, TEXT("Node was Invalid, returning default value for Type()!"))
        return EYamlNodeType::Undefined;
    }
}

EYamlEmitterStyle FYamlNode::Style() const {
    try {
        return static_cast<EYamlEmitterStyle>(Node.Style());
    } catch (YAML::InvalidNode) {
        UE_LOG(LogTemp, Warning, TEXT("Node was Invalid, returning default value for Style()!"))
        return EYamlEmitterStyle::Default;
    }
}

void FYamlNode::SetStyle(const EYamlEmitterStyle Style) {
    Node.SetStyle(static_cast<YAML::EmitterStyle>(Style));
}

bool FYamlNode::Is(const FYamlNode& Other) const {
    try {
        return Node.is(Other.Node);
    } catch (YAML::InvalidNode) {
        UE_LOG(LogTemp, Warning, TEXT("Node was Invalid, returning default value for Is() / Equals-Operation!"))
        return false;
    }
}

bool FYamlNode::Reset(const FYamlNode& Other) {
    try {
        Node.reset(Other.Node);
        return true;
    } catch (YAML::InvalidNode) {
        UE_LOG(LogTemp, Warning, TEXT("Node was Invalid and will not be Reset!"))
        return false;
    }
}

FString FYamlNode::Scalar() const {
    try {
        return FString(Node.Scalar().c_str());
    } catch (YAML::InvalidNode) {
        UE_LOG(LogTemp, Warning, TEXT("Node was Invalid, returning default value for Scalar()"))
        return "";
    }
}

FString FYamlNode::GetContent() const {
    std::stringstream Stream;
    Stream << Node;
    return UTF8_TO_TCHAR(Stream.str().c_str());
}

int32 FYamlNode::Size() const {
    try {
        return Node.size();
    } catch (YAML::InvalidNode) {
        UE_LOG(LogTemp, Warning, TEXT("Node was Invalid, returning default value for Size()"))
        return 0;
    }
}
