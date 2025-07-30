// priorities
#include <inc/lib.h>

void
umain(int argc, char **argv)
{
    for (int i = 0; i < 70; i++) {
        cprintf("I'm %08x and my current priority is %d\n", thisenv->env_id, sys_get_priority());
        sys_yield();
    }
}
