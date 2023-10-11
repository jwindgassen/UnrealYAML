#include "Misc/AutomationTest.h"
#include "Parsing.h"
#include "Inputs.h"
#include "TestStructs.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(ConvertToStruct, "UnrealYAML.ConvertToStruct",
                                 EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool ConvertToStruct::RunTest(const FString& Parameters) {
    // Simple Yaml
    {
        FYamlNode Node;
        UYamlParsing::ParseYaml(SimpleYaml, Node);

        FSimpleStruct SimpleStruct;
        TestTrue("Parse Node into SimpleStruct", ParseNodeIntoStruct(Node, SimpleStruct));
    }
    
    return !HasAnyErrors();
}


#endif
