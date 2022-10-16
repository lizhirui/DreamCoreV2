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

add_executable(sim ${src_files} include/network/network.h include/cycle_model/component/dff.h test/test.cpp include/test.h test/dff.h test/dff.cpp include/cycle_model/component/fifo.h include/cycle_model/component/stack.h include/cycle_model/component/ras.h include/cycle_model/component/port.h include/cycle_model/component/interrupt_interface.h include/cycle_model/component/rob.h include/cycle_model/component/regfile.h include/cycle_model/component/rat.h include/cycle_model/component/store_buffer.h include/cycle_model/component/slave_base.h include/cycle_model/component/bus.h include/cycle_model/component/slave/clint.h include/cycle_model/component/slave/memory.h include/cycle_model/component/issue_queue.h include/cycle_model/component/free_list.h src/cycle_model/pipeline/fetch1.cpp src/cycle_model/pipeline/fetch2.cpp src/cycle_model/pipeline/decode.cpp src/cycle_model/pipeline/rename.cpp src/cycle_model/pipeline/issue.cpp src/cycle_model/pipeline/issue.h src/cycle_model/pipeline/issue.h src/cycle_model/pipeline/issue.cpp src/cycle_model/pipeline/readreg.cpp src/cycle_model/pipeline/execute/alu.cpp src/cycle_model/pipeline/execute/bru.cpp src/cycle_model/pipeline/execute/csr.cpp src/cycle_model/pipeline/execute/div.cpp src/cycle_model/pipeline/execute/lsu.cpp src/cycle_model/pipeline/execute/mul.cpp include/cycle_model/pipeline/fetch1.h include/cycle_model/pipeline/fetch1_fetch2.h include/cycle_model/pipeline/fetch2_decode.h include/cycle_model/pipeline/decode_rename.h include/cycle_model/pipeline/rename_issue.h include/cycle_model/pipeline/issue_readreg.h include/cycle_model/pipeline/readreg_execute.h include/cycle_model/pipeline/execute_wb.h include/cycle_model/pipeline/wb_commit.h include/cycle_model/pipeline/decode.h include/cycle_model/pipeline/rename.h include/cycle_model/pipeline/issue.h include/cycle_model/pipeline/readreg.h include/cycle_model/pipeline/wb.h include/cycle_model/pipeline/commit.h src/cycle_model/pipeline/wb.cpp src/cycle_model/pipeline/commit.cpp include/cycle_model/pipeline/execute/alu.h include/cycle_model/pipeline/execute/bru.h include/cycle_model/pipeline/execute/csr.h include/cycle_model/pipeline/execute/div.h include/cycle_model/pipeline/execute/lsu.h include/cycle_model/pipeline/execute/mul.h include/cycle_model/pipeline/fetch2.h include/cycle_model/pipeline/pipeline_common.h include/cycle_model/component/handshake_dff.h)
