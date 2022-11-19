/*
 * Copyright lizhirui
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-10-15     lizhirui     the first version
 */

#include "common.h"
#include "cycle_model/pipeline/execute/bru.h"
#include "cycle_model/component/handshake_dff.h"
#include "cycle_model/pipeline/integer_readreg_execute.h"
#include "cycle_model/pipeline/execute_wb.h"
#include "cycle_model/pipeline/execute.h"

namespace cycle_model::pipeline::execute
{
    bru::bru(global_inst *global, uint32_t id, component::handshake_dff<integer_readreg_execute_pack_t> *readreg_bru_hdff, component::port<execute_wb_pack_t> *bru_wb_port, component::csrfile *csr_file, component::rat *speculative_rat, component::rob *rob, component::regfile<uint32_t> *phy_regfile, component::free_list *phy_id_free_list, component::fifo<component::checkpoint_t> *checkpoint_buffer, component::branch_predictor_set *branch_predictor_set) : tdb(TRACE_EXECUTE_BRU)
    {
        this->global = global;
        this->id = id;
        this->readreg_bru_hdff = readreg_bru_hdff;
        this->bru_wb_port = bru_wb_port;
        this->csr_file = csr_file;
        this->speculative_rat = speculative_rat;
        this->rob = rob;
        this->phy_regfile = phy_regfile;
        this->phy_id_free_list = phy_id_free_list;
        this->checkpoint_buffer = checkpoint_buffer;
        this->branch_predictor_set = branch_predictor_set;
        this->bru::reset();
    }
    
    void bru::reset()
    {
    
    }
    
    std::variant<execute_feedback_channel_t, bru_feedback_pack_t> bru::run(const commit_feedback_pack_t &commit_feedback_pack, bool need_bru_feedback_only)
    {
        execute_wb_pack_t send_pack;
        
        if(!readreg_bru_hdff->is_empty() && !commit_feedback_pack.flush)
        {
            integer_readreg_execute_pack_t rev_pack;
            
            if(need_bru_feedback_only)
            {
                verify(readreg_bru_hdff->get_data(&rev_pack));
            }
            else
            {
                verify(readreg_bru_hdff->pop(&rev_pack));
            }
            
            send_pack.enable = rev_pack.enable;
            send_pack.value = rev_pack.value;
            send_pack.valid = rev_pack.valid;
            send_pack.last_uop = rev_pack.last_uop;
            send_pack.rob_id = rev_pack.rob_id;
            send_pack.rob_id_stage = rev_pack.rob_id_stage;
            send_pack.pc = rev_pack.pc;
            send_pack.imm = rev_pack.imm;
            send_pack.has_exception = rev_pack.has_exception;
            send_pack.exception_id = rev_pack.exception_id;
            send_pack.exception_value = rev_pack.exception_value;
            send_pack.branch_predictor_info_pack = rev_pack.branch_predictor_info_pack;
            
            send_pack.rs1 = rev_pack.rs1;
            send_pack.arg1_src = rev_pack.arg1_src;
            send_pack.rs1_need_map = rev_pack.rs1_need_map;
            send_pack.rs1_phy = rev_pack.rs1_phy;
            send_pack.src1_value = rev_pack.src1_value;
            
            send_pack.rs2 = rev_pack.rs2;
            send_pack.arg2_src = rev_pack.arg2_src;
            send_pack.rs2_need_map = rev_pack.rs2_need_map;
            send_pack.rs2_phy = rev_pack.rs2_phy;
            send_pack.src2_value = rev_pack.src2_value;
            
            send_pack.rd = rev_pack.rd;
            send_pack.rd_enable = rev_pack.rd_enable;
            send_pack.need_rename = rev_pack.need_rename;
            send_pack.rd_phy = rev_pack.rd_phy;
            
            send_pack.csr = rev_pack.csr;
            send_pack.op = rev_pack.op;
            send_pack.op_unit = rev_pack.op_unit;
            memcpy((void *)&send_pack.sub_op, (void *)&rev_pack.sub_op, sizeof(rev_pack.sub_op));
            send_pack.bru_next_pc = rev_pack.pc + rev_pack.imm;
            
            if(rev_pack.enable)
            {
                verify_only(rev_pack.valid);
                verify_only(rev_pack.op_unit == op_unit_t::bru);
                
                switch(rev_pack.sub_op.bru_op)
                {
                    case bru_op_t::beq:
                        send_pack.bru_jump = rev_pack.src1_value == rev_pack.src2_value;
                        break;
                    
                    case bru_op_t::bge:
                        send_pack.bru_jump = ((int32_t)rev_pack.src1_value) >= ((int32_t)rev_pack.src2_value);
                        break;
                    
                    case bru_op_t::bgeu:
                        send_pack.bru_jump = ((uint32_t)rev_pack.src1_value) >= ((uint32_t)rev_pack.src2_value);
                        break;
                    
                    case bru_op_t::blt:
                        send_pack.bru_jump = ((int32_t)rev_pack.src1_value) < ((int32_t)rev_pack.src2_value);
                        break;
                    
                    case bru_op_t::bltu:
                        send_pack.bru_jump = ((uint32_t)rev_pack.src1_value) < ((uint32_t)rev_pack.src2_value);
                        break;
                    
                    case bru_op_t::bne:
                        send_pack.bru_jump = rev_pack.src1_value != rev_pack.src2_value;
                        break;
                    
                    case bru_op_t::jal:
                        send_pack.rd_value = rev_pack.pc + 4;
                        send_pack.bru_jump = true;
                        break;
                    
                    case bru_op_t::jalr:
                        send_pack.rd_value = rev_pack.pc + 4;
                        send_pack.bru_jump = true;
                        send_pack.bru_next_pc = (rev_pack.imm + rev_pack.src1_value) & (~0x01);
                        break;
                    
                    case bru_op_t::mret:
                        send_pack.bru_jump = true;
                        send_pack.bru_next_pc = csr_file->read_sys(CSR_MEPC);
                        break;
                }
            }
            
            if(!send_pack.bru_jump)
            {
                send_pack.bru_next_pc = rev_pack.pc + 4;
            }
        }
        
        if(!need_bru_feedback_only)
        {
            bru_wb_port->set(send_pack);
        }

        execute_feedback_channel_t feedback_pack;
        feedback_pack.enable = send_pack.enable && send_pack.valid && send_pack.need_rename && !send_pack.has_exception;
        feedback_pack.phy_id = send_pack.rd_phy;
        feedback_pack.value = send_pack.rd_value;
        bru_feedback_pack_t bru_feedback_pack;
        bru_feedback_pack.flush = send_pack.enable && send_pack.valid && !send_pack.has_exception &&
                                  ((send_pack.branch_predictor_info_pack.jump != send_pack.bru_jump) || (send_pack.branch_predictor_info_pack.next_pc != send_pack.bru_next_pc)) &&
                                  send_pack.branch_predictor_info_pack.checkpoint_id_valid;
        bru_feedback_pack.next_pc = send_pack.bru_next_pc;
        bru_feedback_pack.rob_id_stage = send_pack.rob_id_stage;
        bru_feedback_pack.rob_id = send_pack.rob_id;
        
        if(need_bru_feedback_only)
        {
            return bru_feedback_pack;
        }
        else
        {
            if(bru_feedback_pack.flush)
            {
                auto cp = checkpoint_buffer->customer_get_item(send_pack.branch_predictor_info_pack.checkpoint_id);
                speculative_rat->restore_from_checkpoint(cp);
                uint32_t new_rob_id = send_pack.rob_id;
                bool new_rob_id_stage = send_pack.rob_id_stage;
                rob->customer_get_next_id_stage(new_rob_id, new_rob_id_stage, &new_rob_id, &new_rob_id_stage);
                rob->update_wptr(new_rob_id, new_rob_id_stage);
                phy_regfile->restore(bru_feedback_pack.rob_id, bru_feedback_pack.rob_id_stage);
                checkpoint_buffer->update_wptr(cp.new_checkpoint_buffer_wptr, cp.new_checkpoint_buffer_wstage);
                phy_id_free_list->restore(cp.new_phy_id_free_list_rptr, cp.new_phy_id_free_list_rstage);
                branch_predictor_set->bi_mode.bru_speculative_update_sync(send_pack.pc, send_pack.bru_jump, send_pack.bru_next_pc, false, send_pack.branch_predictor_info_pack);
                branch_predictor_set->l1_btb.bru_speculative_update_sync(send_pack.pc, send_pack.bru_jump, send_pack.bru_next_pc, false, send_pack.branch_predictor_info_pack);
            }
            
            return feedback_pack;
        }
    }
}