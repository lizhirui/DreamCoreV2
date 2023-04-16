/*
 * Copyright lizhirui
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-10-14     lizhirui     the first version
 */

#include "common.h"
#include "config.h"
#include "network/network.h"
#include "network/network_command.h"
#include "main.h"
#include "breakpoint.h"

static std::string socket_cmd_quit(std::vector<std::string> args)
{
    if(!args.empty())
    {
        return "argerror";
    }

    set_recv_thread_stop(true);
    set_program_stop(true);
    return "ok";
}

static std::string socket_cmd_reset(std::vector<std::string> args)
{
    if(!args.empty())
    {
        return "argerror";
    }

    reset();
    return "ok";
}

static std::string socket_cmd_continue(std::vector<std::string> args)
{
    if(!args.empty())
    {
        return "argerror";
    }
    
    set_pause_state(false);
    set_step_state(false);
    set_wait_commit(false);
    recv_ioc_stop();
    return "ok";
}

static std::string socket_cmd_pause(std::vector<std::string> args)
{
    if(!args.empty())
    {
        return "argerror";
    }
    
    set_step_state(true);
    set_wait_commit(false);
    return "ok";
}

static std::string socket_cmd_step(std::vector<std::string> args)
{
    if(!args.empty())
    {
        return "argerror";
    }
    
    set_pause_state(false);
    set_step_state(true);
    set_wait_commit(false);
    recv_ioc_stop();
    return "ok";
}

static std::string socket_cmd_stepcommit(std::vector<std::string> args)
{
    if(!args.empty())
    {
        return "argerror";
    }
    
    set_pause_state(false);
    set_step_state(true);
    set_wait_commit(true);
    recv_ioc_stop();
    return "ok";
}

static std::string socket_cmd_read_memory(std::vector<std::string> args)
{
    if(args.size() != 2)
    {
        return "argerror";
    }

    uint32_t address = 0;
    uint32_t size = 0;
    std::stringstream address_str(args[0]);
    std::stringstream size_str(args[1]);
    address_str.unsetf(std::ios::dec);
    address_str.setf(std::ios::hex);
    address_str >> address;
    size_str >> size;

    std::stringstream result;

    result << std::hex << address;

    for(auto addr = address;addr < (address + size);addr++)
    {
#ifdef NEED_CYCLE_MODEL
        result << "," << std::hex << (uint32_t)cycle_model_inst->cacheable_bus.read8_sys(addr);
#else
        result << "," << std::hex << (uint32_t)isa_model_inst->bus.read8(addr, false);
#endif
    }

    return result.str();
}

static std::string socket_cmd_write_memory(std::vector<std::string> args)
{
    if(args.size() != 2)
    {
        return "argerror";
    }

    uint32_t address = 0;
    std::stringstream address_str(args[0]);
    std::string data_str = args[1];
    address_str.unsetf(std::ios::dec);
    address_str.setf(std::ios::hex);
    address_str >> address;

    for(size_t offset = 0;offset < (data_str.length() >> 1);offset++)
    {
        std::stringstream hex_str(data_str.substr(offset << 1, 2));
        hex_str.unsetf(std::ios::dec);
        hex_str.setf(std::ios::hex);
        uint32_t value = 0;
        hex_str >> value;
#ifdef NEED_CYCLE_MODEL
        cycle_model_inst->cacheable_bus.write8_sys(address + offset, (uint8_t)value);
#else
        isa_model_inst->bus.write8(address + offset, (uint8_t)value);
#endif
    }

    return "ok";
}

static std::string socket_cmd_read_archreg(std::vector<std::string> args)
{
    if(!args.empty())
    {
        return "argerror";
    }

    std::stringstream result;

    for(uint32_t i = 0;i < ARCH_REG_NUM;i++)
    {
        if(i == 0)
        {
            result << std::hex << 0;
        }
        else
        {
#ifdef NEED_CYCLE_MODEL
            uint32_t phy_id = 0;
            cycle_model_inst->retire_rat.customer_get_phy_id(i, &phy_id);
            auto v = cycle_model_inst->phy_regfile.read(phy_id);
            verify_only(cycle_model_inst->phy_regfile.read_data_valid(phy_id));
            result << "," << std::hex << v;
#else
            result << "," << std::hex << isa_model_inst->arch_regfile.read(i);
#endif
        }
    }

    return result.str();
}

static std::string socket_cmd_read_csr(std::vector<std::string> args)
{
    if(!args.empty())
    {
        return "argerror";
    }

#ifdef NEED_CYCLE_MODEL
    return cycle_model_inst->csr_file.get_info_packet();
#else
    return isa_model_inst->csr_file.get_info_packet();
#endif
}

static std::string socket_cmd_get_pc(std::vector<std::string> args)
{
    if(!args.empty())
    {
        return "argerror";
    }

    std::stringstream result;
    result << outhex(get_current_pc());
    return result.str();
}

static std::string socket_cmd_get_cycle(std::vector<std::string> args)
{
    if(!args.empty())
    {
        return "argerror";
    }

    std::stringstream result;
#ifdef NEED_CYCLE_MODEL
    result << cycle_model_inst->cpu_clock_cycle;
#else
    result << isa_model_inst->cpu_clock_cycle;
#endif
    return result.str();
}

#ifdef NEED_CYCLE_MODEL
static std::string socket_cmd_get_pipeline_status(std::vector<std::string> args)
{
    if(!args.empty())
    {
        return "argerror";
    }

    json ret;
    ret["fetch1"] = cycle_model_inst->fetch1_stage.get_json();
    ret["fetch1_fetch2"] = cycle_model_inst->fetch1_fetch2_port.get_json();
    ret["fetch2"] = cycle_model_inst->fetch2_stage.get_json();
    ret["fetch2_decode"] = cycle_model_inst->fetch2_decode_fifo.get_json();
    ret["decode_rename"] = cycle_model_inst->decode_rename_fifo.get_json();
    ret["rename_dispatch"] = cycle_model_inst->rename_dispatch_port.get_json();
    ret["dispatch"] = cycle_model_inst->dispatch_stage.get_json();
    ret["dispatch_integer_issue"] = cycle_model_inst->dispatch_integer_issue_port.get_json();
    ret["dispatch_lsu_issue"] = cycle_model_inst->dispatch_lsu_issue_port.get_json();
    ret["integer_issue"] = cycle_model_inst->integer_issue_stage.get_json();
    ret["lsu_issue"] = cycle_model_inst->lsu_issue_stage.get_json();
    ret["integer_issue_readreg"] = cycle_model_inst->integer_issue_readreg_port.get_json();
    ret["lsu_issue_readreg"] = cycle_model_inst->lsu_issue_readreg_port.get_json();
    ret["lsu_readreg"] = cycle_model_inst->lsu_readreg_stage.get_json();

    json ire;
    json ire_alu, ire_bru, ire_csr, ire_div, ire_mul;
    json lre;
    json lre_lu, lre_sau, lre_sdu;
    
    ire_alu = json::array();
    ire_bru = json::array();
    ire_csr = json::array();
    ire_div = json::array();
    ire_mul = json::array();
    lre_lu = json::array();
    lre_sau = json::array();
    lre_sdu = json::array();

    for(uint32_t i = 0;i < ALU_UNIT_NUM;i++)
    {
        ire_alu.push_back(cycle_model_inst->readreg_alu_hdff[i]->get_json());
    }

    for(uint32_t i = 0;i < BRU_UNIT_NUM;i++)
    {
        ire_bru.push_back(cycle_model_inst->readreg_bru_hdff[i]->get_json());
    }

    for(uint32_t i = 0;i < CSR_UNIT_NUM;i++)
    {
        ire_csr.push_back(cycle_model_inst->readreg_csr_hdff[i]->get_json());
    }

    for(uint32_t i = 0;i < DIV_UNIT_NUM;i++)
    {
        ire_div.push_back(cycle_model_inst->readreg_div_hdff[i]->get_json());
    }

    for(uint32_t i = 0;i < MUL_UNIT_NUM;i++)
    {
        ire_mul.push_back(cycle_model_inst->readreg_mul_hdff[i]->get_json());
    }
    
    for(uint32_t i = 0;i < LU_UNIT_NUM;i++)
    {
        lre_lu.push_back(cycle_model_inst->readreg_lu_hdff[i]->get_json());
    }
    
    for(uint32_t i = 0;i < SAU_UNIT_NUM;i++)
    {
        lre_sau.push_back(cycle_model_inst->readreg_sau_hdff[i]->get_json());
    }
    
    for(uint32_t i = 0;i < SDU_UNIT_NUM;i++)
    {
        lre_sdu.push_back(cycle_model_inst->readreg_sdu_hdff[i]->get_json());
    }

    ire["alu"] = ire_alu;
    ire["bru"] = ire_bru;
    ire["csr"] = ire_csr;
    ire["div"] = ire_div;
    ire["mul"] = ire_mul;
    ret["integer_readreg_execute"] = ire;
    lre["lu"] = lre_lu;
    lre["sau"] = lre_sau;
    lre["sdu"] = lre_sdu;
    ret["lsu_readreg_execute"] = lre;
    
    json te;
    auto te_div = json::array();
    auto te_lu = json::array();
    
    for(uint32_t i = 0;i < DIV_UNIT_NUM;i++)
    {
        te_div.push_back(cycle_model_inst->execute_div_stage[i]->get_json());
    }
    
    for(uint32_t i = 0;i < LU_UNIT_NUM;i++)
    {
        te_lu.push_back(cycle_model_inst->execute_lu_stage[i]->get_json());
    }
    
    te["div"] = te_div;
    te["lu"] = te_lu;
    ret["execute"] = te;

    json tew;
    json tew_alu, tew_bru, tew_csr, tew_div, tew_mul, tew_lu;
    tew_alu = json::array();
    tew_bru = json::array();
    tew_csr = json::array();
    tew_div = json::array();
    tew_mul = json::array();
    tew_lu = json::array();

    for(uint32_t i = 0;i < ALU_UNIT_NUM;i++)
    {
        tew_alu.push_back(cycle_model_inst->alu_wb_port[i]->get_json());
    }

    for(uint32_t i = 0;i < BRU_UNIT_NUM;i++)
    {
        tew_bru.push_back(cycle_model_inst->bru_wb_port[i]->get_json());
    }

    for(uint32_t i = 0;i < CSR_UNIT_NUM;i++)
    {
        tew_csr.push_back(cycle_model_inst->csr_wb_port[i]->get_json());
    }

    for(uint32_t i = 0;i < DIV_UNIT_NUM;i++)
    {
        tew_div.push_back(cycle_model_inst->div_wb_port[i]->get_json());
    }

    for(uint32_t i = 0;i < MUL_UNIT_NUM;i++)
    {
        tew_mul.push_back(cycle_model_inst->mul_wb_port[i]->get_json());
    }
    
    for(uint32_t i = 0;i < LU_UNIT_NUM;i++)
    {
        tew_lu.push_back(cycle_model_inst->lu_wb_port[i]->get_json());
    }

    tew["alu"] = tew_alu;
    tew["bru"] = tew_bru;
    tew["csr"] = tew_csr;
    tew["div"] = tew_div;
    tew["mul"] = tew_mul;
    tew["lu"] = tew_lu;
    ret["execute_wb"] = tew;
    json tec;
    json tec_alu, tec_bru, tec_csr, tec_div, tec_mul, tec_lu, tec_sau, tec_sdu;
    tec_alu = json::array();
    tec_bru = json::array();
    tec_csr = json::array();
    tec_div = json::array();
    tec_mul = json::array();
    tec_lu = json::array();
    tec_sau = json::array();
    tec_sdu = json::array();
    
    for(uint32_t i = 0;i < ALU_UNIT_NUM;i++)
    {
        tec_alu.push_back(cycle_model_inst->alu_commit_port[i]->get_json());
    }
    
    for(uint32_t i = 0;i < BRU_UNIT_NUM;i++)
    {
        tec_bru.push_back(cycle_model_inst->bru_commit_port[i]->get_json());
    }
    
    for(uint32_t i = 0;i < CSR_UNIT_NUM;i++)
    {
        tec_csr.push_back(cycle_model_inst->csr_commit_port[i]->get_json());
    }
    
    for(uint32_t i = 0;i < DIV_UNIT_NUM;i++)
    {
        tec_div.push_back(cycle_model_inst->div_commit_port[i]->get_json());
    }
    
    for(uint32_t i = 0;i < MUL_UNIT_NUM;i++)
    {
        tec_mul.push_back(cycle_model_inst->mul_commit_port[i]->get_json());
    }
    
    for(uint32_t i = 0;i < LU_UNIT_NUM;i++)
    {
        tec_lu.push_back(cycle_model_inst->lu_commit_port[i]->get_json());
    }
    
    for(uint32_t i = 0;i < SAU_UNIT_NUM;i++)
    {
        tec_sau.push_back(cycle_model_inst->sau_commit_port[i]->get_json());
    }
    
    for(uint32_t i = 0;i < SDU_UNIT_NUM;i++)
    {
        tec_sdu.push_back(cycle_model_inst->sdu_commit_port[i]->get_json());
    }
    
    tec["alu"] = tec_alu;
    tec["bru"] = tec_bru;
    tec["csr"] = tec_csr;
    tec["div"] = tec_div;
    tec["mul"] = tec_mul;
    tec["lu"] = tec_lu;
    tec["sau"] = tec_sau;
    tec["sdu"] = tec_sdu;
    ret["execute_commit"] = tec;
    ret["fetch2_feedback_pack"] = cycle_model_inst->fetch2_feedback_pack.get_json();
    ret["decode_feedback_pack"] = cycle_model_inst->decode_feedback_pack.get_json();
    ret["rename_feedback_pack"] = cycle_model_inst->rename_feedback_pack.get_json();
    ret["dispatch_feedback_pack"] = cycle_model_inst->dispatch_feedback_pack.get_json();
    ret["integer_issue_output_feedback_pack"] = cycle_model_inst->integer_issue_output_feedback_pack.get_json();
    ret["integer_issue_feedback_pack"] = cycle_model_inst->integer_issue_feedback_pack.get_json();
    ret["lsu_issue_feedback_pack"] = cycle_model_inst->lsu_issue_feedback_pack.get_json();
    ret["lsu_readreg_feedback_pack"] = cycle_model_inst->lsu_readreg_feedback_pack.get_json();
    ret["execute_feedback_pack"] = cycle_model_inst->execute_feedback_pack.get_json();
    ret["wb_feedback_pack"] = cycle_model_inst->wb_feedback_pack.get_json();
    ret["commit_feedback_pack"] = cycle_model_inst->commit_feedback_pack.get_json();
    ret["rob"] = cycle_model_inst->rob.get_json();
    ret["speculative_rat"] = cycle_model_inst->speculative_rat.get_json();
    ret["retire_rat"] = cycle_model_inst->retire_rat.get_json();
    ret["phy_regfile"] = cycle_model_inst->phy_regfile.get_json();
    ret["phy_id_free_list"] = cycle_model_inst->phy_id_free_list.get_json();
    ret["store_buffer"] = cycle_model_inst->store_buffer.get_json();
    return ret.dump();
}
#endif

static std::string socket_cmd_get_commit_num(std::vector<std::string> args)
{
    if(!args.empty())
    {
        return "argerror";
    }

    std::stringstream result;
#ifdef NEED_CYCLE_MODEL
    result << cycle_model_inst->rob.get_commit_num();
    cycle_model_inst->rob.clear_commit_num();
#else
    result << isa_model_inst->commit_num;
    isa_model_inst->commit_num = 0;
#endif
    return result.str();
}

static std::string socket_cmd_get_finish(std::vector<std::string> args)
{
    if(!args.empty())
    {
        return "argerror";
    }

    std::stringstream result;
#ifdef NEED_CYCLE_MODEL
    result << (int)cycle_model_inst->csr_file.read_sys(CSR_FINISH);
#else
    result << (int)isa_model_inst->csr_file.read_sys(CSR_FINISH);
#endif
    return result.str();
}

static std::string socket_cmd_get_mode(std::vector<std::string> args)
{
    if(!args.empty())
    {
        return "argerror";
    }
    
#if MODE == MODE_ISA_MODEL_ONLY
    return "isa_model_only";
#elif MODE == MODE_CYCLE_MODEL_ONLY
    return "cycle_model_only";
#else
    return "isa_and_cycle_model_compare";
#endif
}

static std::string socket_cmd_breakpoint_add(std::vector<std::string> args)
{
    if(args.size() != 2)
    {
        return "argerror";
    }
    
    auto type = args[0];
    auto value = get_multi_radix_num(args[1]);
    breakpoint_info_t info;
    
    if(type == "cycle")
    {
        info.type = breakpoint_type_t::cycle;
    }
    else if(type == "inst")
    {
        info.type = breakpoint_type_t::instruction_num;
    }
    else if(type == "pc")
    {
        info.type = breakpoint_type_t::pc;
    }
    else if(type == "csrw")
    {
        info.type = breakpoint_type_t::csrw;
    }
    else
    {
        return "typeerror";
    }
    
    info.value = value;
    breakpoint_add(info.type, info.value);
    return "ok";
}

static std::string socket_cmd_breakpoint_remove(std::vector<std::string> args)
{
    if(args.size() != 1)
    {
        return "argerror";
    }
    
    auto id = get_multi_radix_num(args[0]);
    breakpoint_remove(id);
    return "ok";
}

static std::string socket_cmd_breakpoint_get_list(std::vector<std::string> args)
{
    if(!args.empty())
    {
        return "argerror";
    }
    
    return breakpoint_get_list();
}

static std::string socket_cmd_breakpoint_clear(std::vector<std::string> args)
{
    if(!args.empty())
    {
        return "argerror";
    }
    
    breakpoint_clear();
    return "ok";
}

void network_command_init()
{
    register_socket_cmd("quit", socket_cmd_quit);
    register_socket_cmd("reset", socket_cmd_reset);
    register_socket_cmd("continue", socket_cmd_continue);
    register_socket_cmd("c", socket_cmd_continue);
    register_socket_cmd("pause", socket_cmd_pause);
    register_socket_cmd("p", socket_cmd_pause);
    register_socket_cmd("step", socket_cmd_step);
    register_socket_cmd("s", socket_cmd_step);
    register_socket_cmd("stepcommit", socket_cmd_stepcommit);
    register_socket_cmd("sc", socket_cmd_stepcommit);
    register_socket_cmd("read_memory", socket_cmd_read_memory);
    register_socket_cmd("write_memory", socket_cmd_write_memory);
    register_socket_cmd("read_archreg", socket_cmd_read_archreg);
    register_socket_cmd("read_csr", socket_cmd_read_csr);
    register_socket_cmd("get_pc", socket_cmd_get_pc);
    register_socket_cmd("get_cycle", socket_cmd_get_cycle);
#ifdef NEED_CYCLE_MODEL
    register_socket_cmd("get_pipeline_status", socket_cmd_get_pipeline_status);
#endif
    register_socket_cmd("get_commit_num", socket_cmd_get_commit_num);
    register_socket_cmd("get_finish", socket_cmd_get_finish);
    register_socket_cmd("get_mode", socket_cmd_get_mode);
    register_socket_cmd("b", socket_cmd_breakpoint_add);
    register_socket_cmd("br", socket_cmd_breakpoint_remove);
    register_socket_cmd("bl", socket_cmd_breakpoint_get_list);
    register_socket_cmd("bc", socket_cmd_breakpoint_clear);
}