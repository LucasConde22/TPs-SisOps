// proceso para generar carga en cpu
#include <inc/lib.h>

void
umain(int argc, char **argv)
{
    cprintf("My ID is %08x and I'm designed to bother!\n", thisenv->env_id);
    sys_yield();
    for (int i = 0; i < 50; i++) {
        for (int j = 0; j < 500000; j++) {
            int x = 2 * i + i*i*i;
        }
        cprintf("%08x bothering here!\n", thisenv->env_id);
        sys_yield();
    }
}
