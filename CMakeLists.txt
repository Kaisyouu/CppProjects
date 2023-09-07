cmake_minimum_required(VERSION 3.24)
project(CppProjects)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)


# 添加子目录
add_subdirectory(fixed_price_matcher)

if (BUILD_TEST)
    add_subdirectory(test)
endif()