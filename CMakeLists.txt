cmake_minimum_required(VERSION 3.4.3)
project(tinyfort C CXX)

# Colors when compiling with clang
# set(CMAKE_CXX_FLAGS "-fdiagnostics-color=always -fno-rtti -g -Wall -Werror")
set(CMAKE_CXX_FLAGS "-fdiagnostics-color=always -fno-rtti -g -Wall")

# LLVM
find_package(LLVM REQUIRED)

# Boost
find_package(Boost REQUIRED)
# HACK: I'm using all. I need to ask on the mailing list how to fix this.

llvm_map_components_to_libnames(LLVM_LIBS Target
     Core Support ORCJIT MCJIT ExecutionEngine
    ${LLVM_TARGETS_TO_BUILD}
    ASMPrinter
    CodeGen
    AllTargetsAsmParsers
    AllTargetsAsmPrinters
    AllTargetsDescs
    AllTargetsInfos TransformUtils Object
    ipo Passes Analysis
    nativecodegen MC)

#llvm_map_components_to_libnames(LLVM_LIBS AllTargetsAsmPrinters 
#    AllTargetsAsmParsers Core Support ORCJIT MCJIT ExecutionEngine
#    ASMPrinter CodeGen AllTargetsDescs AllTargetsInfos TransformUtils Object
#    ipo Passes Analysis
#    nativecodegen MC)

# FLEX AND BISON
find_package(BISON)
find_package(FLEX)
bison_target(Parser src/parser.yy ${CMAKE_SOURCE_DIR}/src/parser.generated.cpp COMPILE_FLAGS -v)
flex_target(Lexer src/lexer.l  ${CMAKE_SOURCE_DIR}/src/lexer.generated.cpp)
add_flex_bison_dependency(Lexer Parser)


# INCLUDES AND SOURCES
include_directories(include ${LLVM_INCLUDE_DIRS} include/tinyfort)

add_executable(tinyfort
    src/main.cpp
    ${BISON_Parser_OUTPUTS}
    ${FLEX_Lexer_OUTPUTS}
    )

target_link_libraries(tinyfort ${LLVM_LIBS}  -lstdc++)
set_target_properties(tinyfort PROPERTIES CXX_STANDARD 14)

# CLANG FORMAT
file(GLOB_RECURSE ALL_SOURCE_FILES *.cpp *.h)

find_program(CLANG_FORMAT clang-format)
add_custom_target(
        format
        COMMAND ${CLANG_FORMAT}
        -style=LLVM
        -i
        ${ALL_SOURCE_FILES}
)
