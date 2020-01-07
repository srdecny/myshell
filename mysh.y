%define parse.error verbose

%{
#include <stdio.h>
#include <string.h>
#include "commands.c"
#include "utils.h"
#include "shared.h"

// Declaration to suppress implicit declaration warnings
extern int yylineno;
int yylex(); 
void yyerror(const char *s);
void set_input_string(const char* in);
void set_input_file(FILE *file);
void end_lexical_scan();

%}

%code requires {
  #include "tailq.h"
}

%union {
  char *str;
  struct head_s stail;
  struct cmd_s cmd;
}

%token EOL
%token GROUP
%token SEMI

%type<stail> command
%type<str> GROUP
%type<cmd> line

%%

file: program file | EOL file | program | EOL;

program: line EOL { process_command(&$1); } 

line: command SEMI line {
    struct cmd *c = safe_malloc(sizeof(struct cmd));
    c->node = $1;
    STAILQ_INSERT_HEAD(&$3, c, entries);
    $$ = $3;
  }
  | command opt_semi {
      program_t program;
      STAILQ_INIT(&program);void set_input_string(const char* in);
      struct cmd *c = safe_malloc(sizeof(struct cmd));
      c->node = $1;
      STAILQ_INSERT_HEAD(&program, c, entries);
      $$ = program;
    };

command: GROUP command {
    struct node *e = safe_malloc(sizeof(struct node));
    e->group = $1;
    STAILQ_INSERT_HEAD(&$2, e, entries);/* Declarations */
    $$ = $2;
  }
  | GROUP {
    head_t head;
    STAILQ_INIT(&head);
    struct node *e = safe_malloc(sizeof(struct node));
    e->group = $1;
    STAILQ_INSERT_HEAD(&head, e, entries);
    $$ = head;
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
