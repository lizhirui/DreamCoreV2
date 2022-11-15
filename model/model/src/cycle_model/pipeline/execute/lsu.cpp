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
#include "cycle_model/pipeline/execute/lsu.h"
#include "cycle_model/component/handshake_dff.h"
#include "cycle_model/pipeline/lsu_readreg_execute.h"
#include "cycle_model/pipeline/execute_wb.h"
#include "cycle_model/component/bus.h"
#include "cycle_model/component/store_buffer.h"
#include "cycle_model/component/slave/clint.h"

namespace cycle_model::pipeline::execute
{
    lsu::lsu(uint32_t id, component::handshake_dff<lsu_readreg_execute_pack_t> *readreg_lsu_hdff, component::port<execute_wb_pack_t> *lsu_wb_port, component::bus *bus, component::store_buffer *store_buffer, component::slave::clint *clint) : tdb(TRACE_EXECUTE_LSU)
    {
        this->id = id;
        this->readreg_lsu_hdff = readreg_lsu_hdff;
        this->lsu_wb_port = lsu_wb_port;
        this->bus = bus;
        this->store_buffer = store_buffer;
        this->clint = clint;
        this->lsu::reset();
    }
    
    void lsu::reset()
    {
        this->l2_stall = false;
        this->l2_rev_pack = lsu_readreg_execute_pack_t();
        this->l2_addr = 0;
    }
    
    execute_feedback_channel_t lsu::run(commit_feedback_pack_t commit_feedback_pack)
    {
        //level 2 pipeline: get memory data/push store_buffer
        execute_wb_pack_t send_pack;
        
        this->l2_stall = false;//cancel l2_stall signal temporarily
        
        if(l2_rev_pack.enable && !commit_feedback_pack.flush)
        {
            verify_only(l2_rev_pack.valid);
            send_pack.enable = l2_rev_pack.enable;
            send_pack.value = l2_rev_pack.value;
            send_pack.valid = l2_rev_pack.valid;
            send_pack.last_uop = l2_rev_pack.last_uop;
            send_pack.rob_id = l2_rev_pack.rob_id;
            send_pack.pc = l2_rev_pack.pc;
            send_pack.imm = l2_rev_pack.imm;
            send_pack.has_exception = l2_rev_pack.has_exception;
            send_pack.exception_id = l2_rev_pack.exception_id;
            send_pack.exception_value = l2_rev_pack.exception_value;
            send_pack.branch_predictor_info_pack.predicted = false;
            
            send_pack.rs1 = l2_rev_pack.rs1;
            send_pack.arg1_src = l2_rev_pack.arg1_src;
            send_pack.rs1_need_map = l2_rev_pack.rs1_need_map;
            send_pack.rs1_phy = l2_rev_pack.rs1_phy;
            send_pack.src1_value = l2_rev_pack.src1_value;
            
            send_pack.rs2 = l2_rev_pack.rs2;
            send_pack.arg2_src = l2_rev_pack.arg2_src;
            send_pack.rs2_need_map = l2_rev_pack.rs2_need_map;
            send_pack.rs2_phy = l2_rev_pack.rs2_phy;
            send_pack.src2_value = l2_rev_pack.src2_value;
            
            send_pack.rd = l2_rev_pack.rd;
            send_pack.rd_enable = l2_rev_pack.rd_enable;
            send_pack.need_rename = l2_rev_pack.need_rename;
            send_pack.rd_phy = l2_rev_pack.rd_phy;
            
            send_pack.csr = l2_rev_pack.csr;
            send_pack.op = l2_rev_pack.op;
            send_pack.op_unit = l2_rev_pack.op_unit;
            memcpy((void *)&send_pack.sub_op, (void *)&l2_rev_pack.sub_op, sizeof(l2_rev_pack.sub_op));
            
            if(!l2_rev_pack.has_exception)
            {
                component::store_buffer_item_t item;
                uint32_t bus_value = 0;
                uint32_t feedback_value = 0;
                bool load_stall = false;
                bool store_stall = false;
                
                switch(l2_rev_pack.sub_op.lsu_op)
                {
                    case lsu_op_t::lb:
                        load_stall = !bus->get_data_value(&bus_value);
                        feedback_value = (bus_value & (~l2_feedback_mask)) | l2_feedback_value;
                        send_pack.rd_value = sign_extend(feedback_value, 8);
                        break;
                    
                    case lsu_op_t::lbu:
                        load_stall = !bus->get_data_value(&bus_value);
                        feedback_value = (bus_value & (~l2_feedback_mask)) | l2_feedback_value;
                        send_pack.rd_value = feedback_value;
                        break;
                    
                    case lsu_op_t::lh:
                        load_stall = !bus->get_data_value(&bus_value);
                        feedback_value = (bus_value & (~l2_feedback_mask)) | l2_feedback_value;
                        send_pack.rd_value = sign_extend(feedback_value, 16);
                        break;
                    
                    case lsu_op_t::lhu:
                        load_stall = !bus->get_data_value(&bus_value);
                        feedback_value = (bus_value & (~l2_feedback_mask)) | l2_feedback_value;
                        send_pack.rd_value = feedback_value;
                        break;
                    
                    case lsu_op_t::lw:
                        load_stall = !bus->get_data_value(&bus_value);
                        feedback_value = (bus_value & (~l2_feedback_mask)) | l2_feedback_value;
                        send_pack.rd_value = feedback_value;
                        break;
                    
                    case lsu_op_t::sb:
                        if(store_buffer->producer_is_full())
                        {
                            store_stall = true;
                        }
                        else
                        {
                            item.enable = true;
                            item.addr = l2_addr;
                            item.size = 1;
                            item.data = l2_rev_pack.src2_value & 0xff;
                            item.committed = false;
                            item.pc = l2_rev_pack.pc;
                            item.rob_id = l2_rev_pack.rob_id;
                            item.cycle = get_cpu_clock_cycle();//only for debug
                            store_buffer->push(item);
                        }
                        
                        break;
                    
                    case lsu_op_t::sh:
                        if(store_buffer->producer_is_full())
                        {
                            store_stall = true;
                        }
                        else
                        {
                            item.enable = true;
                            item.addr = l2_addr;
                            item.size = 2;
                            item.data = l2_rev_pack.src2_value & 0xffff;
                            item.committed = false;
                            item.pc = l2_rev_pack.pc;
                            item.rob_id = l2_rev_pack.rob_id;
                            item.cycle = get_cpu_clock_cycle();//only for debug
                            store_buffer->push(item);
                        }
                        
                        break;
                    
                    case lsu_op_t::sw:
                        if(store_buffer->producer_is_full())
                        {
                            store_stall = true;
                        }
                        else
                        {
                            item.enable = true;
                            item.addr = l2_addr;
                            item.size = 4;
                            item.data = l2_rev_pack.src2_value;
                            item.committed = false;
                            item.pc = l2_rev_pack.pc;
                            item.rob_id = l2_rev_pack.rob_id;
                            item.cycle = get_cpu_clock_cycle();//only for debug
                            store_buffer->push(item);
                        }
                        
                        break;
                }
                
                this->l2_stall = load_stall || store_stall;//generate l2_stall signal
                
                if(load_stall || store_stall)
                {
                    send_pack = execute_wb_pack_t();
                }
            }
        }
        
        lsu_wb_port->set(send_pack);
        
        //level 1 pipeline: calculate effective address
        lsu_readreg_execute_pack_t rev_pack;
        
        if(!commit_feedback_pack.flush)
        {
            if(!l2_stall)
            {
                if(readreg_lsu_hdff->pop(&rev_pack))
                {
                    l2_addr = 0;
                    l2_rev_pack.enable = rev_pack.enable;
                    l2_rev_pack.value = rev_pack.value;
                    l2_rev_pack.valid = rev_pack.valid;
                    l2_rev_pack.last_uop = rev_pack.last_uop;
                    l2_rev_pack.rob_id = rev_pack.rob_id;
                    l2_rev_pack.pc = rev_pack.pc;
                    l2_rev_pack.imm = rev_pack.imm;
                    l2_rev_pack.has_exception = rev_pack.has_exception;
                    l2_rev_pack.exception_id = rev_pack.exception_id;
                    l2_rev_pack.exception_value = rev_pack.exception_value;
    
                    l2_rev_pack.rs1 = rev_pack.rs1;
                    l2_rev_pack.arg1_src = rev_pack.arg1_src;
                    l2_rev_pack.rs1_need_map = rev_pack.rs1_need_map;
                    l2_rev_pack.rs1_phy = rev_pack.rs1_phy;
                    l2_rev_pack.src1_value = rev_pack.src1_value;
    
                    l2_rev_pack.rs2 = rev_pack.rs2;
                    l2_rev_pack.arg2_src = rev_pack.arg2_src;
                    l2_rev_pack.rs2_need_map = rev_pack.rs2_need_map;
                    l2_rev_pack.rs2_phy = rev_pack.rs2_phy;
                    l2_rev_pack.src2_value = rev_pack.src2_value;
    
                    l2_rev_pack.rd = rev_pack.rd;
                    l2_rev_pack.rd_enable = rev_pack.rd_enable;
                    l2_rev_pack.need_rename = rev_pack.need_rename;
                    l2_rev_pack.rd_phy = rev_pack.rd_phy;
    
                    l2_rev_pack.csr = rev_pack.csr;
                    l2_rev_pack.op = rev_pack.op;
                    l2_rev_pack.op_unit = rev_pack.op_unit;
                    memcpy(&l2_rev_pack.sub_op, &rev_pack.sub_op, sizeof(rev_pack.sub_op));
                    
                    if(rev_pack.enable)
                    {
                        verify_only(rev_pack.valid);
                        verify_only(rev_pack.op_unit == op_unit_t::lsu);
                        clint_sync_list[rev_pack.rob_id].mtime = clint->get_mtime();
                        clint_sync_list[rev_pack.rob_id].mtimecmp = clint->get_mtimecmp();
                        clint_sync_list[rev_pack.rob_id].msip = clint->get_msip();
                        
                        if(!rev_pack.has_exception)
                        {
                            l2_addr = rev_pack.src1_value + rev_pack.imm;
                            l2_rev_pack.exception_value = l2_addr;
                            
                            switch(rev_pack.sub_op.lsu_op)
                            {
                                case lsu_op_t::lb:
                                case lsu_op_t::lbu:
                                    l2_rev_pack.has_exception = !component::bus::check_align(l2_addr, 1);
                                    l2_rev_pack.exception_id = !component::bus::check_align(l2_addr, 1) ? riscv_exception_t::load_address_misaligned : riscv_exception_t::load_access_fault;
                                    bus->read8(l2_addr);
                                    
                                    {
                                        auto feedback_param = store_buffer->get_feedback_value(l2_addr, 1);
                                        l2_feedback_value = feedback_param.first;
                                        l2_feedback_mask = feedback_param.second;
                                    }
                                    
                                    break;
                                    
                                case lsu_op_t::lh:
                                case lsu_op_t::lhu:
                                    l2_rev_pack.has_exception = !component::bus::check_align(l2_addr, 2);
                                    l2_rev_pack.exception_id = !component::bus::check_align(l2_addr, 2) ? riscv_exception_t::load_address_misaligned : riscv_exception_t::load_access_fault;
                                    bus->read16(l2_addr);
                                    
                                    {
                                        auto feedback_param = store_buffer->get_feedback_value(l2_addr, 2);
                                        l2_feedback_value = feedback_param.first;
                                        l2_feedback_mask = feedback_param.second;
                                    }
                                    
                                    break;
                                    
                                case lsu_op_t::lw:
                                    l2_rev_pack.has_exception = !component::bus::check_align(l2_addr, 4);
                                    l2_rev_pack.exception_id = !component::bus::check_align(l2_addr, 4) ? riscv_exception_t::load_address_misaligned : riscv_exception_t::load_access_fault;
                                    bus->read32(l2_addr);
        
                                    {
                                        auto feedback_param = store_buffer->get_feedback_value(l2_addr, 4);
                                        l2_feedback_value = feedback_param.first;
                                        l2_feedback_mask = feedback_param.second;
                                    }
                                    
                                    break;
                                    
                                case lsu_op_t::sb:
                                    l2_rev_pack.has_exception = !component::bus::check_align(l2_addr, 1);
                                    l2_rev_pack.exception_id = !component::bus::check_align(l2_addr, 1) ? riscv_exception_t::store_amo_address_misaligned : riscv_exception_t::store_amo_access_fault;
                                    break;
                                    
                                case lsu_op_t::sh:
                                    l2_rev_pack.has_exception = !component::bus::check_align(l2_addr, 2);
                                    l2_rev_pack.exception_id = !component::bus::check_align(l2_addr, 2) ? riscv_exception_t::store_amo_address_misaligned : riscv_exception_t::store_amo_access_fault;
                                    break;
                                    
                                case lsu_op_t::sw:
                                    l2_rev_pack.has_exception = !component::bus::check_align(l2_addr, 4);
                                    l2_rev_pack.exception_id = !component::bus::check_align(l2_addr, 4) ? riscv_exception_t::store_amo_address_misaligned : riscv_exception_t::store_amo_access_fault;
                                    break;
                                    
                                default:
                                    verify_only(0);
                                    break;
                            }
                        }
                    }
                }
                else
                {
                    l2_addr = 0;
                    l2_rev_pack = lsu_readreg_execute_pack_t();
                }
            }
        }
        else
        {
            l2_addr = 0;
            l2_rev_pack = lsu_readreg_execute_pack_t();
        }

        execute_feedback_channel_t feedback_pack;
        feedback_pack.enable = send_pack.enable && send_pack.valid && send_pack.need_rename && !send_pack.has_exception;
        feedback_pack.phy_id = send_pack.rd_phy;
        feedback_pack.value = send_pack.rd_value;
        return feedback_pack;
    }
    
    json lsu::get_json()
    {
        json t;
        
        t["l2_stall"] = this->l2_stall;
        t["l2_addr"] = this->l2_addr;
        t["l2_rev_pack"] = this->l2_rev_pack.get_json();
        return t;
    }
}