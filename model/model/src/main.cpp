#include "common.h"
#include "config.h"
#include "network/network.h"
#include "main.h"

static std::atomic<bool> pause_detected = false;
static bool pause_state = false;
static bool step_state = false;
static bool wait_commit = false;

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

static uint32_t get_current_pc()
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

void reset()
{
#if NEED_ISA_MODEL
    isa_model_inst->reset();
#endif
#if NEED_CYCLE_MODEL
    cycle_model_inst->reset();
#endif
}

static void run()
{
    while(true)
    {
#if NEED_CYCLE_MODEL
        if(pause_state || (step_state && (!wait_commit || cycle_model_inst->rob.get_committed())))
#else
        if(pause_state || step_state)
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
        isa_model_inst->run();
#endif
    }
}

static void sub_main()
{
    init();
    reset();
    run();
}

int main(int argc, char **argv)
{
    sub_main();
    while(!get_server_thread_stopped() || !get_charfifo_thread_stopped());
    return 0;
}