#include "Misc/AutomationTest.h"
#include "Parsing.h"
#include "Inputs.h"
#include "TestStructs.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(ConvertToStruct, "UnrealYAML.ConvertToStruct",
                                 EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

void AssertSimpleStructValues(ConvertToStruct* TestCase, const FSimpleStruct& SimpleStruct);
template <typename StructType>
void AssertInvalidParseInto(const TCHAR* Yaml, const TCHAR* What, ConvertToStruct* TestCase, const TArray<FString> Errors);

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

    // Invalid simple struct.
    {
        const auto Yaml = TEXT(R"yaml(
str: A String
int: "not an int"
bool: {not: a bool}
arr: {not: an array}
map:  [1, 2, 3]
)yaml");

        const auto Test = TEXT("Invalid data");

        AssertInvalidParseInto<FSimpleStruct>(Yaml, Test, this, {
            ".Int: cannot convert \"not an int\" to type integer",
            ".Bool: value is not a scalar",
            ".Arr: value is not a sequence",
            ".Map: value is not a map",
        });
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
        const auto Yaml = TEXT(R"yaml(
embedded:
    somevalues: {}
    afloat: foobar
children:
    - notanobject
mappedchildren:
    value1: [1, 2, 3]
    value3: 13
)yaml");
        const auto Test = TEXT("Invalid parent child");

        AssertInvalidParseInto<FParentStruct>(Yaml, Test, this, {
            ".Embedded.SomeValues: value is not a sequence",
            ".Embedded.AFloat: cannot convert \"foobar\" to type float",
            ".Children.[0]: value is not a map",
            ".MappedChildren.value1: value is not a map",
            ".MappedChildren.value3: value is not a map"
        });
    }

    // Test parsing in to an enum.
    {
        FYamlNode Node;
        UYamlParsing::ParseYaml("anenum: value3", Node);

        FEnumStruct Struct;
        FYamlParseIntoCtx Result;
        ParseNodeIntoStruct(Node, Struct, Result, FYamlParseIntoOptions::Strict());

        TestTrue("Enum parse", Result.Success());
        TestEqual("Enum parse value", Struct.AnEnum, EAnEnum::Value3);
    }

    // Test parsing in to an TEnumAsByte wrapper.
    {
        FYamlNode Node;
        UYamlParsing::ParseYaml("anenum: VALUE2", Node);

        FEnumAsByteStruct Struct;
        FYamlParseIntoCtx Result;
        ParseNodeIntoStruct(Node, Struct, Result, FYamlParseIntoOptions::Strict());

        TestTrue("EnumAsByte parse", Result.Success());
        TestEqual("EnumAsByte parse value", Struct.AnEnum, TEnumAsByte(EAnEnum::Value2));
    }

    // Test invalid parsing for enums.
    {
        const auto Yaml = TEXT("anenum: notaknownvalue");
        const auto Test = TEXT("Invalid EnumAsByte");
        AssertInvalidParseInto<FEnumStruct>(Yaml, Test, this, {
            ".AnEnum: \"notaknownvalue\" is not an allowed value for enum EAnEnum",
        });
    }

    // Test invalid parsing for EnumAsByte wrappers.
    {
        const auto Yaml = TEXT("anenum: notaknownvalue");
        const auto Test = TEXT("Invalid EnumAsByte");
        AssertInvalidParseInto<FEnumAsByteStruct>(Yaml, Test, this, {
            ".AnEnum: \"notaknownvalue\" is not an allowed value for enum EAnEnum",
        });
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

template <typename StructType>
void AssertInvalidParseInto(const TCHAR* Yaml, const TCHAR* What, ConvertToStruct* TestCase, const TArray<FString> Errors) {
    FYamlNode Node;
    UYamlParsing::ParseYaml(Yaml, Node);

    StructType Struct;
    FYamlParseIntoCtx Result;
    ParseNodeIntoStruct(Node, Struct, Result, FYamlParseIntoOptions::Strict());

    TestCase->TestFalse(FString::Printf(TEXT("%ls fails as expected"), What), Result.Success());
    if (!TestCase->TestEqual(FString::Printf(TEXT("%ls error count"), What), Result.Errors.Num(), Errors.Num())) {
        return;
    }

    for (auto I = 0; I < Errors.Num(); ++I) {
        TestCase->TestEqual(FString::Printf(TEXT("%ls error[%d]"), What, I), Result.Errors[I], Errors[I]);
    }
}

#endif
