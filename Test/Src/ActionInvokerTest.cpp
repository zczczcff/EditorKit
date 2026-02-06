// Test for ActionInvoker - efficient execution without hash lookup
#include <iostream>
#include <string>
#include <vector>
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>
#include <EditorKit/ActionSystem.h>

// Test basic ActionInvoker functionality
TEST_CASE("ActionInvoker - Basic functionality", "[ActionSystem][ActionInvoker]")
{
    StringActionSystem system;
    std::string actionKey = "test_action";
    int processorExecuted = 0;

    // Add a processor
    system.AddSequentialProcessor(actionKey,
        [&processorExecuted](int value)
        {
            processorExecuted = value;
        }, "Test processor");

    SECTION("Acquire valid invoker and execute")
    {
        // Acquire invoker with explicit template parameter
        auto invoker = system.AcquireInvoker<int>(actionKey);

        REQUIRE(invoker.isValid() == true);
        REQUIRE(invoker.GetActionKey() == actionKey);

        // Execute through invoker
        auto result = invoker.Execute(42);

        REQUIRE(result.success == true);
        REQUIRE(processorExecuted == 42);
    }

    SECTION("Acquire invoker for non-existent action")
    {
        auto invoker = system.AcquireInvoker<int>("non_existent");
        REQUIRE(invoker.isValid() == false);
    }

    SECTION("Acquire invoker with wrong parameter type")
    {
        // Try to acquire invoker with wrong parameter type
        auto invoker = system.AcquireInvoker<std::string>(actionKey);
        REQUIRE(invoker.isValid() == false);
    }
}

// Test ActionInvoker performance advantage
TEST_CASE("ActionInvoker - Performance comparison", "[ActionSystem][ActionInvoker]")
{
    StringActionSystem system;
    std::string actionKey = "perf_test";
    int callCount = 0;

    system.AddSequentialProcessor(actionKey,
        [&callCount](int value)
        {
            callCount += value;
        }, "Perf processor");

    // Acquire invoker once
    auto invoker = system.AcquireInvoker<int>(actionKey);
    REQUIRE(invoker.isValid() == true);

    SECTION("Execute via invoker multiple times")
    {
        const int iterations = 1000;
        for (int i = 0; i < iterations; ++i)
        {
            invoker.Execute(1);
        }

        REQUIRE(callCount == iterations);
    }

    SECTION("Compare invoker vs direct Execute")
    {
        // Reset counter
        callCount = 0;

        // Execute via invoker
        invoker.Execute(10);
        REQUIRE(callCount == 10);

        // Execute via system
        auto result = system.Execute(actionKey, 5);
        REQUIRE(result.success == true);
        REQUIRE(callCount == 15);
    }
}

// Test ActionInvoker with multiple parameter types
TEST_CASE("ActionInvoker - Multiple parameter types", "[ActionSystem][ActionInvoker]")
{
    StringActionSystemOverload system;
    std::string actionKey = "multi_param";

    int intCount = 0;
    int stringCount = 0;
    int pairCount = 0;

    system.AddSequentialProcessor(actionKey,
        [&intCount](int x) { intCount = x; }, "Int processor");

    system.AddSequentialProcessor(actionKey,
        [&stringCount](const std::string& s) { stringCount = s.length(); }, "String processor");

    system.AddSequentialProcessor(actionKey,
        [&pairCount](int a, const std::string& b) { pairCount = a + b.length(); }, "Pair processor");

    SECTION("Acquire int invoker")
    {
        auto invoker = system.AcquireInvoker<int>(actionKey);
        REQUIRE(invoker.isValid() == true);

        invoker.Execute(42);
        REQUIRE(intCount == 42);
        REQUIRE(stringCount == 0);
        REQUIRE(pairCount == 0);
    }

    SECTION("Acquire string invoker")
    {
        auto invoker = system.AcquireInvoker<const std::string&>(actionKey);
        REQUIRE(invoker.isValid() == true);

        invoker.Execute(std::string("hello"));
        REQUIRE(intCount == 0);
        REQUIRE(stringCount == 5);
        REQUIRE(pairCount == 0);
    }

    SECTION("Acquire pair invoker")
    {
        auto invoker = system.AcquireInvoker<int, const std::string&>(actionKey);
        REQUIRE(invoker.isValid() == true);

        invoker.Execute(10, std::string("test"));
        REQUIRE(intCount == 0);
        REQUIRE(stringCount == 0);
        REQUIRE(pairCount == 14);
    }
}

// Test ActionInvoker with validators
TEST_CASE("ActionInvoker - With validators", "[ActionSystem][ActionInvoker]")
{
    StringActionSystem system;
    std::string actionKey = "validated_action";

    system.AddValidator(actionKey,
        [](int value) -> bool
        {
            return value > 0;
        }, "Positive validator");

    int processedValue = 0;
    system.AddSequentialProcessor(actionKey,
        [&processedValue](int value)
        {
            processedValue = value;
        }, "Processor");

    SECTION("Invoker respects validation")
    {
        auto invoker = system.AcquireInvoker<int>(actionKey);
        REQUIRE(invoker.isValid() == true);

        // Valid value
        auto result1 = invoker.Execute(10);
        REQUIRE(result1.success == true);
        REQUIRE(result1.validationPassed == true);
        REQUIRE(processedValue == 10);

        // Invalid value
        auto result2 = invoker.Execute(-5);
        REQUIRE(result2.success == false);
        REQUIRE(result2.validationPassed == false);
    }
}

// Test ActionInvoker lifecycle
TEST_CASE("ActionInvoker - Lifecycle", "[ActionSystem][ActionInvoker]")
{
    StringActionSystem system;
    std::string actionKey = "lifecycle_test";

    SECTION("Invoker remains valid after processor added")
    {
        auto invoker = system.AcquireInvoker<int>(actionKey);
        REQUIRE(invoker.isValid() == false);

        // Add processor
        system.AddSequentialProcessor(actionKey, [](int) {}, "Processor");

        // Old invoker still invalid (need to re-acquire)
        REQUIRE(invoker.isValid() == false);

        // Acquire new invoker
        auto newInvoker = system.AcquireInvoker<int>(actionKey);
        REQUIRE(newInvoker.isValid() == true);
    }

    SECTION("Multiple invokers for same action")
    {
        system.AddSequentialProcessor(actionKey, [](int) {}, "Processor");

        auto invoker1 = system.AcquireInvoker<int>(actionKey);
        auto invoker2 = system.AcquireInvoker<int>(actionKey);

        REQUIRE(invoker1.isValid() == true);
        REQUIRE(invoker2.isValid() == true);

        // Both should work
        auto result1 = invoker1.Execute(1);
        auto result2 = invoker2.Execute(2);

        REQUIRE(result1.success == true);
        REQUIRE(result2.success == true);
    }
}

// Test ActionInvoker with all handler types
TEST_CASE("ActionInvoker - Complete handler pipeline", "[ActionSystem][ActionInvoker]")
{
    StringActionSystem system;
    std::string actionKey = "complete_pipeline";

    int triggerCount = 0;
    int validationCount = 0;
    int processorCount = 0;
    int completionCount = 0;

    system.AddTriggerListener(actionKey,
        [&triggerCount](int) { triggerCount++; }, "Trigger");

    system.AddValidator(actionKey,
        [](int x) -> bool { return x > 0; }, "Validator");

    system.AddValidationListener(actionKey,
        [&validationCount](int) { validationCount++; }, "Validation listener");

    system.AddSequentialProcessor(actionKey,
        [&processorCount](int) { processorCount++; }, "Processor");

    system.AddCompletionListener(actionKey,
        [&completionCount](int) { completionCount++; }, "Completion");

    SECTION("Invoker executes all handlers")
    {
        auto invoker = system.AcquireInvoker<int>(actionKey);
        REQUIRE(invoker.isValid() == true);

        auto result = invoker.Execute(10);

        REQUIRE(result.success == true);
        REQUIRE(result.validationPassed == true);
        REQUIRE(triggerCount == 1);
        REQUIRE(validationCount == 1);
        REQUIRE(processorCount == 1);
        REQUIRE(completionCount == 1);
    }

    SECTION("Invoker stops at failed validation")
    {
        auto invoker = system.AcquireInvoker<int>(actionKey);

        auto result = invoker.Execute(-5);

        REQUIRE(result.success == false);
        REQUIRE(result.validationPassed == false);
        REQUIRE(triggerCount == 1);
        REQUIRE(validationCount == 0);  // Validation listener not called
        REQUIRE(processorCount == 0);
        REQUIRE(completionCount == 0);
    }
}
