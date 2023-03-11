#pragma once
#include "common.h"
const uint32_t FETCH_WIDTH = 4;
const uint32_t DECODE_WIDTH = 4;
const uint32_t RENAME_WIDTH = 4;
const uint32_t DISPATCH_WIDTH = 4;
const uint32_t INTEGER_ISSUE_WIDTH = 2;
const uint32_t LSU_ISSUE_WIDTH = 3;
const uint32_t INTEGER_READREG_WIDTH = INTEGER_ISSUE_WIDTH;
const uint32_t LSU_READREG_WIDTH = LSU_ISSUE_WIDTH;
const uint32_t COMMIT_WIDTH = 4;

const uint32_t PHY_REG_NUM = 128;
const uint32_t ARCH_REG_NUM = 32;

const uint32_t FETCH2_DECODE_FIFO_SIZE = 256;
const uint32_t DECODE_RENAME_FIFO_SIZE = 16;
const uint32_t INTEGER_ISSUE_QUEUE_SIZE = 16;
const uint32_t LSU_ISSUE_QUEUE_SIZE = 16;
const uint32_t ROB_SIZE = 64;
const uint32_t STORE_BUFFER_SIZE = 16;
const uint32_t CHECKPOINT_BUFFER_SIZE = 64;
const uint32_t LOAD_QUEUE_SIZE = 32;
#define ENABLE_CHECKPOINT

const uint32_t ALU_UNIT_NUM = 2;
const uint32_t BRU_UNIT_NUM = 1;
const uint32_t CSR_UNIT_NUM = 1;
const uint32_t DIV_UNIT_NUM = 1;
const uint32_t MUL_UNIT_NUM = 2;
const uint32_t LU_UNIT_NUM = 1;
const uint32_t SAU_UNIT_NUM = 1;
const uint32_t SDU_UNIT_NUM = 1;

const uint32_t EXECUTE_UNIT_NUM = ALU_UNIT_NUM + BRU_UNIT_NUM + CSR_UNIT_NUM + DIV_UNIT_NUM + MUL_UNIT_NUM + LU_UNIT_NUM + SAU_UNIT_NUM + SDU_UNIT_NUM;
const uint32_t FEEDBACK_EXECUTE_UNIT_NUM = EXECUTE_UNIT_NUM - SAU_UNIT_NUM - SDU_UNIT_NUM;

const uint32_t EXECUTE_UNIT_TYPE_NUM = 6;
const uint32_t ALU_SHIFT = 0;
const uint32_t BRU_SHIFT = 1;
const uint32_t CSR_SHIFT = 2;
const uint32_t DIV_SHIFT = 3;
const uint32_t MUL_SHIFT = 4;
const uint32_t LSU_SHIFT = 5;

const uint32_t ALU_LATENCY = 1;
const uint32_t BRU_LATENCY = 1;
const uint32_t CSR_LATENCY = 1;
const uint32_t DIV_LATENCY = 8;
const uint32_t MUL_LATENCY = 1;
const uint32_t LSU_LATENCY = 2;

const uint32_t INIT_LPV = 0x04;//RF - 100 LU_L1 - 10 LU_L2 - 1

const uint32_t INIT_PC = 0x80000000;

const uint32_t MEMORY_BASE = 0x80000000;
const uint32_t MEMORY_SIZE = 1048576;

const uint32_t CLINT_BASE = 0x20000000;
const uint32_t CLINT_SIZE = 0x10000;

const uint32_t L0_BTB_ADDR_WIDTH = 4;
const uint32_t L0_BTB_SIZE = 1 << L0_BTB_ADDR_WIDTH;
const uint32_t L0_BTB_ADDR_MASK = L0_BTB_SIZE - 1;
const uint32_t BI_MODAL_ADDR_WIDTH = 4;
const uint32_t BI_MODAL_SIZE = 1 << BI_MODAL_ADDR_WIDTH;
const uint32_t BI_MODAL_ADDR_MASK = BI_MODAL_SIZE - 1;

const uint32_t BI_MODE_GLOBAL_HISTORY_LENGTH = 16;
const uint32_t BI_MODE_BRANCH_PC_LENGTH = 16;
const uint32_t BI_MODE_PHT_CHOICE_LENGTH = 6;
const uint32_t BI_MODE_PHT_ADDR_WIDTH = BI_MODE_BRANCH_PC_LENGTH;
const uint32_t BI_MODE_PHT_SIZE = 1 << BI_MODE_PHT_ADDR_WIDTH;
const uint32_t BI_MODE_PHT_CHOICE_ADDR_WIDTH = BI_MODE_PHT_CHOICE_LENGTH;
const uint32_t BI_MODE_PHT_CHOICE_SIZE = 1 << BI_MODE_PHT_CHOICE_ADDR_WIDTH;
const uint32_t BI_MODE_PHT_CHOICE_ADDR_MASK = BI_MODE_PHT_CHOICE_SIZE - 1;
const uint32_t BI_MODE_BRANCH_PC_MASK = (1 << BI_MODE_BRANCH_PC_LENGTH) - 1;
const uint32_t BI_MODE_GLOBAL_HISTORY_MASK = (1 << BI_MODE_GLOBAL_HISTORY_LENGTH) - 1;

const uint32_t RAS_SIZE = 256;

const uint32_t CHARFIFO_SEND_FIFO_SIZE = 1024;
const uint32_t CHARFIFO_REV_FIFO_SIZE = 1024;

const uint32_t WAIT_TABLE_SIZE = 1024;

typedef boost::lockfree::spsc_queue<char, boost::lockfree::capacity<CHARFIFO_SEND_FIFO_SIZE>> charfifo_send_fifo_t;
typedef boost::lockfree::spsc_queue<char, boost::lockfree::capacity<CHARFIFO_REV_FIFO_SIZE>> charfifo_rev_fifo_t;

#define MODEL_NAME "DreamCoreV2 Model"

#define MESSAGE_OUTPUT_PREFIX "[" MODEL_NAME "]: "

#define MODE_ISA_MODEL_ONLY 0
#define MODE_CYCLE_MODEL_ONLY 1
#define MODE_ISA_AND_CYCLE_MODEL_COMPARE 2

//#define MODE MODE_ISA_MODEL_ONLY
//#define MODE MODE_CYCLE_MODEL_ONLY
#define MODE MODE_ISA_AND_CYCLE_MODEL_COMPARE

#if (MODE == MODE_ISA_MODEL_ONLY) || (MODE == MODE_ISA_AND_CYCLE_MODEL_COMPARE)
#define NEED_ISA_MODEL
#endif
#if (MODE == MODE_CYCLE_MODEL_ONLY) || (MODE == MODE_ISA_AND_CYCLE_MODEL_COMPARE)
#define NEED_CYCLE_MODEL
#endif
#if (MODE == MODE_ISA_AND_CYCLE_MODEL_COMPARE)
#define NEED_ISA_AND_CYCLE_MODEL_COMPARE
#endif

//#define BRANCH_DUMP
const std::string BRANCH_DUMP_FILE = "../../../dump/branch_dump/coremark_10_7_2.txt";

//#define BRANCH_PREDICTOR_UPDATE_DUMP
const std::string BRANCH_PREDICTOR_UPDATE_DUMP_FILE = "../../../dump/branch_predictor_update_dump/coremark_10_7_2.txt";

//#define BRANCH_PREDICTOR_DUMP
const std::string BRANCH_PREDICTOR_DUMP_FILE = "../../../dump/branch_predictor_dump/coremark_10_7_2_checkpoint.txt";

const std::string TRACE_DIR = "../../../trace/coremark_10/";

//#define TRACE_ENABLE
//#define TRACE_ENABLE_FULL

#ifdef TRACE_ENABLE_FULL
    const bool TRACE_FETCH1 = true;
    const bool TRACE_FETCH2 = true;
    const bool TRACE_DECODE = true;
    const bool TRACE_RENAME = true;
    const bool TRACE_DISPATCH = false;
    const bool TRACE_INTEGER_ISSUE = false;
    const bool TRACE_LSU_ISSUE = false;
    const bool TRACE_INTEGER_READREG = false;
    const bool TRACE_LSU_READREG = false;
    const bool TRACE_EXECUTE_ALU = true;
    const bool TRACE_EXECUTE_BRU = true;
    const bool TRACE_EXECUTE_CSR = true;
    const bool TRACE_EXECUTE_DIV = true;
    const bool TRACE_EXECUTE_LSU = true;
    const bool TRACE_EXECUTE_MUL = true;
    const bool TRACE_WB = true;
    const bool TRACE_COMMIT = true;
    const bool TRACE_BRANCH_PREDICTOR = true;
    const bool TRACE_RAS = true;
    const bool TRACE_RAT = true;
    const bool TRACE_ROB = true;
    const bool TRACE_PHY_REGFILE = true;
    const bool TRACE_STORE_BUFFER = true;
    const bool TRACE_CHECKPOINT_BUFFER = true;
    const bool TRACE_CSRFILE = true;
    const bool TRACE_INTERRUPT_INTERFACE = true;
    const bool TRACE_BUS = true;
    const bool TRACE_CLINT = true;
    const bool TRACE_TCM = true;
#elif defined(TRACE_ENABLE)
    const bool TRACE_FETCH1 = false;
    const bool TRACE_FETCH2 = false;
    const bool TRACE_DECODE = false;
    const bool TRACE_RENAME = false;
    const bool TRACE_DISPATCH = false;
    const bool TRACE_INTEGER_ISSUE = false;
    const bool TRACE_LSU_ISSUE = false;
    const bool TRACE_INTEGER_READREG = false;
    const bool TRACE_LSU_READREG = false;
    const bool TRACE_EXECUTE_ALU = false;
    const bool TRACE_EXECUTE_BRU = false;
    const bool TRACE_EXECUTE_CSR = false;
    const bool TRACE_EXECUTE_DIV = false;
    const bool TRACE_EXECUTE_LSU = false;
    const bool TRACE_EXECUTE_MUL = false;
    const bool TRACE_WB = false;
    const bool TRACE_COMMIT = false;
    const bool TRACE_BRANCH_PREDICTOR = false;
    const bool TRACE_RAS = false;
    const bool TRACE_RAT = false;
    const bool TRACE_ROB = false;
    const bool TRACE_PHY_REGFILE = false;
    const bool TRACE_STORE_BUFFER = false;
    const bool TRACE_CHECKPOINT_BUFFER = false;
    const bool TRACE_CSRFILE = false;
    const bool TRACE_INTERRUPT_INTERFACE = false;
    const bool TRACE_BUS = false;
    const bool TRACE_CLINT = false;
    const bool TRACE_TCM = true;
#else
    const bool TRACE_FETCH1 = false;
    const bool TRACE_FETCH2 = false;
    const bool TRACE_DECODE = false;
    const bool TRACE_RENAME = false;
    const bool TRACE_DISPATCH = false;
    const bool TRACE_INTEGER_ISSUE = false;
    const bool TRACE_LSU_ISSUE = false;
    const bool TRACE_INTEGER_READREG = false;
    const bool TRACE_LSU_READREG = false;
    const bool TRACE_EXECUTE_ALU = false;
    const bool TRACE_EXECUTE_BRU = false;
    const bool TRACE_EXECUTE_CSR = false;
    const bool TRACE_EXECUTE_DIV = false;
    const bool TRACE_EXECUTE_LSU = false;
    const bool TRACE_EXECUTE_MUL = false;
    const bool TRACE_WB = false;
    const bool TRACE_COMMIT = false;
    const bool TRACE_BRANCH_PREDICTOR = false;
    const bool TRACE_RAS = false;
    const bool TRACE_RAT = false;
    const bool TRACE_ROB = false;
    const bool TRACE_PHY_REGFILE = false;
    const bool TRACE_STORE_BUFFER = false;
    const bool TRACE_CHECKPOINT_BUFFER = false;
    const bool TRACE_CSRFILE = false;
    const bool TRACE_INTERRUPT_INTERFACE = false;
    const bool TRACE_BUS = false;
    const bool TRACE_CLINT = false;
    const bool TRACE_TCM = false;
#endif