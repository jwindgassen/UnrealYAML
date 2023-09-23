#pragma once

#include "node/convert.h"

static const TMap<FString, FColor> ColorMap = {
    {"Red", FColor::Red},
    {"Yellow", FColor::Yellow},
    {"Green", FColor::Green},
    {"Blue", FColor::Blue},
    {"White", FColor::White},
    {"Black", FColor::Black},
    {"Transparent", FColor::Transparent},
    {"Cyan", FColor::Cyan},
    {"Magenta", FColor::Magenta},
    {"Orange", FColor::Orange},
    {"Purple", FColor::Purple},
    {"Turquoise", FColor::Turquoise},
    {"Silver", FColor::Silver},
    {"Emerald", FColor::Emerald}
};


namespace YAML {
// encode and decode an FString
template<>
struct convert<FString> {
    static Node encode(const FString& String) {
        return Node(std::string(TCHAR_TO_UTF8(*String)));
    }

    static bool decode(const Node& Node, FString& Out) {
        if (!Node.IsScalar()) {
            return false;
        }

        Out = UTF8_TO_TCHAR(Node.as<std::string>().c_str());
        return true;
    }
};


// encode and decode an FText
template<>
struct convert<FText> {
    static Node encode(const FText& Text) {
        return Node(Text.ToString());
    }

    static bool decode(const Node& Node, FText& Out) {
        if (!Node.IsScalar()) {
            return false;
        }

        Out = FText::FromString(Node.as<FString>());
        return true;
    }
};


// encode FColor and FLinearColor as Vector or String
template<>
struct convert<FColor> {
    static Node encode(const FColor& Color) {
        for (const auto& Pair : ColorMap) {
            if (Color == Pair.Value) {
                return Node(Pair.Key);
            }
        }
        Node node(TArray<int>({Color.R, Color.G, Color.B, Color.A}));
        node.SetStyle(EmitterStyle::Flow);
        return node;
    }

    static bool decode(const Node& Node, FColor& Out) {
        for (const auto& Pair : ColorMap) {
            if (Node.as<FString>() == Pair.Key) {
                Out = Pair.Value;
                return true;
            }
        }

        if (Node.Type() != NodeType::Sequence || (Node.size() != 3 && Node.size() != 4)) {
            return false;
        }


        const float A = Node.size() == 4 ? Node[3].as<uint8>() : 1.f;
        Out = FColor(Node[0].as<uint8>(), Node[1].as<uint8>(), Node[2].as<uint8>(), A);
        return true;
    }
};

template<>
struct convert<FLinearColor> {
    static Node encode(const FLinearColor& Color) {
        return convert<FColor>::encode(Color.ToFColor(true));
    }

    static bool decode(const Node& Node, FLinearColor& Out) {
        FColor Color;
        const bool Success = convert<FColor>::decode(Node, Color);
        Out = Color.ReinterpretAsLinear();
        return Success;
    }
};


// encode and decode an FVector
template<>
struct convert<FVector> {
    static Node encode(const FVector& Vector) {
        Node Node;
        Node.SetStyle(EmitterStyle::Flow);
        Node.push_back(Vector.X);
        Node.push_back(Vector.Y);
        Node.push_back(Vector.Z);
        return Node;
    }

    static bool decode(const Node& Node, FVector& Out) {
        if (Node.IsSequence() && Node.size() == 3) {
            Out.X = Node[0].as<double>();
            Out.Y = Node[1].as<double>();
            Out.Z = Node[2].as<double>();
            return true;
        }

        // Constant Vector
        if (Node.IsScalar()) {
            Out.X = Out.Y = Out.Z = Node.as<double>();
        }

        return false;
    }
};


// encode and decode an FQuad
// We accept 3-Component and 4-Component Vectors as a Rotation, but we only ever emit Quaternions
template<>
struct convert<FQuat> {
    static Node encode(const FQuat& Quad) {
        Node Node;
        Node.SetStyle(EmitterStyle::Flow);
        Node.push_back(Quad.X);
        Node.push_back(Quad.Y);
        Node.push_back(Quad.Z);
        Node.push_back(Quad.W);
        return Node;
    }

    static bool decode(const Node& Node, FQuat& Out) {
        if (Node.IsSequence()) {
            if (Node.size() == 4) {
                Out.X = Node[0].as<double>();
                Out.Y = Node[1].as<double>();
                Out.Z = Node[2].as<double>();
                Out.W = Node[3].as<double>();
                return true;
            }

            if (Node.size() == 3) {
                Out = FRotator(Node[0].as<double>(), Node[1].as<double>(), Node[2].as<double>()).Quaternion();
                return true;
            }
        }

        return false;
    }
};

// encode and decode an FRotator
// We accept 3-Component Pitch, Roll, Yaw Vectors
template<>
struct convert<FRotator> {
    static Node encode(const FRotator& Rotator) {
        Node Node;
        Node.SetStyle(EmitterStyle::Flow);
        Node.push_back(Rotator.Pitch);
        Node.push_back(Rotator.Roll);
        Node.push_back(Rotator.Yaw);

        return Node;
    }

    static bool decode(const Node& Node, FRotator& Out) {
        if (Node.IsSequence() && Node.size() == 3) {
            Out.Pitch = Node[0].as<double>();
            Out.Roll = Node[1].as<double>();
            Out.Yaw = Node[2].as<double>();

            return true;
        }

        return false;
    }
};

// encode and decode an FTransform
template<>
struct convert<FTransform> {
    static Node encode(const FTransform& Transform) {
        Node Node;
        Node.SetStyle(EmitterStyle::Flow);
        Node.push_back(Transform.GetLocation());
        Node.push_back(Transform.GetRotation());
        Node.push_back(Transform.GetScale3D());
        return Node;
    }

    static bool decode(const Node& Node, FTransform& Out) {
        if (!Node.IsSequence() || Node.size() != 3) {
            return false;
        }

        Out.SetTranslation(Node[0].as<FVector>());
        Out.SetRotation(Node[1].as<FQuat>());
        Out.SetScale3D(Node[2].as<FVector>());
        return true;
    }
};


// encode and decode an TArray to a sequence
template<class T>
struct convert<TArray<T>> {
    static Node encode(const TArray<T> Array) {
        Node Node(NodeType::Sequence);
        for (const T& Element : Array) {
            Node.push_back(Element);
        }
        return Node;
    }

    static bool decode(const Node& Node, TArray<T>& Out) {
        if (!(Node.Type() == NodeType::Map || Node.Type() == NodeType::Sequence)) {
            return false;
        }

        Out = {};
        for (const_iterator Iterator = Node.begin(); Iterator != Node.end(); ++Iterator) {
            Out.Add(Iterator->as<T>());
        }

        return true;
    }
};


// encode and decode an TArray to a sequence
template<class T>
struct convert<TSet<T>> {
    static Node encode(const TSet<T> Array) {
        Node Node(NodeType::Sequence);
        for (const T& Element : Array) {
            Node.push_back(Element);
        }
        return Node;
    }

    static bool decode(const Node& Node, TSet<T>& Out) {
        if (!(Node.Type() == NodeType::Map || Node.Type() == NodeType::Sequence)) {
            return false;
        }

        Out = {};
        for (const_iterator Iterator = Node.begin(); Iterator != Node.end(); ++Iterator) {
            Out.Add(Iterator->as<T>());
        }

        return true;
    }
};


// encode and decode an TMap to a Map
template<class TKey, class TValue>
struct convert<TMap<TKey, TValue>> {
    static Node encode(const TMap<TKey, TValue> Map) {
        Node Node(NodeType::Map);

        for (const TTuple<TKey, TValue> Element : Map) {
            Node[Element.Key] = Element.Value;
        }
        return Node;
    }

    static bool decode(const Node& Node, TMap<TKey, TValue>& Out) {
        if (Node.Type() != NodeType::Map) {
            return false;
        }

        Out = {};
        for (const_iterator Iterator = Node.begin(); Iterator != Node.end(); ++Iterator) {
            Out.Add(Iterator->first.as<TKey>(), Iterator->second.as<TValue>());
        }

        return true;
    }
};
}
