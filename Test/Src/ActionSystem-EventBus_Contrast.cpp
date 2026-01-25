#include <iostream>
#include <string>
#include <vector>
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>
#include <EditorKit/ActionSystem.h>
#include <EditorKit/KEventBus.h>

#include <iostream>
#include <chrono>
#include <string>

using namespace std;
using namespace std::chrono;

// 性能测试类
class PerformanceTester
{
private:
    // 测试计数器，用于验证回调确实被执行
    static atomic<int> callbackCounter;

    // 简单的回调函数
    static void simpleCallback(int value)
    {
        callbackCounter.fetch_add(1, memory_order_relaxed);
    }

    static bool validatorCallback(int value)
    {
        callbackCounter.fetch_add(1, memory_order_relaxed);
        return value > 0;  // 简单的验证逻辑
    }

public:
    // 测试ActionSystem性能
    static void testActionSystem(int subscriberCount, int publishCount)
    {
        cout << "=== ActionSystem性能测试 ===" << endl;
        cout << "订阅者数量: " << subscriberCount << ", 发布次数: " << publishCount << endl;

        StringActionSystem actionSystem;
        callbackCounter = 0;

        // 注册订阅者
        string actionKey = "test_action";
        vector<ActionHandle<string>> handles;

        auto startSetup = high_resolution_clock::now();

        // 添加验证器
        for (int i = 0; i < subscriberCount; i++)
        {
            auto handle = actionSystem.AddValidator(actionKey,
                [](int value) -> bool
                {
                    callbackCounter.fetch_add(1, memory_order_relaxed);
                    return value > 0;
                },
                "Validator " + to_string(i));
            handles.push_back(handle);
        }

        // 添加顺序处理器
        for (int i = 0; i < subscriberCount; i++)
        {
            auto handle = actionSystem.AddSequentialProcessor(actionKey,
                [](int value)
                {
                    callbackCounter.fetch_add(1, memory_order_relaxed);
                },
                "Processor " + to_string(i));
            handles.push_back(handle);
        }

        auto endSetup = high_resolution_clock::now();
        auto setupTime = duration_cast<microseconds>(endSetup - startSetup);

        // 预热（避免冷启动影响）
        for (int i = 0; i < 100; i++)
        {
            actionSystem.Execute(actionKey, 1);
        }
        callbackCounter = 0;

        // 性能测试
        auto startTime = high_resolution_clock::now();

        for (int i = 0; i < publishCount; i++)
        {
            auto result = actionSystem.Execute(actionKey, i + 1);
            if (!result.success)
            {
                cerr << "ActionSystem执行失败: " << result.errorMessage << endl;
            }
        }

        auto endTime = high_resolution_clock::now();
        auto totalTime = duration_cast<microseconds>(endTime - startTime);

        // 输出结果
        cout << "总耗时: " << totalTime.count() << " 微秒" << endl;
        cout << "单次发布平均耗时: " << static_cast<double>(totalTime.count()) / publishCount << " 微秒" << endl;
        cout << "回调执行次数: " << callbackCounter << endl;
        cout << "设置时间: " << setupTime.count() << " 微秒" << endl;
        cout << endl;
    }

    // 测试EventBus性能
    static void testEventBus(int subscriberCount, int publishCount)
    {
        cout << "=== EventBus性能测试 ===" << endl;
        cout << "订阅者数量: " << subscriberCount << ", 发布次数: " << publishCount << endl;

        EventBus<string> eventBus;
        callbackCounter = 0;

        auto startSetup = high_resolution_clock::now();

        // 注册订阅者
        string eventName = "test_event";
        vector<EventID> tokens;

        for (int i = 0; i < subscriberCount; i++)
        {
            auto token = eventBus.Subscribe(eventName,
                [](int value)
                {
                    callbackCounter.fetch_add(1, memory_order_relaxed);
                },
                "Subscriber " + to_string(i));
            tokens.push_back(token);
        }

        auto endSetup = high_resolution_clock::now();
        auto setupTime = duration_cast<microseconds>(endSetup - startSetup);

        // 预热
        for (int i = 0; i < 100; i++)
        {
            eventBus.Publish(eventName, 1);
        }
        callbackCounter = 0;

        // 性能测试
        auto startTime = high_resolution_clock::now();

        for (int i = 0; i < publishCount; i++)
        {
            auto result = eventBus.Publish(eventName, i + 1);
            if (!result.success && subscriberCount > 0)
            {
                cerr << "EventBus发布失败: " << result.errorMessage << endl;
            }
        }

        auto endTime = high_resolution_clock::now();
        auto totalTime = duration_cast<microseconds>(endTime - startTime);

        // 输出结果
        cout << "总耗时: " << totalTime.count() << " 微秒" << endl;
        cout << "单次发布平均耗时: " << static_cast<double>(totalTime.count()) / publishCount << " 微秒" << endl;
        cout << "回调执行次数: " << callbackCounter << endl;
        cout << "设置时间: " << setupTime.count() << " 微秒" << endl;
        cout << endl;
    }

    // 综合对比测试
    static void runComparativeTest()
    {
        cout << "开始性能对比测试..." << endl << endl;

        vector<int> subscriberCounts = { 1, 5, 20 };
        int publishCount = 100000;  // 10万次发布

        for (int count : subscriberCounts)
        {
            cout << "┌────────────────────────────────────────┐" << endl;
            cout << "│         订阅者数量: " << count << "                   │" << endl;
            cout << "└────────────────────────────────────────┘" << endl;

            testActionSystem(count, publishCount);
            testEventBus(count, publishCount);

            // 添加分隔线
            cout << "==========================================" << endl;
            cout << endl;
        }
    }

    // 内存使用测试（可选）
    static void testMemoryUsage()
    {
        cout << "=== 内存使用测试 ===" << endl;

        // 测试ActionSystem内存
        {
            StringActionSystem system;
            for (int i = 0; i < 1000; i++)
            {
                system.AddSequentialProcessor("action_" + to_string(i),
                    [](int value) {});
            }
            cout << "ActionSystem创建1000个动作后的内存使用情况需要在实际运行中观察" << endl;
        }

        // 测试EventBus内存
        {
            EventBus<string> bus;
            for (int i = 0; i < 1000; i++)
            {
                bus.Subscribe("event_" + to_string(i),
                    [](int value) {});
            }
            cout << "EventBus创建1000个订阅后的内存使用情况需要在实际运行中观察" << endl;
        }
        cout << endl;
    }
};

// 静态成员初始化
atomic<int> PerformanceTester::callbackCounter(0);


// 测试用例
TEST_CASE("ActionSystem-KEventBus对比测试", "[ActionSystem-KEventBus]")
{


    SECTION("ActionSystem-KEventBus对比测试")
    {
        PerformanceTester::runComparativeTest();
    }

}