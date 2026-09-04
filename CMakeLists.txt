cmake_minimum_required(VERSION 3.28.3)
project(ParsC)

set(CMAKE_CXX_STANDARD 20)

include_directories(
        src/
        deps/unordered_dense/include
        deps/magic_enum/include)

add_subdirectory(deps/unordered_dense/)
add_subdirectory(deps/magic_enum/)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

include_directories(${LLVM_INCLUDE_DIRS})

add_executable(ParsC src/main.cpp
        src/containers/hash_map.hpp
        src/text_reader.hpp
        src/text_reader.cpp
        src/type_defs.hpp
        src/util/fmt.cpp
        src/util/fmt.hpp
        src/util/io.cpp
        src/util/io.hpp
        src/token.cpp
        src/token.hpp
        src/lexer.cpp
        src/lexer.hpp
        src/debug/token_printer.cpp
        src/debug/token_printer.hpp
        src/overload.hpp
        src/ast.cpp
        src/ast.hpp
        src/stmt.hpp
        src/expr.hpp
        src/location.hpp
        src/file_manager.cpp
        src/file_manager.hpp
        src/node.cpp
        src/node.hpp
        src/type.hpp
        src/memory/virtual_alloc.hpp
        src/memory/linux/linux_virtual_alloc.cpp
        src/memory/linux/linux_virtual_alloc.hpp
        src/visitor.hpp
        src/type.cpp
        src/debug/ast_printer.cpp
        src/debug/ast_printer.hpp
        src/symbol.hpp
        src/symbol.cpp
        src/frontend_error.cpp
        src/frontend_error.hpp
        src/util/macros.hpp
        src/emit_context.hpp
        src/compiler.cpp
        src/compiler.hpp
        src/expr.cpp
        src/compile_error.cpp
        src/compile_error.hpp
        src/stmt.cpp
        src/scope_table.cpp
        src/scope_table.hpp
        src/module_manager.cpp
        src/module_manager.hpp
        src/module.cpp
        src/module.hpp
        src/memory/defs.hpp
        src/memory/arena.cpp
        src/memory/arena.hpp
        src/analyzer.cpp
        src/analyzer.hpp
        src/visitor.cpp
        src/parse_ctx.hpp
        src/type_meta.hpp
        src/util/llvm_utils.cpp
        src/util/llvm_utils.hpp
        src/util/defer.hpp
        src/comp_eval.cpp
        src/comp_eval.hpp
        src/mangle.hpp
)

set(LLVM_STATIC ON)

target_compile_definitions(ParsC PRIVATE LLVM_DISABLE_ABI_BREAKING_CHECKS_ENFORCING=1)
target_compile_options(ParsC PRIVATE -include ${CMAKE_SOURCE_DIR}/src/type_defs.hpp)

set_target_properties(ParsC PROPERTIES OUTPUT_NAME "parsc")
add_custom_command(TARGET ParsC
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:ParsC> ./../../)

llvm_map_components_to_libnames(llvm_libs
    core
    support
    irreader
)

target_link_libraries(ParsC PRIVATE LLVM)
