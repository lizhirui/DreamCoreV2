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
#include "common.h"
#include "config.h"
#include "pipeline_common.h"
#include "../component/branch_predictor_base.h"

namespace cycle_model::pipeline
{
    typedef struct rename_dispatch_op_info_t
    {
        inst_common_info_t inst_common_info;
        bool enable = false;//this item has op
        uint32_t value = 0;
        bool valid = false;//this item has valid op
        bool last_uop = false;//this is the last uop of an ISA instruction
        
        uint32_t rob_id = 0;
        bool rob_id_stage = false;
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
        uint32_t store_buffer_id = 0;
        uint32_t load_queue_id = 0;
        op_t op = op_t::add;
        op_unit_t op_unit = op_unit_t::alu;
        
        union
        {
            alu_op_t alu_op = alu_op_t::add;
            bru_op_t bru_op;
            csr_op_t csr_op;
            div_op_t div_op;
            mul_op_t mul_op;
            lu_op_t lu_op;
            sau_op_t sau_op;
            sdu_op_t sdu_op;
        }sub_op;
    }rename_readreg_op_info_t;
    
    typedef struct rename_dispatch_pack_t : public if_print_t
    {
        rename_dispatch_op_info_t op_info[RENAME_WIDTH];
        
        virtual void print(std::string indent)
        {
            std::string blank = "  ";
            
            for(uint32_t i = 0;i < RENAME_WIDTH;i++)
            {
                std::cout << indent << "Item " << i << ":" << std::endl;
                
                std::cout << indent << "\tenable = " << outbool(op_info[i].enable);
                std::cout << blank << "value = 0x" << fillzero(8) << outhex(op_info[i].value);
                std::cout << blank << "valid = " << outbool(op_info[i].valid);
                std::cout << blank << "last_uop = " << outbool(op_info[i].last_uop);
                std::cout << blank << "rob_id = " << op_info[i].rob_id;
                std::cout << blank << "rob_id_stage = " << outbool(op_info[i].rob_id_stage);
                std::cout << blank << "pc = 0x" << fillzero(8) << outhex(op_info[i].pc);
                std::cout << blank << "imm = 0x" << fillzero(8) << outhex(op_info[i].imm);
                std::cout << blank << "has_exception = " << outbool(op_info[i].has_exception);
                std::cout << blank << "exception_id = " << outenum(op_info[i].exception_id);
                std::cout << blank << "exception_value = 0x" << fillzero(8) << outhex(op_info[i].exception_value) << std::endl;
                
                std::cout << indent << "\trs1 = " << op_info[i].rs1;
                std::cout << blank << "arg1_src = " << outenum(op_info[i].arg1_src);
                std::cout << blank << "rs1_need_map = " << outbool(op_info[i].rs1_need_map);
                std::cout << blank << "rs1_phy = " << op_info[i].rs1_phy;
                
                std::cout << blank << "rs2 = " << op_info[i].rs2;
                std::cout << blank << "arg2_src = " << outenum(op_info[i].arg2_src);
                std::cout << blank << "rs2_need_map = " << outbool(op_info[i].rs2_need_map);
                std::cout << blank << "rs2_phy = " << op_info[i].rs2_phy << std::endl;
                
                std::cout << indent << "\trd = " << op_info[i].rd;
                std::cout << blank << "rd_enable = " << outbool(op_info[i].rd_enable);
                std::cout << blank << "need_rename = " << outbool(op_info[i].need_rename);
                std::cout << blank << "rd_phy = " << op_info[i].rd_phy;
                
                std::cout << blank << "csr = 0x" << fillzero(8) << outhex(op_info[i].csr);
                std::cout << blank << "op = " << outenum(op_info[i].op);
                std::cout << blank << "op_unit = " << outenum(op_info[i].op_unit);
                std::cout << blank << "sub_op = ";
                
                switch(op_info[i].op_unit)
                {
                    case op_unit_t::alu:
                        std::cout << outenum(op_info[i].sub_op.alu_op);
                        break;
                    
                    case op_unit_t::bru:
                        std::cout << outenum(op_info[i].sub_op.bru_op);
                        break;
                    
                    case op_unit_t::csr:
                        std::cout << outenum(op_info[i].sub_op.csr_op);
                        break;
                    
                    case op_unit_t::div:
                        std::cout << outenum(op_info[i].sub_op.div_op);
                        break;
                    
                    case op_unit_t::mul:
                        std::cout << outenum(op_info[i].sub_op.mul_op);
                        break;
    
                    case op_unit_t::lu:
                        std::cout << outenum(op_info[i].sub_op.lu_op);
                        break;
    
                    case op_unit_t::sau:
                        std::cout << outenum(op_info[i].sub_op.sau_op);
                        break;
    
                    case op_unit_t::sdu:
                        std::cout << outenum(op_info[i].sub_op.sdu_op);
                        break;
                    
                    default:
                        std::cout << "<Unsupported>";
                        break;
                }
                
                std::cout << std::endl;
            }
        }
        
        virtual json get_json()
        {
            json ret = json::array();
            
            for(uint32_t i = 0;i < RENAME_WIDTH;i++)
            {
                json t;
                t["inst_common_info"] = op_info[i].inst_common_info.get_json();
                t["enable"] = op_info[i].enable;
                t["value"] = op_info[i].value;
                t["valid"] = op_info[i].valid;
                t["last_uop"] = op_info[i].last_uop;
                t["rob_id"] = op_info[i].rob_id;
                t["rob_id_stage"] = op_info[i].rob_id_stage;
                t["pc"] = op_info[i].pc;
                t["imm"] = op_info[i].imm;
                t["has_exception"] = op_info[i].has_exception;
                t["exception_id"] = outenum(op_info[i].exception_id);
                t["exception_value"] = op_info[i].exception_value;
                t["branch_predictor_info_pack"] = op_info[i].branch_predictor_info_pack.get_json();
                t["rs1"] = op_info[i].rs1;
                t["arg1_src"] = outenum(op_info[i].arg1_src);
                t["rs1_need_map"] = op_info[i].rs1_need_map;
                t["rs1_phy"] = op_info[i].rs1_phy;
                t["rs2"] = op_info[i].rs2;
                t["arg2_src"] = outenum(op_info[i].arg2_src);
                t["rs2_need_map"] = op_info[i].rs2_need_map;
                t["rs2_phy"] = op_info[i].rs2_phy;
                t["rd"] = op_info[i].rd;
                t["rd_enable"] = op_info[i].rd_enable;
                t["need_rename"] = op_info[i].need_rename;
                t["rd_phy"] = op_info[i].rd_phy;
                t["csr"] = op_info[i].csr;
                t["op"] = outenum(op_info[i].op);
                t["op_unit"] = outenum(op_info[i].op_unit);
                
                switch(op_info[i].op_unit)
                {
                    case op_unit_t::alu:
                        t["sub_op"] = outenum(op_info[i].sub_op.alu_op);
                        break;
                    
                    case op_unit_t::bru:
                        t["sub_op"] = outenum(op_info[i].sub_op.bru_op);
                        break;
                    
                    case op_unit_t::csr:
                        t["sub_op"] = outenum(op_info[i].sub_op.csr_op);
                        break;
                    
                    case op_unit_t::div:
                        t["sub_op"] = outenum(op_info[i].sub_op.div_op);
                        break;
                    
                    case op_unit_t::mul:
                        t["sub_op"] = outenum(op_info[i].sub_op.mul_op);
                        break;
    
                    case op_unit_t::lu:
                        t["sub_op"] = outenum(op_info[i].sub_op.lu_op);
                        break;
    
                    case op_unit_t::sau:
                        t["sub_op"] = outenum(op_info[i].sub_op.sau_op);
                        break;
    
                    case op_unit_t::sdu:
                        t["sub_op"] = outenum(op_info[i].sub_op.sdu_op);
                        break;
                    
                    default:
                        t["sub_op"] = "<Unsupported>";
                        break;
                }
                
                ret.push_back(t);
            }
            
            return ret;
        }
    }rename_issue_pack_t;
}