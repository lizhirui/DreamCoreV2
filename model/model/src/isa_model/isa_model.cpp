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
#include "isa_model/csr_inst.h"
#include "breakpoint.h"

namespace isa_model
{
    isa_model *isa_model::instance = nullptr;
    
    isa_model *isa_model::create(charfifo_send_fifo_t *charfifo_send_fifo, charfifo_rev_fifo_t *charfifo_rev_fifo)
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
    
    isa_model::isa_model(charfifo_send_fifo_t *charfifo_send_fifo, charfifo_rev_fifo_t *charfifo_rev_fifo) :
    charfifo_send_fifo(charfifo_send_fifo),
    charfifo_rev_fifo(charfifo_rev_fifo),
#ifdef BRANCH_DUMP
    branch_dump_stream(BRANCH_DUMP_FILE),
#endif
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
        csr_file.map(CSR_MSTATUS, false, std::shared_ptr<component::csr::mstatus>(&csr_inst::mstatus, boost::null_deleter()));
        csr_file.map(CSR_MISA, false, std::make_shared<component::csr::misa>());
        csr_file.map(CSR_MIE, false, std::shared_ptr<component::csr::mie>(&csr_inst::mie, boost::null_deleter()));
        csr_file.map(CSR_MTVEC, false, std::shared_ptr<component::csr::mtvec>(&csr_inst::mtvec, boost::null_deleter()));
        csr_file.map(CSR_MCOUNTEREN, false, std::make_shared<component::csr::mcounteren>());
        csr_file.map(CSR_MSTATUSH, false, std::make_shared<component::csr::mstatush>());
        csr_file.map(CSR_MSCRATCH, false, std::make_shared<component::csr::mscratch>());
        csr_file.map(CSR_MEPC, false, std::shared_ptr<component::csr::mepc>(&csr_inst::mepc, boost::null_deleter()));
        csr_file.map(CSR_MCAUSE, false, std::shared_ptr<component::csr::mcause>(&csr_inst::mcause, boost::null_deleter()));
        csr_file.map(CSR_MTVAL, false, std::shared_ptr<component::csr::mtval>(&csr_inst::mtval, boost::null_deleter()));
        csr_file.map(CSR_MIP, false, std::shared_ptr<component::csr::mip>(&csr_inst::mip, boost::null_deleter()));
        csr_file.map(CSR_CHARFIFO, false, std::make_shared<component::csr::charfifo>(charfifo_send_fifo, charfifo_rev_fifo));
        csr_file.map(CSR_FINISH, false, std::shared_ptr<component::csr::finish>(&csr_inst::finish, boost::null_deleter()));
        csr_file.map(CSR_MCYCLE, false, std::shared_ptr<component::csr::mcycle>(&csr_inst::mcycle, boost::null_deleter()));
        csr_file.map(CSR_MINSTRET, false, std::shared_ptr<component::csr::minstret>(&csr_inst::minstret, boost::null_deleter()));
        csr_file.map(CSR_MCYCLEH, false, std::shared_ptr<component::csr::mcycleh>(&csr_inst::mcycleh, boost::null_deleter()));
        csr_file.map(CSR_MINSTRETH, false, std::shared_ptr<component::csr::minstreth>(&csr_inst::minstreth, boost::null_deleter()));
        csr_file.map(CSR_BRANCHNUM, true, std::shared_ptr<component::csr::mhpmcounter>(&csr_inst::branchnum, boost::null_deleter()));
        csr_file.map(CSR_BRANCHNUMH, true, std::shared_ptr<component::csr::mhpmcounterh>(&csr_inst::branchnumh, boost::null_deleter()));
        csr_file.map(CSR_BRANCHPREDICTED, true, std::shared_ptr<component::csr::mhpmcounter>(&csr_inst::branchpredicted, boost::null_deleter()));
        csr_file.map(CSR_BRANCHPREDICTEDH, true, std::shared_ptr<component::csr::mhpmcounterh>(&csr_inst::branchpredictedh, boost::null_deleter()));
        csr_file.map(CSR_BRANCHHIT, true, std::shared_ptr<component::csr::mhpmcounter>(&csr_inst::branchhit, boost::null_deleter()));
        csr_file.map(CSR_BRANCHHITH, true, std::shared_ptr<component::csr::mhpmcounterh>(&csr_inst::branchhith, boost::null_deleter()));
        csr_file.map(CSR_BRANCHMISS, true, std::shared_ptr<component::csr::mhpmcounter>(&csr_inst::branchmiss, boost::null_deleter()));
        csr_file.map(CSR_BRANCHMISSH, true, std::shared_ptr<component::csr::mhpmcounterh>(&csr_inst::branchmissh, boost::null_deleter()));
    
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
        verify_only(size < MEMORY_SIZE);
    
        for(size_t i = 0;i < size;i++)
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
    
    void isa_model::pause_event()
    {
#ifdef BRANCH_DUMP
        branch_dump_stream.flush();
#endif
    }
    
    void isa_model::profile(uint32_t pc)
    {
        const uint iter_count = 100000000;
        clock_t start, end;
        long double duration;
        
        //fetch profile
        {
            auto i = iter_count;
            isa_state_t isa_state;
            start = clock();
            
            do
            {
                this->pc = pc;
                fetch(isa_state);
            }while(--i);
            
            end = clock();
            duration = ((end - start) * 1000000000.0l) / CLOCKS_PER_SEC / iter_count;
            std::cout << "fetch profile: " << duration << "ns" << std::endl;
        }
        
        //decode profile
        {
            auto i = iter_count;
            isa_state_t isa_state;
            fetch(isa_state);
            auto has_exception = isa_state.has_exception;
            start = clock();
    
            do
            {
                isa_state.has_exception = has_exception;
                decode(isa_state);
            }while(--i);
            
            end = clock();
            duration = ((end - start) * 1000000000.0l) / CLOCKS_PER_SEC / iter_count;
            std::cout << "decode profile: " << duration << "ns" << std::endl;
        }
        
        //execute profile
        {
            auto i = iter_count;
            isa_state_t isa_state;
            fetch(isa_state);
            decode(isa_state);
            auto has_exception = isa_state.has_exception;
            start = clock();
    
            do
            {
                isa_state.has_exception = has_exception;
                execute(isa_state);
            }while(--i);
            
            end = clock();
            duration = ((end - start) * 1000000000.0l) / CLOCKS_PER_SEC / iter_count;
            std::cout << "execute profile: " << duration << "ns" << std::endl;
        }
        
        //interrupt_interface profile
        {
            auto i = iter_count;
            start = clock();
    
            do
            {
                interrupt_interface.run();
            }while(--i);
            
            end = clock();
            duration = ((end - start) * 1000000000.0l) / CLOCKS_PER_SEC / iter_count;
            std::cout << "interrupt_interface profile: " << duration << "ns" << std::endl;
        }
        
        //clint profile
        {
            auto i = iter_count;
            start = clock();
    
            do
            {
                clint.run();
            }while(--i);
            
            end = clock();
            duration = ((end - start) * 1000000000.0l) / CLOCKS_PER_SEC / iter_count;
            std::cout << "clint profile: " << duration << "ns" << std::endl;
        }
        
        //csr_write_sys profile
        {
            auto i = iter_count;
            start = clock();
    
            do
            {
                csr_file.write_sys(CSR_BRANCHHIT, 0);
            }while(--i);
            
            end = clock();
            duration = ((end - start) * 1000000000.0l) / CLOCKS_PER_SEC / iter_count;
            std::cout << "csr_write_sys profile: " << duration << "ns" << std::endl;
        }
        
        //csr_read_sys profile
        {
            auto i = iter_count;
            start = clock();
    
            do
            {
                volatile uint32_t ret = csr_file.read_sys(CSR_BRANCHHIT);
            }while(--i);
            
            end = clock();
            duration = ((end - start) * 1000000000.0l) / CLOCKS_PER_SEC / iter_count;
            std::cout << "csr_read_sys profile: " << duration << "ns" << std::endl;
        }
        
        //csr_specified_write profile
        {
            auto i = iter_count;
            start = clock();
        
            do
            {
                volatile uint32_t t = 0;
                csr_inst::branchnum.write(t);
            }while(--i);
        
            end = clock();
            duration = ((end - start) * 1000000000.0l) / CLOCKS_PER_SEC / iter_count;
            std::cout << "csr_specified_write profile: " << duration << "ns" << std::endl;
        }
    
        //csr_specified_read profile
        {
            auto i = iter_count;
            start = clock();
        
            do
            {
                volatile uint32_t ret = csr_inst::branchnum.read();
            }while(--i);
        
            end = clock();
            duration = ((end - start) * 1000000000.0l) / CLOCKS_PER_SEC / iter_count;
            std::cout << "csr_specified_read profile: " << duration << "ns" << std::endl;
        }
        
        //memory_write profile
        {
            auto i = iter_count;
            start = clock();
            
            do
            {
                bus.write32(0x80000000, 0x12345678);
            }while(--i);
    
            end = clock();
            duration = ((end - start) * 1000000000.0l) / CLOCKS_PER_SEC / iter_count;
            std::cout << "memory_write profile: " << duration << "ns" << std::endl;
        }
    
        //memory_read profile
        {
            auto i = iter_count;
            start = clock();
        
            do
            {
                volatile uint32_t ret = bus.read32(0x80000000, true);
            }while(--i);
        
            end = clock();
            duration = ((end - start) * 1000000000.0l) / CLOCKS_PER_SEC / iter_count;
            std::cout << "memory_read profile: " << duration << "ns" << std::endl;
        }
    }
    
    void isa_model::run()
    {
        riscv_interrupt_t interrupt_id;
        isa_state_t isa_state;
        fetch(isa_state);
        
#ifdef BRANCH_DUMP
        branch_dump_stream << outhex(isa_state.pc) << "," << outhex(isa_state.value) << std::endl;
#endif
        decode(isa_state);
        execute(isa_state);
        interrupt_interface.run();
        clint.run();
    
        if(interrupt_interface.get_cause(&interrupt_id))
        {
            csr_inst::mepc.write(this->pc);
            csr_inst::mtval.write(0);
            csr_inst::mcause.write(0x80000000 | static_cast<uint32_t>(interrupt_id));
            component::csr::mstatus mstatus;
            mstatus.load(csr_inst::mstatus.read());
            mstatus.set_mpie(mstatus.get_mie());
            mstatus.set_mie(false);
            csr_inst::mstatus.write(mstatus.get_value());
            this->pc = csr_inst::mtvec.read();
        }
        
        cpu_clock_cycle++;
        committed_instruction_num++;
        commit_num++;//only for debugger
        csr_inst::mcycle.write((uint32_t)(cpu_clock_cycle & 0xffffffffu));
        csr_inst::mcycleh.write((uint32_t)(cpu_clock_cycle >> 32));
        csr_inst::minstret.write((uint32_t)(committed_instruction_num & 0xffffffffu));
        csr_inst::minstreth.write((uint32_t)(committed_instruction_num >> 32));
        csr_inst::branchnum.write((uint32_t)(branch_num & 0xffffffffu));
        csr_inst::branchnumh.write((uint32_t)(branch_num >> 32));
        csr_inst::branchpredicted.write((uint32_t)(branch_predicted & 0xffffffffu));
        csr_inst::branchpredictedh.write((uint32_t)(branch_predicted >> 32));
        csr_inst::branchhit.write((uint32_t)(branch_hit & 0xffffffffu));
        csr_inst::branchhith.write((uint32_t)(branch_hit >> 32));
        csr_inst::branchmiss.write((uint32_t)(branch_miss & 0xffffffffu));
        csr_inst::branchmissh.write((uint32_t)(branch_miss >> 32));
    }
    
    void isa_model::fetch(isa_state_t &isa_state)
    {
        isa_state.pc = this->pc;
        isa_state.value  = 0;
        isa_state.has_exception = false;
        isa_state.exception_id = riscv_exception_t::instruction_access_fault;
        isa_state.exception_value = 0;
        
        if(!component::bus::check_align(this->pc, 4))
        {
            isa_state.has_exception = true;
            isa_state.exception_id = riscv_exception_t::instruction_address_misaligned;
            isa_state.exception_value = this->pc;
        }
        
        isa_state.value = bus.read32(this->pc, true);
        this->pc += 4;
    }
    
    void isa_model::decode(isa_state_t &isa_state)
    {
        decode_execute_pack_t send_pack;
        
        auto op_data = isa_state.value;
        auto opcode = op_data & 0x7f;
        auto rd = (op_data >> 7) & 0x1f;
        auto funct3 = (op_data >> 12) & 0x07;
        auto rs1 = (op_data >> 15) & 0x1f;
        auto rs2 = (op_data >> 20) & 0x1f;
        auto funct7 = (op_data >> 25) & 0x7f;
        auto imm_i = (op_data >> 20) & 0xfff;
        auto imm_s = (((op_data >> 7) & 0x1f)) | (((op_data >> 25) & 0x7f) << 5);
        auto imm_b = (((op_data >> 8) & 0x0f) << 1) | (((op_data >> 25) & 0x3f) << 5) | (((op_data >> 7) & 0x01) << 11) | (((op_data >> 31) & 0x01) << 12);
        auto imm_u = op_data & (~0xfff);
        auto imm_j = (((op_data >> 12) & 0xff) << 12) | (((op_data >> 20) & 0x01) << 11) | (((op_data >> 21) & 0x3ff) << 1) | (((op_data >> 31) & 0x01) << 20);
        
        isa_state.value = 0;
        isa_state.valid = !isa_state.has_exception;
        isa_state.imm = 0;
        isa_state.rs1 = 0;
        isa_state.arg1_src = arg_src_t::reg;
        isa_state.rs2 = 0;
        isa_state.rd = 0;
        isa_state.rd_enable = false;
        isa_state.csr = 0;
        isa_state.op = op_t::add;
        isa_state.op_unit = op_unit_t::alu;
        isa_state.sub_op.alu_op = alu_op_t::add;
    
        switch(opcode)
        {
            case 0x37://lui
                isa_state.op = op_t::lui;
                isa_state.op_unit = op_unit_t::alu;
                isa_state.sub_op.alu_op = alu_op_t::lui;
                isa_state.arg1_src = arg_src_t::disable;
                isa_state.arg2_src = arg_src_t::disable;
                isa_state.imm = imm_u;
                isa_state.rd = rd;
                isa_state.rd_enable = true;
                break;
        
            case 0x17://auipc
                isa_state.op = op_t::auipc;
                isa_state.op_unit = op_unit_t::alu;
                isa_state.sub_op.alu_op = alu_op_t::auipc;
                isa_state.arg1_src = arg_src_t::disable;
                isa_state.arg2_src = arg_src_t::disable;
                isa_state.imm = imm_u;
                isa_state.rd = rd;
                isa_state.rd_enable = true;
                break;
        
            case 0x6f://jal
                isa_state.op = op_t::jal;
                isa_state.op_unit = op_unit_t::bru;
                isa_state.sub_op.bru_op = bru_op_t::jal;
                isa_state.arg1_src = arg_src_t::disable;
                isa_state.arg2_src = arg_src_t::disable;
                isa_state.imm = sign_extend(imm_j, 21);
                isa_state.rd = rd;
                isa_state.rd_enable = true;
                break;
        
            case 0x67://jalr
                isa_state.op = op_t::jalr;
                isa_state.op_unit = op_unit_t::bru;
                isa_state.sub_op.bru_op = bru_op_t::jalr;
                isa_state.arg1_src = arg_src_t::reg;
                isa_state.rs1 = rs1;
                isa_state.arg2_src = arg_src_t::disable;
                isa_state.imm = sign_extend(imm_i, 12);
                isa_state.rd = rd;
                isa_state.rd_enable = true;
                break;
        
            case 0x63://beq bne blt bge bltu bgeu
                isa_state.op_unit = op_unit_t::bru;
                isa_state.arg1_src = arg_src_t::reg;
                isa_state.rs1 = rs1;
                isa_state.arg2_src = arg_src_t::reg;
                isa_state.rs2 = rs2;
                isa_state.imm = sign_extend(imm_b, 13);
            
                switch(funct3)
                {
                    case 0x0://beq
                        isa_state.op = op_t::beq;
                        isa_state.sub_op.bru_op = bru_op_t::beq;
                        break;
                
                    case 0x1://bne
                        isa_state.op = op_t::bne;
                        isa_state.sub_op.bru_op = bru_op_t::bne;
                        break;
                
                    case 0x4://blt
                        isa_state.op = op_t::blt;
                        isa_state.sub_op.bru_op = bru_op_t::blt;
                        break;
                
                    case 0x5://bge
                        isa_state.op = op_t::bge;
                        isa_state.sub_op.bru_op = bru_op_t::bge;
                        break;
                
                    case 0x6://bltu
                        isa_state.op = op_t::bltu;
                        isa_state.sub_op.bru_op = bru_op_t::bltu;
                        break;
                
                    case 0x7://bgeu
                        isa_state.op = op_t::bgeu;
                        isa_state.sub_op.bru_op = bru_op_t::bgeu;
                        break;
                
                    default://invalid
                        isa_state.valid = false;
                        break;
                }
            
                break;
        
            case 0x03://lb lh lw lbu lhu
                isa_state.op_unit = op_unit_t::lsu;
                isa_state.arg1_src = arg_src_t::reg;
                isa_state.rs1 = rs1;
                isa_state.arg2_src = arg_src_t::disable;
                isa_state.imm = sign_extend(imm_i, 12);
                isa_state.rd = rd;
                isa_state.rd_enable = true;
            
                switch(funct3)
                {
                    case 0x0://lb
                        isa_state.op = op_t::lb;
                        isa_state.sub_op.lsu_op = lsu_op_t::lb;
                        break;
                
                    case 0x1://lh
                        isa_state.op = op_t::lh;
                        isa_state.sub_op.lsu_op = lsu_op_t::lh;
                        break;
                
                    case 0x2://lw
                        isa_state.op = op_t::lw;
                        isa_state.sub_op.lsu_op = lsu_op_t::lw;
                        break;
                
                    case 0x4://lbu
                        isa_state.op = op_t::lbu;
                        isa_state.sub_op.lsu_op = lsu_op_t::lbu;
                        break;
                
                    case 0x5://lhu
                        isa_state.op = op_t::lhu;
                        isa_state.sub_op.lsu_op = lsu_op_t::lhu;
                        break;
                
                    default://invalid
                        isa_state.valid = false;
                        break;
                }
            
                break;
        
            case 0x23://sb sh sw
                isa_state.op_unit = op_unit_t::lsu;
                isa_state.arg1_src = arg_src_t::reg;
                isa_state.rs1 = rs1;
                isa_state.arg2_src = arg_src_t::reg;
                isa_state.rs2 = rs2;
                isa_state.imm = sign_extend(imm_s, 12);
            
                switch(funct3)
                {
                    case 0x0://sb
                        isa_state.op = op_t::sb;
                        isa_state.sub_op.lsu_op = lsu_op_t::sb;
                        break;
                
                    case 0x1://sh
                        isa_state.op = op_t::sh;
                        isa_state.sub_op.lsu_op = lsu_op_t::sh;
                        break;
                
                    case 0x2://sw
                        isa_state.op = op_t::sw;
                        isa_state.sub_op.lsu_op = lsu_op_t::sw;
                        break;
                
                    default://invalid
                        isa_state.valid = false;
                        break;
                }
            
                break;
        
            case 0x13://addi slti sltiu xori ori andi slli srli srai
                isa_state.op_unit = op_unit_t::alu;
                isa_state.arg1_src = arg_src_t::reg;
                isa_state.rs1 = rs1;
                isa_state.arg2_src = arg_src_t::imm;
                isa_state.imm = sign_extend(imm_i, 12);
                isa_state.rd = rd;
                isa_state.rd_enable = true;
            
                switch(funct3)
                {
                    case 0x0://addi
                        isa_state.op = op_t::addi;
                        isa_state.sub_op.alu_op = alu_op_t::add;
                        break;
                
                    case 0x2://slti
                        isa_state.op = op_t::slti;
                        isa_state.sub_op.alu_op = alu_op_t::slt;
                        break;
                
                    case 0x3://sltiu
                        isa_state.op = op_t::sltiu;
                        isa_state.sub_op.alu_op = alu_op_t::sltu;
                        break;
                
                    case 0x4://xori
                        isa_state.op = op_t::xori;
                        isa_state.sub_op.alu_op = alu_op_t::_xor;
                        break;
                
                    case 0x6://ori
                        isa_state.op = op_t::ori;
                        isa_state.sub_op.alu_op = alu_op_t::_or;
                        break;
                
                    case 0x7://andi
                        isa_state.op = op_t::andi;
                        isa_state.sub_op.alu_op = alu_op_t::_and;
                        break;
                
                    case 0x1://slli
                        if(funct7 == 0x00)//slli
                        {
                            isa_state.op = op_t::slli;
                            isa_state.sub_op.alu_op = alu_op_t::sll;
                        }
                        else//invalid
                        {
                            isa_state.valid = false;
                        }
                    
                        break;
                
                    case 0x5://srli srai
                        if(funct7 == 0x00)//srli
                        {
                            isa_state.op = op_t::srli;
                            isa_state.sub_op.alu_op = alu_op_t::srl;
                        }
                        else if(funct7 == 0x20)//srai
                        {
                            isa_state.op = op_t::srai;
                            isa_state.sub_op.alu_op = alu_op_t::sra;
                        }
                        else//invalid
                        {
                            isa_state.valid = false;
                        }
                    
                        break;
                
                    default://invalid
                        isa_state.valid = false;
                        break;
                }
            
                break;
        
            case 0x33://add sub sll slt sltu xor srl sra or and mul mulh mulhsu mulhu div divu rem remu
                isa_state.op_unit = op_unit_t::alu;
                isa_state.arg1_src = arg_src_t::reg;
                isa_state.rs1 = rs1;
                isa_state.arg2_src = arg_src_t::reg;
                isa_state.rs2 = rs2;
                isa_state.rd = rd;
                isa_state.rd_enable = true;
            
                switch(funct3)
                {
                    case 0x0://add sub mul
                        if(funct7 == 0x00)//add
                        {
                            isa_state.op = op_t::add;
                            isa_state.sub_op.alu_op = alu_op_t::add;
                        }
                        else if(funct7 == 0x20)//sub
                        {
                            isa_state.op = op_t::sub;
                            isa_state.sub_op.alu_op = alu_op_t::sub;
                        }
                        else if(funct7 == 0x01)//mul
                        {
                            isa_state.op = op_t::mul;
                            isa_state.op_unit = op_unit_t::mul;
                            isa_state.sub_op.mul_op = mul_op_t::mul;
                        }
                        else//invalid
                        {
                            isa_state.valid = false;
                        }
                    
                        break;
                
                    case 0x1://sll mulh
                        if(funct7 == 0x00)//sll
                        {
                            isa_state.op = op_t::sll;
                            isa_state.sub_op.alu_op = alu_op_t::sll;
                        }
                        else if(funct7 == 0x01)//mulh
                        {
                            isa_state.op = op_t::mulh;
                            isa_state.op_unit = op_unit_t::mul;
                            isa_state.sub_op.mul_op = mul_op_t::mulh;
                        }
                        else//invalid
                        {
                            isa_state.valid = false;
                        }
                    
                        break;
                
                    case 0x2://slt mulhsu
                        if(funct7 == 0x00)//slt
                        {
                            isa_state.op = op_t::slt;
                            isa_state.sub_op.alu_op = alu_op_t::slt;
                        }
                        else if(funct7 == 0x01)//mulhsu
                        {
                            isa_state.op = op_t::mulhsu;
                            isa_state.op_unit = op_unit_t::mul;
                            isa_state.sub_op.mul_op = mul_op_t::mulhsu;
                        }
                        else//invalid
                        {
                            isa_state.valid = false;
                        }
                    
                        break;
                
                    case 0x3://sltu mulhu
                        if(funct7 == 0x00)//sltu
                        {
                            isa_state.op = op_t::sltu;
                            isa_state.sub_op.alu_op = alu_op_t::sltu;
                        }
                        else if(funct7 == 0x01)//mulhu
                        {
                            isa_state.op = op_t::mulhu;
                            isa_state.op_unit = op_unit_t::mul;
                            isa_state.sub_op.mul_op = mul_op_t::mulhu;
                        }
                        else//invalid
                        {
                            isa_state.valid = false;
                        }
                    
                        break;
                
                    case 0x4://xor div
                        if(funct7 == 0x00)//xor
                        {
                            isa_state.op = op_t::_xor;
                            isa_state.sub_op.alu_op = alu_op_t::_xor;
                        }
                        else if(funct7 == 0x01)//div
                        {
                            isa_state.op = op_t::div;
                            isa_state.op_unit = op_unit_t::div;
                            isa_state.sub_op.div_op = div_op_t::div;
                        }
                        else//invalid
                        {
                            isa_state.valid = false;
                        }
                    
                        break;
                
                    case 0x5://srl sra divu
                        if(funct7 == 0x00)//srl
                        {
                            isa_state.op = op_t::srl;
                            isa_state.sub_op.alu_op = alu_op_t::srl;
                        }
                        else if(funct7 == 0x20)//sra
                        {
                            isa_state.op = op_t::sra;
                            isa_state.sub_op.alu_op = alu_op_t::sra;
                        }
                        else if(funct7 == 0x01)//divu
                        {
                            isa_state.op = op_t::divu;
                            isa_state.op_unit = op_unit_t::div;
                            isa_state.sub_op.div_op = div_op_t::divu;
                        }
                        else//invalid
                        {
                            isa_state.valid = false;
                        }
                    
                        break;
                
                    case 0x6://or rem
                        if(funct7 == 0x00)//or
                        {
                            isa_state.op = op_t::_or;
                            isa_state.sub_op.alu_op = alu_op_t::_or;
                        }
                        else if(funct7 == 0x01)//rem
                        {
                            isa_state.op = op_t::rem;
                            isa_state.op_unit = op_unit_t::div;
                            isa_state.sub_op.div_op = div_op_t::rem;
                        }
                        else//invalid
                        {
                            isa_state.valid = false;
                        }
                    
                        break;
                
                    case 0x7://and remu
                        if(funct7 == 0x00)//and
                        {
                            isa_state.op = op_t::_and;
                            isa_state.sub_op.alu_op = alu_op_t::_and;
                        }
                        else if(funct7 == 0x01)//remu
                        {
                            isa_state.op = op_t::remu;
                            isa_state.op_unit = op_unit_t::div;
                            isa_state.sub_op.div_op = div_op_t::remu;
                        }
                        else//invalid
                        {
                            isa_state.valid = false;
                        }
                    
                        break;
                
                    default://invalid
                        isa_state.valid = false;
                        break;
                }
            
                break;
        
            case 0x0f://fence fence.i
                switch(funct3)
                {
                    case 0x0://fence
                        if((rd == 0x00) && (rs1 == 0x00) && (((op_data >> 28) & 0x0f) == 0x00))//fence
                        {
                            isa_state.op = op_t::fence;
                            isa_state.op_unit = op_unit_t::alu;
                            isa_state.sub_op.alu_op = alu_op_t::fence;
                            isa_state.arg1_src = arg_src_t::disable;
                            isa_state.arg2_src = arg_src_t::disable;
                            isa_state.imm = imm_i;
                        }
                        else//invalid
                        {
                            isa_state.valid = false;
                        }
                    
                        break;
                
                    case 0x1://fence.i
                        if((rd == 0x00) && (rs1 == 0x00) && (imm_i == 0x00))//fence.i
                        {
                            isa_state.op = op_t::fence_i;
                            isa_state.op_unit = op_unit_t::alu;
                            isa_state.sub_op.alu_op = alu_op_t::fence_i;
                            isa_state.arg1_src = arg_src_t::disable;
                            isa_state.arg2_src = arg_src_t::disable;
                        }
                        else//invalid
                        {
                            isa_state.valid = false;
                        }
                    
                        break;
                
                    default://invalid
                        isa_state.valid = false;
                        break;
                }
                break;
        
            case 0x73://ecall ebreak csrrw csrrs csrrc csrrwi csrrsi csrrci mret
                switch(funct3)
                {
                    case 0x0://ecall ebreak mret
                        if((rd == 0x00) && (rs1 == 0x00))//ecall ebreak mret
                        {
                            switch(funct7)
                            {
                                case 0x00://ecall ebreak
                                    switch(rs2)
                                    {
                                        case 0x00://ecall
                                            isa_state.op = op_t::ecall;
                                            isa_state.op_unit = op_unit_t::alu;
                                            isa_state.sub_op.alu_op = alu_op_t::ecall;
                                            isa_state.arg1_src = arg_src_t::disable;
                                            isa_state.arg2_src = arg_src_t::disable;
                                            break;
                                    
                                        case 0x01://ebreak
                                            isa_state.op = op_t::ebreak;
                                            isa_state.op_unit = op_unit_t::alu;
                                            isa_state.sub_op.alu_op = alu_op_t::ebreak;
                                            isa_state.arg1_src = arg_src_t::disable;
                                            isa_state.arg2_src = arg_src_t::disable;
                                            break;
                                    
                                        default://invalid
                                            isa_state.valid = false;
                                            break;
                                    }
                                
                                    break;
                            
                                case 0x18://mret
                                    switch(rs2)
                                    {
                                        case 0x02://mret
                                            isa_state.op = op_t::mret;
                                            isa_state.op_unit = op_unit_t::bru;
                                            isa_state.sub_op.bru_op = bru_op_t::mret;
                                            isa_state.arg1_src = arg_src_t::disable;
                                            isa_state.arg2_src = arg_src_t::disable;
                                            break;
                                    
                                        default://invalid
                                            isa_state.valid = false;
                                            break;
                                    }
                                
                                    break;
                            
                                default://invalid
                                    isa_state.valid = false;
                                    break;
                            }
                        }
                        else//invalid
                        {
                            isa_state.valid = false;
                        }
                    
                        break;
                
                    case 0x1://csrrw
                        isa_state.op = op_t::csrrw;
                        isa_state.op_unit = op_unit_t::csr;
                        isa_state.sub_op.csr_op = csr_op_t::csrrw;
                        isa_state.arg1_src = arg_src_t::reg;
                        isa_state.rs1 = rs1;
                        isa_state.arg2_src = arg_src_t::disable;
                        isa_state.csr = imm_i;
                        isa_state.rd = rd;
                        isa_state.rd_enable = true;
                        break;
                
                    case 0x2://csrrs
                        isa_state.op = op_t::csrrs;
                        isa_state.op_unit = op_unit_t::csr;
                        isa_state.sub_op.csr_op = csr_op_t::csrrs;
                        isa_state.arg1_src = arg_src_t::reg;
                        isa_state.rs1 = rs1;
                        isa_state.arg2_src = arg_src_t::disable;
                        isa_state.csr = imm_i;
                        isa_state.rd = rd;
                        isa_state.rd_enable = true;
                        break;
                
                    case 0x3://csrrc
                        isa_state.op = op_t::csrrc;
                        isa_state.op_unit = op_unit_t::csr;
                        isa_state.sub_op.csr_op = csr_op_t::csrrc;
                        isa_state.arg1_src = arg_src_t::reg;
                        isa_state.rs1 = rs1;
                        isa_state.arg2_src = arg_src_t::disable;
                        isa_state.csr = imm_i;
                        isa_state.rd = rd;
                        isa_state.rd_enable = true;
                        break;
                
                    case 0x5://csrrwi
                        isa_state.op = op_t::csrrwi;
                        isa_state.op_unit = op_unit_t::csr;
                        isa_state.sub_op.csr_op = csr_op_t::csrrw;
                        isa_state.arg1_src = arg_src_t::imm;
                        isa_state.imm = rs1;//zimm
                        isa_state.arg2_src = arg_src_t::disable;
                        isa_state.csr = imm_i;
                        isa_state.rd = rd;
                        isa_state.rd_enable = true;
                        break;
                
                    case 0x6://csrrsi
                        isa_state.op = op_t::csrrsi;
                        isa_state.op_unit = op_unit_t::csr;
                        isa_state.sub_op.csr_op = csr_op_t::csrrs;
                        isa_state.arg1_src = arg_src_t::imm;
                        isa_state.imm = rs1;//zimm
                        isa_state.arg2_src = arg_src_t::disable;
                        isa_state.csr = imm_i;
                        isa_state.rd = rd;
                        isa_state.rd_enable = true;
                        break;
                
                    case 0x7://csrrci
                        isa_state.op = op_t::csrrci;
                        isa_state.op_unit = op_unit_t::csr;
                        isa_state.sub_op.csr_op = csr_op_t::csrrc;
                        isa_state.arg1_src = arg_src_t::imm;
                        isa_state.imm = rs1;//zimm
                        isa_state.arg2_src = arg_src_t::disable;
                        isa_state.csr = imm_i;
                        isa_state.rd = rd;
                        isa_state.rd_enable = true;
                        break;
                
                    default://invalid
                        isa_state.valid = false;
                        break;
                }
                break;
        
            default://invalid
                isa_state.valid = false;
                break;
        }
    }
    
    void isa_model::execute(isa_state_t &isa_state)
    {
        if(isa_state.valid && !isa_state.has_exception)
        {
            switch(isa_state.arg1_src)
            {
                case arg_src_t::reg:
                    isa_state.src1_value = arch_regfile.read(isa_state.rs1);
                    break;
                    
                case arg_src_t::imm:
                    isa_state.src1_value = isa_state.imm;
                    break;
                    
                default:
                    break;
            }
    
            switch(isa_state.arg2_src)
            {
                case arg_src_t::reg:
                    isa_state.src2_value = arch_regfile.read(isa_state.rs2);
                    break;
        
                case arg_src_t::imm:
                    isa_state.src2_value = isa_state.imm;
                    break;
        
                default:
                    break;
            }
        }
        
        switch(isa_state.op_unit)
        {
            case op_unit_t::alu:
                execute_alu(isa_state);
                break;
                
            case op_unit_t::bru:
                execute_bru(isa_state);
                break;
                
            case op_unit_t::csr:
                execute_csr(isa_state);
                break;
                
            case op_unit_t::div:
                execute_div(isa_state);
                break;
                
            case op_unit_t::mul:
                execute_mul(isa_state);
                break;
                
            case op_unit_t::lsu:
                execute_lsu(isa_state);
                break;
                
            default:
                verify_only(0);
                break;
        }
        
        if(isa_state.has_exception)
        {
            csr_inst::mepc.write(isa_state.pc);
            csr_inst::mtval.write(isa_state.exception_value);
            csr_inst::mcause.write(static_cast<uint32_t>(isa_state.exception_id));
            this->pc = csr_inst::mtvec.read();
        }
        else if(isa_state.rd_enable && (isa_state.rd != 0))
        {
            arch_regfile.write(isa_state.rd, isa_state.rd_value);
        }
    }
    
    void isa_model::execute_alu(isa_state_t &isa_state)
    {
        if(!isa_state.has_exception)
        {
            if(isa_state.valid)
            {
                switch(isa_state.sub_op.alu_op)
                {
                    case alu_op_t::add:
                        isa_state.rd_value = isa_state.src1_value + isa_state.src2_value;
                        break;
            
                    case alu_op_t::_and:
                        isa_state.rd_value = isa_state.src1_value & isa_state.src2_value;
                        break;
            
                    case alu_op_t::auipc:
                        isa_state.rd_value = isa_state.imm + isa_state.pc;
                        break;
            
                    case alu_op_t::ebreak:
                        isa_state.rd_value = 0;
                        isa_state.has_exception = true;
                        isa_state.exception_id = riscv_exception_t::breakpoint;
                        isa_state.exception_value = 0;
                        break;
            
                    case alu_op_t::ecall:
                        isa_state.rd_value = 0;
                        isa_state.has_exception = true;
                        isa_state.exception_id = riscv_exception_t::environment_call_from_m_mode;
                        isa_state.exception_value = 0;
                        break;
            
                    case alu_op_t::fence:
                    case alu_op_t::fence_i:
                        isa_state.rd_value = 0;
                        break;
            
                    case alu_op_t::lui:
                        isa_state.rd_value = isa_state.imm;
                        break;
            
                    case alu_op_t::_or:
                        isa_state.rd_value = isa_state.src1_value | isa_state.src2_value;
                        break;
            
                    case alu_op_t::sll:
                        isa_state.rd_value = isa_state.src1_value << (isa_state.src2_value & 0x1f);
                        break;
            
                    case alu_op_t::slt:
                        isa_state.rd_value = (((int32_t)isa_state.src1_value) < ((int32_t)isa_state.src2_value)) ? 1 : 0;
                        break;
            
                    case alu_op_t::sltu:
                        isa_state.rd_value = (isa_state.src1_value < isa_state.src2_value) ? 1 : 0;
                        break;
            
                    case alu_op_t::sra:
                        isa_state.rd_value = (uint32_t)(((int32_t)isa_state.src1_value) >> (isa_state.src2_value & 0x1f));
                        break;
            
                    case alu_op_t::srl:
                        isa_state.rd_value = isa_state.src1_value >> (isa_state.src2_value & 0x1f);
                        break;
            
                    case alu_op_t::sub:
                        isa_state.rd_value = isa_state.src1_value - isa_state.src2_value;
                        break;
            
                    case alu_op_t::_xor:
                        isa_state.rd_value = isa_state.src1_value ^ isa_state.src2_value;
                        break;
                }
            }
            else
            {
                isa_state.has_exception = true;
                isa_state.exception_id = riscv_exception_t::illegal_instruction;
                isa_state.exception_value = 0;
            }
        }
    }
    
    void isa_model::execute_bru(isa_state_t &isa_state)
    {
        verify_only(isa_state.valid && !isa_state.has_exception);
        bool bru_jump = false;
        uint32_t bru_next_pc = isa_state.pc + isa_state.imm;
    
        switch(isa_state.sub_op.bru_op)
        {
            case bru_op_t::beq:
                bru_jump = isa_state.src1_value == isa_state.src2_value;
                break;
        
            case bru_op_t::bge:
                bru_jump = ((int32_t)isa_state.src1_value) >= ((int32_t)isa_state.src2_value);
                break;
        
            case bru_op_t::bgeu:
                bru_jump = ((uint32_t)isa_state.src1_value) >= ((uint32_t)isa_state.src2_value);
                break;
        
            case bru_op_t::blt:
                bru_jump = ((int32_t)isa_state.src1_value) < ((int32_t)isa_state.src2_value);
                break;
        
            case bru_op_t::bltu:
                bru_jump = ((uint32_t)isa_state.src1_value) < ((uint32_t)isa_state.src2_value);
                break;
        
            case bru_op_t::bne:
                bru_jump = isa_state.src1_value != isa_state.src2_value;
                break;
        
            case bru_op_t::jal:
                isa_state.rd_value = isa_state.pc + 4;
                bru_jump = true;
                break;
        
            case bru_op_t::jalr:
                isa_state.rd_value = isa_state.pc + 4;
                bru_jump = true;
                bru_next_pc = (isa_state.imm + isa_state.src1_value) & (~0x01);
                break;
        
            case bru_op_t::mret:
                bru_jump = true;
                bru_next_pc = csr_inst::mepc.read();
                component::csr::mstatus mstatus;
                mstatus.load(csr_inst::mstatus.read());
                mstatus.set_mie(mstatus.get_mpie());
                csr_inst::mstatus.write(mstatus.get_value());
                break;
        }
        
        if(bru_jump)
        {
            this->pc = bru_next_pc;
        }
    }
    
    void isa_model::execute_csr(isa_state_t &isa_state)
    {
        verify_only(isa_state.valid && !isa_state.has_exception);
        uint32_t csr_value = 0;
        auto read_ret = csr_file.read(isa_state.csr, &csr_value);
        
        if(isa_state.rd_enable && isa_state.rd != 0 && !read_ret)
        {
            isa_state.has_exception = true;
            isa_state.exception_id = riscv_exception_t::illegal_instruction;
            isa_state.exception_value = isa_state.value;
        }
        else
        {
            isa_state.rd_value = csr_value;
#ifdef NEED_ISA_MODEL
            breakpoint_csr_trigger(isa_state.csr, csr_value, false);
#endif
            
            if(!(isa_state.arg1_src == arg_src_t::reg && (isa_state.rs1 == 0)))
            {
                switch(isa_state.sub_op.csr_op)
                {
                    case csr_op_t::csrrc:
                        csr_value = csr_value & ~(isa_state.src1_value);
                        break;
        
                    case csr_op_t::csrrs:
                        csr_value = csr_value | isa_state.src1_value;
                        break;
        
                    case csr_op_t::csrrw:
                        csr_value = isa_state.src1_value;
                        break;
                }
    
                if(!csr_file.write(isa_state.csr, csr_value))
                {
                    isa_state.has_exception = true;
                    isa_state.exception_id = riscv_exception_t::illegal_instruction;
                    isa_state.exception_value = isa_state.value;
                }
                else
                {
#ifdef NEED_ISA_MODEL
                    breakpoint_csr_trigger(isa_state.csr, csr_value, true);
#endif
                }
            }
        }
    }
    
    void isa_model::execute_div(isa_state_t &isa_state)
    {
        verify_only(isa_state.valid && !isa_state.has_exception);
        auto overflow = (isa_state.src1_value == 0x80000000) && (isa_state.src2_value == 0xFFFFFFFF);
    
        switch(isa_state.sub_op.div_op)
        {
            case div_op_t::div:
                isa_state.rd_value = (isa_state.src2_value == 0) ? 0xFFFFFFFF : overflow ? 0x80000000 : ((uint32_t)(((int32_t)isa_state.src1_value) / ((int32_t)isa_state.src2_value)));
                break;
        
            case div_op_t::divu:
                isa_state.rd_value = (isa_state.src2_value == 0) ? 0xFFFFFFFF : ((uint32_t)(((uint32_t)isa_state.src1_value) / ((uint32_t)isa_state.src2_value)));
                break;
        
            case div_op_t::rem:
                isa_state.rd_value = (isa_state.src2_value == 0) ? isa_state.src1_value : overflow ? 0 : ((uint32_t)(((int32_t)isa_state.src1_value) % ((int32_t)isa_state.src2_value)));
                break;
        
            case div_op_t::remu:
                isa_state.rd_value = (isa_state.src2_value == 0) ? isa_state.src1_value : (((uint32_t)((int32_t)isa_state.src1_value) % ((int32_t)isa_state.src2_value)));
                break;
        }
    }
    
    void isa_model::execute_mul(isa_state_t &isa_state)
    {
        verify_only(isa_state.valid && !isa_state.has_exception);
    
        switch(isa_state.sub_op.mul_op)
        {
            case mul_op_t::mul:
                isa_state.rd_value = (uint32_t)(((uint64_t)(((int64_t)(int32_t)isa_state.src1_value) * ((int64_t)(int32_t)isa_state.src2_value))) & 0xffffffffull);
                break;
        
            case mul_op_t::mulh:
                isa_state.rd_value = (uint32_t)((((uint64_t)(((int64_t)(int32_t)isa_state.src1_value) * ((int64_t)(int32_t)isa_state.src2_value))) >> 32) & 0xffffffffull);
                break;
        
            case mul_op_t::mulhsu:
                isa_state.rd_value = (uint32_t)((((uint64_t)(((int64_t)(int32_t)isa_state.src1_value) * ((uint64_t)isa_state.src2_value))) >> 32) & 0xffffffffull);
                break;
        
            case mul_op_t::mulhu:
                isa_state.rd_value = (uint32_t)((((uint64_t)(((uint64_t)isa_state.src1_value) * ((uint64_t)isa_state.src2_value))) >> 32) & 0xffffffffull);
                break;
        }
    }
    
    void isa_model::execute_lsu(isa_state_t &isa_state)
    {
        verify_only(isa_state.valid && !isa_state.has_exception);
        auto addr = isa_state.src1_value + isa_state.imm;
    
        switch(isa_state.sub_op.lsu_op)
        {
            case lsu_op_t::lb:
                isa_state.rd_value = sign_extend(bus.read8(addr, false), 8);
                break;
        
            case lsu_op_t::lbu:
                isa_state.rd_value = bus.read8(addr, false);
                break;
        
            case lsu_op_t::lh:
                isa_state.rd_value = sign_extend(bus.read16(addr, false), 16);
                break;
        
            case lsu_op_t::lhu:
                isa_state.rd_value = bus.read16(addr, false);
                break;
        
            case lsu_op_t::lw:
                isa_state.rd_value = bus.read32(addr, false);
                break;
        
            case lsu_op_t::sb:
                bus.write8(addr, isa_state.src2_value & 0xff);
                break;
        
            case lsu_op_t::sh:
                bus.write16(addr, isa_state.src2_value & 0xffff);
                break;
        
            case lsu_op_t::sw:
                bus.write32(addr, isa_state.src2_value);
                break;
        }
    }
}