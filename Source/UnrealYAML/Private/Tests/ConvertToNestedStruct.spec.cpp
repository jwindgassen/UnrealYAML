#include "Misc/AutomationTest.h"
#include "YamlParsing.h"
#include "TestStructs.h"

#if WITH_DEV_AUTOMATION_TESTS

#if ENGINE_MAJOR_VERSION >= 5
DEFINE_SPEC(FConvertToNestedStructSpec, "UnrealYAML.ConvertToNestedStruct",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
#else
DEFINE_SPEC(FConvertToNestedStructSpec, "UnrealYAML.ConvertToNestedStruct",
                                 EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)
#endif

void FConvertToNestedStructSpec::Define()
{
    Describe("ParseInto parent/child", [this]() {
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
      anenum: value2
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
        FYamlParseIntoResult Result = ParseNodeIntoStruct(Node, Struct, FYamlParseIntoOptions::Strict());
        
        It("Embedded child values", [this, Result, Struct]() {
            TestTrue("Success", Result.Success());
            TestEqual("SomeValues", Struct.Embedded.SomeValues, {"one", "two"});
            TestEqual("AFloat", Struct.Embedded.AFloat, 13.124f);
            TestEqual("AnEnum", Struct.Embedded.AnEnum, TEnumAsByte(Value1));
        });
        
        It("Array child values", [this, Struct]() {
            if (!TestEqual("Array count", Struct.Children.Num(), 2)) {
                return;
            }
            
            TestEqual("Child[0].SomeValues", Struct.Children[0].SomeValues, {"three"});
            TestEqual("Child[0].AFloat", Struct.Children[0].AFloat, 1.0f);
            TestEqual("Child[0].AnEnum", Struct.Children[0].AnEnum, TEnumAsByte(Value3));
            TestEqual("Child[1].SomeValues", Struct.Children[1].SomeValues, {"four"});
            TestEqual("Child[1].AFloat", Struct.Children[1].AFloat, 2.0f);
            TestEqual("Child[1].AnEnum", Struct.Children[1].AnEnum, TEnumAsByte(Value2));
        });

        Describe("Map child values", [this, Struct]() {
            if (!TestEqual("map count", Struct.MappedChildren.Num(), 3)) {
                return;
            }
            
            It("value1", [this, Struct]() {
                if (TestTrue("key exists", Struct.MappedChildren.Contains(TEnumAsByte(Value1)))) {
                    TestEqual("SomeValues", Struct.MappedChildren[TEnumAsByte(Value1)].SomeValues, {"five", "six"});
                    TestEqual("AFloat", Struct.MappedChildren[TEnumAsByte(Value1)].AFloat, 0.0f);
                    TestEqual("AnEnum", Struct.MappedChildren[TEnumAsByte(Value1)].AnEnum, TEnumAsByte(Value1));
                }
            });
            
            It("value2", [this, Struct]() {
                if (TestTrue("key exists", Struct.MappedChildren.Contains(TEnumAsByte(Value2)))) {
                    TestEqual("SomeValues", Struct.MappedChildren[TEnumAsByte(Value2)].SomeValues, {"seven"});
                    TestEqual("AFloat", Struct.MappedChildren[TEnumAsByte(Value2)].AFloat, -13.f);
                    TestEqual("AnEnum", Struct.MappedChildren[TEnumAsByte(Value2)].AnEnum, TEnumAsByte(Value2));
                }
            });
            
            It("value3", [this, Struct]() {
                if (TestTrue("key exists", Struct.MappedChildren.Contains(TEnumAsByte(Value3)))) {
                    TestEqual("SomeValues", Struct.MappedChildren[TEnumAsByte(Value3)].SomeValues, {});
                    TestEqual("AFloat", Struct.MappedChildren[TEnumAsByte(Value3)].AFloat, -26.f);
                    TestEqual("AnEnum", Struct.MappedChildren[TEnumAsByte(Value3)].AnEnum, TEnumAsByte(Value3));
                }
            });
        });
    });
}

#endif
