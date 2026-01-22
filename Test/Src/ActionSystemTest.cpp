#include <iostream>
#include <string>
#include <vector>
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>
#include <EditorKit/ActionSystem.h>

// 测试用例
TEST_CASE("基本功能测试", "[ActionSystem][Basic]")
{
    StringActionSystem system;
    std::string actionKey = "test_action";
    bool processorExecuted = false;

    // 添加简单的处理器
    auto handle = system.AddSequentialProcessor(actionKey,
        [&processorExecuted](int value)
        {
            processorExecuted = true;
            std::cout << "处理器执行，值: " << value << std::endl;
        }, "基本处理器");

    SECTION("基本执行成功")
    {
        auto result = system.Execute(actionKey, 42);
        
        REQUIRE(result.success == true);
        REQUIRE(result.executedProcessors == 1);
        REQUIRE(processorExecuted == true);
    }

    SECTION("未注册的动作应该失败")
    {
        auto result = system.Execute("unknown_action", 42);
        REQUIRE(result.success == false);
    }
}

TEST_CASE("验证器功能测试", "[ActionSystem][Validator]")
{
    StringActionSystem system;
    std::string actionKey = "validator_test";

    // 添加验证器 - 只允许偶数
    system.AddValidator(actionKey,
        [](int value) -> bool
        {
            return value % 2 == 0;
        }, "偶数验证器");

    // 添加处理器
    system.AddSequentialProcessor(actionKey,
        [](int value)
        {
            std::cout << "验证通过，处理偶数: " << value << std::endl;
        }, "偶数处理器");

    SECTION("偶数验证应该通过")
    {
        auto result = system.Execute(actionKey, 4);
        
        REQUIRE(result.success == true);
        REQUIRE(result.validationPassed == true);
    }

    SECTION("奇数验证应该失败")
    {
        auto result = system.Execute(actionKey, 3);
        
        REQUIRE(result.success == false);
        REQUIRE(result.validationPassed == false);
    }
}

TEST_CASE("处理器类型测试", "[ActionSystem][ProcessorTypes]")
{
    StringActionSystem system;
    std::string actionKey = "processor_types";
    int triggerCount = 0, validationCount = 0, completionCount = 0;

    // 添加各种类型的处理器
    system.AddTriggerListener(actionKey,
        [&triggerCount](std::string& msg)
        {
            triggerCount++;
            std::cout << "触发监听器: msg.length()=" << msg.length()
                << ", content: '" << msg << "'" << std::endl;
            msg = "test";
        }, "触发监听器");

    system.AddValidationListener(actionKey,
        [&validationCount](std::string& msg)
        {
            validationCount++;
            std::cout << "验证监听器: msg.length()=" << msg.length()
                << ", content: '" << msg << "'" << std::endl;
        }, "验证监听器");

    system.AddSequentialProcessor(actionKey,
        [](std::string& msg)
        {
            std::cout << "顺序处理器: " << msg << std::endl;
        }, "顺序处理器");

    system.AddCompletionListener(actionKey,
        [&completionCount](std::string& msg)
        {
            completionCount++;
            std::cout << "完成监听器: " << msg << std::endl;
        }, "完成监听器");

    SECTION("所有监听器正确执行")
    {
        std::string s = std::string("测试消息");
        auto result = system.Execute(actionKey, s);

        REQUIRE(triggerCount == 1);
        REQUIRE(validationCount == 1);
        REQUIRE(completionCount == 1);
        REQUIRE(result.executedListeners == 3);
    }
}

TEST_CASE("错误处理测试", "[ActionSystem][ErrorHandling]")
{
    StringActionSystem system;
    std::string actionKey = "error_handling";

    // 添加会抛出异常的验证器
    system.AddValidator(actionKey,
        [](int value) -> bool
        {
            if (value < 0)
            {
                throw std::runtime_error("数值不能为负数");
            }
            return value > 0;
        }, "正数验证器");

    // 添加处理器
    system.AddSequentialProcessor(actionKey,
        [](int value)
        {
            std::cout << "处理正数: " << value << std::endl;
        }, "正数处理器");

    SECTION("异常应该被正确捕获")
    {
        auto result = system.Execute(actionKey, -5);

        REQUIRE(result.success == false);
        REQUIRE_FALSE(result.errorMessage.empty());
    }

    SECTION("正常值应该成功执行")
    {
        auto result = system.Execute(actionKey, 10);
        REQUIRE(result.success == true);
    }
}

TEST_CASE("移除处理器测试", "[ActionSystem][RemoveHandler]")
{
    StringActionSystem system;
    std::string actionKey = "remove_test";
    int processor1Count = 0, processor2Count = 0;

    // 添加处理器
    auto handle1 = system.AddSequentialProcessor(actionKey,
        [&processor1Count](int value)
        {
            processor1Count++;
            std::cout << "处理器1: " << value << std::endl;
        }, "第一个处理器");

    auto handle2 = system.AddSequentialProcessor(actionKey,
        [&processor2Count](int value)
        {
            processor2Count++;
            std::cout << "处理器2: " << value << std::endl;
        }, "第二个处理器");

    SECTION("移除前两个处理器都执行")
    {
        auto result = system.Execute(actionKey, 10);
        REQUIRE(result.executedProcessors == 2);
        REQUIRE(processor1Count == 1);
        REQUIRE(processor2Count == 1);
    }

    SECTION("移除后只剩一个处理器执行")
    {
        bool removed = system.RemoveHandler(handle1);
        REQUIRE(removed == true);

        auto result = system.Execute(actionKey, 20);
        REQUIRE(result.executedProcessors == 1);
        REQUIRE(processor2Count == 1);
    }
}

TEST_CASE("优先级系统测试", "[ActionSystem][Priority]")
{
    StringActionSystem system;
    std::string actionKey = "priority_test";
    std::vector<int> executionOrder;

    // 添加不同优先级的处理器（优先级数字越小，优先级越高）
    system.AddSequentialProcessor(actionKey,
        [&executionOrder](int value)
        {
            executionOrder.push_back(1);
            std::cout << "高优先级处理器" << std::endl;
        }, "高优先级", 1);

    system.AddSequentialProcessor(actionKey,
        [&executionOrder](int value)
        {
            executionOrder.push_back(3);
            std::cout << "低优先级处理器" << std::endl;
        }, "低优先级", 3);

    system.AddSequentialProcessor(actionKey,
        [&executionOrder](int value)
        {
            executionOrder.push_back(2);
            std::cout << "中优先级处理器" << std::endl;
        }, "中优先级", 2);

    SECTION("优先级排序正确")
    {
        system.Execute(actionKey, 0);

        REQUIRE(executionOrder.size() == 3);
        REQUIRE(executionOrder[0] == 1);
        REQUIRE(executionOrder[1] == 2);
        REQUIRE(executionOrder[2] == 3);
    }
}

TEST_CASE("统计信息测试", "[ActionSystem][Statistics]")
{
    StringActionSystem system;
    std::string actionKey = "stats_test";

    // 添加各种处理器
    system.AddValidator(actionKey, [](int x) { return x > 0; }, "验证器");
    system.AddSequentialProcessor(actionKey, [](int x) {}, "顺序处理器1");
    system.AddSequentialProcessor(actionKey, [](int x) {}, "顺序处理器2");
    system.SetFinalProcessor(actionKey, [](int x) {}, "最终处理器");
    system.AddTriggerListener(actionKey, [](int x) {}, "触发监听器");

    SECTION("统计信息生成成功")
    {
        std::string stats = system.GetStatistics();
        REQUIRE_FALSE(stats.empty());
        
        // 检查是否包含关键信息
        REQUIRE(stats.find("统计信息") != std::string::npos);
    }
}

TEST_CASE("复杂场景测试", "[ActionSystem][Complex]")
{
    StringActionSystem system;
    std::string actionKey = "complex_scenario";
    std::vector<std::string> executionLog;

    // 复杂的业务逻辑场景
    system.AddTriggerListener(actionKey,
        [&executionLog](const std::string& user, int amount)
        {
            executionLog.push_back("触发:用户" + user + "尝试转账" + std::to_string(amount));
        }, "交易触发监听器");

    system.AddValidator(actionKey,
        [](const std::string& user, int amount) -> bool
        {
            return !user.empty() && amount > 0;
        }, "基础验证");

    system.AddValidator(actionKey,
        [](const std::string& user, int amount) -> bool
        {
            return amount <= 10000; // 单笔限额1万
        }, "限额验证");

    system.AddSequentialProcessor(actionKey,
        [](const std::string& user, int amount)
        {
            // 模拟扣款
        }, "扣款处理", 1);

    system.AddCompletionListener(actionKey,
        [&executionLog](const std::string& user, int amount)
        {
            executionLog.push_back("交易完成:用户" + user + "成功转账" + std::to_string(amount));
        }, "完成通知");

    SECTION("正常交易应该成功")
    {
        auto result = system.Execute(actionKey, std::string("Alice"), 5000);
        REQUIRE(result.success == true);
    }

    SECTION("超额交易应该被拒绝")
    {
        auto result = system.Execute(actionKey, std::string("Bob"), 15000);
        REQUIRE(result.success == false);
        REQUIRE(result.validationPassed == false);
    }
}
