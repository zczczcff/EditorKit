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
        // 新行为：动作不存在时会创建动作
        auto invoker = system.AcquireInvoker<int>("non_existent");
        REQUIRE(invoker.isValid() == true);

        // 可以添加处理器并执行
        int executed = 0;
        system.AddSequentialProcessor("non_existent",
            [&executed](int value) { executed = value; }, "Processor");

        auto result = invoker.Execute(99);
        REQUIRE(result.success == true);
        REQUIRE(executed == 99);
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

    SECTION("Invoker created for non-existent action remains valid")
    {
        // 新行为：动作不存在时创建动作，调用器立即有效
        auto invoker = system.AcquireInvoker<int>(actionKey);
        REQUIRE(invoker.isValid() == true);

        // Add processor
        int executed = 0;
        system.AddSequentialProcessor(actionKey, [&executed](int x) { executed = x; }, "Processor");

        // 调用器仍然有效，可以直接执行
        auto result = invoker.Execute(42);
        REQUIRE(result.success == true);
        REQUIRE(executed == 42);
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

// Test ActionInvoker creates new actions when they don't exist
TEST_CASE("ActionInvoker - Create actions on demand", "[ActionSystem][ActionInvoker]")
{
    SECTION("Non-overload mode: Create new action when not exists")
    {
        StringActionSystem system;
        std::string actionKey = "new_action";

        // Action doesn't exist yet
        auto invoker = system.AcquireInvoker<int>(actionKey);
        REQUIRE(invoker.isValid() == true);

        // Add processor and execute through invoker
        int executed = 0;
        system.AddSequentialProcessor(actionKey,
            [&executed](int value) { executed = value; }, "Processor");

        auto result = invoker.Execute(42);
        REQUIRE(result.success == true);
        REQUIRE(executed == 42);
    }

    SECTION("Non-overload mode: Return invalid for type mismatch")
    {
        StringActionSystem system;
        std::string actionKey = "type_mismatch_action";

        // Create action with int parameter
        auto invoker1 = system.AcquireInvoker<int>(actionKey);
        REQUIRE(invoker1.isValid() == true);

        // Try to acquire with different parameter type
        auto invoker2 = system.AcquireInvoker<std::string>(actionKey);
        REQUIRE(invoker2.isValid() == false);

        // Original invoker still works
        int executed = 0;
        system.AddSequentialProcessor(actionKey,
            [&executed](int value) { executed = value; }, "Processor");

        auto result = invoker1.Execute(99);
        REQUIRE(result.success == true);
        REQUIRE(executed == 99);
    }

    SECTION("Overload mode: Create new overload for type mismatch")
    {
        StringActionSystemOverload system;
        std::string actionKey = "overload_action";

        int intExecuted = 0;
        int stringExecuted = 0;

        // Create action with int parameter
        auto invoker1 = system.AcquireInvoker<int>(actionKey);
        REQUIRE(invoker1.isValid() == true);

        // Create overload with string parameter
        auto invoker2 = system.AcquireInvoker<const std::string&>(actionKey);
        REQUIRE(invoker2.isValid() == true);

        // Both should work independently
        system.AddSequentialProcessor(actionKey,
            [&intExecuted](int value) { intExecuted = value; }, "Int processor");

        system.AddSequentialProcessor(actionKey,
            [&stringExecuted](const std::string& s) { stringExecuted = s.length(); }, "String processor");

        auto result1 = invoker1.Execute(42);
        REQUIRE(result1.success == true);
        REQUIRE(intExecuted == 42);
        REQUIRE(stringExecuted == 0);

        auto result2 = invoker2.Execute(std::string("hello"));
        REQUIRE(result2.success == true);
        REQUIRE(stringExecuted == 5);
        // Int processor not called for string overload
        REQUIRE(intExecuted == 42);
    }

    SECTION("Pre-create actions before adding handlers")
    {
        StringActionSystem system;

        // Pre-create invokers (actions don't have any handlers yet)
        auto invoker1 = system.AcquireInvoker<int>("action1");
        auto invoker2 = system.AcquireInvoker<double>("action2");
        auto invoker3 = system.AcquireInvoker<const std::string&>("action3");

        REQUIRE(invoker1.isValid() == true);
        REQUIRE(invoker2.isValid() == true);
        REQUIRE(invoker3.isValid() == true);

        // Now add handlers
        int count1 = 0, count2 = 0, count3 = 0;
        system.AddSequentialProcessor("action1", [&count1](int x) { count1 = x; }, "P1");
        system.AddSequentialProcessor("action2", [&count2](double x) { count2 = (int)x; }, "P2");
        system.AddSequentialProcessor("action3", [&count3](const std::string& s) { count3 = s.length(); }, "P3");

        // Execute through pre-acquired invokers (no hash lookup!)
        auto r1 = invoker1.Execute(10);
        auto r2 = invoker2.Execute(20.5);
        auto r3 = invoker3.Execute(std::string("hello"));

        REQUIRE(r1.success == true);
        REQUIRE(r2.success == true);
        REQUIRE(r3.success == true);
        REQUIRE(count1 == 10);
        REQUIRE(count2 == 20);
        REQUIRE(count3 == 5);
    }
}
