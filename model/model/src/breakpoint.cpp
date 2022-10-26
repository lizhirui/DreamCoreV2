/*
 * Copyright lizhirui
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-10-22     lizhirui     the first version
 */

#include "common.h"
#include "config.h"
#include "breakpoint.h"
#include "main.h"

static std::vector<breakpoint_info_t> breakpoint_info_list;

void breakpoint_clear()
{
    breakpoint_info_list.clear();
}

void breakpoint_add(breakpoint_type_t type, uint32_t value)
{
    breakpoint_info_t breakpoint_info;
    breakpoint_info.type = type;
    breakpoint_info.value = value;
    breakpoint_info_list.push_back(breakpoint_info);
}

void breakpoint_remove(uint32_t id)
{
    if(id < breakpoint_info_list.size())
    {
        breakpoint_info_list.erase(breakpoint_info_list.begin() + id);
    }
}

std::string breakpoint_get_list()
{
    std::stringstream stream;
    
    for(size_t i = 0;i < breakpoint_info_list.size();i++)
    {
        stream << i << ": ";
        
        switch(breakpoint_info_list[i].type)
        {
            case breakpoint_type_t::cycle:
                stream << "cycle = " << breakpoint_info_list[i].value;
                break;
            
            case breakpoint_type_t::instruction_num:
                stream << "instruction_num = " << breakpoint_info_list[i].value;
                break;
            
            case breakpoint_type_t::pc:
                stream << "pc = 0x" << outhex(breakpoint_info_list[i].value);
                break;
            
            case breakpoint_type_t::csrw:
                stream << "csrw = 0x" << outhex(breakpoint_info_list[i].value);
                break;
        }
        
        stream << std::endl;
    }
    
    return stream.str();
}

static bool breakpoint_found = false;
static bool breakpoint_csr_finish_found = false;
static uint32_t breakpoint_csr_finish_value = 0;

void breakpoint_csr_trigger(uint32_t csr, uint32_t value, bool write)
{
    if((csr == CSR_FINISH) && write)
    {
        breakpoint_csr_finish_found = true;
        breakpoint_csr_finish_value = value;
        breakpoint_found = true;
        return;
    }
    
    for(size_t i = 0;i < breakpoint_info_list.size();i++)
    {
        if((breakpoint_info_list[i].type == breakpoint_type_t::csrw) && write)
        {
            if(breakpoint_info_list[i].value == csr)
            {
                breakpoint_found = true;
                break;
            }
        }
    }
}

std::optional<uint32_t> breakpoint_get_finish()
{
    if(breakpoint_csr_finish_found)
    {
        breakpoint_csr_finish_found = false;
        return breakpoint_csr_finish_value;
    }
    
    return std::nullopt;
}

bool breakpoint_check(uint32_t cycle, uint32_t instruction_num, uint32_t pc[], size_t pc_num)
{
    if(breakpoint_found)
    {
        breakpoint_found = false;
        return true;
    }
    
    for(size_t i = 0;i < breakpoint_info_list.size();i++)
    {
        switch(breakpoint_info_list[i].type)
        {
            case breakpoint_type_t::cycle:
                if(cycle == breakpoint_info_list[i].value)
                {
                    return true;
                }
    
                break;
            
            case breakpoint_type_t::instruction_num:
                if(instruction_num == breakpoint_info_list[i].value)
                {
                    return true;
                }
                
                break;
                
            case breakpoint_type_t::pc:
                for(size_t j = 0;j < pc_num;j++)
                {
                    if(pc[j] == breakpoint_info_list[i].value)
                    {
                        return true;
                    }
                }
                
                break;
                
            default:
                break;
        }
    }
    
    return false;
}