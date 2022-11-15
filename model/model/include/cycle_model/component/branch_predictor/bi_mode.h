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
            
        public:
            virtual void reset()
            {
                for(uint32_t i = 0;i < FETCH_WIDTH;i++)
                {
                    predict_next_pc[i].set(0);
                    predict_jump[i].set(false);
                }
            }
            
            virtual void update(uint32_t pc, bool jump, uint32_t next_pc, bool hit, const branch_predictor_info_pack_t &bp_pack)
            {
            
            }
            
            virtual void speculative_update(uint32_t pc, bool jump)
            {
            
            }
        
            virtual void predict(uint32_t port, uint32_t pc, uint32_t inst)
            {
                verify_only(port < FETCH_WIDTH);
                uint32_t imm_b = (((inst >> 8) & 0x0f) << 1) | (((inst >> 25) & 0x3f) << 5) | (((inst >> 7) & 0x01) << 11) | (((inst >> 31) & 0x01) << 12);
                uint32_t target = pc + sign_extend(imm_b, 13);
                predict_jump[port].set(target < pc);
                predict_next_pc[port].set((target < pc) ? target : pc + 4);
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
    };
}