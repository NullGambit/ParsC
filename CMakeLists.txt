cmake_minimum_required(VERSION 4.2)
project(ParsC)

set(CMAKE_CXX_STANDARD 20)

include_directories(
        src/
        deps/unordered_dense/include
        deps/magic_enum/include)

add_subdirectory(deps/unordered_dense/)
add_subdirectory(deps/magic_enum/)

add_executable(ParsC src/main.cpp
        src/containers/hash_map.hpp
        src/frontend/text_reader.hpp
        src/frontend/text_reader.cpp
        src/type_defs.hpp
        src/util/fmt.cpp
        src/util/fmt.hpp
        src/util/io.cpp
        src/util/io.hpp
        src/frontend/token.cpp
        src/frontend/token.hpp
        src/frontend/lexer.cpp
        src/frontend/lexer.hpp
        src/debug/token_printer.cpp
        src/debug/token_printer.hpp
        src/overload.hpp
        src/frontend/parser.cpp
        src/frontend/parser.hpp
        src/frontend/stmt.hpp
        src/frontend/expr.hpp
        src/frontend/location.hpp
        src/frontend/file_manager.cpp
        src/frontend/file_manager.hpp
        src/frontend/node.cpp
        src/frontend/node.hpp
        src/frontend/type.hpp
        src/memory/virtual_alloc.hpp
        src/memory/linux/linux_virtual_alloc.cpp
        src/memory/linux/linux_virtual_alloc.hpp
        src/frontend/visitor.hpp
        src/frontend/type.cpp
        src/debug/ast_printer.cpp
        src/debug/ast_printer.hpp
        src/frontend/symbol.hpp
        src/frontend/symbol.cpp
        src/frontend/frontend_error.cpp
        src/frontend/frontend_error.hpp
)

target_compile_options(ParsC PRIVATE -include ${CMAKE_SOURCE_DIR}/src/type_defs.hpp)