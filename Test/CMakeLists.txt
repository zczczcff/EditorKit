# 测试项目的独立CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
set(TARGET_NAME EditorKitTest)
project(${TARGET_NAME} LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 通过FetchContent获取Catch2
include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.4.0
)
FetchContent_MakeAvailable(Catch2)

# 通过add_subdirectory添加EditorKit依赖
# 使用硬编码的路径指向EditorKit目录
message(STATUS "添加EditorKit作为子目录: ${CMAKE_CURRENT_SOURCE_DIR}/../../EditorKit")
add_subdirectory(../../EditorKit ${CMAKE_CURRENT_BINARY_DIR}/EditorKit)

# 确认EditorKit目标已定义
if(NOT TARGET EditorKit)
    message(FATAL_ERROR "EditorKit目标未找到！请确保EditorKit/CMakeLists.txt正确定义了EditorKit目标。")
endif()

# 查找所有测试源文件
file(GLOB_RECURSE TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/*.cxx"
)

# 打印找到的测试文件
message(STATUS "测试源文件: ${TEST_SOURCES}")

# 创建测试可执行文件
add_executable(${TARGET_NAME} ${TEST_SOURCES})

# 链接EditorKit库和Catch2
target_link_libraries(${TARGET_NAME} PRIVATE EditorKit Catch2::Catch2WithMain)

# 添加包含目录
# EditorKit库已经通过target_include_directories设置了PUBLIC包含目录
# 所以链接EditorKit会自动包含其头文件目录
# 如果需要添加测试专用的包含目录，可以添加：
target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# 编译器选项
if(MSVC)
    target_compile_options(${TARGET_NAME} PRIVATE
        /W4
        /WX-
    )
else()
    target_compile_options(${TARGET_NAME} PRIVATE
        -Wall
        -Wextra
    )
endif()

# 设置目标属性
set_target_properties(${TARGET_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# 注册测试
include(CTest)
include(Catch)
catch_discover_tests(${TARGET_NAME})
