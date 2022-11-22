#pragma once

//import header files
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <queue>
#include <string>
#include <iomanip>
#include <type_traits>
#include <unordered_map>
#include <memory>
#include <utility>
#include <algorithm>
#include <map>
#include <vector>
#include <list>
#include <set>
#include <unordered_set>
#include <cctype>
#include <thread>
#include <tuple>
#include <variant>

#include "magic_enum.h"
#include "asio.hpp"
#include "json.hpp"
#include "boost/lockfree/spsc_queue.hpp"
#include "boost/core/null_deleter.hpp"
#include "tclap/CmdLine.h"

#include "trace/trace.h"

using json = nlohmann::json;

#undef assert

#define DEBUG

#ifdef DEBUG
    #define verify(cond) \
        do \
        { \
            if(!(cond)) \
            { \
                printf("In file %s, Line %d, %s\n", __FILE__, __LINE__, #cond); \
                fflush(stdout); \
                abort();\
            } \
        }while(0)
        
    #define verify_only(cond) verify(cond)
#else
    #define verify(cond) cond
    #define verify_only(cond) do{}while(0)
#endif

//machine types
using size_t = std::size_t;

//unsigned integer types
using uint8_t = std::uint8_t;
using uint16_t = std::uint16_t;
using uint32_t = std::uint32_t;
using uint64_t = std::uint64_t;

//signed integer types
using int8_t = std::int8_t;
using int16_t = std::int16_t;
using int32_t = std::int32_t;
using int64_t = std::int64_t;

#define bitsizeof(x) (sizeof(x) * 8)

#define fillzero(length) std::setw(length) << std::setfill('0')
#define outhex(x) std::setiosflags(std::ios::uppercase) << std::hex << (x) << std::dec
#define outbool(x) std::boolalpha << (x)
#define outenum(x) magic_enum::enum_name(x)
#define div_round_up(n,d) (((n) + (d) - 1) / (d))

enum class riscv_exception_t
{
    instruction_address_misaligned = 0,
    instruction_access_fault,
    illegal_instruction,
    breakpoint,
    load_address_misaligned,
    load_access_fault,
    store_amo_address_misaligned,
    store_amo_access_fault,
    environment_call_from_u_mode,
    environment_call_from_s_mode,
    reserved_10,
    environment_call_from_m_mode,
    instruction_page_fault,
    load_page_fault,
    reserved_14,
    store_amo_page_fault,
};

enum class riscv_interrupt_t
{
    user_software = 0,
    supervisor_software,
    reserved_software,
    machine_software,
    user_timer,
    supervisor_timer,
    reserved_timer,
    machine_timer,
    user_external,
    supervisor_external,
    reserved_external,
    machine_external
};

inline bool is_align(uint32_t x, uint32_t access_size)
{
    return !(x & (access_size - 1));
}

inline uint32_t sign_extend(uint32_t imm, uint32_t imm_length)
{
    verify_only((imm_length > 0) && (imm_length < 32));
    auto sign_bit = (imm >> (imm_length - 1));
    auto extended_imm = ((sign_bit == 0) ? 0 : (((sign_bit << (32 - imm_length)) - 1) << imm_length)) | imm;
    return extended_imm;
}

inline uint32_t get_multi_radix_num(std::string str)
{
    uint32_t value;
    std::stringstream stream(str);
    
    if((str.size() > 2) && (str[0] == '0') && (tolower(str[1]) == 'x'))
    {
        stream.unsetf(std::ios::dec);
        stream.setf(std::ios::hex);
    }
    
    stream >> value;
    return value;
}

typedef struct if_print_t
{
    virtual ~if_print_t() = default;
    
    virtual void print(std::string indent)
    {
        std::cout << "<Not Implemented Method>" << std::endl;
    }

    virtual json get_json()
    {
        return {};
    }
}if_print_t;

template<typename T> class if_print_fake : public if_print_t
{
    private:
        T value;

    public:
        if_print_fake()
        {
        }

        if_print_fake(const T &value) : value(value)
        {
        }

        bool operator==(const T &x)
        {
           return this->value == x; 
        }
        
        T get()
        {
            return this->value;
        }
};

template<typename T> class if_print_direct : public if_print_t
{
    private:
        T value;
    
    public:
        if_print_direct()
        {
        }
        
        if_print_direct(const T &value) : value(value)
        {
        }
        
        bool operator==(const T &x)
        {
            return this->value == x;
        }
        
        T get()
        {
            return this->value;
        }
        
        virtual json get_json()
        {
            json r;
            r = this->value;
            return r;
        }
};

typedef struct if_reset_t
{
    virtual ~if_reset_t() = default;
    virtual void reset() = 0;
}if_reset_t;

template <typename T> 
void clear_queue(std::queue<T> &c) 
{ 
    std::queue<T> empty; 
    std::swap(empty, c);
} 

void branch_num_add();
void branch_predicted_add();
void branch_hit_add();
void branch_miss_add();
void fetch_decode_fifo_full_add();
void decode_rename_fifo_full_add();
void issue_queue_full_add();
void issue_execute_fifo_full_add();
void checkpoint_buffer_full_add();
void rob_full_add();
void phy_regfile_full_add();
void ras_full_add();
void fetch_not_full_add();

uint64_t get_cpu_clock_cycle();

#define EXIT_CODE_OK 0
#define EXIT_CODE_ERROR 1
#define EXIT_CODE_SERVER_PORT_ERROR 2
#define EXIT_CODE_TELNET_PORT_ERROR 3
#define EXIT_CODE_CSR_FINISH_OK_DETECTED 4
#define EXIT_CODE_CSR_FINISH_FAILURE_DETECTED 5
#define EXIT_CODE_OTHER_BREAKPOINT_DETECTED 6