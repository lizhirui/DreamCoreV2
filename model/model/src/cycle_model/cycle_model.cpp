/*
 * Copyright lizhirui
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-10-18     lizhirui     the first version
 */

#include "common.h"
#include "config.h"
#include "cycle_model/cycle_model.h"
#include "cycle_model/component/slave/memory.h"
#include "cycle_model/component/slave/clint.h"
#include "cycle_model/component/bus.h"
#include "cycle_model/component/csr_all.h"
#include "cycle_model/component/csrfile.h"
#include "cycle_model/component/dff.h"
#include "cycle_model/component/fifo.h"
#include "cycle_model/component/free_list.h"
#include "cycle_model/component/handshake_dff.h"
#include "cycle_model/component/interrupt_interface.h"
#include "cycle_model/component/port.h"
#include "cycle_model/component/rat.h"
#include "cycle_model/component/regfile.h"
#include "cycle_model/component/rob.h"
#include "cycle_model/component/store_buffer.h"
#include "cycle_model/pipeline/pipeline_common.h"
#include "cycle_model/pipeline/fetch1.h"
#include "cycle_model/pipeline/fetch1_fetch2.h"
#include "cycle_model/pipeline/fetch2.h"
#include "cycle_model/pipeline/fetch2_decode.h"
#include "cycle_model/pipeline/decode.h"
#include "cycle_model/pipeline/decode_rename.h"
#include "cycle_model/pipeline/rename.h"
#include "cycle_model/pipeline/rename_dispatch.h"
#include "cycle_model/pipeline/dispatch.h"
#include "cycle_model/pipeline/dispatch_issue.h"
#include "cycle_model/pipeline/integer_issue.h"
#include "cycle_model/pipeline/lsu_issue.h"
#include "cycle_model/pipeline/integer_issue_readreg.h"
#include "cycle_model/pipeline/lsu_issue_readreg.h"
#include "cycle_model/pipeline/integer_readreg.h"
#include "cycle_model/pipeline/lsu_readreg.h"
#include "cycle_model/pipeline/integer_readreg_execute.h"
#include "cycle_model/pipeline/lsu_readreg_execute.h"
#include "cycle_model/pipeline/execute.h"
#include "cycle_model/pipeline/execute/alu.h"
#include "cycle_model/pipeline/execute/bru.h"
#include "cycle_model/pipeline/execute/csr.h"
#include "cycle_model/pipeline/execute/div.h"
#include "cycle_model/pipeline/execute/mul.h"
#include "cycle_model/pipeline/execute/lsu.h"
#include "cycle_model/pipeline/execute_wb.h"
#include "cycle_model/pipeline/wb.h"
#include "cycle_model/pipeline/wb_commit.h"
#include "cycle_model/pipeline/commit.h"

namespace cycle_model
{
    cycle_model *cycle_model::instance = nullptr;
    
    cycle_model *cycle_model::create(charfifo_send_fifo_t *charfifo_send_fifo, charfifo_rev_fifo_t *charfifo_rev_fifo)
    {
        if(instance == nullptr)
        {
            instance = new cycle_model(charfifo_send_fifo, charfifo_rev_fifo);
        }
        
        return instance;
    }
    
    void cycle_model::destroy()
    {
        if(instance != nullptr)
        {
            delete instance;
            instance = nullptr;
        }
    }
    
    cycle_model::cycle_model(charfifo_send_fifo_t *charfifo_send_fifo, charfifo_rev_fifo_t *charfifo_rev_fifo) :
    charfifo_send_fifo(charfifo_send_fifo),
    charfifo_rev_fifo(charfifo_rev_fifo),
    fetch1_fetch2_port(pipeline::fetch1_fetch2_pack_t()),
    fetch2_decode_fifo(FETCH2_DECODE_FIFO_SIZE),
    decode_rename_fifo(DECODE_RENAME_FIFO_SIZE),
    rename_dispatch_port(pipeline::rename_dispatch_pack_t()),
    dispatch_integer_issue_port(pipeline::dispatch_issue_pack_t()),
    dispatch_lsu_issue_port(pipeline::dispatch_issue_pack_t()),
    integer_issue_readreg_port(pipeline::integer_issue_readreg_pack_t()),
    lsu_issue_readreg_port(pipeline::lsu_issue_readreg_pack_t()),
    readreg_alu_hdff{nullptr},
    readreg_bru_hdff{nullptr},
    readreg_csr_hdff{nullptr},
    readreg_div_hdff{nullptr},
    readreg_mul_hdff{nullptr},
    readreg_lsu_hdff{nullptr},
    alu_wb_port{nullptr},
    bru_wb_port{nullptr},
    csr_wb_port{nullptr},
    div_wb_port{nullptr},
    mul_wb_port{nullptr},
    lsu_wb_port{nullptr},
    wb_commit_port(pipeline::wb_commit_pack_t()),
    phy_id_free_list(PHY_REG_NUM),
    interrupt_interface(&csr_file),
    speculative_rat(PHY_REG_NUM, ARCH_REG_NUM),
    retire_rat(PHY_REG_NUM, ARCH_REG_NUM),
    phy_regfile(PHY_REG_NUM),
    rob(ROB_SIZE),
    store_buffer(STORE_BUFFER_SIZE, &bus),
    clint(&bus, &interrupt_interface),
    fetch1_stage(&bus, &fetch1_fetch2_port, &store_buffer, INIT_PC),
    fetch2_stage(&fetch1_fetch2_port, &fetch2_decode_fifo),
    decode_stage(&fetch2_decode_fifo, &decode_rename_fifo),
    rename_stage(&decode_rename_fifo, &rename_dispatch_port, &speculative_rat, &rob, &phy_id_free_list),
    dispatch_stage(&rename_dispatch_port, &dispatch_integer_issue_port, &dispatch_lsu_issue_port, &store_buffer),
    integer_issue_stage(&dispatch_integer_issue_port, &integer_issue_readreg_port, &phy_regfile),
    lsu_issue_stage(&dispatch_lsu_issue_port, &lsu_issue_readreg_port, &phy_regfile),
    integer_readreg_stage(&integer_issue_readreg_port, readreg_alu_hdff, readreg_bru_hdff, readreg_csr_hdff, readreg_div_hdff, readreg_mul_hdff, &phy_regfile),
    lsu_readreg_stage(&lsu_issue_readreg_port, readreg_lsu_hdff, &phy_regfile),
    execute_alu_stage{nullptr},
    execute_bru_stage{nullptr},
    execute_csr_stage{nullptr},
    execute_div_stage{nullptr},
    execute_mul_stage{nullptr},
    execute_lsu_stage{nullptr},
    wb_stage(alu_wb_port, bru_wb_port, csr_wb_port, div_wb_port, mul_wb_port, lsu_wb_port, &wb_commit_port, &phy_regfile),
    commit_stage(&wb_commit_port, &speculative_rat, &retire_rat, &rob, &csr_file, &phy_regfile, &phy_id_free_list, &interrupt_interface)
    {
        bus.map(MEMORY_BASE, MEMORY_SIZE, std::make_shared<component::slave::memory>(&bus), true);
        bus.map(CLINT_BASE, CLINT_SIZE, std::shared_ptr<component::slave::clint>(&clint), false);
        
        for(uint32_t i = 0;i < ALU_UNIT_NUM;i++)
        {
            readreg_alu_hdff[i] = new component::handshake_dff<pipeline::integer_readreg_execute_pack_t>();
            execute_alu_stage[i] = new pipeline::execute::alu(i, readreg_alu_hdff[i], alu_wb_port[i]);
            alu_wb_port[i] = new component::port<pipeline::execute_wb_pack_t>(pipeline::execute_wb_pack_t());
        }
        
        for(uint32_t i = 0;i < BRU_UNIT_NUM;i++)
        {
            readreg_bru_hdff[i] = new component::handshake_dff<pipeline::integer_readreg_execute_pack_t>();
            execute_bru_stage[i] = new pipeline::execute::bru(i, readreg_bru_hdff[i], bru_wb_port[i], &csr_file);
            bru_wb_port[i] = new component::port<pipeline::execute_wb_pack_t>(pipeline::execute_wb_pack_t());
        }
        
        for(uint32_t i = 0;i < CSR_UNIT_NUM;i++)
        {
            readreg_csr_hdff[i] = new component::handshake_dff<pipeline::integer_readreg_execute_pack_t>();
            execute_csr_stage[i] = new pipeline::execute::csr(i, readreg_csr_hdff[i], csr_wb_port[i], &csr_file);
            csr_wb_port[i] = new component::port<pipeline::execute_wb_pack_t>(pipeline::execute_wb_pack_t());
        }
        
        for(uint32_t i = 0;i < DIV_UNIT_NUM;i++)
        {
            readreg_div_hdff[i] = new component::handshake_dff<pipeline::integer_readreg_execute_pack_t>();
            execute_div_stage[i] = new pipeline::execute::div(i, readreg_div_hdff[i], div_wb_port[i]);
            div_wb_port[i] = new component::port<pipeline::execute_wb_pack_t>(pipeline::execute_wb_pack_t());
        }
        
        for(uint32_t i = 0;i < MUL_UNIT_NUM;i++)
        {
            readreg_mul_hdff[i] = new component::handshake_dff<pipeline::integer_readreg_execute_pack_t>();
            execute_mul_stage[i] = new pipeline::execute::mul(i, readreg_mul_hdff[i], mul_wb_port[i]);
            mul_wb_port[i] = new component::port<pipeline::execute_wb_pack_t>(pipeline::execute_wb_pack_t());
        }
        
        for(uint32_t i = 0;i < LSU_UNIT_NUM;i++)
        {
            readreg_lsu_hdff[i] = new component::handshake_dff<pipeline::lsu_readreg_execute_pack_t>();
            execute_lsu_stage[i] = new pipeline::execute::lsu(i, readreg_lsu_hdff[i], lsu_wb_port[i], &bus, &store_buffer);
            lsu_wb_port[i] = new component::port<pipeline::execute_wb_pack_t>(pipeline::execute_wb_pack_t());
        }
    
        csr_file.map(CSR_MVENDORID, true, std::make_shared<component::csr::mvendorid>());
        csr_file.map(CSR_MARCHID, true, std::make_shared<component::csr::marchid>());
        csr_file.map(CSR_MIMPID, true, std::make_shared<component::csr::mimpid>());
        csr_file.map(CSR_MHARTID, true, std::make_shared<component::csr::mhartid>());
        csr_file.map(CSR_MCONFIGPTR, true, std::make_shared<component::csr::mconfigptr>());
        csr_file.map(CSR_MSTATUS, false, std::make_shared<component::csr::mstatus>());
        csr_file.map(CSR_MISA, false, std::make_shared<component::csr::misa>());
        csr_file.map(CSR_MIE, false, std::make_shared<component::csr::mie>());
        csr_file.map(CSR_MTVEC, false, std::make_shared<component::csr::mtvec>());
        csr_file.map(CSR_MCOUNTEREN, false, std::make_shared<component::csr::mcounteren>());
        csr_file.map(CSR_MSTATUSH, false, std::make_shared<component::csr::mstatush>());
        csr_file.map(CSR_MSCRATCH, false, std::make_shared<component::csr::mscratch>());
        csr_file.map(CSR_MEPC, false, std::make_shared<component::csr::mepc>());
        csr_file.map(CSR_MCAUSE, false, std::make_shared<component::csr::mcause>());
        csr_file.map(CSR_MTVAL, false, std::make_shared<component::csr::mtval>());
        csr_file.map(CSR_MIP, false, std::make_shared<component::csr::mip>());
        csr_file.map(CSR_CHARFIFO, false, std::make_shared<component::csr::charfifo>(charfifo_send_fifo, charfifo_rev_fifo));
        csr_file.map(CSR_FINISH, false, std::make_shared<component::csr::finish>());
        csr_file.map(CSR_MCYCLE, false, std::make_shared<component::csr::mcycle>());
        csr_file.map(CSR_MINSTRET, false, std::make_shared<component::csr::minstret>());
        csr_file.map(CSR_MCYCLEH, false, std::make_shared<component::csr::mcycleh>());
        csr_file.map(CSR_MINSTRETH, false, std::make_shared<component::csr::minstreth>());
        csr_file.map(CSR_BRANCHNUM, true, std::make_shared<component::csr::mhpmcounter>("branchnum"));
        csr_file.map(CSR_BRANCHNUMH, true, std::make_shared<component::csr::mhpmcounterh>("branchnumh"));
        csr_file.map(CSR_BRANCHPREDICTED, true, std::make_shared<component::csr::mhpmcounter>("branchpredicted"));
        csr_file.map(CSR_BRANCHPREDICTEDH, true, std::make_shared<component::csr::mhpmcounterh>("branchpredictedh"));
        csr_file.map(CSR_BRANCHHIT, true, std::make_shared<component::csr::mhpmcounter>("branchhit"));
        csr_file.map(CSR_BRANCHHITH, true, std::make_shared<component::csr::mhpmcounterh>("branchhith"));
        csr_file.map(CSR_BRANCHMISS, true, std::make_shared<component::csr::mhpmcounter>("branchmiss"));
        csr_file.map(CSR_BRANCHMISSH, true, std::make_shared<component::csr::mhpmcounterh>("branchmissh"));
    
        for(auto i = 0;i < 16;i++)
        {
            csr_file.map(0x3A0 + i, false, std::make_shared<component::csr::pmpcfg>(i));
        }
    
        for(auto i = 0;i < 64;i++)
        {
            csr_file.map(0x3B0 + i, false, std::make_shared<component::csr::pmpaddr>(i));
        }
    
        wb_stage.init();
        this->reset();
    }
    
    cycle_model::~cycle_model()
    {
        for(uint32_t i = 0;i < ALU_UNIT_NUM;i++)
        {
            delete readreg_alu_hdff[i];
            delete execute_alu_stage[i];
            delete alu_wb_port[i];
        }
        
        for(uint32_t i = 0;i < BRU_UNIT_NUM;i++)
        {
            delete readreg_bru_hdff[i];
            delete execute_bru_stage[i];
            delete bru_wb_port[i];
        }
        
        for(uint32_t i = 0;i < CSR_UNIT_NUM;i++)
        {
            delete readreg_csr_hdff[i];
            delete execute_csr_stage[i];
            delete csr_wb_port[i];
        }
        
        for(uint32_t i = 0;i < DIV_UNIT_NUM;i++)
        {
            delete readreg_div_hdff[i];
            delete execute_div_stage[i];
            delete div_wb_port[i];
        }
        
        for(uint32_t i = 0;i < MUL_UNIT_NUM;i++)
        {
            delete readreg_mul_hdff[i];
            delete execute_mul_stage[i];
            delete mul_wb_port[i];
        }
        
        for(uint32_t i = 0;i < LSU_UNIT_NUM;i++)
        {
            delete readreg_lsu_hdff[i];
            delete execute_lsu_stage[i];
            delete lsu_wb_port[i];
        }
    }
    
    void cycle_model::load(void *mem, size_t size)
    {
        auto buf = (uint8_t *)mem;
        verify(size < MEMORY_SIZE);
        
        for(size_t i = 0;i < size;i++)
        {
            bus.write8(MEMORY_BASE + i, buf[i]);
        }
    }
    
    void cycle_model::reset()
    {
        speculative_rat.reset();
        retire_rat.reset();
        speculative_rat.init_start();
        retire_rat.init_start();
        phy_regfile.reset();
        
        for(uint32_t i = 1;i < ARCH_REG_NUM;i++)
        {
            speculative_rat.set_map(i, i);
            retire_rat.set_map(i, i);
            phy_regfile.write(i, i, true);
        }
        
        speculative_rat.init_finish();
        retire_rat.init_finish();
        fetch1_fetch2_port.reset();
        fetch2_decode_fifo.reset();
        decode_rename_fifo.reset();
        rename_dispatch_port.reset();
        dispatch_integer_issue_port.reset();
        dispatch_lsu_issue_port.reset();
        integer_issue_readreg_port.reset();
        lsu_issue_readreg_port.reset();
        
        for(uint32_t i = 0;i < ALU_UNIT_NUM;i++)
        {
            readreg_alu_hdff[i]->reset();
            execute_alu_stage[i]->reset();
            alu_wb_port[i]->reset();
        }
    
        for(uint32_t i = 0;i < BRU_UNIT_NUM;i++)
        {
            readreg_bru_hdff[i]->reset();
            execute_bru_stage[i]->reset();
            bru_wb_port[i]->reset();
        }
    
        for(uint32_t i = 0;i < CSR_UNIT_NUM;i++)
        {
            readreg_csr_hdff[i]->reset();
            execute_csr_stage[i]->reset();
            csr_wb_port[i]->reset();
        }
    
        for(uint32_t i = 0;i < DIV_UNIT_NUM;i++)
        {
            readreg_div_hdff[i]->reset();
            execute_div_stage[i]->reset();
            div_wb_port[i]->reset();
        }
    
        for(uint32_t i = 0;i < MUL_UNIT_NUM;i++)
        {
            readreg_mul_hdff[i]->reset();
            execute_mul_stage[i]->reset();
            mul_wb_port[i]->reset();
        }
        
        for(uint32_t i = 0;i < LSU_UNIT_NUM;i++)
        {
            readreg_lsu_hdff[i]->reset();
            execute_lsu_stage[i]->reset();
            lsu_wb_port[i]->reset();
        }
        
        wb_commit_port.reset();
        bus.reset();
        rob.reset();
        csr_file.reset();
        store_buffer.reset();
        interrupt_interface.reset();
        ((component::slave::memory *)bus.get_slave_obj(0x80000000, false))->reset();
        clint.reset();
        
        fetch1_stage.reset();
        fetch2_stage.reset();
        decode_stage.reset();
        rename_stage.reset();
        dispatch_stage.reset();
        integer_issue_stage.reset();
        lsu_issue_stage.reset();
        integer_readreg_stage.reset();
        lsu_readreg_stage.reset();
        wb_stage.reset();
        commit_stage.reset();
        
        cpu_clock_cycle = 0;
        committed_instruction_num = 0;
        
        branch_num = 1;
        branch_predicted = 0;
        branch_hit = 0;
        branch_miss = 0;
        
        component::dff_base::reset_all();
    }
    
    void cycle_model::run()
    {
        rob.set_committed(false);
        clint.run_pre();
        commit_feedback_pack = commit_stage.run();
        wb_feedback_pack = wb_stage.run(commit_feedback_pack);
        
        uint32_t execute_feedback_channel = 0;
        
        for(uint32_t i = 0;i < ALU_UNIT_NUM;i++)
        {
            execute_feedback_pack.channel[execute_feedback_channel++] = execute_alu_stage[i]->run(commit_feedback_pack);
        }
        
        for(uint32_t i = 0;i < BRU_UNIT_NUM;i++)
        {
            execute_feedback_pack.channel[execute_feedback_channel++] = execute_bru_stage[i]->run(commit_feedback_pack);
        }
        
        for(uint32_t i = 0;i < CSR_UNIT_NUM;i++)
        {
            execute_feedback_pack.channel[execute_feedback_channel++] = execute_csr_stage[i]->run(commit_feedback_pack);
        }
        
        for(uint32_t i = 0;i < DIV_UNIT_NUM;i++)
        {
            execute_feedback_pack.channel[execute_feedback_channel++] = execute_div_stage[i]->run(commit_feedback_pack);
        }
        
        for(uint32_t i = 0;i < MUL_UNIT_NUM;i++)
        {
            execute_feedback_pack.channel[execute_feedback_channel++] = execute_mul_stage[i]->run(commit_feedback_pack);
        }
        
        for(uint32_t i = 0;i < LSU_UNIT_NUM;i++)
        {
            execute_feedback_pack.channel[execute_feedback_channel++] = execute_lsu_stage[i]->run(commit_feedback_pack);
        }
        
        integer_readreg_stage.run(execute_feedback_pack, wb_feedback_pack, commit_feedback_pack);
        lsu_readreg_feedback_pack = lsu_readreg_stage.run(execute_feedback_pack, wb_feedback_pack, commit_feedback_pack);
        integer_issue_output_feedback_pack = integer_issue_stage.run_output(commit_feedback_pack);
        lsu_issue_stage.run_output(lsu_readreg_feedback_pack, commit_feedback_pack);
        integer_issue_stage.run_wakeup(integer_issue_output_feedback_pack, execute_feedback_pack, commit_feedback_pack);
        lsu_issue_stage.run_wakeup(integer_issue_output_feedback_pack, execute_feedback_pack, commit_feedback_pack);
        integer_issue_feedback_pack = integer_issue_stage.run_input(execute_feedback_pack, wb_feedback_pack, commit_feedback_pack);
        lsu_issue_feedback_pack = lsu_issue_stage.run_input(execute_feedback_pack, wb_feedback_pack, commit_feedback_pack);
        dispatch_feedback_pack = dispatch_stage.run(integer_issue_feedback_pack, lsu_issue_feedback_pack, commit_feedback_pack);
        rename_feedback_pack = rename_stage.run(dispatch_feedback_pack, commit_feedback_pack);
        decode_feedback_pack = decode_stage.run(commit_feedback_pack);
        fetch2_feedback_pack = fetch2_stage.run(commit_feedback_pack);
        fetch1_stage.run(fetch2_feedback_pack, decode_feedback_pack, rename_feedback_pack, commit_feedback_pack);
        interrupt_interface.run();
        clint.run_post();
        store_buffer.run(commit_feedback_pack);
        bus.run();
        bus.sync();
        cpu_clock_cycle++;
        committed_instruction_num = rob.get_global_commit_num();
        csr_file.write_sys(CSR_MCYCLE, (uint32_t)(cpu_clock_cycle & 0xffffffffu));
        csr_file.write_sys(CSR_MCYCLEH, (uint32_t)(cpu_clock_cycle >> 32));
        csr_file.write_sys(CSR_MINSTRET, (uint32_t)(committed_instruction_num & 0xffffffffu));
        csr_file.write_sys(CSR_MINSTRETH, (uint32_t)(committed_instruction_num >> 32));
        csr_file.write_sys(CSR_BRANCHNUM, (uint32_t)(branch_num & 0xffffffffu));
        csr_file.write_sys(CSR_BRANCHNUMH, (uint32_t)(branch_num >> 32));
        csr_file.write_sys(CSR_BRANCHPREDICTED, (uint32_t)(branch_predicted & 0xffffffffu));
        csr_file.write_sys(CSR_BRANCHPREDICTEDH, (uint32_t)(branch_predicted >> 32));
        csr_file.write_sys(CSR_BRANCHHIT, (uint32_t)(branch_hit & 0xffffffffu));
        csr_file.write_sys(CSR_BRANCHHITH, (uint32_t)(branch_hit >> 32));
        csr_file.write_sys(CSR_BRANCHMISS, (uint32_t)(branch_miss & 0xffffffffu));
        csr_file.write_sys(CSR_BRANCHMISSH, (uint32_t)(branch_miss >> 32));
        component::dff_base::sync_all();
    }
}