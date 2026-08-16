cmake_minimum_required(VERSION 3.10)
project(CinderWick)

set(CMAKE_BUILD_TYPE Debug)

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)

set(MAIN_SOURCE_FILES
    ${SRC_DIR}/main.c
)

set(CMAKE_EXPORT_COMMANDS ON)
include_directories(${INCLUDE_DIR})

add_executable(${PROJECT_NAME} ${MAIN_SOURCE_FILES})

add_custom_target(run
    COMMAND ${PROJECT_NAME}
    DEPENDS ${PROJECT_NAME}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT ${PROJECT_NAME}
)

