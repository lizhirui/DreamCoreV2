/*
 * Copyright lizhirui
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-10-15     lizhirui     the first version
 */

#pragma once
//to prevent from error of clion
#ifndef __INTEGER_ISSUE_H__
#define __INTEGER_ISSUE_H__
namespace cycle_model::pipeline
{
    typedef struct lsu_issue_output_feedback_pack_t lsu_issue_output_feedback_pack_t;
}

#include "common.h"
#include "config.h"
#include "../component/branch_predictor_base.h"
#include "../component/port.h"
#include "../component/ooo_issue_queue.h"
#include "../component/regfile.h"
#include "dispatch_issue.h"
#include "integer_issue_readreg.h"
#include "integer_readreg.h"
#include "lsu_issue.h"
#include "wb.h"
#include "commit.h"

namespace cycle_model::pipeline
{
    typedef struct integer_issue_output_feedback_pack_t : if_print_t
    {
        bool wakeup_valid[INTEGER_ISSUE_WIDTH] = {false};
        uint32_t wakeup_rd[INTEGER_ISSUE_WIDTH] = {0};
        uint32_t wakeup_shift[INTEGER_ISSUE_WIDTH] = {0};
        
        virtual json get_json()
        {
            json t;
            json t1 = json::array();
            json t2 = json::array();
            json t3 = json::array();
            
            for(uint32_t i = 0;i < INTEGER_ISSUE_WIDTH;i++)
            {
                t1.push_back(wakeup_valid[i]);
                t2.push_back(wakeup_rd[i]);
                t3.push_back(wakeup_shift[i]);
            }
            
            t["wakeup_valid"] = t1;
            t["wakeup_rd"] = t2;
            t["wakeup_shift"] = t3;
            return t;
        }
    }integer_issue_output_feedback_pack_t;
    
    typedef struct integer_issue_feedback_pack_t : if_print_t
    {
        bool stall = false;
        
        virtual json get_json()
        {
            json t;
            t["stall"] = stall;
            return t;
        }
    }integer_issue_feedback_pack_t;
    
    class integer_issue : public if_print_t, public if_reset_t
    {
        private:
            typedef struct issue_queue_item_t : public if_print_t
            {
                bool enable = false;//this item has op
                uint32_t value = 0;
                bool valid = false;//this item has valid op
                bool last_uop = false;//this is the last uop of an ISA instruction
                
                uint32_t rob_id = 0;
                uint32_t pc = 0;
                uint32_t imm = 0;
                bool has_exception = false;
                riscv_exception_t exception_id = riscv_exception_t::instruction_address_misaligned;
                uint32_t exception_value = 0;
                component::branch_predictor_info_pack_t branch_predictor_info_pack;
                
                uint32_t rs1 = 0;
                arg_src_t arg1_src = arg_src_t::reg;
                bool rs1_need_map = false;
                uint32_t rs1_phy = 0;
                
                uint32_t rs2 = 0;
                arg_src_t arg2_src = arg_src_t::reg;
                bool rs2_need_map = false;
                uint32_t rs2_phy = 0;
                
                uint32_t rd = 0;
                bool rd_enable = false;
                bool need_rename = false;
                uint32_t rd_phy = 0;
                
                uint32_t csr = 0;
                op_t op = op_t::add;
                op_unit_t op_unit = op_unit_t::alu;
                
                union
                {
                    alu_op_t alu_op = alu_op_t::add;
                    bru_op_t bru_op;
                    csr_op_t csr_op;
                    div_op_t div_op;
                    mul_op_t mul_op;
                    lsu_op_t lsu_op;
                }sub_op;
                
                virtual void print(std::string indent)
                {
                    std::string blank = "  ";
                    
                    std::cout << indent << "\tenable = " << outbool(enable);
                    std::cout << blank << "value = 0x" << fillzero(8) << outhex(value);
                    std::cout << blank << "valid = " << outbool(valid);
                    std::cout << blank << "last_uop = " << outbool(last_uop);
                    std::cout << blank << "rob_id = " << rob_id;
                    std::cout << blank << "pc = 0x" << fillzero(8) << outhex(pc);
                    std::cout << blank << "imm = 0x" << fillzero(8) << outhex(imm);
                    std::cout << blank << "has_exception = " << outbool(has_exception);
                    std::cout << blank << "exception_id = " << outenum(exception_id);
                    std::cout << blank << "exception_value = 0x" << fillzero(8) << outhex(exception_value) << std::endl;
                    
                    std::cout << indent << "\trs1 = " << rs1;
                    std::cout << blank << "arg1_src = " << outenum(arg1_src);
                    std::cout << blank << "rs1_need_map = " << outbool(rs1_need_map);
                    std::cout << blank << "rs1_phy = " << rs1_phy;
                    
                    std::cout << indent << "\trs2 = " << rs2;
                    std::cout << blank << "arg2_src = " << outenum(arg2_src);
                    std::cout << blank << "rs2_need_map = " << outbool(rs2_need_map);
                    std::cout << blank << "rs2_phy = " << rs2_phy;
                    
                    std::cout << blank << "rd = " << rd;
                    std::cout << blank << "rd_enable = " << outbool(rd_enable);
                    std::cout << blank << "need_rename = " << outbool(need_rename);
                    std::cout << blank << "rd_phy = " << rd_phy << std::endl;
                    
                    std::cout << indent << "\tcsr = 0x" << fillzero(8) << outhex(csr);
                    std::cout << blank << "op = " << outenum(op);
                    std::cout << blank << "op_unit = " << outenum(op_unit);
                    std::cout << blank << "sub_op = ";
                    
                    switch(op_unit)
                    {
                        case op_unit_t::alu:
                            std::cout << outenum(sub_op.alu_op);
                            break;
                        
                        case op_unit_t::bru:
                            std::cout << outenum(sub_op.bru_op);
                            break;
                        
                        case op_unit_t::csr:
                            std::cout << outenum(sub_op.csr_op);
                            break;
                        
                        case op_unit_t::div:
                            std::cout << outenum(sub_op.div_op);
                            break;
                        
                        case op_unit_t::mul:
                            std::cout << outenum(sub_op.mul_op);
                            break;
    
                        case op_unit_t::lsu:
                            std::cout << outenum(sub_op.lsu_op);
                            break;
                        
                        default:
                            std::cout << "<Unsupported>";
                            break;
                    }
                    
                    std::cout << std::endl;
                }
                
                virtual json get_json()
                {
                    json t;
                    t["enable"] = enable;
                    t["value"] = value;
                    t["valid"] = valid;
                    t["last_uop"] = last_uop;
                    t["rob_id"] = rob_id;
                    t["pc"] = pc;
                    t["imm"] = imm;
                    t["has_exception"] = has_exception;
                    t["exception_id"] = outenum(exception_id);
                    t["exception_value"] = exception_value;
                    t["branch_predictor_info_pack"] = branch_predictor_info_pack.get_json();
                    t["rs1"] = rs1;
                    t["arg1_src"] = outenum(arg1_src);
                    t["rs1_need_map"] = rs1_need_map;
                    t["rs1_phy"] = rs1_phy;
                    t["rs2"] = rs2;
                    t["arg2_src"] = outenum(arg2_src);
                    t["rs2_need_map"] = rs2_need_map;
                    t["rs2_phy"] = rs2_phy;
                    t["rd"] = rd;
                    t["rd_enable"] = rd_enable;
                    t["need_rename"] = need_rename;
                    t["rd_phy"] = rd_phy;
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
            }issue_queue_item_t;
        
            global_inst *global;
            component::port<dispatch_issue_pack_t> *dispatch_integer_issue_port;
            component::port<integer_issue_readreg_pack_t> *integer_issue_readreg_port;
            
            component::regfile<uint32_t> *phy_regfile;
            
            component::ooo_issue_queue<issue_queue_item_t> issue_q;
            bool busy = false;
            dispatch_issue_pack_t hold_rev_pack;
            
            uint32_t alu_idle[ALU_UNIT_NUM] = {1};
            uint32_t bru_idle[BRU_UNIT_NUM] = {1};
            uint32_t csr_idle[CSR_UNIT_NUM] = {1};
            uint32_t div_idle[DIV_UNIT_NUM] = {1};
            uint32_t mul_idle[MUL_UNIT_NUM] = {1};
        
            uint32_t alu_idle_shift[ALU_UNIT_NUM] = {0};
            uint32_t bru_idle_shift[BRU_UNIT_NUM] = {0};
            uint32_t csr_idle_shift[CSR_UNIT_NUM] = {0};
            uint32_t div_idle_shift[DIV_UNIT_NUM] = {0};
            uint32_t mul_idle_shift[MUL_UNIT_NUM] = {0};
            
            uint32_t wakeup_shift_src1[INTEGER_ISSUE_QUEUE_SIZE] = {0};
            bool src1_ready[INTEGER_ISSUE_QUEUE_SIZE] = {false};
        
            uint32_t wakeup_shift_src2[INTEGER_ISSUE_QUEUE_SIZE] = {0};
            bool src2_ready[INTEGER_ISSUE_QUEUE_SIZE] = {false};
            
            uint32_t port_index[INTEGER_ISSUE_QUEUE_SIZE] = {0};
            uint32_t op_unit_seq[INTEGER_ISSUE_QUEUE_SIZE] = {0};//one-hot
            uint32_t rob_id[INTEGER_ISSUE_QUEUE_SIZE] = {0};
            uint32_t rob_id_stage[INTEGER_ISSUE_QUEUE_SIZE] = {0};
            uint32_t wakeup_rd[INTEGER_ISSUE_QUEUE_SIZE] = {0};
            bool wakeup_rd_valid[INTEGER_ISSUE_QUEUE_SIZE] = {false};
            uint32_t wakeup_shift[INTEGER_ISSUE_QUEUE_SIZE] = {0};
            uint32_t new_idle_shift[INTEGER_ISSUE_QUEUE_SIZE] = {0};
            
            uint32_t next_port_index = 0;
            
            trace::trace_database tdb;
            
            static uint32_t latency_to_wakeup_shift(uint32_t latency);
            static uint32_t latency_to_idle_shift(uint32_t latency);
        
        public:
            integer_issue(global_inst *global, component::port<dispatch_issue_pack_t> *dispatch_integer_issue_port, component::port<integer_issue_readreg_pack_t> *integer_issue_readreg_port, component::regfile<uint32_t> *phy_regfile);
            virtual void reset();
            integer_issue_output_feedback_pack_t run_output(const commit_feedback_pack_t &commit_feedback_pack);
            void run_wakeup(const integer_issue_output_feedback_pack_t &integer_issue_output_feedback_pack, const lsu_issue_output_feedback_pack_t &lsu_issue_output_feedback_pack, const execute_feedback_pack_t &execute_feedback_pack, const commit_feedback_pack_t &commit_feedback_pack);
            integer_issue_feedback_pack_t run_input(const execute_feedback_pack_t &execute_feedback_pack, const wb_feedback_pack_t &wb_feedback_pack, const commit_feedback_pack_t &commit_feedback_pack);
            virtual void print(std::string indent);
            virtual json get_json();
    };
}
#endif