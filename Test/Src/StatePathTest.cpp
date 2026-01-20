#include <iostream>
#include <string>
#include <memory>
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>
#include <EditorKit/StatePath.h>

TEST_CASE("基础操作测试", "[StatePath][Basic]")
{
    StatePath system;

    SECTION("设置和获取整型值")
    {
        system.setInt("config/width", 1920);
        int width = 0;
        bool gotWidth = system.getInt("config/width", width);

        REQUIRE(gotWidth == true);
        REQUIRE(width == 1920);
    }

    SECTION("设置和获取浮点值")
    {
        system.setFloat("config/ratio", 1.618f);
        float ratio = 0;
        bool gotRatio = system.getFloat("config/ratio", ratio);

        REQUIRE(gotRatio == true);
        REQUIRE(ratio > 1.617f);
        REQUIRE(ratio < 1.619f);
    }

    SECTION("设置和获取布尔值")
    {
        system.setBool("config/enabled", true);
        bool enabled = false;
        bool gotEnabled = system.getBool("config/enabled", enabled);

        REQUIRE(gotEnabled == true);
        REQUIRE(enabled == true);
    }

    SECTION("设置和获取字符串")
    {
        system.setString("config/name", "测试系统");
        std::string name;
        bool gotName = system.getString("config/name", name);

        REQUIRE(gotName == true);
        REQUIRE(name == "测试系统");
    }

    SECTION("创建对象节点")
    {
        system.setObject("config/display");
        REQUIRE(system.hasNode("config/display") == true);
    }

    SECTION("操作符重载设置和获取")
    {
        system["config/height"] = 1080;
        int height = 0;
        bool gotHeight = system["config/height"].get(height);

        REQUIRE(gotHeight == true);
        REQUIRE(height == 1080);
    }
}

TEST_CASE("事件系统测试", "[StatePath][Events]")
{
    StatePath system;
    int eventCount = 0;
    std::string lastEventPath;

    SECTION("ADD事件触发")
    {
        auto listenerId = system.addEventListener("config", ListenGranularity::ALL_CHILDREN,
            EventType::ADD, [&](const PathEvent& event)
            {
                eventCount++;
                lastEventPath = event.path;
            });

        system.setInt("config/newSetting", 42);

        REQUIRE(eventCount > 0);
        REQUIRE(lastEventPath == "config/newSetting");

        system.removeEventListener(listenerId);
    }

    SECTION("UPDATE事件触发")
    {
        eventCount = 0;
        auto listenerId = system.addEventListener("config/width", ListenGranularity::NODE,
            EventType::UPDATE, [&](const PathEvent& event)
            {
                eventCount++;
            });

        system.setInt("config/width", 100);
        system.setInt("config/width", 200); // 触发UPDATE

        REQUIRE(eventCount > 0);

        system.removeEventListener(listenerId);
    }

    SECTION("REMOVE事件触发")
    {
        eventCount = 0;
        system.setInt("config/width", 100);

        auto listenerId = system.addEventListener("config/width", ListenGranularity::NODE,
            EventType::REMOVE, [&](const PathEvent& event)
            {
                eventCount++;
            });

        system.removeNode("config/width");

        REQUIRE(eventCount > 0);

        system.removeEventListener(listenerId);
    }

    SECTION("移除事件监听器")
    {
        auto listenerId = system.addEventListener("config", ListenGranularity::NODE,
            EventType::ADD, [](const PathEvent& event) {});

        bool removed = system.removeEventListener(listenerId);
        REQUIRE(removed == true);
    }
}

TEST_CASE("节点类型测试", "[StatePath][NodeTypes]")
{
    StatePath system;

    system.setInt("types/int", 100);
    system.setFloat("types/float", 3.14f);
    system.setBool("types/bool", true);
    system.setString("types/string", "hello");
    system.setObject("types/object");

    SECTION("整型节点类型")
    {
        REQUIRE(system.getNodeType("types/int") == NodeType::INT);
    }

    SECTION("浮点节点类型")
    {
        REQUIRE(system.getNodeType("types/float") == NodeType::FLOAT);
    }

    SECTION("布尔节点类型")
    {
        REQUIRE(system.getNodeType("types/bool") == NodeType::BOOL);
    }

    SECTION("字符串节点类型")
    {
        REQUIRE(system.getNodeType("types/string") == NodeType::STRING);
    }

    SECTION("对象节点类型")
    {
        REQUIRE(system.getNodeType("types/object") == NodeType::OBJECT);
    }

    SECTION("不存在节点类型")
    {
        REQUIRE(system.getNodeType("types/nonexistent") == NodeType::EMPTY);
    }

    SECTION("模板值获取")
    {
        std::string strValue;
        bool gotString = system.getValue<std::string>("types/string", strValue);

        REQUIRE(gotString == true);
        REQUIRE(strValue == "hello");
    }
}

TEST_CASE("路径操作测试", "[StatePath][PathOperations]")
{
    StatePath system;

    SECTION("嵌套路径创建")
    {
        system.setInt("deep/nested/path/value", 999);
        int deepValue = 0;
        bool gotDeep = system.getInt("deep/nested/path/value", deepValue);

        REQUIRE(gotDeep == true);
        REQUIRE(deepValue == 999);
    }

    SECTION("节点存在性检查")
    {
        system.setInt("test/exists", 123);

        REQUIRE(system.hasNode("test/exists") == true);
        REQUIRE(system.hasNode("test/nonexistent") == false);
    }

    SECTION("移动节点")
    {
        system.setInt("source/data", 123);
        bool moved = system.moveNode("source/data", "destination/data");
        int movedValue = 0;
        bool gotMoved = system.getInt("destination/data", movedValue);

        REQUIRE(moved == true);
        REQUIRE(gotMoved == true);
        REQUIRE(movedValue == 123);
        REQUIRE(system.hasNode("source/data") == false);
    }

    SECTION("获取子节点列表")
    {
        system.setInt("parent/child1", 1);
        system.setInt("parent/child2", 2);
        system.setInt("parent/child3", 3);

        auto children = system.getChildNames("parent");
        REQUIRE(children.size() == 3);
    }

    SECTION("遍历子节点")
    {
        system.setInt("parent/child1", 1);
        system.setInt("parent/child2", 2);

        int childCount = 0;
        system.forEachChild("parent", [&](const std::string& name, BaseNode* node)
            {
                childCount++;
            });

        REQUIRE(childCount == 2);
    }
}

TEST_CASE("高级功能测试", "[StatePath][Advanced]")
{
    StatePath system;

    SECTION("值更新不创建节点")
    {
        system.setInt("advanced/value", 10);
        bool updated = system.TrySetIntValue("advanced/value", 20);
        int finalValue = 0;
        system.getInt("advanced/value", finalValue);

        REQUIRE(updated == true);
        REQUIRE(finalValue == 20);
    }

    SECTION("不存在节点值更新失败")
    {
        bool updateFailed = system.TrySetIntValue("advanced/nonexistent", 30);
        REQUIRE(updateFailed == false);
    }

    SECTION("指针节点操作")
    {
        int dummyData = 100;
        system.setPointer("advanced/pointer", &dummyData);
        void* ptr = nullptr;
        bool gotPointer = system.getPointer("advanced/pointer", ptr);

        REQUIRE(gotPointer == true);
        REQUIRE(ptr == &dummyData);
    }

    SECTION("节点访问器")
    {
        system.setInt("advanced/value", 42);
        auto accessor = system["advanced/value"];

        REQUIRE(accessor.exists() == true);
        REQUIRE(accessor.type() == NodeType::INT);
    }

    SECTION("事件启用禁用")
    {
        int eventCount = 0;
        auto listenerId = system.addEventListener("advanced", ListenGranularity::DIRECT_CHILD,
            EventType::ADD, [&](const PathEvent& event) { eventCount++; });

        system.setEventEnabled(false);
        system.setInt("advanced/temp", 1);
        REQUIRE(eventCount == 0); // 事件被禁用，不应触发
        system.removeNode("advanced/temp");

        system.setEventEnabled(true);
        system.setInt("advanced/temp2", 2);
        REQUIRE(eventCount > 0); // 事件启用后应触发

        system.removeEventListener(listenerId);
    }

    SECTION("复杂对象结构")
    {
        system.setObject("complex");
        system.setInt("complex/level1/level2/value", 42);
        system.setString("complex/level1/name", "测试名称");

        REQUIRE(system.hasNode("complex/level1/level2/value") == true);
        REQUIRE(system.hasNode("complex/level1/name") == true);
    }
}

TEST_CASE("事件监听详细测试", "[StatePath][EventDetailed]")
{
    StatePath system;
    std::vector<std::string> eventRecords;

    SECTION("基本事件类型")
    {
        auto listener = system.addEventListener("test/events", ListenGranularity::NODE,
            EventType::ADD, [&](const PathEvent& event)
            {
                eventRecords.push_back("ADD:" + event.path);
            });

        system.setInt("test/events", 100);
        REQUIRE(eventRecords.size() == 1);
        REQUIRE(eventRecords[0] == "ADD:test/events");

        system.removeEventListener(listener);
    }

    SECTION("监听粒度测试")
    {
        auto listener = system.addEventListener("granularity", ListenGranularity::ALL_CHILDREN,
            EventType::ADD, [&](const PathEvent& event)
            {
                eventRecords.push_back(event.path);
            });

        system.setInt("granularity/level1", 1);
        system.setInt("granularity/level1/level2", 2);
        system.setInt("other/branch", 3);

        REQUIRE(eventRecords.size() == 2);
        REQUIRE(std::find(eventRecords.begin(), eventRecords.end(), "granularity/level1") != eventRecords.end());
        REQUIRE(std::find(eventRecords.begin(), eventRecords.end(), "granularity/level1/level2") != eventRecords.end());

        system.removeEventListener(listener);
    }
}

TEST_CASE("性能测试", "[StatePath][Performance]")
{
    StatePath system;
    const int NUM_OPERATIONS = 100;
    int eventCount = 0;

    auto listener = system.addEventListener("perf", ListenGranularity::ALL_CHILDREN,
        EventType::ADD, [&](const PathEvent& event) { eventCount++; });

    SECTION("批量操作性能")
    {
        for (int i = 0; i < NUM_OPERATIONS; i++)
        {
            system.setInt("perf/operation" + std::to_string(i), i);
        }

        REQUIRE(eventCount == NUM_OPERATIONS);
    }

    system.removeEventListener(listener);
}