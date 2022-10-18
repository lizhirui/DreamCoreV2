cmake_minimum_required(VERSION 3.12)
project(model)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CMAKE_CXX_STACK_SIZE "1048576")

if(CMAKE_COMPILER_IS_GNUCXX)
    add_compile_options(-Wall -Werror)
else()
    message(FATAL_ERROR "This compiler isn't supported by this model!")
endif()

include_directories(./include ./tools)
file(GLOB_RECURSE src_files CONFIGURE_DEPENDS "./src/*.cpp" "./include/*.h" "./include/*.hpp" "./tools/*.h" "./tools/*.cpp")

add_executable(sim ${src_files})
