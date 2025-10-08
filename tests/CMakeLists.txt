cmake_minimum_required(VERSION 3.20)

# Test sources
set(TEST_SOURCES
    basic_tests.cpp
    ../src/crc.cpp
    ../src/packet.cpp
    ../src/link.cpp
)

# Test executable
add_executable(satcom_tests ${TEST_SOURCES})

# Link threads
target_link_libraries(satcom_tests PRIVATE Threads::Threads)

# Add test
add_test(NAME BasicTests COMMAND satcom_tests)
