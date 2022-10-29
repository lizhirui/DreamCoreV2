#include "common.h"
#include "config.h"
#include "network/network.h"
#include "main.h"
#include "breakpoint.h"
#include "elf_loader.h"

#ifdef NEED_ISA_MODEL
#include "isa_model/isa_model.h"
isa_model::isa_model *isa_model_inst;
#endif

#ifdef NEED_CYCLE_MODEL
#include "cycle_model/cycle_model.h"
cycle_model::cycle_model *cycle_model_inst;
#endif

static std::atomic<bool> pause_detected = false;
static bool pause_state = false;
static bool step_state = false;
static bool wait_commit = false;

static bool load_program = false;
static std::shared_ptr<uint8_t[]> program_buffer = nullptr;
static size_t program_size = 0;

#ifdef NEED_ISA_AND_CYCLE_MODEL_COMPARE
static charfifo_send_fifo_t isa_model_charfifo_send_fifo, cycle_model_charfifo_send_fifo;
static charfifo_rev_fifo_t isa_model_charfifo_rev_fifo, cycle_model_charfifo_rev_fifo;

charfifo_rev_fifo_t *get_isa_model_charfifo_rev_fifo()
{
    return &isa_model_charfifo_rev_fifo;
}

charfifo_rev_fifo_t *get_cycle_model_charfifo_rev_fifo()
{
    return &cycle_model_charfifo_rev_fifo;
}
#endif

uint64_t get_cpu_clock_cycle()
{
#ifdef NEED_CYCLE_MODEL
    return cycle_model_inst->cpu_clock_cycle;
#else
    return 0;
#endif
}

void set_pause_state(bool value)
{
    pause_state = value;
}

void set_step_state(bool value)
{
    step_state = value;
}

void set_wait_commit(bool value)
{
    wait_commit = value;
}

void set_pause_detected(bool value)
{
    pause_detected = value;
}

uint32_t get_current_pc()
{
#ifdef NEED_CYCLE_MODEL
    cycle_model::component::rob_item_t rob_item;
    
    if(cycle_model_inst->rob.customer_get_front(&rob_item))
    {
        return rob_item.pc;
    }
    
    cycle_model::pipeline::decode_rename_pack_t drpack;
    
    if(cycle_model_inst->decode_rename_fifo.customer_get_front(&drpack) && drpack.enable)
    {
        return drpack.pc;
    }
    
    cycle_model::pipeline::fetch2_decode_pack_t f2dpack;
    
    if(cycle_model_inst->fetch2_decode_fifo.customer_get_front(&f2dpack) && f2dpack.enable)
    {
        return f2dpack.pc;
    }
    
    auto f1f2pack = cycle_model_inst->fetch1_fetch2_port.get();
    
    if(f1f2pack.op_info[0].enable)
    {
        return f1f2pack.op_info[0].pc;
    }
    
    return cycle_model_inst->fetch1_stage.get_pc();
#else
    return isa_model_inst->pc;
#endif
}

static void init(const command_line_arg_t &arg)
{
    network_init(arg);
#ifdef NEED_ISA_MODEL
#ifdef NEED_ISA_AND_CYCLE_MODEL_COMPARE
    isa_model_inst = isa_model::isa_model::create(&isa_model_charfifo_send_fifo, &isa_model_charfifo_rev_fifo);
#else
    isa_model_inst = isa_model::isa_model::create(get_charfifo_send_fifo(), get_charfifo_rev_fifo());
#endif
#endif
#ifdef NEED_CYCLE_MODEL
#ifdef NEED_ISA_AND_CYCLE_MODEL_COMPARE
    cycle_model_inst = cycle_model::cycle_model::create(&cycle_model_charfifo_send_fifo, &cycle_model_charfifo_rev_fifo);
#else
    cycle_model_inst = cycle_model::cycle_model::create(get_charfifo_send_fifo(), get_charfifo_rev_fifo());
#endif
#endif
    
    for(auto item : arg.breakpoint_list)
    {
        breakpoint_add(item.type, item.value);
    }
}

void load(std::shared_ptr<uint8_t[]>buf, size_t size)
{
    load_program = true;
    program_buffer = buf;
    program_size = size;
    
#ifdef NEED_ISA_MODEL
    isa_model_inst->load(buf.get(), size);
    #endif
#ifdef NEED_CYCLE_MODEL
    cycle_model_inst->reset();
    cycle_model_inst->load(buf.get(), size);
#endif
}

void reset()
{
#ifdef NEED_ISA_MODEL
    isa_model_inst->reset();
#endif
#ifdef NEED_CYCLE_MODEL
    cycle_model_inst->reset();
#endif
    
    if(load_program)
    {
        load(program_buffer, program_size);
    }
}

static void pause_event()
{
#ifdef NEED_ISA_MODEL
    isa_model_inst->pause_event();
#endif
}

static void run(const command_line_arg_t &arg)
{
#ifdef NEED_CYCLE_MODEL
    uint32_t last_retire_cycle = 0;
#endif
#ifdef NEED_ISA_MODEL
#ifdef NDEBUG
    /*isa_model_inst->profile(INIT_PC);
    reset();*/
#endif
#endif
    while(true)
    {
        if(arg.no_controller && !arg.no_telnet)
        {
            while(get_charfifo_recv_thread_stopped())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
        
#ifdef NEED_CYCLE_MODEL
        uint32_t breakpoint_pc[FETCH_WIDTH];
        uint32_t pc_num = 0;
        
        {
            auto item = cycle_model_inst->fetch1_fetch2_port.get();
            
            for(uint32_t i = 0;i < FETCH_WIDTH;i++)
            {
                if(item.op_info[i].enable)
                {
                    breakpoint_pc[pc_num++] = item.op_info[i].pc;
                }
            }
        }
        
        if(breakpoint_check(cycle_model_inst->cpu_clock_cycle, cycle_model_inst->committed_instruction_num, breakpoint_pc, pc_num))
#else
        if(breakpoint_check(isa_model_inst->cpu_clock_cycle, isa_model_inst->committed_instruction_num, &isa_model_inst->pc, 1))
#endif
        {
            set_pause_detected(true);
            send_cmd("main", "breakpoint_trigger", "");
            auto finish_value = breakpoint_get_finish();
            
            if(finish_value.has_value())
            {
                if(finish_value.value() > 0)
                {
                    std::cout << std::endl << MESSAGE_OUTPUT_PREFIX "Program exit signal is detected, result is success!" << std::endl;
    
                    if(arg.no_controller)
                    {
                        exit(EXIT_CODE_CSR_FINISH_OK_DETECTED);
                    }
                }
                else
                {
                    std::cout << std::endl << MESSAGE_OUTPUT_PREFIX "Program exit signal is detected, result is failure!" << std::endl;
    
                    if(arg.no_controller)
                    {
                        exit(EXIT_CODE_CSR_FINISH_FAILURE_DETECTED);
                    }
                }
            }
            else if(arg.no_controller)
            {
                std::cout << std::endl << MESSAGE_OUTPUT_PREFIX "Breakpoint is detected, but current mode is no_controller, so the program will exit!" << std::endl;
                exit(EXIT_CODE_OTHER_BREAKPOINT_DETECTED);
            }
        }
#ifdef NEED_CYCLE_MODEL
        if(cycle_model_inst->rob.get_committed())
        {
            last_retire_cycle = cycle_model_inst->cpu_clock_cycle;
        }
        
        if(pause_detected || (step_state && (!wait_commit || cycle_model_inst->rob.get_committed())))
#else
        if(pause_detected || step_state)
#endif
        {
            if(pause_detected)
            {
                step_state = true;
                wait_commit = false;
            }
            
            pause_state = true;
            pause_event();
            
            while(pause_state)
            {
                debug_event_handle();
#ifdef NEED_ISA_MODEL
                clear_queue(isa_model_inst->bus.error_msg_queue);
#endif
#ifdef NEED_CYCLE_MODEL
                clear_queue(cycle_model_inst->bus.error_msg_queue);
                
                if(last_retire_cycle > cycle_model_inst->cpu_clock_cycle)
                {
                    last_retire_cycle = cycle_model_inst->cpu_clock_cycle;
                }
#endif
                
                if(get_program_stop())
                {
                    break;
                }
                
                pause_detected = false;
            }
        }
        
        if(get_program_stop())
        {
            break;
        }

#ifdef NEED_CYCLE_MODEL
        auto model_cycle = cycle_model_inst->cpu_clock_cycle;
        auto model_pc = get_current_pc();
        //std::cout << "cycle = " << model_cycle << ", pc = 0x" << outhex(model_pc) << std::endl;
        cycle_model_inst->rob.set_committed(false);
        cycle_model_inst->run();
        
#ifdef NEED_ISA_MODEL
        auto compare_error = false;
        std::vector<std::pair<uint32_t, cycle_model::component::rob_item_t>> compare_error_item;
        
        while(!cycle_model_inst->commit_stage.rob_retire_queue.empty())
        {
            auto rob_item = cycle_model_inst->commit_stage.rob_retire_queue.front();
            cycle_model_inst->commit_stage.rob_retire_queue.pop();
            
            if(compare_error)
            {
                //pop csr item only
                if(!cycle_model_inst->execute_csr_stage[0]->csr_read_queue.empty())
                {
                    auto csr_item = cycle_model_inst->execute_csr_stage[0]->csr_read_queue.front();
    
                    if(csr_item.rob_id == rob_item.first)
                    {
                        cycle_model_inst->execute_csr_stage[0]->csr_read_queue.pop();
                    }
                }
                
                continue;
            }
            else
            {
                compare_error_item.push_back(rob_item);
            }
            
            //hpmcounter sync
            if(!cycle_model_inst->execute_csr_stage[0]->csr_read_queue.empty())
            {
                auto csr_item = cycle_model_inst->execute_csr_stage[0]->csr_read_queue.front();
                
                if(csr_item.rob_id == rob_item.first)
                {
                    cycle_model_inst->execute_csr_stage[0]->csr_read_queue.pop();
                    std::vector<uint32_t> csr_sync_list = {CSR_MCYCLE, CSR_MCYCLEH, CSR_MINSTRET, CSR_MINSTRETH,
                                                             CSR_BRANCHNUM, CSR_BRANCHNUMH, CSR_BRANCHPREDICTED, CSR_BRANCHPREDICTEDH,
                                                             CSR_BRANCHHIT, CSR_BRANCHHITH, CSR_BRANCHMISS, CSR_BRANCHMISSH, CSR_MIP};
                    
                    for(auto csr_id : csr_sync_list)
                    {
                        if(csr_item.csr == csr_id)
                        {
                            isa_model_inst->csr_file.write_sys(csr_id, csr_item.value);
                            break;
                        }
                    }
                }
            }
            
            if(!compare_error)
            {
                if(isa_model_inst->pc != rob_item.second.pc)
                {
                    compare_error = true;
                    std::cout << MESSAGE_OUTPUT_PREFIX "PC is not match, cycle model pc is 0x" << outhex(rob_item.second.pc) << ", isa model pc is 0x" << outhex(isa_model_inst->pc) << std::endl;
                }
                
                //clint sync only for the instruction
                auto item = cycle_model_inst->execute_lsu_stage[0]->clint_sync_list[rob_item.first];
                isa_model_inst->clint.set_mtime(item.mtime);
                isa_model_inst->clint.set_mtimecmp(item.mtimecmp);
                isa_model_inst->clint.set_msip(item.msip);
                
                isa_model_inst->run();
            }
            
            if(!cycle_model_charfifo_send_fifo.empty())
            {
                auto v = cycle_model_charfifo_send_fifo.front();
                while(!get_charfifo_send_fifo()->push(v));
                while(!cycle_model_charfifo_send_fifo.pop());
            }
            
            if(!isa_model_charfifo_send_fifo.empty())
            {
                while(!isa_model_charfifo_send_fifo.pop());
            }
        }
    
        //clint sync
        isa_model_inst->clint.set_mtime(cycle_model_inst->clint.get_mtime());
        isa_model_inst->clint.set_mtimecmp(cycle_model_inst->clint.get_mtimecmp());
        isa_model_inst->clint.set_msip(cycle_model_inst->clint.get_msip());
        isa_model_inst->clint.run();
    
        //interrupt sync
        isa_model_inst->interrupt_interface.set_meip(cycle_model_inst->interrupt_interface.get_meip());
        isa_model_inst->interrupt_interface.set_msip(cycle_model_inst->interrupt_interface.get_msip());
        isa_model_inst->interrupt_interface.set_mtip(cycle_model_inst->interrupt_interface.get_mtip());
        isa_model_inst->interrupt_sync(cycle_model_inst->commit_feedback_pack.has_interrupt, cycle_model_inst->commit_feedback_pack.interrupt_id);
        
        //mip sync
        isa_model::csr_inst::mip.write(cycle_model_inst->csr_file.read_sys(CSR_MIP));
    
        std::vector<uint32_t> compare_csr_list = {CSR_MVENDORID, CSR_MARCHID, CSR_MIMPID, CSR_MHARTID, CSR_MCONFIGPTR, CSR_MSTATUS, CSR_MISA, CSR_MIE,
                                                  CSR_MTVEC, CSR_MCOUNTEREN, CSR_MSTATUSH, CSR_MSCRATCH, CSR_MEPC, CSR_MCAUSE, CSR_MTVAL, CSR_MIP,
                                                  CSR_FINISH, CSR_MINSTRET, CSR_MINSTRETH};
    
        for(uint32_t i = 1;i < ARCH_REG_NUM;i++)
        {
            uint32_t phy_id = 0;
            cycle_model_inst->retire_rat.customer_get_phy_id(i, &phy_id);
            auto v = cycle_model_inst->phy_regfile.read(phy_id);
        
            if(!cycle_model_inst->phy_regfile.read_data_valid(phy_id))
            {
                std::cout << MESSAGE_OUTPUT_PREFIX << "Error: GPR " << i << " is not valid!" << std::endl;
                compare_error = true;
            }
            else
            {
                auto v2 = isa_model_inst->arch_regfile.read(i);
            
                if(v != v2)
                {
                    std::cout << MESSAGE_OUTPUT_PREFIX << "Error: GPR " << i << " is not consistent, cycle model value is 0x" << outhex(v) << ", isa model value is 0x" << outhex(v2) << std::endl;
                    compare_error = true;
                }
            }
        }
    
        for(auto csr_id : compare_csr_list)
        {
            auto v = cycle_model_inst->csr_file.read_sys(csr_id);
            auto v2 = isa_model_inst->csr_file.read_sys(csr_id);
        
            if(v != v2)
            {
                std::cout << MESSAGE_OUTPUT_PREFIX << "Error: CSR " << isa_model_inst->csr_file.get_name(csr_id) << " is not consistent, cycle model value is 0x" << outhex(v) << ", isa model value is 0x" << outhex(v2) << std::endl;
                compare_error = true;
            }
        }
    
        if(isa_model_charfifo_send_fifo.empty() != cycle_model_charfifo_send_fifo.empty())
        {
            std::cout << MESSAGE_OUTPUT_PREFIX << "Error: charfifo send fifo empty property is not consistent, cycle model is " << cycle_model_charfifo_send_fifo.empty() << ", isa model is " << isa_model_charfifo_send_fifo.empty() << std::endl;
            compare_error = true;
        }
        else if(!cycle_model_charfifo_send_fifo.empty())
        {
            auto v = cycle_model_charfifo_send_fifo.front();
            auto v2 = isa_model_charfifo_send_fifo.front();
        
            if(v != v2)
            {
                std::cout << MESSAGE_OUTPUT_PREFIX << "Error: charfifo send fifo front value is not consistent, cycle model is 0x" << outhex(v) << ", isa model is 0x" << outhex(v2) << std::endl;
                compare_error = true;
            }
        }
    
        //dump compare result
        if(compare_error)
        {
            std::cout << MESSAGE_OUTPUT_PREFIX "cycle = " << model_cycle << ", pc = 0x" << outhex(model_pc) << " compare error detected!" << std::endl;
            set_pause_detected(true);
            send_cmd("main", "breakpoint_trigger", "");
        
            for(const auto &item : compare_error_item)
            {
                auto rob_id = item.first;
                auto rob_item = item.second;
            
                std::cout << MESSAGE_OUTPUT_PREFIX << "rob_id = " << rob_id << ", pc = 0x" << outhex(rob_item.pc) << ", inst = 0x" << outhex(rob_item.inst_value) << std::endl;
            }
            
            compare_error_item.clear();
            std::cout << MESSAGE_OUTPUT_PREFIX << "Compare Result:" << std::endl;
            std::cout << MESSAGE_OUTPUT_PREFIX << "Cycle Model\tISA Model" << std::endl;
        
            for(uint32_t i = 1;i < ARCH_REG_NUM;i++)
            {
                std::cout << MESSAGE_OUTPUT_PREFIX << "x" << i << " = ";
                uint32_t phy_id = 0;
                cycle_model_inst->retire_rat.customer_get_phy_id(i, &phy_id);
                auto v = cycle_model_inst->phy_regfile.read(phy_id);
            
                if(!cycle_model_inst->phy_regfile.read_data_valid(phy_id))
                {
                    std::cout << "<Invalid>\t";
                }
                else
                {
                    std::cout << "0x" << outhex(v) << "\t";
                }
            
                std::cout << "0x" << outhex(isa_model_inst->arch_regfile.read(i)) << std::endl;
            }
        
            for(auto csr_id : compare_csr_list)
            {
                auto v = cycle_model_inst->csr_file.read_sys(csr_id);
                auto v2 = isa_model_inst->csr_file.read_sys(csr_id);
                std::cout << MESSAGE_OUTPUT_PREFIX << "CSR[" << isa_model_inst->csr_file.get_name(csr_id) << "] = 0x" << outhex(v) << "\t0x" << outhex(v2) << std::endl;
            }
    
            if(arg.no_controller)
            {
                std::cout << std::endl << MESSAGE_OUTPUT_PREFIX "Current mode is no_controller, so the program will exit!" << std::endl;
                exit(EXIT_CODE_OTHER_BREAKPOINT_DETECTED);
            }
        }
        
        while(!isa_model_inst->bus.error_msg_queue.empty())
        {
            isa_model_inst->bus.error_msg_queue.pop();
        }
#endif
    
        if(!cycle_model_inst->bus.error_msg_queue.empty())
#else
        auto model_cycle = isa_model_inst->cpu_clock_cycle;
        auto model_pc = isa_model_inst->pc;
        isa_model_inst->run();
        
        if(!isa_model_inst->bus.error_msg_queue.empty())
#endif
        {
            std::cout << MESSAGE_OUTPUT_PREFIX "cycle = " << model_cycle << ", pc = 0x" << outhex(model_pc) << " error detected!" << std::endl;
            set_pause_detected(true);
            send_cmd("main", "breakpoint_trigger", "");

#ifdef NEED_CYCLE_MODEL
            while(!cycle_model_inst->bus.error_msg_queue.empty())
            {
                auto msg = cycle_model_inst->bus.error_msg_queue.front();
                cycle_model_inst->bus.error_msg_queue.pop();
#else
            while(!isa_model_inst->bus.error_msg_queue.empty())
            {
                auto msg = isa_model_inst->bus.error_msg_queue.front();
                isa_model_inst->bus.error_msg_queue.pop();
#endif
                
                if(msg.is_fetch)
                {
                    std::cout << MESSAGE_OUTPUT_PREFIX "instruction fetch error: ";
                }
                
                if(msg.is_write)
                {
                    std::cout << MESSAGE_OUTPUT_PREFIX "memory write error: ";
                    std::cout << MESSAGE_OUTPUT_PREFIX "address = 0x" << outhex(msg.addr) << ", size = " << msg.size << ", value = 0x" << outhex(msg.value) << std::endl;
                }
                else
                {
                    if(!msg.is_fetch)
                    {
                        std::cout << MESSAGE_OUTPUT_PREFIX "memory read error: ";
                    }
                    
                    std::cout << MESSAGE_OUTPUT_PREFIX "address = 0x" << outhex(msg.addr) << ", size = " << msg.size << std::endl;
                }
            }
        
            if(arg.no_controller)
            {
                std::cout << std::endl << MESSAGE_OUTPUT_PREFIX "Current mode is no_controller, so the program will exit!" << std::endl;
                exit(EXIT_CODE_OTHER_BREAKPOINT_DETECTED);
            }
        }
        
#ifdef NEED_CYCLE_MODEL
        if(cycle_model_inst->cpu_clock_cycle - last_retire_cycle >= 100)
        {
            std::cout << MESSAGE_OUTPUT_PREFIX << "Error: Retire Timeout, last instruction retire cycle is " << last_retire_cycle << "!" << std::endl;
            last_retire_cycle = cycle_model_inst->cpu_clock_cycle;
            set_pause_detected(true);
    
            if(arg.no_controller)
            {
                std::cout << std::endl << MESSAGE_OUTPUT_PREFIX "Current mode is no_controller, so the program will exit!" << std::endl;
                exit(EXIT_CODE_OTHER_BREAKPOINT_DETECTED);
            }
            else
            {
                send_cmd("main", "breakpoint_trigger", "");
            }
        }
#endif
    }
}

static void load_bin_file(std::string path)
{
    std::ifstream binfile(path, std::ios::binary);
    
    if(!binfile || !binfile.is_open())
    {
        std::cout << MESSAGE_OUTPUT_PREFIX << path << " Open Failed!" << std::endl;
        exit(EXIT_CODE_ERROR);
    }
    
    binfile.seekg(0, std::ios::end);
    auto size = binfile.tellg();
    std::cout << MESSAGE_OUTPUT_PREFIX "File Size: " << size << " Byte(s)" << std::endl;
    binfile.seekg(0, std::ios::beg);
    auto buf = std::shared_ptr<uint8_t[]>(new uint8_t[size]);
    binfile.read((char *)buf.get(), size);
    binfile.close();
    load(buf, size);
    std::cout << MESSAGE_OUTPUT_PREFIX << path << " Load OK!" << std::endl;
}

static void load_elf_file(std::string path)
{
    auto ret = load_elf(path);
    load(ret.first, ret.second);
    std::cout << MESSAGE_OUTPUT_PREFIX << path << " Load OK!" << std::endl;
}

static void sub_main(const command_line_arg_t &arg)
{
    init(arg);
    reset();
    
    if(arg.load_elf)
    {
        load_elf_file(arg.path);
    }
    else if(arg.load_bin)
    {
        load_bin_file(arg.path);
    }
    else
    {
        load_bin_file("../../../image/rtthread.bin");
        //load_bin_file("../../../image/coremark_10.bin");
        //load_bin_file("../../../testcase/benchmark/coremark_10_7_2.bin");
        //load_elf_file("../../../testcase/riscv-tests/rv32ui-p-fence_i");
        //load_bin_file("../../../testcase/base-tests/ipc_test2_12_2.bin");
        //load_elf_file("../../../testcase/compiled/rtthread.elf");
    }
    
    if(!arg.no_controller)
    {
        set_pause_detected(true);
    }
    
    run(arg);
}

static void command_line_parse(int argc, char **argv, command_line_arg_t &arg)
{
    try
    {
        TCLAP::CmdLine cmd("DreamCoreV2 Model by LiZhirui", ' ', "none", true);
        TCLAP::SwitchArg no_controller_arg("", "nocontroller", "No Controller", cmd, false);
        TCLAP::SwitchArg no_telnet_arg("", "notelnet", "No Telnet", cmd, false);
        TCLAP::ValueArg<std::string> load_elf_arg("", "loadelf", "Load ELF Execution File", false, "", "path", cmd);
        TCLAP::ValueArg<std::string> load_bin_arg("", "loadbin", "Load Binary Execution File", false, "", "path", cmd);
        TCLAP::ValueArg<uint32_t> controller_port_arg("", "controllerport", "Controller Port", false, 10240, "port", cmd);
        TCLAP::ValueArg<uint32_t> telnet_port_arg("", "telnetport", "Telnet Port", false, 10241, "port", cmd);
        TCLAP::MultiArg<std::string> breakpoint_arg("b", "breakpoint", "Breakpoint", false, "[cycle|inst|pc|csrw]:value", cmd);
        
        cmd.parse(argc, argv);
        
        if(load_elf_arg.isSet() && load_bin_arg.isSet())
        {
            std::cout << MESSAGE_OUTPUT_PREFIX << "Error: Can't set both ELF and Binary Execution File!" << std::endl;
            exit(EXIT_CODE_ERROR);
        }
        
        arg.no_controller = no_controller_arg.getValue();
        arg.no_telnet = no_telnet_arg.getValue();
        arg.load_elf = load_elf_arg.isSet();
        arg.load_bin = load_bin_arg.isSet();
        arg.path = arg.load_elf ? load_elf_arg.getValue() : load_bin_arg.getValue();
        arg.controller_port = controller_port_arg.getValue();
        arg.telnet_port = telnet_port_arg.getValue();
        
        for(auto iter = breakpoint_arg.begin(); iter != breakpoint_arg.end(); iter++)
        {
            std::stringstream stream(*iter);
            std::vector<std::string> arg_list;
            std::string token;
            
            while(getline(stream, token, ':'))
            {
                arg_list.push_back(token);
            }
            
            if(arg_list.size() != 2)
            {
                std::cout << MESSAGE_OUTPUT_PREFIX << "Invalid breakpoint argument: " << *iter << std::endl;
                exit(EXIT_CODE_ERROR);
            }
    
            auto type = arg_list[0];
            auto value = get_multi_radix_num(arg_list[1]);
            breakpoint_info_t info;
    
            if(type == "cycle")
            {
                info.type = breakpoint_type_t::cycle;
            }
            else if(type == "inst")
            {
                info.type = breakpoint_type_t::instruction_num;
            }
            else if(type == "pc")
            {
                info.type = breakpoint_type_t::pc;
            }
            else if(type == "csrw")
            {
                info.type = breakpoint_type_t::csrw;
            }
            else
            {
                std::cout << MESSAGE_OUTPUT_PREFIX << " breakpoint type error: " << type << std::endl;
            }
    
            info.value = value;
            arg.breakpoint_list.push_back(info);
        }
    }
    catch(TCLAP::ArgException &e)
    {
        std::cout << MESSAGE_OUTPUT_PREFIX << "Error: " << e.error() << " for arg " << e.argId() << std::endl;
        exit(EXIT_CODE_ERROR);
    }
}

static void show_copyright()
{
    std::cout << std::endl;
    std::cout << "***********************************************" << std::endl;
    std::cout << "        DreamCore V2 Model by LiZhirui" << std::endl;
    std::cout << std::endl;
#if MODE == MODE_ISA_MODEL_ONLY
    std::cout << "           Cur Mode: ISA Model Only" << std::endl;
#elif MODE == MODE_CYCLE_MODEL_ONLY
    std::cout << "         Cur Mode: Cycle Model Only" << std::endl;
#else
    std::cout << "     Cur Mode: ISA and Cycle Model Compare" << std::endl;
#endif
    std::cout << "***********************************************" << std::endl;
}

static void print_time()
{
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    std::cout << MESSAGE_OUTPUT_PREFIX << "Time: " << std::ctime(&now_time_t) << std::endl;
}

static void atexit_func()
{
    std::cout << std::endl;
    std::cout << "***********************************************" << std::endl;
    print_time();
#ifdef NEED_CYCLE_MODEL
    if(cycle_model_inst != nullptr)
    {
        std::cout << MESSAGE_OUTPUT_PREFIX << "Cycle: " << cycle_model_inst->cpu_clock_cycle << std::endl;
        std::cout << MESSAGE_OUTPUT_PREFIX << "Instruction Num: " << cycle_model_inst->committed_instruction_num << std::endl;
        std::cout << MESSAGE_OUTPUT_PREFIX << "IPC: " << ((long double)cycle_model_inst->committed_instruction_num) / cycle_model_inst->cpu_clock_cycle << std::endl;
        std::cout << MESSAGE_OUTPUT_PREFIX << "PC: 0x" << outhex(get_current_pc()) << std::endl;
    }
#else
    if(isa_model_inst != nullptr)
    {
        std::cout << "Cycle: " << isa_model_inst->cpu_clock_cycle << std::endl;
        std::cout << "Instruction Num: " << isa_model_inst->committed_instruction_num << std::endl;
        std::cout << "PC: 0x" << outhex(get_current_pc()) << std::endl;
    }
#endif
    std::cout << "***********************************************" << std::endl;
}

int main(int argc, char **argv)
{
    print_time();
    std::cout.setf(std::ios::unitbuf);
    std::cout << MESSAGE_OUTPUT_PREFIX << "main thread tid: " << gettid() << std::endl;
    std::atexit(atexit_func);
    show_copyright();
    command_line_arg_t arg;
    command_line_parse(argc, argv, arg);
    sub_main(arg);
    while((!arg.no_controller && !get_server_thread_stopped()) || (!arg.no_telnet && !get_charfifo_thread_stopped()));
    return EXIT_CODE_OK;
}