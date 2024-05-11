#ifndef __PARGS__
#define __PARGS__

/**
 * Possibility to re-define options[] capacity before
 * header import.
 */
#ifndef PARGS_CAPACITY
#   define PARGS_CAPACITY 128
#endif

#include <stdlib.h>

#define pargs_free(pargs) free(pargs)

enum P_ArgsType {
    LONG_NAME,
    SHORT_NAME,
    VALUE,
    EOA
};

struct P_ArgsOption {
  enum P_ArgsType type;
  union {
    char *long_name;
    char short_name;
  } name;
  char *value;
};

typedef struct {
    size_t size;
    struct P_ArgsOption options[PARGS_CAPACITY];
} P_Args;

P_Args *pargs_parse(int *argc, char ***argv);
const char *pargs_getl(P_Args *pargs, const char *name);
const char *pargs_gets(P_Args *pargs, char name);

#endif /* __PARGS__ */