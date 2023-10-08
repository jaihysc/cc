cmake_minimum_required(VERSION 3.15)
project(cc)

# Setup compiler warnings
if ("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
	add_compile_options(/W4)
elseif ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
	add_compile_options(
        -Wall -Wextra -Wshadow -pedantic -Wmissing-prototypes -Wpointer-arith -Wcast-qual -Wswitch-default
		-Wswitch -Wbad-function-cast -Wlogical-op -Wredundant-decls -Wconversion -Wsign-conversion -Wdouble-promotion -Wmisleading-indentation
		-Wduplicated-cond -Wduplicated-branches -Wunreachable-code -Wuninitialized -Wmaybe-uninitialized -Wundef
		-Wno-unused-parameter
	)
endif()

add_subdirectory(src)
add_subdirectory(testu)
