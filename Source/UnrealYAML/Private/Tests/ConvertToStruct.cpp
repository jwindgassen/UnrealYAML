#include "Misc/AutomationTest.h"
#include "YamlParsing.h"
#include "YamlSerialization.h"
#include "Inputs.h"
#include "TestStructs.h"

#if WITH_DEV_AUTOMATION_TESTS

#if ENGINE_MAJOR_VERSION >= 5
IMPLEMENT_SIMPLE_AUTOMATION_TEST(ConvertToStruct, "UnrealYAML.ConvertToStruct",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)
#else
IMPLEMENT_SIMPLE_AUTOMATION_TEST(ConvertToStruct, "UnrealYAML.ConvertToStruct",
                                 EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::SmokeFilter)
#endif

void AssertSimpleStructValues(ConvertToStruct* TestCase, const FSimpleStruct& SimpleStruct);
template <typename StructType>
void AssertInvalidDeserialization(const TCHAR* Yaml, const TCHAR* What, ConvertToStruct* TestCase, const TArray<FString> Errors);

bool ConvertToStruct::RunTest(const FString& Parameters) {
    
    // ToDo: Make this proper
    AddExpectedError("Cannot convert \".*\" to a", EAutomationExpectedErrorFlags::MatchType::Contains, 0);
    AddExpectedError("Expected '.*' but found '.*'", EAutomationExpectedErrorFlags::MatchType::Contains, 0);
    AddExpectedError("not an valid enum value", EAutomationExpectedErrorFlags::MatchType::Contains, 0);
    AddExpectedError("cannot be parsed as an enum", EAutomationExpectedErrorFlags::MatchType::Contains, 0);
    AddExpectedError("Cannot find Object", EAutomationExpectedErrorFlags::MatchType::Contains, 0);
    AddExpectedError("Cannot find Class", EAutomationExpectedErrorFlags::MatchType::Contains, 0);
    AddExpectedError("Missing Required Key", EAutomationExpectedErrorFlags::MatchType::Contains, 0);
    AddExpectedError("additional unused Keys", EAutomationExpectedErrorFlags::MatchType::Contains, 0);
    
    // Simple YAML to Struct
    {
        FYamlNode Node;
        UYamlParsing::ParseYaml(SimpleYaml, Node);

        FSimpleStruct SimpleStruct;
        TestTrue("Deserialize SimpleStruct", !!DeserializeStruct(Node, SimpleStruct));
        AssertSimpleStructValues(this, SimpleStruct);
    }

    // CPP non-template DeserializeStruct
    {
        FYamlNode Node;
        UYamlParsing::ParseYaml(SimpleYaml, Node);

        uint8* StructData = (uint8*)FMemory::Malloc(FSimpleStruct::StaticStruct()->GetStructureSize());
        FSimpleStruct::StaticStruct()->InitializeDefaultValue(StructData);
        TestTrue("Deserialize dynamic SimpleStruct", !!DeserializeStruct(Node, FSimpleStruct::StaticStruct(), StructData));

        FSimpleStruct* Struct = reinterpret_cast<FSimpleStruct*>(StructData);
        AssertSimpleStructValues(this, *Struct);
        FSimpleStruct::StaticStruct()->DestroyStruct(StructData);
    }
    
    // Simple YAML to Object
    {
        FYamlNode Node;
        UYamlParsing::ParseYaml(SimpleYaml, Node);

        USimpleObject* SimpleObject = NewObject<USimpleObject>(GetTransientPackage());
        TestTrue("Deserialize SimpleObject", !!DeserializeObject(Node, SimpleObject));
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

        AssertInvalidDeserialization<FSimpleStruct>(Yaml, Test, this, {
            "Int: Cannot convert \"not an int\" to an Integer",
            "Bool: Expected 'Scalar' but found 'Map'",
            "Arr: Expected 'Sequence' but found 'Map'",
            "Map: Expected 'Map' but found 'Sequence'",
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
        TestTrue(TEXT("Invalid data: lax parsing ok"), !!DeserializeStruct(Node, Struct));
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
        
        AssertInvalidDeserialization<FParentStruct>(Yaml, TEXT("Invalid parent child"), this, {
            "Embedded.SomeValues: Expected 'sequence' but found 'map'",
            "Embedded.AFloat: Cannot convert \"foobar\" to a Float",
            "Children.[0]: Expected 'map' but found 'scalar'",
            "MappedChildren.value1: Expected 'map' but found 'sequence'",
            "MappedChildren.value3: Expected 'map' but found 'scalar'"
        });
    }

    // Test parsing in to an enum.
    {
        FYamlNode Node;
        UYamlParsing::ParseYaml("anenum: value3", Node);

        FEnumStruct EnumStruct;
        FYamlSerializationResult Result = DeserializeStruct(Node, EnumStruct, FYamlDeserializeOptions::Strict());

        TestTrue("Enum parse", Result.Success());
        TestEqual("Enum parse value", EnumStruct.AnEnum, EAnEnumClass::Value3);

        TestTrue("Enum parse operator bool()", !!Result);
    }

    // Test parsing in to an TEnumAsByte wrapper.
    {
        FYamlNode Node;
        UYamlParsing::ParseYaml("anenum: VALUE2", Node);

        FEnumAsByteStruct EnumStruct;
        FYamlSerializationResult Result = DeserializeStruct(Node, EnumStruct, FYamlDeserializeOptions::Strict());

        TestTrue("EnumAsByte parse", Result.Success());
        TestEqual("EnumAsByte parse value", EnumStruct.AnEnum, TEnumAsByte(EAnEnum::Value2));
    }

    // Test parsing in to an enum (as integer).
    {
        FYamlNode Node;
        UYamlParsing::ParseYaml("anenum: 0", Node);

        FEnumStruct EnumStruct;
        FYamlSerializationResult Result = DeserializeStruct(Node, EnumStruct, FYamlDeserializeOptions::Strict());

        TestTrue("Enum(int) parse", Result.Success());
        TestEqual("Enum(int) parse value", EnumStruct.AnEnum, EAnEnumClass::Value1);

        TestTrue("Enum(int) parse operator bool()", !!Result);
    }

    // Test parsing in to an TEnumAsByte wrapper (as integer).
    {
        FYamlNode Node;
        UYamlParsing::ParseYaml("anenum: 1", Node);

        FEnumAsByteStruct EnumStruct;
        FYamlSerializationResult Result = DeserializeStruct(Node, EnumStruct, FYamlDeserializeOptions::Strict());

        TestTrue("EnumAsByte(int) parse", Result.Success());
        TestEqual("EnumAsByte(int) parse value", EnumStruct.AnEnum, TEnumAsByte(EAnEnum::Value2));
    }

    // Test invalid parsing for enums.
    {
        const auto Yaml = TEXT("anenum: []");
        const auto Test = TEXT("Invalid EnumClass");
        AssertInvalidDeserialization<FEnumStruct>(Yaml, Test, this, {
            "AnEnum: Value of type \"Sequence\" cannot be parsed as an enum value for EAnEnumClass",
        });
    }

    // Test invalid parsing for EnumAsByte wrappers.
    {
        const auto Yaml = TEXT("anenum: notaknownvalue");
        const auto Test = TEXT("Invalid EnumAsByte");
        AssertInvalidDeserialization<FEnumAsByteStruct>(Yaml, Test, this, {
            "AnEnum: \"notaknownvalue\" is not an valid enum value of EAnEnum",
        });
    }

    // Tests default are preserved.
    {
        FYamlNode Node;
        UYamlParsing::ParseYaml("{}", Node);

        FDefaultStruct Struct;
        FYamlSerializationResult Result = DeserializeStruct(Node, Struct, FYamlDeserializeOptions::Strict());

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
        TestEqual("Default Array", Struct.AnArray, {EAnEnumClass::Value1, EAnEnumClass::Value2});
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
        FYamlSerializationResult Result = DeserializeStruct(Node, Struct, FYamlDeserializeOptions::Strict());

        TestTrue("DefaultOverwrite success", Result.Success());
        TestEqual("DefaultOverwrite Array", Struct.AnArray, {EAnEnumClass::Value3});
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
name: MyTestName
)yaml");

        FYamlNode Node;
        UYamlParsing::ParseYaml(Yaml, Node);

        FUnrealTypeStruct Struct;
        FYamlSerializationResult Result = DeserializeStruct(Node, Struct, FYamlDeserializeOptions::Strict());
        TestTrue("UnrealTypes", !!Result);

        TestTrue("UnrealTypes success", Result.Success());

        TestEqual("UnrealTypes Transform.Location", Struct.Transform.GetLocation(), FVector(1, 2, 3));
        TestEqual("UnrealTypes Transform.Rotation", Struct.Transform.Rotator(), FRotator(0, 90, 0));
        TestEqual("UnrealTypes Transform.Scale", Struct.Transform.GetScale3D(), FVector(2));
        TestEqual("UnrealTypes Quat", Struct.Quat, FQuat::Identity);
        TestEqual("UnrealTypes Rotator", Struct.Rotator, FRotator(90, 180, 0));
        TestEqual("UnrealTypes Vector", Struct.Vector, FVector(13.23, 0, -12.4));
        TestEqual("UnrealTypes Vector2D", Struct.Vector2D, FVector2D(5, 4));
        TestEqual("UnrealTypes Set", Struct.Set.Difference({0, 1, 2, 3, 4}).Num(), 0);
        TestEqual("UnrealTypes LinearColor", Struct.LinearColor, FColor::Red.ReinterpretAsLinear());
        TestEqual("UnrealTypes Color", Struct.Color, FColor::White);
        TestEqual("UnrealTypes Text", Struct.Text.ToString(), "this is some text");
        TestEqual("UnrealTypes Name", Struct.Name, FName("MyTestName"));
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
        FYamlSerializationResult Result = DeserializeStruct(Node, Struct, FYamlDeserializeOptions::Strict());
        TestTrue("UnrealReferenceTypes", !!Result);

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
        FYamlSerializationResult Result = DeserializeStruct(Node, Struct, FYamlDeserializeOptions::Strict());

        TestTrue("UnrealReferenceTypes success", Result.Success());

        TestTrue("UnrealReferenceTypes TSubclassOf Blueprint", Struct.SubclassOf.Get() != nullptr);
    }

    // Unreal reference types fail if the referenced entities cannot be found.
    {
        auto Yaml = TEXT(R"yaml(
subclassOf: "not a uclass"
softObjectPtr: "not a uobject"
)yaml");

        AssertInvalidDeserialization<FUnrealReferenceTypeStruct>(Yaml, TEXT("Invalid UnrealReferenceTypes"), this, {
            "SubclassOf: Cannot find class: not a uclass",
            "SoftObjectPtr: Cannot find object: not a uobject",
        });
    }

    // Tests negative integers are parsed.
    {
        const auto Yaml = TEXT("int: -1");
        FYamlNode Node;
        UYamlParsing::ParseYaml(Yaml, Node);

        FSimpleStruct Struct;
        FYamlSerializationResult Result = DeserializeStruct(Node, Struct, FYamlDeserializeOptions::Strict());

        TestTrue("NegativeInteger", Result.Success());
        TestEqual("NegativeInteger Value", Struct.Int, -1);
    }

    // Tests that the YamlRequired meta flag rejects missing YAML.
    {
        const auto Yaml = TEXT("optional: 13");

        AssertInvalidDeserialization<FRequiredFieldsStruct>(Yaml, TEXT("Required: missing"), this, {
            "Required: Missing Required Key: Required"
        });
    }

    // Tests that the YamlRequired meta flag passes if present.
    {
        const auto Yaml = TEXT("{ optional: 13, required: -1 }");
        FYamlNode Node;
        UYamlParsing::ParseYaml(Yaml, Node);

        FRequiredFieldsStruct Struct;
        FYamlSerializationResult Result = DeserializeStruct(Node, Struct, FYamlDeserializeOptions::Strict());

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

        AssertInvalidDeserialization<FSimpleStruct>(Yaml, TEXT("Additional properties"), this, {
            ": Struct has additional unused Keys: randomprop"
        });
    }

    // Tests that custom types can be handled via custom TypeHandlers.
    {
        const auto Yaml = TEXT("customtype: 13");
        FYamlNode Node;
        UYamlParsing::ParseYaml(Yaml, Node);

        FWithCustomType Struct;
        auto Options = FYamlDeserializeOptions::Strict();
        Options.TypeHandlers.Emplace(
            TEXT("FCustomType"),
            FCustomTypeDeserializer::CreateLambda([](const FYamlNode& Node, const UScriptStruct* Struct, void* StructValue, FYamlSerializationResult& Result) {
                auto AsNumber = Node.As<int>();
                FCustomType Ct;
                Ct.Value = FString::FromInt(AsNumber);
                *static_cast<FCustomType*>(StructValue) = Ct;
            })
        );

        FYamlSerializationResult Result = DeserializeStruct(Node, Struct, Options);

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
void AssertInvalidDeserialization(const TCHAR* Yaml, const TCHAR* What, ConvertToStruct* TestCase, const TArray<FString> Errors) {
    FYamlNode Node;
    UYamlParsing::ParseYaml(Yaml, Node);

    StructType Struct;
    FYamlSerializationResult Result = DeserializeStruct(Node, Struct, FYamlDeserializeOptions::Strict());

    TestCase->TestFalse(FString::Printf(TEXT("%ls fails as expected"), What), Result.Success());
    if (!TestCase->TestEqual(FString::Printf(TEXT("%ls error count"), What), Result.Errors.Num(), Errors.Num())) {
        for (const auto& Entry : Result.Errors) {
            TestCase->AddError(FString::Printf(TEXT("Parse error: %s"), *Entry));
        }
        
        return;
    }

    for (auto I = 0; I < Errors.Num(); ++I) {
        TestCase->TestEqual(FString::Printf(TEXT("%ls error[%d]"), What, I), Result.Errors[I], Errors[I]);
    }
}

#endif
