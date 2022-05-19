#ifndef NODESET_H
#define NODESET_H

#include <stdio.h>

#define debug(fmt, ...) \
    do { \
        fprintf(stderr, "DEBUG %s:%s:%d " fmt "\n", \
                __FILE__, __func__, __LINE__ \
                __VA_OPT__(,)__VA_ARGS__); \
    } \
    while (0)

#define trace() debug("")

enum nodeset_type {
    NODESET_EMPTY,
    NODESET_VALUE,
    NODESET_RANGE,
};

struct nodeset {
    enum nodeset_type type;
    union {
        unsigned int value;
        struct {
            unsigned int begin;
            unsigned int end;
        } range;
    } data;
    struct nodeset *next;
};

struct nodeset *
nodeset_from_str(const char *input);

struct nodeset *
nodeset_new();

void
nodeset_destroy(struct nodeset *nodeset);

inline struct nodeset *
nodeset_next(struct nodeset *n)
{
    return n->next;
}

#endif
