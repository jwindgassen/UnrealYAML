// Copyright (c) 2021-2026, Forschungszentrum Jülich GmbH. All rights reserved.
// Licensed under the MIT License. See LICENSE file for details.

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

BEGIN_DEFINE_SPEC(FTestSerialization, "UnrealYAML.Serialization", Flags)

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

END_DEFINE_SPEC(FTestSerialization)


void FTestSerialization::Define() {
    Describe("Simple Structs", [this]() {
        It("should be serialized correctly", [this]() {
            const FSimpleStruct Simple;

            // Serialize the struct to a YAML node
            FYamlNode Node;
            const auto Result = SerializeStruct(Node, Simple, FYamlSerializeOptions{});
            TestTrue("Serialization should be successful", Result.Success());

            // Verify the serialized values
            TestEqual("Simple.Str", Node["str"].Scalar(), "Hello, World");
            TestEqual("Simple.Int", Node["int"].As<int32>(), 42);
            TestEqual("Simple.Bool", Node["bool"].As<bool>(), true);
            TestEqual("Simple.Arr", Node["arr"].As<TArray<int32>>(), {3, 4, 5});
            TestTrue("Simple.Map",
                     Node["map"].As<TMap<FString, int32>>().OrderIndependentCompareEqual(
                         TMap<FString, int32>{{"A", 1}, {"B", 2}}));
        });

        It("should serialize with custom values", [this]() {
            // Modify values in struct
            FSimpleStruct Simple;
            Simple.Str = "Custom String";
            Simple.Int = 123;
            Simple.Bool = false;
            Simple.Arr = {10, 20, 30};
            Simple.Map = {{"X", 100}, {"Y", 200}, {"Z", 300}};

            // Serialize the struct to a YAML node
            FYamlNode Node;
            const auto Result = SerializeStruct(Node, Simple, FYamlSerializeOptions{});
            TestTrue("Serialization should be successful", Result.Success());

            // Verify the serialized values
            TestEqual("Simple.Str", Node["str"].Scalar(), "Custom String");
            TestEqual("Simple.Int", Node["int"].As<int32>(), 123);
            TestEqual("Simple.Bool", Node["bool"].As<bool>(), false);
            TestEqual("Simple.Arr", Node["arr"].As<TArray<int32>>(), {10, 20, 30});
            TestTrue(
                "Simple.Map", 
                Node["map"].As<TMap<FString, int32>>().OrderIndependentCompareEqual(TMap<FString, int32>{{"X", 100}, {"Y", 200}, {"Z", 300}})
            );
        });

        It("should serialize as a dynamic struct", [this]() {
            FSimpleStruct Simple;

            // Serialize using UScriptStruct
            FYamlNode Node;
            const auto Result = SerializeStruct(Node, FSimpleStruct::StaticStruct(), &Simple, FYamlSerializeOptions{});
            TestTrue("Serialization should be successful", Result.Success());

            // Verify the serialized values
            TestEqual("Simple.Str", Node["str"].Scalar(), "Hello, World");
            TestEqual("Simple.Int", Node["int"].As<int32>(), 42);
            TestEqual("Simple.Bool", Node["bool"].As<bool>(), true);
            TestEqual("Simple.Arr", Node["arr"].As<TArray<int32>>(), {3, 4, 5});
            TestTrue(
                "Simple.Map", 
                Node["map"].As<TMap<FString, int32>>().OrderIndependentCompareEqual(TMap<FString, int32>{{"A", 1}, {"B", 2}})
            );
        });

        It("should handle empty arrays and maps", [this]() {
            FSimpleStruct Simple;
            Simple.Arr.Empty();
            Simple.Map.Empty();

            // Serialize the struct to a YAML node
            FYamlNode Node;
            const auto Result = SerializeStruct(Node, Simple, FYamlSerializeOptions{});
            TestTrue("Serialization should be successful", Result.Success());

            // Verify empty collections
            TestEqual("Empty array size", Node["arr"].Size(), 0);
            TestEqual("Empty map size", Node["map"].Size(), 0);
        });
    });

    Describe("Simple Objects", [this]() {
        It("should be serialized correctly", [this]() {
            USimpleObject* Simple = NewObject<USimpleObject>();

            // Serialize the struct to a YAML node
            FYamlNode Node;
            const auto Result = SerializeObject(Node, Simple, FYamlSerializeOptions{});
            TestTrue("Serialization should be successful", Result.Success());

            // Verify the serialized values
            TestEqual("Simple.Str", Node["str"].Scalar(), "Hello, World");
            TestEqual("Simple.Int", Node["int"].As<int32>(), 42);
            TestEqual("Simple.Bool", Node["bool"].As<bool>(), true);
            TestEqual("Simple.Arr", Node["arr"].As<TArray<int32>>(), {3, 4, 5});
            TestTrue(
                "Simple.Map", 
                Node["map"].As<TMap<FString, int32>>().OrderIndependentCompareEqual(TMap<FString, int32>{{"A", 1}, {"B", 2}})
            );
        });
    });

    Describe("Nested Structs", [this]() {
        It("should serialize nested structs correctly", [this]() {
            // Create a nested struct with known values
            FNestedStruct Struct;
            Struct.Inner.Strings = {"one", "two"};
            Struct.Inner.Float = 13.124f;

            Struct.ChildArray.Add(FChildStruct{{"three"}, 1.0f});
            Struct.ChildArray.Add(FChildStruct{{"four"}, 2.0f});

            Struct.ChildMap.Add("child1", FChildStruct{{"five", "six"}, 0.0f});
            Struct.ChildMap.Add("child2", FChildStruct{{"seven"}, -13.0f});
            Struct.ChildMap.Add("child3", FChildStruct{{}, -26.0f});

            // Serialize the struct to a YAML node
            FYamlNode Node;
            const auto Result = SerializeStruct(Node, Struct, FYamlSerializeOptions{});
            TestTrue("Serialization should be successful", Result.Success());

            // Verify the serialized values
            TestEqual("Inner.Strings[0]", Node["inner"]["strings"].As<TArray<FString>>(), {"one", "two"});
            TestEqual("Inner.Float", Node["inner"]["float"].As<float>(), 13.124f);

            if (TestEqual("ChildArray length", Node["childArray"].Size(), 2)) {
                TestEqual("ChildArray[0].Strings", Node["childArray"][0]["strings"].As<TArray<FString>>(), {"three"});
                TestEqual("ChildArray[0].Float", Node["childArray"][0]["float"].As<float>(), 1.0f);
                TestEqual("ChildArray[1].Strings", Node["childArray"][1]["strings"].As<TArray<FString>>(), {"four"});
                TestEqual("ChildArray[1].Float", Node["childArray"][1]["float"].As<float>(), 2.0f);
            }

            if (TestEqual("ChildMap length", Node["childMap"].Size(), 3)) {
                TestEqual("ChildMap.child1.Strings", Node["childMap"]["child1"]["strings"].As<TArray<FString>>(), {"five", "six"});
                TestEqual("ChildMap.child1.Float", Node["childMap"]["child1"]["float"].As<float>(), 0.0f);
    
                TestEqual("ChildMap.child2.Strings", Node["childMap"]["child2"]["strings"].As<TArray<FString>>(), {"seven"});
                TestEqual("ChildMap.child2.Float", Node["childMap"]["child2"]["float"].As<float>(), -13.0f);

                TestEqual("ChildMap.child3.Strings", Node["childMap"]["child3"]["strings"].As<TArray<FString>>(), {});
                TestEqual("ChildMap.child3.Float", Node["childMap"]["child3"]["float"].As<float>(), -26.0f);
            }
        });

        It("should serialize empty nested structs", [this]() {
            const FNestedStruct Struct;

            // Serialize the struct to a YAML node
            FYamlNode Node;
            const auto Result = SerializeStruct(Node, Struct, FYamlSerializeOptions{});
            TestTrue("Serialization should be successful", Result.Success());

            // Verify the serialized values are empty
            TestEqual("Inner.Strings size", Node["inner"]["strings"].Size(), 0);
            TestEqual("Inner.Float", Node["inner"]["float"].As<float>(), 0.0f);
            TestEqual("ChildArray size", Node["childArray"].Size(), 0);
            TestEqual("ChildMap size", Node["childMap"].Size(), 0);
        });
    });

    Describe("Capitalization", [this]() {
        It("should correctly capitalize in PascalCase", [this]() {
            FYamlSerializeOptions Options;
            Options.Capitalization = EYamlKeyCapitalization::PascalCase;

            // Serialize a Struct to a YAML node
            FYamlNode Node;
            const auto Result = SerializeStruct(Node, FStructWithVariousKeyNames{}, Options);
            TestTrue("Serialization should be successful", Result.Success());

            // Check if keys are CamelCase!
            const TSet<FString> Keys = Node.Keys<FString>();
            TestTrueExpr(Keys.Contains("AnInteger"));
            TestTrueExpr(Keys.Contains("AnFString"));
            TestTrueExpr(Keys.Contains("NotCamelCase"));
            TestTrueExpr(Keys.Contains("AnIntegerPlus2"));
            TestTrueExpr(Keys.Contains("AnIntegerPlus42"));
            TestTrueExpr(Keys.Contains("CapitalA"));
        });

        It("should correctly capitalize in CamelCase", [this]() {
            FYamlSerializeOptions Options;
            Options.Capitalization = EYamlKeyCapitalization::CamelCase;

            // Serialize a Struct to a YAML node
            FYamlNode Node;
            const auto Result = SerializeStruct(Node, FStructWithVariousKeyNames{}, Options);
            TestTrue("Serialization should be successful", Result.Success());

            // Check if keys are CamelCase!
            const TSet<FString> Keys = Node.Keys<FString>();
            TestTrueExpr(Keys.Contains("anInteger"));
            TestTrueExpr(Keys.Contains("anFString"));
            TestTrueExpr(Keys.Contains("notCamelCase"));
            TestTrueExpr(Keys.Contains("anIntegerPlus2"));
            TestTrueExpr(Keys.Contains("anIntegerPlus42"));
            TestTrueExpr(Keys.Contains("capitalA"));
        });

        It("should correctly capitalize in SnakeCase", [this]() {
            FYamlSerializeOptions Options;
            Options.Capitalization = EYamlKeyCapitalization::SnakeCase;

            // Serialize a Struct to a YAML node
            FYamlNode Node;
            const auto Result = SerializeStruct(Node, FStructWithVariousKeyNames{}, Options);
            TestTrue("Serialization should be successful", Result.Success());

            // Check if keys are CamelCase!
            const TSet<FString> Keys = Node.Keys<FString>();
            TestTrueExpr(Keys.Contains("an_integer"));
            TestTrueExpr(Keys.Contains("an_fstring"));
            TestTrueExpr(Keys.Contains("not_camel_case"));
            TestTrueExpr(Keys.Contains("an_integer_plus_2"));
            TestTrueExpr(Keys.Contains("an_integer_plus_42"));
            TestTrueExpr(Keys.Contains("capital_a"));
        });
    });

    Describe("Enums", [this]() {
        It("should correctly serialize as strings", [this]() {
            FEnumStruct EnumStruct;
            EnumStruct.EnumValue = EEnumClass::Value2;
            EnumStruct.EnumAsByte = ENamespaceEnum::Value3;

            FYamlSerializeOptions Options;
            Options.EnumAsNumber = false;

            // Serialize a Struct to a YAML node
            FYamlNode Node;
            const auto Result = SerializeStruct(Node, EnumStruct, Options);
            TestTrue("Serialization should be successful", Result.Success());

            TestEqual("EnumValue", Node["enumValue"].Scalar(), "Value2");
            TestEqual("EnumAsByte", Node["enumAsByte"].Scalar(), "Value3");
        });

        It("should correctly serialize as integers", [this]() {
            FEnumStruct EnumStruct;
            EnumStruct.EnumValue = EEnumClass::Value2;
            EnumStruct.EnumAsByte = ENamespaceEnum::Value3;

            FYamlSerializeOptions Options;
            Options.EnumAsNumber = true;

            // Serialize a Struct to a YAML node
            FYamlNode Node;
            const auto Result = SerializeStruct(Node, EnumStruct, Options);
            TestTrue("Serialization should be successful", Result.Success());
            FString S = Node.Scalar();

            TestEqual("EnumValue", Node["enumValue"].As<int32>(), 43);
            TestEqual("EnumAsByte", Node["enumAsByte"].As<int32>(), 47);
        });
    });

    Describe("References and Classes", [this]() {
        It("should be serialized correctly", [this]() {
            FReferencesStruct References;
            References.Class = AActor::StaticClass();
            References.MeshObject = LoadObject<UStaticMesh>(
                nullptr,  TEXT("/Script/Engine.StaticMesh'/Engine/BasicShapes/Cube.Cube'")
            );
            
            FYamlNode Node;
            const auto Result = SerializeStruct(Node, References, FYamlSerializeOptions{});
            TestTrue("Serialization should be successful", Result.Success());

            // Verify both are serialized correctly
            TestEqual("Class", Node["class"].Scalar(), "/Script/CoreUObject.Class'/Script/Engine.Actor'");
            TestEqual("MeshObject", Node["meshObject"].Scalar(), "/Script/Engine.StaticMesh'/Engine/BasicShapes/Cube.Cube'");
        });
    });

    Describe("Native Types", [this]() {
        It("should serialize native types correctly", [this]() {
            FNativeTypesStruct Struct;
            Struct.Transform = FTransform(FQuat::Identity, FVector(1, 2, 3), FVector(2));
            Struct.Rotator = FRotator(90, 180, 0);
            Struct.Vector = FVector(13.23f, 0.0f, -12.4f);
            Struct.Vector2D = FVector2D(5.0f, 4.0f);
            Struct.Set = {0, 1, 2, 3, 4};
            Struct.LinearColor = FLinearColor::Red;
            Struct.Color = FColor::White;
            Struct.Text = FText::FromString("this is some text");
            Struct.Name = FName("MyTestName");

            // Serialize the struct to a YAML node
            FYamlNode Node;
            const auto Result = SerializeStruct(Node, Struct, FYamlSerializeOptions{});
            TestTrue("Serialization should be successful", Result.Success());

            // Verify the serialized values
            TestTrue("Transform", Node["transform"].As<FTransform>().Equals({FQuat::Identity, {1, 2, 3}, FVector(2)}));
            TestTrue("Rotator", Node["rotator"].As<FRotator>().Equals({90, 180, 0}));
            TestTrue("Vector", Node["vector"].As<FVector>().Equals({13.23f, 0.0f, -12.4f}));
            TestTrue("Vector2D", Node["vector2D"].As<FVector2D>().Equals({5.0f, 4.0f}));
            TestEqual("Set", Node["set"].As<TSet<int32>>().Array(), TArray{0, 1, 2, 3, 4});
            TestEqual("LinearColor", Node["linearColor"].Scalar(), "red");
            TestEqual("Color size", Node["color"].Scalar(), "white");
            TestEqual("Text", Node["text"].Scalar(), "this is some text");
            TestEqual("Name", Node["name"].Scalar(), "MyTestName");
        });
    });

    Describe("Type Information", [this]() {
        It("should be emitted for maps", [this]() {
            FObjectTypes Objects;
            Objects.Values = TMap<FString, int32>{{"a", 1}, {"b", 2}};
            Objects.ChildStruct = FChildStruct{{"string1"}, 42.0f};

            Objects.ChildObject = NewObject<UChildObject>();
            Objects.ChildObject->Strings = {"string2"};
            Objects.ChildObject->Float = 43.0f;

            // Emit type information
            FYamlSerializeOptions Options;
            Options.IncludeTypeInformation = true;

            // Serialize the struct to a YAML node
            FYamlNode Node;
            const auto Result = SerializeStruct(Node, Objects, Options);
            TestTrue("Serialization should be successful", Result.Success());

            // Check if the Tags match the Structs
            TestEqual("Value", Node["values"].Tag(), "TMap");
            TestEqual("ChildStruct", Node["childStruct"].Tag(), "FChildStruct");
            TestEqual("ChildObject", Node["childObject"].Tag(), "UChildObject");
        });
    });

    Describe("Custom Type Handlers", [this]() {
        static auto ParseCustomType = [](
            const UScriptStruct*, const void* StructValue, FYamlSerializationResult& Results
        ) {
            const FCustomType* CustomType = static_cast<const FCustomType*>(StructValue);
            if (!CustomType->InnerValue.StartsWith(TEXT("CustomValue is "))) {
                Results.AddError(TEXT("Invalid Format of FCustomType"));
                return FYamlNode{};
            }

            const FString RemovedPrefix = CustomType->InnerValue.RightChop(15);
            const int32 Value = FCString::Atoi(*RemovedPrefix);
            return FYamlNode{Value};
        };

        It("should use custom type handler for serialization", [this]() {
            // Create a custom type struct
            FCustomTypeStruct CustomTypeStruct;
            CustomTypeStruct.CustomValue.InnerValue = "CustomValue is 42";

            // Register custom type handler
            FYamlSerializeOptions Options;
            Options.TypeHandlers.Emplace("FCustomType", FCustomTypeSerializer::CreateStatic(ParseCustomType));

            // Ensure Serialization workds
            FYamlNode Node;
            const auto Result = SerializeStruct(Node, CustomTypeStruct, Options);
            TestTrue("Serialization should be successful", Result.Success());
            TestEqual("CustomValue", Node["customValue"].As<int32>(), 42);
        });

        It("should propagate errors from custom type handler", [this]() {
            FCustomTypeStruct CustomTypeStruct;
            CustomTypeStruct.CustomValue.InnerValue = "Invalid InnerValue!";

            AddExpectedError("Invalid Format of FCustomType");

            // Register custom type handler that adds an error
            FYamlSerializeOptions Options;
            Options.TypeHandlers.Emplace("FCustomType", FCustomTypeSerializer::CreateStatic(ParseCustomType));

            // Parse and check if errors actually occurred
            FYamlNode Node;
            const auto Result = SerializeStruct(Node, CustomTypeStruct, Options);
            CheckExpectedErrors(Result);
        });
    });
}

#endif