#pragma once

#include "CoreMinimal.h"
#include "yaml.h"
#include "UnrealTypes.h"
#include "Enums.h"
#include "Emitter.h"

#include "YamlNode.generated.h"


/** A wrapper for the Yaml Node class. Base YAML class. Stores a YAML-Structure in a Tree-like hierarchy.
 * Can therefore either hold a single value or be a Container for other Nodes.
 * Conversion from one Type to another will be done automatically as needed
 */
USTRUCT(BlueprintType)
struct UNREALYAML_API FYamlNode {
    GENERATED_BODY()

private:
    friend void operator<<(std::ostream& Out, const FYamlNode& Node);
    friend void operator<<(FYamlEmitter& Out, const FYamlNode& Node);

    /** Forward Input Iterator Base to iterate over Nodes.
     *
     * The native yaml-cpp iterator return either a Node or a Pair of Nodes, depending if the iterated Node is a Sequence
     * or a Map. We extend this behaviour and always return a Pair of Nodes, the first being the Index in case of a Sequence
     */
    template<typename NodeType>
    class TIteratorBase {
        friend FYamlNode;

        static_assert(std::is_same_v<std::remove_const_t<NodeType>, FYamlNode>); // Only allow FYamlNodes
        using IteratorType = std::conditional_t<
            std::is_const_v<NodeType>,
            YAML::const_iterator,
            YAML::iterator
        >;

        IteratorType Iterator;
        int32 Index;

        TIteratorBase(IteratorType&& Iter) :
            Iterator(std::move(Iter)),
            Index(0) {}

    public:
        /** Returns the Key Element of the Key-Value-Pair if the Iterated Node is a Map
        * or a Node containing the <b>Index</b> of the Value if the Iterated Node is a <b>List</b>!
        *
        * The corresponding Value can be retrieved via Value() */
        NodeType Key() const {
            return Iterator->first.IsDefined() ? FYamlNode(Iterator->first) : FYamlNode(Index);
        }


        /** Returns the <b>Value</b> Element of the Key-Value-Pair if the Iterated Node is a <b>Map</b>
        * or a Node containing the <b>Value</b> if the Iterated Node is a <b>List</b>!
        *
        * The corresponding Key (for a Map) or Index (for a List) can be retrieved via Key() */
        NodeType Value() const {
            return Iterator->second.IsDefined() ? FYamlNode(Iterator->second) : FYamlNode(*Iterator);
        }

        /** Dereferencing the Iterator yields the Key-Value Pair */
        TPair<NodeType, NodeType> operator*() const {
            return MakeTuple(Key(), Value());
        }


        /** The Arrow Operator yields a Pointer to the Value */
        TPair<NodeType, NodeType>* operator->() const {
            return &MakeTuple(Key(), Value());
        }


        TIteratorBase& operator++() {
            ++Iterator;
            Index++;
            return *this;
        }


        TIteratorBase operator++(int) {
            TIteratorBase Pre(*this);
            ++(*this);
            Index++;
            return Pre;
        }

        friend bool operator ==(const TIteratorBase& This, const TIteratorBase Other) {
            return This.Iterator == Other.Iterator;
        }

        friend bool operator !=(const TIteratorBase& This, const TIteratorBase Other) {
            return This.Iterator != Other.Iterator;
        }
    };

    using FIterator = TIteratorBase<FYamlNode>;
    using FIteratorConst = TIteratorBase<const FYamlNode>;
    
    YAML::Node Node;

public:
    // Constructors --------------------------------------------------------------------
    /** Generate an Empty YAML Node */
    FYamlNode() = default;

    /** Generate an Empty YAML Node of a specific Type */
    explicit FYamlNode(const EYamlNodeType Type) :
        Node(static_cast<YAML::NodeType>(Type)) {}

    /** Generate a YAML Node that contains the given Data, which is implicitly Converted */
    template<typename T>
    explicit FYamlNode(const T& Data) :
        Node(Data) {}

    /** Generate an YAML Node from an Iterator Value */
    explicit FYamlNode(const YAML::detail::iterator_value& Value) :
        Node(YAML::Node(Value)) {}

    /** Generate an YAML Node from a Native YAML Node*/
    explicit FYamlNode(const YAML::Node Value) :
        Node(Value) {}

    // Types ---------------------------------------------------------------------------
    /** Returns the Type of the Contained Data */
    EYamlNodeType Type() const;

    /** If the Node has been Defined */
    bool IsDefined() const {
        return Node.IsDefined();
    }

    /** Equivalent to Type() == Null (No Value) */
    bool IsNull() const {
        return Node.IsNull();
    }

    /** Equivalent to Type() == Scalar (Singular Value) */
    bool IsScalar() const {
        return Node.IsScalar();
    }

    /** Equivalent to Type() == Sequence (Multiple Values without Keys) */
    bool IsSequence() const {
        return Node.IsSequence();
    }

    /** Equivalent to Type() == Map (List of Key-Value Pairs) */
    bool IsMap() const {
        return Node.IsMap();
    }


    // Conversion to bool and output to a Stream ---------------------------------------
    explicit operator bool() const {
        return Node.IsDefined();
    }

    bool operator!() const {
        return !Node.IsDefined();
    }

    // Style ---------------------------------------------------------------------------
    /** Returns the Style of the Node, mostly relevant for Sequences */
    EYamlEmitterStyle Style() const;

    /** Sets the Style of the Node, mostly relevant for Sequences */
    void SetStyle(const EYamlEmitterStyle Style);


    // Assignment ----------------------------------------------------------------------
    /** Test if 2 Nodes are Equal */
    bool Is(const FYamlNode& Other) const;

    bool operator==(const FYamlNode Other) const {
        return this->Is(Other);
    }

    /** Assign a Value to this Node. Will automatically converted */
    template<typename T>
    FYamlNode& operator=(const T& Value) {
        try {
            Node = Value;
        } catch (YAML::InvalidNode e) {
            UE_LOG(LogTemp, Warning, TEXT("Node was Invalid, won't assign any Value!"))
        }
        return *this;
    }

    /** Assign a Value to this Node. Will automatically converted */
    FYamlNode& operator=(const FYamlNode& Other) {
        Node = Other.Node;
        return *this;
    }

    /** Overwrite the Contents of this Node with the Content of another Node, or delete them if no Argument is given
     *
     * @returns If the Operation was successful
     */
    bool Reset(const FYamlNode& Other = FYamlNode());


    // Access --------------------------------------------------------------------------
    /** Try to Convert the Contents of the Node to the Given Type or a nullptr when conversion is not possible
     *
     * @return A Pointer to the Converted Value. If the Conversion was unsuccessful, return a nullptr
     */
    template<typename T>
    TOptional<T> AsOptional() const {
        try {
            return Node.as<T>();
        } catch (YAML::Exception) {
            return {};
        }
    }

    /** Try to Convert the Contents of the Node to the Given Type or return the Default Value
     * when conversion is not possible.  Serves as a more Secure version of AsPointer()
     *
     * The DefaultValue will default to the default Constructor of the Type. If this Constructor is not available,
     * you must supply a DefaultValue manually
     *
     * @return A Reference to the Converted Value. If the Conversion was unsuccessful, return the DefaultValue
     */
    template<typename T>
    T As(T DefaultValue = T()) const {
        try {
            return Node.as<T>();
        } catch (YAML::Exception) {
            return DefaultValue;
        }
    }

    /** Check if the given node can be converted to the given Type */
    template<typename T>
    bool CanConvertTo() const {
        try {
            Node.as<T>();
            return true;
        } catch (YAML::Exception) {
            return false;
        }
    }

    /** Try to Content of the Node if it is a Scalar */
    FString Scalar() const;

    /** Returns the whole Content of the Node as a single FString */
    FString GetContent() const;

    // Size and Iteration --------------------------------------------------------------
    /** Returns the Size of the Node if it is a Sequence or Map, 0 otherwise */
    int32 Size() const;

    /** Returns the start for an iterator. Use in combination with end() */
    FIteratorConst begin() const {
        return Node.begin();
    }

    FIterator begin() {
        return Node.begin();
    }

    /** Returns the end for a iterator. Use in combination with begin() */
    FIteratorConst end() const {
        return Node.end();
    }

    FIterator end() {
        return Node.end();
    }

    // Sequence ------------------------------------------------------------------------
    /** Converts the Node to a Sequence and adds the Element to this list */
    template<typename T>
    void Push(const T& Element) {
        try {
            Node.push_back(Element);
        } catch (YAML::InvalidNode) {
            UE_LOG(LogTemp, Warning, TEXT("Node was Invalid, can't Push any Value onto it!"))
        }
    }

    /** Converts the Node to a Sequence and adds the Node to this list */
    void Push(const FYamlNode& Element) {
        try {
            Node.push_back(Element.Node);
        } catch (YAML::InvalidNode) {
            UE_LOG(LogTemp, Warning, TEXT("Node was Invalid, can't Push any Value onto it!"))
        }
    }

    // Map -----------------------------------------------------------------------------
    /** Forces a Conversion to a Map and adds the given Key-Value pair to the Map */
    template<typename K, typename V>
    void ForceInsert(const K& Key, const V& Value) {
        Node.force_insert(Key, Value);
    }

    /** Returns all keys if this node is a map, otherwise an empty set */
    template<typename T>
    TSet<T> Keys() const {
        TSet<T> Ret;
        if (!IsMap()) return Ret;
        for (const auto Entry : Node) {
            T Key;
            if (YAML::convert<T>::decode(Entry.first, Key)) {
                Ret.Add(Entry.first.as<T>());
            }
        }
        return Ret;
    }

    // Indexing ------------------------------------------------------------------------
    /** Returns the Value at the given Key or Index */
    template<typename T>
    const FYamlNode operator[](const T& Key) const {
        return FYamlNode(Node[Key]);
    }

    /** Returns the Value at the given Key or Index */
    template<typename T>
    FYamlNode operator[](const T& Key) {
        return FYamlNode(Node[Key]);
    }

    /** Removes the Value at the given Key or Index */
    template<typename T>
    bool Remove(const T& Key) {
        return Node.remove(Key);
    }

    /** Returns the Value at the given Key or Index */
    const FYamlNode operator[](const FYamlNode& Key) const {
        return FYamlNode(Node[Key.Node]);
    }

    /** Returns the Value at the given Key or Index */
    FYamlNode operator[](const FYamlNode& Key) {
        return FYamlNode(Node[Key.Node]);
    }

    /** Removes the Value at the given Key or Index */
    bool Remove(const FYamlNode& Key) {
        return Node.remove(Key.Node);
    }
};

// Global Variables --------------------------------------------------------------------

/** Write the Contents of the Node to an OutputStream */
inline void operator<<(std::ostream& Out, const FYamlNode& Node) {
    Out << Node.Node;
}

/** Write the Contents of the Node into an Emitter */
inline void operator<<(FYamlEmitter& Out, const FYamlNode& Node) {
    Out << Node.Node;
}
