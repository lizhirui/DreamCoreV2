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

namespace component
{
    template<typename T>
    class stack : public if_print_t, public if_reset_t
    {
        protected:
            dff<T> *buffer;
            uint32_t size = 0;
            dff<uint32_t> top_ptr;

        public:
            stack(uint32_t size) : top_ptr(0)
            {
                this->size = size;
                buffer = new dff<T>[size];
                this->stack::reset();
            }

            ~stack()
            {
                delete[] buffer;
            }

            virtual void reset()
            {
                top_ptr.set(0);
            }

            void flush()
            {
                top_ptr.set(0);
            }

            bool push(T element)
            {
                if(is_full())
                {
                    return false;
                }

                buffer[top_ptr.get()].set(element);
                top_ptr.set(top_ptr.get() + 1);
                return true;
            }

            bool pop(T *element)
            {
                if(is_empty())
                {
                    return false;
                }

                *element = buffer[top_ptr.get() - 1].get();
                top_ptr.set(top_ptr.get() - 1);
                return true;
            }

            bool get_top(T *element)
            {
                if(this->is_empty())
                {
                    return false;
                }

                *element = this->buffer[top_ptr - 1].get();
                return true;
            }

            uint32_t get_size()
            {
                return this->size;
            }

            uint32_t get_used_space()
            {
                return this->is_full() ? this->size : ((this->wptr.get() + this->size - this->rptr.get()) % this->size);
            }

            bool is_empty()
            {
                return this->top_ptr.get() == 0;
            }

            bool is_full()
            {
                return this->top_ptr.get() == this->size;
            }
    };
}