#include "datastructs.h"
#include <stdlib.h>

// Frees malloc'd data structures
void free_line(line_t *line) {
    while (!STAILQ_EMPTY(line)) {
        sequence_t *seq = STAILQ_FIRST(line);
        while (!STAILQ_EMPTY(&seq->pipeline)) {
            pipe_segment_t *seg = STAILQ_FIRST(&seq->pipeline);
            while (!STAILQ_EMPTY(&seg->command)) {
                param_t *par = STAILQ_FIRST(&seg->command);
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