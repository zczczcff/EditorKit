[根目录](../CLAUDE.md) > **EditorKit**

# EditorKit核心库

## 模块职责

EditorKit核心库提供了一套完整的编辑器基础设施，包括命令模式、事件系统、动作系统、状态管理等核心组件。所有组件都设计为高性能、类型安全且易于扩展。

## 入口与启动

### 主要头文件
- `CommandBase.h` - 命令模式基类
- `CommandManager.h` - 命令管理器
- `ActionSystem.h` - 动作系统
- `KEventBus.h` - 事件总线
- `KDataBus.h` - 数据总线
- `StatePath.h` - 状态路径系统
- `StateNode.h` - 状态节点
- `StatePathListener.h` - 状态路径监听器
- `StaticString.h` - 静态字符串
- `Type_Check.h` - 类型检查工具

### CMake配置
```cmake
# 项目配置
project(EditorKit LANGUAGES CXX C)
set(CMAKE_CXX_STANDARD 17)

# 依赖配置
find_package(Boost QUIET)  # Boost头文件依赖

# 源文件收集
file(GLOB_RECURSE EDITORKIT_SOURCES "EditorKit/*.cpp")
file(GLOB_RECURSE EDITORKIT_HEADERS "EditorKit/*.h")

# 创建静态库
add_library(EditorKit STATIC ${EDITORKIT_SOURCES} ${EDITORKIT_HEADERS})
```

## 对外接口

### 命令系统接口
```cpp
// 命令基类
template<typename DataType>
class CommandBase {
    virtual bool Execute() = 0;
    virtual bool Undo() = 0;
    virtual std::string GetDescription() const = 0;
};

// 命令管理器
template<typename DataType>
class EditCommandManager {
    bool Execute(std::unique_ptr<CommandBase<DataType>> command);
    bool Undo();
    bool Redo();
};
```

### 事件系统接口
```cpp
// 事件总线
template<typename EventKeyType = std::string, typename Hash = std::hash<EventKeyType>>
class EventBus {
    EventID Subscribe(const EventKeyType& eventName, Callable&& handler);
    PublishResult Publish(const EventKeyType& eventName, Args&&... args);
    bool Unsubscribe(const EventID& token);
};
```

### 动作系统接口
```cpp
// 动作系统
template<typename KeyType, bool AllowOverload = false,
         typename Hash = std::hash<KeyType>,
         typename KeyEqual = std::equal_to<KeyType>>
class ActionSystem {
    ActionHandle<KeyType> AddValidator(const KeyType& actionKey, Callable&& validator);
    ActionHandle<KeyType> AddSequentialProcessor(const KeyType& actionKey, Callable&& processor);
    ActionResult Execute(const KeyType& actionKey, Args&&... args);
};
```

### 状态路径接口
```cpp
// 状态路径系统
class StatePath {
    void setInt(const std::string& path, int value);
    void setFloat(const std::string& path, float value);
    bool getInt(const std::string& path, int& outValue);
    bool getFloat(const std::string& path, float& outValue);
    NodeAccessor operator[](const std::string& path);
};
```

### 数据总线接口
```cpp
// 数据总线
template<typename KeyType = std::string, typename Hash = std::hash<KeyType>>
class DataBus {
    DataBusResult RegisterData(const KeyType& key, T* dataPtr);
    DataBusResult GetData(const KeyType& key);
    T* GetDataSafe(const KeyType& key);
};
```

## 关键依赖与配置

### 外部依赖
- **Boost**: 头文件依赖（type_index等）
- **C++17**: 标准库特性依赖

### 内部依赖关系
```
CommandManager -> CommandBase
ActionSystem -> Type_Check
EventBus -> Type_Check
StatePath -> StateNode + StatePathListener
DataBus -> 无内部依赖
StaticString -> 无内部依赖
```

### 编译选项
```cmake
# MSVC
/W4           # 警告级别4
/WX-          # 警告不视为错误
/permissive-  # 标准一致性

# GCC/Clang
-Wall
-Wextra
-Wpedantic
-Wno-unused-parameter
```

## 数据模型

### 命令系统数据模型
```cpp
template<typename DataType>
class CommandBase {
    DataType m_Data;  // 自定义数据类型成员
    std::chrono::steady_clock::time_point m_Timestamp;
};
```

### 状态路径数据模型
```cpp
enum class NodeType {
    OBJECT, INT, FLOAT, BOOL, POINTER, STRING, EMPTY
};

class BaseNode {
    virtual NodeType getType() const = 0;
    virtual std::string getContent() const = 0;
};
```

### 事件系统数据模型
```cpp
struct PublishResult {
    bool success;
    int totalSubscribers;
    int successfulExecutions;
    std::string errorMessage;
};
```

## 测试与质量

### 单元测试位置
- `../Test/Src/EventBusTest.cpp` - 事件总线测试
- `../Test/Src/ActionSystem-EventBus_Contrast.cpp` - 动作系统与事件总线对比
- `../Test/Src/StatePathTest.cpp` - 状态路径测试
- `../Test/Src/StaticString-EventBus_Test.cpp` - 静态字符串集成测试

### 质量保证
1. **类型安全**: 所有模板组件都包含类型检查
2. **异常安全**: 关键操作提供异常保证
3. **内存安全**: 使用智能指针管理资源
4. **线程安全**: 注意并发访问（部分组件需要外部同步）

### 性能考虑
- 事件总线使用完美转发减少拷贝
- 静态字符串使用ID比较提高哈希效率
- 状态路径使用前缀树优化事件监听

## 常见问题 (FAQ)

### Q: 如何选择事件总线还是动作系统？
**A**: 事件总线适合简单的发布-订阅场景，动作系统适合需要验证、处理流程的复杂动作。

### Q: 命令系统的数据类型如何定义？
**A**: `CommandBase`是模板类，可以自定义`DataType`来存储命令所需的数据。

### Q: 状态路径系统的性能如何？
**A**: 使用路径分割和前缀树优化，适合中等规模的状态管理。

### Q: 如何扩展新的节点类型？
**A**: 继承`BaseNode`并实现纯虚函数，在`StatePath`中添加对应的set/get方法。

### Q: 数据总线的类型安全如何保证？
**A**: 使用`type_index`进行运行时类型检查，模板方法提供编译时类型安全。

## 相关文件清单

### 核心头文件
1. `EditorKit/CommandBase.h` - 命令基类
2. `EditorKit/CommandManager.h` - 命令管理器
3. `EditorKit/ActionSystem.h` - 动作系统（1703行）
4. `EditorKit/KEventBus.h` - 事件总线（962行）
5. `EditorKit/KDataBus.h` - 数据总线（331行）
6. `EditorKit/StatePath.h` - 状态路径系统（968行）
7. `EditorKit/StateNode.h` - 状态节点基类
8. `EditorKit/StatePathListener.h` - 状态路径监听器（303行）
9. `EditorKit/StaticString.h` - 静态字符串
10. `EditorKit/Type_Check.h` - 类型检查工具

### 实现文件
1. `EditorKit/StateNode.cpp` - 状态节点实现

### 配置文件
1. `CMakeLists.txt` - CMake构建配置

## 变更记录 (Changelog)

### 2026-02-05 - AI上下文初始化
- 创建模块级CLAUDE.md文档
- 记录所有核心组件的接口和用法
- 添加常见问题解答

### 近期代码变更
- ActionSystem: 添加处理器不再抛出异常，返回无效Handler
- CommandManager: Undo/Redo添加bool返回值
- 链接boost库，移除RTTI&dynamic_cast依赖
- 增加命令模式模板基类
- 修复事件重载参数匹配问题