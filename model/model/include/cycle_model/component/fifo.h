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
#include "dff.h"

namespace cycle_model::component
{
    template<typename T>
    class fifo : public if_print_t, public if_reset_t
    {
        protected:
            dff<T> *buffer;
            uint32_t size = 0;
            dff<uint32_t> wptr;
            dff<bool> wstage;
            dff<uint32_t> rptr;
            dff<bool> rstage;

        public:
            fifo(uint32_t size) : wptr(0), wstage(false), rptr(0), rstage(false)
            {
                this->size = size;
                buffer = new dff<T>[size];
                this->reset();
            }

            ~fifo()
            {
                delete[] buffer;
            }

            virtual void reset()
            {
                wptr.set(0);
                wstage.set(false);
                rptr.set(0);
                rstage.set(false);
            }

            virtual void flush()
            {
                this->fifo::reset();
            }
        
            virtual bool new_push(T element)
            {
                if(new_is_full())
                {
                    return false;
                }
            
                buffer[wptr.get_new()].set(element);
            
                if(wptr.get_new() >= size - 1)
                {
                    wptr.set(0);
                    wstage.set(!wstage.get_new());
                }
                else
                {
                    wptr.set(wptr.get_new() + 1);
                }
            
                return true;
            }

            virtual bool push(T element)
            {
                if(producer_is_full())
                {
                    return false;
                }

                buffer[wptr.get_new()].set(element);

                if(wptr.get_new() >= size - 1)
                {
                    wptr.set(0);
                    wstage.set(!wstage.get_new());
                }
                else
                {
                    wptr.set(wptr.get_new() + 1);
                }
                
                return true;
            }

            virtual bool pop(T *element)
            {
                if(customer_is_empty())
                {
                    return false;
                }

                *element = buffer[rptr.get_new()].get();

                if(rptr.get_new() >= size - 1)
                {
                    rptr.set(0);
                    rstage.set(!rstage.get_new());
                }
                else
                {
                    rptr.set(rptr.get_new() + 1);
                }
                
                return true;
            }

            bool producer_get_front(T *element)
            {
                if(producer_is_empty())
                {
                    return false;
                }

                *element = buffer[rptr.get()].get();
                return true;
            }

            bool customer_get_front(T *element)
            {
                if(customer_is_empty())
                {
                    return false;
                }

                *element = buffer[rptr.get_new()].get();
                return true;
            }

            bool producer_get_tail(T *element)
            {
                if(producer_is_empty())
                {
                    return false;
                }

                *element = buffer[(wptr.get_new() + this->size - 1) % this->size].get();
                return true;
            }

            bool customer_get_tail(T *element)
            {
                if(customer_is_empty())
                {
                    return false;
                }

                *element = buffer[(wptr.get() + this->size - 1) % this->size].get();
                return true;
            }

            bool producer_get_front_id(uint32_t *front_id)
            {
                if(producer_is_empty())
                {
                    return false;
                }

                *front_id = rptr.get();
                return true;
            }

            bool customer_get_front_id(uint32_t *front_id)
            {
                if(customer_is_empty())
                {
                    return false;
                }

                *front_id = rptr.get_new();
                return true;
            }

            bool producer_get_tail_id(uint32_t *tail_id)
            {
                if(producer_is_empty())
                {
                    return false;
                }

                *tail_id = (wptr.get_new() + this->size - 1) % this->size;
                return true;
            }

            bool customer_get_tail_id(uint32_t *tail_id)
            {
                if(customer_is_empty())
                {
                    return false;
                }

                *tail_id = (wptr.get() + this->size - 1) % this->size;
                return true;
            }

            bool producer_check_id_valid(uint32_t id)
            {
                if(this->producer_is_empty())
                {
                    return false;
                }
                else if(this->wstage.get_new() == this->rstage.get())
                {
                    return (id >= this->rptr.get()) && (id < this->wptr.get_new());
                }
                else
                {
                    return ((id >= this->rptr.get()) && (id < this->size)) || (id < this->wptr.get_new());
                }
            }

            bool customer_check_id_valid(uint32_t id)
            {
                if(this->customer_is_empty())
                {
                    return false;
                }
                else if(this->wstage.get() == this->rstage.get_new())
                {
                    return (id >= this->rptr.get_new()) && (id < this->wptr.get());
                }
                else
                {
                    return ((id >= this->rptr.get_new()) && (id < this->size)) || (id < this->wptr.get());
                }
            }

            bool producer_get_next_id(uint32_t id, uint32_t *next_id)
            {
                verify(producer_check_id_valid(id));
                *next_id = (id + 1) % this->size;
                return producer_check_id_valid(*next_id);
            }

            bool customer_get_next_id(uint32_t id, uint32_t *next_id)
            {
                verify(customer_check_id_valid(id));
                *next_id = (id + 1) % this->size;
                return customer_check_id_valid(*next_id);
            }

            T producer_get_item(uint32_t id)
            {
                return this->buffer[id].get_new();
            }

            T customer_get_item(uint32_t id)
            {
                return this->buffer[id].get();
            }

            virtual void set_item(uint32_t id, T value)
            {
                buffer[id].set(value);
            }

            bool producer_get_next_id_stage(uint32_t id, bool stage, uint32_t *next_id, bool *next_stage)
            {
                verify(producer_check_id_valid(id));
                *next_id = (id + 1) % this->size;
                *next_stage = ((id + 1) >= this->size) != stage;
                return producer_check_id_valid(*next_id);
            }

            bool customer_get_next_id_stage(uint32_t id, bool stage, uint32_t *next_id, bool *next_stage)
            {
                verify(customer_check_id_valid(id));
                *next_id = (id + 1) % this->size;
                *next_stage = ((id + 1) >= this->size) != stage;
                return customer_check_id_valid(*next_id);
            }
        
            void static_get_next_id_stage(uint32_t id, bool stage, uint32_t *next_id, bool *next_stage)
            {
                *next_id = (id + 1) % this->size;
                *next_stage = ((id + 1) >= this->size) != stage;
            }
            
            uint32_t producer_get_wptr() const
            {
                return wptr.get_new();
            }
            
            bool producer_get_wstage() const
            {
                return wstage.get_new();
            }
        
            uint32_t customer_get_wptr() const
            {
                return wptr.get();
            }
        
            bool customer_get_wstage() const
            {
                return wstage.get();
            }
            
            virtual void customer_foreach(std::function<bool(uint32_t, bool, const T&)> func)
            {
                if(!customer_is_empty())
                {
                    auto cur = rptr.get();
                    auto cur_stage = rstage.get();
    
                    while(true)
                    {
                        auto item = buffer[cur].get();
                        
                        if(!func(cur, cur_stage, item))
                        {
                            break;
                        }
        
                        cur++;
        
                        if(cur >= size)
                        {
                            cur = 0;
                            cur_stage = !cur_stage;
                        }
        
                        if((cur == wptr.get()) && (cur_stage == wstage.get()))
                        {
                            break;
                        }
                    }
                }
            }

            uint32_t get_size()
            {
                return this->size;
            }

            uint32_t producer_get_used_space()
            {
                return producer_is_full() ? this->size : (this->wptr.get_new() + this->size - this->rptr.get()) % this->size;
            }

            uint32_t customer_get_used_space()
            {
                return customer_is_full() ? this->size : (this->wptr.get() + this->size - this->rptr.get_new()) % this->size;
            }
        
            uint32_t new_get_used_space()
            {
                return new_is_full() ? this->size : (this->wptr.get_new() + this->size - this->rptr.get_new()) % this->size;
            }

            uint32_t producer_get_free_space()
            {
                return this->size - producer_get_used_space();
            }

            uint32_t customer_get_free_space()
            {
                return this->size - customer_get_used_space();
            }

            bool producer_is_empty()
            {
                return (this->wptr.get_new() == this->rptr.get()) && (this->wstage.get_new() == this->rstage.get());
            }

            bool customer_is_empty()
            {
                return (this->wptr.get() == this->rptr.get_new()) && (this->wstage.get() == this->rstage.get_new());
            }
        
            bool new_is_empty()
            {
                return (this->wptr.get_new() == this->rptr.get_new()) && (this->wstage.get_new() == this->rstage.get_new());
            }

            virtual bool producer_is_full()
            {
                return (this->wptr.get_new() == this->rptr.get()) && (this->wstage.get_new() != this->rstage.get());
            }

            bool customer_is_full()
            {
                return (this->wptr.get() == this->rptr.get_new()) && (this->wstage.get() != this->rstage.get_new());
            }
            
            bool new_is_full()
            {
                return (this->wptr.get_new() == this->rptr.get_new()) && (this->wstage.get_new() != this->rstage.get_new());
            }
            
            void update_wptr(uint32_t wptr, bool wstage)
            {
                this->wptr.set(wptr);
                this->wstage.set(wstage);
            }

            virtual void print(std::string indent)
            {
                std::cout << indent << "Item Count = " << this->get_size() << "/" << this->size << std::endl;
                std::cout << indent << "Input:" << std::endl;
                T item;
                if_print_t *if_print;

                if(!this->producer_get_tail(&item))
                {
                    std::cout << indent << "\t<Empty>" << std::endl;
                }
                else
                {
                    if_print = dynamic_cast<if_print_t *>(&item);
                    if_print->print(indent + "\t");
                }

                std::cout << indent << "Output:" << std::endl;

                if(!this->producer_get_front(&item))
                {
                    std::cout << indent << "\t<Empty>" << std::endl;
                }
                else
                {
                    if_print = dynamic_cast<if_print_t *>(&item);
                    if_print->print(indent + "\t");
                }
            }

            virtual json get_json()
            {
                json ret = json::array();
                if_print_t *if_print;

                if(!customer_is_empty())
                {
                    auto cur = rptr.get();
                    auto cur_stage = rstage.get();

                    while(true)
                    {
                        auto item = buffer[cur].get();
                        if_print = dynamic_cast<if_print_t *>(&item);
                        ret.push_back(if_print->get_json());

                        cur++;

                        if(cur >= size)
                        {
                            cur = 0;
                            cur_stage = !cur_stage;
                        }

                        if((cur == wptr.get()) && (cur_stage == wstage.get()))
                        {
                            break;
                        }
                    }
                }

                return ret;
            }
    };
}