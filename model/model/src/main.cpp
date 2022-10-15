#include <common.h>
#include <config.h>
#include <network/network.h>

static void init()
{
    network_init();

}

static void reset()
{

}

static void run()
{

}

static void sub_main()
{
    init();
    reset();

}

int main(int argc, char **argv)
{
    sub_main();
}