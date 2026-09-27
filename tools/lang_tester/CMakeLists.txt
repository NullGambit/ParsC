cmake_minimum_required(VERSION 3.28.3)
project(LangTester)

set(CMAKE_CXX_STANDARD 23)

set(ROOT_DIR ${CMAKE_SOURCE_DIR}/../..)

include_directories(
        ../../src
        ${ROOT_DIR}/deps/unordered_dense/include
        ${ROOT_DIR}/deps/magic_enum/include)

#add_subdirectory(${ROOT_DIR} ${CMAKE_BINARY_DIR}/deps/unordered_dense/)
#add_subdirectory(${ROOT_DIR} ${CMAKE_BINARY_DIR}/deps/magic_enum/)

add_executable(LangTester src/main.cpp
        ../../src/cli.hpp
        ../../src/cli.cpp
        ../../src/containers/hash_map.hpp
        ../../src/util/fmt.cpp
        ../../src/util/fmt.hpp)

set_target_properties(LangTester PROPERTIES OUTPUT_NAME "lang_tester")
add_custom_command(TARGET LangTester
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:LangTester> ./../../../../)

target_compile_options(LangTester PRIVATE -include ${CMAKE_SOURCE_DIR}/../../src/type_defs.hpp)