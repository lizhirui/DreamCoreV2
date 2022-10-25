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
#include "config.h"
#include "../csr_base.h"

namespace isa_model::component::csr
{
    class charfifo : public csr_base
    {
        private:
            charfifo_send_fifo_t *send_fifo;
            charfifo_rev_fifo_t *rev_fifo;

        public:
            charfifo(charfifo_send_fifo_t *send_fifo, charfifo_rev_fifo_t *rev_fifo) : csr_base("charfifo", 0x00000000)
            {
                this->send_fifo = send_fifo;
                this->rev_fifo = rev_fifo;
            }

            virtual uint32_t filter(uint32_t value)
            {
                if(value & 0x80000000)
                {
                    if(!rev_fifo->empty())
                    {
                        while(!rev_fifo->pop());
                    }
                }
                else
                {
                    while(!send_fifo->push((char)(value & 0xff)));
                }

                return 0;
            }

            virtual uint32_t read()
            {
                if(rev_fifo->empty())
                {
                    return 0;
                }
                else
                {
                    auto ch = rev_fifo->front();
                    return 0x80000000 | ch;
                }
            }
    };
}