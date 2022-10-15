/*
 * Copyright lizhirui
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-10-14     lizhirui     the first version
 */

#include <common.h>
#include <config.h>
#include <network/network_command.h>

static std::string socket_cmd_quit(std::vector<std::string> args)
{
    if(args.size() != 0)
    {
        return "argerror";
    }

    recv_thread_stop = true;
    program_stop = true;
    return "ok";
}

static std::string socket_cmd_reset(std::vector<std::string> args)
{
    if(args.size() != 0)
    {
        return "argerror";
    }

    reset();
    return "ok";
}

static std::string socket_cmd_continue(std::vector<std::string> args)
{
    if(args.size() != 0)
    {
        return "argerror";
    }

    pause_state = false;
    step_state = false;
    wait_commit = false;
    return "ok";
}

static std::string socket_cmd_pause(std::vector<std::string> args)
{
    if(args.size() != 0)
    {
        return "argerror";
    }

    step_state = true;
    wait_commit = false;
    return "ok";
}

static std::string socket_cmd_step(std::vector<std::string> args)
{
    if(args.size() != 0)
    {
        return "argerror";
    }

    pause_state = false;
    step_state = true;
    wait_commit = false;
    return "ok";
}

static std::string socket_cmd_stepcommit(std::vector<std::string> args)
{
    if(args.size() != 0)
    {
        return "argerror";
    }

    pause_state = false;
    step_state = true;
    wait_commit = true;
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
        result << "," << std::hex << (uint32_t)bus.read8(addr);
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

    for(auto offset = 0;offset < (data_str.length() >> 1);offset++)
    {
        std::stringstream hex_str(data_str.substr(offset << 1, 2));
        hex_str.unsetf(std::ios::dec);
        hex_str.setf(std::ios::hex);
        uint32_t value = 0;
        hex_str >> value;
        bus.write8(address + offset, (uint8_t)value);
    }

    return "ok";
}

static std::string socket_cmd_read_archreg(std::vector<std::string> args)
{
    if(args.size() != 0)
    {
        return "argerror";
    }

    std::stringstream result;

    for(auto i = 0;i < ARCH_REG_NUM;i++)
    {
        if(i == 0)
        {
            result << std::hex << 0;
        }
        else
        {
            uint32_t phy_id;
            rat.get_commit_phy_id(i, &phy_id);
            auto v = phy_regfile.read(phy_id);
            assert(phy_regfile.read_data_valid(phy_id));
            result << "," << std::hex << v.value;
        }
    }

    return result.str();
}

static std::string socket_cmd_read_csr(std::vector<std::string> args)
{
    if(args.size() != 0)
    {
        return "argerror";
    }

    return csr_file.get_info_packet();
}

static std::string socket_cmd_get_pc(std::vector<std::string> args)
{
    if(args.size() != 0)
    {
        return "argerror";
    }

    std::stringstream result;
    result << outhex(get_current_pc());
    return result.str();
}

static std::string socket_cmd_get_cycle(std::vector<std::string> args)
{
    if(args.size() != 0)
    {
        return "argerror";
    }

    std::stringstream result;
    result << cpu_clock_cycle;
    return result.str();
}

static std::string socket_cmd_get_pipeline_status(std::vector<std::string> args)
{
    if(args.size() != 0)
    {
        return "argerror";
    }

    json ret;
    ret["fetch"] = fetch_stage.get_json();
    ret["fetch_decode"] = fetch_decode_fifo.get_json();
    ret["decode_rename"] = decode_rename_fifo.get_json();
    ret["rename_readreg"] = rename_readreg_port.get_json();
    ret["readreg_issue"] = readreg_issue_port.get_json();
    ret["issue"] = issue_stage.get_json();

    json tie;
    json tie_alu, tie_bru, tie_csr, tie_div, tie_lsu, tie_mul;
    tie_alu = json::array();
    tie_bru = json::array();
    tie_csr = json::array();
    tie_div = json::array();
    tie_lsu = json::array();
    tie_mul = json::array();

    for(auto i = 0;i < ALU_UNIT_NUM;i++)
    {
        tie_alu.push_back(issue_alu_fifo[i]->get_json());
    }

    for(auto i = 0;i < BRU_UNIT_NUM;i++)
    {
        tie_bru.push_back(issue_bru_fifo[i]->get_json());
    }

    for(auto i = 0;i < CSR_UNIT_NUM;i++)
    {
        tie_csr.push_back(issue_csr_fifo[i]->get_json());
    }

    for(auto i = 0;i < DIV_UNIT_NUM;i++)
    {
        tie_div.push_back(issue_div_fifo[i]->get_json());
    }

    for(auto i = 0;i < LSU_UNIT_NUM;i++)
    {
        tie_lsu.push_back(issue_lsu_fifo[i]->get_json());
    }

    for(auto i = 0;i < MUL_UNIT_NUM;i++)
    {
        tie_mul.push_back(issue_mul_fifo[i]->get_json());
    }

    tie["alu"] = tie_alu;
    tie["bru"] = tie_bru;
    tie["csr"] = tie_csr;
    tie["div"] = tie_div;
    tie["lsu"] = tie_lsu;
    tie["mul"] = tie_mul;
    ret["issue_execute"] = tie;

    json tew;
    json tew_alu, tew_bru, tew_csr, tew_div, tew_lsu, tew_mul;
    tew_alu = json::array();
    tew_bru = json::array();
    tew_csr = json::array();
    tew_div = json::array();
    tew_lsu = json::array();
    tew_mul = json::array();

    for(auto i = 0;i < ALU_UNIT_NUM;i++)
    {
        tew_alu.push_back(alu_wb_port[i]->get_json());
    }

    for(auto i = 0;i < BRU_UNIT_NUM;i++)
    {
        tew_bru.push_back(bru_wb_port[i]->get_json());
    }

    for(auto i = 0;i < CSR_UNIT_NUM;i++)
    {
        tew_csr.push_back(csr_wb_port[i]->get_json());
    }

    for(auto i = 0;i < DIV_UNIT_NUM;i++)
    {
        tew_div.push_back(div_wb_port[i]->get_json());
    }

    for(auto i = 0;i < LSU_UNIT_NUM;i++)
    {
        tew_lsu.push_back(lsu_wb_port[i]->get_json());
    }

    for(auto i = 0;i < MUL_UNIT_NUM;i++)
    {
        tew_mul.push_back(mul_wb_port[i]->get_json());
    }

    tew["alu"] = tew_alu;
    tew["bru"] = tew_bru;
    tew["csr"] = tew_csr;
    tew["div"] = tew_div;
    tew["lsu"] = tew_lsu;
    tew["mul"] = tew_mul;
    ret["execute_wb"] = tew;
    ret["wb_commit"] = wb_commit_port.get_json();

    ret["issue_feedback_pack"] = t_issue_feedback_pack.get_json();
    ret["wb_feedback_pack"] = t_wb_feedback_pack.get_json();
    ret["commit_feedback_pack"] = t_commit_feedback_pack.get_json();
    ret["rob"] = rob.get_json();
    return ret.dump();
}

static std::string socket_cmd_get_commit_num(std::vector<std::string> args)
{
    if(args.size() != 0)
    {
        return "argerror";
    }

    std::stringstream result;
    result << rob.get_commit_num();
    rob.clear_commit_num();
    return result.str();
}

static std::string socket_cmd_get_finish(std::vector<std::string> args)
{
    if(args.size() != 0)
    {
        return "argerror";
    }

    std::stringstream result;
    result << (int)csr_file.read_sys(CSR_FINISH);
    return result.str();
}

void network_command_init()
{
    register_socket_cmd("quit", socket_cmd_quit);
    register_socket_cmd("reset", socket_cmd_reset);
    register_socket_cmd("continue", socket_cmd_continue);
    register_socket_cmd("pause", socket_cmd_pause);
    register_socket_cmd("step", socket_cmd_step);
    register_socket_cmd("stepcommit", socket_cmd_stepcommit);
    register_socket_cmd("read_memory", socket_cmd_read_memory);
    register_socket_cmd("write_memory", socket_cmd_write_memory);
    register_socket_cmd("read_archreg", socket_cmd_read_archreg);
    register_socket_cmd("read_csr", socket_cmd_read_csr);
    register_socket_cmd("get_pc", socket_cmd_get_pc);
    register_socket_cmd("get_cycle", socket_cmd_get_cycle);
    register_socket_cmd("get_pipeline_status", socket_cmd_get_pipeline_status);
    register_socket_cmd("get_commit_num", socket_cmd_get_commit_num);
    register_socket_cmd("get_finish", socket_cmd_get_finish);
}