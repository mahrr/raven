# Syntax Specification

```EBNF
program ::= piece;

piece ::= {statement} [return_statment];

statement ::= decl_statement
            | assign_statement
            | expr_statement
            | fixed_statement;

ret_statement ::= "return" expression delimiter;

decl_statement ::= let_statement
                 | fn_statement;

assign_statement ::= name_expr "=" expression delimiter;

expr_statement ::= expression delimiter;

fixed_statement ::= "break" | "continue";

let_statement ::= "let" pattern "=" expr_list delimiter;

fn_statement ::= "fn" name param_list piece "end";

name_expr ::= name
            | name_prefix "[" expression "]"
            | name_prefix "." name;

name_prefix ::= name_expr
              | group_expression
              | call_expression;

param_list ::= "(" [pattern {, pattern}] ")";

expression ::= literal_expression
             | prefix_expression
             | infix_expression
             | index_expression
             | group_expression
             | call_expression
             | if_expression
             | for_expression
             | while_expression
             | match_expression
             | ident_expression;

expr_list ::= expression [{, expression}];

literal_expression ::= fn_literal
                     | list_literal
                     | record_literal
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

group_expression ::= "(" expression ")";

call_expression ::= expression "(" expr_list ")";

if_expression ::= "if" expression "do" piece 
  {"elif" expression "do" piece} [else piece] "end";

for_expression ::= "for" name "in" expression "do" piece "end";

while_expression ::= "while" expression "do" piece "end";

match_expression ::= "match" expression "do" 
  {"case" pattern "->" (expression | "do" piece "end")} "end";

ident_expression ::= name;

fn_literal ::= "fn" "(" param_list ")" piece "end";

list_literal ::= "[" [expression {, expression}] "]";

record_literal ::= "{" [record_field {, record_field}] "}";

record_field ::= name ":" expression;

pattern ::= const_pattern
          | ident_pattern
          | list_pattern
          | record_pattern;

const_pattern ::= INTEGER
                | FLOAT
                | STRING
                | RSTRING
                | TRUE
                | FALSE
                | NIL;

ident_pattern ::= name;

list_pattern ::= "[" [pattern {, pattern}] "]"
               | pattern "::" pattern;

record_pattern ::= "{" [pattern_field {, pattern_field}] "}";

pattern_field ::= name ":" pattern;

prefix_op ::= "-" | "~" | "not";

infix_op ::= "." | "+" | "-" | "*" | "/" | "%" | ".." | "@"
           | "::" | "<<" | ">>" | "&" | "^" | "|" | "<" 
           | ">" | "<=" | ">=" | "==" | "!=" | "and" | "or";

name ::= IDENTIFIER;

delimiter ::= ";" | NEWLINE;
```