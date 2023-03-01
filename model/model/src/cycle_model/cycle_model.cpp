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
#include "cycle_model/component/branch_predictor_base.h"
#include "cycle_model/component/branch_predictor_set.h"
#include "cycle_model/component/ras.h"
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
#include "cycle_model/pipeline/execute/lu.h"
#include "cycle_model/pipeline/execute/sau.h"
#include "cycle_model/pipeline/execute/sdu.h"
#include "cycle_model/pipeline/execute_wb.h"
#include "cycle_model/pipeline/wb.h"
#include "cycle_model/pipeline/execute_commit.h"
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
    readreg_lu_hdff{nullptr},
    readreg_sau_hdff{nullptr},
    readreg_sdu_hdff{nullptr},
    alu_wb_port{nullptr},
    bru_wb_port{nullptr},
    csr_wb_port{nullptr},
    div_wb_port{nullptr},
    mul_wb_port{nullptr},
    lu_wb_port{nullptr},
    sau_wb_port{nullptr},
    sdu_wb_port{nullptr},
    alu_commit_port{nullptr},
    bru_commit_port{nullptr},
    csr_commit_port{nullptr},
    div_commit_port{nullptr},
    mul_commit_port{nullptr},
    lu_commit_port{nullptr},
    sau_commit_port{nullptr},
    sdu_commit_port{nullptr},
    phy_id_free_list(PHY_REG_NUM),
    interrupt_interface(&csr_file),
    speculative_rat(PHY_REG_NUM, ARCH_REG_NUM),
    retire_rat(PHY_REG_NUM, ARCH_REG_NUM),
    phy_regfile(PHY_REG_NUM),
    rob(ROB_SIZE),
    store_buffer(STORE_BUFFER_SIZE, &bus),
    clint(&bus, &interrupt_interface),
    checkpoint_buffer(CHECKPOINT_BUFFER_SIZE),
    load_queue(LOAD_QUEUE_SIZE),
    fetch1_stage(&global, &bus, &fetch1_fetch2_port, &store_buffer, &branch_predictor_set, INIT_PC),
    fetch2_stage(&global, &fetch1_fetch2_port, &fetch2_decode_fifo, &branch_predictor_set),
    decode_stage(&global, &fetch2_decode_fifo, &decode_rename_fifo),
    rename_stage(&global, &decode_rename_fifo, &rename_dispatch_port, &speculative_rat, &rob, &phy_id_free_list, &checkpoint_buffer, &load_queue, &store_buffer),
    dispatch_stage(&global, &rename_dispatch_port, &dispatch_integer_issue_port, &dispatch_lsu_issue_port, &store_buffer),
    integer_issue_stage(&global, &dispatch_integer_issue_port, &integer_issue_readreg_port, &phy_regfile),
    lsu_issue_stage(&global, &dispatch_lsu_issue_port, &lsu_issue_readreg_port, &phy_regfile),
    integer_readreg_stage(&global, &integer_issue_readreg_port, readreg_alu_hdff, readreg_bru_hdff, readreg_csr_hdff, readreg_div_hdff, readreg_mul_hdff, &phy_regfile),
    lsu_readreg_stage(&global, &lsu_issue_readreg_port, readreg_lu_hdff, readreg_sau_hdff, readreg_sdu_hdff, &phy_regfile),
    execute_alu_stage{nullptr},
    execute_bru_stage{nullptr},
    execute_csr_stage{nullptr},
    execute_div_stage{nullptr},
    execute_mul_stage{nullptr},
    execute_lu_stage{nullptr},
    execute_sau_stage{nullptr},
    execute_sdu_stage{nullptr},
    wb_stage(&global, alu_wb_port, bru_wb_port, csr_wb_port, div_wb_port, mul_wb_port, lu_wb_port, &phy_regfile),
    commit_stage(&global, alu_commit_port, bru_commit_port, csr_commit_port, div_commit_port, mul_commit_port, lu_commit_port, sau_commit_port, sdu_commit_port, &speculative_rat, &retire_rat, &rob, &csr_file, &phy_regfile, &phy_id_free_list, &interrupt_interface, &branch_predictor_set, &checkpoint_buffer, &load_queue)
    {
        bus.map(MEMORY_BASE, MEMORY_SIZE, std::make_shared<component::slave::memory>(&bus), true);
        bus.map(CLINT_BASE, CLINT_SIZE, std::shared_ptr<component::slave::clint>(&clint, boost::null_deleter()), false);
        
        for(uint32_t i = 0;i < ALU_UNIT_NUM;i++)
        {
            readreg_alu_hdff[i] = new component::handshake_dff<pipeline::integer_readreg_execute_pack_t>();
            alu_wb_port[i] = new component::port<pipeline::execute_wb_pack_t>(pipeline::execute_wb_pack_t());
            alu_commit_port[i] = new component::port<pipeline::execute_commit_pack_t>(pipeline::execute_commit_pack_t());
            execute_alu_stage[i] = new pipeline::execute::alu(&global, i, readreg_alu_hdff[i], alu_wb_port[i]);
        }
        
        for(uint32_t i = 0;i < BRU_UNIT_NUM;i++)
        {
            readreg_bru_hdff[i] = new component::handshake_dff<pipeline::integer_readreg_execute_pack_t>();
            bru_wb_port[i] = new component::port<pipeline::execute_wb_pack_t>(pipeline::execute_wb_pack_t());
            bru_commit_port[i] = new component::port<pipeline::execute_commit_pack_t>(pipeline::execute_commit_pack_t());
            execute_bru_stage[i] = new pipeline::execute::bru(&global, i, readreg_bru_hdff[i], bru_wb_port[i], &csr_file, &speculative_rat, &rob, &phy_regfile, &phy_id_free_list, &checkpoint_buffer, &branch_predictor_set, &load_queue);
        }
        
        for(uint32_t i = 0;i < CSR_UNIT_NUM;i++)
        {
            readreg_csr_hdff[i] = new component::handshake_dff<pipeline::integer_readreg_execute_pack_t>();
            csr_wb_port[i] = new component::port<pipeline::execute_wb_pack_t>(pipeline::execute_wb_pack_t());
            csr_commit_port[i] = new component::port<pipeline::execute_commit_pack_t>(pipeline::execute_commit_pack_t());
            execute_csr_stage[i] = new pipeline::execute::csr(&global, i, readreg_csr_hdff[i], csr_wb_port[i], &csr_file);
        }
        
        for(uint32_t i = 0;i < DIV_UNIT_NUM;i++)
        {
            readreg_div_hdff[i] = new component::handshake_dff<pipeline::integer_readreg_execute_pack_t>();
            div_wb_port[i] = new component::port<pipeline::execute_wb_pack_t>(pipeline::execute_wb_pack_t());
            div_commit_port[i] = new component::port<pipeline::execute_commit_pack_t>(pipeline::execute_commit_pack_t());
            execute_div_stage[i] = new pipeline::execute::div(&global, i, readreg_div_hdff[i], div_wb_port[i]);
        }
        
        for(uint32_t i = 0;i < MUL_UNIT_NUM;i++)
        {
            readreg_mul_hdff[i] = new component::handshake_dff<pipeline::integer_readreg_execute_pack_t>();
            mul_wb_port[i] = new component::port<pipeline::execute_wb_pack_t>(pipeline::execute_wb_pack_t());
            mul_commit_port[i] = new component::port<pipeline::execute_commit_pack_t>(pipeline::execute_commit_pack_t());
            execute_mul_stage[i] = new pipeline::execute::mul(&global, i, readreg_mul_hdff[i], mul_wb_port[i]);
        }
        
        for(uint32_t i = 0;i < LU_UNIT_NUM;i++)
        {
            readreg_lu_hdff[i] = new component::handshake_dff<pipeline::lsu_readreg_execute_pack_t>();
            lu_wb_port[i] = new component::port<pipeline::execute_wb_pack_t>(pipeline::execute_wb_pack_t());
            lu_commit_port[i] = new component::port<pipeline::execute_commit_pack_t>(pipeline::execute_commit_pack_t());
            execute_lu_stage[i] = new pipeline::execute::lu(&global, i, readreg_lu_hdff[i], lu_wb_port[i], &bus, &store_buffer, &clint, &load_queue);
        }
    
        for(uint32_t i = 0;i < SAU_UNIT_NUM;i++)
        {
            readreg_sau_hdff[i] = new component::handshake_dff<pipeline::lsu_readreg_execute_pack_t>();
            sau_wb_port[i] = new component::port<pipeline::execute_wb_pack_t>(pipeline::execute_wb_pack_t());
            sau_commit_port[i] = new component::port<pipeline::execute_commit_pack_t>(pipeline::execute_commit_pack_t());
            execute_sau_stage[i] = new pipeline::execute::sau(&global, i, readreg_sau_hdff[i], sau_wb_port[i], &store_buffer, &load_queue, &speculative_rat, &rob, &phy_regfile, &phy_id_free_list, &checkpoint_buffer);
        }
    
        for(uint32_t i = 0;i < SDU_UNIT_NUM;i++)
        {
            readreg_sdu_hdff[i] = new component::handshake_dff<pipeline::lsu_readreg_execute_pack_t>();
            sdu_wb_port[i] = new component::port<pipeline::execute_wb_pack_t>(pipeline::execute_wb_pack_t());
            sdu_commit_port[i] = new component::port<pipeline::execute_commit_pack_t>(pipeline::execute_commit_pack_t());
            execute_sdu_stage[i] = new pipeline::execute::sdu(&global, i, readreg_sdu_hdff[i], sdu_wb_port[i], &store_buffer);
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
        csr_file.map(CSR_LOADNUM, true, std::make_shared<component::csr::mhpmcounter>("loadnum"));
        csr_file.map(CSR_LOADNUMH, true, std::make_shared<component::csr::mhpmcounterh>("loadnumh"));
        csr_file.map(CSR_REPLAYNUM, true, std::make_shared<component::csr::mhpmcounter>("replaynum"));
        csr_file.map(CSR_REPLAYNUMH, true, std::make_shared<component::csr::mhpmcounterh>("replaynumh"));
        csr_file.map(CSR_REPLAYLOADNUM, true, std::make_shared<component::csr::mhpmcounter>("replayloadnum"));
        csr_file.map(CSR_REPLAYLOADNUMH, true, std::make_shared<component::csr::mhpmcounterh>("replayloadnumh"));
        csr_file.map(CSR_CONFLICTLOADNUM, true, std::make_shared<component::csr::mhpmcounter>("conflictloadnum"));
        csr_file.map(CSR_CONFLICTLOADNUMH, true, std::make_shared<component::csr::mhpmcounterh>("conflictloadnumh"));
    
        for(auto i = 0;i < 16;i++)
        {
            csr_file.map(0x3A0 + i, false, std::make_shared<component::csr::pmpcfg>(i));
        }
    
        for(auto i = 0;i < 64;i++)
        {
            csr_file.map(0x3B0 + i, false, std::make_shared<component::csr::pmpaddr>(i));
        }
    
        wb_stage.init();
        commit_stage.init();
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
        
        for(uint32_t i = 0;i < LU_UNIT_NUM;i++)
        {
            delete readreg_lu_hdff[i];
            delete execute_lu_stage[i];
            delete lu_wb_port[i];
        }
    
        for(uint32_t i = 0;i < SAU_UNIT_NUM;i++)
        {
            delete readreg_sau_hdff[i];
            delete execute_sau_stage[i];
            delete sau_wb_port[i];
        }
    
        for(uint32_t i = 0;i < SDU_UNIT_NUM;i++)
        {
            delete readreg_sdu_hdff[i];
            delete execute_sdu_stage[i];
            delete sdu_wb_port[i];
        }
    }
    
    void cycle_model::load(void *mem, size_t size)
    {
        auto buf = (uint8_t *)mem;
        verify_only(size < MEMORY_SIZE);
        
        for(size_t i = 0;i < size;i++)
        {
            bus.write8(MEMORY_BASE + i, buf[i]);
        }
    }
    
    void cycle_model::reset()
    {
        component::dff_base::reset_all();
        phy_id_free_list.reset();
        speculative_rat.reset();
        retire_rat.reset();
        speculative_rat.init_start();
        retire_rat.init_start();
        phy_regfile.reset();
        component::dff_base::sync_all();
        
        for(uint32_t i = 1;i < ARCH_REG_NUM;i++)
        {
            uint32_t phy_id = 0;
            verify(phy_id_free_list.pop(&phy_id));
            speculative_rat.set_map(i, phy_id);
            retire_rat.set_map(i, phy_id);
            phy_regfile.write(phy_id, 0, true, 0, false, true);
            component::dff_base::sync_all();
        }
        
        speculative_rat.init_finish();
        retire_rat.init_finish();
        component::branch_predictor_base::batch_reset();
        branch_predictor_set.main_ras.reset();
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
            alu_commit_port[i]->reset();
        }
    
        for(uint32_t i = 0;i < BRU_UNIT_NUM;i++)
        {
            readreg_bru_hdff[i]->reset();
            execute_bru_stage[i]->reset();
            bru_wb_port[i]->reset();
            bru_commit_port[i]->reset();
        }
    
        for(uint32_t i = 0;i < CSR_UNIT_NUM;i++)
        {
            readreg_csr_hdff[i]->reset();
            execute_csr_stage[i]->reset();
            csr_wb_port[i]->reset();
            csr_commit_port[i]->reset();
        }
    
        for(uint32_t i = 0;i < DIV_UNIT_NUM;i++)
        {
            readreg_div_hdff[i]->reset();
            execute_div_stage[i]->reset();
            div_wb_port[i]->reset();
            div_commit_port[i]->reset();
        }
    
        for(uint32_t i = 0;i < MUL_UNIT_NUM;i++)
        {
            readreg_mul_hdff[i]->reset();
            execute_mul_stage[i]->reset();
            mul_wb_port[i]->reset();
            mul_commit_port[i]->reset();
        }
        
        for(uint32_t i = 0;i < LU_UNIT_NUM;i++)
        {
            readreg_lu_hdff[i]->reset();
            execute_lu_stage[i]->reset();
            lu_wb_port[i]->reset();
            lu_commit_port[i]->reset();
        }
    
        for(uint32_t i = 0;i < SAU_UNIT_NUM;i++)
        {
            readreg_sau_hdff[i]->reset();
            execute_sau_stage[i]->reset();
            sau_wb_port[i]->reset();
            sau_commit_port[i]->reset();
        }
        
        for(uint32_t i = 0;i < SDU_UNIT_NUM;i++)
        {
            readreg_sdu_hdff[i]->reset();
            execute_sdu_stage[i]->reset();
            sdu_wb_port[i]->reset();
            sdu_commit_port[i]->reset();
        }
        
        bus.reset();
        rob.reset();
        csr_file.reset();
        store_buffer.reset();
        load_queue.reset();
        checkpoint_buffer.reset();
        interrupt_interface.reset();
        ((component::slave::memory *)bus.get_slave_obj(0x80000000, false))->reset();
        clint.reset();
        checkpoint_buffer.reset();
        
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
        
        global.branch_num = 0;
        global.branch_predicted = 0;
        global.branch_hit = 0;
        global. branch_miss = 0;
    
        component::dff_base::sync_all();
    }
    
    void cycle_model::run()
    {
        rob.set_committed(false);
        clint.run_pre();
        commit_feedback_pack = commit_stage.run();
        bru_feedback_pack.flush = false;
        sau_feedback_pack.flush = false;
        bru_feedback_pack = std::get<pipeline::execute::bru_feedback_pack_t>(execute_bru_stage[0]->run(sau_feedback_pack, commit_feedback_pack, true));
        sau_feedback_pack = execute_sau_stage[0]->run(bru_feedback_pack, commit_feedback_pack, true);
        wb_feedback_pack = wb_stage.run(bru_feedback_pack, sau_feedback_pack, commit_feedback_pack);
        
        if(bru_feedback_pack.flush && sau_feedback_pack.flush)
        {
            if(component::age_compare(bru_feedback_pack.rob_id, bru_feedback_pack.rob_id_stage) < component::age_compare(sau_feedback_pack.rob_id, sau_feedback_pack.rob_id_stage))
            {
                bru_feedback_pack.flush = false;
            }
            else
            {
                sau_feedback_pack.flush = false;
            }
        }
        
        uint32_t execute_feedback_channel = 0;
        
        for(uint32_t i = 0;i < ALU_UNIT_NUM;i++)
        {
            execute_feedback_pack.channel[execute_feedback_channel++] = execute_alu_stage[i]->run(bru_feedback_pack, sau_feedback_pack, commit_feedback_pack);
        }
        
        for(uint32_t i = 0;i < BRU_UNIT_NUM;i++)
        {
            execute_feedback_pack.channel[execute_feedback_channel++] = std::get<pipeline::execute_feedback_channel_t>(execute_bru_stage[i]->run(sau_feedback_pack, commit_feedback_pack, false));
        }
        
        for(uint32_t i = 0;i < CSR_UNIT_NUM;i++)
        {
            execute_feedback_pack.channel[execute_feedback_channel++] = execute_csr_stage[i]->run(commit_feedback_pack);
        }
        
        for(uint32_t i = 0;i < DIV_UNIT_NUM;i++)
        {
            execute_feedback_pack.channel[execute_feedback_channel++] = execute_div_stage[i]->run(bru_feedback_pack, sau_feedback_pack, commit_feedback_pack);
        }
        
        for(uint32_t i = 0;i < MUL_UNIT_NUM;i++)
        {
            execute_feedback_pack.channel[execute_feedback_channel++] = execute_mul_stage[i]->run(bru_feedback_pack, sau_feedback_pack, commit_feedback_pack);
        }
        
        for(uint32_t i = 0;i < LU_UNIT_NUM;i++)
        {
            std::tie(execute_feedback_pack.channel[execute_feedback_channel++], lu_feedback_pack[i]) = execute_lu_stage[i]->run(bru_feedback_pack, sau_feedback_pack, commit_feedback_pack);
        }
        
        for(uint32_t i = 0;i < SAU_UNIT_NUM;i++)
        {
            execute_sau_stage[i]->run(bru_feedback_pack, commit_feedback_pack, false);
        }
        
        for(uint32_t i = 0;i < SDU_UNIT_NUM;i++)
        {
            execute_sdu_stage[i]->run(bru_feedback_pack, sau_feedback_pack, commit_feedback_pack);
        }
        
        integer_readreg_stage.run(bru_feedback_pack, lu_feedback_pack[0], sau_feedback_pack, execute_feedback_pack, wb_feedback_pack, commit_feedback_pack);
        lsu_readreg_feedback_pack = lsu_readreg_stage.run(bru_feedback_pack, lu_feedback_pack[0], sau_feedback_pack, execute_feedback_pack, wb_feedback_pack, commit_feedback_pack);
        integer_issue_output_feedback_pack = integer_issue_stage.run_output(bru_feedback_pack, lu_feedback_pack[0], sau_feedback_pack, commit_feedback_pack);
        lsu_issue_output_feedback_pack = lsu_issue_stage.run_output(lsu_readreg_feedback_pack, bru_feedback_pack, lu_feedback_pack[0], sau_feedback_pack, commit_feedback_pack);
        integer_issue_stage.run_wakeup(integer_issue_output_feedback_pack, lsu_issue_output_feedback_pack, bru_feedback_pack, lu_feedback_pack[0], sau_feedback_pack, execute_feedback_pack, commit_feedback_pack);
        lsu_issue_stage.run_wakeup(integer_issue_output_feedback_pack, lsu_issue_output_feedback_pack, bru_feedback_pack, lu_feedback_pack[0], sau_feedback_pack, execute_feedback_pack, commit_feedback_pack);
        integer_issue_feedback_pack = integer_issue_stage.run_input(bru_feedback_pack, sau_feedback_pack, execute_feedback_pack, wb_feedback_pack, commit_feedback_pack);
        lsu_issue_feedback_pack = lsu_issue_stage.run_input(bru_feedback_pack, sau_feedback_pack, execute_feedback_pack, wb_feedback_pack, commit_feedback_pack);
        dispatch_feedback_pack = dispatch_stage.run(integer_issue_feedback_pack, lsu_issue_feedback_pack, bru_feedback_pack, sau_feedback_pack, commit_feedback_pack);
        rename_feedback_pack = rename_stage.run(dispatch_feedback_pack, bru_feedback_pack, sau_feedback_pack, commit_feedback_pack);
        decode_feedback_pack = decode_stage.run(bru_feedback_pack, sau_feedback_pack, commit_feedback_pack);
        fetch2_feedback_pack = fetch2_stage.run(bru_feedback_pack, sau_feedback_pack, commit_feedback_pack);
        fetch1_stage.run(fetch2_feedback_pack, decode_feedback_pack, rename_feedback_pack, bru_feedback_pack, sau_feedback_pack, commit_feedback_pack);
        interrupt_interface.run();
        clint.run_post();
        store_buffer.run(bru_feedback_pack, sau_feedback_pack, commit_feedback_pack);
        bus.run();
        bus.sync();
        component::branch_predictor_base::batch_sync();
        cpu_clock_cycle++;
        committed_instruction_num = rob.get_global_commit_num();
        csr_file.write_sys(CSR_MCYCLE, (uint32_t)(cpu_clock_cycle & 0xffffffffu));
        csr_file.write_sys(CSR_MCYCLEH, (uint32_t)(cpu_clock_cycle >> 32));
        csr_file.write_sys(CSR_MINSTRET, (uint32_t)(committed_instruction_num & 0xffffffffu));
        csr_file.write_sys(CSR_MINSTRETH, (uint32_t)(committed_instruction_num >> 32));
        csr_file.write_sys(CSR_BRANCHNUM, (uint32_t)(global.branch_num & 0xffffffffu));
        csr_file.write_sys(CSR_BRANCHNUMH, (uint32_t)(global.branch_num >> 32));
        csr_file.write_sys(CSR_BRANCHPREDICTED, (uint32_t)(global.branch_predicted & 0xffffffffu));
        csr_file.write_sys(CSR_BRANCHPREDICTEDH, (uint32_t)(global.branch_predicted >> 32));
        csr_file.write_sys(CSR_BRANCHHIT, (uint32_t)(global.branch_hit & 0xffffffffu));
        csr_file.write_sys(CSR_BRANCHHITH, (uint32_t)(global.branch_hit >> 32));
        csr_file.write_sys(CSR_BRANCHMISS, (uint32_t)(global.branch_miss & 0xffffffffu));
        csr_file.write_sys(CSR_BRANCHMISSH, (uint32_t)(global.branch_miss >> 32));
        csr_file.write_sys(CSR_LOADNUM, (uint32_t)(global.load_num & 0xffffffffu));
        csr_file.write_sys(CSR_LOADNUMH, (uint32_t)(global.load_num >> 32));
        csr_file.write_sys(CSR_REPLAYNUM, (uint32_t)(global.replay_num & 0xffffffffu));
        csr_file.write_sys(CSR_REPLAYNUMH, (uint32_t)(global.replay_num >> 32));
        csr_file.write_sys(CSR_REPLAYLOADNUM, (uint32_t)(global.replay_load_num & 0xffffffffu));
        csr_file.write_sys(CSR_REPLAYLOADNUMH, (uint32_t)(global.replay_load_num >> 32));
        csr_file.write_sys(CSR_CONFLICTLOADNUM, (uint32_t)(global.conflict_load_num & 0xffffffffu));
        csr_file.write_sys(CSR_CONFLICTLOADNUMH, (uint32_t)(global.conflict_load_num >> 32));
        
        for(uint32_t i = 0;i < ALU_UNIT_NUM;i++)
        {
            alu_commit_port[i]->set(execute_wb_to_commit_pack(alu_wb_port[i]->get_new()));
        }
    
        for(uint32_t i = 0;i < BRU_UNIT_NUM;i++)
        {
            bru_commit_port[i]->set(execute_wb_to_commit_pack(bru_wb_port[i]->get_new()));
        }
    
        for(uint32_t i = 0;i < CSR_UNIT_NUM;i++)
        {
            csr_commit_port[i]->set(execute_wb_to_commit_pack(csr_wb_port[i]->get_new()));
        }
        
        for(uint32_t i = 0;i < DIV_UNIT_NUM;i++)
        {
            div_commit_port[i]->set(execute_wb_to_commit_pack(div_wb_port[i]->get_new()));
        }
        
        for(uint32_t i = 0;i < MUL_UNIT_NUM;i++)
        {
            mul_commit_port[i]->set(execute_wb_to_commit_pack(mul_wb_port[i]->get_new()));
        }
    
        for(uint32_t i = 0;i < LU_UNIT_NUM;i++)
        {
            lu_commit_port[i]->set(execute_wb_to_commit_pack(lu_wb_port[i]->get_new()));
        }
    
        for(uint32_t i = 0;i < SAU_UNIT_NUM;i++)
        {
            sau_commit_port[i]->set(execute_wb_to_commit_pack(sau_wb_port[i]->get_new()));
        }
    
        for(uint32_t i = 0;i < SDU_UNIT_NUM;i++)
        {
            sdu_commit_port[i]->set(execute_wb_to_commit_pack(sdu_wb_port[i]->get_new()));
        }
    
        component::dff_base::sync_all();
    }
    
    pipeline::execute_commit_pack_t cycle_model::execute_wb_to_commit_pack(const pipeline::execute_wb_pack_t &rev_pack)
    {
        pipeline::execute_commit_pack_t send_pack;
    
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
    
        send_pack.bru_jump = rev_pack.bru_jump;
        send_pack.bru_next_pc = rev_pack.bru_next_pc;
    
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
        send_pack.rd_value = rev_pack.rd_value;
    
        send_pack.csr = rev_pack.csr;
        send_pack.load_queue_id = rev_pack.load_queue_id;
        send_pack.csr_newvalue = rev_pack.csr_newvalue;
        send_pack.csr_newvalue_valid = rev_pack.csr_newvalue_valid;
        send_pack.op = rev_pack.op;
        send_pack.op_unit = rev_pack.op_unit;
        memcpy((void *)&send_pack.sub_op, (void *)&rev_pack.sub_op, sizeof(rev_pack.sub_op));
        return send_pack;
    }
}