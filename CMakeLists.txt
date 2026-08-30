cmake_minimum_required(VERSION 3.10)
project(CinderWick)

set(CMAKE_BUILD_TYPE Debug)

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(TEST_DIR ${CMAKE_SOURCE_DIR}/tests)

set(MAIN_SOURCE_FILES
    ${SRC_DIR}/main.c
    ${SRC_DIR}/serial.c
    ${SRC_DIR}/reset.c
    ${SRC_DIR}/bridge.c
    ${SRC_DIR}/protocol.c
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include_directories(${INCLUDE_DIR})

add_executable(${PROJECT_NAME} ${MAIN_SOURCE_FILES})

add_custom_target(run
    COMMAND ${PROJECT_NAME}
    DEPENDS ${PROJECT_NAME}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT ${PROJECT_NAME}
)


# --- Tests ---
enable_testing()
set(TEST_SOURCE_FILES
    ${TEST_DIR}/test_protocol.c
    ${SRC_DIR}/serial.c
    ${SRC_DIR}/protocol.c
)

add_executable(test_protocol ${TEST_SOURCE_FILES})

add_test(NAME protocol_tests COMMAND test_protocol)

add_custom_target(run_tests
    COMMAND test_protocol
    DEPENDS test_protocol
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Running protocol tests"
)
