#include "Inputs.h"
#include "TestStructs.h"
#include "Misc/AutomationTest.h"
#include "YamlParsing.h"
#include "YamlSerialization.h"

#if WITH_DEV_AUTOMATION_TESTS

constexpr EAutomationTestFlags Flags = EAutomationTestFlags::ProductFilter | EAutomationTestFlags::HighPriority |
#if ENGINE_MAJOR_VERSION >= 5
    EAutomationTestFlags_ApplicationContextMask;
#else
    EAutomationTestFlags::ApplicationContextMask;
#endif

BEGIN_DEFINE_SPEC(FTestDeserialization, "UnrealYAML.Deserialization", Flags)

    // Takes the errors from `AddExpectedErrors` and checks that the Results contain the expected error messages.
    void CheckExpectedErrors(const FYamlSerializationResult& Result) {
        TArray<FAutomationExpectedMessage> Messages;
        GetExpectedMessages(Messages);

        for (const FString& Error : Result.Errors) {
            FString Err = Error.TrimStartAndEnd();
            bool Expected = false;

            for (auto& Message : Messages) {
                FString Msg = Message.MessagePatternString.TrimStartAndEnd();
                Expected |= Message.Matches(Error);
            }

            if (!Expected) {
                AddError(FString::Printf(TEXT("Deserialization generated unexpected error: %s"), *Error));
            }
        }
    }

END_DEFINE_SPEC(FTestDeserialization)


void FTestDeserialization::Define() {
    Describe("Simple Structs", [this]() {
        It("should be parseable from the templated wrapper", [this]() {
            FYamlNode Node;
            UYamlParsing::ParseYaml(SimpleYaml, Node);

            // Ensure parsing produces no errors
            FSimpleStruct Simple;
            const auto Result = DeserializeStruct(Node, Simple, FYamlDeserializeOptions::Strict());
            TestTrue("Deserialization should be successful", Result.Success());

            // Test whether fields match
            TestEqual("Simple.Str", Simple.Str, "A String");
            TestEqual("Simple.Int", Simple.Int, 42);
            TestEqual("Simple.Bool", Simple.Bool, true);
            TestEqual("Simple.Arr", Simple.Arr, {1, 2, 3});
            TestTrue(
                "Simple.Map == {\"a\" => 1, \"b\" => 3}",
                Simple.Map.OrderIndependentCompareEqual(TMap<FString, int32>{{"a", 1}, {"b", 2}})
            );
        });

        It("should be parseable to a dynamic struct", [this]() {
            FYamlNode Node;
            UYamlParsing::ParseYaml(SimpleYaml, Node);

            // Ensure parsing produces no errors
            uint8* SimpleData = (uint8*)FMemory::Malloc(FSimpleStruct::StaticStruct()->GetStructureSize());
            FSimpleStruct::StaticStruct()->InitializeDefaultValue(SimpleData);
            const auto Result = DeserializeStruct(Node, FSimpleStruct::StaticStruct(), SimpleData, FYamlDeserializeOptions::Strict());
            TestTrue("Deserialization should be successful", Result.Success());

            // Test whether fields match
            const FSimpleStruct Simple = *reinterpret_cast<FSimpleStruct*>(SimpleData);
            // Test whether fields match
            TestEqual("Simple.Str", Simple.Str, "A String");
            TestEqual("Simple.Int", Simple.Int, 42);
            TestEqual("Simple.Bool", Simple.Bool, true);
            TestEqual("Simple.Arr", Simple.Arr, {1, 2, 3});
            TestTrue(
                "Simple.Map == {\"a\" => 1, \"b\" => 3}",
                Simple.Map.OrderIndependentCompareEqual(TMap<FString, int32>{{"a", 1}, {"b", 2}})
            );
        });

        It("should print errors when the types do not match", [this]() {
            FYamlNode Node;
            UYamlParsing::ParseYaml(SimpleYamlWrongTypes, Node);

            AddExpectedErrorPlain("Int: Cannot convert 'not an int' to an Integer");
            AddExpectedErrorPlain("Bool: Expected 'Scalar' but found 'Map'");
            AddExpectedErrorPlain("Arr: Expected 'Sequence' but found 'Map'");
            AddExpectedErrorPlain("Map: Expected 'Map' but found 'Sequence'");

            // Parse and check if errors actually occurred
            FSimpleStruct Simple;
            const auto Result = DeserializeStruct(Node, Simple, FYamlDeserializeOptions::Strict());
            CheckExpectedErrors(Result);
        });

        It("should not print errors when using default options", [this]() {
            FYamlNode Node;
            UYamlParsing::ParseYaml(SimpleYamlWrongTypes, Node);

            // Ensure parsing produces no errors
            FSimpleStruct Simple;
            const auto Result = DeserializeStruct(Node, Simple);
            TestTrue("Deserialization should be successful", Result.Success());

            // Default options => Any wrong values will be skipped
            // Ints: atoi returns 0 when value cannot be parsed
            // Arrays: contain as many defaulted values as the actual value in the node is in size
            // Maps: Iterator.Key will return the index when the Node is a Sequence
            // ToDo: This varies wildly, we should probably enforce skipping for all types!
            TestEqual("Simple.Str", Simple.Str, "Hello, World");
            TestEqual("Simple.Int", Simple.Int, 0);
            TestEqual("Simple.Bool", Simple.Bool, true);
            TestEqual("Simple.Arr", Simple.Arr, {0});
            TestTrue(
                "Simple.Map == {\"0\" => 42, \"1\" => 43, \"2\" => 44}",
                Simple.Map.OrderIndependentCompareEqual(TMap<FString, int32>{{"0", 42}, {"1", 43}, {"2", 44}})
            );
        });

        It("should preserve default values when there is no entry in the YamlNode", [this]() {
            FYamlNode Node;
            UYamlParsing::ParseYaml("{}", Node);

            // Ensure parsing produces no errors
            FSimpleStruct Simple;
            const auto Result = DeserializeStruct(Node, Simple);
            TestTrue("Deserialization should be successful", Result.Success());

            // Test for default values
            TestEqual("Simple.Str", Simple.Str, "Hello, World");
            TestEqual("Simple.Int", Simple.Int, 42);
            TestEqual("Simple.Bool", Simple.Bool, true);
            TestEqual("Simple.Arr", Simple.Arr, {3, 4, 5});
            TestTrue(
                "Simple.Map == {\"0\" => 42, \"1\" => 43, \"2\" => 44}",
                Simple.Map.OrderIndependentCompareEqual(TMap<FString, int32>{{"A", 1}, {"B", 2}})
            );
        });

        It("should print error when YAML has additional unused values", [this]() {
            FYamlNode Node;
            UYamlParsing::ParseYaml(SimpleYaml + "foo: bar\n", Node);

            AddExpectedErrorPlain("<root>: Struct has additional unused Keys: foo");

            // Parse and check if errors actually occurred
            FSimpleStruct Simple;
            const auto Result = DeserializeStruct(Node, Simple, FYamlDeserializeOptions::Strict());
            CheckExpectedErrors(Result);
        });
    });

    Describe("Simple Objects", [this]() {
        It("should be parsed correctly", [this]() {
            FYamlNode Node;
            UYamlParsing::ParseYaml(SimpleYaml, Node);

            // Ensure parsing produces no errors
            USimpleObject* Simple = NewObject<USimpleObject>();
            const auto Result = DeserializeObject(Node, Simple, FYamlDeserializeOptions::Strict());
            TestTrue("Deserialization should be successful", Result.Success());

            // Test whether fields match
            TestEqual("Simple.Str", Simple->Str, "A String");
            TestEqual("Simple.Int", Simple->Int, 42);
            TestEqual("Simple.Bool", Simple->Bool, true);
            TestEqual("Simple.Arr", Simple->Arr, {1, 2, 3});
            TestTrue(
                "Simple.Map == {\"a\" => 1, \"b\" => 3}",
                Simple->Map.OrderIndependentCompareEqual(TMap<FString, int32>{{"a", 1}, {"b", 2}})
            );
        });
    });

    Describe("Nested Structs", [this]() {
        It("should be parsed correctly", [this]() {
            FYamlNode Node;
            UYamlParsing::ParseYaml(NestedStruct, Node);

            FNestedStruct Struct;
            const auto Result = DeserializeStruct(Node, Struct, FYamlDeserializeOptions::Strict());
            TestTrue("Deserialization should be successful", Result.Success());

            // Test Inner
            TestEqual("Inner.Strings", Struct.Inner.Strings, TArray<FString>{"one", "two"});
            TestEqual("Inner.Float", Struct.Inner.Float, 13.124f);

            // Test ChildArray
            if (TestEqual("Length of ChildArray", Struct.ChildArray.Num(), 2)) {
                TestEqual("ChildArray[0].Strings", Struct.ChildArray[0].Strings, {"three"});
                TestEqual("ChildArray[0].Float", Struct.ChildArray[0].Float, 1.0f);
                TestEqual("ChildArray[1].Strings", Struct.ChildArray[1].Strings, {"four"});
                TestEqual("ChildArray[1].Float", Struct.ChildArray[1].Float, 2.0f);
            };

            // Test ChildMap
            if (TestEqual("Number of Entries in ChildMap", Struct.ChildMap.Num(), 3)) {
                const FChildStruct* Child1 = Struct.ChildMap.Find("child1");
                if (TestTrue("Child1 exists", Child1 != nullptr)) {
                    TestEqual("Child1.Strings", Child1->Strings, {"five", "six"});
                    TestEqual("Child1.Float", Child1->Float, 0.0f);
                }

                const FChildStruct* Child2 = Struct.ChildMap.Find("child2");
                if (TestTrue("Child2 exists", Child2 != nullptr)) {
                    TestEqual("Child2.Strings", Child2->Strings, {"seven"});
                    TestEqual("Child2.Float", Child2->Float, -13.0f);
                }

                const FChildStruct* Child3 = Struct.ChildMap.Find("child3");
                if (TestTrue("Child3 exists", Child3 != nullptr)) {
                    TestEqual("Child3.Strings", Child3->Strings, {});
                    TestEqual("Child3.Float", Child3->Float, -26.0f);
                }
            }
        });

        It("should not parse invalid nested objects", [this]() {
            FYamlNode Node;
            UYamlParsing::ParseYaml(NestedStructInvalid, Node);

            AddExpectedErrorPlain("Inner.Strings: Expected 'Sequence' but found 'Map'");
            AddExpectedErrorPlain("Inner.Float: Cannot convert 'foobar' to a Float");
            AddExpectedErrorPlain("ChildArray.[0]: Expected 'Map' but found 'Scalar'");
            AddExpectedErrorPlain("ChildMap.child1: Expected 'Map' but found 'Sequence'");
            AddExpectedErrorPlain("ChildMap.notAnObject: Expected 'Map' but found 'Scalar'");

            // Parse and check if errors actually occurred
            FNestedStruct Struct;
            const auto Result = DeserializeStruct(Node, Struct, FYamlDeserializeOptions::Strict());
            CheckExpectedErrors(Result);
        });
    });

    Describe("Enums", [this]() {
        It("should parse from a string", [this]() {
            FYamlNode Node;
            UYamlParsing::ParseYaml("enumValue: value2\nenumAsByte: value3", Node);

            FEnumStruct EnumStruct;
            const auto Result = DeserializeStruct(Node, EnumStruct, FYamlDeserializeOptions::Strict());

            TestTrue("Deserialization should be successful", Result.Success());
            TestEqual("EnumValue matches", EnumStruct.EnumValue, EEnumClass::Value2);
            TestEqual("EnumAsByte matches", EnumStruct.EnumAsByte.GetValue(), ENamespaceEnum::Value3);
        });

        It("should parse from a integer", [this]() {
            FYamlNode Node;
            UYamlParsing::ParseYaml("enumValue: 43\nenumAsByte: 47\n", Node);

            FEnumStruct EnumStruct;
            const auto Result = DeserializeStruct(Node, EnumStruct, FYamlDeserializeOptions::Strict());

            TestTrue("Deserialization should be successful", Result.Success());
            TestEqual("EnumValue matches", EnumStruct.EnumValue, EEnumClass::Value2);
            TestEqual("EnumAsByte matches", EnumStruct.EnumAsByte.GetValue(), ENamespaceEnum::Value3);
        });

        It("should not parse from invalid strings", [this]() {
            FYamlNode Node;
            UYamlParsing::ParseYaml("enumValue: foo\nenumAsByte: bar\n", Node);

            AddExpectedErrorPlain("EnumValue: 'foo' is not an valid enum value of EEnumClass");
            AddExpectedErrorPlain("EnumAsByte: 'bar' is not an valid enum value of ENamespaceEnum");

            // Parse and check if errors actually occurred
            FEnumStruct EnumStruct;
            const auto Result = DeserializeStruct(Node, EnumStruct, FYamlDeserializeOptions::Strict());
            CheckExpectedErrors(Result);
        });

        It("should not parse from integers not matching any underlying values", [this]() {
            FYamlNode Node;
            UYamlParsing::ParseYaml("enumValue: 1\nenumAsByte: 2\n", Node);

            AddExpectedErrorPlain("EnumValue: 1 is not an valid enum value of EEnumClass");
            AddExpectedErrorPlain("EnumAsByte: 2 is not an valid enum value of ENamespaceEnum");

            // Parse and check if errors actually occurred
            FEnumStruct EnumStruct;
            const auto Result = DeserializeStruct(Node, EnumStruct, FYamlDeserializeOptions::Strict());
            CheckExpectedErrors(Result);
        });
    });

    Describe("References and Classes", [this]() {
        It("should correctly work with Blueprints", [this]() {
            FYamlNode Node;
            UYamlParsing::ParseYaml(
                "class: \"/Script/Engine.Blueprint'/Engine/EngineSky/BP_Sky_Sphere.BP_Sky_Sphere'\" \n"
                "meshObject: \"/Script/Engine.StaticMesh'/Engine/BasicShapes/Cube.Cube'\" \n",
                Node
            );

            FReferencesStruct References;
            const auto Result = DeserializeStruct(Node, References, FYamlDeserializeOptions::Strict());
            TestTrue("Deserialization should be successful", Result.Success());

            TestEqual("Class", References.Class->GetName(), "BP_Sky_Sphere_C");
            TestTrue("Class is a Blueprint", References.Class->IsInBlueprint());
            TestTrue("MeshObject valid", References.MeshObject.IsValid());
            TestTrue("MeshObject has Vertices", References.MeshObject.LoadSynchronous()->GetNumVertices(0) > 0);
        });

        It("should correctly work with C++ classes", [this]() {
            FYamlNode Node;
            UYamlParsing::ParseYaml("class: \"/Script/CoreUObject.Class'/Script/Engine.Actor'\"\n", Node);

            FReferencesStruct References;
            const auto Result = DeserializeStruct(Node, References, FYamlDeserializeOptions::Strict());
            TestTrue("Deserialization should be successful", Result.Success());

            TestEqual("Class", *References.Class, AActor::StaticClass());
        });

        It("should not parse from invalid object and class names", [this]() {
            FYamlNode Node;
            UYamlParsing::ParseYaml("class: not a class\nmeshObject: not an object\n", Node);

            AddExpectedErrorPlain("Class: Cannot find Class 'not a class'");
            AddExpectedErrorPlain("MeshObject: Cannot find Object 'not an object'");

            // Parse and check if errors actually occurred
            FReferencesStruct References;
            const auto Result = DeserializeStruct(Node, References, FYamlDeserializeOptions::Strict());
            CheckExpectedErrors(Result);
        });
    });

    Describe("Native Types", [this]() {
        It("should parse correctly", [this]() {
            FYamlNode Node;
            UYamlParsing::ParseYaml(NativeTypesStruct, Node);

            // Ensure parsing produces no errors
            FNativeTypesStruct Struct;
            const auto Result = DeserializeStruct(Node, Struct, FYamlDeserializeOptions::Strict());
            TestTrue("Deserialization should be successful", Result.Success());

            TestEqual("Transform.Location", Struct.Transform.GetLocation(), FVector(1, 2, 3));
            TestEqual("Transform.Rotation", Struct.Transform.Rotator(), FRotator(0, 90, 0));
            TestEqual("Transform.Scale", Struct.Transform.GetScale3D(), FVector(2));
            TestEqual("Quat", Struct.Quat, FQuat::Identity);
            TestEqual("Rotator", Struct.Rotator, FRotator(90, 180, 0));
            TestEqual("Vector", Struct.Vector, FVector(13.23, 0, -12.4));
            TestEqual("Vector2D", Struct.Vector2D, FVector2D(5, 4));
            TestEqual("Set", Struct.Set.Difference({0, 1, 2, 3, 4}).Num(), 0);
            TestEqual("LinearColor", Struct.LinearColor, FColor::Red.ReinterpretAsLinear());
            TestEqual("Color", Struct.Color, FColor::White);
            TestEqual("Text", Struct.Text.ToString(), "this is some text");
            TestEqual("Name", Struct.Name, FName("MyTestName"));
        });
    });

    Describe("Required Fields", [this]() {
        It("should parse correctly when value is given", [this]() {
            FYamlNode Node;
            UYamlParsing::ParseYaml("required: 1\noptional: 2", Node);

            FRequiredFieldsStruct Struct;
            const auto Result = DeserializeStruct(Node, Struct, FYamlDeserializeOptions::Strict());

            TestTrue("Deserialization should be successful", Result.Success());
            TestEqual("Required", Struct.Required, 1);
            TestEqual("Optional", Struct.Optional, 2);
        });

        It("should print an error when required value is missing", [this]() {
            FYamlNode Node;
            UYamlParsing::ParseYaml("optional: 2", Node);

            AddExpectedErrorPlain("Required: Missing Required Key: Required");

            // Parse and check if errors actually occurred
            FRequiredFieldsStruct Struct;
            const auto Result = DeserializeStruct(Node, Struct, FYamlDeserializeOptions::Strict());
            CheckExpectedErrors(Result);
        });
    });

    Describe("Custom Type Handlers", [this]() {
        // Custom Parser
        static auto ParseCustomType = [](
            const FYamlNode& Node, const UScriptStruct* Struct, void* StructValue, FYamlSerializationResult& Result
        ) {
            if (!Node.IsScalar() || !Node.CanConvertTo<int32>()) {
                Result.AddError(TEXT("Value cannot be parsed to a FCustomType"));
            }

            const FString Value = FString::Printf(TEXT("CustomValue is %d"), Node.As<int32>());
            static_cast<FCustomType*>(StructValue)->InnerValue = Value;
        };

        It("should correctly deserialize a value", [this]() {
            FYamlNode Node;
            UYamlParsing::ParseYaml("customValue: 42", Node);

            // Register custom type handlers
            FYamlDeserializeOptions Options = FYamlDeserializeOptions::Strict();
            Options.TypeHandlers.Emplace("FCustomType", FCustomTypeDeserializer::CreateStatic(ParseCustomType));

            // Ensure parsing produces no errors
            FCustomTypeStruct CustomTypeStruct;
            const auto Result = DeserializeStruct(Node, CustomTypeStruct, Options);
            TestTrue("Deserialization should be successful", Result.Success());

            TestEqual("Parsed Value", CustomTypeStruct.CustomValue.InnerValue, "CustomValue is 42");
        });

        It("should correctly propagate a printed error", [this]() {
            FYamlNode Node;
            UYamlParsing::ParseYaml("customValue: not an int!", Node);

            AddExpectedError("CustomValue: Value cannot be parsed to a FCustomType");

            // Register custom type handlers
            FYamlDeserializeOptions Options = FYamlDeserializeOptions::Strict();
            Options.TypeHandlers.Emplace("FCustomType", FCustomTypeDeserializer::CreateStatic(ParseCustomType));

            // Parse and check if errors actually occurred
            FCustomTypeStruct CustomTypeStruct;
            const auto Result = DeserializeStruct(Node, CustomTypeStruct, Options);
            CheckExpectedErrors(Result);
        });
    });
}

#endif
