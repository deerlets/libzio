cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_STANDARD 11)
set(PROJECT_NAME libzio-test)

project(${PROJECT_NAME} C CXX)
add_definitions(-DWITH_TESTS)

# serial
add_executable(runSerialTests serial_test.cpp)
target_link_libraries(runSerialTests gtest gtest_main pthread zio)
add_test(runDeviceTests ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/runSerialTests)

# socket
add_executable(runSocketTests socket_test.cpp)
target_link_libraries(runSocketTests gtest gtest_main pthread zio)
add_test(runSocketTests ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/runSocketTests)
