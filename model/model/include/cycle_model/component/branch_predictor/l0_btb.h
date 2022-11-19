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
    class l0_btb : public component::branch_predictor_base
    {
        private:
            uint32_t predict_next_pc[FETCH_WIDTH];
            uint32_t tag[L0_BTB_SIZE];
            uint32_t target_addr[L0_BTB_SIZE];
            bool valid[L0_BTB_SIZE];
            
        public:
            virtual void reset()
            {
                memset(tag, 0, sizeof(tag));
                memset(target_addr, 0, sizeof(target_addr));
                memset(valid, 0, sizeof(valid));
            }
            
            virtual void update(uint32_t pc, bool jump, uint32_t next_pc, bool hit, const branch_predictor_info_pack_t &bp_pack)
            {
                if(bp_pack.uncondition_indirect_jump)
                {
                    uint32_t index = (pc >> 2) & L0_BTB_ADDR_MASK;
                    tag[index] = pc >> (L0_BTB_ADDR_WIDTH + 2);
                    target_addr[index] = next_pc >> 2;
                    valid[index] = true;
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
                uint32_t index = (pc >> 2) & L0_BTB_ADDR_MASK;
                predict_next_pc[port] = (valid[index] && (tag[index] == (pc >> (L0_BTB_ADDR_WIDTH + 2)))) ? (target_addr[index] << 2) : (pc + 4);
            }
            
            virtual uint32_t get_next_pc(uint32_t port)
            {
                verify_only(port < FETCH_WIDTH);
                return predict_next_pc[port];
            }
            
            virtual bool is_jump(uint32_t port)
            {
                return true;
            }
            
            virtual void fill_info_pack(branch_predictor_info_pack_t &pack)
            {
                pack.uncondition_indirect_jump = true;
            }
        
            virtual void restore(const branch_predictor_info_pack_t &bp_pack)
            {
            
            }
    };
}