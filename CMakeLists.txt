cmake_minimum_required(VERSION 3.5)

project(dump-stack LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES main.cpp other.cpp BfdInfo.cpp)

set(WIN32, 1)

if (WIN32)
set(SOURCES ${SOURCES} PrintStackUtilsWin.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})
get_filename_component(CURR_MINGW_CXX_DIR ${CMAKE_CXX_COMPILER} DIRECTORY ABSOLUTE)
set(CURR_MINGW_LIB_DIR ${CURR_MINGW_CXX_DIR}/../lib)
find_library(BFD_LIBRARY bfd HINTS ${CURR_MINGW_LIB_DIR} ${CURR_MINGW_LIB_DIR}/binutils)
find_path(BINUTILS_INCLUDE_DIR
NAMES bfd.h
HINTS ${CURR_MINGW_CXX_DIR}/../include ${CURR_MINGW_CXX_DIR}/../include/binutils
)

get_filename_component(BFD_LIBRARY_DIR ${BFD_LIBRARY} DIRECTORY ABSOLUTE)

if (NOT BFD_LIBRARY)
    message("Failed to locate BFD library ${BFD_LIBRARY} / ${BINUTILS_INCLUDE_DIR} ")
else()
    message("Found BFD library ${BFD_LIBRARY} / ${BINUTILS_INCLUDE_DIR}  ")
endif()
message("${SOURCES}")
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Dbghelp
        -L${BFD_LIBRARY_DIR} bfd
        intl
        z
        iberty
        )
        target_include_directories(${PROJECT_NAME}
        PRIVATE
            ${BINUTILS_INCLUDE_DIR}
        )
else()
set(SOURCES ${SOURCES} PrintStackUtils.cpp)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} dl bfd)
add_executable(${PROJECT_NAME} ${SOURCES})
endif()

