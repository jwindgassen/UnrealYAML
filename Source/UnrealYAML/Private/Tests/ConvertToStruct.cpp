#include "Misc/AutomationTest.h"
#include "Parsing.h"
#include "Inputs.h"
#include "TestStructs.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(ConvertToStruct, "UnrealYAML.ConvertToStruct",
                                 EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

void AssertSimpleStructValues(ConvertToStruct* TestCase, const FSimpleStruct& SimpleStruct);

bool ConvertToStruct::RunTest(const FString& Parameters) {
    // Simple Yaml
    {
        FYamlNode Node;
        UYamlParsing::ParseYaml(SimpleYaml, Node);

        FSimpleStruct SimpleStruct;
        TestTrue("Parse Node into SimpleStruct", ParseNodeIntoStruct(Node, SimpleStruct));
        AssertSimpleStructValues(this, SimpleStruct);
    }

    // CPP non-template ParseIntoStruct
    {
        FYamlNode Node;
        UYamlParsing::ParseYaml(SimpleYaml, Node);

        uint8* StructData = (uint8*)FMemory::Malloc(FSimpleStruct::StaticStruct()->GetStructureSize());
        FSimpleStruct::StaticStruct()->InitializeDefaultValue(StructData);
        TestTrue("Parse Node into dynamic SimpleStruct", ParseNodeIntoStruct(Node, FSimpleStruct::StaticStruct(), StructData));

        FSimpleStruct* Struct = reinterpret_cast<FSimpleStruct*>(StructData);
        AssertSimpleStructValues(this, *Struct);
        FSimpleStruct::StaticStruct()->DestroyStruct(StructData);
    }

    // Invalid data.
    {
        const FString InvalidYaml(R"yaml(
str: A String
int: "not an int"
bool: {not: a bool}
arr: {not: an array}
map:  [1, 2, 3]
)yaml");

        FYamlNode Node;
        UYamlParsing::ParseYaml(InvalidYaml, Node);

        FSimpleStruct Struct;
        FYamlParseIntoCtx Result;
        ParseNodeIntoStruct(Node, Struct, Result, FYamlParseIntoOptions::StrictParsing());

        TestFalse("Invalid data: failed", Result.Success());
        if (TestEqual("Invalid data: errors length", Result.Errors.Num(), 4)) {
            TestEqual("Invalid data: errors[0]", Result.Errors[0], TEXT(".Int: cannot convert \"not an int\" to type integer"));
            TestEqual("Invalid data: errors[1]", Result.Errors[1], TEXT(".Bool: value is not a scalar"));
            TestEqual("Invalid data: errors[2]", Result.Errors[2], TEXT(".Arr: value is not a sequence"));
            TestEqual("Invalid data: errors[3]", Result.Errors[3], TEXT(".Map: value is not a map"));
        }
    }

    // Lax parsing of invalid data (default behaviour).
    {
        auto InvalidYaml = TEXT(R"yaml(
str: foo
int: not an int
bool: somevalue
arr: notarray
map: notmap
)yaml");

        FYamlNode Node;
        UYamlParsing::ParseYaml(InvalidYaml, Node);

        FSimpleStruct Struct;
        TestTrue(TEXT("Invalid data: lax parsing ok"), ParseNodeIntoStruct(Node, Struct));
    }

    // Invalid parent child.
    {
        FString InvalidParentChildYaml(R"yaml(
embedded:
    somevalues: {}
    afloat: foobar
children:
    - notanobject
mappedchildren:
    value1: [1, 2, 3]
    value3: 13
)yaml");

        FYamlNode Node;
        UYamlParsing::ParseYaml(InvalidParentChildYaml, Node);

        FParentStruct Struct;
        FYamlParseIntoCtx Result;
        ParseNodeIntoStruct(Node, Struct, Result, FYamlParseIntoOptions::StrictParsing());

        TestFalse("Invalid parent child: failed", Result.Success());
        if (TestEqual("Invalid parent child: errors length", Result.Errors.Num(), 7)) {
            TestEqual("Invalid parent child: errors[0]", Result.Errors[0], TEXT(".Embedded.SomeValues: value is not a sequence"));
            TestEqual("Invalid parent child: errors[1]", Result.Errors[1], TEXT(".Embedded.AFloat: cannot convert \"foobar\" to type float"));
            TestEqual("Invalid parent child: errors[2]", Result.Errors[2], TEXT(".Children.[0]: value is not a map"));
            TestEqual("Invalid parent child: errors[3]", Result.Errors[3], TEXT(".MappedChildren.value1: cannot convert \"value1\" to type integer"));
            TestEqual("Invalid parent child: errors[4]", Result.Errors[4], TEXT(".MappedChildren.value1: value is not a map"));
            TestEqual("Invalid parent child: errors[5]", Result.Errors[5], TEXT(".MappedChildren.value3: cannot convert \"value3\" to type integer"));
            TestEqual("Invalid parent child: errors[6]", Result.Errors[6], TEXT(".MappedChildren.value3: value is not a map"));
        }
    }

    return !HasAnyErrors();
}

void AssertSimpleStructValues(ConvertToStruct* TestCase, const FSimpleStruct& SimpleStruct) {
    TestCase->TestEqual("SimpleStruct: Str", SimpleStruct.Str, TEXT("A String"));
    TestCase->TestEqual("SimpleStruct: Int", SimpleStruct.Int, 42);
    TestCase->TestEqual("SimpleStruct: Bool", SimpleStruct.Bool, true);
    TestCase->TestEqual("SimpleStruct: Array length", SimpleStruct.Arr.Num(), 3);
    TestCase->TestEqual("SimpleStruct: Array[0]", SimpleStruct.Arr[0], 1);
    TestCase->TestEqual("SimpleStruct: Array[1]", SimpleStruct.Arr[1], 2);
    TestCase->TestEqual("SimpleStruct: Array[2]", SimpleStruct.Arr[2], 3);
    TestCase->TestEqual("SimpleStruct: Map length", SimpleStruct.Map.Num(), 2);
    TestCase->TestEqual("SimpleStruct: Map contains a", SimpleStruct.Map.Contains("a"), true);
    TestCase->TestEqual("SimpleStruct: Map value a", SimpleStruct.Map["a"], 1);
    TestCase->TestEqual("SimpleStruct: Map contains b", SimpleStruct.Map.Contains("b"), true);
    TestCase->TestEqual("SimpleStruct: Map value b", SimpleStruct.Map["b"], 2);
}

#endif
