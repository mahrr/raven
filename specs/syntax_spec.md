# Syntax Specification

```EBNF
program ::= piece;

piece ::= {statement} [ret_statment];

statement ::= decl_statement
            | expr_statement
            | fixed_statement;

ret_statement ::= "return" expression delimiter;

decl_statement ::= type_statement
                 | let_statement
                 | fn_statement;

expr_statement ::= expression delimiter;

fixed_statement ::= ("break" | "continue") delimiter;

type_statement ::= "type" name "=" data_variants "end";

data_variants ::= cons_declaration {cons_declaration};

cons_declaration ::= "|" name ["(" name {"," name} ")"];

let_statement ::= "let" pattern "=" expression delimiter;

fn_statement ::= "fn" name param_list piece "end";

param_list ::= "(" [pattern {"," pattern}] ")";

expression ::= access_expression
             | prefix_expression
             | infix_expression
             | index_expression
             | literal_expression
             | group_expression
             | call_expression
             | if_expression
             | for_expression
             | while_expression
             | cond_expression
             | match_expression
             | ident_expression;

expr_list ::= [expression {"," expression}];

literal_expression ::= fn_literal
                     | list_literal
                     | hash_literal
                     | INTEGER
                     | FLOAT
                     | STRING
                     | RSTRING
                     | TRUE
                     | FALSE
                     | NIL;

prefix_expression ::= prefix_op expression;

infix_expression ::= expression infix_op expression;

index_expression ::= expression "[" expression "]";

access_expression ::= expression "." name;

group_expression ::= "(" expression ")";

call_expression ::= expression "(" expr_list ")";

if_expression ::= "if" expression "do" piece 
  {"elif" expression "do" piece} ["else" piece] "end";

for_expression ::= "for" pattern "in" expression "do" 
    piece "end";

while_expression ::= "while" expression "do" piece "end";

cond_expression ::= cond {"|" expression arm_branch} "end";

match_expression ::= "match" expression "do" 
    {"|" pattern arm_branch } "end";

arm_branch ::= "->" (expression | "do" piece "end");

ident_expression ::= name;

fn_literal ::= "fn" "(" param_list ")" piece "end";

list_literal ::= "[" expr_list "]";

hash_literal ::= "{" [hash_field {"," hash_field}] "}";

hash_field ::= hash_key ":" expression
             | name;

pattern ::= const_pattern
          | ident_pattern
          | cons_pattern
          | pair_pattern
          | list_pattern
          | hash_pattern
          | "(" pattern ")";

const_pattern ::= INTEGER
                | FLOAT
                | STRING
                | RSTRING
                | TRUE
                | FALSE
                | NIL;

ident_pattern ::= name;

cons_pattern ::= name ["(" pattern {"," pattern} ")"];

list_pattern ::= "[" [pattern {"," pattern}] "]";

pair_pattern ::= pattern "::" pattern;

hash_pattern ::= "{" [pattern_field {"," pattern_field}] "}";

pattern_field ::= hash_key ":" pattern
                | name;

hash_key ::= "[" expression "]" 
           | name;
           
prefix_op ::= "-" | "not";

infix_op ::= "." | "+" | "-" | "*" | "/" | "%" | "@" | "="
           | "<" | ">" | "<=" | ">=" | "==" | "!=" | "::"
           | "and" | "or";

name ::= IDENTIFIER;

delimiter ::= ";" | NEWLINE;
```
