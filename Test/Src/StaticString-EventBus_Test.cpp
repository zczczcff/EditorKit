#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <EditorKit/KEventBus.h>
#include <EditorKit/StaticString.h>

TEST_CASE("EventBus with StaticString key type", "[EventBus][StaticString]")
{
    // 创建EventBus实例，使用StaticString作为键类型
    EventBus<StaticString> eventBus;

    SECTION("基本订阅和发布测试")
    {
        int callbackCount = 0;
        std::string receivedMessage;

        // 订阅事件
        auto token = eventBus.Subscribe("test_event", [&](std::string message)
            {
                callbackCount++;
                receivedMessage = message;
            });

        REQUIRE(callbackCount == 0);

        // 发布事件
        auto result = eventBus.Publish("test_event", std::string("Hello World"));

        REQUIRE(result.success == true);
        REQUIRE(callbackCount == 1);
        REQUIRE(receivedMessage == "Hello World");
    }

    SECTION("多播事件测试")
    {
        int callback1Count = 0;
        int callback2Count = 0;

        // 订阅多个回调
        auto token1 = eventBus.Subscribe("multicast_event", [&](int value)
            {
                callback1Count += value;
            });

        auto token2 = eventBus.Subscribe("multicast_event", [&](int value)
            {
                callback2Count += value * 2;
            });

        REQUIRE(eventBus.GetSubscriberCount("multicast_event") == 2);

        // 发布事件，两个回调都应该执行
        auto result = eventBus.Publish("multicast_event", 5);

        REQUIRE(result.success == true);
        REQUIRE(result.successfulExecutions == 2);
        REQUIRE(callback1Count == 5);
        REQUIRE(callback2Count == 10);
    }

    SECTION("单播事件测试")
    {
        int firstCallbackCount = 0;
        int secondCallbackCount = 0;

        // 第一个订阅者（单播模式）
        auto token1 = eventBus.SubscribeUnicast("unicast_event", [&](double value)
            {
                firstCallbackCount++;
            });

        REQUIRE(eventBus.HasUnicastSubscribers("unicast_event") == true);
        REQUIRE(eventBus.GetUnicastSubscriberCount("unicast_event") == 1);

        // 第二个订阅者会覆盖第一个
        auto token2 = eventBus.SubscribeUnicast("unicast_event", [&](double value)
            {
                secondCallbackCount++;
            });

        // 发布事件，只有第二个回调执行
        auto result = eventBus.PublishUnicast("unicast_event", 3.14);

        REQUIRE(result.success == true);
        REQUIRE(firstCallbackCount == 0);
        REQUIRE(secondCallbackCount == 1);
    }

    SECTION("StaticString键比较和哈希测试")
    {
        int callbackCount = 0;

        // 使用不同的StaticString构造方式订阅事件
        StaticString eventName1("same_event");
        StaticString eventName2("same_event");
        StaticString eventName3 = "same_event";

        // 这些应该被认为是相同的事件
        auto token1 = eventBus.Subscribe(eventName1, [&]()
            {
                callbackCount++;
            });

        // 使用不同构造方式发布到相同事件
        auto result1 = eventBus.Publish(eventName2);
        REQUIRE(callbackCount == 1);

        auto result2 = eventBus.Publish(eventName3);
        REQUIRE(callbackCount == 2);

        // 测试不同的事件名称
        StaticString differentEvent("different_event");
        auto result3 = eventBus.Publish(differentEvent);
        REQUIRE(result3.success == false);  // 应该失败，因为没有订阅者
        REQUIRE(callbackCount == 2);  // 计数不应改变
    }

    SECTION("取消订阅测试")
    {
        int callbackCount = 0;

        auto token = eventBus.Subscribe("unsubscribe_test", [&](bool flag)
            {
                callbackCount++;
            });

        REQUIRE(eventBus.HasSubscribers("unsubscribe_test") == true);

        // 取消订阅
        bool unsubResult = eventBus.Unsubscribe(token);
        REQUIRE(unsubResult == true);
        REQUIRE(eventBus.HasSubscribers("unsubscribe_test") == false);

        // 发布事件应该不会触发回调
        auto result = eventBus.Publish("unsubscribe_test", true);
        REQUIRE(result.success == false);
        REQUIRE(callbackCount == 0);
    }

    SECTION("一次性事件测试")
    {
        int callbackCount = 0;

        // 订阅一次性事件
        auto token = eventBus.Subscribe("once_event", [&](int value)
            {
                callbackCount += value;
            }, "", true);  // once = true

        REQUIRE(eventBus.HasSubscribers("once_event") == true);

        // 第一次发布应该成功
        auto result1 = eventBus.Publish("once_event", 10);
        REQUIRE(result1.success == true);
        REQUIRE(callbackCount == 10);

        // 第二次发布应该失败（事件已被自动取消）
        auto result2 = eventBus.Publish("once_event", 5);
        REQUIRE(result2.success == false);
        REQUIRE(callbackCount == 10);  // 计数不应改变
    }

    SECTION("类型安全测试")
    {
        int intCallbackCount = 0;
        std::string stringCallbackValue;

        // 订阅int类型事件
        auto intToken = eventBus.Subscribe("type_test", [&](int value)
            {
                intCallbackCount = value;
            });

        // 订阅string类型事件（相同事件名称，不同类型）
        auto stringToken = eventBus.Subscribe("type_test", [&](std::string value)
            {
                stringCallbackValue = value;
            });

        // 发布int类型事件
        auto intResult = eventBus.Publish("type_test", 42);
        REQUIRE(intResult.success == true);
        REQUIRE(intResult.totalSubscribers == 2);
        REQUIRE(intResult.successfulExecutions == 1);  // 只有int回调匹配
        REQUIRE(intResult.failedExecutions == 1);      // string回调不匹配
        REQUIRE(intCallbackCount == 42);
        REQUIRE(stringCallbackValue.empty());  // string回调不应被调用

        // 发布string类型事件
        auto stringResult = eventBus.Publish("type_test", std::string("test"));
        REQUIRE(stringResult.success == true);
        REQUIRE(stringResult.successfulExecutions == 1);  // 只有string回调匹配
        REQUIRE(stringResult.failedExecutions == 1);      // int回调不匹配
        REQUIRE(stringCallbackValue == "test");
    }

    SECTION("事件统计信息测试")
    {
        // 设置多个事件
        eventBus.Subscribe("event1", []() {});
        eventBus.Subscribe("event1", [](int) {});
        eventBus.SubscribeUnicast("event2", [](double) {});
        eventBus.Subscribe("event3", [](std::string) {});

        // 获取统计信息
        auto stats = eventBus.GetEventStatistics();
        auto allEvents = eventBus.PrintAllEvents();

        REQUIRE_FALSE(stats.empty());
        REQUIRE_FALSE(allEvents.empty());

        // 验证基本统计
        REQUIRE(eventBus.HasEvent("event1") == true);
        REQUIRE(eventBus.HasEvent("event2") == true);
        REQUIRE(eventBus.HasEvent("event3") == true);
        REQUIRE(eventBus.HasEvent("nonexistent_event") == false);

        // 验证订阅模式
        REQUIRE(eventBus.GetEventMode("event1") == SubscriptionMode::Multicast);
        REQUIRE(eventBus.GetEventMode("event2") == SubscriptionMode::Unicast);
    }

    SECTION("复杂StaticString场景测试")
    {
        std::vector<std::string> receivedEvents;

        // 使用复杂的StaticString场景
        StaticString complexEventName("complex.event.name.with.dots");
        StaticString anotherEvent("another-event");
        StaticString utf8Event(u8"测试事件");

        // 订阅各种事件
        eventBus.Subscribe(complexEventName, [&]()
            {
                receivedEvents.push_back("complex");
            });

        eventBus.Subscribe(anotherEvent, [&](int x)
            {
                receivedEvents.push_back("another_" + std::to_string(x));
            });

        eventBus.Subscribe(utf8Event, [&](std::string msg)
            {
                receivedEvents.push_back("utf8_" + msg);
            });

        // 发布事件
        eventBus.Publish(complexEventName);
        eventBus.Publish(anotherEvent, 100);
        eventBus.Publish(utf8Event, std::string("message"));

        REQUIRE(receivedEvents.size() == 3);
        REQUIRE(receivedEvents[0] == "complex");
        REQUIRE(receivedEvents[1] == "another_100");
        REQUIRE(receivedEvents[2] == "utf8_message");
    }
}

TEST_CASE("StaticString哈希和比较特性", "[StaticString]")
{
    SECTION("相同内容的StaticString应该有相同的哈希值")
    {
        StaticString str1("test");
        StaticString str2("test");
        StaticString str3 = "test";

        REQUIRE(str1.hash() == str2.hash());
        REQUIRE(str1.hash() == str3.hash());
        REQUIRE(str1 == str2);
        REQUIRE(str1 == str3);
    }

    SECTION("不同内容的StaticString应该有不同的哈希值")
    {
        StaticString str1("test1");
        StaticString str2("test2");

        REQUIRE(str1.hash() != str2.hash());
        REQUIRE(str1 != str2);
    }

    SECTION("空字符串处理")
    {
        StaticString empty1;
        StaticString empty2("");
        StaticString empty3 = "";

        REQUIRE(empty1 == empty2);
        REQUIRE(empty1 == empty3);
        REQUIRE(empty1.hash() == empty2.hash());
        REQUIRE(empty1.str().empty());
    }
}
#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <chrono>
#include <unordered_map>


// 预定义的StaticString常量
const StaticString PREDEFINED_EVENT_1("predefined_event_1");
const StaticString PREDEFINED_EVENT_2("predefined_event_2");
const StaticString PREDEFINED_EVENT_3("predefined_event_3");

// 枚举类型用于对比
enum class EventEnum
{
    EVENT_1,
    EVENT_2,
    EVENT_3,
    EVENT_4,
    EVENT_5
};

// 测试函数
void performanceTest()
{
    const int TEST_COUNT = 100000; // 测试次数
    std::cout << "开始性能测试，测试次数: " << TEST_COUNT << std::endl;
    std::cout << "==========================================" << std::endl;

    // 1. 预定义StaticString常量测试
    {
        EventBus<StaticString> bus;
        auto start = std::chrono::high_resolution_clock::now();

        // 订阅
        bus.Subscribe(PREDEFINED_EVENT_1, [](int data) { /* 空回调 */ });
        bus.Subscribe(PREDEFINED_EVENT_2, [](int data) { /* 空回调 */ });
        bus.Subscribe(PREDEFINED_EVENT_3, [](int data) { /* 空回调 */ });

        // 发布
        for (int i = 0; i < TEST_COUNT; ++i)
        {
            bus.Publish(PREDEFINED_EVENT_1, i);
            bus.Publish(PREDEFINED_EVENT_2, i);
            bus.Publish(PREDEFINED_EVENT_3, i);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "预定义StaticString常量 - 耗时: " << duration.count() << " 微秒" << std::endl;
    }

    // 2. 动态构造StaticString测试
    {
        EventBus<StaticString> bus;
        auto start = std::chrono::high_resolution_clock::now();

        // 订阅动态创建的事件
        bus.Subscribe(StaticString("dynamic_event_1"), [](int data) {});
        bus.Subscribe(StaticString("dynamic_event_2"), [](int data) {});
        bus.Subscribe(StaticString("dynamic_event_3"), [](int data) {});

        // 发布到动态创建的事件
        for (int i = 0; i < TEST_COUNT; ++i)
        {
            bus.Publish(StaticString("dynamic_event_1"), i);
            bus.Publish(StaticString("dynamic_event_2"), i);
            bus.Publish(StaticString("dynamic_event_3"), i);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "动态构造StaticString - 耗时: " << duration.count() << " 微秒" << std::endl;
    }

    // 3. std::string测试
    {
        EventBus<std::string> bus;
        auto start = std::chrono::high_resolution_clock::now();

        // 订阅
        bus.Subscribe("string_event_1", [](int data) {});
        bus.Subscribe("string_event_2", [](int data) {});
        bus.Subscribe("string_event_3", [](int data) {});

        // 发布
        for (int i = 0; i < TEST_COUNT; ++i)
        {
            bus.Publish("string_event_1", i);
            bus.Publish("string_event_2", i);
            bus.Publish("string_event_3", i);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "std::string - 耗时: " << duration.count() << " 微秒" << std::endl;
    }

    // 4. 枚举类型测试
    {
        EventBus<EventEnum> bus;
        auto start = std::chrono::high_resolution_clock::now();

        // 订阅
        bus.Subscribe(EventEnum::EVENT_1, [](int data) {});
        bus.Subscribe(EventEnum::EVENT_2, [](int data) {});
        bus.Subscribe(EventEnum::EVENT_3, [](int data) {});

        // 发布
        for (int i = 0; i < TEST_COUNT; ++i)
        {
            bus.Publish(EventEnum::EVENT_1, i);
            bus.Publish(EventEnum::EVENT_2, i);
            bus.Publish(EventEnum::EVENT_3, i);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "枚举类型 - 耗时: " << duration.count() << " 微秒" << std::endl;
    }

    std::cout << "==========================================" << std::endl;
}

// 单独测试哈希查找性能
void hashLookupTest()
{
    const int LOOKUP_COUNT = 1000000;
    std::cout << "\n哈希查找性能测试，查找次数: " << LOOKUP_COUNT << std::endl;
    std::cout << "==========================================" << std::endl;

    // 准备测试数据
    std::vector<StaticString> staticStrings;
    std::vector<std::string> stdStrings;
    std::vector<EventEnum> enums;

    for (int i = 0; i < 1000; ++i)
    {
        staticStrings.push_back(StaticString(("test_event_" + std::to_string(i)).c_str()));
        stdStrings.push_back("test_event_" + std::to_string(i));
        enums.push_back(static_cast<EventEnum>(i % 5));
    }

    // StaticString哈希查找测试
    {
        auto start = std::chrono::high_resolution_clock::now();

        std::hash<StaticString> hasher;
        size_t totalHash = 0;
        for (int i = 0; i < LOOKUP_COUNT; ++i)
        {
            const auto& str = staticStrings[i % staticStrings.size()];
            totalHash += hasher(str);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "StaticString哈希计算 - 耗时: " << duration.count()
            << " 微秒, 总哈希值: " << totalHash << std::endl;
    }

    // std::string哈希查找测试
    {
        auto start = std::chrono::high_resolution_clock::now();

        std::hash<std::string> hasher;
        size_t totalHash = 0;
        for (int i = 0; i < LOOKUP_COUNT; ++i)
        {
            const auto& str = stdStrings[i % stdStrings.size()];
            totalHash += hasher(str);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "std::string哈希计算 - 耗时: " << duration.count()
            << " 微秒, 总哈希值: " << totalHash << std::endl;
    }

    // 枚举哈希查找测试
    {
        auto start = std::chrono::high_resolution_clock::now();

        std::hash<EventEnum> hasher;
        size_t totalHash = 0;
        for (int i = 0; i < LOOKUP_COUNT; ++i)
        {
            EventEnum e = enums[i % enums.size()];
            totalHash += hasher(e);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "枚举类型哈希计算 - 耗时: " << duration.count()
            << " 微秒, 总哈希值: " << totalHash << std::endl;
    }

    std::cout << "==========================================" << std::endl;
}

// 内存使用测试
void memoryUsageTest()
{
    std::cout << "\n内存使用分析" << std::endl;
    std::cout << "==========================================" << std::endl;

    // 创建大量事件进行内存测试
    const int EVENT_COUNT = 10000;

    {
        EventBus<StaticString> bus;
        // 订阅大量事件
        for (int i = 0; i < EVENT_COUNT; ++i)
        {
            StaticString eventName(("static_event_" + std::to_string(i)).c_str());
            bus.Subscribe(eventName, [](int data) {});
        }
        std::cout << "StaticString事件总线 - 创建了 " << EVENT_COUNT << " 个事件" << std::endl;
        std::cout << "StaticString使用内部字符串池，重复字符串只存储一次" << std::endl;
    }

    {
        EventBus<std::string> bus;
        // 订阅大量事件
        for (int i = 0; i < EVENT_COUNT; ++i)
        {
            std::string eventName = "string_event_" + std::to_string(i);
            bus.Subscribe(eventName, [](int data) {});
        }
        std::cout << "std::string事件总线 - 创建了 " << EVENT_COUNT << " 个事件" << std::endl;
        std::cout << "每个std::string独立存储，可能占用更多内存" << std::endl;
    }

    std::cout << "==========================================" << std::endl;
}

TEST_CASE("实际使用场景性能测试", "[EventBus][RealWorld][Performance]")
{
    SECTION("性能测试")
    {
        // 运行性能测试
        performanceTest();

        // 运行哈希查找测试
        hashLookupTest();

        // 运行内存使用分析
        memoryUsageTest();
    }
}

