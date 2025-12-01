cmake_minimum_required(VERSION 3.10)
project(MyWebServer)

# C++14 标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)

add_compile_options(-Wall -Wextra)

include_directories(include)

# 查找所有源文件 (会自动包含 src/heaptimer.cpp, src/log.cpp, src/main.cpp 等)
file(GLOB_RECURSE SOURCES "src/*.cpp")

# 查找线程库
find_package(Threads REQUIRED)

# --- 1. 主服务器 ---
# 这一行只能出现一次！
add_executable(server ${SOURCES})
target_link_libraries(server Threads::Threads)

# --- 2. 线程池测试 ---
# 因为 test_threadpool.cpp 用到了 Log 类，需要链接 src/log.cpp
add_executable(test_pool tests/test_threadpool.cpp src/log.cpp)
target_link_libraries(test_pool Threads::Threads)

# --- 3. 日志测试 ---
# 同理，需要 src/log.cpp
add_executable(test_log tests/test_log.cpp src/log.cpp)
target_link_libraries(test_log Threads::Threads)