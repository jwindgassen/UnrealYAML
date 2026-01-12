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
// Constant Types


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

// encode and decode an FName
template<>
struct convert<FName> {
    static Node encode(const FName& Name) {
        return Node(Name.ToString());
    }

    static bool decode(const Node& Node, FName& Out) {
        if (!Node.IsScalar()) {
            return false;
        }

        Out = FName(Node.as<FString>());
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
        if (Node.Type() == NodeType::Scalar) {
            for (const auto& Pair : ColorMap) {
                if (Node.as<FString>() == Pair.Key) {
                    Out = Pair.Value;
                    return true;
                }
            }
            return false;
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


// Container Types


#if ENGINE_MAJOR_VERSION >= 5

namespace detail {
// encode and decode an 2D Vector
template<template<typename> class ContainerType, typename InnerType>
struct convert2dBase {
    static Node encode(const ContainerType<InnerType>& Vector) {
        Node Node;
        Node.SetStyle(EmitterStyle::Flow);
        Node.push_back(Vector.X);
        Node.push_back(Vector.Y);
        return Node;
    }

    static bool decode(const Node& Node, ContainerType<InnerType>& Out) {
        if (Node.IsSequence() && Node.size() == 2) {
            Out.X = Node[0].as<InnerType>();
            Out.Y = Node[1].as<InnerType>();
            return true;
        }

        // Constant Vector
        if (Node.IsScalar()) {
            Out.X = Out.Y = Node.as<InnerType>();
        }

        return false;
    }
};


// encode and decode an 3D Vector
template<template<typename> class ContainerType, typename InnerType>
struct convert3dBase {
    static Node encode(const ContainerType<InnerType>& Vector) {
        Node Node;
        Node.SetStyle(EmitterStyle::Flow);
        Node.push_back(Vector.X);
        Node.push_back(Vector.Y);
        Node.push_back(Vector.Z);
        return Node;
    }

    static bool decode(const Node& Node, ContainerType<InnerType>& Out) {
        if (Node.IsSequence() && Node.size() == 3) {
            Out.X = Node[0].as<InnerType>();
            Out.Y = Node[1].as<InnerType>();
            Out.Z = Node[2].as<InnerType>();
            return true;
        }

        // Constant Vector
        if (Node.IsScalar()) {
            Out.X = Out.Y = Out.Z = Node.as<InnerType>();
        }

        return false;
    }
};

// encode and decode an 4D Vector
template<template<typename> class ContainerType, typename InnerType>
struct convert4dBase {
    static Node encode(const ContainerType<InnerType>& Vector) {
        Node Node;
        Node.SetStyle(EmitterStyle::Flow);
        Node.push_back(Vector.X);
        Node.push_back(Vector.Y);
        Node.push_back(Vector.Z);
        Node.push_back(Vector.W);
        return Node;
    }

    static bool decode(const Node& Node, ContainerType<InnerType>& Out) {
        if (Node.IsSequence() && Node.size() == 4) {
            Out.X = Node[0].as<InnerType>();
            Out.Y = Node[1].as<InnerType>();
            Out.Z = Node[2].as<InnerType>();
            Out.W = Node[3].as<InnerType>();
            return true;
        }

        // Constant Vector
        if (Node.IsScalar()) {
            Out.X = Out.Y = Out.Z = Out.W = Node.as<InnerType>();
        }

        return false;
    }
};
}

// encode and decode 2D Float Vector
template<typename FloatType>
struct convert<UE::Math::TVector2<FloatType>> : detail::convert2dBase<UE::Math::TVector2, FloatType> {};

// encode and decode 2D Int Vector
template<typename IntType>
struct convert<UE::Math::TIntVector2<IntType>> : detail::convert2dBase<UE::Math::TIntVector2, IntType> {};

// encode and decode 3D Float Vector
template<typename FloatType>
struct convert<UE::Math::TVector<FloatType>> : detail::convert3dBase<UE::Math::TVector, FloatType> {};

// encode and decode 3D Int Vector
template<typename IntType>
struct convert<UE::Math::TIntVector3<IntType>> : detail::convert3dBase<UE::Math::TIntVector3, IntType> {};

// encode and decode 4D Float Vector
template<typename FloatType>
struct convert<UE::Math::TVector4<FloatType>> : detail::convert4dBase<UE::Math::TVector4, FloatType> {};

// encode and decode 4D Int Vector
template<typename IntType>
struct convert<UE::Math::TIntVector4<IntType>> : detail::convert4dBase<UE::Math::TIntVector4, IntType> {};

// encode and decode an FQuad
// We accept 3-Component and 4-Component Vectors as a Rotation, but we only ever emit Quaternions
template<typename FloatType>
struct convert<UE::Math::TQuat<FloatType>> {
    static Node encode(const UE::Math::TQuat<FloatType>& Quad) {
        Node Node;
        Node.SetStyle(EmitterStyle::Flow);
        Node.push_back(Quad.X);
        Node.push_back(Quad.Y);
        Node.push_back(Quad.Z);
        Node.push_back(Quad.W);
        return Node;
    }

    static bool decode(const Node& Node, UE::Math::TQuat<FloatType>& Out) {
        if (Node.IsSequence()) {
            if (Node.size() == 4) {
                Out.X = Node[0].as<FloatType>();
                Out.Y = Node[1].as<FloatType>();
                Out.Z = Node[2].as<FloatType>();
                Out.W = Node[3].as<FloatType>();
                return true;
            }

            if (Node.size() == 3) {
                Out = Node.as<UE::Math::TRotator<FloatType>>().Quaternion();
                return true;
            }
        }

        return false;
    }
};

// encode and decode a 4x4 Matrix
// We accept 16-Component and 4x4-Component Vectors, but we only ever emit a 4x4 nested Sequence
template<typename FloatType>
struct convert<UE::Math::TMatrix<FloatType>> {
    static Node encode(const UE::Math::TMatrix<FloatType>& Matrix) {
        Node Node;

        Node.SetStyle(EmitterStyle::Block);
        for (unsigned i = 0; i < 4; ++i) {
            Node.push_back(Matrix.M[i]);
            Node[i].SetStyle(EmitterStyle::Flow);
        }

        return Node;
    }

    static bool decode(const Node& Node, UE::Math::TMatrix<FloatType>& Out) {
        if (Node.IsSequence()) {
            if (Node.size() == 4) {
                for (unsigned i = 0; i < 4; ++i) {
                    if (Node[i].size() != 4) {
                        return false;
                    }

                    FloatType Array[4];
                    memcpy(Array, Node[i].begin(), sizeof(FloatType) * 4);
                    Out.M[i] = Array;
                }
            }

            if (Node.size() == 16) {
                memcpy(Out.M, Node.begin(), sizeof(FloatType) * 16);
            }
        }

        return false;
    }
};

// encode and decode Plane
template<typename FloatType>
struct convert<UE::Math::TPlane<FloatType>> : detail::convert4dBase<UE::Math::TPlane, FloatType> {};

// encode and decode a Transform
template<typename FloatType>
struct convert<UE::Math::TTransform<FloatType>> {
    static Node encode(const UE::Math::TTransform<FloatType>& Transform) {
        Node Node;
        Node.SetStyle(EmitterStyle::Flow);
        Node.push_back(Transform.GetLocation());
        Node.push_back(Transform.GetRotation());
        Node.push_back(Transform.GetScale3D());
        return Node;
    }

    static bool decode(const Node& Node, UE::Math::TTransform<FloatType>& Out) {
        if (!Node.IsSequence() || Node.size() != 3) {
            return false;
        }

        Out.SetTranslation(Node[0].as<UE::Math::TVector<FloatType>>());
        Out.SetRotation(Node[1].as<UE::Math::TQuat<FloatType>>());
        Out.SetScale3D(Node[2].as<UE::Math::TVector<FloatType>>());
        return true;
    }
};

// encode and decode an Rotator
template<typename FloatType>
struct convert<UE::Math::TRotator<FloatType>> {
    static Node encode(const UE::Math::TRotator<FloatType>& Rotator) {
        Node Node;
        Node.SetStyle(EmitterStyle::Flow);
        Node.push_back(Rotator.Pitch);
        Node.push_back(Rotator.Roll);
        Node.push_back(Rotator.Yaw);

        return Node;
    }

    static bool decode(const Node& Node, UE::Math::TRotator<FloatType>& Out) {
        if (Node.IsSequence() && Node.size() == 3) {
            Out.Pitch = Node[0].as<FloatType>();
            Out.Roll = Node[1].as<FloatType>();
            Out.Yaw = Node[2].as<FloatType>();

            return true;
        }

        return false;
    }
};

#else

namespace detail {
    // encode and decode an 2D Vector
    template<class ContainerType, typename InnerType>
    struct convert2dBase {
        static Node encode(const ContainerType& Vector) {
            Node Node;
            Node.SetStyle(EmitterStyle::Flow);
            Node.push_back(Vector.X);
            Node.push_back(Vector.Y);
            return Node;
        }

        static bool decode(const Node& Node, ContainerType& Out) {
            if (Node.IsSequence() && Node.size() == 2) {
                Out.X = Node[0].as<InnerType>();
                Out.Y = Node[1].as<InnerType>();
                return true;
            }

            // Constant Vector
            if (Node.IsScalar()) {
                Out.X = Out.Y = Node.as<InnerType>();
            }

            return false;
        }
    };
        

    // encode and decode an 3D Vector
    template<class ContainerType, typename InnerType>
    struct convert3dBase {
        static Node encode(const ContainerType& Vector) {
            Node Node;
            Node.SetStyle(EmitterStyle::Flow);
            Node.push_back(Vector.X);
            Node.push_back(Vector.Y);
            Node.push_back(Vector.Z);
            return Node;
        }

        static bool decode(const Node& Node, ContainerType& Out) {
            if (Node.IsSequence() && Node.size() == 3) {
                Out.X = Node[0].as<InnerType>();
                Out.Y = Node[1].as<InnerType>();
                Out.Z = Node[2].as<InnerType>();
                return true;
            }

            // Constant Vector
            if (Node.IsScalar()) {
                Out.X = Out.Y = Out.Z = Node.as<InnerType>();
            }

            return false;
        }
    };

    // encode and decode an 4D Vector
    template<class ContainerType, typename InnerType>
    struct convert4dBase {
        static Node encode(const ContainerType& Vector) {
            Node Node;
            Node.SetStyle(EmitterStyle::Flow);
            Node.push_back(Vector.X);
            Node.push_back(Vector.Y);
            Node.push_back(Vector.Z);
            Node.push_back(Vector.W);
            return Node;
        }

        static bool decode(const Node& Node, ContainerType& Out) {
            if (Node.IsSequence() && Node.size() == 4) {
                Out.X = Node[0].as<InnerType>();
                Out.Y = Node[1].as<InnerType>();
                Out.Z = Node[2].as<InnerType>();
                Out.W = Node[3].as<InnerType>();
                return true;
            }

            // Constant Vector
            if (Node.IsScalar()) {
                Out.X = Out.Y = Out.Z = Out.W = Node.as<InnerType>();
            }

            return false;
        }
    };
}
    
// encode and decode an 2D float Vector
template<>
struct convert<FVector2D> : detail::convert2dBase<FVector2D, float> {};
    
// encode and decode an 3D float Vector
template<>
struct convert<FVector> : detail::convert3dBase<FVector, float> {};

// encode and decode an 3D int Vector
template<>
struct convert<FIntVector> : detail::convert3dBase<FIntVector, int32> {};
    
// encode and decode an 4D float Vector
template<>
struct convert<FVector4> : detail::convert3dBase<FVector4, float> {};

// encode and decode an 4D int Vector
template<>
struct convert<FIntVector4> : detail::convert3dBase<FIntVector4, int32> {};

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
                Out = Node.as<FRotator>().Quaternion();
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

#endif

// encode and decode an TArray to a sequence
template<class InnerType>
struct convert<TArray<InnerType>> {
    static Node encode(const TArray<InnerType> Array) {
        Node Node(NodeType::Sequence);
        for (const InnerType& Element : Array) {
            Node.push_back(Element);
        }
        return Node;
    }

    static bool decode(const Node& Node, TArray<InnerType>& Out) {
        if (!(Node.Type() == NodeType::Map || Node.Type() == NodeType::Sequence)) {
            return false;
        }

        Out = {};
        for (const_iterator Iterator = Node.begin(); Iterator != Node.end(); ++Iterator) {
            Out.Add(Iterator->as<InnerType>());
        }

        return true;
    }
};

// encode and decode an Set
template<typename InnerType>
struct convert<TSet<InnerType>> {
    static Node encode(const TSet<InnerType>& Set) {
        Node Node(NodeType::Sequence);
        for (const InnerType& Element : Set) {
            Node.push_back(Element);
        }
        return Node;
    }

    static bool decode(const Node& Node, TSet<InnerType>& Out) {
        if (!(Node.Type() == NodeType::Map || Node.Type() == NodeType::Sequence)) {
            return false;
        }

        Out = {};
        for (const_iterator Iterator = Node.begin(); Iterator != Node.end(); ++Iterator) {
            Out.Add(Iterator->as<InnerType>());
        }

        return true;
    }
};

// encode and decode an Map
template<typename KeyType, typename ValueType>
struct convert<TMap<KeyType, ValueType>> {
    static Node encode(const TMap<KeyType, ValueType>& Map) {
        Node Node(NodeType::Map);

        for (const TTuple<KeyType, ValueType> Element : Map) {
            Node[Element.Key] = Element.Value;
        }
        return Node;
    }

    static bool decode(const Node& Node, TMap<KeyType, ValueType>& Out) {
        if (Node.Type() != NodeType::Map) {
            return false;
        }

        Out = {};
        for (const_iterator Iterator = Node.begin(); Iterator != Node.end(); ++Iterator) {
            Out.Add(Iterator->first.as<KeyType>(), Iterator->second.as<ValueType>());
        }

        return true;
    }
};
}
