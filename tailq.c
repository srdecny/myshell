#include "tailq.h"
#include <stdlib.h>

void free_tailq(struct cmd_s *cmd) {

    struct cmd *c = NULL;
    while (!STAILQ_EMPTY(cmd)) {
        c = STAILQ_FIRST(cmd);
        struct head_s *head = &c->node;
        struct node *e = NULL;
        while (!STAILQ_EMPTY(head)) {
            e = STAILQ_FIRST(head);
            STAILQ_REMOVE_HEAD(head, entries);
            free(e->group);
            free(e);
            e = NULL;
        }
        STAILQ_REMOVE_HEAD(cmd, entries);
        free(c);
        c = NULL;
    }
}