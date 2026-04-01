#pragma once

#include "CoreMinimal.h"

#include "Enums.generated.h"


// Possible States a Node can be in
UENUM()
enum class EYamlNodeType : uint8 {
    // Invalid Node
    Undefined,

    // Node has no Content
    Empty,

    // Node is storing a single Value
    Scalar,

    // Node is a List
    Sequence,

    // Node is Storing Key-Value pairs
    Map
};

DECLARE_ENUM_TO_STRING(EYamlNodeType);


// Different ways to align data in a list/map in a file
UENUM()
enum class EYamlEmitterStyle : uint8 {
    // Keep the default alignment
    Default,

    // Align the data blocking -> 1 item per line
    Block,

    // Align the data flowing -> all items on the same line
    Flow
};
