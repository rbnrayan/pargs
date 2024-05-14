#include "pargs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define DA_DEFAULT_CAPACITY 256
#define da_append(da, item)                                                                \
    do {                                                                                   \
        if ((da)->size >= (da)->capacity) {                                                \
            (da)->capacity = (da)->capacity == 0 ? DA_DEFAULT_CAPACITY : (da)->capacity*2; \
            (da)->items = realloc((da)->items, sizeof(*(da)->items) * (da)->capacity);     \
            assert((da)->items != NULL && "Failed to realloc dynamic array");              \
        }                                                                                  \
        (da)->items[(da)->size++] = (item);                                                \
    } while (0)
#define da_free(da) free((da)->items)

#define EOA_STR "--"

enum P_ArgsTokenType {
    NAME_LONG,
    NAME_SHORT,
    VALUE,
    EOA
};

struct P_ArgsToken {
    enum P_ArgsTokenType type;
    union {
        char *name_long_value;
        char  name_short_value;
        char *value;
        char  equal_value;
    };
};

struct P_ArgsLexer {
    char **args;
    size_t args_count;
    size_t args_offset, ch_offset;
};

struct Tokens {
    struct P_ArgsToken *items;
    size_t capacity;
    size_t size;
};

struct P_ArgsParser {
    struct Tokens *tokens;
    size_t offset;
};

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

static long find_first_equal(const char *src, size_t len)
{
    for (size_t i = 0; i < len; ++i) {
        if (src[i] == '=')
            return i;
    }

    return -1;
}

static void init_lexer(struct P_ArgsLexer *lexer, int *argc, char ***argv)
{
    lexer->args_offset = 0;
    lexer->ch_offset = 0;
    lexer->args_count = *argc;
    lexer->args = malloc(sizeof(char *) * (*argc));
    memset(lexer->args, 0, lexer->args_count);

    if (lexer->args == NULL)
        pargs_panic("failed to allocate memory for lexer");

    size_t i = 0;
    char *arg = shift_args(argc, argv);
    while (arg != NULL && strcmp(EOA_STR, arg) != 0) {
        lexer->args[i++] = arg;
        arg = shift_args(argc, argv);
    }
}

static void free_lexer(struct P_ArgsLexer *lexer)
{
    free(lexer->args);
}

static struct P_ArgsToken lex_next_token(struct P_ArgsLexer *lexer)
{
    struct P_ArgsToken token = { 0 };
    
    if (lexer->args_offset >= lexer->args_count || strcmp(EOA_STR, lexer->args[lexer->args_offset]) == 0) {
        token.type = EOA;
        return token;
    }

    char *current_arg = lexer->args[lexer->args_offset];
    size_t current_arg_len = strlen(current_arg);

    if (lexer->ch_offset == 0 && current_arg[lexer->ch_offset] == '-') {
        if (current_arg_len > 2 && current_arg[lexer->ch_offset+1] == '-') {
            token.type = NAME_LONG;
            token.name_long_value = current_arg + 2;

            long equal_idx = find_first_equal(current_arg, current_arg_len);
            if (equal_idx > 0) {
                current_arg[equal_idx] = '\0';
                lexer->ch_offset = equal_idx + 1;
            } else {
                lexer->args_offset += 1;
            }
        } else if (current_arg_len > 2) {
            token.type = NAME_SHORT;
            token.name_short_value = *(current_arg + 1);

            lexer->ch_offset += 2;
        } else {
            if (current_arg_len < 2)
                pargs_panic("no character provided for short named option");

            token.type = NAME_SHORT;
            token.name_short_value = *(current_arg + 1);

            lexer->args_offset += 1;
        }
    } else {
        token.type = VALUE;
        token.value = current_arg + lexer->ch_offset;

        lexer->ch_offset = 0;
        lexer->args_offset += 1;
    }

    return token;
}

static struct P_ArgsOption parse_next_option(struct P_ArgsParser *parser)
{
    struct P_ArgsOption opt = { 0 };
    
    struct P_ArgsToken token = parser->tokens->items[parser->offset++];
    switch (token.type) {
    case NAME_LONG:
        opt.name_type = LONG;
        opt.long_name = token.name_long_value;
        break;
    case NAME_SHORT:
        opt.name_type = SHORT;
        opt.short_name = token.name_short_value;
        break;
    case VALUE:
    case EOA:
        pargs_panic("unexpected token while parsing");
    }

    token = parser->tokens->items[parser->offset];
    if (token.type == VALUE) {
        opt.value = token.value;
        parser->offset += 1;
    }

    return opt;
}

static void parse_tokens(P_Args *pargs, struct P_ArgsParser *parser)
{
    size_t i = 0;
    struct P_ArgsOption opt = parse_next_option(parser);
    while (parser->offset < parser->tokens->size) {
        pargs->options[i++] = opt;
        opt = parse_next_option(parser);
    }
}

P_Args *pargs_parse(int *argc, char ***argv)
{
    // shift program name.
    shift_args(argc, argv);

    struct P_ArgsLexer lexer = { 0 };
    init_lexer(&lexer, argc, argv);

    struct Tokens tokens = { 0 };
    struct P_ArgsToken token;
    while ((token = lex_next_token(&lexer)).type != EOA)
        da_append(&tokens, token);

    P_Args *pargs = malloc(sizeof(P_Args));
    memset(pargs->options, 0, PARGS_CAPACITY);
    pargs->size = 0;

    struct P_ArgsParser parser = {
        .tokens = &tokens,
        .offset = 0
    };
    parse_tokens(pargs, &parser);

    da_free(&tokens);
    free_lexer(&lexer);
    return pargs;
}

const char *pargs_getl(P_Args *pargs, const char *name)
{
    for (size_t i = 0; i < pargs->size; ++i) {
        if (pargs->options[i].name_type != LONG)
            continue;
        
        const struct P_ArgsOption option = pargs->options[i];
        if (strcmp(name, option.long_name) == 0)
            return option.value;
    }
    return NULL;
}

const char *pargs_gets(P_Args *pargs, char name)
{
    for (size_t i = 0; i < pargs->size; ++i) {
        if (pargs->options[i].name_type != SHORT)
            continue;

        const struct P_ArgsOption option = pargs->options[i];
        if (option.short_name == name)
            return option.value;
    }
    return NULL;
}
