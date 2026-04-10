#pragma once

// Invalid YAML
const FString ErroneousYaml("err: -");

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

const FString SimpleYamlWrongTypes(
    "int: \"not an int\" \n"
    "bool: {not: a bool} \n"
    "arr: {not: an array} \n"
    "map:  [42, 43, 44] \n"
);

// YAML with nested types
const FString ComplexYaml(
    "nested: [[1, 2, 3], [a, b, c, d], null] \n"
    "mixed: [true, 3.1415926535897932] \n"
    "struct: \n"
    "  color: magenta \n"
    "  set: [0, 0, 1, 3, 5, 5, 6] \n"
);

const FString NestedStruct(
    "inner:\n"
    "    strings: [one, two] \n"
    "    float: 13.124 \n"
    "childArray: \n"
    "    - strings: [three] \n"
    "      float: 1.0 \n"
    "    - strings: [four] \n"
    "      float: 2.0 \n"
    "childMap: \n"
    "    child1: \n"
    "        strings: [five, six] \n"
    "        float: 0 \n"
    "    child2: \n"
    "        strings: [seven] \n"
    "        float: -13 \n"
    "    child3: \n"
    "        strings: [] \n"
    "        float: -26 \n"
);

const FString NestedStructInvalid(
    "inner: \n"
    "    strings: {} \n"
    "    float: foobar \n"
    "childArray: \n"
    "    - notAnObject \n"
    "childMap: \n"
    "    child1: [1, 2, 3] \n"
    "    notAnObject: 13 \n"
);

// YAML with all the native types from `(De)SerializeNativeType`
const FString NativeTypesStruct(
    "transform: \n"
    "- [1, 2, 3] \n"
    "- [0, 90, 0] \n"  // Test rotator form here and quad below
    "- [2, 2, 2] \n"
    "quat: [0, 0, 0, 1] \n"
    "rotator: [90, 180, 0] \n"
    "vector: [13.23, 0, -12.4] \n"
    "vector2d: [5, 4] \n"
    "set: [0, 1, 2, 3, 4] \n"
    "linearColor: red \n"
    "color: [255, 255, 255, 255] \n"
    "text: this is some text \n"
    "name: MyTestName \n"
);
