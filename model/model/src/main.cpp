#include "common.h"
#include "config.h"
#include "network/network.h"
#include "main.h"
#include "breakpoint.h"

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

static void init()
{
    network_init();
#if NEED_ISA_MODEL
    isa_model_inst = isa_model::isa_model::create(get_charfifo_send_fifo(), get_charfifo_rev_fifo());
#endif
#if NEED_CYCLE_MODEL
    cycle_model_inst = cycle_model::cycle_model::create(get_charfifo_send_fifo(), get_charfifo_rev_fifo());
#endif
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

static void run()
{
    while(true)
    {
#if NEED_CYCLE_MODEL

#else
        if(breakpoint_check(isa_model_inst->cpu_clock_cycle, isa_model_inst->committed_instruction_num, isa_model_inst->pc))
        {
            set_pause_detected(true);
            send_cmd("main", "breakpoint_trigger", "");
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
            std::cout << "cycle = " << isa_model_cycle << ", pc = " << outhex(isa_model_pc) << " error detected!" << std::endl;
            set_pause_detected(true);
            
            while(!isa_model_inst->bus.error_msg_queue.empty())
            {
                auto msg = isa_model_inst->bus.error_msg_queue.front();
                isa_model_inst->bus.error_msg_queue.pop();
                
                if(msg.is_fetch)
                {
                    std::cout << "instruction fetch error: ";
                }
                
                if(msg.is_write)
                {
                    std::cout << "memory write error: ";
                    std::cout << "address = " << outhex(msg.addr) << ", size = " << msg.size << ", value = " << outhex(msg.value) << std::endl;
                }
                else
                {
                    if(!msg.is_fetch)
                    {
                        std::cout << "memory read error: ";
                    }
                    
                    std::cout << "address = " << outhex(msg.addr) << ", size = " << msg.size << std::endl;
                }
            }
        }
#endif
    }
}

static void load_file(std::string path)
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
    load(buf, size);
    std::cout << path << " Load OK!" << std::endl;
}

static void sub_main()
{
    init();
    reset();
    set_pause_detected(true);
    load_file("../../../image/rtthread.bin");
    run();
}

int main(int argc, char **argv)
{
    sub_main();
    while(!get_server_thread_stopped() || !get_charfifo_thread_stopped());
    return 0;
}