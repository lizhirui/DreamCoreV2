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
#include <elf.h>

typedef Elf32_Ehdr elf_ehdr_t;
typedef Elf32_Shdr elf_shdr_t;
typedef Elf32_Phdr elf_phdr_t;
typedef Elf32_Sym elf_sym_t;
typedef Elf32_Addr elf_addr_t;
typedef Elf32_Half elf_half_t;
typedef Elf32_Off elf_off_t;
typedef Elf32_Sword elf_sword_t;
typedef Elf32_Word elf_word_t;

std::pair<std::shared_ptr<uint8_t[]>, size_t> load_elf(std::string path)
{
    std::ifstream binfile(path, std::ios::binary);
    
    if(!binfile || !binfile.is_open())
    {
        std::cout << path << " Open Failed!" << std::endl;
        exit(1);
    }
    
    binfile.seekg(0, std::ios::end);
    auto size = binfile.tellg();
    std::cout << "File Size: " << size << " Byte(s)" << std::endl;
    binfile.seekg(0, std::ios::beg);
    auto buf = std::shared_ptr<uint8_t[]>(new uint8_t[size]);
    binfile.read((char *)buf.get(), size);
    binfile.close();
    auto ehdr = (elf_ehdr_t *)buf.get();
    
    if(ehdr->e_ident[EI_MAG0] != ELFMAG0 || ehdr->e_ident[EI_MAG1] != ELFMAG1 || ehdr->e_ident[EI_MAG2] != ELFMAG2 || ehdr->e_ident[EI_MAG3] != ELFMAG3)
    {
        std::cout << path << " is not a valid ELF file!" << std::endl;
        exit(1);
    }
    
    if(ehdr->e_ident[EI_CLASS] != ELFCLASS32)
    {
        std::cout << path << " is not a 32-bit ELF file!" << std::endl;
        exit(1);
    }
    
    if(ehdr->e_ident[EI_DATA] != ELFDATA2LSB)
    {
        std::cout << path << " is not a little-endian ELF file!" << std::endl;
        exit(1);
    }
    
    if(ehdr->e_ident[EI_VERSION] != EV_CURRENT)
    {
        std::cout << path << " is not a valid ELF file!" << std::endl;
        exit(1);
    }
    
    if(ehdr->e_type != ET_EXEC)
    {
        std::cout << path << " is not a executable ELF file!" << std::endl;
        exit(1);
    }
    
    if(ehdr->e_machine != EM_RISCV)
    {
        std::cout << path << " is not a RISC-V ELF file!" << std::endl;
        exit(1);
    }
    
    if(ehdr->e_version != EV_CURRENT)
    {
        std::cout << path << " is not a valid ELF file!" << std::endl;
        exit(1);
    }
    
    if(ehdr->e_phoff == 0 || ehdr->e_phentsize != sizeof(elf_phdr_t) || ehdr->e_phnum == 0)
    {
        std::cout << path << " is not a valid ELF file!" << std::endl;
        exit(1);
    }
    
    if(ehdr->e_shoff == 0 || ehdr->e_shentsize != sizeof(elf_shdr_t) || ehdr->e_shnum == 0)
    {
        std::cout << path << " is not a valid ELF file!" << std::endl;
        exit(1);
    }
    
    if(ehdr->e_entry != INIT_PC)
    {
        std::cout << path << " has an invalid entry 0x" << outhex(INIT_PC) << "!" << std::endl;
        exit(1);
    }
    
    auto phdr = (elf_phdr_t *)(buf.get() + ehdr->e_phoff);
    
    size_t bin_size = 0;
    
    for(int i = 0; i < ehdr->e_phnum; i++)
    {
        if(phdr[i].p_type != PT_LOAD)
        {
            continue;
        }
        
        if(phdr[i].p_offset + phdr[i].p_filesz > size)
        {
            std::cout << path << " is not a valid ELF file!" << std::endl;
            exit(1);
        }
        
        if(phdr[i].p_paddr < MEMORY_BASE)
        {
            std::cout << path << " has an invalid virtual address 0x" << outhex(phdr[i].p_paddr) << "!" << std::endl;
            exit(1);
        }
        
        if(phdr[i].p_paddr + phdr[i].p_memsz > (MEMORY_BASE + MEMORY_SIZE))
        {
            std::cout << path << " is too large!" << std::endl;
            exit(1);
        }
        
        if(phdr[i].p_memsz < phdr[i].p_filesz)
        {
            std::cout << path << " is not a valid ELF file!" << std::endl;
            exit(1);
        }
        
        bin_size = std::max(bin_size, (size_t)(phdr[i].p_paddr + phdr[i].p_memsz - MEMORY_BASE));
    }
    
    if(bin_size > MEMORY_SIZE)
    {
        std::cout << path << " is too large!" << std::endl;
        exit(1);
    }
    
    std::cout << "Binary Size: " << bin_size << " Byte(s)" << std::endl;
    auto bin_buf = std::shared_ptr<uint8_t[]>(new uint8_t[bin_size]);
    memset(bin_buf.get(), 0, bin_size);
    
    for(int i = 0; i < ehdr->e_phnum; i++)
    {
        if(phdr[i].p_type != PT_LOAD)
        {
            continue;
        }
        
        memcpy(bin_buf.get() + phdr[i].p_paddr - MEMORY_BASE, buf.get() + phdr[i].p_offset, phdr[i].p_filesz);
    }
    
    return {bin_buf, bin_size};
}

