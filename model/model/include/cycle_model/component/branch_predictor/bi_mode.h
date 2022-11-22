/*
 * Copyright lizhirui
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-11-13     lizhirui     the first version
 */

#pragma once
#include "common.h"
#include "config.h"
#include "../branch_predictor_base.h"

namespace cycle_model::component::branch_predictor
{
    class bi_mode : public component::branch_predictor_base
    {
        private:
            component::dff<uint32_t> predict_next_pc[FETCH_WIDTH];
            component::dff<bool> predict_jump[FETCH_WIDTH];
            uint32_t global_history;
            uint32_t global_history_retired;
            uint32_t pht_left[BI_MODE_PHT_SIZE];
            uint32_t pht_right[BI_MODE_PHT_SIZE];
            uint32_t pht_choice[BI_MODE_PHT_SIZE];
        
            void _update(uint32_t history, uint32_t pc, bool jump, uint32_t next_pc, bool hit, const branch_predictor_info_pack_t &bp_pack)
            {
                if(bp_pack.condition_jump)
                {
                    uint32_t branch_pc = (pc >> 2) & BI_MODE_BRANCH_PC_MASK;
                    uint32_t pht_addr = history ^ branch_pc;
                    uint32_t pht_choice_addr = (pc >> 2) & BI_MODE_PHT_CHOICE_ADDR_MASK;
                    bool is_pht_left = pht_choice[pht_choice_addr] <= 1;
                
                    if(is_pht_left)
                    {
                        if(jump)
                        {
                            if(pht_left[pht_addr] < 3)
                            {
                                pht_left[pht_addr]++;
                            }
                        }
                        else
                        {
                            if(pht_left[pht_addr] > 0)
                            {
                                pht_left[pht_addr]--;
                            }
                        }
                    }
                    else
                    {
                        if(jump)
                        {
                            if(pht_right[pht_addr] < 3)
                            {
                                pht_right[pht_addr]++;
                            }
                        }
                        else
                        {
                            if(pht_right[pht_addr] > 0)
                            {
                                pht_right[pht_addr]--;
                            }
                        }
                    }
                
                    if(!(((pht_choice[pht_choice_addr] >= 2) != jump) && hit))
                    {
                        if(jump)
                        {
                            if(pht_choice[pht_choice_addr] < 3)
                            {
                                pht_choice[pht_choice_addr]++;
                            }
                        }
                        else
                        {
                            if(pht_choice[pht_choice_addr] > 0)
                            {
                                pht_choice[pht_choice_addr]--;
                            }
                        }
                    }
                }
            }
            
        public:
            virtual void reset()
            {
                for(uint32_t i = 0;i < FETCH_WIDTH;i++)
                {
                    predict_next_pc[i].set(0);
                    predict_jump[i].set(false);
                }
                
                global_history = 0;
                global_history_retired = 0;
                memset(pht_left, 0, sizeof(pht_left));
                memset(pht_right, 0, sizeof(pht_right));
                memset(pht_choice, 0, sizeof(pht_choice));
            }
        
            virtual void update(uint32_t pc, bool jump, uint32_t next_pc, bool hit, const branch_predictor_info_pack_t &bp_pack)
            {
                _update(bp_pack.bi_mode_global_history, pc, jump, next_pc, hit, bp_pack);
    
                if(bp_pack.condition_jump)
                {
                    if(!hit && !bp_pack.checkpoint_id_valid)
                    {
                        global_history = global_history_retired;
                    }
                    
                    global_history_retired = ((global_history_retired << 1) & BI_MODE_GLOBAL_HISTORY_MASK) | (jump ? 1 : 0);
                }
            }
        
            virtual void bru_speculative_update(uint32_t pc, bool jump, uint32_t next_pc, bool hit, const branch_predictor_info_pack_t &bp_pack)
            {
                if(bp_pack.condition_jump)
                {
                    if(!hit)
                    {
                        global_history = ((bp_pack.bi_mode_global_history << 1) & BI_MODE_GLOBAL_HISTORY_MASK) | (jump ? 1 : 0);
                    }
                }
            }
            
            virtual void speculative_update(uint32_t pc, bool jump)
            {
                global_history = ((global_history << 1) & BI_MODE_GLOBAL_HISTORY_MASK) | (jump ? 1 : 0);
            }
        
            virtual void predict(uint32_t port, uint32_t pc, uint32_t inst)
            {
                verify_only(port < FETCH_WIDTH);
                uint32_t imm_b = (((inst >> 8) & 0x0f) << 1) | (((inst >> 25) & 0x3f) << 5) | (((inst >> 7) & 0x01) << 11) | (((inst >> 31) & 0x01) << 12);
                uint32_t target = pc + sign_extend(imm_b, 13);
                uint32_t branch_pc = (pc >> 2) & BI_MODE_BRANCH_PC_MASK;
                uint32_t pht_addr = global_history ^ branch_pc;
                uint32_t pht_choice_addr = (pc >> 2) & BI_MODE_PHT_CHOICE_ADDR_MASK;
                bool jump = (pht_choice[pht_choice_addr] <= 1) ? (pht_left[pht_addr] >= 2) : (pht_right[pht_addr] >= 2);
                predict_jump[port].set(jump);
                predict_next_pc[port].set(jump ? target : (pc + 4));
            }
            
            uint32_t get_pht(uint32_t pc, uint32_t history)
            {
                uint32_t branch_pc = (pc >> 2) & BI_MODE_BRANCH_PC_MASK;
                uint32_t pht_addr = history ^ branch_pc;
                uint32_t pht_choice_addr = (pc >> 2) & BI_MODE_PHT_CHOICE_ADDR_MASK;
                return (pht_choice[pht_choice_addr] <= 1) ? pht_left[pht_addr] : pht_right[pht_addr];
            }
            
            uint32_t get_global_history_retired() const
            {
                return global_history_retired;
            }
            
            virtual uint32_t get_next_pc(uint32_t port)
            {
                verify_only(port < FETCH_WIDTH);
                return predict_next_pc[port];
            }
            
            virtual bool is_jump(uint32_t port)
            {
                verify_only(port < FETCH_WIDTH);
                return predict_jump[port];
            }
            
            virtual void fill_info_pack(branch_predictor_info_pack_t &pack)
            {
                pack.condition_jump = true;
                pack.bi_mode_global_history = global_history;
            }
        
            void fill_info_pack_debug(uint32_t pc, branch_predictor_info_pack_t &pack)
            {
                pack.condition_jump = true;
                pack.bi_mode_global_history = global_history;
                pack.bi_mode_pht_value = get_pht(pc, global_history);
            }
            
            virtual void restore(const branch_predictor_info_pack_t &bp_pack)
            {
                global_history = bp_pack.bi_mode_global_history;
            }
    };
}