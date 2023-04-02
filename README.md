# Raven

Raven is simple dynamically-typed programming language, the goal of the project was to experiment with different language constructs regarding design and implementations.

## Implementation

The language execution model is very simple, the source code is compiled to bytecode which is executed by a stack-based [virtual machine](src/vm.c), the total count of the virtual machine [opcodes](src/opcode.h) is 45 right now. The [compiler](src/compiler.c) has only one pass with handwritten recursive-descent [parser](src/compiler.c) and [lexer](src/lexer.c), the expressions are parsed with a [Pratt parser](https://en.wikipedia.org/wiki/Operator-precedence_parser#Pratt_parsing).

The language is implemented in portable C99, the implementation uses some constructs that are not standard C, like computed goto to speed up the VM dispatch loop, to use this feature, you need a GNU99 C compiler (e.g., GCC, Clang). The implementation also uses NaN boxing to encode the language [values](src/value.h), this is only available if you're compiling on x86_64 systems.

## The language

The language is an expression-based one, so the most of the language constructs are expressions. A Raven program is an optional sequence of declarations, `let` for variable declarations and `fn` for functions, and a sequence of expressions, the two could interleave, the vm evaluates all declarations/expressions in sequence and prints the value of last evaluated expression. The language has no standard library, as it wasn't meant for real-world use, only for experimenting, for that, there are some native functions implemented in the host language like `print` for printing stuff on the standard output, `import` for importing values from other files, and `assert` for debugging. Raven has the usual programming language constructs:

```
# Primitives Values

nil              # the billion dollar mistake
true false       # booleans
1                # numbers (all numbers are represented as IEEE 754 double precision floating point)
3.14159265359
0xffff
0o5346
0b1010101

# Reference Values

"Hello, World!"          # strings use double quotation, all strings are interned
"sqrt(x) = {{sqrt(x)}}"  # interpolated strings

1 :: 2                   # cons pairs (à la Lisps)
"foo" :: false :: nil

["foo", "bar", "baz"]    # dynamic arrays

{x: 5.9, y: 1.6}         # maps, only string-keys are supported

\x, y -> x + y           # lambdas
\x -> x * x

# Numerical Binary Expressions

x + y
x - y
x * y
x / y
x % y

# Numerical Comparison Expressions

x < y
x > y
x <= y
x >= y

# Comparison Expression

x == y               # identity comparison
"foo" == "foo"       # true, all strings are interned, so they are the same objects
["foo"] == ["foo"]   # false, they are different objects

# Access Expressions

let array = [1, 2, 3]
array[0]                 # 1
array[3]                 # runtime error

let object = {x: 3, y: 4}
object.x                 # 3
object.y                 # 4
object.x = 0             # field assignment

# Conditional Expressions

if foo do 1 else 2 end    # 1 if foo is true, 2 otherwise
if foo do 1 end           # 1 if foo is true, nil otherwise

while i < counter do      # while expression is always nil
    println("count = {{i}}")
end

# Evaluates to the right-side value that corresponds to first
# left-side value that evaluates to true
cond
    x == 0 -> "zero",
    x == 1 -> "one",
    x == 2 -> "two",
    x == 3 -> "three",
    true -> "bigger than three"
end

# Pattern Matching

# Matches `x` with the given patterns, it evaluates to the right
# side of the first matched pattern in an environment where the
# the names in the id patterns are binded recursively to the
# corresponding values in `x`

# literal patterns
match x do
    nil   -> "nil",
    true  -> "true",
    false -> "false,
    1     -> "int",
    "one" -> "string",
end

# wildcard pattern and id pattern (always matches)
match x do
    _   -> "wild card pattern",
    foo -> "id pattern",
end

# compound patterns
match x do
    (head :: tail) -> "pair pattern",
    [first, second, third] -> "array pattern",
    {key1: foo, key2: 1} -> "map pattern"
end

# patterns can be nested
let object = {foo: [1, 2, 3], bar: 4 :: 5}

# this evaluates to 6
match object
    {foo: [_, x, 3], bar: (y :: 5)} -> x + y, # 2 + 4
    _ -> "no match"
end

# Functions

# Very naive way to implement the Fibonacci function
fn fib(number)
    if number < 2 do
        number
    else
        fib(number - 1) + fib(number - 2)
    end
end

fib(35) # 9.22746e+06

# Functions can captures variables from parent scopes (Closures)
fn map(seq, mapper)
    let result = []
    let length = len(seq)
    let i = 0

    while i < length do
        push(result, mapper(seq[i]))
    end

    result
end

let increment = 1
map([1, 2, 3], \x -> x + increment) # [2, 3, 4]

# Importing mechanism

# File: math.rav

# Approximation of the sine function
fn sin(angle)
    let p = \x -> (x * 3) - x * x * x * 4
    let abs = \x -> if x < 0 do -x else x end

    cond
        abs(angle) <= 0.1 -> angle,
        true -> p(sin(angle/3))
    end
end

# Raven files exports the last evaluated expression in the file either
# by using `return` in the top-level scope, or by positioning the
# expression to be the last one in the file, you can acquire this
# expression's value in other files by calling the `import` function
# passing the name of this file

{sin: sin}  # or return {sin:sin}

# File: main.rav

# Note that `import` is a regular function
let math = import("math.rav")

print("sin(60) = {{math.sin(60)}}")  # sin(60) = -0.3687717227358598

```

## Build

On Unix systems the only requirement is GNU make:

```bash
$ make debug                         # build in debug mode
$ make release                       # build in release mode
$ make release_symbols               # build in release mode with debug symbols

$ ./build/release/raven              # starts a REPL session
$ ./build/release/raven script.rav   # executes the given script, multiple files are not supported
```

## Credits

The method used to implement closures in the language is based on the one designed by the [Lua](https://www.lua.org/home.html) language team, detailed explanation of how this method work could be found in their amazing paper ['Closures in Lua'](https://www.cs.tufts.edu/~nr/cs257/archive/roberto-ierusalimschy/closures-draft.pdf). The virtual machine implementation is based on the one implemented in the [CLox](https://github.com/munificent/craftinginterpreters/tree/master/c) language from  Bob Nystrom's great book, [Crafting Interpreters](https://craftinginterpreters.com/).