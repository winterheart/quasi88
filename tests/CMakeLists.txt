include(CTest)
find_package(GTest REQUIRED)

include_directories(${PROJECT_SOURCE_DIR}/src)

add_executable(endianess endianess.cpp)
target_link_libraries(endianess GTest::gtest_main)

add_test(NAME endianess	COMMAND endianess)
