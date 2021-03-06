%define parse.error verbose

%{
#include <stdio.h>
#include <string.h>
#include "commands.h"
#include "utils.h"
#include "datastructs.h"

// Declaration to suppress implicit declaration warnings
extern int yylineno;
int yylex(); 
void yyerror(const char *s);
void set_input_string(const char* in);
void set_input_file(FILE *file);
void end_lexical_scan();

%}


%union {
  char *str;
  command_t command;
  pipeline_t pipeline;
  line_t line;
  redirection_t *redirection;
}

%token EOL
%token STRING
%token SEMI
%token PIPE
%token GT
%token LT

%type<str> STRING
%type<command> command
%type<pipeline> pipeline
%type<line> line
%type<redirection> redirection


%%

file: program file | EOL file | program | EOL;

program: line EOL { 
  process_line(&$1); 
  free_line(&$1);
} 

line: pipeline SEMI line {
    sequence_t *s = safe_malloc(sizeof(sequence_t));
    s->pipeline = $1;
    STAILQ_INSERT_HEAD(&$3, s, entries);
    $$ = $3;
  }
  | pipeline opt_semi {
    line_t line;
    STAILQ_INIT(&line);
    sequence_t *s = safe_malloc(sizeof(sequence_t));
    s->pipeline = $1;
    STAILQ_INSERT_HEAD(&line, s, entries);
    $$ = line;
    };

pipeline: command PIPE pipeline {
    pipe_segment_t *p = safe_malloc(sizeof(pipe_segment_t));
    p->command = $1;
    STAILQ_INSERT_HEAD(&$3, p, entries);
    $$ = $3;
  } 
  | command {
    pipeline_t pipeline;
    STAILQ_INIT(&pipeline);
    pipe_segment_t *p = safe_malloc(sizeof(pipe_segment_t));
    p->command = $1;
    STAILQ_INSERT_HEAD(&pipeline, p, entries);
    $$ = pipeline;
  };
command: STRING command {
    param_t *p = safe_malloc(sizeof(param_t));
    p->string = $1;
    p->redirection = NULL;
    STAILQ_INSERT_HEAD(&$2, p, entries);
    $$ = $2;
  }
  | STRING {
    command_t command;
    STAILQ_INIT(&command);
    param_t *p = safe_malloc(sizeof(param_t));
    p->string = $1;
    p->redirection = NULL;
    STAILQ_INSERT_HEAD(&command, p, entries);
    $$ = command;
  }
  | redirection command {
    param_t *p = safe_malloc(sizeof(param_t));
    p->string = NULL;
    p->redirection = $1;
    STAILQ_INSERT_HEAD(&$2, p, entries);
    $$ = $2;
  }
  | redirection {
    command_t command;
    STAILQ_INIT(&command);
    param_t *p = safe_malloc(sizeof(param_t));
    p->string = NULL;
    p->redirection = $1;
    STAILQ_INSERT_HEAD(&command, p, entries);
    $$ = command;
  };

  redirection: GT STRING {
    redirection_t *r = safe_malloc(sizeof(redirection_t));
    r->file = $2;
    r->type = OUTPUT;
    $$ = r;
  }
  | GT GT STRING {
    redirection_t *r = safe_malloc(sizeof(redirection_t));
    r->file = $3;
    r->type = OUTPUT_APPEND;
    $$ = r;
  }
  | LT STRING {
    redirection_t *r = safe_malloc(sizeof(redirection_t));
    r->file = $2;
    r->type = INPUT;
    $$ = r;
  };

opt_semi: SEMI | /* nothing */ ;

%%

void yyerror(const char *s) {
  shell_retval = 254;
  fprintf(stderr, "error:%d: %s\n", yylineno, s);
}

int parse_string(const char* in) {
  set_input_string(in);
  int rv = yyparse();
  end_lexical_scan();
  return rv;
}

int parse_file(FILE *file) {
  set_input_file(file);
  int rv = yyparse();
  end_lexical_scan();
  return rv;
}
