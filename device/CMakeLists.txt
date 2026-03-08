cmake_minimum_required(VERSION 3.0)
project(device_sim C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(device_sim src/main.c src/transport_select.c src/mqtt_transport.c src/shared_mem.c src/ws_transport.c src/string_parser.c)
target_include_directories(device_sim PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/src 
)
find_package(PkgConfig REQUIRED)
pkg_check_modules(MOSQUITTO REQUIRED IMPORTED_TARGET libmosquitto libwebsockets)
enable_testing()


#target_link_libraries(device_sim pthread)

# String parser tests
add_executable(test_string_parser
    tests/unity/unity.c
    tests/tests_string_parser.c
    src/string_parser.c
    src/shared_mem.c
)
target_include_directories(test_string_parser PRIVATE
    include
    tests/unity
)
target_link_libraries(test_string_parser pthread)
add_test(NAME test_string_parser COMMAND test_string_parser)

# Queue tests
add_executable(test_queue
    tests/unity/unity.c
    tests/tests_shared_mem.c
    src/shared_mem.c
)
target_include_directories(test_queue PRIVATE
    include
    tests/unity
)

target_link_libraries(test_queue pthread)
add_test(NAME test_queue COMMAND test_queue)

add_executable(test_transport_select
    tests/unity/unity.c
    tests/test_transport_select.c
    src/transport_select.c
)

target_include_directories(test_transport_select PRIVATE
    include
    tests/unity
)

add_test(NAME test_transport_select COMMAND test_transport_select)

target_link_libraries(device_sim PRIVATE PkgConfig::MOSQUITTO)
target_compile_options(device_sim PRIVATE -Wall -Wextra -Wpedantic)
