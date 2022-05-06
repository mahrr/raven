## Lexical Specification

```EBNF

token ::= keyword
        | identifier
        | integer
        | float
        | string
        | string_begin
        | string_part
        | string_end
        | operator
        | delimiter;

keyword ::= "break"
          | "cond"
          | "else"
          | "false"
          | "fn"
          | "for"
          | "if"
          | "let"
          | "match"
          | "nil"
          | "return"
          | "while"
          | "true"
          | "type";

identifier ::= (alpha | "_") (decimal_digit | alpha | "_")*;

integer ::= decimal_digit+
          | "0b" binary_digit+
          | "0o" octal_digit+
          | "0x" hexadecimal_digit+;

float ::= decimal_digit+ "." decimal_digit+;

string ::= "'" <ASCII character> "'"
         | "\"" <ASCII character>  "\"";

string_begin ::= "\"" <ASCII character> "|";

string_part ::= "|" <ASCII character> "|";

string_end ::= "|" <ASCII character>; "\"";

operator ::= "and" | "not" | "or"
           | "()"  | "[]"  | "."
           | "*"   | "/"   | "%"
           | "+"   | "-"   | ".."
           | "::"  | "<"   | ">"
           | "<="  | ">="  | "=="
           | "!="  | "=";

delimiter ::= "do" | "end" | "in"
            | "="  | "->"  | "("
            | ")"  | "["   | "]"
            | ","  | ":";

comment ::= "#" <extends until the end of the line>;

hexadecimal_digit ::= decimal_digit
                    | "a" ... "f"
                    | "A" ... "F";

decimal_digit ::= oct_digit
                | "8" | "9";

octal_digit ::= "0" ... "7";
binary_digit ::= "0" | "1";

alpha ::= "a" ... "z"
        | "A" ... "Z";

```
