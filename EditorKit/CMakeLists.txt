# 设置库版本
set(EDITORKIT_VERSION_MAJOR 1)
set(EDITORKIT_VERSION_MINOR 0)
set(EDITORKIT_VERSION_PATCH 0)
set(EDITORKIT_VERSION "${EDITORKIT_VERSION_MAJOR}.${EDITORKIT_VERSION_MINOR}.${EDITORKIT_VERSION_PATCH}")

# 递归查找所有源文件
# 注意：使用GLOB时，如果添加新文件需要重新运行cmake
file(GLOB_RECURSE EDITORKIT_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/*.cxx"
)

file(GLOB_RECURSE EDITORKIT_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/*.hh"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/*.hxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/*.inl"
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
    target_include_directories(EditorKit
        PUBLIC 
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Src>
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

