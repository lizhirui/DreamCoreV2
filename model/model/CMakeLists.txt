cmake_minimum_required(VERSION 3.12)
project(model)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CMAKE_CXX_STACK_SIZE "1048576")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/bin_debug")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/bin")

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast")
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
    add_compile_options(-Wall -Werror -Wno-unused-function -Wno-unused-but-set-variable -Wno-unused-variable)
else()
    message(FATAL_ERROR "This compiler isn't supported by this model!")
endif()

include_directories(./include ./tools ./include_third_party)
file(GLOB_RECURSE src_files CONFIGURE_DEPENDS "./src/*.cpp" "./include/*.h" "./include/*.hpp" "./tools/*.h" "./tools/*.cpp")

add_executable(sim ${src_files} include/fixed_size_buffer.h)
target_link_libraries(sim pthread)
