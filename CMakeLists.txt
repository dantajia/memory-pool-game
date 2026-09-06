cmake_minimum_required(VERSION 3.15)
project(MemoryPoolGame CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 【核心】把 exe 直接输出到项目根目录（你要的效果）
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR})

# 加载源码
file(GLOB_RECURSE SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

add_executable(memorypool ${SRC_FILES})

if(MSVC)
    target_compile_options(memorypool PRIVATE /O2 /Oi /Ot)
    target_link_libraries(memorypool PRIVATE kernel32 user32 psapi)
endif()