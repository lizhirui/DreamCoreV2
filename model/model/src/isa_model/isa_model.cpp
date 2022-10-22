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
    
    void isa_model::run()
    {
        riscv_interrupt_t interrupt_id;
        
        auto fetch_decode_pack = fetch();
        auto decode_execute_pack = decode(fetch_decode_pack);
        execute(decode_execute_pack);
        interrupt_interface.run();
        clint.run();
    
        if(interrupt_interface.get_cause(&interrupt_id))
        {
            csr_file.write_sys(CSR_MEPC, this->pc);
            csr_file.write_sys(CSR_MTVAL, 0);
            csr_file.write_sys(CSR_MCAUSE, 0x80000000 | static_cast<uint32_t>(interrupt_id));
            component::csr::mstatus mstatus;
            mstatus.load(csr_file.read_sys(CSR_MSTATUS));
            mstatus.set_mpie(mstatus.get_mie());
            mstatus.set_mie(false);
            csr_file.write_sys(CSR_MSTATUS, mstatus.get_value());
            this->pc = csr_file.read_sys(CSR_MTVEC);
        }
        
        cpu_clock_cycle++;
        committed_instruction_num++;
        commit_num++;//only for debugger
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
        fetch_decode_pack_t send_pack;
        send_pack.pc = this->pc;
        send_pack.value  = 0;
        send_pack.has_exception = false;
        send_pack.exception_id = riscv_exception_t::instruction_access_fault;
        send_pack.exception_value = 0;
        
        if(!component::bus::check_align(this->pc, 4))
        {
            send_pack.has_exception = true;
            send_pack.exception_id = riscv_exception_t::instruction_address_misaligned;
            send_pack.exception_value = this->pc;
        }
        
        send_pack.value = bus.read32(this->pc, true);
        this->pc += 4;
        return send_pack;
    }
    
    decode_execute_pack_t isa_model::decode(const fetch_decode_pack_t &fetch_decode_pack)
    {
        decode_execute_pack_t send_pack;
        
        auto op_data = fetch_decode_pack.value;
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
        
        send_pack.value = 0;
        send_pack.valid = !fetch_decode_pack.has_exception;
        send_pack.pc = fetch_decode_pack.pc;
        send_pack.imm = 0;
        send_pack.has_exception = false;
        send_pack.exception_id = riscv_exception_t::instruction_address_misaligned;
        send_pack.exception_value = 0;
        send_pack.rs1 = 0;
        send_pack.arg1_src = arg_src_t::reg;
        send_pack.rs2 = 0;
        send_pack.rd = 0;
        send_pack.rd_enable = false;
        send_pack.csr = 0;
        send_pack.op = op_t::add;
        send_pack.op_unit = op_unit_t::alu;
        send_pack.sub_op.alu_op = alu_op_t::add;
    
        switch(opcode)
        {
            case 0x37://lui
                send_pack.op = op_t::lui;
                send_pack.op_unit = op_unit_t::alu;
                send_pack.sub_op.alu_op = alu_op_t::lui;
                send_pack.arg1_src = arg_src_t::disable;
                send_pack.arg2_src = arg_src_t::disable;
                send_pack.imm = imm_u;
                send_pack.rd = rd;
                send_pack.rd_enable = true;
                break;
        
            case 0x17://auipc
                send_pack.op = op_t::auipc;
                send_pack.op_unit = op_unit_t::alu;
                send_pack.sub_op.alu_op = alu_op_t::auipc;
                send_pack.arg1_src = arg_src_t::disable;
                send_pack.arg2_src = arg_src_t::disable;
                send_pack.imm = imm_u;
                send_pack.rd = rd;
                send_pack.rd_enable = true;
                break;
        
            case 0x6f://jal
                send_pack.op = op_t::jal;
                send_pack.op_unit = op_unit_t::bru;
                send_pack.sub_op.bru_op = bru_op_t::jal;
                send_pack.arg1_src = arg_src_t::disable;
                send_pack.arg2_src = arg_src_t::disable;
                send_pack.imm = sign_extend(imm_j, 21);
                send_pack.rd = rd;
                send_pack.rd_enable = true;
                break;
        
            case 0x67://jalr
                send_pack.op = op_t::jalr;
                send_pack.op_unit = op_unit_t::bru;
                send_pack.sub_op.bru_op = bru_op_t::jalr;
                send_pack.arg1_src = arg_src_t::reg;
                send_pack.rs1 = rs1;
                send_pack.arg2_src = arg_src_t::disable;
                send_pack.imm = sign_extend(imm_i, 12);
                send_pack.rd = rd;
                send_pack.rd_enable = true;
                break;
        
            case 0x63://beq bne blt bge bltu bgeu
                send_pack.op_unit = op_unit_t::bru;
                send_pack.arg1_src = arg_src_t::reg;
                send_pack.rs1 = rs1;
                send_pack.arg2_src = arg_src_t::reg;
                send_pack.rs2 = rs2;
                send_pack.imm = sign_extend(imm_b, 13);
            
                switch(funct3)
                {
                    case 0x0://beq
                        send_pack.op = op_t::beq;
                        send_pack.sub_op.bru_op = bru_op_t::beq;
                        break;
                
                    case 0x1://bne
                        send_pack.op = op_t::bne;
                        send_pack.sub_op.bru_op = bru_op_t::bne;
                        break;
                
                    case 0x4://blt
                        send_pack.op = op_t::blt;
                        send_pack.sub_op.bru_op = bru_op_t::blt;
                        break;
                
                    case 0x5://bge
                        send_pack.op = op_t::bge;
                        send_pack.sub_op.bru_op = bru_op_t::bge;
                        break;
                
                    case 0x6://bltu
                        send_pack.op = op_t::bltu;
                        send_pack.sub_op.bru_op = bru_op_t::bltu;
                        break;
                
                    case 0x7://bgeu
                        send_pack.op = op_t::bgeu;
                        send_pack.sub_op.bru_op = bru_op_t::bgeu;
                        break;
                
                    default://invalid
                        send_pack.valid = false;
                        break;
                }
            
                break;
        
            case 0x03://lb lh lw lbu lhu
                send_pack.op_unit = op_unit_t::lsu;
                send_pack.arg1_src = arg_src_t::reg;
                send_pack.rs1 = rs1;
                send_pack.arg2_src = arg_src_t::disable;
                send_pack.imm = sign_extend(imm_i, 12);
                send_pack.rd = rd;
                send_pack.rd_enable = true;
            
                switch(funct3)
                {
                    case 0x0://lb
                        send_pack.op = op_t::lb;
                        send_pack.sub_op.lsu_op = lsu_op_t::lb;
                        break;
                
                    case 0x1://lh
                        send_pack.op = op_t::lh;
                        send_pack.sub_op.lsu_op = lsu_op_t::lh;
                        break;
                
                    case 0x2://lw
                        send_pack.op = op_t::lw;
                        send_pack.sub_op.lsu_op = lsu_op_t::lw;
                        break;
                
                    case 0x4://lbu
                        send_pack.op = op_t::lbu;
                        send_pack.sub_op.lsu_op = lsu_op_t::lbu;
                        break;
                
                    case 0x5://lhu
                        send_pack.op = op_t::lhu;
                        send_pack.sub_op.lsu_op = lsu_op_t::lhu;
                        break;
                
                    default://invalid
                        send_pack.valid = false;
                        break;
                }
            
                break;
        
            case 0x23://sb sh sw
                send_pack.op_unit = op_unit_t::lsu;
                send_pack.arg1_src = arg_src_t::reg;
                send_pack.rs1 = rs1;
                send_pack.arg2_src = arg_src_t::reg;
                send_pack.rs2 = rs2;
                send_pack.imm = sign_extend(imm_s, 12);
            
                switch(funct3)
                {
                    case 0x0://sb
                        send_pack.op = op_t::sb;
                        send_pack.sub_op.lsu_op = lsu_op_t::sb;
                        break;
                
                    case 0x1://sh
                        send_pack.op = op_t::sh;
                        send_pack.sub_op.lsu_op = lsu_op_t::sh;
                        break;
                
                    case 0x2://sw
                        send_pack.op = op_t::sw;
                        send_pack.sub_op.lsu_op = lsu_op_t::sw;
                        break;
                
                    default://invalid
                        send_pack.valid = false;
                        break;
                }
            
                break;
        
            case 0x13://addi slti sltiu xori ori andi slli srli srai
                send_pack.op_unit = op_unit_t::alu;
                send_pack.arg1_src = arg_src_t::reg;
                send_pack.rs1 = rs1;
                send_pack.arg2_src = arg_src_t::imm;
                send_pack.imm = sign_extend(imm_i, 12);
                send_pack.rd = rd;
                send_pack.rd_enable = true;
            
                switch(funct3)
                {
                    case 0x0://addi
                        send_pack.op = op_t::addi;
                        send_pack.sub_op.alu_op = alu_op_t::add;
                        break;
                
                    case 0x2://slti
                        send_pack.op = op_t::slti;
                        send_pack.sub_op.alu_op = alu_op_t::slt;
                        break;
                
                    case 0x3://sltiu
                        send_pack.op = op_t::sltiu;
                        send_pack.sub_op.alu_op = alu_op_t::sltu;
                        break;
                
                    case 0x4://xori
                        send_pack.op = op_t::xori;
                        send_pack.sub_op.alu_op = alu_op_t::_xor;
                        break;
                
                    case 0x6://ori
                        send_pack.op = op_t::ori;
                        send_pack.sub_op.alu_op = alu_op_t::_or;
                        break;
                
                    case 0x7://andi
                        send_pack.op = op_t::andi;
                        send_pack.sub_op.alu_op = alu_op_t::_and;
                        break;
                
                    case 0x1://slli
                        if(funct7 == 0x00)//slli
                        {
                            send_pack.op = op_t::slli;
                            send_pack.sub_op.alu_op = alu_op_t::sll;
                        }
                        else//invalid
                        {
                            send_pack.valid = false;
                        }
                    
                        break;
                
                    case 0x5://srli srai
                        if(funct7 == 0x00)//srli
                        {
                            send_pack.op = op_t::srli;
                            send_pack.sub_op.alu_op = alu_op_t::srl;
                        }
                        else if(funct7 == 0x20)//srai
                        {
                            send_pack.op = op_t::srai;
                            send_pack.sub_op.alu_op = alu_op_t::sra;
                        }
                        else//invalid
                        {
                            send_pack.valid = false;
                        }
                    
                        break;
                
                    default://invalid
                        send_pack.valid = false;
                        break;
                }
            
                break;
        
            case 0x33://add sub sll slt sltu xor srl sra or and mul mulh mulhsu mulhu div divu rem remu
                send_pack.op_unit = op_unit_t::alu;
                send_pack.arg1_src = arg_src_t::reg;
                send_pack.rs1 = rs1;
                send_pack.arg2_src = arg_src_t::reg;
                send_pack.rs2 = rs2;
                send_pack.rd = rd;
                send_pack.rd_enable = true;
            
                switch(funct3)
                {
                    case 0x0://add sub mul
                        if(funct7 == 0x00)//add
                        {
                            send_pack.op = op_t::add;
                            send_pack.sub_op.alu_op = alu_op_t::add;
                        }
                        else if(funct7 == 0x20)//sub
                        {
                            send_pack.op = op_t::sub;
                            send_pack.sub_op.alu_op = alu_op_t::sub;
                        }
                        else if(funct7 == 0x01)//mul
                        {
                            send_pack.op = op_t::mul;
                            send_pack.op_unit = op_unit_t::mul;
                            send_pack.sub_op.mul_op = mul_op_t::mul;
                        }
                        else//invalid
                        {
                            send_pack.valid = false;
                        }
                    
                        break;
                
                    case 0x1://sll mulh
                        if(funct7 == 0x00)//sll
                        {
                            send_pack.op = op_t::sll;
                            send_pack.sub_op.alu_op = alu_op_t::sll;
                        }
                        else if(funct7 == 0x01)//mulh
                        {
                            send_pack.op = op_t::mulh;
                            send_pack.op_unit = op_unit_t::mul;
                            send_pack.sub_op.mul_op = mul_op_t::mulh;
                        }
                        else//invalid
                        {
                            send_pack.valid = false;
                        }
                    
                        break;
                
                    case 0x2://slt mulhsu
                        if(funct7 == 0x00)//slt
                        {
                            send_pack.op = op_t::slt;
                            send_pack.sub_op.alu_op = alu_op_t::slt;
                        }
                        else if(funct7 == 0x01)//mulhsu
                        {
                            send_pack.op = op_t::mulhsu;
                            send_pack.op_unit = op_unit_t::mul;
                            send_pack.sub_op.mul_op = mul_op_t::mulhsu;
                        }
                        else//invalid
                        {
                            send_pack.valid = false;
                        }
                    
                        break;
                
                    case 0x3://sltu mulhu
                        if(funct7 == 0x00)//sltu
                        {
                            send_pack.op = op_t::sltu;
                            send_pack.sub_op.alu_op = alu_op_t::sltu;
                        }
                        else if(funct7 == 0x01)//mulhu
                        {
                            send_pack.op = op_t::mulhu;
                            send_pack.op_unit = op_unit_t::mul;
                            send_pack.sub_op.mul_op = mul_op_t::mulhu;
                        }
                        else//invalid
                        {
                            send_pack.valid = false;
                        }
                    
                        break;
                
                    case 0x4://xor div
                        if(funct7 == 0x00)//xor
                        {
                            send_pack.op = op_t::_xor;
                            send_pack.sub_op.alu_op = alu_op_t::_xor;
                        }
                        else if(funct7 == 0x01)//div
                        {
                            send_pack.op = op_t::div;
                            send_pack.op_unit = op_unit_t::div;
                            send_pack.sub_op.div_op = div_op_t::div;
                        }
                        else//invalid
                        {
                            send_pack.valid = false;
                        }
                    
                        break;
                
                    case 0x5://srl sra divu
                        if(funct7 == 0x00)//srl
                        {
                            send_pack.op = op_t::srl;
                            send_pack.sub_op.alu_op = alu_op_t::srl;
                        }
                        else if(funct7 == 0x20)//sra
                        {
                            send_pack.op = op_t::sra;
                            send_pack.sub_op.alu_op = alu_op_t::sra;
                        }
                        else if(funct7 == 0x01)//divu
                        {
                            send_pack.op = op_t::divu;
                            send_pack.op_unit = op_unit_t::div;
                            send_pack.sub_op.div_op = div_op_t::divu;
                        }
                        else//invalid
                        {
                            send_pack.valid = false;
                        }
                    
                        break;
                
                    case 0x6://or rem
                        if(funct7 == 0x00)//or
                        {
                            send_pack.op = op_t::_or;
                            send_pack.sub_op.alu_op = alu_op_t::_or;
                        }
                        else if(funct7 == 0x01)//rem
                        {
                            send_pack.op = op_t::rem;
                            send_pack.op_unit = op_unit_t::div;
                            send_pack.sub_op.div_op = div_op_t::rem;
                        }
                        else//invalid
                        {
                            send_pack.valid = false;
                        }
                    
                        break;
                
                    case 0x7://and remu
                        if(funct7 == 0x00)//and
                        {
                            send_pack.op = op_t::_and;
                            send_pack.sub_op.alu_op = alu_op_t::_and;
                        }
                        else if(funct7 == 0x01)//remu
                        {
                            send_pack.op = op_t::remu;
                            send_pack.op_unit = op_unit_t::div;
                            send_pack.sub_op.div_op = div_op_t::remu;
                        }
                        else//invalid
                        {
                            send_pack.valid = false;
                        }
                    
                        break;
                
                    default://invalid
                        send_pack.valid = false;
                        break;
                }
            
                break;
        
            case 0x0f://fence fence.i
                switch(funct3)
                {
                    case 0x0://fence
                        if((rd == 0x00) && (rs1 == 0x00) && (((op_data >> 28) & 0x0f) == 0x00))//fence
                        {
                            send_pack.op = op_t::fence;
                            send_pack.op_unit = op_unit_t::alu;
                            send_pack.sub_op.alu_op = alu_op_t::fence;
                            send_pack.arg1_src = arg_src_t::disable;
                            send_pack.arg2_src = arg_src_t::disable;
                            send_pack.imm = imm_i;
                        }
                        else//invalid
                        {
                            send_pack.valid = false;
                        }
                    
                        break;
                
                    case 0x1://fence.i
                        if((rd == 0x00) && (rs1 == 0x00) && (imm_i == 0x00))//fence.i
                        {
                            send_pack.op = op_t::fence_i;
                            send_pack.op_unit = op_unit_t::alu;
                            send_pack.sub_op.alu_op = alu_op_t::fence_i;
                            send_pack.arg1_src = arg_src_t::disable;
                            send_pack.arg2_src = arg_src_t::disable;
                        }
                        else//invalid
                        {
                            send_pack.valid = false;
                        }
                    
                        break;
                
                    default://invalid
                        send_pack.valid = false;
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
                                            send_pack.op = op_t::ecall;
                                            send_pack.op_unit = op_unit_t::alu;
                                            send_pack.sub_op.alu_op = alu_op_t::ecall;
                                            send_pack.arg1_src = arg_src_t::disable;
                                            send_pack.arg2_src = arg_src_t::disable;
                                            break;
                                    
                                        case 0x01://ebreak
                                            send_pack.op = op_t::ebreak;
                                            send_pack.op_unit = op_unit_t::alu;
                                            send_pack.sub_op.alu_op = alu_op_t::ebreak;
                                            send_pack.arg1_src = arg_src_t::disable;
                                            send_pack.arg2_src = arg_src_t::disable;
                                            break;
                                    
                                        default://invalid
                                            send_pack.valid = false;
                                            break;
                                    }
                                
                                    break;
                            
                                case 0x18://mret
                                    switch(rs2)
                                    {
                                        case 0x02://mret
                                            send_pack.op = op_t::mret;
                                            send_pack.op_unit = op_unit_t::bru;
                                            send_pack.sub_op.bru_op = bru_op_t::mret;
                                            send_pack.arg1_src = arg_src_t::disable;
                                            send_pack.arg2_src = arg_src_t::disable;
                                            break;
                                    
                                        default://invalid
                                            send_pack.valid = false;
                                            break;
                                    }
                                
                                    break;
                            
                                default://invalid
                                    send_pack.valid = false;
                                    break;
                            }
                        }
                        else//invalid
                        {
                            send_pack.valid = false;
                        }
                    
                        break;
                
                    case 0x1://csrrw
                        send_pack.op = op_t::csrrw;
                        send_pack.op_unit = op_unit_t::csr;
                        send_pack.sub_op.csr_op = csr_op_t::csrrw;
                        send_pack.arg1_src = arg_src_t::reg;
                        send_pack.rs1 = rs1;
                        send_pack.arg2_src = arg_src_t::disable;
                        send_pack.csr = imm_i;
                        send_pack.rd = rd;
                        send_pack.rd_enable = true;
                        break;
                
                    case 0x2://csrrs
                        send_pack.op = op_t::csrrs;
                        send_pack.op_unit = op_unit_t::csr;
                        send_pack.sub_op.csr_op = csr_op_t::csrrs;
                        send_pack.arg1_src = arg_src_t::reg;
                        send_pack.rs1 = rs1;
                        send_pack.arg2_src = arg_src_t::disable;
                        send_pack.csr = imm_i;
                        send_pack.rd = rd;
                        send_pack.rd_enable = true;
                        break;
                
                    case 0x3://csrrc
                        send_pack.op = op_t::csrrc;
                        send_pack.op_unit = op_unit_t::csr;
                        send_pack.sub_op.csr_op = csr_op_t::csrrc;
                        send_pack.arg1_src = arg_src_t::reg;
                        send_pack.rs1 = rs1;
                        send_pack.arg2_src = arg_src_t::disable;
                        send_pack.csr = imm_i;
                        send_pack.rd = rd;
                        send_pack.rd_enable = true;
                        break;
                
                    case 0x5://csrrwi
                        send_pack.op = op_t::csrrwi;
                        send_pack.op_unit = op_unit_t::csr;
                        send_pack.sub_op.csr_op = csr_op_t::csrrw;
                        send_pack.arg1_src = arg_src_t::imm;
                        send_pack.imm = rs1;//zimm
                        send_pack.arg2_src = arg_src_t::disable;
                        send_pack.csr = imm_i;
                        send_pack.rd = rd;
                        send_pack.rd_enable = true;
                        break;
                
                    case 0x6://csrrsi
                        send_pack.op = op_t::csrrsi;
                        send_pack.op_unit = op_unit_t::csr;
                        send_pack.sub_op.csr_op = csr_op_t::csrrs;
                        send_pack.arg1_src = arg_src_t::imm;
                        send_pack.imm = rs1;//zimm
                        send_pack.arg2_src = arg_src_t::disable;
                        send_pack.csr = imm_i;
                        send_pack.rd = rd;
                        send_pack.rd_enable = true;
                        break;
                
                    case 0x7://csrrci
                        send_pack.op = op_t::csrrci;
                        send_pack.op_unit = op_unit_t::csr;
                        send_pack.sub_op.csr_op = csr_op_t::csrrc;
                        send_pack.arg1_src = arg_src_t::imm;
                        send_pack.imm = rs1;//zimm
                        send_pack.arg2_src = arg_src_t::disable;
                        send_pack.csr = imm_i;
                        send_pack.rd = rd;
                        send_pack.rd_enable = true;
                        break;
                
                    default://invalid
                        send_pack.valid = false;
                        break;
                }
                break;
        
            default://invalid
                send_pack.valid = false;
                break;
        }
        
        send_pack.value = fetch_decode_pack.value;
        send_pack.has_exception = fetch_decode_pack.has_exception;
        send_pack.exception_id = fetch_decode_pack.exception_id;
        send_pack.exception_value = fetch_decode_pack.exception_value;
        return send_pack;
    }
    
    void isa_model::execute(decode_execute_pack_t &decode_execute_pack)
    {
        if(decode_execute_pack.valid && !decode_execute_pack.has_exception)
        {
            switch(decode_execute_pack.arg1_src)
            {
                case arg_src_t::reg:
                    decode_execute_pack.src1_value = arch_regfile.read(decode_execute_pack.rs1);
                    break;
                    
                case arg_src_t::imm:
                    decode_execute_pack.src1_value = decode_execute_pack.imm;
                    break;
                    
                default:
                    break;
            }
    
            switch(decode_execute_pack.arg2_src)
            {
                case arg_src_t::reg:
                    decode_execute_pack.src2_value = arch_regfile.read(decode_execute_pack.rs2);
                    break;
        
                case arg_src_t::imm:
                    decode_execute_pack.src2_value = decode_execute_pack.imm;
                    break;
        
                default:
                    break;
            }
        }
        
        switch(decode_execute_pack.op_unit)
        {
            case op_unit_t::alu:
                execute_alu(decode_execute_pack);
                break;
                
            case op_unit_t::bru:
                execute_bru(decode_execute_pack);
                break;
                
            case op_unit_t::csr:
                execute_csr(decode_execute_pack);
                break;
                
            case op_unit_t::div:
                execute_div(decode_execute_pack);
                break;
                
            case op_unit_t::mul:
                execute_mul(decode_execute_pack);
                break;
                
            case op_unit_t::lsu:
                execute_lsu(decode_execute_pack);
                break;
                
            default:
                verify(0);
                break;
        }
        
        if(decode_execute_pack.has_exception)
        {
            csr_file.write_sys(CSR_MEPC, decode_execute_pack.pc);
            csr_file.write_sys(CSR_MTVAL, decode_execute_pack.exception_value);
            csr_file.write_sys(CSR_MCAUSE, static_cast<uint32_t>(decode_execute_pack.exception_id));
            this->pc = csr_file.read_sys(CSR_MTVEC);
        }
        else if(decode_execute_pack.rd_enable && (decode_execute_pack.rd != 0))
        {
            arch_regfile.write(decode_execute_pack.rd, decode_execute_pack.rd_value);
        }
    }
    
    void isa_model::execute_alu(decode_execute_pack_t &decode_execute_pack)
    {
        if(decode_execute_pack.valid && !decode_execute_pack.has_exception)
        {
            switch(decode_execute_pack.sub_op.alu_op)
            {
                case alu_op_t::add:
                    decode_execute_pack.rd_value = decode_execute_pack.src1_value + decode_execute_pack.src2_value;
                    break;
        
                case alu_op_t::_and:
                    decode_execute_pack.rd_value = decode_execute_pack.src1_value & decode_execute_pack.src2_value;
                    break;
        
                case alu_op_t::auipc:
                    decode_execute_pack.rd_value = decode_execute_pack.imm + decode_execute_pack.pc;
                    break;
        
                case alu_op_t::ebreak:
                    decode_execute_pack.rd_value = 0;
                    decode_execute_pack.has_exception = true;
                    decode_execute_pack.exception_id = riscv_exception_t::breakpoint;
                    decode_execute_pack.exception_value = 0;
                    break;
        
                case alu_op_t::ecall:
                    decode_execute_pack.rd_value = 0;
                    decode_execute_pack.has_exception = true;
                    decode_execute_pack.exception_id = riscv_exception_t::environment_call_from_m_mode;
                    decode_execute_pack.exception_value = 0;
                    break;
        
                case alu_op_t::fence:
                case alu_op_t::fence_i:
                    decode_execute_pack.rd_value = 0;
                    break;
        
                case alu_op_t::lui:
                    decode_execute_pack.rd_value = decode_execute_pack.imm;
                    break;
        
                case alu_op_t::_or:
                    decode_execute_pack.rd_value = decode_execute_pack.src1_value | decode_execute_pack.src2_value;
                    break;
        
                case alu_op_t::sll:
                    decode_execute_pack.rd_value = decode_execute_pack.src1_value << (decode_execute_pack.src2_value & 0x1f);
                    break;
        
                case alu_op_t::slt:
                    decode_execute_pack.rd_value = (((int32_t)decode_execute_pack.src1_value) < ((int32_t)decode_execute_pack.src2_value)) ? 1 : 0;
                    break;
        
                case alu_op_t::sltu:
                    decode_execute_pack.rd_value = (decode_execute_pack.src1_value < decode_execute_pack.src2_value) ? 1 : 0;
                    break;
        
                case alu_op_t::sra:
                    decode_execute_pack.rd_value = (uint32_t)(((int32_t)decode_execute_pack.src1_value) >> (decode_execute_pack.src2_value & 0x1f));
                    break;
        
                case alu_op_t::srl:
                    decode_execute_pack.rd_value = decode_execute_pack.src1_value >> (decode_execute_pack.src2_value & 0x1f);
                    break;
        
                case alu_op_t::sub:
                    decode_execute_pack.rd_value = decode_execute_pack.src1_value - decode_execute_pack.src2_value;
                    break;
        
                case alu_op_t::_xor:
                    decode_execute_pack.rd_value = decode_execute_pack.src1_value ^ decode_execute_pack.src2_value;
                    break;
            }
        }
    }
    
    void isa_model::execute_bru(decode_execute_pack_t &decode_execute_pack)
    {
        verify(decode_execute_pack.valid && !decode_execute_pack.has_exception);
        bool bru_jump = false;
        uint32_t bru_next_pc = decode_execute_pack.pc + decode_execute_pack.imm;
    
        switch(decode_execute_pack.sub_op.bru_op)
        {
            case bru_op_t::beq:
                bru_jump = decode_execute_pack.src1_value == decode_execute_pack.src2_value;
                break;
        
            case bru_op_t::bge:
                bru_jump = ((int32_t)decode_execute_pack.src1_value) >= ((int32_t)decode_execute_pack.src2_value);
                break;
        
            case bru_op_t::bgeu:
                bru_jump = ((uint32_t)decode_execute_pack.src1_value) >= ((uint32_t)decode_execute_pack.src2_value);
                break;
        
            case bru_op_t::blt:
                bru_jump = ((int32_t)decode_execute_pack.src1_value) < ((int32_t)decode_execute_pack.src2_value);
                break;
        
            case bru_op_t::bltu:
                bru_jump = ((uint32_t)decode_execute_pack.src1_value) < ((uint32_t)decode_execute_pack.src2_value);
                break;
        
            case bru_op_t::bne:
                bru_jump = decode_execute_pack.src1_value != decode_execute_pack.src2_value;
                break;
        
            case bru_op_t::jal:
                decode_execute_pack.rd_value = decode_execute_pack.pc + 4;
                bru_jump = true;
                break;
        
            case bru_op_t::jalr:
                decode_execute_pack.rd_value = decode_execute_pack.pc + 4;
                bru_jump = true;
                bru_next_pc = (decode_execute_pack.imm + decode_execute_pack.src1_value) & (~0x01);
                break;
        
            case bru_op_t::mret:
                bru_jump = true;
                bru_next_pc = csr_file.read_sys(CSR_MEPC);
                component::csr::mstatus mstatus;
                mstatus.load(csr_file.read_sys(CSR_MSTATUS));
                mstatus.set_mie(mstatus.get_mpie());
                csr_file.write_sys(CSR_MSTATUS, mstatus.get_value());
                break;
        }
        
        if(bru_jump)
        {
            this->pc = bru_next_pc;
        }
    }
    
    void isa_model::execute_csr(decode_execute_pack_t &decode_execute_pack)
    {
        verify(decode_execute_pack.valid && !decode_execute_pack.has_exception);
        uint32_t csr_value = 0;
        
        if(!csr_file.read(decode_execute_pack.csr, &csr_value))
        {
            decode_execute_pack.has_exception = true;
            decode_execute_pack.exception_id = riscv_exception_t::illegal_instruction;
            decode_execute_pack.exception_value = decode_execute_pack.value;
        }
        else
        {
            decode_execute_pack.rd_value = csr_value;
#ifdef NEED_ISA_MODEL
            breakpoint_csr_trigger(decode_execute_pack.csr, csr_value, false);
#endif
            
            if(!(decode_execute_pack.arg1_src == arg_src_t::reg && (decode_execute_pack.rs1 == 0)))
            {
                switch(decode_execute_pack.sub_op.csr_op)
                {
                    case csr_op_t::csrrc:
                        csr_value = csr_value & ~(decode_execute_pack.src1_value);
                        break;
        
                    case csr_op_t::csrrs:
                        csr_value = csr_value | decode_execute_pack.src1_value;
                        break;
        
                    case csr_op_t::csrrw:
                        csr_value = decode_execute_pack.src1_value;
                        break;
                }
    
                if(!csr_file.write(decode_execute_pack.csr, csr_value))
                {
                    decode_execute_pack.has_exception = true;
                    decode_execute_pack.exception_id = riscv_exception_t::illegal_instruction;
                    decode_execute_pack.exception_value = decode_execute_pack.value;
                }
                else
                {
#ifdef NEED_ISA_MODEL
                    breakpoint_csr_trigger(decode_execute_pack.csr, csr_value, true);
#endif
                }
            }
        }
    }
    
    void isa_model::execute_div(decode_execute_pack_t &decode_execute_pack)
    {
        verify(decode_execute_pack.valid && !decode_execute_pack.has_exception);
        auto overflow = (decode_execute_pack.src1_value == 0x80000000) && (decode_execute_pack.src2_value == 0xFFFFFFFF);
    
        switch(decode_execute_pack.sub_op.div_op)
        {
            case div_op_t::div:
                decode_execute_pack.rd_value = (decode_execute_pack.src2_value == 0) ? 0xFFFFFFFF : overflow ? 0x80000000 : ((uint32_t)(((int32_t)decode_execute_pack.src1_value) / ((int32_t)decode_execute_pack.src2_value)));
                break;
        
            case div_op_t::divu:
                decode_execute_pack.rd_value = (decode_execute_pack.src2_value == 0) ? 0xFFFFFFFF : ((uint32_t)(((uint32_t)decode_execute_pack.src1_value) / ((uint32_t)decode_execute_pack.src2_value)));
                break;
        
            case div_op_t::rem:
                decode_execute_pack.rd_value = (decode_execute_pack.src2_value == 0) ? decode_execute_pack.src1_value : overflow ? 0 : ((uint32_t)(((int32_t)decode_execute_pack.src1_value) % ((int32_t)decode_execute_pack.src2_value)));
                break;
        
            case div_op_t::remu:
                decode_execute_pack.rd_value = (decode_execute_pack.src2_value == 0) ? decode_execute_pack.src1_value : (((uint32_t)((int32_t)decode_execute_pack.src1_value) % ((int32_t)decode_execute_pack.src2_value)));
                break;
        }
    }
    
    void isa_model::execute_mul(decode_execute_pack_t &decode_execute_pack)
    {
        verify(decode_execute_pack.valid && !decode_execute_pack.has_exception);
    
        switch(decode_execute_pack.sub_op.mul_op)
        {
            case mul_op_t::mul:
                decode_execute_pack.rd_value = (uint32_t)(((uint64_t)(((int64_t)(int32_t)decode_execute_pack.src1_value) * ((int64_t)(int32_t)decode_execute_pack.src2_value))) & 0xffffffffull);
                break;
        
            case mul_op_t::mulh:
                decode_execute_pack.rd_value = (uint32_t)((((uint64_t)(((int64_t)(int32_t)decode_execute_pack.src1_value) * ((int64_t)(int32_t)decode_execute_pack.src2_value))) >> 32) & 0xffffffffull);
                break;
        
            case mul_op_t::mulhsu:
                decode_execute_pack.rd_value = (uint32_t)((((uint64_t)(((int64_t)(int32_t)decode_execute_pack.src1_value) * ((uint64_t)decode_execute_pack.src2_value))) >> 32) & 0xffffffffull);
                break;
        
            case mul_op_t::mulhu:
                decode_execute_pack.rd_value = (uint32_t)((((uint64_t)(((uint64_t)decode_execute_pack.src1_value) * ((uint64_t)decode_execute_pack.src2_value))) >> 32) & 0xffffffffull);
                break;
        }
    }
    
    void isa_model::execute_lsu(decode_execute_pack_t &decode_execute_pack)
    {
        verify(decode_execute_pack.valid && !decode_execute_pack.has_exception);
        auto addr = decode_execute_pack.src1_value + decode_execute_pack.imm;
    
        switch(decode_execute_pack.sub_op.lsu_op)
        {
            case lsu_op_t::lb:
                decode_execute_pack.rd_value = sign_extend(bus.read8(addr, false), 8);
                break;
        
            case lsu_op_t::lbu:
                decode_execute_pack.rd_value = bus.read8(addr, false);
                break;
        
            case lsu_op_t::lh:
                decode_execute_pack.rd_value = sign_extend(bus.read16(addr, false), 16);
                break;
        
            case lsu_op_t::lhu:
                decode_execute_pack.rd_value = bus.read16(addr, false);
                break;
        
            case lsu_op_t::lw:
                decode_execute_pack.rd_value = bus.read32(addr, false);
                break;
        
            case lsu_op_t::sb:
                bus.write8(addr, decode_execute_pack.src2_value & 0xff);
                break;
        
            case lsu_op_t::sh:
                bus.write16(addr, decode_execute_pack.src2_value & 0xffff);
                break;
        
            case lsu_op_t::sw:
                bus.write32(addr, decode_execute_pack.src2_value);
                break;
        }
    }
}