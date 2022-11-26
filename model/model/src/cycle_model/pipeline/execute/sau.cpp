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
#include "cycle_model/pipeline/execute/sau.h"
#include "cycle_model/component/handshake_dff.h"
#include "cycle_model/pipeline/lsu_readreg_execute.h"
#include "cycle_model/pipeline/execute_wb.h"
#include "cycle_model/component/bus.h"
#include "cycle_model/component/store_buffer.h"
#include "cycle_model/component/slave/clint.h"
#include "cycle_model/component/age_compare.h"

namespace cycle_model::pipeline::execute
{
    sau::sau(global_inst *global, uint32_t id, component::handshake_dff<lsu_readreg_execute_pack_t> *readreg_sau_hdff, component::port<execute_wb_pack_t> *sau_wb_port, component::store_buffer *store_buffer) : tdb(TRACE_EXECUTE_LSU)
    {
        this->global = global;
        this->id = id;
        this->readreg_sau_hdff = readreg_sau_hdff;
        this->sau_wb_port = sau_wb_port;
        this->store_buffer = store_buffer;
        this->sau::reset();
    }
    
    void sau::reset()
    {
    
    }
    
    void sau::run(const execute::bru_feedback_pack_t &bru_feedback_pack, const commit_feedback_pack_t &commit_feedback_pack)
    {
        execute_wb_pack_t send_pack;
        lsu_readreg_execute_pack_t rev_pack;
        uint32_t addr = 0;
        
        if(!commit_feedback_pack.flush)
        {
            if(readreg_sau_hdff->pop(&rev_pack))
            {
                if(bru_feedback_pack.flush && (component::age_compare(rev_pack.rob_id, rev_pack.rob_id_stage) < component::age_compare(bru_feedback_pack.rob_id, bru_feedback_pack.rob_id_stage)))
                {
                    goto exit;
                }
    
                verify_only(rev_pack.valid);
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
                    verify_only(rev_pack.op_unit == op_unit_t::sau);
                    
                    if(!rev_pack.has_exception)
                    {
                        addr = rev_pack.src1_value + rev_pack.imm;
                        send_pack.exception_value = addr;
                        
                        switch(rev_pack.sub_op.sau_op)
                        {
                            case sau_op_t::stab:
                                send_pack.has_exception = !component::bus::check_align(addr, 1);
                                send_pack.exception_id = !component::bus::check_align(addr, 1) ? riscv_exception_t::store_amo_address_misaligned : riscv_exception_t::store_amo_access_fault;
                                store_buffer->write_addr(rev_pack.store_buffer_id, addr, 1, true);
                                break;
                            
                            case sau_op_t::stah:
                                send_pack.has_exception = !component::bus::check_align(addr, 2);
                                send_pack.exception_id = !component::bus::check_align(addr, 2) ? riscv_exception_t::store_amo_address_misaligned : riscv_exception_t::store_amo_access_fault;
                                store_buffer->write_addr(rev_pack.store_buffer_id, addr, 2, true);
                                break;
                            
                            case sau_op_t::staw:
                                send_pack.has_exception = !component::bus::check_align(addr, 4);
                                send_pack.exception_id = !component::bus::check_align(addr, 4) ? riscv_exception_t::store_amo_address_misaligned : riscv_exception_t::store_amo_access_fault;
                                store_buffer->write_addr(rev_pack.store_buffer_id, addr, 4, true);
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
        sau_wb_port->set(send_pack);
    }
    
    json sau::get_json()
    {
        json t;
        return t;
    }
}