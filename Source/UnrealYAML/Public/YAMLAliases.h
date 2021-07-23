#pragma once

#include "yaml.h"

// Basic Nodes ---------------------------------------------------------------------------

/** Base YAML class. Stores a YAML-Structure in a Tree-like hierarchy. Can therefore either hold a single value or be a Container for other Nodes.
 * Conversion from one Type to another will be done automatically as needed.
 */
using FYamlNode = YAML::Node;


/** Possible Types a Node can have:
 *  - Undefined: Invalid Node
 *  - Null: Node has no Content
 *  - Scalar: Node is Storing a single Value
 *  - Sequence: Node is a List
 *  - Map: Node is Storing Key-Value pairs.
 */
using EYamlNodeType = YAML::NodeType::value;

// Emitter --------------------------------------------------------------------------------------------

/** An Emitter used to create new YAML-Structures. You can use the "<<"-operator to add Elements to the Structure. */
using FYamlEmitter = YAML::Emitter;

/** Use these to begin a Sequence or create Map Key-Value pairs */
using EYamlEmitterTags = YAML::EMITTER_MANIP;


using FYamlConstIterator = YAML::const_iterator;

inline FYamlNode LoadYamlNode(const FString& Path) {
    return YAML::LoadFile(TCHAR_TO_UTF8(&Path));
}
