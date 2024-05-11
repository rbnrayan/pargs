#include "pargs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void pargs_panic(const char *msg)
{
    fprintf(stderr, "PARGS FAILED: %s.\n", msg);
    exit(EXIT_FAILURE);
}

static char *shift_args(int *argc, char ***argv)
{
    if (argc <= 0)
      return NULL;

    char *arg = **argv;
    *argc -= 1;
    *argv += 1;
    return arg;
}

static const char *peek_args(const int argc, const char **argv)
{
    if (argc <= 0)
      return NULL;
    return *argv;
}

static enum P_ArgsType parse_option_type(const char *opt_name)
{
    if (opt_name == NULL)
        return EOA;

    size_t len = strlen(opt_name);
    if (len == 2 && strcmp("--", opt_name) == 0)
        return EOA;
    else if (len > 2 && (opt_name[0] == '-' && opt_name[1] == '-'))
        return LONG_NAME;
    else if (len > 1 && opt_name[0] == '-')
        return SHORT_NAME;
    else 
        return VALUE;
}

static struct P_ArgsOption parse_next_option(int *argc, char ***argv)
{
    struct P_ArgsOption opt = { 0 };

    char *opt_name = shift_args(argc, argv);
    enum P_ArgsType opt_type = parse_option_type(opt_name);

    opt.type = opt_type;
    switch (opt.type) {
    case LONG_NAME:
        opt.name.long_name = (opt_name + 2);
        break;
    case SHORT_NAME:
        opt.name.short_name = *(opt_name + 1);
        break;
    case EOA:
        return opt;
    case VALUE:
        pargs_panic("Failed to parse option");
    }

    const char *peek_value = peek_args(*argc, (const char **) *argv);
    if (parse_option_type(peek_value) != VALUE) {
        return opt;
    }

    char *opt_value = shift_args(argc, argv);
    opt.value = opt_value;

    return opt;
}

P_Args *pargs_parse(int *argc, char ***argv)
{
    P_Args *pargs = malloc(sizeof(P_Args));
    memset(pargs->options, 0, PARGS_CAPACITY);
    pargs->size = 0;

    // shift program name.
    shift_args(argc, argv);

    struct P_ArgsOption next_opt;
    while ((next_opt = parse_next_option(argc, argv)).type != EOA) {
        pargs->options[pargs->size] = next_opt;
        pargs->size += 1;
    }

    return pargs;
}

const char *pargs_getl(P_Args *pargs, const char *name)
{
    for (size_t i = 0; i < pargs->size; ++i) {
        if (pargs->options[i].type != LONG_NAME)
            continue;
        
        const struct P_ArgsOption option = pargs->options[i];
        if (strcmp(name, option.name.long_name) == 0) {
            return option.value;
        }
    }
    return NULL;
}

const char *pargs_gets(P_Args *pargs, char name)
{
    for (size_t i = 0; i < pargs->size; ++i) {
        if (pargs->options[i].type != SHORT_NAME)
            continue;

        const struct P_ArgsOption option = pargs->options[i];
        if (option.name.short_name == name) {
            return option.value;
        }
    }
    return NULL;
}