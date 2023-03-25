/*
 * Copyright lizhirui
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-11-26     lizhirui     the first version
 */

#include "common.h"
#include "cycle_model/pipeline/execute/sdu.h"
#include "cycle_model/component/handshake_dff.h"
#include "cycle_model/pipeline/lsu_readreg_execute.h"
#include "cycle_model/pipeline/execute_wb.h"
#include "cycle_model/component/bus.h"
#include "cycle_model/component/store_buffer.h"
#include "cycle_model/component/slave/clint.h"
#include "cycle_model/component/age_compare.h"

namespace cycle_model::pipeline::execute
{
    sdu::sdu(global_inst *global, uint32_t id, component::handshake_dff<lsu_readreg_execute_pack_t> *readreg_sdu_hdff, component::port<execute_wb_pack_t> *sdu_wb_port, component::store_buffer *store_buffer) : tdb(TRACE_EXECUTE_LSU)
    {
        this->global = global;
        this->id = id;
        this->readreg_sdu_hdff = readreg_sdu_hdff;
        this->sdu_wb_port = sdu_wb_port;
        this->store_buffer = store_buffer;
        this->sdu::reset();
    }
    
    void sdu::reset()
    {
    
    }
    
    void sdu::run(const execute::bru_feedback_pack_t &bru_feedback_pack, const execute::sau_feedback_pack_t &sau_feedback_pack, const commit_feedback_pack_t &commit_feedback_pack)
    {
        execute_wb_pack_t send_pack;
        lsu_readreg_execute_pack_t rev_pack;
        component::store_buffer_item_t item;
        
        if(!commit_feedback_pack.flush)
        {
            if(readreg_sdu_hdff->pop(&rev_pack))
            {
                if(bru_feedback_pack.flush && (component::age_compare(rev_pack.rob_id, rev_pack.rob_id_stage) < component::age_compare(bru_feedback_pack.rob_id, bru_feedback_pack.rob_id_stage)))
                {
                    goto exit;
                }
    
                if(sau_feedback_pack.flush && (component::age_compare(rev_pack.rob_id, rev_pack.rob_id_stage) <= component::age_compare(sau_feedback_pack.rob_id, sau_feedback_pack.rob_id_stage)))
                {
                    goto exit;
                }
                
                verify_only(rev_pack.valid);
                send_pack.inst_common_info = rev_pack.inst_common_info;
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
                send_pack.branch_predictor_info_pack.predicted = false;
                
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
                
                if(rev_pack.enable)
                {
                    verify_only(rev_pack.valid);
                    verify_only(rev_pack.op_unit == op_unit_t::sdu);
                    
                    if(!rev_pack.has_exception)
                    {
                        switch(rev_pack.sub_op.sdu_op)
                        {
                            case sdu_op_t::sb:
                                item = store_buffer->get_item(rev_pack.store_buffer_id);
                                item.data_valid = true;
                                item.data = rev_pack.src2_value & 0xff;
                                item.committed = false;
                                item.pc = rev_pack.pc;
                                item.rob_id = rev_pack.rob_id;
                                item.rob_id_stage = rev_pack.rob_id_stage;
                                item.cycle = get_cpu_clock_cycle();//only for debug
                                store_buffer->set_item(rev_pack.store_buffer_id, item);
                                break;
    
                            case sdu_op_t::sh:
                                item.data_valid = true;
                                item.data = rev_pack.src2_value & 0xffff;
                                item.committed = false;
                                item.pc = rev_pack.pc;
                                item.rob_id = rev_pack.rob_id;
                                item.rob_id_stage = rev_pack.rob_id_stage;
                                item.cycle = get_cpu_clock_cycle();//only for debug
                                store_buffer->set_item(rev_pack.store_buffer_id, item);
                                break;
    
                            case sdu_op_t::sw:
                                item.data_valid = true;
                                item.data = rev_pack.src2_value;
                                item.committed = false;
                                item.pc = rev_pack.pc;
                                item.rob_id = rev_pack.rob_id;
                                item.rob_id_stage = rev_pack.rob_id_stage;
                                item.cycle = get_cpu_clock_cycle();//only for debug
                                store_buffer->set_item(rev_pack.store_buffer_id, item);
                                break;
                            
                            default:
                                verify_only(0);
                                break;
                        }
                    }
                }
            }
        }
        
        exit:
        sdu_wb_port->set(send_pack);
    }
    
    json sdu::get_json()
    {
        json t;
        return t;
    }
}