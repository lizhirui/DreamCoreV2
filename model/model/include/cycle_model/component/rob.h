/*
 * Copyright lizhirui
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-10-14     lizhirui     the first version
 */

#pragma once
#include "common.h"
#include "fifo.h"

namespace cycle_model::component
{
    typedef struct rob_item_t : public if_print_t
    {
        uint32_t new_phy_reg_id = 0;
        uint32_t old_phy_reg_id = 0;
        bool old_phy_reg_id_valid = false;
        bool finish = false;
        uint32_t pc = 0;
        uint32_t inst_value = 0;
        uint32_t rd = 0;
        bool has_exception = false;
        riscv_exception_t exception_id = riscv_exception_t::instruction_address_misaligned;
        uint32_t exception_value = 0;
        bool bru_op = false;
        bool bru_jump = false;
        uint32_t bru_next_pc = 0;
        bool is_mret = false;
        uint32_t csr_addr = 0;
        uint32_t csr_newvalue = 0;
        bool csr_newvalue_valid = false;
        uint32_t old_phy_id_free_list_rptr = 0;
        bool old_phy_id_free_list_rstage = false;
        uint32_t new_phy_id_free_list_rptr = 0;
        bool new_phy_id_free_list_rstage = false;
        
        virtual void print(std::string indent)
        {
            std::string blank = "    ";
            std::cout << indent << "new_phy_reg_id = " << new_phy_reg_id;
            std::cout << blank << "old_phy_reg_id = " << old_phy_reg_id;
            std::cout << blank << "old_phy_reg_id_valid = " << outbool(old_phy_reg_id_valid);
            std::cout << blank << "finish = " << outbool(finish);
            std::cout << blank << "pc = 0x" << fillzero(8) << outhex(pc);
            std::cout << blank << "inst_value = 0x" << fillzero(8) << outhex(inst_value);
            std::cout << blank << "has_exception = " << outbool(has_exception);
            std::cout << blank << "exception_id = " << outenum(exception_id);
            std::cout << blank << "exception_value = 0x" << fillzero(8) << outhex(exception_value) << std::endl;
        }
        
        virtual json get_json()
        {
            json ret;
            
            ret["new_phy_reg_id"] = new_phy_reg_id;
            ret["old_phy_reg_id"] = old_phy_reg_id;
            ret["old_phy_reg_id_valid"] = old_phy_reg_id_valid;
            ret["finish"] = finish;
            ret["pc"] = pc;
            ret["inst_value"] = inst_value;
            ret["rd"] = rd;
            ret["has_exception"] = has_exception;
            ret["exception_id"] = outenum(exception_id);
            ret["exception_value"] = exception_value;
            ret["bru_op"] = bru_op;
            ret["bru_jump"] = bru_jump;
            ret["bru_next_pc"] = bru_next_pc;
            ret["is_mret"] = is_mret;
            ret["csr_addr"] = csr_addr;
            ret["csr_newvalue"] = csr_newvalue;
            ret["csr_newvalue_valid"] = csr_newvalue_valid;
            ret["old_phy_id_free_list_rptr"] = old_phy_id_free_list_rptr;
            ret["old_phy_id_free_list_rstage"] = old_phy_id_free_list_rstage;
            ret["new_phy_id_free_list_rptr"] = new_phy_id_free_list_rptr;
            ret["new_phy_id_free_list_rstage"] = new_phy_id_free_list_rstage;
            return ret;
        }
    }rob_item_t;
    
    class rob : public fifo<rob_item_t>
    {
        private:
            trace::trace_database tdb;
            
            bool committed = false;
            uint32_t commit_num = 0;
            uint64_t global_commit_num = 0;
        
        public:
            rob(uint32_t size) : fifo<rob_item_t>(size), tdb(TRACE_ROB)
            {
                this->rob::reset();
            }
            
            virtual void reset()
            {
                fifo<rob_item_t>::reset();
                committed = false;
                commit_num = 0;
                global_commit_num = 0;
            }
            
            void trace_pre()
            {
            
            }
            
            void trace_post()
            {
            
            }
            
            trace::trace_database *get_tdb()
            {
                return &tdb;
            }
            
            bool get_committed() const
            {
                return committed;
            }
            
            void set_committed(bool value)
            {
                committed = value;
            }
            
            void add_commit_num(uint32_t add_num)
            {
                commit_num += add_num;
                global_commit_num += add_num;
            }
            
            uint64_t get_global_commit_num() const
            {
                return global_commit_num;
            }
            
            uint32_t get_commit_num() const
            {
                return commit_num;
            }
            
            void clear_commit_num()
            {
                commit_num = 0;
            }
            
            virtual bool push(rob_item_t element)
            {
                return fifo<rob_item_t>::push(element);
            }
            
            rob_item_t get_item(uint32_t item_id)
            {
                return this->buffer[item_id].get();
            }
        
            bool check_new_id_valid(uint32_t id)
            {
                if(this->producer_is_full())
                {
                    return false;
                }
                else if(this->wstage.get_new() == this->rstage.get())
                {
                    return !((id >= this->rptr.get()) && (id < this->wptr.get_new()));
                }
                else
                {
                    return !(((id >= this->rptr.get()) && (id < this->size)) || (id < this->wptr.get_new()));
                }
            }
            
            bool get_new_id(uint32_t *new_id)
            {
                if(this->producer_is_full())
                {
                    return false;
                }
                
                *new_id = this->wptr.get_new();
                return true;
            }
            
            bool get_new_id_stage(bool *new_id_stage)
            {
                if(this->producer_is_full())
                {
                    return false;
                }
                
                *new_id_stage = this->wptr.get_new();
                return true;
            }
            
            bool get_next_new_id(uint32_t cur_new_id, uint32_t *next_new_id)
            {
                if(!check_new_id_valid(cur_new_id))
                {
                    return false;
                }
                
                *next_new_id = (cur_new_id + 1) % this->size;
                return check_new_id_valid(*next_new_id);
            }
            
            bool get_front_id(uint32_t *front_id)
            {
                return customer_get_front_id(front_id);
            }
            
            bool get_tail_id(uint32_t *tail_id)
            {
                return customer_get_tail_id(tail_id);
            }
            
            bool get_prev_id(uint32_t id, uint32_t *prev_id)
            {
                verify(customer_check_id_valid(id));
                *prev_id = (id + this->size - 1) % this->size;
                return customer_check_id_valid(*prev_id);
            }
            
            bool get_next_id(uint32_t id, uint32_t *next_id)
            {
                verify(customer_check_id_valid(id));
                *next_id = (id + 1) % this->size;
                return customer_check_id_valid(*next_id);
            }
            
            virtual bool pop(rob_item_t *element)
            {
                return fifo<rob_item_t>::pop(element);
            }
            
            bool pop()
            {
                rob_item_t t;
                return fifo<rob_item_t>::pop(&t);
            }
            
            virtual json get_json()
            {
                json ret = json::array();
                if_print_t *if_print;
                
                if(!producer_is_empty())
                {
                    auto cur = rptr.get();
                    auto cur_stage = rstage.get();
                    
                    while(true)
                    {
                        if_print = dynamic_cast<if_print_t *>(&buffer[cur]);
                        json item = if_print->get_json();
                        item["rob_id"] = cur;
                        ret.push_back(item);
                        
                        cur++;
                        
                        if(cur >= size)
                        {
                            cur = 0;
                            cur_stage = !cur_stage;
                        }
                        
                        if((cur == wptr) && (cur_stage == wstage))
                        {
                            break;
                        }
                    }
                }
                
                return ret;
            }
    };
}