// Test for ActionInvoker - efficient execution without hash lookup
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>
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

// Performance test: Compare system.Execute vs invoker.Execute overhead
TEST_CASE("ActionInvoker - Performance benchmark", "[ActionSystem][ActionInvoker][Performance]")
{
    using Clock = std::chrono::high_resolution_clock;
    using Duration = std::chrono::duration<double, std::micro>;

    // Helper function to print performance results
    auto printResult = [](const std::string& name, double totalTimeUs, size_t iterations,
        size_t expectedCount, size_t actualCount)
    {
        double avgTimeNs = (totalTimeUs * 1000.0) / iterations;
        double throughput = (iterations / totalTimeUs) * 1000.0; // ops per ms

        std::cout << "\n  " << name << ":\n";
        std::cout << "    Total time: " << std::fixed << std::setprecision(2) << totalTimeUs << " us\n";
        std::cout << "    Average time: " << std::fixed << std::setprecision(2) << avgTimeNs << " ns/call\n";
        std::cout << "    Throughput: " << std::fixed << std::setprecision(0) << throughput << " calls/ms\n";
        std::cout << "    Verification: " << actualCount << "/" << expectedCount << " executions\n";
    };

    SECTION("Benchmark: Single parameter action")
    {
        const size_t iterations = 1000000;  // 1 million iterations
        StringActionSystem system;
        std::string actionKey = "perf_benchmark";

        // Setup processors
        size_t systemExecuteCount = 0;
        size_t invokerExecuteCount = 0;

        system.AddSequentialProcessor(actionKey,
            [&systemExecuteCount](int value) { systemExecuteCount += value; },
            "System processor");

        // Method 1: Real-time execution (system.Execute - with hash lookup)
        auto systemStart = Clock::now();
        for (size_t i = 0; i < iterations; ++i)
        {
            system.Execute(actionKey, 1);
        }
        auto systemEnd = Clock::now();
        Duration systemDuration = systemEnd - systemStart;

        // Method 2: Invoker cache (invoker.Execute - no hash lookup)
        auto invoker = system.AcquireInvoker<int>(actionKey);
        REQUIRE(invoker.isValid() == true);

        // Add different processor for method 2 to distinguish counts
        std::string actionKey2 = "perf_benchmark2";
        system.AddSequentialProcessor(actionKey2,
            [&invokerExecuteCount](int value) { invokerExecuteCount += value; },
            "Invoker processor");

        auto invoker2 = system.AcquireInvoker<int>(actionKey2);
        REQUIRE(invoker2.isValid() == true);

        auto invokerStart = Clock::now();
        for (size_t i = 0; i < iterations; ++i)
        {
            invoker2.Execute(1);
        }
        auto invokerEnd = Clock::now();
        Duration invokerDuration = invokerEnd - invokerStart;

        // Print results
        std::cout << "\n=== Single Parameter Action Performance Benchmark (" << iterations << " iterations) ===";
        printResult("system.Execute (real-time, with hash lookup)",
            systemDuration.count(), iterations, iterations, systemExecuteCount);
        printResult("invoker.Execute (cached, no hash lookup)",
            invokerDuration.count(), iterations, iterations, invokerExecuteCount);

        // Calculate performance improvement
        double speedup = systemDuration.count() / invokerDuration.count();
        double improvement = ((systemDuration.count() - invokerDuration.count()) / systemDuration.count()) * 100.0;

        std::cout << "\n  Performance improvement:\n";
        std::cout << "    Invoker cache is " << std::fixed << std::setprecision(2)
            << speedup << "x faster\n";
        std::cout << "    Time saved: " << std::fixed << std::setprecision(1)
            << improvement << "%\n";

        // Verify execution counts
        REQUIRE(systemExecuteCount == iterations);
        REQUIRE(invokerExecuteCount == iterations);
    }

    SECTION("Benchmark: Multi-parameter overload action")
    {
        const size_t iterations = 500000;  // 500k iterations
        StringActionSystemOverload system;
        std::string actionKey = "overload_perf";

        // Setup multiple overload processors
        size_t intCount = 0;
        size_t stringCount = 0;
        size_t multiCount = 0;

        system.AddSequentialProcessor(actionKey,
            [&intCount](int x) { intCount += x; }, "Int processor");

        system.AddSequentialProcessor(actionKey,
            [&stringCount](const std::string& s) { stringCount += s.length(); }, "String processor");

        system.AddSequentialProcessor(actionKey,
            [&multiCount](int a, const std::string& b) { multiCount += a + b.length(); }, "Multi processor");

        // Method 1: Real-time execution
        auto systemStart = Clock::now();
        for (size_t i = 0; i < iterations; ++i)
        {
            system.Execute(actionKey, 1);
            system.Execute(actionKey, std::string("x"));
            system.Execute(actionKey, 1, std::string("x"));
        }
        auto systemEnd = Clock::now();
        Duration systemDuration = systemEnd - systemStart;

        // Method 2: Invoker cache
        auto intInvoker = system.AcquireInvoker<int>(actionKey);
        auto stringInvoker = system.AcquireInvoker<const std::string&>(actionKey);
        auto multiInvoker = system.AcquireInvoker<int, const std::string&>(actionKey);

        REQUIRE(intInvoker.isValid() == true);
        REQUIRE(stringInvoker.isValid() == true);
        REQUIRE(multiInvoker.isValid() == true);

        auto invokerStart = Clock::now();
        for (size_t i = 0; i < iterations; ++i)
        {
            intInvoker.Execute(1);
            stringInvoker.Execute(std::string("x"));
            multiInvoker.Execute(1, std::string("x"));
        }
        auto invokerEnd = Clock::now();
        Duration invokerDuration = invokerEnd - invokerStart;

        // Print results
        std::cout << "\n=== Multi-Parameter Overload Action Benchmark (3 overloads x " << iterations << " iterations) ===";
        printResult("system.Execute (real-time)",
            systemDuration.count(), iterations * 3, iterations * 3, intCount + stringCount + multiCount);
        printResult("invoker.Execute (cached)",
            invokerDuration.count(), iterations * 3, iterations * 3, intCount + stringCount + multiCount);

        double speedup = systemDuration.count() / invokerDuration.count();
        double improvement = ((systemDuration.count() - invokerDuration.count()) / systemDuration.count()) * 100.0;

        std::cout << "\n  Performance improvement:\n";
        std::cout << "    Invoker cache is " << std::fixed << std::setprecision(2)
            << speedup << "x faster\n";
        std::cout << "    Time saved: " << std::fixed << std::setprecision(1)
            << improvement << "%\n";
    }

    SECTION("Benchmark: Complex action (with validators and listeners)")
    {
        const size_t iterations = 200000;  // 200k iterations
        StringActionSystem system;

        // Setup for system.Execute test
        std::string actionKey1 = "complex_perf1";
        size_t triggerCount1 = 0;
        size_t processorCount1 = 0;
        size_t completionCount1 = 0;

        system.AddTriggerListener(actionKey1,
            [&triggerCount1](int value) { triggerCount1 += value; },
            "Trigger listener");

        system.AddValidator(actionKey1,
            [](int value) -> bool { return value > 0; },
            "Positive validator");

        system.AddSequentialProcessor(actionKey1,
            [&processorCount1](int value) { processorCount1 += value; },
            "Processor");

        system.AddCompletionListener(actionKey1,
            [&completionCount1](int value) { completionCount1 += value; },
            "Completion listener");

        // Method 1: Real-time execution
        auto systemStart = Clock::now();
        for (size_t i = 0; i < iterations; ++i)
        {
            system.Execute(actionKey1, 1);
        }
        auto systemEnd = Clock::now();
        Duration systemDuration = systemEnd - systemStart;

        // Setup for invoker.Execute test
        std::string actionKey2 = "complex_perf2";
        size_t triggerCount2 = 0;
        size_t processorCount2 = 0;
        size_t completionCount2 = 0;

        system.AddTriggerListener(actionKey2,
            [&triggerCount2](int value) { triggerCount2 += value; },
            "Trigger listener");

        system.AddValidator(actionKey2,
            [](int value) -> bool { return value > 0; },
            "Positive validator");

        system.AddSequentialProcessor(actionKey2,
            [&processorCount2](int value) { processorCount2 += value; },
            "Processor");

        system.AddCompletionListener(actionKey2,
            [&completionCount2](int value) { completionCount2 += value; },
            "Completion listener");

        // Method 2: Invoker cache
        auto invoker = system.AcquireInvoker<int>(actionKey2);
        auto invokerStart = Clock::now();
        for (size_t i = 0; i < iterations; ++i)
        {
            invoker.Execute(1);
        }
        auto invokerEnd = Clock::now();
        Duration invokerDuration = invokerEnd - invokerStart;

        // Print results
        std::cout << "\n=== Complex Action Benchmark (with validators+listeners, " << iterations << " iterations) ===";
        printResult("system.Execute (real-time)",
            systemDuration.count(), iterations, iterations, processorCount1);
        printResult("invoker.Execute (cached)",
            invokerDuration.count(), iterations, iterations, processorCount2);

        double speedup = systemDuration.count() / invokerDuration.count();
        double improvement = ((systemDuration.count() - invokerDuration.count()) / systemDuration.count()) * 100.0;

        std::cout << "\n  Performance improvement:\n";
        std::cout << "    Invoker cache is " << std::fixed << std::setprecision(2)
            << speedup << "x faster\n";
        std::cout << "    Time saved: " << std::fixed << std::setprecision(1)
            << improvement << "%\n";

        // Verify all handlers were called
        REQUIRE(triggerCount1 == iterations);
        REQUIRE(processorCount1 == iterations);
        REQUIRE(completionCount1 == iterations);
        REQUIRE(triggerCount2 == iterations);
        REQUIRE(processorCount2 == iterations);
        REQUIRE(completionCount2 == iterations);
    }

    SECTION("Micro-benchmark: Pure call overhead")
    {
        const size_t iterations = 10000000;  // 10 million iterations
        StringActionSystem system;
        std::string actionKey = "micro_perf";

        volatile int sink = 0;  // Prevent compiler from optimizing away loop
        system.AddSequentialProcessor(actionKey,
            [&sink](int value) { sink += value; }, "Sink processor");

        auto invoker = system.AcquireInvoker<int>(actionKey);
        REQUIRE(invoker.isValid() == true);

        // Warmup
        for (int i = 0; i < 1000; ++i)
        {
            system.Execute(actionKey, 1);
            invoker.Execute(1);
        }

        // Method 1: Real-time execution
        sink = 0;
        auto systemStart = Clock::now();
        for (size_t i = 0; i < iterations; ++i)
        {
            system.Execute(actionKey, 1);
        }
        auto systemEnd = Clock::now();
        Duration systemDuration = systemEnd - systemStart;

        // Method 2: Invoker cache
        sink = 0;
        auto invokerStart = Clock::now();
        for (size_t i = 0; i < iterations; ++i)
        {
            invoker.Execute(1);
        }
        auto invokerEnd = Clock::now();
        Duration invokerDuration = invokerEnd - invokerStart;

        // Print results
        std::cout << "\n=== Micro-benchmark (pure call overhead, " << iterations << " iterations) ===";
        printResult("system.Execute (real-time)",
            systemDuration.count(), iterations, iterations, 0);
        printResult("invoker.Execute (cached)",
            invokerDuration.count(), iterations, iterations, 0);

        double speedup = systemDuration.count() / invokerDuration.count();
        double overheadNs = (systemDuration.count() - invokerDuration.count()) / iterations * 1000.0;

        std::cout << "\n  Performance analysis:\n";
        std::cout << "    Invoker cache is " << std::fixed << std::setprecision(2)
            << speedup << "x faster\n";
        std::cout << "    Hash lookup overhead: ~" << std::fixed << std::setprecision(2)
            << overheadNs << " ns/call\n";

        REQUIRE(sink > 0);  // Ensure loop was executed
    }
}
