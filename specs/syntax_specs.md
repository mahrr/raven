## Syntax Specification

```EBNF

program ::= chunk;

chunk ::= {declaration} [expression];

declaration ::= let_declaration
              | fn_declaration
              | statement;

let_declaration ::= "let" name "=" expression;

fn_declaration ::= "fn" name "(" parameter_list ")" chunk "end";

statement ::= assert_statement
            | expression_statement
            | return_statement
            | "continue"
            | "break";

assert_statement ::= "assert" expression;

expression_statement ::= expression;

return_statement ::= "return" [expression];

expression ::= prefix_expression
             | infix_expression
             | block_expression
             | index_expression
             | dot_expression
             | group_expression
             | call_expression
             | if_expression
             | while_expression
             | cond_expression
             | identifier_expression
             | literal_expression;

prefix_expression ::= prefix_operator expression;

infix_expression ::= expression infix_operator expression;

block_expression ::= "do" chunk "end";

group_expression ::= "(" expression ")";

index_expression ::= expression "[" expression "]";

dot_expression ::= expression "." expression;

call_expression ::= expression "(" [expression_list] ")";

if_expression ::= "if" expression "do" chunk ["else" chunk] "end";

while_expression ::= "while" expression "do" chunk "end";

cond_expression ::= "cond:" expression -> expression
                      [{"," expression -> expression}]
                    "end";

identifier_expression ::= name;

literal_expression ::= lambda_literal
                     | array_literal
                     | map_literal
                     | INTEGER
                     | FLOAT
                     | STRING
                     | TRUE
                     | FALSE
                     | NIL;

lambda_literal ::= ["\" parameter_list] "->" expression;

array_literal ::= "[" [expression_list] "]";

map_literal ::= "{" [map_field {"," map_field}] "}";

map_field ::= map_key ":" expression
             | name;

map_key ::= "[" expression "]"
           | name;

parameter_list ::= [name {',' name}];

expression_list ::= expression {"," expression};

prefix_operator ::= "-" | "not";

infix_operator ::= "." | "+" | "-" | "*" | "/" | "%" | "@" | "="
                 | "<" | ">" | "<=" | ">=" | "==" | "!=" | "::"
                 | "and" | "or";

name ::= IDENTIFIER;

```

## Operators and Precedence

Operator precedence follows this table, from higher to lower priority.

Operator | Function    | Associations
---------|-------------|-------------
() [] .   | Group, Index, Dot       | left
not -     | Not, Negate             | right
\* / %    | Multply, Divide, Module | left
\+ -      | Add, Subtract           | left
@         | Concatenation           | left
::        | list Cons               | right
< > <= >= | Comparsion              | left
== !=     | Equality                | left
and       | Logical AND             | left
or        | Logical OR              | left
=         | Assign                  | right
