#include "catch2/catch_test_macros.hpp"
#include "catch2/catch_all.hpp"
#include <EditorKit/KEventBus.h>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>

// 测试用事件类型
enum class TestEventType {
    EVENT_A,
    EVENT_B,
    EVENT_C
};

// 自定义哈希函数用于TestEventType
struct TestEventTypeHash {
    std::size_t operator()(const TestEventType& type) const {
        return static_cast<std::size_t>(type);
    }
};

// 测试用结构体
struct TestData {
    int id;
    std::string name;
    double value;
    
    bool operator==(const TestData& other) const {
        return id == other.id && name == other.name && value == other.value;
    }
};

// 测试用例开始
TEST_CASE("EventBus 基本功能测试", "[EventBus]") {
    EventBus<std::string> bus;
    
    SECTION("基本订阅和发布") {
        bool handlerCalled = false;
        int receivedValue = 0;
        
        auto token = bus.Subscribe("test_event", [&](int value) {
            handlerCalled = true;
            receivedValue = value;
        });
        
        REQUIRE(handlerCalled == false);
        
        auto result = bus.Publish("test_event", 42);
        REQUIRE(handlerCalled == true);
        REQUIRE(receivedValue == 42);
        REQUIRE(result.success == true);
        REQUIRE(result.totalSubscribers == 1);
        REQUIRE(result.successfulExecutions == 1);
    }
    
    SECTION("多播事件测试") {
        std::vector<int> receivedValues;
        
        bus.Subscribe("multicast_event", [&](int value) {
            receivedValues.push_back(value * 1);
        });
        
        bus.Subscribe("multicast_event", [&](int value) {
            receivedValues.push_back(value * 2);
        });
        
        bus.Subscribe("multicast_event", [&](int value) {
            receivedValues.push_back(value * 3);
        });
        
        auto result = bus.Publish("multicast_event", 10);
        
        REQUIRE(result.success == true);
        REQUIRE(result.totalSubscribers == 3);
        REQUIRE(result.successfulExecutions == 3);
        REQUIRE(receivedValues.size() == 3);
        REQUIRE(std::find(receivedValues.begin(), receivedValues.end(), 10) != receivedValues.end());
        REQUIRE(std::find(receivedValues.begin(), receivedValues.end(), 20) != receivedValues.end());
        REQUIRE(std::find(receivedValues.begin(), receivedValues.end(), 30) != receivedValues.end());
    }
    
    SECTION("单播事件测试") {
        int firstHandlerValue = 0;
        int secondHandlerValue = 0;
        
        bus.SubscribeUnicast("unicast_event", [&](int value) {
            firstHandlerValue = value;
        });
        
        // 第二个订阅应该覆盖第一个
        bus.SubscribeUnicast("unicast_event", [&](int value) {
            secondHandlerValue = value;
        });
        
        auto result = bus.PublishUnicast("unicast_event", 100);
        
        REQUIRE(result.success == true);
        REQUIRE(result.totalSubscribers == 1);
        REQUIRE(firstHandlerValue == 0);  // 第一个处理器不应该被调用
        REQUIRE(secondHandlerValue == 100); // 第二个处理器应该被调用
    }
    
    SECTION("取消订阅测试") {
        bool handler1Called = false;
        bool handler2Called = false;
        
        auto token1 = bus.Subscribe("unsubscribe_test", [&](int) {
            handler1Called = true;
        });
        
        auto token2 = bus.Subscribe("unsubscribe_test", [&](int) {
            handler2Called = true;
        });
        
        // 取消第一个订阅
        REQUIRE(bus.Unsubscribe(token1) == true);
        
        bus.Publish("unsubscribe_test", 1);
        
        REQUIRE(handler1Called == false);
        REQUIRE(handler2Called == true);
        
        // 取消不存在的token应该返回false
        REQUIRE(bus.Unsubscribe(EventID::generate()) == false);
    }
}

TEST_CASE("EventBus 参数类型测试", "[EventBus]") {
    EventBus<std::string> bus;
    
    SECTION("多种参数类型测试") {
        std::string receivedString;
        double receivedDouble = 0.0;
        TestData receivedData;
        
        bus.Subscribe("string_event", [&](std::string str) {
            receivedString = str;
        });
        
        bus.Subscribe("double_event", [&](double d) {
            receivedDouble = d;
        });
        
        bus.Subscribe("struct_event", [&](TestData data) {
            receivedData = data;
        });
        
        bus.Publish("string_event", std::string("Hello World"));
        bus.Publish("double_event", 3.14159);
        bus.Publish("struct_event", TestData{1, "test", 2.5});
        
        REQUIRE(receivedString == "Hello World");
        REQUIRE(receivedDouble == Catch::Approx(3.14159));
        REQUIRE(receivedData.id == 1);
        REQUIRE(receivedData.name == "test");
        REQUIRE(receivedData.value == 2.5);
    }
    
    SECTION("多参数测试") {
        int receivedInt = 0;
        std::string receivedStr;
        double receivedDouble = 0.0;
        
        bus.Subscribe("multi_param_event", [&](int a, std::string b, double c) {
            receivedInt = a;
            receivedStr = b;
            receivedDouble = c;
        });
        
        bus.Publish("multi_param_event", 42, std::string("test"), 1.234);
        
        REQUIRE(receivedInt == 42);
        REQUIRE(receivedStr == "test");
        REQUIRE(receivedDouble == Catch::Approx(1.234));
    }
    
    SECTION("参数类型不匹配测试") {
        bool handlerCalled = false;
        
        bus.Subscribe("type_mismatch_event", [&](int value) {
            handlerCalled = true;
        });
        
        // 发布错误类型的参数
        auto result = bus.Publish("type_mismatch_event", std::string("wrong_type"));
        
        REQUIRE(handlerCalled == false);
        REQUIRE(result.success == false);
        REQUIRE(result.failedExecutions == 1);
        REQUIRE(result.errorMessage.find("执行失败") != std::string::npos);
    }
}

TEST_CASE("EventBus 一次性订阅测试", "[EventBus]") {
    EventBus<std::string> bus;
    
    SECTION("一次性多播订阅") {
        int callCount = 0;
        
        bus.Subscribe("once_multicast", [&](int value) {
            callCount += value;
        }, "一次性多播测试", true);
        
        bus.Publish("once_multicast", 1);
        bus.Publish("once_multicast", 1);
        bus.Publish("once_multicast", 1);
        
        // 应该只被调用一次
        REQUIRE(callCount == 1);
        REQUIRE(bus.GetSubscriberCount("once_multicast") == 0);
    }
    
    SECTION("一次性单播订阅") {
        int callCount = 0;
        
        bus.SubscribeUnicast("once_unicast", [&](int value) {
            callCount += value;
        }, "一次性单播测试", true);
        
        bus.PublishUnicast("once_unicast", 1);
        bus.PublishUnicast("once_unicast", 1);
        bus.PublishUnicast("once_unicast", 1);
        
        REQUIRE(callCount == 1);
        REQUIRE(bus.HasUnicastSubscribers("once_unicast") == false);
    }
}

TEST_CASE("EventBus 查询功能测试", "[EventBus]") {
    EventBus<std::string> bus;
    
    SECTION("订阅者数量查询") {
        REQUIRE(bus.GetSubscriberCount("nonexistent_event") == 0);
        REQUIRE(bus.HasSubscribers("nonexistent_event") == false);
        
        bus.Subscribe("query_test", [](int) {});
        bus.Subscribe("query_test", [](int) {});
        bus.Subscribe("query_test", [](int) {});
        
        REQUIRE(bus.GetSubscriberCount("query_test") == 3);
        REQUIRE(bus.HasSubscribers("query_test") == true);
    }
    
    SECTION("单播订阅者查询") {
        REQUIRE(bus.GetUnicastSubscriberCount("nonexistent_unicast") == 0);
        REQUIRE(bus.HasUnicastSubscribers("nonexistent_unicast") == false);
        
        bus.SubscribeUnicast("unicast_query_test", [](int) {});
        
        REQUIRE(bus.GetUnicastSubscriberCount("unicast_query_test") == 1);
        REQUIRE(bus.HasUnicastSubscribers("unicast_query_test") == true);
    }
    
    SECTION("事件存在性检查") {
        REQUIRE(bus.HasEvent("nonexistent") == false);
        
        bus.Subscribe("test_event", [](int) {});
        REQUIRE(bus.HasEvent("test_event") == true);
        
        bus.SubscribeUnicast("unicast_event", [](int) {});
        REQUIRE(bus.HasEvent("unicast_event") == true);
    }
    
    SECTION("事件模式查询") {
        bus.Subscribe("multicast_event", [](int) {});
        REQUIRE(bus.GetEventMode("multicast_event") == SubscriptionMode::Multicast);
        
        bus.SubscribeUnicast("unicast_event", [](int) {});
        REQUIRE(bus.GetEventMode("unicast_event") == SubscriptionMode::Unicast);
        
        // 不存在的事件返回多播模式（默认）
        REQUIRE(bus.GetEventMode("nonexistent") == SubscriptionMode::Multicast);
    }
}

TEST_CASE("EventBus 自定义键类型测试", "[EventBus]") {
    SECTION("枚举类型键") {
        EventBus<TestEventType, TestEventTypeHash> enumBus;
        
        bool eventACalled = false;
        bool eventBCalled = false;
        
        enumBus.Subscribe(TestEventType::EVENT_A, [&](int) {
            eventACalled = true;
        });
        
        enumBus.Subscribe(TestEventType::EVENT_B, [&](int) {
            eventBCalled = true;
        });
        
        enumBus.Publish(TestEventType::EVENT_A, 1);
        enumBus.Publish(TestEventType::EVENT_B, 2);
        
        REQUIRE(eventACalled == true);
        REQUIRE(eventBCalled == true);
    }
    
    SECTION("整数类型键") {
        EventBus<int> intBus;
        
        std::vector<int> receivedValues;
        
        intBus.Subscribe(100, [&](int value) {
            receivedValues.push_back(value);
        });
        
        intBus.Subscribe(200, [&](int value) {
            receivedValues.push_back(value * 2);
        });
        
        intBus.Publish(100, 50);
        intBus.Publish(200, 30);
        
        REQUIRE(receivedValues.size() == 2);
        REQUIRE(receivedValues[0] == 50);
        REQUIRE(receivedValues[1] == 60);
    }
}

TEST_CASE("EventBus 统计信息测试", "[EventBus]") {
    EventBus<std::string> bus;
    
    SECTION("发布结果统计") {
        bus.Subscribe("stats_test", [](int) {});
        bus.Subscribe("stats_test", [](int) {});
        bus.Subscribe("stats_test", [](int) {});
        
        auto result = bus.Publish("stats_test", 42);
        
        REQUIRE(result.success == true);
        REQUIRE(result.totalSubscribers == 3);
        REQUIRE(result.successfulExecutions == 3);
        REQUIRE(result.failedExecutions == 0);
        REQUIRE(result.publishedArgTypes.find("int") != std::string::npos);
        REQUIRE(result.publishMode == SubscriptionMode::Multicast);
        
        std::string stats = result.GetStatistics();
        REQUIRE(stats.find("3/3") != std::string::npos);
    }
    
    SECTION("错误统计") {
        bus.Subscribe("error_test", [](int) {});
        
        // 发布错误类型参数
        auto result = bus.Publish("error_test", std::string("wrong_type"));
        
        REQUIRE(result.success == false);
        REQUIRE(result.failedExecutions == 1);
        REQUIRE(result.errorMessage.empty() == false);
        REQUIRE(result.failedSubscriberTypes.size() == 1);
    }
    
    SECTION("事件信息打印") {
        bus.Subscribe("print_test", [](int value) {}, "测试事件1");
        bus.Subscribe("print_test", [](int value) {}, "测试事件2");
        bus.SubscribeUnicast("unicast_print_test", [](std::string value) {}, "单播测试事件");
        
        std::string eventsInfo = bus.PrintAllEvents();
        std::string stats = bus.GetEventStatistics();
        
        REQUIRE(eventsInfo.find("多播事件类型数量") != std::string::npos);
        REQUIRE(eventsInfo.find("单播事件类型数量") != std::string::npos);
        REQUIRE(eventsInfo.find("print_test") != std::string::npos);
        REQUIRE(stats.find("Multicast Event Types") != std::string::npos);
        REQUIRE(stats.find("Unicast Event Types") != std::string::npos);
    }
}

//TEST_CASE("EventBus 线程安全基本测试", "[EventBus]") {
//    EventBus<std::string> bus;
//    
//    SECTION("多线程订阅发布") {
//        std::atomic<int> callCount{0};
//        const int threadCount = 10;
//        const int eventsPerThread = 100;
//        
//        // 在多个线程中订阅
//        std::vector<std::thread> subscriberThreads;
//        for (int i = 0; i < threadCount; ++i) {
//            subscriberThreads.emplace_back([&, i]() {
//                bus.Subscribe("thread_test", [&](int value) {
//                    callCount.fetch_add(1, std::memory_order_relaxed);
//                });
//            });
//        }
//        
//        for (auto& thread : subscriberThreads) {
//            thread.join();
//        }
//        
//        // 在多个线程中发布
//        std::vector<std::thread> publisherThreads;
//        for (int i = 0; i < threadCount; ++i) {
//            publisherThreads.emplace_back([&]() {
//                for (int j = 0; j < eventsPerThread; ++j) {
//                    bus.Publish("thread_test", j);
//                }
//            });
//        }
//        
//        for (auto& thread : publisherThreads) {
//            thread.join();
//        }
//        
//        // 验证总调用次数
//        int expectedCalls = threadCount * eventsPerThread * bus.GetSubscriberCount("thread_test");
//        REQUIRE(callCount.load() == expectedCalls);
//    }
//}

TEST_CASE("EventID 功能测试", "[EventID]") {
    SECTION("EventID 生成和比较") {
        EventID uuid1 = EventID::generate();
        EventID uuid2 = EventID::generate();
        
        REQUIRE(uuid1 == uuid1);
        REQUIRE((uuid1 < uuid2 || uuid2 < uuid1)); // 至少有一个为真
        REQUIRE_FALSE(uuid1 == uuid2);
        
        // 测试哈希
        EventID::Hash hasher;
        REQUIRE(hasher(uuid1) != hasher(uuid2));
    }
    
    SECTION("EventID 字符串转换") {
        EventID uuid(0x123456789ABCDEF0, 0xFEDCBA9876543210);
        std::string str = uuid.toString();
        
        REQUIRE(str.length() == 32);
        REQUIRE(str.find("123456789abcdef0") != std::string::npos);
        REQUIRE(str.find("fedcba9876543210") != std::string::npos);
    }
}

TEST_CASE("EventBus 边界情况测试", "[EventBus]") {
    EventBus<std::string> bus;
    
    SECTION("无参数事件") {
        bool called = false;
        
        bus.Subscribe("no_args", [&]() {
            called = true;
        });
        
        bus.Publish("no_args");
        
        REQUIRE(called == true);
    }
    
    SECTION("大量订阅者") {
        const int largeCount = 1000;
        std::atomic<int> counter{0};
        
        for (int i = 0; i < largeCount; ++i) {
            bus.Subscribe("mass_subscribe", [&](int value) {
                counter.fetch_add(value, std::memory_order_relaxed);
            });
        }
        
        bus.Publish("mass_subscribe", 1);
        
        REQUIRE(counter.load() == largeCount);
        REQUIRE(bus.GetSubscriberCount("mass_subscribe") == largeCount);
    }
    
}

// 性能测试（可选，需要CATCH_CONFIG_ENABLE_BENCHMARK）
#ifdef CATCH_CONFIG_ENABLE_BENCHMARK
TEST_CASE("EventBus 性能测试", "[.][benchmark]") {
    EventBus<std::string> bus;
    
    BENCHMARK("基本发布性能") {
        static bool handlerCalled = false;
        static auto token = bus.Subscribe("benchmark", [](int value) {
            handlerCalled = true;
        });
        
        return bus.Publish("benchmark", 42);
    };
    
    BENCHMARK("1000个订阅者的发布性能") {
        static std::atomic<int> counter{0};
        
        // 一次性设置
        static bool initialized = []() {
            for (int i = 0; i < 1000; ++i) {
                bus.Subscribe("mass_benchmark", [](int value) {
                    counter.fetch_add(1, std::memory_order_relaxed);
                });
            }
            return true;
        }();
        
        return bus.Publish("mass_benchmark", 1);
    };
}
#endif

// 测试主函数（如果使用独立的测试运行器，则不需要）
// #define CATCH_CONFIG_MAIN
// #include "catch2/catch_all.hpp"