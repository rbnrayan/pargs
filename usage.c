#define PARGS_CAPACITY 8
#include "pargs.h"

#include <stdio.h>

int main(int argc, char *argv[])
{
    P_Args *pargs = pargs_parse(&argc, &argv);

    const char *test_value = pargs_getl(pargs, "test");
    printf("--test = %s\n", test_value);

    test_value = pargs_gets(pargs, 't');
    printf("-t = %s\n", test_value);

    pargs_free(pargs);
    return 0;
}
