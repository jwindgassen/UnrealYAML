#pragma once

// Empty YAML
const FString EmptyYaml("");

// POD YAML
const FString SimpleYaml(
    "str: A String \n"
    "int: 42 \n"
    "bool: true \n"
    "arr: [1, 2, 3] \n"
    "map: \n"
    "  a: 1 \n"
    "  b: 2 \n"
    );

const FString ComplexYaml(
    "nested: [[1, 2, 3], [a, b, c, d], null] \n"
    "mixed: [true, 3.1415926535897932] \n"
    "struct: \n"
    "  color: magenta \n"
    "  set: [0, 0, 1, 3, 5, 5, 6] \n"
    );

const FString ErroneousYaml("err: -");
