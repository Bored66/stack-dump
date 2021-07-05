cmake_minimum_required(VERSION 3.5)

project(dump-stack LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES main.cpp other.cpp BfdInfo.cpp PrintStackUtils.cpp)

add_executable(${PROJECT_NAME} ${SOURCES})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} dl bfd)

