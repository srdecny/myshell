#include "datastructs.h"
#include <stdlib.h>

// Frees malloc'd data structures
void free_line(struct line_s *line) {
    while (!STAILQ_EMPTY(line)) {
        struct sequence *seq = STAILQ_FIRST(line);
        while (!STAILQ_EMPTY(&seq->pipeline)) {
            struct pipe_segment *seg = STAILQ_FIRST(&seq->pipeline);
            while (!STAILQ_EMPTY(&seg->command)) {
                struct param *par = STAILQ_FIRST(&seg->command);
                free(par->string);
                free(par->redirection);
                STAILQ_REMOVE_HEAD(&seg->command, entries);
                free(par);
            }
            STAILQ_REMOVE_HEAD(&seq->pipeline, entries);
            free(seg);
        }
        STAILQ_REMOVE_HEAD(line, entries);
        free(seq);
    }
}