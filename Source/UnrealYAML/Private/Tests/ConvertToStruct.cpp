#include "Misc/AutomationTest.h"
#include "Parsing.h"
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

bool ConvertToStruct::RunTest(const FString& Parameters) {
    // Simple YAML to Struct
    {
        FYamlNode Node;
        UYamlParsing::ParseYaml(SimpleYaml, Node);

        FSimpleStruct SimpleStruct;
        TestTrue("Parse Node into SimpleStruct", ParseNodeIntoStruct(Node, SimpleStruct));
    }
    
    // Simple YAML to Object
    {
        FYamlNode Node;
        UYamlParsing::ParseYaml(SimpleYaml, Node);

        USimpleObject* SimpleObject = NewObject<USimpleObject>(GetTransientPackage());
        TestTrue("Parse Node into SimpleObject", ParseNodeIntoObject(Node, SimpleObject));
    }

    return !HasAnyErrors();
}


#endif
