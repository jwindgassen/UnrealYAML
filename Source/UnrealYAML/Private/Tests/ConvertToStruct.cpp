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

    // ParseInto parent child. Tests multiple features over a complex nested struct.
    {
        const auto Yaml = TEXT(R"yaml(
embedded:
    somevalues: [one, two]
    afloat: 13.124
    anenum: value1
children:
    - somevalues: [three]
      afloat: 1
      anenum: value3
    - somevalues: [four]
      afloat: 2
      anenum: value3
mappedchildren:
    value1:
        somevalues: [five, six]
        afloat: 0
        anenum: value1
    value2:
        somevalues: [seven]
        afloat: -13
        anenum: value2
    value3:
        somevalues: []
        afloat: -26
        anenum: value3
)yaml");

        FYamlNode Node;
        UYamlParsing::ParseYaml(Yaml, Node);

        FParentStruct Struct;
        FYamlParseIntoCtx Result;
        ParseNodeIntoStruct(Node, Struct, Result, FYamlParseIntoOptions::Strict());

        TestTrue("ParseInto ParentChild success", Result.Success());
        TestEqual("ParentChild.Embedded.SomeValues", Struct.Embedded.SomeValues, {"one", "two"});
        TestEqual("ParentChild.Embedded.AFloat", Struct.Embedded.AFloat, 13.124f);
        TestEqual("ParentChild.Embedded.AnEnum", Struct.Embedded.AnEnum, TEnumAsByte(EAnEnum::Value1));

        if (TestEqual("ParentChild.Child", Struct.Children.Num(), 2)) {
            TestEqual("ParentChild.Child[0].SomeValues", Struct.Children[0].SomeValues, {"three"});
            TestEqual("ParentChild.Child[0].AFloat", Struct.Children[0].AFloat, 1.0f);
            TestEqual("ParentChild.Child[0].AFloat", Struct.Children[0].AnEnum, TEnumAsByte(EAnEnum::Value3));
            TestEqual("ParentChild.Child[10].SomeValues", Struct.Children[1].SomeValues, {"four"});
            TestEqual("ParentChild.Child[10].AFloat", Struct.Children[1].AFloat, 2.0f);
            TestEqual("ParentChild.Child[10].AFloat", Struct.Children[1].AnEnum, TEnumAsByte(EAnEnum::Value3));
        }

        TestEqual("ParentChild.MappedChildren", Struct.MappedChildren.Num(), 3);
        if (TestTrue("ParentChild.MappedChildren[value1]", Struct.MappedChildren.Contains(TEnumAsByte(EAnEnum::Value1)))) {
            TestEqual(
                "ParentChild.MappedChildren[value1].SomeValues",
                Struct.MappedChildren[TEnumAsByte(EAnEnum::Value1)].SomeValues,
                {"five", "six"});

            TestEqual(
                "ParentChild.MappedChildren[value1].AFloat",
                Struct.MappedChildren[TEnumAsByte(EAnEnum::Value1)].AFloat,
                0.0f);

            TestEqual(
                "ParentChild.MappedChildren[value1].AnEnum",
                Struct.MappedChildren[TEnumAsByte(EAnEnum::Value1)].AnEnum,
                TEnumAsByte(EAnEnum::Value1));
        }

        if (TestTrue("ParentChild.MappedChildren[value2]", Struct.MappedChildren.Contains(TEnumAsByte(EAnEnum::Value2)))) {
            TestEqual(
                "ParentChild.MappedChildren[value2].SomeValues",
                Struct.MappedChildren[TEnumAsByte(EAnEnum::Value2)].SomeValues,
                {"seven"});

            TestEqual(
                "ParentChild.MappedChildren[value2].AFloat",
                Struct.MappedChildren[TEnumAsByte(EAnEnum::Value2)].AFloat,
                -13.0f);

            TestEqual(
                "ParentChild.MappedChildren[value2].AnEnum",
                Struct.MappedChildren[TEnumAsByte(EAnEnum::Value2)].AnEnum,
                TEnumAsByte(EAnEnum::Value2));
        }

        if (TestTrue("ParentChild.MappedChildren[value3]", Struct.MappedChildren.Contains(TEnumAsByte(EAnEnum::Value3)))) {
            TestEqual(
                "ParentChild.MappedChildren[value3].SomeValues",
                Struct.MappedChildren[TEnumAsByte(EAnEnum::Value3)].SomeValues,
                {});

            TestEqual(
                "ParentChild.MappedChildren[value3].AFloat",
                Struct.MappedChildren[TEnumAsByte(EAnEnum::Value3)].AFloat,
                -26.0f);

            TestEqual(
                "ParentChild.MappedChildren[value3].AnEnum",
                Struct.MappedChildren[TEnumAsByte(EAnEnum::Value3)].AnEnum,
                TEnumAsByte(EAnEnum::Value3));
        }
    }

    // Tests default are preserved.
    {
        FYamlNode Node;
        UYamlParsing::ParseYaml("{}", Node);

        FDefaultStruct Struct;
        FYamlParseIntoCtx Result;
        TestFalse("Default", ParseNodeIntoStruct(Node, Struct, Result, FYamlParseIntoOptions::Strict()));

        TestTrue("Default success", Result.Success());

        TestEqual("Default Int", Struct.AnInt, 13);
        TestEqual("Default Float", Struct.AFloat, 13.24f);
        TestEqual("Default String", Struct.AString, "Hello world!");
        TestEqual("Default Enum", Struct.AnEnum, TEnumAsByte(EAnEnum::Value3));
        if (TestEqual("Default Map", Struct.AMap.Num(), 3)) {
            TestEqual("Default Map[one]", Struct.AMap["one"], "1");
            TestEqual("Default Map[two]", Struct.AMap["two"], "2");
            TestEqual("Default Map[three]", Struct.AMap["three"], "3");
        }
        TestEqual("Default Array", Struct.AnArray, {EAnEnum::Value1, EAnEnum::Value2});
    }

    // Checks that container default values are correctly lost if specified in YAML.
    {
        auto Yaml = TEXT(R"yaml(
anarray: [value3]
amap:
    1: one
    2: two
)yaml");

        FYamlNode Node;
        UYamlParsing::ParseYaml(Yaml, Node);

        FDefaultStruct Struct;
        FYamlParseIntoCtx Result;
        TestFalse("DefaultOverwrite", ParseNodeIntoStruct(Node, Struct, Result, FYamlParseIntoOptions::Strict()));

        TestTrue("DefaultOverwrite success", Result.Success());
        TestEqual("DefaultOverwrite Array", Struct.AnArray, {EAnEnum::Value3});
        if (TestEqual("DefaultOverwrite Map", Struct.AMap.Num(), 2)) {
            TestEqual("DefaultOverwrite Map[1]", Struct.AMap["1"], "one");
            TestEqual("DefaultOverwrite Map[2]", Struct.AMap["2"], "two");
        }
    }

    // Tests support registered with yaml-cpp for Unreal types.
    {
        auto Yaml = TEXT(R"yaml(
transform:
    - [1, 2, 3]
    - [0, 90, 0] # rotator form (quat is also supported).
    - [2, 2, 2]
quat: [0, 0, 0, 1]
rotator: [90, 180, 0]
vector: [13.23, 0, -12.4]
vector2d: [5, 4]
set: [0, 1, 2, 3, 4]
linearcolor: red
color: [255, 255, 255, 255]
text: this is some text
)yaml");

        FYamlNode Node;
        UYamlParsing::ParseYaml(Yaml, Node);

        FUnrealTypeStruct Struct;
        FYamlParseIntoCtx Result;
        TestTrue("UnrealTypes", ParseNodeIntoStruct(Node, Struct, Result, FYamlParseIntoOptions::Strict()));

        TestTrue("UnrealTypes success", Result.Success());

        TestEqual("UnrealTypes Transform.Location", Struct.Transform.GetLocation(), FVector(1, 2, 3));
        TestEqual("UnrealTypes Transform.Rotation", Struct.Transform.Rotator(), FRotator(0, 0, 90));
        TestEqual("UnrealTypes Transform.Scale", Struct.Transform.GetScale3D(), FVector(2));
        TestEqual("UnrealTypes Quat", Struct.Quat, FQuat::Identity);
        TestEqual("UnrealTypes Rotator", Struct.Rotator, FRotator(90, 0, 180));
        TestEqual("UnrealTypes Vector", Struct.Vector, FVector(13.23, 0, -12.4));
        TestEqual("UnrealTypes Vector2D", Struct.Vector2D, FVector2D(5, 4));
        TestEqual("UnrealTypes Set", Struct.Set.Difference({0, 1, 2, 3, 4}).Num(), 0);
        TestEqual("UnrealTypes LinearColor", Struct.LinearColor, FColor::Red.ReinterpretAsLinear());
        TestEqual("UnrealTypes Color", Struct.Color, FColor::White);
        TestEqual("UnrealTypes Text", Struct.Text.ToString(), "this is some text");
    }

    // Unreal types which are references to other things that needs special handling.
    {
        auto Yaml = TEXT(R"yaml(
subclassOf: "/Script/CoreUObject.Class'/Script/Engine.Actor'"

# Note: this requires loading from disk of a .uasset from engine content. We assume this is available for this test.
softObjectPtr: "/Script/Engine.StaticMesh'/Engine/BasicShapes/Cube.Cube'"
)yaml");

        FYamlNode Node;
        UYamlParsing::ParseYaml(Yaml, Node);

        FUnrealReferenceTypeStruct Struct;
        FYamlParseIntoCtx Result;
        TestTrue("UnrealReferenceTypes", ParseNodeIntoStruct(Node, Struct, Result, FYamlParseIntoOptions::Strict()));

        TestTrue("UnrealReferenceTypes success", Result.Success());

        TestEqual("UnrealReferenceTypes TSubclassOf", Struct.SubclassOf.Get()->GetName(), "Actor");
        if (TestEqual("UnrealReferenceTypes TSoftObjectPtr", Struct.SoftObjectPtr.IsNull(), false)) {
            // Check it's a valid mesh.
            TestTrue("UnrealReferenceTypes TSoftObjectPtr Value", Struct.SoftObjectPtr.LoadSynchronous()->GetNumVertices(0) > 0);
        }
    }

    // Tests subclassof resolves blueprint classes.
    {
        auto Yaml = TEXT(R"yaml(
# Note: this requires loading from disk of a .uasset from engine content. We assume this is available for this test.
subclassOf: "/Script/Engine.Blueprint'/Engine/EngineSky/BP_Sky_Sphere.BP_Sky_Sphere'"
)yaml");

        FYamlNode Node;
        UYamlParsing::ParseYaml(Yaml, Node);

        FUnrealReferenceTypeStruct Struct;
        FYamlParseIntoCtx Result;
        ParseNodeIntoStruct(Node, Struct, Result, FYamlParseIntoOptions::Strict());

        TestTrue("UnrealReferenceTypes success", Result.Success());

        TestTrue("UnrealReferenceTypes TSubclassOf Blueprint", Struct.SubclassOf.Get() != nullptr);
    }

    // Unreal reference types fail if the referenced entities cannot be found.
    {
        auto Yaml = TEXT(R"yaml(
subclassOf: "not a uclass"
softObjectPtr: "not a uobject"
)yaml");

        AssertInvalidParseInto<FUnrealReferenceTypeStruct>(Yaml, TEXT("Invalid UnrealReferenceTypes"), this, {
            ".SubclassOf: Cannot find class: not a uclass",
            ".SoftObjectPtr: Cannot find object: not a uobject",
        });
    }

    // Tests negative integers are parsed.
    {
        const auto Yaml = TEXT("int: -1");
        FYamlNode Node;
        UYamlParsing::ParseYaml(Yaml, Node);

        FSimpleStruct Struct;
        FYamlParseIntoCtx Result;
        ParseNodeIntoStruct(Node, Struct, Result, FYamlParseIntoOptions::Strict());

        TestTrue("NegativeInteger", Result.Success());
        TestEqual("NegativeInteger Value", Struct.Int, -1);
    }

    // Tests that the YamlRequired meta flag rejects missing YAML.
    {
        const auto Yaml = TEXT("optional: 13");

        AssertInvalidParseInto<FRequiredFieldsStruct>(Yaml, TEXT("Required: missing"), this, {
            ".Required: yaml does not contain this required field"
        });
    }

    // Tests that the YamlRequired meta flag passes if present.
    {
        const auto Yaml = TEXT("{ optional: 13, required: -1 }");
        FYamlNode Node;
        UYamlParsing::ParseYaml(Yaml, Node);

        FRequiredFieldsStruct Struct;
        FYamlParseIntoCtx Result;
        ParseNodeIntoStruct(Node, Struct, Result, FYamlParseIntoOptions::Strict());

        TestTrue("Required: present", Result.Success());
        TestEqual("Required: present required value", Struct.Required, -1);
        TestEqual("Required: present optional value", Struct.Optional, 13);
    }

    // When the YAML provides additional properties.
    {
        const auto Yaml = TEXT(R"yaml(
str: "foo"
INT: 13
bOOl: false
ArR: [1, 2, 3]
map: { foo: 1, bar: 2}
randomprop: [1, 2, 3]
)yaml");

        AssertInvalidParseInto<FSimpleStruct>(Yaml, TEXT("Additional properties"), this, {
            ".randomprop: additional property does not match a property in USTRUCT"
        });
    }

    // Tests that custom types can be handled via custom TypeHandlers.
    {
        const auto Yaml = TEXT("customtype: 13");
        FYamlNode Node;
        UYamlParsing::ParseYaml(Yaml, Node);

        FWithCustomType Struct;
        FYamlParseIntoCtx Result;
        auto Options = FYamlParseIntoOptions::Strict();
        Options.TypeHandlers.Add("FCustomType", [](const FYamlNode& YamlNode, const UScriptStruct* ScriptStruct,
                                                   void* StructValue, FYamlParseIntoCtx& YamlParseIntoCtx) {
            auto AsNumber = YamlNode.As<int>();
            FCustomType Ct;
            Ct.Value = FString::FromInt(AsNumber);
            *static_cast<FCustomType*>(StructValue) = Ct;
        });

        ParseNodeIntoStruct(Node, Struct, Result, Options);

        TestTrue("CustomType: success", Result.Success());
        TestEqual("CustomType: value", Struct.CustomType.Value, "13");
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
