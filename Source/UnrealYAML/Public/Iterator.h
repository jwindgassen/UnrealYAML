#pragma once


/** Forward Input Iterator Base to iterate over Nodes.
 *
 * The native yaml-cpp iterator return either a Node or a Pair of Nodes, depending if the iterated Node is a Sequence
 * or a Map. We extend this behaviour and always return a Pair of Nodes, the first being the Index in case of a Sequence
 */
template<typename NodeType>
class TYamlIteratorBase {
    friend struct FYamlNode;

    static_assert(std::is_same_v<std::remove_const_t<NodeType>, FYamlNode>); // Only allow FYamlNodes
    using IteratorType = std::conditional_t<
        std::is_const_v<NodeType>,
        YAML::const_iterator,
        YAML::iterator
    >;

    IteratorType Iterator;
    int32 Index;

    TYamlIteratorBase(IteratorType&& Iter) :
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


    TYamlIteratorBase& operator++() {
        ++Iterator;
        Index++;
        return *this;
    }


    TYamlIteratorBase operator++(int) {
        TYamlIteratorBase Pre(*this);
        ++(*this);
        Index++;
        return Pre;
    }

    friend bool operator ==(const TYamlIteratorBase& This, const TYamlIteratorBase Other) {
        return This.Iterator == Other.Iterator;
    }

    friend bool operator !=(const TYamlIteratorBase& This, const TYamlIteratorBase Other) {
        return This.Iterator != Other.Iterator;
    }
};
