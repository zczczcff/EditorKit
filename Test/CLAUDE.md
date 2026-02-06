[根目录](../CLAUDE.md) > **Test**

# EditorKit测试套件

## 模块职责

Test模块是EditorKit的测试套件，包含所有核心组件的单元测试和集成测试。使用Catch2测试框架，确保代码质量和功能正确性。

## 入口与启动

### 测试入口文件
- `Src/EventBusTest.cpp` - 事件总线功能测试
- `Src/ActionSystem-EventBus_Contrast.cpp` - 动作系统与事件总线对比测试
- `Src/StatePathTest.cpp` - 状态路径系统测试
- `Src/StaticString-EventBus_Test.cpp` - 静态字符串与事件总线集成测试

### CMake配置
```cmake
# 测试项目配置
cmake_minimum_required(VERSION 3.10)
project(EditorKitTest LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)

# 获取Catch2测试框架
include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.4.0
)
FetchContent_MakeAvailable(Catch2)

# 添加EditorKit依赖
add_subdirectory(../../EditorKit ${CMAKE_CURRENT_BINARY_DIR}/EditorKit)

# 创建测试可执行文件
add_executable(EditorKitTest ${TEST_SOURCES})
target_link_libraries(EditorKitTest PRIVATE EditorKit Catch2::Catch2WithMain)
```

### 运行测试
```bash
# 构建测试
cd Test
mkdir build && cd build
cmake ..
cmake --build .

# 运行所有测试
ctest --output-on-failure

# 运行特定测试
./bin/EditorKitTest "[EventBus]"
```

## 对外接口

### 测试用例结构
```cpp
TEST_CASE("测试描述", "[标签]") {
    // 测试设置

    SECTION("测试部分") {
        // 具体测试逻辑
        REQUIRE(condition);
    }
}
```

### 测试标签系统
- `[EventBus]` - 事件总线相关测试
- `[ActionSystem]` - 动作系统相关测试
- `[StatePath]` - 状态路径系统测试
- `[StaticString]` - 静态字符串测试
- `[Integration]` - 集成测试

## 关键依赖与配置

### 外部依赖
- **Catch2 v3.4.0**: 测试框架
- **EditorKit**: 被测试的核心库

### 测试数据模型

#### 事件总线测试数据
```cpp
enum class TestEventType {
    EVENT_A,
    EVENT_B,
    EVENT_C
};

struct TestData {
    int id;
    std::string name;
    double value;
};
```

#### 自定义哈希函数
```cpp
struct TestEventTypeHash {
    std::size_t operator()(const TestEventType& type) const {
        return static_cast<std::size_t>(type);
    }
};
```

## 测试策略

### 测试覆盖范围

#### 事件总线测试 (`EventBusTest.cpp`)
- 基本订阅和发布功能
- 多播和单播模式
- 参数类型匹配
- 一次性订阅
- 取消订阅功能
- 自定义键类型支持
- 线程安全测试

#### 动作系统对比测试 (`ActionSystem-EventBus_Contrast.cpp`)
- 动作系统与事件总线功能对比
- 验证器、处理器、监听器测试
- 参数重载支持
- 执行结果验证

#### 状态路径测试 (`StatePathTest.cpp`)
- 节点创建和访问
- 值设置和获取
- 事件监听功能
- 树形结构操作
- JSON初始化支持

#### 静态字符串集成测试 (`StaticString-EventBus_Test.cpp`)
- 静态字符串作为事件键
- 哈希性能测试
- 与事件总线集成

### 测试质量保证

#### 断言类型
- `REQUIRE`: 失败时终止测试
- `CHECK`: 失败时继续测试
- `REQUIRE_FALSE`/`CHECK_FALSE`: 反向断言

#### 测试夹具
```cpp
TEST_CASE("测试", "[标签]") {
    // 每个测试用例独立设置
    EventBus<std::string> bus;

    SECTION("部分1") {
        // 测试逻辑
    }

    SECTION("部分2") {
        // 另一个独立测试
    }
}
```

#### 性能测试
- 事件发布性能
- 静态字符串哈希性能
- 状态路径访问性能

## 常见问题 (FAQ)

### Q: 如何添加新的测试？
**A**: 在`Src/`目录下创建新的`.cpp`文件，使用Catch2测试宏，确保包含正确的头文件。

### Q: 测试失败如何调试？
**A**: 使用`ctest --output-on-failure`查看详细输出，或直接运行可执行文件`./bin/EditorKitTest [测试标签]`。

### Q: 如何测试多线程场景？
**A**: 使用`std::thread`创建并发测试，注意同步和数据竞争问题。

### Q: 测试数据如何管理？
**A**: 每个测试用例应该独立设置测试数据，避免测试间的相互影响。

### Q: 如何测试异常情况？
**A**: 使用`REQUIRE_THROWS`、`REQUIRE_THROWS_AS`、`REQUIRE_NOTHROW`等异常断言。

## 相关文件清单

### 测试源文件
1. `Src/EventBusTest.cpp` - 事件总线全面测试
2. `Src/ActionSystem-EventBus_Contrast.cpp` - 动作系统功能测试和对比
3. `Src/StatePathTest.cpp` - 状态路径系统测试
4. `Src/StaticString-EventBus_Test.cpp` - 静态字符串集成测试

### 配置文件
1. `CMakeLists.txt` - 测试项目构建配置

### 构建输出
1. `build/` - 构建目录（在.gitignore中）
2. `bin/EditorKitTest` - 测试可执行文件
3. `TestResults/` - 测试结果输出

## 变更记录 (Changelog)

### 2026-02-05 - AI上下文初始化
- 创建测试模块CLAUDE.md文档
- 记录测试策略和用例结构
- 添加测试运行指南

### 测试开发指南
- 新功能必须包含相应的单元测试
- 测试代码与产品代码同等重要
- 保持测试的独立性和可重复性
- 定期运行测试确保回归