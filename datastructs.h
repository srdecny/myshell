#ifndef DATASTRUCTS_H
#define DATASTRUCTS_H

#include <sys/queue.h>

/**
A line consists of multiple sequences separeted by semicolons. 
A sequence consists of multiple pipelines separated by pipe symbol.
A pipeline consists of multiple commands separated by spaces.
A command consists of multiple parameters.
**/

enum redirection_type {
  OUTPUT,
  OUTPUT_APPEND,
  INPUT
};

typedef struct redirection {
  char *file;
  enum redirection_type type;
} redirection_t;

typedef struct param {
    STAILQ_ENTRY(param) entries;
    char *string; 
    redirection_t *redirection;
} param_t;
typedef STAILQ_HEAD(command_s, param) command_t;

typedef struct pipe_segment {
    STAILQ_ENTRY(pipe_segment) entries;
    command_t command;
} pipe_segment_t; 
typedef STAILQ_HEAD(pipeline_s, pipe_segment) pipeline_t;

typedef struct sequence {
  STAILQ_ENTRY(sequence) entries;
  pipeline_t pipeline;
} sequence_t;
typedef STAILQ_HEAD(line_s, sequence) line_t;

void free_line(line_t *line);

#endif