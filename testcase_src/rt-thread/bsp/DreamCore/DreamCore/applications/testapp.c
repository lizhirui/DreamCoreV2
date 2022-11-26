#include <rtthread.h>

static rt_thread_t thread_a;
static rt_thread_t thread_b;
static rt_thread_t thread_c;
static rt_sem_t finish_sem;

void thread_a_entry(void *arg)
{
    int i;

    for(i = 0;i < 10;i++)
    {
        rt_kprintf("[%d]Test rt__kprintf-a\n",i);
        rt_thread_mdelay(1);
    }

    rt_sem_release(finish_sem);
}

void thread_b_entry(void *arg)
{
    int i;

    for(i = 0;i < 10;i++)
    {
        rt_kprintf("[%d]Test rt__kprintf-b\n",i);
        rt_thread_mdelay(10);
    }

    rt_sem_release(finish_sem);
}

void thread_c_entry(void *arg)
{
    int i;

    for(i = 0;i < 10;i++)
    {
        rt_kprintf("[%d]Test rt__kprintf-c\n",i);
        rt_thread_mdelay(5);
    }

    rt_sem_release(finish_sem);
}

void set_finish(uint32_t v);

long testapp()
{
    uint32_t i;
    thread_a = rt_thread_create("thread_a", thread_a_entry, RT_NULL, 4096, 10, 10);
    thread_b = rt_thread_create("thread_b", thread_b_entry, RT_NULL, 4096, 10, 10);
    thread_c = rt_thread_create("thread_c", thread_c_entry, RT_NULL, 4096, 10, 10);
    finish_sem = rt_sem_create("finish_sem", 0, RT_IPC_FLAG_FIFO);
    rt_thread_startup(thread_a);
    rt_thread_startup(thread_b);
    rt_thread_startup(thread_c);
    
    for(i = 0;i < 3;i++)
    {
        rt_sem_take(finish_sem, RT_WAITING_FOREVER);
    }

    rt_sem_delete(finish_sem);
    rt_kprintf("testapp finished!");
    set_finish(1);
    return 0;
}
MSH_CMD_EXPORT(testapp, testapp);