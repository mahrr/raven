# Lexical Specification

## Encoding

raven reads an ASCII coded text. UTF-8 is consedred for
future versions.

<br>

## Comments

Short comments start with `#` and end with the end of the current line.

```
# this is a comment
```

Long comments start with `#-` and end with `-#`.

```
#-
  a multiple
  line 
  comment
-#
```

Long comments can be nested.

```
#- a nested #- comment -#-#
```

<br>

## Identifiers

```EBNF
identifier ::= (alpha | '_') (digit | alpha | '_')* ;
```

raven has the traditional naming rules. Identifiers start
with letter or underscore and then may extends to letters,
digits and underscores.

<br>

## Literals

### Integer literals

```EBNF
integer   ::= dec_digit+
            | '0b' bin_digit+
            | '0o' oct_digit+
            | '0x' hex_digit+ ;

hex_digit ::= dec_digit
            | 'a' ... 'f'
            | 'A' ... 'F' ;
dec_digit ::= oct_digit 
            | '8' | '9';
oct_digit ::= '0' ... '7' ;
```

Hexidicemal, octal and binary forms are supported.

### Float literals

```EBNF
float         ::= point | exponent ;

point         ::= digit+ '.' digit+ ;
exponent      ::= digit_part exponent_part ;

digit_part    ::= digit ('.' digit)? ;
exponent_part ::= ('e' | 'E') ('-' | '+')? digit+ ;
```

Scientific notation is supported.

Note that the `-` in the scientific notation is a part of the float literal token. but if the `-` was leading a numeric literal, it's considered an prefix operator not part of the literal token.

### String literals

raven support escaped strings literals and raw string literals.

Escaped strings use double and single qoutes and raw strings use backticks.

```EBNF

escaped_string ::= '"' non_dq_char*  | escape_seq* '"'
                 | '\'' non_sq_char* | escape_seq* '\'' ;

raw_string     ::= '`' non_bt_char* '`' ;

non_dq_char    ::= <any ASCII char expect double qoute or newline or back slash> ;

non_sq_char    ::= <any ASCII char expect single qoute or newline or back slash> ;

non_bt_char    ::= <any ASCII char expect backtick> ;

escape_seq     ::= '\\' escape_form ;
escape_form    ::= escape_char
                 | escape_oct
                 | escape_hex ;

escape_char    ::= 'a' | 'b' | 'f' | 'n'
                 | 'r' | 't' | 'v' | '"'
                 | '\'' | '\\' ;

escape_oct     ::= oct_digit oct_digit_oct_digit ;
escape_hex     ::= hex_digit hex_digit ;

```

Escape sequences are interpreted according to the same rules applied in the [C standard](https://en.wikipedia.org/wiki/Escape_sequences_in_C).

### Fixed literals

```
true false nil
```

<br>

## Reserved

The following are the langauge keywords. They Can't be used as an identifier name.

```
break case continue do elif else end false 
fn for if in let match nil return while true
```

<br>

## Operators and Precedence

 Operator precedence follows this table ,from higher to lower priority.

Operator | Function    | Associations
---------|-------------|-------------
() [] .   | Group, Index, Access    | left
not -     | Not, Negate             | right
\* / %    | Multply, Divide, Module | left
\+ -      | Add, Subtract           | left
@         | Concatenation           | left
\|        | list Cons               | right
< > <= >= | Comparsion              | left
== !=     | Equality                | left
and       | Logical AND             | left
or        | Logical OR              | left
=         | Assign                  | right

<br>

## Delimiters

```
do end = -> ( ) [ ] { } ; , NL
```

Usually newlines could be used as statements delimiters, but you can also use semicolons to seperate statements on the same line.
