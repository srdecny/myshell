%{
#include "datastructs.h"
#include "mysh.tab.h"
%}

%option nounput
%option noinput
%option yylineno
%option noyywrap

%%

#.+\n { return EOL; }
\n { return EOL; }
; { return SEMI; }
\| { return PIPE; }
\> { return GT; }
\< { return LT; }
[ \t] { /* ignore white space */ }
[^|>< \t\n;]+ { yylval.str = strdup(yytext); return STRING; }

%%

void set_input_string(const char* in) {
  yy_scan_string(in);
}

void set_input_file(FILE *file) {
  yyin = file;
}

void end_lexical_scan() {
  yy_delete_buffer(YY_CURRENT_BUFFER);
}
