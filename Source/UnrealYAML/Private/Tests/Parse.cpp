#include "Misc/AutomationTest.h"
#include "Parsing.h"
#include "Inputs.h"

#if WITH_DEV_AUTOMATION_TESTS

// For Engine Versions <5
#ifndef UE_PI
#define UE_PI PI
#endif

#if ENGINE_MAJOR_VERSION >= 5
IMPLEMENT_SIMPLE_AUTOMATION_TEST(Parsing, "UnrealYAML.Parsing",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)
#else
IMPLEMENT_SIMPLE_AUTOMATION_TEST(Parsing, "UnrealYAML.Parsing",
                                 EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::SmokeFilter)
#endif

bool Parsing::RunTest(const FString& Parameters) {
    // From String
    {
        FYamlNode Node;
        TestTrue("Parse Empty", UYamlParsing::ParseYaml(EmptyYaml, Node));
        TestTrue("Parse Simple", UYamlParsing::ParseYaml(SimpleYaml, Node));
        TestTrue("Parse Complex", UYamlParsing::ParseYaml(ComplexYaml, Node));
        TestFalse("Parse Erroneous", UYamlParsing::ParseYaml(ErroneousYaml, Node));
    }

    // Simple
    {
        FYamlNode Node;
        UYamlParsing::ParseYaml(SimpleYaml, Node);

        TestEqual("Parse String", Node["str"].As<FString>(), "A String");
        TestEqual("Parse Integer", Node["int"].As<int32>(), 42);
        TestTrue("Parse Boolean", Node["bool"].As<bool>());
        TestEqual("Parse Array", Node["arr"].As<TArray<int>>(), {1, 2, 3});
        TestTrue("Parse Map", Node["map"].As<TMap<FString, int32>>()
                                         .OrderIndependentCompareEqual(TMap<FString, int32>{{"a", 1}, {"b", 2}}));
    }

    {
        FYamlNode Node;
        UYamlParsing::ParseYaml(ComplexYaml, Node);

        TestTrue("Parse nested Array",
                 Node["nested"].IsSequence() &&
                 Node["nested"][0].As<TArray<int>>() == TArray<int>{1, 2, 3} &&
                 Node["nested"][1].As<TArray<FString>>() == TArray<FString>{"a", "b", "c", "d"} &&
                 Node["nested"][2].IsNull()
            );
        TestTrue("Parse mixed Array",
                 Node["mixed"][0].As<bool>() &&
                 FMath::IsNearlyEqual(Node["mixed"][1].As<float>(), UE_PI)
            );
        TestTrue("Parse Color and Set",
                 Node["struct"].IsMap() &&
                 Node["struct"]["color"].As<FColor>() == FColor::Magenta &&
                 Node["struct"]["set"].As<TSet<int>>().Difference(TSet<int>{0, 1, 3, 5, 6}).Num() == 0
            );
    }

    return !HasAnyErrors();
}


#endif
