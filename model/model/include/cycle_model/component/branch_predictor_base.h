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

namespace cycle_model::component
{
    typedef struct branch_predictor_info_pack_t : if_print_t
    {
        uint32_t bi_mode_global_history = 0;
        uint32_t bi_mode_pht_value = 0;//only for debug
        bool predicted = false;
        bool jump = false;
        uint32_t next_pc = 0;
        bool uncondition_indirect_jump = false;
        bool condition_jump = false;
        bool checkpoint_id_valid = false;
        uint32_t checkpoint_id = 0;
    }branch_predictor_info_pack_t;
    
    class branch_predictor_base
    {
        private:
            static std::unordered_set<branch_predictor_base *> bp_list;
            
            enum class sync_request_type_t
            {
                update,
                speculative_update,
                bru_speculative_update,
                restore
            };
        
            typedef struct sync_request_t
            {
                sync_request_type_t req = sync_request_type_t::update;
                uint32_t pc = 0;
                bool jump = false;
                uint32_t next_pc = 0;
                bool hit = false;
                branch_predictor_info_pack_t bp_pack;
            }sync_request_t;
        
            std::queue<sync_request_t> sync_request_q;
            
        public:
            branch_predictor_base()
            {
                bp_list.insert(this);
            }
            
            ~branch_predictor_base()
            {
                bp_list.erase(this);
            }
            
            static void foreach(std::function<void(branch_predictor_base *)> func)
            {
                for(auto bp : bp_list)
                {
                    func(bp);
                }
            }
            
            static void batch_reset()
            {
                branch_predictor_base::foreach([](branch_predictor_base *bp){bp->reset();});
            }
            
            static void batch_update(uint32_t pc, bool jump, uint32_t next_pc, bool hit, const branch_predictor_info_pack_t &bp_pack)
            {
                branch_predictor_base::foreach([pc, jump, next_pc, hit, &bp_pack](branch_predictor_base *bp){bp->update(pc, jump, next_pc, hit, bp_pack);});
            }
        
            static void batch_update_sync(uint32_t pc, bool jump, uint32_t next_pc, bool hit, const branch_predictor_info_pack_t &bp_pack)
            {
                branch_predictor_base::foreach([pc, jump, next_pc, hit, &bp_pack](branch_predictor_base *bp){bp->update_sync(pc, jump, next_pc, hit, bp_pack);});
            }
            
            static void batch_speculative_update(uint32_t pc, bool jump)
            {
                branch_predictor_base::foreach([pc, jump](branch_predictor_base *bp){bp->speculative_update(pc, jump);});
            }
        
            static void batch_speculative_update_sync(uint32_t pc, bool jump)
            {
                branch_predictor_base::foreach([pc, jump](branch_predictor_base *bp){bp->speculative_update_sync(pc, jump);});
            }
            
            static void batch_restore(const branch_predictor_info_pack_t &bp_pack)
            {
                branch_predictor_base::foreach([&bp_pack](branch_predictor_base *bp){bp->restore(bp_pack);});
            }
            
            static void batch_restore_sync(const branch_predictor_info_pack_t &bp_pack)
            {
                branch_predictor_base::foreach([&bp_pack](branch_predictor_base *bp){bp->restore_sync(bp_pack);});
            }
            
            static void batch_predict(uint32_t port, uint32_t pc, uint32_t inst)
            {
                branch_predictor_base::foreach([port, pc, inst](branch_predictor_base *bp){bp->predict(port, pc, inst);});
            }
            
            static void batch_sync()
            {
                branch_predictor_base::foreach([](branch_predictor_base *bp){bp->sync();});
            }
            
            void update_sync(uint32_t pc, bool jump, uint32_t next_pc, bool hit, const branch_predictor_info_pack_t &bp_pack)
            {
                sync_request_t req;
                req.req = sync_request_type_t::update;
                req.pc = pc;
                req.jump = jump;
                req.next_pc = next_pc;
                req.hit = hit;
                req.bp_pack = bp_pack;
                sync_request_q.push(req);
            }
            
            void speculative_update_sync(uint32_t pc, bool jump)
            {
                sync_request_t req;
                req.req = sync_request_type_t::speculative_update;
                req.pc = pc;
                req.jump = jump;
                sync_request_q.push(req);
            }
        
            void bru_speculative_update_sync(uint32_t pc, bool jump, uint32_t next_pc, bool hit, const branch_predictor_info_pack_t &bp_pack)
            {
                sync_request_t req;
                req.req = sync_request_type_t::bru_speculative_update;
                req.pc = pc;
                req.jump = jump;
                req.next_pc = next_pc;
                req.hit = hit;
                req.bp_pack = bp_pack;
                sync_request_q.push(req);
            }
            
            void restore_sync(const branch_predictor_info_pack_t &bp_pack)
            {
                sync_request_t req;
                req.req = sync_request_type_t::restore;
                req.bp_pack = bp_pack;
                sync_request_q.push(req);
            }
            
            void sync()
            {
                while(!sync_request_q.empty())
                {
                    sync_request_t req = sync_request_q.front();
                    sync_request_q.pop();
                    
                    switch(req.req)
                    {
                        case sync_request_type_t::update:
                            this->update(req.pc, req.jump, req.next_pc, req.hit, req.bp_pack);
                            break;
                            
                        case sync_request_type_t::speculative_update:
                            this->speculative_update(req.pc, req.jump);
                            break;
    
                        case sync_request_type_t::bru_speculative_update:
                            this->bru_speculative_update(req.pc, req.jump, req.next_pc, req.hit, req.bp_pack);
                            break;
                            
                        case sync_request_type_t::restore:
                            this->restore(req.bp_pack);
                            break;
                    }
                }
            }
            
            virtual void reset() = 0;
            virtual void update(uint32_t pc, bool jump, uint32_t next_pc, bool hit, const branch_predictor_info_pack_t &bp_pack) = 0;
            virtual void speculative_update(uint32_t pc, bool jump) = 0;
            virtual void bru_speculative_update(uint32_t pc, bool jump, uint32_t next_pc, bool hit, const branch_predictor_info_pack_t &bp_pack) = 0;
            virtual void predict(uint32_t port, uint32_t pc, uint32_t inst) = 0;
            virtual uint32_t get_next_pc(uint32_t port) = 0;
            virtual bool is_jump(uint32_t port) = 0;
            virtual void fill_info_pack(branch_predictor_info_pack_t &pack) = 0;
            virtual void restore(const branch_predictor_info_pack_t &bp_pack) = 0;
    };
}