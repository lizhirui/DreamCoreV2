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
#include "isa_model/isa_model.h"
#include "isa_model/component/bus.h"
#include "isa_model/component/csr_all.h"
#include "isa_model/component/csrfile.h"
#include "isa_model/component/interrupt_interface.h"
#include "isa_model/component/regfile.h"
#include "isa_model/component/slave/memory.h"
#include "isa_model/component/slave/clint.h"

namespace isa_model
{
    isa_model *isa_model::instance = nullptr;
    
    isa_model *isa_model::create(boost::lockfree::spsc_queue<char, boost::lockfree::capacity<CHARFIFO_SEND_FIFO_SIZE>> *charfifo_send_fifo, boost::lockfree::spsc_queue<char, boost::lockfree::capacity<CHARFIFO_REV_FIFO_SIZE>> *charfifo_rev_fifo)
    {
        if(instance == nullptr)
        {
            instance = new isa_model(charfifo_send_fifo, charfifo_rev_fifo);
        }
        
        return instance;
    }
    
    void isa_model::destroy()
    {
        if(instance != nullptr)
        {
            delete instance;
            instance = nullptr;
        }
    }
    
    isa_model::isa_model(boost::lockfree::spsc_queue<char, boost::lockfree::capacity<CHARFIFO_SEND_FIFO_SIZE>> *charfifo_send_fifo, boost::lockfree::spsc_queue<char, boost::lockfree::capacity<CHARFIFO_REV_FIFO_SIZE>> *charfifo_rev_fifo) :
    charfifo_send_fifo(charfifo_send_fifo),
    charfifo_rev_fifo(charfifo_rev_fifo),
    interrupt_interface(&csr_file),
    arch_regfile(ARCH_REG_NUM),
    clint(&bus, &interrupt_interface)
    {
        bus.map(MEMORY_BASE, MEMORY_SIZE, std::make_shared<component::slave::memory>(&bus), true);
        bus.map(CLINT_BASE, CLINT_SIZE, std::shared_ptr<component::slave::clint>(&clint), false);
    
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
        
        this->reset();
    }
    
    isa_model::~isa_model()
    {
    
    }
    
    void isa_model::load(void *mem, size_t size)
    {
        auto buf = (uint8_t *)mem;
        verify(size < MEMORY_SIZE);
    
        for(auto i = 0;i < size;i++)
        {
            bus.write8(MEMORY_BASE + i, buf[i]);
        }
    }
    
    void isa_model::reset()
    {
        arch_regfile.reset();
        bus.reset();
        csr_file.reset();
        interrupt_interface.reset();
        ((component::slave::memory *)bus.get_slave_obj(0x80000000, false))->reset();
        clint.reset();
    
        cpu_clock_cycle = 0;
        committed_instruction_num = 0;
    
        branch_num = 1;
        branch_predicted = 0;
        branch_hit = 0;
        branch_miss = 0;
        pc = INIT_PC;
    }
    
    void isa_model::run()
    {
        clint.run_pre();
        auto fetch_decode_pack = fetch();
        auto decode_execute_pack = decode(fetch_decode_pack);
        execute(decode_execute_pack);
        interrupt_interface.run();
        clint.run_post();
        cpu_clock_cycle++;
        committed_instruction_num++;
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
    }
    
    fetch_decode_pack_t isa_model::fetch()
    {
    
    }
    
    decode_execute_pack_t isa_model::decode(const fetch_decode_pack_t &fetch_decode_pack)
    {
    
    }
    
    void isa_model::execute(const decode_execute_pack_t &decode_execute_pack)
    {
    
    }
    
    void isa_model::execute_alu(const decode_execute_pack_t &decode_execute_pack)
    {
    
    }
    
    void isa_model::execute_bru(const decode_execute_pack_t &decode_execute_pack)
    {
    
    }
    
    void isa_model::execute_csr(const decode_execute_pack_t &decode_execute_pack)
    {
    
    }
    
    void isa_model::execute_div(const decode_execute_pack_t &decode_execute_pack)
    {
    
    }
    
    void isa_model::execute_mul(const decode_execute_pack_t &decode_execute_pack)
    {
    
    }
    
    void isa_model::execute_lsu(const decode_execute_pack_t &decode_execute_pack)
    {
    
    }
    
    void isa_model::handle_exception()
    {
    
    }
}