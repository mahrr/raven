## Syntax Specification

```EBNF

program ::= chunk;

chunk ::= {declaration} [expression];

declaration ::= let_declaration
              | fn_declaration
              | type_declaration
              | statement;

let_declaration ::= "let" pattern "=" expression;

fn_declaration ::= "fn" name "(" parameter_list ")" chunk "end";

type_declaration ::= "type" name ":" type_variant [{"|" type_variant}] "end";

type_variant ::= name "("[name {"," name} ]")";

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
             | for_expression
             | while_expression
             | cond_expression
             | match_exrepssion
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

for_expression ::= "for" pattern "in" expression "do" chunk "end";

cond_expression ::= "cond:" expression -> expression
                      [{"," expression -> expression}]
                    "end";

match_expression ::= "match" expression ":"
                             pattern -> expression
                       [{"," pattern -> expression}]
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

pattern ::= constant_pattern
          | identifier_pattern
          | type_pattern
          | pair_pattern
          | array_pattern
          | map_pattern
          | "(" pattern ")";

constant_pattern ::= INTEGER
                   | FLOAT
                   | STRING
                   | TRUE
                   | FALSE
                   | NIL;

identifier_pattern ::= name;

type_pattern ::= name ["(" pattern_list ")"];

pair_pattern ::= pattern "::" pattern;

array_pattern ::= "[" [pattern_list] "]";

map_pattern ::= "{" [pattern_field {"," pattern_field}] "}";

pattern_field ::= map_key ":" pattern
                | name;

map_key ::= "[" expression "]"
           | name;

parameter_list ::= [pattern_list];

expression_list ::= expression {"," expression};

pattern_list ::= pattern {"," pattern};

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
