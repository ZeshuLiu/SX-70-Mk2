# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

# 把 src/ 注册为额外组件目录（外设/控制代码）
set(EXTRA_COMPONENT_DIRS ${CMAKE_SOURCE_DIR}/src)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(SX70_ModelZ)
