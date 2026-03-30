%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.tab.h"

static char* dup_text(const char* s) {
    size_t n = strlen(s);
    char* out = (char*)malloc(n + 1);
    if (!out) {
        return NULL;
    }
    memcpy(out, s, n + 1);
    return out;
}
%}

%option noyywrap yylineno

%%

"auth"      { return AUTH; }
"secure"    { return SECURE; }
"packet"    { return PACKET; }
"scan"      { return SCAN; }
"log"       { return LOG; }
"firewall"  { return FIREWALL; }
"breach"    { return BREACH; }
"monitor"   { return MONITOR; }
"iterate"   { return ITERATE; }
"exitsec"   { return EXITSEC; }
"abort"     { return ABORT; }

"=="        { return EQ; }
"!="        { return NEQ; }

[0-9]+\.[0-9]+ {
    yylval.str = dup_text(yytext);
    return FLOAT_LITERAL;
}

[0-9]+ {
    yylval.str = dup_text(yytext);
    return INT_LITERAL;
}

\"([^\\\"]|\\.)*\" {
    yylval.str = dup_text(yytext);
    return STRING_LITERAL;
}

[a-zA-Z_][a-zA-Z0-9_]* {
    yylval.str = dup_text(yytext);
    return ID;
}

"//".*      { /* ignore comment */ }
[ \t\r\n]+  { /* ignore whitespace */ }

"("         { return '('; }
")"         { return ')'; }
"{"         { return '{'; }
"}"         { return '}'; }
";"         { return ';'; }
"+"         { return '+'; }
"-"         { return '-'; }
"*"         { return '*'; }
"/"         { return '/'; }
"="         { return '='; }
">"         { return '>'; }
"<"         { return '<'; }

. {
    fprintf(stderr, "Lexical error at line %d: invalid token '%s'\n", yylineno, yytext);
}

%%
