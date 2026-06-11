#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ---------- token types ---------- */
#define TOK_ID       1
#define TOK_NUM      2
#define TOK_STR      3
#define TOK_FSTR     4
#define TOK_LPAREN   5
#define TOK_RPAREN   6
#define TOK_PLUS     7
#define TOK_MINUS    8
#define TOK_STAR     9
#define TOK_SLASH    10
#define TOK_ASSIGN   11
#define TOK_EOF      12
#define TOK_TRUE     13
#define TOK_FALSE    14
#define TOK_FLOAT    15
#define TOK_RSHIFT   16
#define TOK_COMMA    17
#define TOK_EQ       18
#define TOK_NEQ      19
#define TOK_LEQ      20
#define TOK_GEQ      21
#define TOK_LT       22
#define TOK_GT       23
#define TOK_AND      24
#define TOK_OR       25
#define TOK_NOT      26
#define TOK_FN       27
#define TOK_RET      28

typedef struct {
    int type;
    char text[256];
} Token;

/* ---------- variable types ---------- */
typedef enum { V_INT, V_FLOAT, V_STRING, V_BOOL, V_VOID } VarType;

typedef struct VarInfo {
    char name[64];
    VarType type;
    struct VarInfo *next;
} VarInfo;

/* ---------- scope stack ---------- */
VarInfo *scope_stack[100];
int scope_top = -1;

void push_scope() {
    scope_stack[++scope_top] = NULL;
}

void pop_scope() {
    if (scope_top >= 0) {
        VarInfo *v = scope_stack[scope_top];
        while (v) {
            VarInfo *tmp = v;
            v = v->next;
            free(tmp);
        }
        scope_top--;
    }
}

void add_var(const char *name, VarType type) {
    VarInfo *n = malloc(sizeof(VarInfo));
    strcpy(n->name, name);
    n->type = type;
    n->next = scope_stack[scope_top];
    scope_stack[scope_top] = n;
}

VarType find_var_type(const char *name) {
    for (int i = scope_top; i >= 0; i--) {
        VarInfo *v = scope_stack[i];
        while (v) {
            if (strcmp(v->name, name) == 0)
                return v->type;
            v = v->next;
        }
    }
    return V_VOID;
}

/* ---------- function definition ---------- */
typedef struct FuncDef {
    char name[64];
    char **params;
    int param_count;
    VarType *param_types;
    VarType return_type;
    char **body_lines;
    int body_count;
    struct FuncDef *next;
} FuncDef;

FuncDef *func_list = NULL;

/* ---------- tokenizer ---------- */
int tokenize(const char *line, Token *tokens, int max_tok) {
    int i = 0, n = 0;
    while (line[i] && n < max_tok) {
        if (isspace(line[i])) { i++; continue; }

        if (line[i] == '(') { tokens[n++] = (Token){TOK_LPAREN, "("}; i++; continue; }
        if (line[i] == ')') { tokens[n++] = (Token){TOK_RPAREN, ")"}; i++; continue; }
        if (line[i] == '+') { tokens[n++] = (Token){TOK_PLUS, "+"}; i++; continue; }
        if (line[i] == '-') { tokens[n++] = (Token){TOK_MINUS, "-"}; i++; continue; }
        if (line[i] == '*') { tokens[n++] = (Token){TOK_STAR, "*"}; i++; continue; }
        if (line[i] == '/') { tokens[n++] = (Token){TOK_SLASH, "/"}; i++; continue; }
        if (line[i] == ',') { tokens[n++] = (Token){TOK_COMMA, ","}; i++; continue; }

        if (line[i] == '=' && line[i+1] == '=') { tokens[n++] = (Token){TOK_EQ, "=="}; i+=2; continue; }
        if (line[i] == '!' && line[i+1] == '=') { tokens[n++] = (Token){TOK_NEQ, "!="}; i+=2; continue; }
        if (line[i] == '<' && line[i+1] == '=') { tokens[n++] = (Token){TOK_LEQ, "<="}; i+=2; continue; }
        if (line[i] == '>' && line[i+1] == '=') { tokens[n++] = (Token){TOK_GEQ, ">="}; i+=2; continue; }
        if (line[i] == '=') { tokens[n++] = (Token){TOK_ASSIGN, "="}; i++; continue; }
        if (line[i] == '<') { tokens[n++] = (Token){TOK_LT, "<"}; i++; continue; }
        if (line[i] == '>') {
            if (line[i+1] == '>') {
                tokens[n++] = (Token){TOK_RSHIFT, ">>"};
                i += 2;
            } else {
                tokens[n++] = (Token){TOK_GT, ">"};
                i++;
            }
            continue;
        }

        if (isdigit(line[i]) || (line[i] == '.' && isdigit(line[i+1]))) {
            int j = 0, has_dot = 0;
            while (isdigit(line[i]) || line[i] == '.') {
                if (line[i] == '.') {
                    if (has_dot) break;
                    has_dot = 1;
                }
                tokens[n].text[j++] = line[i++];
            }
            tokens[n].text[j] = '\0';
            tokens[n].type = has_dot ? TOK_FLOAT : TOK_NUM;
            n++;
            continue;
        }

        if (line[i] == '"' || (line[i] == 'f' && line[i+1] == '"')) {
            int is_f = 0, j = 0;
            if (line[i] == 'f') { is_f = 1; i++; }
            i++;
            while (line[i] && line[i] != '"') {
                if (line[i] == '\\' && line[i+1]) i++;
                tokens[n].text[j++] = line[i++];
            }
            tokens[n].text[j] = '\0';
            tokens[n].type = is_f ? TOK_FSTR : TOK_STR;
            n++;
            if (line[i] == '"') i++;
            continue;
        }

        if (isalpha(line[i]) || line[i] == '_') {
            int j = 0;
            while (isalnum(line[i]) || line[i] == '_')
                tokens[n].text[j++] = line[i++];
            tokens[n].text[j] = '\0';
            if (strcmp(tokens[n].text, "true") == 0) tokens[n].type = TOK_TRUE;
            else if (strcmp(tokens[n].text, "false") == 0) tokens[n].type = TOK_FALSE;
            else if (strcmp(tokens[n].text, "and") == 0) tokens[n].type = TOK_AND;
            else if (strcmp(tokens[n].text, "or") == 0) tokens[n].type = TOK_OR;
            else if (strcmp(tokens[n].text, "not") == 0) tokens[n].type = TOK_NOT;
            else if (strcmp(tokens[n].text, "fn") == 0) tokens[n].type = TOK_FN;
            else if (strcmp(tokens[n].text, "ret") == 0) tokens[n].type = TOK_RET;
            else tokens[n].type = TOK_ID;
            n++;
            continue;
        }
        i++;
    }
    tokens[n].type = TOK_EOF;
    return n;
}

/* ---------- expression parser ---------- */
static Token *expr_tokens;
static int expr_pos;

char *parse_expr(VarType *type);
char *parse_factor(VarType *type);

char *parse_term(VarType *type) {
    char *left = parse_factor(type);
    VarType left_type = *type;
    while (expr_tokens[expr_pos].type == TOK_STAR || expr_tokens[expr_pos].type == TOK_SLASH) {
        if (left_type != V_INT && left_type != V_FLOAT) {
            fprintf(stderr, "Error: non-numeric type in arithmetic\n"); exit(1);
        }
        char op = expr_tokens[expr_pos].text[0];
        expr_pos++;
        VarType right_type;
        char *right = parse_factor(&right_type);
        if (right_type != V_INT && right_type != V_FLOAT) {
            fprintf(stderr, "Error: non-numeric type in arithmetic\n"); exit(1);
        }
        char *new_expr = malloc(strlen(left) + strlen(right) + 10);
        sprintf(new_expr, "(%s %c %s)", left, op, right);
        free(left); free(right);
        left = new_expr;
        left_type = (left_type == V_FLOAT || right_type == V_FLOAT) ? V_FLOAT : V_INT;
    }
    *type = left_type;
    return left;
}

char *parse_expr(VarType *type) {
    char *left = parse_term(type);
    VarType left_type = *type;
    while (expr_tokens[expr_pos].type == TOK_PLUS || expr_tokens[expr_pos].type == TOK_MINUS) {
        if (left_type != V_INT && left_type != V_FLOAT) {
            fprintf(stderr, "Error: non-numeric type in arithmetic\n"); exit(1);
        }
        char op = expr_tokens[expr_pos].text[0];
        expr_pos++;
        VarType right_type;
        char *right = parse_term(&right_type);
        if (right_type != V_INT && right_type != V_FLOAT) {
            fprintf(stderr, "Error: non-numeric type in arithmetic\n"); exit(1);
        }
        char *new_expr = malloc(strlen(left) + strlen(right) + 10);
        sprintf(new_expr, "(%s %c %s)", left, op, right);
        free(left); free(right);
        left = new_expr;
        left_type = (left_type == V_FLOAT || right_type == V_FLOAT) ? V_FLOAT : V_INT;
    }
    *type = left_type;
    return left;
}

FuncDef *find_func(const char *name) {
    for (FuncDef *f = func_list; f; f = f->next)
        if (strcmp(f->name, name) == 0) return f;
    return NULL;
}

char *parse_factor(VarType *type) {
    Token tok = expr_tokens[expr_pos];
    if (tok.type == TOK_NUM) {
        expr_pos++;
        *type = V_INT;
        return strdup(tok.text);
    }
    if (tok.type == TOK_FLOAT) {
        expr_pos++;
        *type = V_FLOAT;
        return strdup(tok.text);
    }
    if (tok.type == TOK_TRUE || tok.type == TOK_FALSE) {
        expr_pos++;
        *type = V_BOOL;
        return strdup(strcmp(tok.text, "true") == 0 ? "1" : "0");
    }
    if (tok.type == TOK_STR) {
        expr_pos++;
        *type = V_STRING;
        char *quoted = malloc(strlen(tok.text) + 3);
        sprintf(quoted, "\"%s\"", tok.text);
        return quoted;
    }
    if (tok.type == TOK_ID) {
        if (expr_tokens[expr_pos+1].type == TOK_LPAREN) {
            // function call
            char *func_name = strdup(tok.text);
            expr_pos++; expr_pos++;
            FuncDef *func = find_func(func_name);
            if (!func) { fprintf(stderr, "Error: function '%s' not defined\n", func_name); exit(1); }
            int arg_count = 0;
            char *arg_code[10];
            VarType arg_types[10];
            while (expr_tokens[expr_pos].type != TOK_RPAREN) {
                if (arg_count > 0 && expr_tokens[expr_pos].type == TOK_COMMA) expr_pos++;
                VarType atype;
                char *aexpr = parse_expr(&atype);
                arg_code[arg_count] = aexpr;
                arg_types[arg_count] = atype;
                arg_count++;
            }
            expr_pos++;
            // infer parameter types from arguments
            for (int i = 0; i < arg_count; i++) {
                if (i < func->param_count) {
                    if (func->param_types[i] == V_VOID) {
                        func->param_types[i] = arg_types[i];
                    } else if (func->param_types[i] != arg_types[i]) {
                        fprintf(stderr, "Error: argument type mismatch for parameter '%s'\n", func->params[i]);
                        exit(1);
                    }
                }
            }
            char *call = malloc(256);
            sprintf(call, "%s(", func_name);
            for (int i = 0; i < arg_count; i++) {
                if (i > 0) strcat(call, ", ");
                strcat(call, arg_code[i]);
                free(arg_code[i]);
            }
            strcat(call, ")");
            *type = func->return_type;
            free(func_name);
            return call;
        } else {
            // variable lookup
            expr_pos++;
            VarType vtype = find_var_type(tok.text);
            if (vtype == V_VOID) {
                fprintf(stderr, "Error: variable '%s' not declared\n", tok.text);
                exit(1);
            }
            *type = vtype;
            return strdup(tok.text);
        }
    }
    if (tok.type == TOK_LPAREN) {
        expr_pos++;
        char *inner = parse_expr(type);
        if (expr_tokens[expr_pos].type == TOK_RPAREN) expr_pos++;
        char *s = malloc(strlen(inner) + 3);
        sprintf(s, "(%s)", inner);
        free(inner);
        return s;
    }
    fprintf(stderr, "Error: unexpected token in expression\n");
    exit(1);
}

/* ---------- boolean expression parser ---------- */
static char *parse_comparison() {
    VarType left_type;
    char *left = parse_expr(&left_type);
    int op_type = expr_tokens[expr_pos].type;
    if (op_type == TOK_EQ || op_type == TOK_NEQ ||
        op_type == TOK_LEQ || op_type == TOK_GEQ ||
        op_type == TOK_LT  || op_type == TOK_GT) {
        const char *op_str = expr_tokens[expr_pos].text;
        expr_pos++;
        VarType right_type;
        char *right = parse_expr(&right_type);
        if (left_type != right_type) {
            fprintf(stderr, "Error: type mismatch in comparison\n"); exit(1);
        }
        char *result = malloc(strlen(left) + strlen(right) + 10);
        if (left_type == V_STRING) {
            if (op_type == TOK_EQ)
                sprintf(result, "(strcmp(%s, %s) == 0)", left, right);
            else if (op_type == TOK_NEQ)
                sprintf(result, "(strcmp(%s, %s) != 0)", left, right);
            else {
                fprintf(stderr, "Error: strings only support == and !=\n"); exit(1);
            }
        } else {
            sprintf(result, "(%s %s %s)", left, op_str, right);
        }
        free(left); free(right);
        return result;
    }
    char *result = malloc(strlen(left) + 10);
    if (left_type == V_STRING) sprintf(result, "(%s[0] != 0)", left);
    else if (left_type == V_BOOL) sprintf(result, "%s", left);
    else sprintf(result, "(%s != 0)", left);
    free(left);
    return result;
}

static char *parse_and_expr() {
    char *left = parse_comparison();
    while (expr_tokens[expr_pos].type == TOK_AND) {
        expr_pos++;
        char *right = parse_comparison();
        char *new_expr = malloc(strlen(left) + strlen(right) + 10);
        sprintf(new_expr, "(%s && %s)", left, right);
        free(left); free(right);
        left = new_expr;
    }
    return left;
}

static char *parse_bool_expr() {
    char *left = parse_and_expr();
    while (expr_tokens[expr_pos].type == TOK_OR) {
        expr_pos++;
        char *right = parse_and_expr();
        char *new_expr = malloc(strlen(left) + strlen(right) + 10);
        sprintf(new_expr, "(%s || %s)", left, right);
        free(left); free(right);
        left = new_expr;
    }
    return left;
}

/* ---------- compile a single statement ---------- */
void compile_line(const char *line, FILE *out, int inside_function) {
    Token tokens[100];
    int n = tokenize(line, tokens, 100);
    if (n == 0) return;

    if (tokens[0].type == TOK_RET) {
        if (!inside_function) { fprintf(stderr, "Error: 'ret' outside function\n"); exit(1); }
        if (tokens[1].type == TOK_EOF) {
            fprintf(out, "    return;\n");
        } else {
            expr_tokens = tokens + 1;
            expr_pos = 0;
            VarType etype;
            char *c_expr = parse_expr(&etype);
            fprintf(out, "    return %s;\n", c_expr);
            free(c_expr);
        }
        return;
    }

    if (tokens[0].type == TOK_ID && strcmp(tokens[0].text, "ask") == 0 && tokens[1].type == TOK_LPAREN) {
        int i = 2;
        if (tokens[i].type != TOK_STR) { fprintf(stderr, "Error: ask expects a string\n"); exit(1); }
        char *prompt = tokens[i].text;
        i++;
        if (tokens[i].type != TOK_RSHIFT) { fprintf(stderr, "Error: missing >> after prompt\n"); exit(1); }
        i++;
        char varnames[10][64]; int vc = 0;
        while (tokens[i].type == TOK_ID) {
            strcpy(varnames[vc++], tokens[i].text);
            i++;
            if (tokens[i].type == TOK_COMMA) i++; else break;
        }
        if (tokens[i].type != TOK_RPAREN) { fprintf(stderr, "Error: ask syntax\n"); exit(1); }

        fprintf(out, "    printf(\"%s\\n\");\n", prompt);
        for (int j = 0; j < vc; j++) {
            VarType vt = find_var_type(varnames[j]);
            if (vt == V_VOID) { fprintf(stderr, "Error: variable '%s' not declared\n", varnames[j]); exit(1); }
            if (vt == V_INT)
                fprintf(out, "    if (scanf(\" %%d\", &%s) != 1) { printf(\"error: wrong assign\\n\"); exit(1); }\n", varnames[j]);
            else if (vt == V_FLOAT)
                fprintf(out, "    if (scanf(\" %%lf\", &%s) != 1) { printf(\"error: wrong assign\\n\"); exit(1); }\n", varnames[j]);
            else if (vt == V_STRING)
                fprintf(out, "    if (scanf(\" %%s\", %s) != 1) { printf(\"error: wrong assign\\n\"); exit(1); }\n", varnames[j]);
            else if (vt == V_BOOL)
                fprintf(out, "    if (scanf(\" %%d\", &%s) != 1) { printf(\"error: wrong assign\\n\"); exit(1); }\n", varnames[j]);
        }
        return;
    }

    if (tokens[0].type == TOK_ID && strcmp(tokens[0].text, "show") == 0 && tokens[1].type == TOK_LPAREN) {
        int arg_start = 2, arg_end = 2, depth = 1;
        while (depth > 0 && tokens[arg_end].type != TOK_EOF) {
            if (tokens[arg_end].type == TOK_LPAREN) depth++;
            else if (tokens[arg_end].type == TOK_RPAREN) depth--;
            if (depth == 0) break;
            arg_end++;
        }
        int arg_tok_count = arg_end - arg_start;

        if (arg_tok_count == 1 && tokens[arg_start].type == TOK_FSTR) {
            char *fstr = tokens[arg_start].text;
            fprintf(out, "    printf(\"");
            const char *p = fstr;
            while (*p) {
                if (*p == '{') {
                    p++;
                    char expr_buf[256]; int j = 0;
                    while (*p && *p != '}') expr_buf[j++] = *p++;
                    if (*p == '}') p++;
                    expr_buf[j] = '\0';
                    Token inner[50];
                    tokenize(expr_buf, inner, 50);
                    expr_tokens = inner; expr_pos = 0;
                    VarType inner_type;
                    char *c_expr = parse_expr(&inner_type);
                    if (inner_type == V_INT) fprintf(out, "%%d");
                    else if (inner_type == V_FLOAT) fprintf(out, "%%f");
                    else if (inner_type == V_STRING) fprintf(out, "%%s");
                    else if (inner_type == V_BOOL) fprintf(out, "%%s");
                    free(c_expr);
                } else { fputc(*p, out); p++; }
            }
            fprintf(out, "\\n\"");
            p = fstr;
            while (*p) {
                if (*p == '{') {
                    p++;
                    char expr_buf[256]; int j = 0;
                    while (*p && *p != '}') expr_buf[j++] = *p++;
                    if (*p == '}') p++;
                    expr_buf[j] = '\0';
                    Token inner[50];
                    tokenize(expr_buf, inner, 50);
                    expr_tokens = inner; expr_pos = 0;
                    VarType inner_type;
                    char *c_expr = parse_expr(&inner_type);
                    if (inner_type == V_BOOL)
                        fprintf(out, ", (%s ? \"true\" : \"false\")", c_expr);
                    else
                        fprintf(out, ", %s", c_expr);
                    free(c_expr);
                } else p++;
            }
            fprintf(out, ");\n");
            return;
        }

        if (arg_tok_count == 1 && tokens[arg_start].type == TOK_STR) {
            fprintf(out, "    printf(\"%s\\n\");\n", tokens[arg_start].text);
            return;
        }

        expr_tokens = tokens + arg_start;
        expr_pos = 0;
        VarType etype;
        char *c_expr = parse_expr(&etype);
        if (etype == V_INT) fprintf(out, "    printf(\"%%d\\n\", %s);\n", c_expr);
        else if (etype == V_FLOAT) fprintf(out, "    printf(\"%%f\\n\", %s);\n", c_expr);
        else if (etype == V_STRING) fprintf(out, "    printf(\"%%s\\n\", %s);\n", c_expr);
        else if (etype == V_BOOL) fprintf(out, "    printf(\"%%s\\n\", (%s ? \"true\" : \"false\"));\n", c_expr);
        free(c_expr);
        return;
    }

    if (tokens[0].type == TOK_ID && tokens[1].type == TOK_LPAREN && find_func(tokens[0].text)) {
        expr_tokens = tokens;
        expr_pos = 0;
        VarType etype;
        char *call_expr = parse_expr(&etype);
        fprintf(out, "    %s;\n", call_expr);
        free(call_expr);
        return;
    }

    if (tokens[0].type == TOK_ID && tokens[1].type == TOK_ASSIGN) {
        char *var = tokens[0].text;
        VarType lhs_type = find_var_type(var);
        if (lhs_type == V_VOID) {
            expr_tokens = tokens + 2;
            expr_pos = 0;
            VarType rhs_type;
            char *dummy = parse_expr(&rhs_type);
            free(dummy);
            lhs_type = rhs_type;
            add_var(var, lhs_type);
        }
        expr_tokens = tokens + 2;
        expr_pos = 0;
        VarType rhs_type;
        char *c_expr = parse_expr(&rhs_type);
        if (lhs_type != rhs_type) {
            fprintf(stderr, "Error: type mismatch in assignment to '%s'\n", var);
            exit(1);
        }
        if (lhs_type == V_STRING)
            fprintf(out, "    strcpy(%s, %s);\n", var, c_expr);
        else
            fprintf(out, "    %s = %s;\n", var, c_expr);
        free(c_expr);
        return;
    }
}

/* ---------- file reading ---------- */
char *read_file(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) { perror("fopen"); exit(1); }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(len + 1);
    fread(buf, 1, len, f);
    buf[len] = '\0';
    fclose(f);
    return buf;
}

/* ---------- collect function definitions ---------- */
void collect_definitions(char *source) {
    char **lines = NULL;
    int line_count = 0;
    char *src_copy = strdup(source);
    char *tok = strtok(src_copy, "\n");
    while (tok) {
        lines = realloc(lines, (line_count+1) * sizeof(char*));
        lines[line_count++] = strdup(tok);
        tok = strtok(NULL, "\n");
    }
    free(src_copy);

    int *indents = malloc(line_count * sizeof(int));
    for (int i = 0; i < line_count; i++) {
        int sp = 0;
        char *l = lines[i];
        while (*l == ' ' || *l == '\t') { sp += (*l == '\t') ? 4 : 1; l++; }
        indents[i] = sp;
    }

    int i = 0;
    while (i < line_count) {
        char *line = lines[i];
        while (*line == ' ' || *line == '\t') line++;
        if (strlen(line) == 0) { i++; continue; }

        if (strncmp(line, "fn ", 3) == 0) {
            char *rest = line + 3;
            char *paren = strchr(rest, '(');
            if (!paren) { fprintf(stderr, "Error: expected '(' in function definition\n"); exit(1); }
            *paren = '\0';
            char *fname = strdup(rest);
            while (isspace(*fname)) fname++;
            int len = strlen(fname);
            while (len>0 && isspace(fname[len-1])) fname[--len]='\0';

            char *params = paren + 1;
            char *colon = strchr(params, ':');
            if (!colon) { fprintf(stderr, "Error: missing ':' in function definition\n"); exit(1); }
            *colon = '\0';
            char *param_list = params;
            int pcount = 0;
            char *p = param_list;
            while (*p) {
                if (*p != ' ' && *p != ',') pcount++;
                while (*p && *p != ',' && *p != ' ') p++;
                while (*p == ' ' || *p == ',') p++;
            }
            char **pnames = malloc(pcount * sizeof(char*));
            p = param_list;
            int idx = 0;
            while (*p) {
                while (*p == ' ' || *p == ',') p++;
                if (*p == '\0') break;
                char *start = p;
                while (*p && *p != ' ' && *p != ',') p++;
                int plen = p - start;
                pnames[idx] = malloc(plen+1);
                strncpy(pnames[idx], start, plen);
                pnames[idx][plen] = '\0';
                idx++;
            }

            FuncDef *func = malloc(sizeof(FuncDef));
            strcpy(func->name, fname);
            func->params = pnames;
            func->param_count = idx;
            func->param_types = malloc(idx * sizeof(VarType));
            for (int j=0; j<idx; j++) func->param_types[j] = V_VOID;
            func->return_type = V_VOID;

            int base_indent = indents[i];
            int j = i+1;
            while (j < line_count && indents[j] > base_indent) j++;
            int body_lines_count = j - (i+1);
            char **body = malloc(body_lines_count * sizeof(char*));
            for (int k=0; k<body_lines_count; k++) body[k] = strdup(lines[i+1+k]);
            func->body_lines = body;
            func->body_count = body_lines_count;

            func->next = func_list;
            func_list = func;
            i = j;
        } else {
            i++;
        }
    }

    for (int k=0; k<line_count; k++) free(lines[k]);
    free(lines);
    free(indents);
}

/* ---------- pre-scan: set parameter types from all function calls ---------- */
void infer_param_types_from_calls(char *source) {
    // Walk every line of the source (except function bodies) and look for function calls
    char *src_copy = strdup(source);
    char *line = strtok(src_copy, "\n");
    while (line) {
        while (*line == ' ' || *line == '\t') line++;
        if (strlen(line) == 0) { line = strtok(NULL, "\n"); continue; }
        // Skip function definitions (they'll be handled separately)
        if (strncmp(line, "fn ", 3) == 0) {
            // skip the whole function body
            int indent = 0; char *l = line;
            while (*l == ' ' || *l == '\t') { indent++; l++; }
            line = strtok(NULL, "\n");
            while (line) {
                int sp = 0; char *t = line;
                while (*t == ' ' || *t == '\t') { sp++; t++; }
                if (sp <= indent) break;
                line = strtok(NULL, "\n");
            }
            continue;
        }
        // Tokenize and search for ID followed by LPAREN
        Token tokens[100];
        int n = tokenize(line, tokens, 100);
        for (int i = 0; i < n; i++) {
            if (tokens[i].type == TOK_ID && tokens[i+1].type == TOK_LPAREN) {
                // found a function call – parse it (this will set parameter types)
                char *func_name = tokens[i].text;
                FuncDef *func = find_func(func_name);
                if (func) {
                    // We parse the whole expression (including the call) using parse_expr.
                    // This will call parse_factor, which handles the function call and sets parameter types.
                    expr_tokens = tokens + i;
                    expr_pos = 0;
                    VarType dummy_type;
                    char *dummy = parse_expr(&dummy_type);
                    free(dummy);
                }
                // else not a function we care about (maybe built-in)
            }
        }
        line = strtok(NULL, "\n");
    }
    free(src_copy);
}

/* ---------- compile main program ---------- */
void compile_main(FILE *out, char *source) {
    char **lines = NULL;
    int line_count = 0;
    char *src_copy = strdup(source);
    char *tok = strtok(src_copy, "\n");
    while (tok) {
        lines = realloc(lines, (line_count+1) * sizeof(char*));
        lines[line_count++] = strdup(tok);
        tok = strtok(NULL, "\n");
    }
    free(src_copy);

    int *indents = malloc(line_count * sizeof(int));
    for (int i = 0; i < line_count; i++) {
        int sp = 0;
        char *l = lines[i];
        while (*l == ' ' || *l == '\t') { sp += (*l == '\t') ? 4 : 1; l++; }
        indents[i] = sp;
    }

    int i = 0;
    while (i < line_count) {
        char *line = lines[i];
        while (*line == ' ' || *line == '\t') line++;
        if (strlen(line) == 0) { i++; continue; }
        if (strncmp(line, "fn ", 3) == 0) {
            int base = indents[i];
            int j = i+1;
            while (j < line_count && indents[j] > base) j++;
            i = j;
        } else {
            compile_line(line, out, 0);
            i++;
        }
    }
    for (int k=0; k<line_count; k++) free(lines[k]);
    free(lines);
    free(indents);
}

/* ---------- compile a function definition ---------- */
void compile_function(FILE *out, FuncDef *func) {
    // Scope already created during type inference, but we need a fresh one for codegen
    push_scope();
    for (int i = 0; i < func->param_count; i++)
        add_var(func->params[i], func->param_types[i]);

    // Declare local variables
    for (int i = 0; i < func->body_count; i++) {
        char *line = func->body_lines[i];
        while (*line == ' ' || *line == '\t') line++;
        Token tokens[100];
        int n = tokenize(line, tokens, 100);
        if (n >= 2 && tokens[0].type == TOK_ID && tokens[1].type == TOK_ASSIGN) {
            char *vname = tokens[0].text;
            if (find_var_type(vname) == V_VOID) {
                expr_tokens = tokens + 2;
                expr_pos = 0;
                VarType vtype;
                char *dummy = parse_expr(&vtype);
                free(dummy);
                add_var(vname, vtype);
            }
        }
    }

    for (VarInfo *v = scope_stack[scope_top]; v; v = v->next) {
        int is_param = 0;
        for (int i = 0; i < func->param_count; i++)
            if (strcmp(v->name, func->params[i]) == 0) { is_param = 1; break; }
        if (!is_param) {
            const char *ctype = "int";
            if (v->type == V_INT) ctype = "int";
            else if (v->type == V_FLOAT) ctype = "double";
            else if (v->type == V_STRING) ctype = "char[256]";
            else if (v->type == V_BOOL) ctype = "int";
            fprintf(out, "    %s %s = 0;\n", ctype, v->name);
        }
    }

    // Determine C return type
    const char *cret = "void";
    if (func->return_type == V_INT) cret = "int";
    else if (func->return_type == V_FLOAT) cret = "double";
    else if (func->return_type == V_BOOL) cret = "int";
    else if (func->return_type == V_STRING) cret = "char*";

    fprintf(out, "%s %s(", cret, func->name);
    for (int i = 0; i < func->param_count; i++) {
        const char *ctype = "int";
        if (func->param_types[i] == V_INT) ctype = "int";
        else if (func->param_types[i] == V_FLOAT) ctype = "double";
        else if (func->param_types[i] == V_BOOL) ctype = "int";
        else if (func->param_types[i] == V_STRING) ctype = "char*";
        fprintf(out, "%s %s", ctype, func->params[i]);
        if (i < func->param_count-1) fprintf(out, ", ");
    }
    fprintf(out, ") {\n");

    for (int i = 0; i < func->body_count; i++) {
        char *line = func->body_lines[i];
        while (*line == ' ' || *line == '\t') line++;
        compile_line(line, out, 1);
    }

    if (func->return_type == V_VOID) fprintf(out, "    return;\n");
    fprintf(out, "}\n");
    pop_scope();
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file.sx> [output.exe]\n", argv[0]);
        return 1;
    }

    char *source = read_file(argv[1]);
    push_scope();   // global scope

    // 1. collect function names, parameters, and body lines
    collect_definitions(source);

    // --- 2. EARLY global variable pass: only literal RHS ---
    {
        char *src_copy = strdup(source);
        char *line = strtok(src_copy, "\n");
        while (line) {
            while (*line == ' ' || *line == '\t') line++;
            if (strlen(line) == 0) { line = strtok(NULL, "\n"); continue; }
            if (strncmp(line, "fn ", 3) == 0) {
                int indent = 0; char *l = line;
                while (*l == ' ' || *l == '\t') { indent++; l++; }
                line = strtok(NULL, "\n");
                while (line) {
                    int sp = 0; char *t = line;
                    while (*t == ' ' || *t == '\t') { sp++; t++; }
                    if (sp <= indent) break;
                    line = strtok(NULL, "\n");
                }
                continue;
            }
            Token tokens[100];
            int n = tokenize(line, tokens, 100);
            if (n >= 2 && tokens[0].type == TOK_ID && tokens[1].type == TOK_ASSIGN) {
                char *vname = tokens[0].text;
                if (find_var_type(vname) == V_VOID) {
                    VarType vt = V_VOID;
                    // only handle literals (no function calls, no complex expressions)
                    if (tokens[2].type == TOK_NUM) vt = V_INT;
                    else if (tokens[2].type == TOK_FLOAT) vt = V_FLOAT;
                    else if (tokens[2].type == TOK_STR) vt = V_STRING;
                    else if (tokens[2].type == TOK_TRUE || tokens[2].type == TOK_FALSE) vt = V_BOOL;

                    if (vt != V_VOID) {
                        add_var(vname, vt);
                    }
                    // else leave it for the late pass
                }
            }
            line = strtok(NULL, "\n");
        }
        free(src_copy);
    }

    // 3. infer parameter types by scanning all function calls (now 'name' etc. exist)
    infer_param_types_from_calls(source);

    // 4. infer return types (with proper scopes so parameters are visible)
    for (FuncDef *f = func_list; f; f = f->next) {
        push_scope();
        for (int i = 0; i < f->param_count; i++)
            add_var(f->params[i], f->param_types[i]);
        VarType ret_type = V_VOID;
        for (int i = 0; i < f->body_count; i++) {
            char *line = f->body_lines[i];
            while (*line == ' ' || *line == '\t') line++;
            if (strncmp(line, "ret ", 4) == 0) {
                Token tokens[100];
                tokenize(line + 4, tokens, 100);
                if (tokens[0].type != TOK_EOF) {
                    expr_tokens = tokens;
                    expr_pos = 0;
                    VarType etype;
                    char *dummy = parse_expr(&etype);
                    free(dummy);
                    if (ret_type == V_VOID) ret_type = etype;
                    else if (ret_type != etype) {
                        fprintf(stderr, "Error: inconsistent return types in '%s'\n", f->name);
                        exit(1);
                    }
                }
            }
        }
        f->return_type = ret_type;
        pop_scope();
    }

    // 5. LATE global variable pass: function‑call assignments (res = add(3,4))
    {
        char *src_copy = strdup(source);
        char *line = strtok(src_copy, "\n");
        while (line) {
            while (*line == ' ' || *line == '\t') line++;
            if (strlen(line) == 0) { line = strtok(NULL, "\n"); continue; }
            if (strncmp(line, "fn ", 3) == 0) {
                int indent = 0; char *l = line;
                while (*l == ' ' || *l == '\t') { indent++; l++; }
                line = strtok(NULL, "\n");
                while (line) {
                    int sp = 0; char *t = line;
                    while (*t == ' ' || *t == '\t') { sp++; t++; }
                    if (sp <= indent) break;
                    line = strtok(NULL, "\n");
                }
                continue;
            }
            Token tokens[100];
            int n = tokenize(line, tokens, 100);
            if (n >= 2 && tokens[0].type == TOK_ID && tokens[1].type == TOK_ASSIGN) {
                char *vname = tokens[0].text;
                if (find_var_type(vname) == V_VOID) {
                    VarType vt;
                    // only process function calls or other complex expressions now
                    if (tokens[2].type == TOK_ID && tokens[3].type == TOK_LPAREN) {
                        FuncDef *func = find_func(tokens[2].text);
                        if (func) vt = func->return_type;
                        else {
                            expr_tokens = tokens + 2;
                            expr_pos = 0;
                            char *dummy = parse_expr(&vt);
                            free(dummy);
                        }
                    } else {
                        expr_tokens = tokens + 2;
                        expr_pos = 0;
                        char *dummy = parse_expr(&vt);
                        free(dummy);
                    }
                    add_var(vname, vt);
                }
            }
            line = strtok(NULL, "\n");
        }
        free(src_copy);
    }

    // 6. generate C code (functions first, then main)
    FILE *cfile = fopen("temp.c", "w");
    if (!cfile) { perror("fopen temp.c"); free(source); return 1; }

    fprintf(cfile, "#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n\n");

    for (VarInfo *v = scope_stack[0]; v; v = v->next) {
        if (v->type == V_INT) fprintf(cfile, "int %s = 0;\n", v->name);
        else if (v->type == V_FLOAT) fprintf(cfile, "double %s = 0.0;\n", v->name);
        else if (v->type == V_STRING) fprintf(cfile, "char %s[256] = \"\";\n", v->name);
        else if (v->type == V_BOOL) fprintf(cfile, "int %s = 0;\n", v->name);
    }

    for (FuncDef *f = func_list; f; f = f->next)
        compile_function(cfile, f);

    fprintf(cfile, "\nint main() {\n");
    compile_main(cfile, source);
    fprintf(cfile, "    printf(\"\\nPress Enter to exit...\");\n");
    fprintf(cfile, "    while (getchar() != '\\n');\n");
    fprintf(cfile, "    getchar();\n");
    fprintf(cfile, "    return 0;\n}\n");

    fclose(cfile);

    char out_exe[256] = "output.exe";
    if (argc >= 3) strcpy(out_exe, argv[2]);
    char cmd[512];
    sprintf(cmd, "gcc temp.c -o %s", out_exe);
    printf("Compiling... %s\n", cmd);
    if (system(cmd) != 0) {
        fprintf(stderr, "GCC compilation failed.\n");
        return 1;
    }
    printf("Running %s...\n", out_exe);
    sprintf(cmd, "%s", out_exe);
    system(cmd);

    remove("temp.c");
    free(source);
    pop_scope();
    return 0;
}