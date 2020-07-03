## Syntax Specification

```EBNF

script ::= chunk;

chunk ::= {declaration} [expression];

declaration ::= let_declaration
              | fn_declaration
              | type_declaration
              | statement;
              
let_declaration ::= "let" pattern "=" expression delimiter;

fn_declaration ::= "fn" name "(" parameter_list ")" chunk "end";

type_declaration ::= "type" name ":" type_variant [{"|" type_variant}] "end";

type_variant ::= name "("[name {"," name} ]")";

statement ::= expression_statement
            | return_statement
            | "break" delimiter;
            
expression_statement ::= expression delimiter;

return_statement ::= "return" [expression] delimiter;

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
                     | hash_literal
                     | INTEGER
                     | FLOAT
                     | STRING
                     | TRUE
                     | FALSE
                     | NIL;
                     
lambda_literal ::= "|" parameter_list "|" "->" expression;

array_literal ::= "[" [expression_list] "]";

hash_literal ::= "{" [hash_field {"," hash_field}] "}";

hash_field ::= hash_key ":" expression
             | name;

pattern ::= constant_pattern
          | identifier_pattern
          | type_pattern
          | pair_pattern
          | array_pattern
          | hash_pattern
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

hash_pattern ::= "{" [pattern_field {"," pattern_field}] "}";

pattern_field ::= hash_key ":" pattern
                | name;

hash_key ::= "[" expression "]"
           | name;

parameter_list ::= [pattern_list];

expression_list ::= expression {"," expression};

pattern_list ::= pattern {"," pattern};

prefix_operator ::= "-" | "not";

infix_operator ::= "." | "+" | "-" | "*" | "/" | "%" | "@" | "="
                 | "<" | ">" | "<=" | ">=" | "==" | "!=" | "::"
                 | "and" | "or";

name ::= IDENTIFIER;

delimiter ::= ";" | NEW_LINE;

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
