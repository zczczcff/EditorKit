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

// 测试允许重载模式的基本功能
TEST_CASE("允许重载模式 - 基本功能测试", "[ActionSystem][Overload][Basic]")
{
    StringActionSystemOverload system;  // 使用允许重载的版本
    std::string actionKey = "overloaded_action";

    int intProcessorCount = 0;
    int stringProcessorCount = 0;
    int floatProcessorCount = 0;

    // 为同一个actionKey添加不同参数类型的处理器（重载）
    auto handle1 = system.AddSequentialProcessor(actionKey,
        [&intProcessorCount](int value)
        {
            intProcessorCount++;
            std::cout << "整数处理器: " << value << std::endl;
        }, "整数处理器");

    auto handle2 = system.AddSequentialProcessor(actionKey,
        [&stringProcessorCount](const std::string& value)
        {
            stringProcessorCount++;
            std::cout << "字符串处理器: " << value << std::endl;
        }, "字符串处理器");

    auto handle3 = system.AddSequentialProcessor(actionKey,
        [&floatProcessorCount](float value)
        {
            floatProcessorCount++;
            std::cout << "浮点数处理器: " << value << std::endl;
        }, "浮点数处理器");

    SECTION("整数参数应调用整数处理器")
    {
        auto result = system.Execute(actionKey, 42);
        REQUIRE(result.success == true);
        REQUIRE(intProcessorCount == 1);
        REQUIRE(stringProcessorCount == 0);
        REQUIRE(floatProcessorCount == 0);
    }

    SECTION("字符串参数应调用字符串处理器")
    {
        auto result = system.Execute(actionKey, std::string("hello"));
        REQUIRE(result.success == true);
        REQUIRE(intProcessorCount == 0);
        REQUIRE(stringProcessorCount == 1);
        REQUIRE(floatProcessorCount == 0);
    }

    SECTION("浮点数参数应调用浮点数处理器")
    {
        auto result = system.Execute(actionKey, 3.14f);
        REQUIRE(result.success == true);
        REQUIRE(intProcessorCount == 0);
        REQUIRE(stringProcessorCount == 0);
        REQUIRE(floatProcessorCount == 1);
    }

    SECTION("不匹配的参数类型应返回错误")
    {
        // 尝试执行一个没有注册的参数类型
        auto result = system.Execute(actionKey, std::vector<int>{1, 2, 3});
        REQUIRE(result.success == false);
        REQUIRE_FALSE(result.errorMessage.empty());
    }
}

// 测试不允许重载模式的类型冲突
TEST_CASE("不允许重载模式 - 类型冲突测试", "[ActionSystem][NonOverload][TypeConflict]")
{
    StringActionSystem system;  // 默认不允许重载

    SECTION("为同一个actionKey添加不同类型处理器应失败")
    {
        std::string actionKey = "conflict_action";

        // 先添加一个整数处理器
        system.AddSequentialProcessor(actionKey,
            [](int value)
            {
                std::cout << "整数处理器" << std::endl;
            }, "第一个处理器");

        // 尝试添加字符串处理器 - 应该抛出异常
        REQUIRE_THROWS_AS(
            system.AddSequentialProcessor(actionKey,
                [](const std::string& value)
                {
                    std::cout << "字符串处理器" << std::endl;
                }, "第二个处理器"),
            std::runtime_error
                    );
    }

    SECTION("已有处理器的情况下，正确类型的处理器应成功添加")
    {
        std::string actionKey = "same_type_action";

        // 添加第一个整数处理器
        system.AddSequentialProcessor(actionKey,
            [](int value)
            {
                std::cout << "处理器1" << std::endl;
            }, "处理器1");

        // 添加第二个整数处理器（相同类型，应该成功）
        REQUIRE_NOTHROW(
            system.AddSequentialProcessor(actionKey,
                [](int value)
                {
                    std::cout << "处理器2" << std::endl;
                }, "处理器2")
        );
    }
}

// 测试重载模式下的各种处理器类型
TEST_CASE("允许重载模式 - 多种处理器类型测试", "[ActionSystem][Overload][ProcessorTypes]")
{
    StringActionSystemOverload system;
    std::string actionKey = "multi_type_action";

    int intValidatorCount = 0;
    int stringValidatorCount = 0;
    int completionListenerCount = 0;

    // 为整数类型添加验证器
    system.AddValidator(actionKey,
        [&intValidatorCount](int value) -> bool
        {
            intValidatorCount++;
            return value > 0;
        }, "整数验证器");

    // 为字符串类型添加验证器
    system.AddValidator(actionKey,
        [&stringValidatorCount](const std::string& value) -> bool
        {
            stringValidatorCount++;
            return !value.empty();
        }, "字符串验证器");

    // 为整数类型添加顺序处理器
    system.AddSequentialProcessor(actionKey,
        [](int value)
        {
            std::cout << "处理整数: " << value << std::endl;
        }, "整数处理器");

    // 为字符串类型添加顺序处理器
    system.AddSequentialProcessor(actionKey,
        [](const std::string& value)
        {
            std::cout << "处理字符串: " << value << std::endl;
        }, "字符串处理器");

    // 为整数类型添加完成监听器
    system.AddCompletionListener(actionKey,
        [&completionListenerCount](int value)
        {
            completionListenerCount++;
            std::cout << "整数完成监听: " << value << std::endl;
        }, "整数完成监听器");

    SECTION("执行整数流程")
    {
        auto result = system.Execute(actionKey, 42);
        REQUIRE(result.success == true);
        REQUIRE(intValidatorCount == 1);
        REQUIRE(stringValidatorCount == 0);
        REQUIRE(completionListenerCount == 1);
    }

    SECTION("执行字符串流程")
    {
        auto result = system.Execute(actionKey, std::string("test"));
        REQUIRE(result.success == true);
        REQUIRE(intValidatorCount == 0);
        REQUIRE(stringValidatorCount == 1);
        REQUIRE(completionListenerCount == 0);  // 字符串没有完成监听器
    }
}

// 测试重载模式下的优先级系统
TEST_CASE("允许重载模式 - 优先级测试", "[ActionSystem][Overload][Priority]")
{
    StringActionSystemOverload system;
    std::string actionKey = "priority_overload";

    std::vector<std::string> intExecutionOrder;
    std::vector<std::string> stringExecutionOrder;

    // 为整数类型添加不同优先级的处理器
    system.AddSequentialProcessor(actionKey,
        [&intExecutionOrder](int value)
        {
            intExecutionOrder.push_back("int-low");
        }, "整数低优先级", 10);

    system.AddSequentialProcessor(actionKey,
        [&intExecutionOrder](int value)
        {
            intExecutionOrder.push_back("int-high");
        }, "整数高优先级", 1);

    // 为字符串类型添加不同优先级的处理器
    system.AddSequentialProcessor(actionKey,
        [&stringExecutionOrder](const std::string& value)
        {
            stringExecutionOrder.push_back("str-low");
        }, "字符串低优先级", 10);

    system.AddSequentialProcessor(actionKey,
        [&stringExecutionOrder](const std::string& value)
        {
            stringExecutionOrder.push_back("str-high");
        }, "字符串高优先级", 1);

    SECTION("整数处理器按优先级排序")
    {
        system.Execute(actionKey, 42);
        REQUIRE(intExecutionOrder.size() == 2);
        REQUIRE(intExecutionOrder[0] == "int-high");  // 优先级1
        REQUIRE(intExecutionOrder[1] == "int-low");   // 优先级10
    }

    SECTION("字符串处理器按优先级排序")
    {
        system.Execute(actionKey, std::string("test"));
        REQUIRE(stringExecutionOrder.size() == 2);
        REQUIRE(stringExecutionOrder[0] == "str-high");  // 优先级1
        REQUIRE(stringExecutionOrder[1] == "str-low");   // 优先级10
    }
}

// 测试重载模式下的复杂类型
TEST_CASE("允许重载模式 - 复杂类型测试", "[ActionSystem][Overload][ComplexTypes]")
{
    StringActionSystemOverload system;
    std::string actionKey = "complex_overload";

    struct Point { int x; int y; };
    struct Rect { Point pos; int width; int height; };

    int pointProcessorCount = 0;
    int rectProcessorCount = 0;
    int mixedProcessorCount = 0;

    // 添加结构体类型的处理器
    system.AddSequentialProcessor(actionKey,
        [&pointProcessorCount](const Point& p)
        {
            pointProcessorCount++;
            std::cout << "点: (" << p.x << ", " << p.y << ")" << std::endl;
        }, "点处理器");

    system.AddSequentialProcessor(actionKey,
        [&rectProcessorCount](const Rect& r)
        {
            rectProcessorCount++;
            std::cout << "矩形: pos(" << r.pos.x << "," << r.pos.y
                << "), size(" << r.width << "x" << r.height << ")" << std::endl;
        }, "矩形处理器");

    // 添加混合参数类型的处理器
    system.AddSequentialProcessor(actionKey,
        [&mixedProcessorCount](int a, const std::string& b, float c)
        {
            mixedProcessorCount++;
            std::cout << "混合参数: " << a << ", " << b << ", " << c << std::endl;
        }, "混合参数处理器");

    SECTION("执行结构体参数")
    {
        Point p{ 10, 20 };
        auto result = system.Execute(actionKey, p);
        REQUIRE(result.success == true);
        REQUIRE(pointProcessorCount == 1);
        REQUIRE(rectProcessorCount == 0);
    }

    SECTION("执行复合结构体参数")
    {
        Rect r{ {0, 0}, 100, 200 };
        auto result = system.Execute(actionKey, r);
        REQUIRE(result.success == true);
        REQUIRE(pointProcessorCount == 0);
        REQUIRE(rectProcessorCount == 1);
    }

    SECTION("执行混合参数")
    {
        auto result = system.Execute(actionKey, 1, std::string("test"), 3.14f);
        REQUIRE(result.success == true);
        REQUIRE(mixedProcessorCount == 1);
    }
}

// 测试重载模式下的处理器移除
TEST_CASE("允许重载模式 - 处理器移除测试", "[ActionSystem][Overload][Removal]")
{
    StringActionSystemOverload system;
    std::string actionKey = "removal_overload";

    int intProcessorCount = 0;
    int stringProcessorCount = 0;

    // 添加不同参数类型的处理器
    auto intHandle = system.AddSequentialProcessor(actionKey,
        [&intProcessorCount](int value)
        {
            intProcessorCount++;
        }, "整数处理器");

    auto stringHandle = system.AddSequentialProcessor(actionKey,
        [&stringProcessorCount](const std::string& value)
        {
            stringProcessorCount++;
        }, "字符串处理器");

    SECTION("移除整数处理器")
    {
        REQUIRE(system.RemoveHandler(intHandle) == true);

        // 执行整数参数 - 应该失败，因为处理器已被移除
        auto result = system.Execute(actionKey, 42);
        REQUIRE(result.success == false);
        REQUIRE(intProcessorCount == 0);

        // 执行字符串参数 - 应该成功
        result = system.Execute(actionKey, std::string("test"));
        REQUIRE(result.success == true);
        REQUIRE(stringProcessorCount == 1);
    }

    SECTION("移除字符串处理器")
    {
        REQUIRE(system.RemoveHandler(stringHandle) == true);

        // 执行字符串参数 - 应该失败
        auto result = system.Execute(actionKey, std::string("test"));
        REQUIRE(result.success == false);
        REQUIRE(stringProcessorCount == 0);

        // 执行整数参数 - 应该成功
        result = system.Execute(actionKey, 42);
        REQUIRE(result.success == true);
        REQUIRE(intProcessorCount == 1);
    }

    SECTION("检查处理器变体数量")
    {
        // 初始应有2个变体（整数和字符串）
        REQUIRE(system.GetActionVariantCount(actionKey) == 2);

        // 移除整数处理器，整数变体应该被清理（如果没有处理器了）
        system.RemoveHandler(intHandle);

        // 现在应该只有1个变体（字符串）
        REQUIRE(system.GetActionVariantCount(actionKey) == 1);
    }
}

// 测试重载模式下的统计信息
TEST_CASE("允许重载模式 - 统计信息测试", "[ActionSystem][Overload][Statistics]")
{
    StringActionSystemOverload system;

    // 创建多个重载的action
    system.AddSequentialProcessor("action1", [](int x) {}, "int处理器");
    system.AddSequentialProcessor("action1", [](float x) {}, "float处理器");

    system.AddSequentialProcessor("action2", [](const std::string& s) {}, "string处理器");
    system.AddSequentialProcessor("action2", [](int a, int b) {}, "双int处理器");

    SECTION("检查统计信息")
    {
        std::string stats = system.GetStatistics();
        REQUIRE_FALSE(stats.empty());

        // 检查是否包含重载相关的信息
        REQUIRE(stats.find("Mode: Allow Overload") != std::string::npos);
        REQUIRE(stats.find("Total Variants: 4") != std::string::npos);  // 2个action，每个2个变体

        // 检查具体action的统计
        REQUIRE(stats.find("Action Key: action1") != std::string::npos);
        REQUIRE(stats.find("Action Key: action2") != std::string::npos);
    }
}

// 测试重载与非重载系统的互操作性
TEST_CASE("重载与非重载系统比较", "[ActionSystem][Comparison]")
{
    //SECTION("检查两种模式的类型定义")
    //{
    //    // 检查类型别名是否正确
    //    static_assert(std::is_same<decltype(StringActionSystem::AllowOverload), const bool>::value,
    //        "StringActionSystem应该有AllowOverload成员");
    //    static_assert(StringActionSystem::AllowOverload == false,
    //        "StringActionSystem默认不应允许重载");

    //    static_assert(StringActionSystemOverload::AllowOverload == true,
    //        "StringActionSystemOverload应允许重载");
    //}

    SECTION("使用模式特定的功能")
    {
        StringActionSystem nonOverload;
        StringActionSystemOverload overload;

        // 非重载模式：同一action只能有一种参数类型
        nonOverload.AddSequentialProcessor("test", [](int x) {}, "int处理器");

        // 重载模式：同一action可以有多种参数类型
        overload.AddSequentialProcessor("test", [](int x) {}, "int处理器");
        overload.AddSequentialProcessor("test", [](float x) {}, "float处理器");
        overload.AddSequentialProcessor("test", [](const std::string& x) {}, "string处理器");

        // 验证
        REQUIRE(nonOverload.HasActionWithArgs<int>("test") == true);
        REQUIRE(nonOverload.HasActionWithArgs<float>("test") == false);  // 只有int

        REQUIRE(overload.HasActionWithArgs<int>("test") == true);
        REQUIRE(overload.HasActionWithArgs<float>("test") == true);
        REQUIRE(overload.HasActionWithArgs<std::string>("test") == true);
    }
}

// 测试边界情况
TEST_CASE("允许重载模式 - 边界情况测试", "[ActionSystem][Overload][EdgeCases]")
{
    StringActionSystemOverload system;

    SECTION("空参数列表的重载")
    {
        int voidCount = 0;
        int intCount = 0;

        system.AddSequentialProcessor("void_action",
            [&voidCount]() { voidCount++; }, "无参处理器");

        system.AddSequentialProcessor("void_action",
            [&intCount](int x) { intCount++; }, "有参处理器");

        SECTION("执行无参版本")
        {
            auto result = system.Execute("void_action");
            REQUIRE(result.success == true);
            REQUIRE(voidCount == 1);
            REQUIRE(intCount == 0);
        }

        SECTION("执行有参版本")
        {
            auto result = system.Execute("void_action", 42);
            REQUIRE(result.success == true);
            REQUIRE(voidCount == 0);
            REQUIRE(intCount == 1);
        }
    }

    SECTION("引用类型和值类型的重载")
    {
        int refCount = 0;
        int valueCount = 0;

        system.AddSequentialProcessor("ref_test",
            [&refCount](std::string& s) { refCount++; s += "_modified"; }, "引用处理器");

        system.AddSequentialProcessor("ref_test",
            [&valueCount](std::string s) { valueCount++; }, "值处理器");

        SECTION("执行引用版本")
        {
            std::string str = "original";
            auto result = system.Execute("ref_test", str);
            REQUIRE(result.success == true);
            REQUIRE(refCount == 1);
            REQUIRE(valueCount == 0);
            REQUIRE(str == "original_modified");  // 引用被修改
        }

        SECTION("执行值版本")
        {
            auto result = system.Execute("ref_test", std::string("value"));
            REQUIRE(result.success == true);
            REQUIRE(refCount == 0);
            REQUIRE(valueCount == 1);
        }
    }
}

// 测试性能相关
TEST_CASE("允许重载模式 - 性能相关测试", "[ActionSystem][Overload][Performance]")
{
    StringActionSystemOverload system;
    const int NUM_OVERLOADS = 10;

    // 添加大量重载版本
    for (int i = 0; i < NUM_OVERLOADS; ++i)
    {
        // 为每个类型创建不同的lambda，模拟不同的处理器
        system.AddSequentialProcessor("performance_test",
            [i](int value)
            {
                // 简单操作
                [[maybe_unused]] auto result = value + i;
            },
            "处理器_" + std::to_string(i));

        system.AddSequentialProcessor("performance_test",
            [i](float value)
            {
                [[maybe_unused]] auto result = value * i;
            },
            "浮点处理器_" + std::to_string(i));
    }

    SECTION("执行大量重载查找")
    {
        // 执行整数版本
        auto result = system.Execute("performance_test", 100);
        REQUIRE(result.success == true);
        REQUIRE(result.executedProcessors == NUM_OVERLOADS);

        // 执行浮点版本
        result = system.Execute("performance_test", 100.0f);
        REQUIRE(result.success == true);
        REQUIRE(result.executedProcessors == NUM_OVERLOADS);
    }
}

// 测试与原有系统的兼容性
TEST_CASE("向后兼容性测试", "[ActionSystem][Compatibility]")
{
    // 测试原有代码是否仍然工作
    SECTION("原有测试用例应仍然通过")
    {
        StringActionSystem system;  // 默认（不允许重载）

        // 原有测试用例的代码应该仍然工作
        system.AddSequentialProcessor("old_action",
            [](int x) { std::cout << "旧处理器: " << x << std::endl; },
            "旧处理器");

        auto result = system.Execute("old_action", 42);
        REQUIRE(result.success == true);
    }

    SECTION("类型别名应正确工作")
    {
        // 测试所有类型别名
        IntActionSystem intSystem;
        IntActionSystemOverload intOverloadSystem;
        EnumActionSystem enumSystem;

        // 确保它们都能正常工作
        intSystem.AddSequentialProcessor(1, [](int x) {}, "int处理器");
        intOverloadSystem.AddSequentialProcessor(1, [](int x) {}, "int处理器");
        intOverloadSystem.AddSequentialProcessor(1, [](float x) {}, "float处理器");
        enumSystem.AddSequentialProcessor(100, [](const std::string& s) {}, "enum处理器");

        REQUIRE(intSystem.HasAction(1) == true);
        REQUIRE(intOverloadSystem.HasAction(1) == true);
        REQUIRE(enumSystem.HasAction(100) == true);
    }
}
