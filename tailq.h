#pragma once
#include <sys/queue.h>

typedef struct node {
    STAILQ_ENTRY(node) entries;
    char *group; 
} node_t;

typedef STAILQ_HEAD(head_s, node) head_t;

typedef struct cmd {
    STAILQ_ENTRY(cmd) entries;
    struct head_s node;
} cmd_t;

typedef STAILQ_HEAD(cmd_s, cmd) program_t;

void free_tailq(struct cmd_s *head);