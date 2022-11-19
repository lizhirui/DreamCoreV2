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
#include "../dff.h"

namespace cycle_model::component::branch_predictor
{
    class bi_modal : public component::branch_predictor_base
    {
        private:
            uint32_t predict_next_pc[FETCH_WIDTH];
            bool predict_jump[FETCH_WIDTH];
            uint32_t pht[BI_MODAL_SIZE];
            
        public:
            virtual void reset()
            {
                for(uint32_t i = 0;i < FETCH_WIDTH;i++)
                {
                    predict_next_pc[i] = 0;
                    predict_jump[i] = false;
                }
                
                memset(pht, 0, sizeof(pht));
            }
            
            virtual void update(uint32_t pc, bool jump, uint32_t next_pc, bool hit, const branch_predictor_info_pack_t &bp_pack)
            {
                if(bp_pack.condition_jump)
                {
                    uint32_t index = (pc >> 2) & BI_MODAL_ADDR_MASK;
                    
                    if(jump)
                    {
                        if(pht[index] < 3)
                        {
                            pht[index]++;
                        }
                    }
                    else
                    {
                        if(pht[index] > 0)
                        {
                            pht[index]--;
                        }
                    }
                }
            }
            
            virtual void speculative_update(uint32_t pc, bool jump)
            {
            
            }
        
            virtual void bru_speculative_update(uint32_t pc, bool jump, uint32_t next_pc, bool hit, const branch_predictor_info_pack_t &bp_pack)
            {
            
            }
            
            virtual void predict(uint32_t port, uint32_t pc, uint32_t inst)
            {
                verify_only(port < FETCH_WIDTH);
                uint32_t imm_b = (((inst >> 8) & 0x0f) << 1) | (((inst >> 25) & 0x3f) << 5) | (((inst >> 7) & 0x01) << 11) | (((inst >> 31) & 0x01) << 12);
                uint32_t target = pc + sign_extend(imm_b, 13);
                uint32_t index = (pc >> 2) & BI_MODAL_ADDR_MASK;
                verify_only(pht[index] >= 0 && pht[index] <= 3);
                predict_jump[port] = pht[index] >= 2;
                predict_next_pc[port] = predict_jump[port] ? target : (pc + 4);
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
            }
        
            virtual void restore(const branch_predictor_info_pack_t &bp_pack)
            {
            
            }
    };
}