#include "common.h"
#include "config.h"
#include "network/network.h"
#include "main.h"
#include "breakpoint.h"
#include "elf_loader.h"

#if NEED_ISA_MODEL
#include "isa_model/isa_model.h"
isa_model::isa_model *isa_model_inst;
#endif

#if NEED_CYCLE_MODEL
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
#if NEED_CYCLE_MODEL
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
#if NEED_ISA_MODEL
    isa_model_inst = isa_model::isa_model::create(get_charfifo_send_fifo(), get_charfifo_rev_fifo());
#endif
#if NEED_CYCLE_MODEL
    cycle_model_inst = cycle_model::cycle_model::create(get_charfifo_send_fifo(), get_charfifo_rev_fifo());
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
    
#if NEED_ISA_MODEL
    isa_model_inst->load(buf.get(), size);
    #endif
#if NEED_CYCLE_MODEL
    cycle_model_inst->reset();
    cycle_model_inst->load(buf.get(), size);
#endif
}

void reset()
{
#if NEED_ISA_MODEL
    isa_model_inst->reset();
#endif
#if NEED_CYCLE_MODEL
    cycle_model_inst->reset();
#endif
    
    if(load_program)
    {
        load(program_buffer, program_size);
    }
}

static void pause_event()
{
#if NEED_ISA_MODEL
    isa_model_inst->pause_event();
#endif
}

static void run(const command_line_arg_t &arg)
{
    while(true)
    {
        if(arg.no_controller && !arg.no_telnet)
        {
            if(get_charfifo_recv_thread_stopped())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
        
#if NEED_CYCLE_MODEL

#else
        if(breakpoint_check(isa_model_inst->cpu_clock_cycle, isa_model_inst->committed_instruction_num, isa_model_inst->pc))
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
#endif
#if NEED_CYCLE_MODEL
        if(pause_state || (step_state && (!wait_commit || cycle_model_inst->rob.get_committed())))
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

#if NEED_CYCLE_MODEL
        cycle_model_inst->rob.set_committed(false);
        cycle_model_inst->run();
#else
        auto isa_model_cycle = isa_model_inst->cpu_clock_cycle;
        auto isa_model_pc = isa_model_inst->pc;
        clear_queue(isa_model_inst->bus.error_msg_queue);
        isa_model_inst->run();
        
        if(!isa_model_inst->bus.error_msg_queue.empty())
        {
            std::cout << MESSAGE_OUTPUT_PREFIX "cycle = " << isa_model_cycle << ", pc = " << outhex(isa_model_pc) << " error detected!" << std::endl;
            set_pause_detected(true);
            
            while(!isa_model_inst->bus.error_msg_queue.empty())
            {
                auto msg = isa_model_inst->bus.error_msg_queue.front();
                isa_model_inst->bus.error_msg_queue.pop();
                
                if(msg.is_fetch)
                {
                    std::cout << MESSAGE_OUTPUT_PREFIX "instruction fetch error: ";
                }
                
                if(msg.is_write)
                {
                    std::cout << MESSAGE_OUTPUT_PREFIX "memory write error: ";
                    std::cout << MESSAGE_OUTPUT_PREFIX "address = " << outhex(msg.addr) << ", size = " << msg.size << ", value = " << outhex(msg.value) << std::endl;
                }
                else
                {
                    if(!msg.is_fetch)
                    {
                        std::cout << MESSAGE_OUTPUT_PREFIX "memory read error: ";
                    }
                    
                    std::cout << MESSAGE_OUTPUT_PREFIX "address = " << outhex(msg.addr) << ", size = " << msg.size << std::endl;
                }
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
        //load_bin_file("../../../image/rtthread.bin");
        load_bin_file("../../../image/coremark_10.bin");
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

static void atexit_func()
{
    std::cout << std::endl;
    std::cout << "***********************************************" << std::endl;
#if NEED_CYCLE_MODEL
    if(cycle_model_inst != nullptr)
    {
        std::cout << "Cycle: " << cycle_model_inst->cpu_clock_cycle << std::endl;
        std::cout << "Instruction Num: " << cycle_model_inst->committed_instruction_num << std::endl;
        std::cout << "PC: 0x" << outhex(get_current_pc()) << std::endl;
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
    std::cout << MESSAGE_OUTPUT_PREFIX << "main thread tid: " << gettid() << std::endl;
    std::atexit(atexit_func);
    show_copyright();
    command_line_arg_t arg;
    command_line_parse(argc, argv, arg);
    sub_main(arg);
    while((!arg.no_controller && !get_server_thread_stopped()) || (!arg.no_telnet && !get_charfifo_thread_stopped()));
    return EXIT_CODE_OK;
}