/*
 * Copyright lizhirui
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-10-18     lizhirui     the first version
 */

#pragma once
#include "common.h"
#include "config.h"
#include "isa_model/component/bus.h"
#include "isa_model/component/csr_all.h"
#include "isa_model/component/csrfile.h"
#include "isa_model/component/interrupt_interface.h"
#include "isa_model/component/regfile.h"
#include "isa_model/component/slave/memory.h"
#include "isa_model/component/slave/clint.h"

namespace isa_model
{
    enum class op_unit_t
    {
        alu,
        bru,
        csr,
        div,
        mul,
        lsu
    };
    
    enum class arg_src_t
    {
        reg,
        imm,
        disable
    };
    
    enum class op_t
    {
        add,
        addi,
        _and,
        andi,
        auipc,
        csrrc,
        csrrci,
        csrrs,
        csrrsi,
        csrrw,
        csrrwi,
        ebreak,
        ecall,
        fence,
        fence_i,
        lui,
        _or,
        ori,
        sll,
        slli,
        slt,
        slti,
        sltiu,
        sltu,
        sra,
        srai,
        srl,
        srli,
        sub,
        _xor,
        xori,
        beq,
        bge,
        bgeu,
        blt,
        bltu,
        bne,
        jal,
        jalr,
        div,
        divu,
        rem,
        remu,
        lb,
        lbu,
        lh,
        lhu,
        lw,
        sb,
        sh,
        sw,
        mul,
        mulh,
        mulhsu,
        mulhu,
        mret
    };
    
    enum class alu_op_t
    {
        add,
        _and,
        auipc,
        ebreak,
        ecall,
        fence,
        fence_i,
        lui,
        _or,
        sll,
        slt,
        sltu,
        sra,
        srl,
        sub,
        _xor
    };
    
    enum class bru_op_t
    {
        beq,
        bge,
        bgeu,
        blt,
        bltu,
        bne,
        jal,
        jalr,
        mret
    };
    
    enum class div_op_t
    {
        div,
        divu,
        rem,
        remu
    };
    
    enum class lsu_op_t
    {
        lb,
        lbu,
        lh,
        lhu,
        lw,
        sb,
        sh,
        sw
    };
    
    enum class mul_op_t
    {
        mul,
        mulh,
        mulhsu,
        mulhu
    };
    
    enum class csr_op_t
    {
        csrrc,
        csrrs,
        csrrw
    };
    
    typedef struct fetch_decode_pack_t : public if_print_t
    {
        uint32_t pc = 0;
        uint32_t value = 0;
        bool has_exception = false;
        riscv_exception_t exception_id = riscv_exception_t::instruction_address_misaligned;
        uint32_t exception_value = 0;
        
        virtual json get_json()
        {
            json t;
            t["pc"] = pc;
            t["value"] = value;
            t["has_exception"] = has_exception;
            t["exception_id"] = outenum(exception_id);
            t["exception_value"] = exception_value;
            return t;
        }
    }fetch_decode_pack_t;
    
    typedef struct decode_execute_pack_t : public if_print_t
    {
        uint32_t value = 0;
        bool valid = false;
        uint32_t pc = 0;
        uint32_t imm = 0;
        bool has_exception = false;
        riscv_exception_t exception_id = riscv_exception_t::instruction_address_misaligned;
        uint32_t exception_value = 0;
        uint32_t rs1 = 0;
        arg_src_t arg1_src = arg_src_t::reg;
        uint32_t src1_value = 0;
        uint32_t rs2 = 0;
        arg_src_t arg2_src = arg_src_t::reg;
        uint32_t src2_value = 0;
        uint32_t rd = 0;
        bool rd_enable = false;
        uint32_t rd_value = 0;
        uint32_t csr = 0;
        op_t op = op_t::add;
        op_unit_t op_unit = op_unit_t::alu;
        
        union
        {
            alu_op_t alu_op = alu_op_t::add;
            bru_op_t bru_op;
            div_op_t div_op;
            lsu_op_t lsu_op;
            mul_op_t mul_op;
            csr_op_t csr_op;
        }sub_op;
        
        virtual json get_json()
        {
            json t;
            t["value"] = value;
            t["valid"] = valid;
            t["pc"] = pc;
            t["imm"] = imm;
            t["has_exception"] = has_exception;
            t["exception_id"] = outenum(exception_id);
            t["exception_value"] = exception_value;
            t["rs1"] = rs1;
            t["arg1_src"] = arg1_src;
            t["src1_value"] = src1_value;
            t["rs2"] = rs2;
            t["arg2_src"] = outenum(arg2_src);
            t["src2_value"] = src2_value;
            t["rd"] = rd;
            t["rd_enable"] = rd_enable;
            t["rd_value"] = rd_value;
            t["csr"] = csr;
            t["op"] = outenum(op);
            t["op_unit"] = outenum(op_unit);
            
            switch(op_unit)
            {
                case op_unit_t::alu:
                    t["sub_op"] = outenum(sub_op.alu_op);
                    break;
                
                case op_unit_t::bru:
                    t["sub_op"] = outenum(sub_op.bru_op);
                    break;
                
                case op_unit_t::csr:
                    t["sub_op"] = outenum(sub_op.csr_op);
                    break;
                
                case op_unit_t::div:
                    t["sub_op"] = outenum(sub_op.div_op);
                    break;
                
                case op_unit_t::mul:
                    t["sub_op"] = outenum(sub_op.mul_op);
                    break;
    
                case op_unit_t::lsu:
                    t["sub_op"] = outenum(sub_op.lsu_op);
                    break;
                
                default:
                    t["sub_op"] = "<Unsupported>";
                    break;
            }
            
            return t;
        }
    }decode_execute_pack_t;
    
    class isa_model
    {
        private:
            static isa_model *instance;
            
            charfifo_send_fifo_t *charfifo_send_fifo;
            charfifo_rev_fifo_t *charfifo_rev_fifo;

#ifdef BRANCH_DUMP
            std::ofstream branch_dump_stream;
#endif
            
            isa_model(charfifo_send_fifo_t *charfifo_send_fifo, charfifo_rev_fifo_t *charfifo_rev_fifo);
            ~isa_model();
        
        public:
            uint64_t cpu_clock_cycle = 0;
            uint64_t committed_instruction_num = 0;
            uint64_t branch_num = 0;
            uint64_t branch_predicted = 0;
            uint64_t branch_hit = 0;
            uint64_t branch_miss = 0;
            
            uint64_t pc = INIT_PC;
            
            uint64_t commit_num = 0;//only for debugger
            
            component::bus bus;
            component::csrfile csr_file;
            component::interrupt_interface interrupt_interface;
            component::regfile<uint32_t> arch_regfile;
            component::slave::clint clint;
            
            static isa_model *create(charfifo_send_fifo_t *charfifo_send_fifo, charfifo_rev_fifo_t *charfifo_rev_fifo);
            static void destroy();
            
            void load(void *mem, size_t size);
            void reset();
            void pause_event();
            void run();
            fetch_decode_pack_t fetch();
            decode_execute_pack_t decode(const fetch_decode_pack_t &fetch_decode_pack);
            void execute(decode_execute_pack_t &decode_execute_pack);
            void execute_alu(decode_execute_pack_t &decode_execute_pack);
            void execute_bru(decode_execute_pack_t &decode_execute_pack);
            void execute_csr(decode_execute_pack_t &decode_execute_pack);
            void execute_div(decode_execute_pack_t &decode_execute_pack);
            void execute_mul(decode_execute_pack_t &decode_execute_pack);
            void execute_lsu(decode_execute_pack_t &decode_execute_pack);
    };
}