#include <iostream>
#include <string>
#include <memory>
#include "EditorKit/StatePath.h"

class StatePathTest
{
private:
    StatePath system;
    int testCount = 0;
    int passedCount = 0;

public:
    void runAllTests()
    {
        std::cout << "=== 开始测试路径状态系统 ===\n" << std::endl;

        testBasicOperations();
        testEventSystem();
        testNodeTypes();
        testPathOperations();
        testAdvancedFeatures();

        std::cout << "\n=== 测试完成 ===" << std::endl;
        std::cout << "总测试数: " << testCount << std::endl;
        std::cout << "通过数: " << passedCount << std::endl;
        std::cout << "通过率: " << (passedCount * 100.0 / testCount) << "%" << std::endl;
    }

private:
    void startTest(const std::string& testName)
    {
        std::cout << "\n🔧 测试: " << testName << std::endl;
        testCount++;
    }

    void checkResult(bool success, const std::string& message)
    {
        if (success)
        {
            std::cout << "测试通过： " << message << std::endl;
            passedCount++;
        }
        else
        {
            std::cout << "测试失败： " << message << std::endl;
        }
    }

    void testBasicOperations()
    {
        startTest("基础操作测试");

        // 测试设置和获取整型值
        system.setInt("config/width", 1920);
        int width = 0;
        bool gotWidth = system.getInt("config/width", width);
        checkResult(gotWidth && width == 1920, "设置和获取整型值");

        // 测试设置和获取浮点值
        system.setFloat("config/ratio", 1.618f);
        float ratio = 0;
        bool gotRatio = system.getFloat("config/ratio", ratio);
        checkResult(gotRatio && ratio > 1.617f && ratio < 1.619f, "设置和获取浮点值");

        // 测试设置和获取布尔值
        system.setBool("config/enabled", true);
        bool enabled = false;
        bool gotEnabled = system.getBool("config/enabled", enabled);
        checkResult(gotEnabled && enabled, "设置和获取布尔值");

        // 测试设置和获取字符串
        system.setString("config/name", "测试系统");
        std::string name;
        bool gotName = system.getString("config/name", name);
        checkResult(gotName && name == "测试系统", "设置和获取字符串");

        // 测试对象节点
        system.setObject("config/display");
        checkResult(system.hasNode("config/display"), "创建对象节点");

        // 测试操作符重载
        system["config/height"] = 1080;
        int height = 0;
        bool gotHeight = system["config/height"].get(height);
        checkResult(gotHeight && height == 1080, "操作符重载设置和获取");
    }

    void testEventSystem()
    {
        startTest("事件系统测试");

        int eventCount = 0;
        std::string lastEventPath;

        // 添加事件监听器
        auto listenerId = system.addEventListener("config", ListenGranularity::ALL_CHILDREN,
            EventType::ADD, [&](const PathEvent& event)
            {
                eventCount++;
                lastEventPath = event.path;
                std::cout << "    事件触发: ADD, 路径: " << event.path << std::endl;
            });

        // 触发添加事件
        system.setInt("config/newSetting", 42);
        checkResult(eventCount > 0 && lastEventPath == "config/newSetting", "ADD事件触发");

        // 测试更新事件
        eventCount = 0;
        system.addEventListener("config/width", ListenGranularity::NODE,
            EventType::UPDATE, [&](const PathEvent& event)
            {
                eventCount++;
                std::cout << "    事件触发: UPDATE, 路径: " << event.path << std::endl;
            });

        system.setInt("config/width", 2048);
        checkResult(eventCount > 0, "UPDATE事件触发");

        // 测试删除事件
        eventCount = 0;
        system.addEventListener("config/width", ListenGranularity::NODE,
            EventType::REMOVE, [&](const PathEvent& event)
            {
                eventCount++;
                std::cout << "    事件触发: REMOVE, 路径: " << event.path << std::endl;
            });

        system.removeNode("config/width");
        checkResult(eventCount > 0, "REMOVE事件触发");

        // 移除事件监听器
        bool removed = system.removeEventListener(listenerId);
        checkResult(removed, "移除事件监听器");
    }

    void testNodeTypes()
    {
        startTest("节点类型测试");

        // 测试各种节点类型
        system.setInt("types/int", 100);
        system.setFloat("types/float", 3.14f);
        system.setBool("types/bool", true);
        system.setString("types/string", "hello");
        system.setObject("types/object");

        checkResult(system.getNodeType("types/int") == NodeType::INT, "整型节点类型");
        checkResult(system.getNodeType("types/float") == NodeType::FLOAT, "浮点节点类型");
        checkResult(system.getNodeType("types/bool") == NodeType::BOOL, "布尔节点类型");
        checkResult(system.getNodeType("types/string") == NodeType::STRING, "字符串节点类型");
        checkResult(system.getNodeType("types/object") == NodeType::OBJECT, "对象节点类型");
        checkResult(system.getNodeType("types/nonexistent") == NodeType::EMPTY, "不存在节点类型");

        // 测试模板值获取
        std::string strValue;
        bool gotString = system.getValue<std::string>("types/string", strValue);
        checkResult(gotString && strValue == "hello", "模板值获取");
    }

    void testPathOperations()
    {
        startTest("路径操作测试");

        // 测试嵌套路径创建
        system.setInt("deep/nested/path/value", 999);
        int deepValue = 0;
        bool gotDeep = system.getInt("deep/nested/path/value", deepValue);
        checkResult(gotDeep && deepValue == 999, "嵌套路径创建");

        // 测试节点存在性检查
        checkResult(system.hasNode("deep/nested/path/value"), "节点存在性检查");
        checkResult(!system.hasNode("deep/nonexistent"), "不存在节点检查");

        // 测试移动节点
        system.setInt("source/data", 123);
        bool moved = system.moveNode("source/data", "destination/data");
        int movedValue = 0;
        bool gotMoved = system.getInt("destination/data", movedValue);
        checkResult(moved && gotMoved && movedValue == 123 && !system.hasNode("source/data"), "移动节点");

        // 测试获取子节点列表
        system.setInt("parent/child1", 1);
        system.setInt("parent/child2", 2);
        system.setInt("parent/child3", 3);

        auto children = system.getChildNames("parent");
        checkResult(children.size() == 3, "获取子节点列表");

        // 测试遍历子节点
        int childCount = 0;
        system.forEachChild("parent", [&](const std::string& name, BaseNode* node)
            {
                childCount++;
                std::cout << "    子节点: " << name << std::endl;
            });
        checkResult(childCount == 3, "遍历子节点");
    }

    void testAdvancedFeatures()
    {
        startTest("高级功能测试");

        // 测试值更新（不创建新节点）
        system.setInt("advanced/value", 10);
        bool updated = system.TrySetIntValue("advanced/value", 20);
        int finalValue = 0;
        system.getInt("advanced/value", finalValue);
        checkResult(updated && finalValue == 20, "值更新不创建节点");

        // 测试不存在的节点值更新
        bool updateFailed = system.TrySetIntValue("advanced/nonexistent", 30);
        checkResult(!updateFailed, "不存在节点值更新失败");

        // 测试指针节点
        int dummyData = 100;
        system.setPointer("advanced/pointer", &dummyData);
        void* ptr = nullptr;
        bool gotPointer = system.getPointer("advanced/pointer", ptr);
        checkResult(gotPointer && ptr == &dummyData, "指针节点操作");

        // 测试节点访问器
        auto accessor = system["advanced/value"];
        checkResult(accessor.exists() && accessor.type() == NodeType::INT, "节点访问器");

        // 测试事件启用/禁用
        system.setEventEnabled(false);
        system.setInt("advanced/temp", 1); // 不应触发事件
        system.setEventEnabled(true);

        // 测试树形结构打印（视觉检查）
        std::cout << "    树形结构打印:" << std::endl;
        std::cout << system.printTree() << std::endl;;
        checkResult(true, "树形结构打印（请人工检查）");

        // 测试复杂对象结构
        system.setObject("complex");
        system.setInt("complex/level1/level2/value", 42);
        system.setString("complex/level1/name", "测试名称");

        checkResult(system.hasNode("complex/level1/level2/value"), "复杂对象结构");
    }
};
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <algorithm>
// 演示用例
void demonstrateUsage()
{
    std::cout << "\n=== 使用示例演示 ===" << std::endl;

    StatePath sys;

    // 配置系统参数
    sys["app/config/width"] = 1920;
    sys["app/config/height"] = 1080;
    sys["app/config/title"] = "我的应用";
    sys["app/config/fullscreen"] = true;

    // 读取配置
    int width, height;
    std::string title;
    bool fullscreen;

    sys["app/config/width"].get(width);
    sys["app/config/height"].get(height);
    sys["app/config/title"].get(title);
    sys["app/config/fullscreen"].get(fullscreen);

    std::cout << "应用配置: " << width << "x" << height
        << ", 标题: " << title
        << ", 全屏: " << (fullscreen ? "是" : "否") << std::endl;

    // 添加配置变化监听
    sys.addEventListener("app/config", ListenGranularity::ALL_CHILDREN,
        EventType::UPDATE, [](const PathEvent& event)
        {
            std::cout << "配置已更新: " << event.path << std::endl;
        });

    // 更新配置（会触发事件）
    sys["app/config/width"] = 2560;
}

class EventListenerTest
{
private:
    StatePath system;
    int testCount = 0;
    int passedCount = 0;

    // 记录事件触发的详细信息
    struct EventRecord
    {
        EventType type;
        std::string path;
        std::string relatedPath;
        NodeType nodeType;
    };
    std::vector<EventRecord> eventRecords;

    // 事件计数器
    std::map<EventType, int> eventCounts;
    std::map<std::string, int> pathEventCounts;

public:
    void runAllEventTests()
    {
        std::cout << "=== 开始详细测试事件监听功能 ===\n" << std::endl;

        clearEventRecords();

        testBasicEventTypes();
        testListenGranularity();
        testEventParameters();
        testEventManagerOperations();
        testComplexEventScenarios();
        testEventPerformance();

        std::cout << "\n=== 事件监听测试完成 ===" << std::endl;
        std::cout << "总测试数: " << testCount << std::endl;
        std::cout << "通过数: " << passedCount << std::endl;
        std::cout << "通过率: " << (passedCount * 100.0 / testCount) << "%" << std::endl;

        printEventStatistics();
    }

private:
    void startTest(const std::string& testName)
    {
        std::cout << "\n🔔 事件测试: " << testName << std::endl;
        testCount++;
    }

    void checkResult(bool success, const std::string& message)
    {
        if (success)
        {
            std::cout << "测试通过： " << message << std::endl;
            passedCount++;
        }
        else
        {
            std::cout << "测试失败： " << message << std::endl;
        }
    }

    void clearEventRecords()
    {
        eventRecords.clear();
        eventCounts.clear();
        pathEventCounts.clear();
    }

    // 通用事件回调函数
    EventCallback createEventCallback(const std::string& listenerName = "")
    {
        return [this, listenerName](const PathEvent& event)
        {
            EventRecord record{ event.type, event.path, event.relatedPath, event.nodeType };
            eventRecords.push_back(record);

            eventCounts[event.type]++;
            pathEventCounts[event.path]++;

            std::cout << "    📢 事件触发 [" << listenerName << "]: "
                << eventTypeToString(event.type) << " path=" << event.path;
            if (!event.relatedPath.empty())
            {
                std::cout << " related=" << event.relatedPath;
            }
            std::cout << " nodeType=" << nodeTypeToString(event.nodeType) << std::endl;
        };
    }

    std::string eventTypeToString(EventType type)
    {
        switch (type)
        {
        case EventType::ADD: return "ADD";
        case EventType::REMOVE: return "REMOVE";
        case EventType::MOVE: return "MOVE";
        case EventType::UPDATE: return "UPDATE";
        default: return "UNKNOWN";
        }
    }

    std::string nodeTypeToString(NodeType type)
    {
        switch (type)
        {
        case NodeType::OBJECT: return "OBJECT";
        case NodeType::INT: return "INT";
        case NodeType::FLOAT: return "FLOAT";
        case NodeType::BOOL: return "BOOL";
        case NodeType::POINTER: return "POINTER";
        case NodeType::STRING: return "STRING";
        case NodeType::EMPTY: return "EMPTY";
        default: return "UNKNOWN";
        }
    }

    void testBasicEventTypes()
    {
        startTest("基本事件类型测试");

        // 测试ADD事件
        auto addListener = system.addEventListener("test/add/value1", ListenGranularity::NODE,
            EventType::ADD, createEventCallback("ADD监听器"));

        system.setInt("test/add/value1", 100);
        checkResult(eventCounts[EventType::ADD] == 1, "ADD事件触发");
        checkResult(pathEventCounts["test/add/value1"] == 1, "正确路径的ADD事件");

        // 测试UPDATE事件
        system.addEventListener("test/add/value1", ListenGranularity::NODE,
            EventType::UPDATE, createEventCallback("UPDATE监听器"));

        system.setInt("test/add/value1", 200);
        checkResult(eventCounts[EventType::UPDATE] == 1, "UPDATE事件触发");

        // 测试REMOVE事件
        system.addEventListener("test/add/value1", ListenGranularity::NODE,
            EventType::REMOVE, createEventCallback("REMOVE监听器"));

        system.removeNode("test/add/value1");
        checkResult(eventCounts[EventType::REMOVE] == 1, "REMOVE事件触发");

        // 测试MOVE事件
        system.setInt("test/move/source", 300);
        system.addEventListener("test/move/source", ListenGranularity::NODE,
            EventType::MOVE, createEventCallback("MOVE监听器"));

        system.moveNode("test/move/source", "test/move/destination");
        checkResult(eventCounts[EventType::MOVE] == 1, "MOVE事件触发");

        // 验证MOVE事件参数
        bool hasMoveEvent = false;
        for (const auto& record : eventRecords)
        {
            if (record.type == EventType::MOVE)
            {
                hasMoveEvent = (record.path == "test/move/source" &&
                    record.relatedPath == "test/move/destination");
                break;
            }
        }
        checkResult(hasMoveEvent, "MOVE事件包含正确的路径参数");

        system.removeEventListener(addListener);
    }

    void testListenGranularity()
    {
        startTest("监听粒度测试");
        clearEventRecords();

        // NODE粒度测试 - 只监听特定节点
        auto nodeListener = system.addEventListener("granularity/node", ListenGranularity::NODE,
            EventType::ADD, createEventCallback("NODE粒度"));

        system.setInt("granularity/node", 1);        // 应该触发
        system.setInt("granularity/node/child", 2);  // 不应该触发
        system.setInt("granularity/other", 3);       // 不应该触发

        checkResult(pathEventCounts["granularity/node"] == 1, "NODE粒度只监听精确路径");
        checkResult(pathEventCounts.count("granularity/node/child") == 0, "NODE粒度不监听子节点");

        // DIRECT_CHILD粒度测试
        clearEventRecords();
        system.removeEventListener(nodeListener);

        auto directChildListener = system.addEventListener("granularity/parent", ListenGranularity::DIRECT_CHILD,
            EventType::ADD, createEventCallback("DIRECT_CHILD粒度"));

        system.setInt("granularity/parent/child1", 1);    // 应该触发
        system.setInt("granularity/parent/child2", 2);    // 应该触发
        system.setInt("granularity/parent/child1/grandchild", 3);  // 不应该触发
        system.setInt("granularity/other", 4);           // 不应该触发

        checkResult(pathEventCounts["granularity/parent/child1"] == 1, "DIRECT_CHILD监听直接子节点");
        checkResult(pathEventCounts["granularity/parent/child2"] == 1, "DIRECT_CHILD监听多个直接子节点");
        checkResult(pathEventCounts.count("granularity/parent/child1/grandchild") == 0, "DIRECT_CHILD不监听孙子节点");

        // ALL_CHILDREN粒度测试
        clearEventRecords();
        system.removeEventListener(directChildListener);

        auto allChildrenListener = system.addEventListener("granularity", ListenGranularity::ALL_CHILDREN,
            EventType::ADD, createEventCallback("ALL_CHILDREN粒度"));

        system.setInt("granularity/level1", 1);           // 应该触发
        system.setInt("granularity/level1/level2", 2);    // 应该触发
        system.setInt("granularity/level1/level2/level3", 3);  // 应该触发
        system.setInt("other/branch", 4);                // 不应该触发

        checkResult(pathEventCounts["granularity/level1"] == 1, "ALL_CHILDREN监听一级子节点");
        checkResult(pathEventCounts["granularity/level1/level2"] == 1, "ALL_CHILDREN监听二级子节点");
        checkResult(pathEventCounts["granularity/level1/level2/level3"] == 1, "ALL_CHILDREN监听多级子节点");
        checkResult(pathEventCounts.count("other/branch") == 0, "ALL_CHILDREN不监听其他分支");

        system.removeEventListener(allChildrenListener);
    }

    void testEventParameters()
    {
        startTest("事件参数测试");
        clearEventRecords();

        // 测试节点类型信息
        auto listener = system.addEventListener("params", ListenGranularity::ALL_CHILDREN,
            EventType::ADD, [this](const PathEvent& event)
            {
                eventRecords.push_back({ event.type, event.path, event.relatedPath, event.nodeType });

                // 验证节点类型正确性
                std::cout << "    🔍 事件节点类型: " << nodeTypeToString(event.nodeType);
                if (event.node)
                {
                    std::cout << " (实际类型: " << nodeTypeToString(event.node->getType()) << ")";
                }
                std::cout << std::endl;
            });

        system.setInt("params/intValue", 42);
        system.setFloat("params/floatValue", 3.14f);
        system.setBool("params/boolValue", true);
        system.setString("params/stringValue", "test");
        system.setObject("params/objectValue");

        // 验证节点类型正确性
        bool intTypeCorrect = false, floatTypeCorrect = false, boolTypeCorrect = false,
            stringTypeCorrect = false, objectTypeCorrect = false;

        for (const auto& record : eventRecords)
        {
            if (record.path == "params/intValue" && record.nodeType == NodeType::INT) intTypeCorrect = true;
            if (record.path == "params/floatValue" && record.nodeType == NodeType::FLOAT) floatTypeCorrect = true;
            if (record.path == "params/boolValue" && record.nodeType == NodeType::BOOL) boolTypeCorrect = true;
            if (record.path == "params/stringValue" && record.nodeType == NodeType::STRING) stringTypeCorrect = true;
            if (record.path == "params/objectValue" && record.nodeType == NodeType::OBJECT) objectTypeCorrect = true;
        }

        checkResult(intTypeCorrect, "INT类型节点事件参数正确");
        checkResult(floatTypeCorrect, "FLOAT类型节点事件参数正确");
        checkResult(boolTypeCorrect, "BOOL类型节点事件参数正确");
        checkResult(stringTypeCorrect, "STRING类型节点事件参数正确");
        checkResult(objectTypeCorrect, "OBJECT类型节点事件参数正确");

        system.removeEventListener(listener);
    }

    void testEventManagerOperations()
    {
        startTest("事件管理器操作测试");
        clearEventRecords();

        // 测试添加多个监听器
        std::vector<ListenerId> listenerIds;

        listenerIds.push_back(system.addEventListener("manager", ListenGranularity::NODE,
            EventType::ADD, createEventCallback("监听器1")));
        listenerIds.push_back(system.addEventListener("manager", ListenGranularity::NODE,
            EventType::ADD, createEventCallback("监听器2")));
        listenerIds.push_back(system.addEventListener("manager", ListenGranularity::NODE,
            EventType::UPDATE, createEventCallback("监听器3")));

        system.setInt("manager", 1);
        checkResult(eventCounts[EventType::ADD] == 2, "多个ADD监听器同时触发");

        system.setInt("manager", 2);
        checkResult(eventCounts[EventType::UPDATE] == 1, "UPDATE监听器正确触发");

        // 测试移除监听器
        bool removeResult = system.removeEventListener(listenerIds[0]);
        checkResult(removeResult, "成功移除监听器");

        clearEventRecords();
        system.removeNode("manager");
        system.setInt("manager", 3);
        system.setInt("manager", 4);
        checkResult(eventCounts[EventType::UPDATE] == 1 && eventCounts[EventType::ADD] == 1,
            "移除监听器后只有剩余监听器触发");

        // 测试移除不存在的监听器
        bool removeInvalid = system.removeEventListener(9999);
        checkResult(!removeInvalid, "移除不存在的监听器返回false");

        // 清理剩余监听器
        for (size_t i = 1; i < listenerIds.size(); i++)
        {
            system.removeEventListener(listenerIds[i]);
        }
    }

    void testComplexEventScenarios()
    {
        startTest("复杂事件场景测试");
        clearEventRecords();

        // 场景1: 嵌套对象操作
        auto scene1Listener = system.addEventListener("scene1", ListenGranularity::ALL_CHILDREN,
            EventType::ADD, createEventCallback("场景1"));

        system.setObject("scene1");
        system.setInt("scene1/level1", 1);
        system.setObject("scene1/level1/level2");
        system.setString("scene1/level1/level2/name", "nested");

        checkResult(pathEventCounts.size() == 4, "嵌套对象创建触发多个事件");

        system.removeEventListener(scene1Listener);

        // 场景2: 批量操作事件顺序
        clearEventRecords();
        auto batchListener = system.addEventListener("batch", ListenGranularity::ALL_CHILDREN,
            EventType::ADD, createEventCallback("批量操作"));

        system.setEventEnabled(false);
        system.setInt("batch/value1", 1);
        system.setInt("batch/value2", 2);
        system.setInt("batch/value3", 3);
        system.setEventEnabled(true);

        checkResult(eventRecords.empty(), "禁用事件时无事件触发");

        system.setInt("batch/value4", 4);
        checkResult(eventRecords.size() == 1, "重新启用事件后事件正常触发");

        system.removeEventListener(batchListener);

        // 场景3: 事件循环检测（节点在事件回调中修改自身）
        clearEventRecords();
        bool recursionDetected = false;

        auto recursiveListener = system.addEventListener("recursive", ListenGranularity::NODE,
            EventType::UPDATE, [this, &recursionDetected](const PathEvent& event)
            {
                eventRecords.push_back({ event.type, event.path, "", event.nodeType });

                // 在回调中再次修改同一个节点（可能造成循环）
                static int count = 0;
                if (count < 3)
                {
                    count++;
                    system.setInt("recursive", count * 10);
                    std::cout << "    🔁 递归修改: " << event.path << " -> " << (count * 10) << std::endl;
                }
                else
                {
                    recursionDetected = true;
                }
            });

        system.setInt("recursive", 1);
        checkResult(recursionDetected, "事件循环场景处理");

        system.removeEventListener(recursiveListener);
    }

    void testEventPerformance()
    {
        startTest("事件性能测试");
        clearEventRecords();

        const int NUM_LISTENERS = 5;
        const int NUM_OPERATIONS = 10;

        std::vector<ListenerId> listeners;

        // 添加多个监听器
        for (int i = 0; i < NUM_LISTENERS; i++)
        {
            listeners.push_back(system.addEventListener("perf", ListenGranularity::ALL_CHILDREN,
                EventType::ADD, createEventCallback("性能监听器" + std::to_string(i))));
        }

        // 执行批量操作
        auto startTime = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < NUM_OPERATIONS; i++)
        {
            system.setInt("perf/operation" + std::to_string(i), i);
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);

        int totalEvents = NUM_OPERATIONS * NUM_LISTENERS;
        checkResult(eventRecords.size() == totalEvents, "性能测试事件数量正确: " +
            std::to_string(eventRecords.size()) + "/" + std::to_string(totalEvents));

        std::cout << "    ⏱️  性能数据: " << NUM_OPERATIONS << " 次操作, "
            << NUM_LISTENERS << " 个监听器, 耗时: " << duration.count() << " 微秒" << std::endl;
        std::cout << "    📊 平均每个事件: " << (duration.count() / (double)totalEvents) << " 微秒" << std::endl;

        // 清理监听器
        for (auto id : listeners)
        {
            system.removeEventListener(id);
        }
    }

    void printEventStatistics()
    {
        std::cout << "\n=== 事件统计 ===" << std::endl;
        std::cout << "总事件触发次数: " << eventRecords.size() << std::endl;

        std::map<EventType, int> typeSummary;
        std::map<std::string, int> pathSummary;

        for (const auto& record : eventRecords)
        {
            typeSummary[record.type]++;
            pathSummary[record.path]++;
        }

        std::cout << "按事件类型统计:" << std::endl;
        for (const auto& [type, count] : typeSummary)
        {
            std::cout << "  " << eventTypeToString(type) << ": " << count << " 次" << std::endl;
        }

        std::cout << "最活跃的事件路径 (前5个):" << std::endl;
        std::vector<std::pair<std::string, int>> sortedPaths(pathSummary.begin(), pathSummary.end());
        std::sort(sortedPaths.begin(), sortedPaths.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });

        for (int i = 0; i < std::min(5, (int)sortedPaths.size()); i++)
        {
            std::cout << "  " << sortedPaths[i].first << ": " << sortedPaths[i].second << " 次" << std::endl;
        }
    }
};

// 演示事件系统的实际应用场景
void demonstrateEventUsage()
{
    std::cout << "\n=== 事件系统使用演示 ===" << std::endl;

    StatePath sys;

    // 场景: 配置热重载系统
    std::cout << "🔧 配置热重载演示:" << std::endl;

    // 监听配置变化
    sys.addEventListener("app/config", ListenGranularity::ALL_CHILDREN,
        EventType::UPDATE, [](const PathEvent& event)
        {
            std::cout << "    🔄 配置已更新: " << event.path << " - 重新加载相关模块" << std::endl;
        });

    // 监听新配置添加
    sys.addEventListener("app/config", ListenGranularity::ALL_CHILDREN,
        EventType::ADD, [](const PathEvent& event)
        {
            std::cout << "    ➕ 新配置添加: " << event.path << " - 初始化新功能" << std::endl;
        });

    // 模拟配置变化
    sys.setInt("app/config/width", 1920);
    sys.setInt("app/config/height", 1080);
    sys.setString("app/config/title", "我的应用");

    // 更新配置触发事件
    sys.setInt("app/config/width", 2560);
    sys.setBool("app/config/fullscreen", true);
}

void JsonValueTest()
{
    // 创建状态路径系统
    StatePath stateSystem;

    // 使用 JSON 风格初始化 - 现在这是类型安全的
    stateSystem("Editor") = JsonValue{
        {"GlobalProperty", JsonValue{
            {"Color", "blue"},
            {"Size", 10},
            {"Visible", true}
        }},
        {"PageCount", 3},
        {"Settings", JsonValue{
            {"AutoSave", true},
            {"Theme", "dark"}
        }}
    };

    // 或者更简洁的写法（如果编译器支持推导）
    stateSystem("App") = {
        {"Window", {
            {"Width", 1024},
            {"Height", 768},
            {"Title", "My Application"}
        }},
        {"Version", "1.0.0"}
    };
    std::cout << stateSystem.printTree() << std::endl;
    // 使用 ObjectNode 直接初始化
    ObjectNode* editorNode = stateSystem.getNode("Editor")->AsObjectNode();
    if (editorNode)
    {
        editorNode->initialize({
            {"GlobalProperty", {
                {"Color", "red"},
                {"Size", 15}
            }},
            {"NewPageCount", 5}
            });
    }
}

int main()
{
    // 运行测试
    StatePathTest tester;
    tester.runAllTests();

    // 演示使用示例
    demonstrateUsage();

    EventListenerTest eventTester;
    eventTester.runAllEventTests();

    // 演示事件系统的实际应用
    demonstrateEventUsage();

    StatePath stateSystem;

    // 设置值
    stateSystem.setInt("root/player/health", 100);
    stateSystem.setString("root/player/name", "John");
    stateSystem["root"]["player"]["name"]="John";
    // 获取节点并进行路径操作
    BaseNode* playerNode = stateSystem.getNode("root/player");
    if (ObjectNode* objPlayerNode = playerNode->AsObjectNode())
    {
        // 方法1：使用相对路径操作接口
        objPlayerNode->setInt("level", 5);           // 设置 "root/player/level"
        objPlayerNode->setFloat("position/x", 10.5f); // 设置 "root/player/position/x"

        // 方法2：使用[]操作符（推荐）
        (*objPlayerNode)["level"] = 5;
        (*objPlayerNode)["position/x"] = 10.5f;
        (*objPlayerNode)["name"] = "John Doe";
        (*objPlayerNode)["alive"] = true;

        // 链式[]操作符支持
        (*objPlayerNode)["position"]["x"] = 15.0f;
        (*objPlayerNode)["inventory"]["weapon"]["damage"] = 50;

        // 获取值
        int level = (*objPlayerNode)["level"].GetIntValue();           // 类型转换
        float posX = (*objPlayerNode)["position/x"].GetFloatValue();
        std::string name = (*objPlayerNode)["name"].GetStringValue();

        // 检查节点是否存在
        if ((*objPlayerNode)["level"].exists())
        {
            std::cout << "Level exists: " << (int)(*objPlayerNode)["level"].GetIntValue() << std::endl;
        }

        // 获取节点类型
        NodeType type = (*objPlayerNode)["level"].type();

        
    }

    JsonValueTest();
    return 0;
}