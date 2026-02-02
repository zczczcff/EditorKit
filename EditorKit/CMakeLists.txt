cmake_minimum_required(VERSION 3.15)

# 设置项目名称和语言
project(EditorKit LANGUAGES CXX C)

# 设置策略 CMP0144 的行为
if (POLICY CMP0144)
    cmake_policy(SET CMP0144 NEW)
endif ()

# 设置库版本
set(EDITORKIT_VERSION_MAJOR 1)
set(EDITORKIT_VERSION_MINOR 0)
set(EDITORKIT_VERSION_PATCH 0)
set(EDITORKIT_VERSION "${EDITORKIT_VERSION_MAJOR}.${EDITORKIT_VERSION_MINOR}.${EDITORKIT_VERSION_PATCH}")

# 从环境变量获取Boost根目录（如果有）
if(DEFINED ENV{Boost_ROOT})
    set(BOOST_ROOT $ENV{Boost_ROOT} CACHE PATH "Boost根目录")
    set(BOOST_INCLUDEDIR ${BOOST_ROOT})
    set(BOOST_LIBRARYDIR ${BOOST_ROOT}/lib)
    message(STATUS "使用环境变量中的Boost_ROOT: ${BOOST_ROOT}")
endif()

# 简化版：只包含Boost头文件
find_package(Boost QUIET)

if(NOT Boost_FOUND)
    # 尝试通过环境变量或自定义路径查找
    if(DEFINED ENV{BOOST_ROOT})
        set(BOOST_ROOT $ENV{BOOST_ROOT})
        set(BOOST_INCLUDEDIR ${BOOST_ROOT})
    endif()
    
    # 查找Boost头文件
    find_path(Boost_INCLUDE_DIRS
        NAMES boost/version.hpp
        PATHS ${BOOST_ROOT} ${BOOST_INCLUDEDIR}
        PATH_SUFFIXES include
    )
    
    if(Boost_INCLUDE_DIRS)
        set(Boost_FOUND TRUE)
        message(STATUS "Boost头文件找到: ${Boost_INCLUDE_DIRS}")
    else()
        message(WARNING "Boost库未找到，可能需要手动设置BOOST_ROOT环境变量")
    endif()
endif()

# 递归查找所有源文件
# 注意：使用GLOB时，如果添加新文件需要重新运行cmake
file(GLOB_RECURSE EDITORKIT_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/EditorKit/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/EditorKit/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/EditorKit/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/EditorKit/*.cxx"
)

file(GLOB_RECURSE EDITORKIT_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/EditorKit/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/EditorKit/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/EditorKit/*.hh"
    "${CMAKE_CURRENT_SOURCE_DIR}/EditorKit/*.hxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/EditorKit/*.inl"
)

# 打印源文件信息用于调试
message(STATUS "EditorKit源文件: ${EDITORKIT_SOURCES}")
message(STATUS "EditorKit头文件: ${EDITORKIT_HEADERS}")

# 检查是否有源文件
if(NOT EDITORKIT_SOURCES)
    message(WARNING "没有找到EditorKit源文件，将创建接口库")
endif()

# 创建静态库
if(EDITORKIT_SOURCES)
    add_library(EditorKit STATIC ${EDITORKIT_SOURCES} ${EDITORKIT_HEADERS})
else()
    # 如果是纯头文件库
    add_library(EditorKit INTERFACE)
endif()


# 设置目标属性
set_target_properties(EditorKit PROPERTIES
    VERSION ${EDITORKIT_VERSION}
    SOVERSION ${EDITORKIT_VERSION_MAJOR}
    POSITION_INDEPENDENT_CODE ON
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    OUTPUT_NAME "EditorKit"
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# 设置包含目录
if(TARGET EditorKit)
    # 添加Boost头文件包含
    if(Boost_INCLUDE_DIRS)
        target_include_directories(EditorKit
            PUBLIC
                ${Boost_INCLUDE_DIRS}
        )
    endif()
    target_include_directories(EditorKit
        PUBLIC 
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
            $<INSTALL_INTERFACE:include>
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()

# 编译器选项
if(MSVC)
    target_compile_options(EditorKit PRIVATE
        /W4           # 警告级别4
        /WX-          # 警告不视为错误
        /permissive-  # 标准一致性
    )
else()
    target_compile_options(EditorKit PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wno-unused-parameter
    )
endif()

