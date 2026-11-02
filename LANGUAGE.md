# do Language

The `do` language is the language that powers the kidDo's dynamic execution system. The language is designed to be simple to learn and use, especially with the on-device editors. 

## Lexical

Instead of symbollic or keyword based block delimiters, _doLang_ uses indentation for block nesting as this provides cleaner mapping to the chevron based editors. Whitespace in any other position that the beginning of the line is ignored. 

Comments start with `#` characters and are always single line, i.e. from the comment character to the end of the line.

### Literal Values

Boolean values `true` and `false` are available. Text must be enclosed in double quotes (`"`) with standard support for escape characters (`\n`, `\r`, `\t`, `\\`, `\"`) and character encoding `\imm` where `imm` is valid natural number immediate.

Numbers can be entered in either integer, or floating point notation, but both are stored internally as rational numbers. Rational values cannot be entered as immediate values, but can be constructed via the division operator, e.g. `2/3` for two thirds, etc. Floating points will be converted to their rational representation, i.e. `1.567` will become `1567/1000`.

Decimal, hexadecimal `0x`, binary `0b` and octal `0o` values are supported for integers. Underscore can be used to separate thousands and their multiplies, i.e. `1_000_000`.

As there is no character type, single characters can be represented as text values of length 1. 

### Keywords

The following are reserved keywords and cannot be used as identifiers:

    and, bool, break, clone, const, constructor, continue, do, else, enum, event, extends, false, for, fun, global, if, in, is, let, match, none, not, num, optional, or, return, self, step, test, text, then, true, type, use, while, write, xor

> TODO and others. Note that `xor` is reserved for the future bitwise operators.

### Indentation

_doLang_ uses indentation to denote nesting. Statements in a block all start at the same column and a block ends when a line starts at a smaller column. Apart from that, line breaks are allowed anywhere and a statement can continue on the next line, if the continuation is indented *more* than the beginning of the statement:

    let ok = lives > 0 and not gameOver
        and score < limit
    foo() # starts at the same column, so it is a new statement

This means that a line starting at the same column as the statement always starts a new statement, so there is no confusion whether the line belongs to the previous statement or not.

Inside `()`, `[]` and `{}` parentheses, the same rule applies to the items of the list: a line starting at the same column as the first item starts a new item, while a line indented more continues the current item. The closing parenthesis, if it is the first token on a line, must be at the same column as the line with the opening parenthesis. Items written on the same line are separated by commas. When items are on separate lines, the commas are optional and a trailing comma after the last item is allowed. This means that any list can be written either as a one-liner, or broken out on different lines:

    foo(a, b, c)
    # or
    foo(
        a
        b
        c
    )
    # or, with optional commas
    foo(
        a,
        b,
        c,
    )

Since the items follow the same rule as statements, a line at the item column is always a new item:

    foo(
        a
        -b      # second argument, not a - b
        + c     # continues the second argument, i.e. -b + c
    )

This works in every context, so for instance map definition can look like this:

    let m = { "a" -> 1, "b" -> 2, "c" -> 3 }
    # or
    let m = {
        "a" -> 1
        "b" -> 2
        "c" -> 3
    }

Blocks (bodies of `if`, loops, functions, etc.) always start on a new line that is indented more than the statement that introduced them. The statements that have a block end with a keyword (`if ... then`, `while ... do`, `for ... do`, `is ... then`), so their headers can span multiple lines as well. Function bodies always follow the `fun` keyword, either directly, or after the function signature (`fun(...)`). Function bodies use indentation inside parentheses too. Such a body starts on a new line indented more than the line where it was introduced and ends at the first line indented less than the body. As the next item starts at the item column, no comma is needed after the body:

    type Player = (
        onHit = fun(damage : num)
            if damage > 10 then
                playSound(ouch)
            self.lives -= damage
        lives = 3
        constructor = fun
            test self.lives > 0
    )

It is always the indentation that decides where an item ends, the comma is only a separator between items. It is therefore possible to write the comma at the end of the last line of an item, regardless of whether that line is a continuation of a multi-line expression, or the last line of a function body:

    foo(
        a
            + b,     # comma after the multi-line item a + b
        c
    )

    type Player = (
        onHit = fun(damage : num)
            if damage > 10 then
                playSound(ouch),     # comma after the whole onHit body
        lives = 3
    )

A comma at the end of a line must be followed by a line at the item column, or by the closing parenthesis. A line indented more than the item column after such comma is an error, as the comma says a new item follows, while the indentation says the current item continues.

A block of statements is always introduced by a keyword (`then`, `do`, `fun`, or the function signature following it), never by an operator. In particular, a newline after `=` (or any other assignment operator) is always a continuation of the statement, regardless of the type of the assignment target. Function bodies must always start with the `fun` keyword:

    d =
        1     # continuation of the statement, d is assigned 43
        + 42
    n = fun
        1     # function body, block of 2 statements (1 and + 42)
        + 42

## Built-in Types

_doLang_ supports rational numbers, boolean values, text strings and simple enumerations as primitive types. Generic arrays, maps and tuples are supported as well. Contrary to mainstream languages, floating point values are *not* supported. 

### `num`

The basic numeric type. All numbers in the language are rational numbers underneath. Uses the `/` (division) character to differentiate between numerator and denominator. This works nicely as the division is *exactly* what constructs the rational number. 

When the numbers are printed or visualized, the same notion is used, i.e. `3/4`.

An iconic representation of a number is the number itself. 

> I am thinking as an extension, very small numbers can be displayed as the number of the dots.

### `bool`

Boolean values, `true` or `false`.

### `text`

The "string" type for the language, uses double quotes. Everything is UTF-8 encoded. 

### `enum`

_doLang_ supports simple enums (similar to C++ `enum class`) with named options, but without an ability of the options to contain values. Keyword `enum` is used to declare new enum types and distinct enum values are separated by `|`:

    enum Btn = 
        Up | Down | Left | Right | A | B | Select | Start

To specify enum values one can either use the `EnumName.Value` syntax, or can use `Value` only for shorthand. Note that this does not make the value names available in global scope in general as those values can only be used when assigning or comparing the enum.

### `array`

A simple array. Array is generic so we can have arrays of other types and even arrays of arrays. Arrays are created with `[]`, i.e. :

    let a : [num]

All types in the array have to be identical. Type inference can be used where the type of the first item will be the type all types of the array. Arrays are growable and shrinkable and support a few methods themselves.

Inline array initialization is possible with the `[]` parentheses as well:

    let a = [1, 2, 3]

### `tuple`

Tuple is sequence of *named* values. Tuples are value types, i.e. they are copied when assigned, passed as arguments or returned:

    let t : ( foo : num, bar : num) = ( foo = 1, bar = 56)

### `map`

This is the associative array. Generic on key and value. Maps are created with `{}`, i.e. the following is a map from number to text:

    let m : {num -> text}

All types in the map have to be identical. Type inference can be used from the first type. The `{}` parentheses can also be used for inline initialization, such as:

    let m = {
        1 -> "foo",
        2 -> "bar",
    }

### Special value types

_doLang_ supports a variety of special purpose value type, but object-like types, such as `Point`, `Color`, etc. They can be used the same way user defined objects can be, but they are *not* user defined objects, cannot be inherited from, cannot have extra methods created, etc. And unlike the user defined types, those are value types, i.e. passed, returned and assigned by value.

## Functions

Functions can be either named:

    fun myFirstFunction(arg : num)
        ...

or anonymous:

    let a = fun(arg : num)
        ...

A function _body_ is always an indented block of statements starting on a new line, and always follows the `fun` keyword. There is no `=` between the signature and the body. The anonymous function can be written in two forms:

- with explicit signature, i.e. `fun(...)` with optional return type, followed by the body
- with implicit signature, i.e. just `fun` followed by the body, in which case the signature is taken from the type known from the context, such as function and event variables, or object constructors

The following are all equivalent ways of creating a function variable `a`:

    let a : fun(arg : num) = fun
        ...

    let a : fun(arg : num) = fun(arg)
        ...

    let a = fun(arg : num)
        ...

If the function returns value, the `->` notation is used, e.g.:

    fun min(a : num, b: num) -> num
        if a <= b then
            return a
        else
            return b

When the `->` part is omitted the function type is expected to not return anything (`void` in C/C++).

Function that is variable can be assigned a lambda, existing global function, or method of an particulkar object as long as the arguments arity, their types and return value are identical. Function variable must *always* pointto a valid function and hence be callable. For otentionally empty function-like variables one can use events instead. 

### Argument names

Contrary to most mainstream languages, names are described when typing the function, but are *not* part of the function type itself, but its signature. Function compatibility depends only on the number of arguments (arity) and their types as well as the return type of the function. Argument names affect keyword calls, editor visualizaton and implicit bindings: When a body is given with the implicit signature (`fun` without parentheses), the arguments are automatically bound under the names from the type. This means that the following code is valid:

    let inc : fun(x : num) -> num
    inc = fun
       return x + 1 # x is bound from inc's type

This is very useful for function variables and events, especially those declared elsewhere (such as in objects or libraries), as the handler can be written without repeating the signature:

    button.onPress = fun
        ... # arguments of onPress available under their declared names

However, it is also possible to rename the arguments by providing the signature explicitly in parentheses, as long as it is compatible with the type. So continuing the example from above, one can write:

    inc = fun(a : num) -> num
        return a + 1

Since the function type is already known from `inc`'s definition, type inference can be used and a shorter version without argument types can be typed:

    inc = fun(a)
        return a + 1

When the signature is given, it must list *all* the arguments, i.e. it is not possible to rename only some of them and keep the others implicit.

Implicitly bound arguments must not hide any other variable visible at the place of the body. For example, if there is a global variable `x`, the `inc = fun` body from above is an error and the arguments must be renamed using the explicit signature. This prevents the confusing situation where `x` in the body looks like the global variable, but is in fact the argument.

> The editors should always display the implicitly bound arguments, e.g. as a greyed out signature above the body, so that the argument names are visible even if they are not part of the code.

Functions can have default values for some of their arguments, in which case those do not have to be specified when calling a function. The default value is part of the _type_. When a function is called, arguments can also be provided using their names as per the type, i.e. we can call all of the `inc` functions from above using this:

    inc(x = 56)

### Optional return types

A function return type can be declared as `optional`, which means the function may either return value of the given type, or `none`. Functions like this can only even be called using the `if let =` syntax and they serve as a primitive maybe type - i.e. functions (including user defined ones) can return it and we force the matching and check, but the optional values cannot be propagated, or stored. This makes the feature a lot simpler to understand:

    fun safeDivide(a : num, d : num) -> optional num
        if d != 0 then
            return a / d
        else 
            return none

### Calling Functions

When a function is called, all non-default arguments must be given value. Arguments can be specified either positionally, or via their names (as described in the function type itself). Argument matching can start positional and then shift to keyword based:

    foo(1, 2, bar = 67)

### Capture

Unnamed function is a closure that can capture local variables. Local variables are captured is if by creating identically named variables inside the closure and assigning to them the outside variables, i.e. primitive types and tuples by value, objects, arrays and maps by reference.

Global variables do not need to be captured and can be referenced directly.

### Overloading

Functions can be overloaded, i.e. single function name can be used with different argument types and arities. Similar to mainstream programming languages, overloading on the return type alone is *not* allowed. Overloading on the argument names alone is not allowed either.

## Events

Events are like functions in the sense that they can be called like a function and they can be assigned functions. Events are particularly useful as fields of objects:

    type Object = (
        field1 : num,
        onSomething : event(a : num)
    )

Unlike functions, events may not return values (`->` missing), but they may provide default values for their arguments. To trigger an event, the event should be called as if it were a function, i.e. the arguments must begiven. Events also implicitly cast to boolean values. An event with no handler is `false`, all other events are `true`

When an event is assigned a function, that function becomes its sole handler. But unlike functions, events also support the `+=` assignment, in which case the handler is *added* to the event. When multiple handlers for the event exist, they are called in the order in which they were added to the event:

    let a : event(a : num, b : num) 
    a += fun
        ... # first
    a += fun
        ... # second

    a(1, 2) # both first and second will be called, in this order

All event handlers can be cleared if they are assigned `none` (`+=` assignment of `none` is syntax error). An event can also be queried about the number of its handlers vioa the `numHandlers() -> num` method. 
    
Events use value matching to allow for conditional dispatch, i.e. unlike the example above, consider the following:

    let a : event(a : num, b : num) 
    a += fun
        ... # first
    a += fun(a, 56)
        ... # second

    a(1, 2) # only first will be called
    a(1, 56) # first and second will be called

Value matching requires the explicit signature, as the matched values take place of the argument names.

## Objects

User defined object types, which must always be named and may contain fields. Note that an object is, in its simplest form just a named tuple, which is reflected in the declaration syntax:

    type ObjectWithEvent = (
        field1 : num = 67,
        onDone: event(status : num),
    )

Every object has default constructor which assigns the fields with the values provided at their declaration (or their default constructors transitively). Fields of an object are accessed with the `.` notation.

> There are no visibility specifiers for the language, everything is public. I might consider adding them later, but for now I do not see a need for them in a teaching language.

### Methods

The classic object definition only specifies the object's fields. Object methods are defined outside of the object itself, similar to how functions are defined, but with the following extras:

- when declared, the method's name is `ObjectType.MethodName`
- all methods have _implicit_ `self` argument that *must* be used to access the field members

So for example:

    type Complex = (
        re : num = 0,
        im : num = 0,
    )

    fun Complex.increaseBy(other : Complex)
        self.re += other.re
        self.im += other.im

The out-of-declaration syntax is useful so that functionality of objects can be extended without touching the objects themselves, i.e. a child may extend say a sprite with method functionality at any point.

### Inheritance

Object can inherit from others. Onky single inheritance is supported and _doLang_ does *not* support virtual methods, meaning that while objects can be extended, their base functionality cannot be changed. A child object can however be used in place of its parent object types:

    type FancyComplex extends Complex = (
        fancy : num
    )

### Constructors

To create an object, the name of the object type is used as if it were a function. Empty arguments will create the object using the default initialization defined for its fields (either via `=`, or if absent via the default initialization for their respective type). Alternatively, a keyword style arguments can be provided as if the type was a function call. Only keyword style arguments are allowed:

    let x = Complex() # re == 0, im == 0
    let i = Complex(im = 1) # re == 0, im == 1

User defined constructors are not supported, instead free functions can be defined with intended meaning, such as:

    fun Im(a : num) -> Complex
        return Complex(im = a)

    let a = Im(56)

However, this simple arcitecture makes construction of invalid object states possible. Canonical example of this is the rational number - and even though it makes no sense in _doLang_ context where rational is built-in - we will thus use it to illustrate as well. The problem is that certain values are invalid without fallback (denominator being zero), while some must be normalized. This can be done by specifying a `constructor` in the type definition, which is a function that is guaranteed to run after the object is created. As with any other function body, it starts with `fun` and is indented even though it is inside the parentheses:

    type Rational = (
        n = 0,
        d = 1,
        constructor = fun
            test self.d != 0
            if self.d < 0 then
                self.d *= -1
                self.n *= -1
            self.normalize()
    )

    fun Rational.normalize()
        ...

Constructor blocks can only be used in named objects, not anonymous tuples.

## Binding

_doLang_ supports three binding types: `let`, `const` and `global`. For assignments, function arguments and return values the following hold true and *cannot* be changed in the language:

- objects, arrays and maps are passed by reference, 
- the other types are passed by value.

### `global`

Declares new global mutable variable. Only valid at the top level. Syntactically `global` is identical to `let`, e.g.:
    
    global foo : num = 67 # type is optional if deducible from the assignment rhs


### `let`

`let` creates new local variable, only available inside functions and methods. When a let bound variable is assigned, the effect depends on the type. Primitive values and tuples are copied, while arrays, maps and objects will be referenced.

    let mySprite = globalSprite
    # mySprite and globalSprite are both the same object

### `const`

Declares constant primitive value (anything but object, array or map). Can be used in both global and local scopes (i.e. whenever `let` or `global` can be used). Value of a constant variable cannot be changed after the initial assignment.

    const pi = 3.14

_doLang_ does not support constant objects, maps and arrays. 

### Cloning

Because objects, arrays and maps are always passed by reference, any assignment creates an alias. In cases this is not wanted, and we do want a copy of the object, the `clone` keyword can be used, which instead of by reference assignment triggers a deep copy:

    let a = clone b # assuming b is an object array or map

## Assignments

The following assignment operators are supported:

    =, +=, -=, *=, /=

Assignment operators work as expected, but restrictions are placed on their placement as they can only be at the root position of standalone expressions or inside let and const statements. This also enforces only one assignment operator per expression. 

> Note that the assignment inside `enum` and `type` definitions is not really assignment operator, but part of the specific rules with distinct semantic meaning.

## Operators

_doLang_ follows standard operator precedence so as not to confuse kids with specific evaluation rules, namely:

- postfix operators: field access `.` (or enum value selection), indexing `[]` and function call `()`
- prefix operators: unary `-` and `clone`
- multiplication and division (`*`, `/`)
- addition and subtraction (`+`, `-`), text concatenation (also `+`)
- comparison operators (`<`, `<=`, `>`, `>=`)
- equality comparisons (`==`, `!=`)
- logical `not`
- logical `and`
- logical `or`

All postfix operators share the same precedence and are applied left to right, so that expressions such as `a[i].x` or `foo().bar[2]` work as expected. Prefix operators apply to the whole postfix expression that follows them, i.e. `-a.x` negates `a.x` and `clone a.items[2]` clones the element of `a.items`, not `a` itself.

Unlike the arithmetic negation, logical `not` has lower precedence than comparisons, so that `not a == b` means `not (a == b)`, which is how it reads in English.

Short-circuiting of logical operators `and` and `or` is supported. 

The comparison operators can be chained, i.e. one can write the following:

    A < B < C

which is equivalent to the following code:

    let a = A # evaluate the values exactly once
    let b = B
    let c = C  
    a < b and b < c

The equality (`==`, not the inequality `!=`) operator can also be chained i.e.:

    A == B == C
    # is identical to
    let a = A # evaluate the values exactly once
    let b = B
    let c = C  
    a == b and b == c

Comparison operators only work on numbers. Equality operators work on all types, comparing the value for value types and the object identity for the reference types (objects, arrays and maps).

### Indexing Operator

Special indexing operator `[]` exists for both arrays and maps:

    array[67] # returns 67th element of the array

    map["key"] # returns value corresponding to the key

### Text concatenation

The `+` operator can be used with text values for simple concatenation. Text values can be concatenated with other values as well and those will be pretty printed.

## Statements

### `if`

The `if` statement supports conditional execution and can be optionally followed by else case. The condition is terminated by the `then` keyword:

    if a < 10 then
        ...
    else if a < 20 then
        ...
    else
        ...

There is a special case of the statement that deals with the `optional` returning functions and map index operator:

    if let a = map["key"] then
        ... # a is bound to value of key in map, if one existed
    else
        ... # key not in map (the index operator returned none)

This is the only `if` form where assignment operator is allowed inside the if expression.

`if` statement can also be used as an expression, i.e. one can write for instance:

    fun min(a : num, b : num) -> num
        return if a <= b then a else b

In the expression form the `if` statemet *must* include both `then` and `else` clauses and only a single expression is allowed in either true or false case. Both expressions must return same type. 

### `while`

The simple loop available in the language, the condition is terminated by the `do` keyword:

    while a < 10 do
        a += 1

The loop also supports `break` and `continue` statements, which do what they are expected to, i.e. stop the loop immediately and move to the next iteration respectively. 

### `for` loop

The `for` loop serves a double duty. First the sequence iterator, which automatically creates let binding of the induction variable. 

    for i in 1 <= i < 100 do
        ... # simple loop with induction variable

    for i in 1 <= i < 100 step 2 do
        ... # custom step

This notation neatly solves the inclusivity or exclusivity of the bounds, but has limitations such as both lower and upper bounds must be present and the middle value *must* be the induction variable itself.

Then a for-each variant for arrays and maps:

    for value in array do
        ...

    for value, index in array do
        ...

    for value in map do
        ...

    for value, key in map do
        ...

During the for each loop variants, any access to the array or map itself other than the element is *not* allowed.

For loop supports the `break` and `continue` statements as well.

### `match`

This is _doLang_'s switch on pattern-matching steroids. No fallback between different cases is allowed and each case is its own block. Each case starts with the `is` keyword followed by a pattern and is terminated by the `then` keyword, while the `match` itself is not, as it is always followed by the cases:

    match value
        is a then
            ...
        is b then
            ...
        else
            ...

The `else` case is executed if *none* of the cases above applies. The cases must be disjunctive, i.e. for any value, only one can apply. Case patterns support extremely simple form of pattern matching to existing values, similar to that of the event handlers:

- primitive values can be matched by their values, including enums
- arrays and tuples can be matched by their contents

As an example:

    match complexTuple
        is (re = 1, im) then
            ... # we now have re and im in scope

    match array
        is [1, a, b] then
            ... # we have a and b in scope

## `return`

Returns from the function, optionally returning value of required type. 

## `write`

`write` is a simple statement that allows debug printing to the output (serial to usb on the device usually). It is followed by a text expression.

## `test`

`test` followed by a boolean evaluating expression is _doLang_'s version of assertion. Upon test failure, the program is paused and user is informed of the invariant problem.

## Type Inference

While _doLang_ uses static typing and every binding as well as function can be typed, type inference is heavily used to avoid the need for writing the types. Crucially, let bindings and tuple and object fields do not have to be explicitly typed if their type can be inferred from the assignment (which is most cases, except an empty array or map). 

When declaring new functions, the functions themselves must be typed explicitly, but note that function definition is fairly complex task already so the need for explicit typing is likely ok.

## Libraries & Code Organization

Libraries can be imported using the `use` statement, which can only be used at top level:

    use libraryName

This loads the library of given name from the system and makes its functions and objects available to the current file. A library is nothing else than a _doLang_ file. Libraries can come from the project itself (then file `libraryName.do` must exist at the same level as the file in which the `use` statement is), or system libraries, in which case the file must exist in the system's libraries folder, or be known to the runtime. Local libraries take precedence over system ones.

Because of the on-device limitations, _doLang_ programs are expected to be single `.do` files as the editors allow the user to browse & select the function, type and event handler definitions one by one. The practice should be that reusable code should become libraries, while the full application code, i.e. all that is intended for the application alone should be in the single `.do` file. 

_doLang_ does not support namespaces. The reason for this is that namespaces complicate things and force kids think about ambiguity resolution. Instead _doLang_ does not allow ambiguity to exist at all, which is probably fine as the projects in the language are not expected to be enormous.

## Standard Library

- mod(x, y)
- pow(x, y)
- sqrt(x)


# Appendix A - Grammar

The lexer does not produce any newline or indentation tokens. Instead, every token remembers its column and whether it is the first token on its line. Line breaks are ignored by the parser, except where the grammar explicitly mentions them, in which case the position of the next token is checked. This is known as the _offside rule_.

The parser keeps track of the column of the current block, `B` (`0` for the top level). Outside of parentheses, the following holds for the first token on a line:

- column greater than `B` continues the current statement (or header)
- column equal to `B` cannot continue the current statement, the statement must end and a new one starts
- column smaller than `B` cannot continue the current statement and ends the current block. It must be equal to the column of one of the enclosing blocks

Inside `()`, `[]` and `{}` parentheses, the columns of the tokens are not checked at all, with the exception of `BLOCK` (see below).

The line breaks in the grammar are denoted by the following symbols, which do not consume any tokens, but check the position of the next token:

- `newline` the next token is the first on its line and its column is `B`
- `indent` the next token is the first on its line and its column is greater than the reference column `R`. The column of the token becomes `B` of the new block. Outside of parentheses `R` is the `B` of the enclosing block, inside parentheses `R` is the column of the first token on the line where the block was introduced (i.e. the line with the `fun` keyword or the function signature)
- `dedent` the next token is the first on its line and its column is smaller than `B` (or end of file). The block ends and `B` of the enclosing block is restored

Empty lines and comments are ignored. Tabs are not allowed for indentation.

    BLOCK := indent STATEMENT { newline STATEMENT } dedent

A block of statements is the only place where the layout matters, i.e. blocks use layout even inside parentheses. Once the block ends, the columns are again ignored for the rest of the parentheses. Since the conditions are checked on the token's position, it does not matter that the token might have been already read as lookahead before the parser knew a block is expected.

All statements end at the end of the line, i.e. the next token must satisfy either `newline`, or `dedent`, and a token that cannot continue the statement on the same line is an error. Statements ending with `BLOCK` are already terminated by its `dedent`.

Lists of items inside parentheses use the following parametrized rule:

    LIST(X) := [ X { ',' X } [ ',' ] ]

Items are separated by commas and an optional trailing comma is allowed. As an exception, the comma after an item that ends with a `BLOCK` is optional, as the end of the block already separates the items.

`BLOCK` is only ever introduced by a keyword (`then`, `do`, `else`, `fun`) or a function signature, never by an operator. The right hand side of `=` (and other assignment operators) is therefore always an `EXPR` and a line break after `=` is an ordinary continuation. Function bodies on the right hand side are function literals, see `FUN_LITERAL`. The grammar is thus context free and the parser never needs to know the type of the left hand side.

## Top Level Statements

    PROGRAM := { USE_DECL } { GLOBAL_DECL | CONST_DECL | FUN_DECL | TYPE_DECL | EVENT_ASSIGN }

Program consists of global or constant declarations, function and type declarations, event assignments and `use` declarations, which must come first:

    USE_DECL := 'use' ident newline

`use` declaration is simply name of library to be used. Each library must be on separate line and with its own `use` keyword.

    GLOBAL_DECL := 'global' ident [ ':' TYPE ] [ '=' EXPR ]
    CONST_DECL := 'const' ident [ ':' TYPE ] [ '=' EXPR ]

Global variable declaration starts with `global` keyword and has name, optional type and initialization value. The value is optional and when omitted default constructor is used. When the global variable is a function or event, the value can be a function literal. Constant declaration is the same, just starts with `const` keyword. As constants can only be primitive values, they cannot be initialized with function literals.

    FUN_DECL := 'fun' ident [ '.' ident ] '(' LIST(ARG) ')' [ '->' [ 'optional' ] TYPE ] BLOCK

Function declaration has a name, which for methods is prefixed with the object type name, arguments, optional return type and the body. Same as anonymous functions, there is no `=` between the signature and the body. The declaration is distinguished from the function literal by the identifier following the `fun` keyword.

    EVENT_ASSIGN := POSTFIX_EXPR ( '=' | '+=' ) EXPR

Event (or function variable) assignment at the top level, such as `button.onPress += fun` followed by the handler body. The handler is usually a function literal, either with implicit signature (arguments bound from the event type), or with explicit signature. 

    TYPE_DECL := 'type' ident [ 'extends' ident ] '=' '(' LIST(FIELD_DECL) ')'
    FIELD_DECL := ident [ ':' TYPE ] [ '=' EXPR ] 
               |= 'constructor' '=' FUN_LITERAL

Object type declaration is a list of fields, each with name, optional type and optional default value, and optionally the constructor. Both the constructor and function or event fields may use function literals inside the parentheses, whose `BLOCK` uses layout as described above. The constructor must use the implicit signature form, i.e. `constructor = fun`.

## Statements

    STATEMENT := LET_STMT | IF_STMT | WHILE_STMT | FOR_STMT | MATCH_STMT | RETURN_STMT 
              |= 'break' | 'continue' | 'write' EXPR | 'test' EXPR | EXPR_STMT

Statements that can appear inside function bodies.

    LET_STMT := ( 'let' | 'const' ) ident [ ':' TYPE ] [ '=' EXPR ]
    EXPR_STMT := EXPR [ ( '=' | '+=' | '-=' | '*=' | '/=' ) EXPR ]
    RETURN_STMT := 'return' [ EXPR ]

Local variable declarations, assignments and standalone expressions. Assignment is only allowed at the root of the expression statement. Whether the left hand side can be assigned to is a semantic check. `return` value is optional, and since the statement ends at the end of the line, the parser knows there is no value when the next token is on a new line.

    IF_STMT := 'if' ( EXPR | 'let' ident '=' EXPR ) 'then' BLOCK [ newline 'else' ( BLOCK | IF_STMT ) ]

The `if` statement condition is terminated by `then`, so it can span multiple lines. The `else` keyword must be at the same column as the `if`. The `else` is followed either by a block, or directly by another `if` statement to form `else if` chains. Statement `if` is distinguished from the inline `if` expression by being at the beginning of the statement.

    WHILE_STMT := 'while' EXPR 'do' BLOCK
    FOR_STMT := 'for' ident [ ',' ident ] 'in' EXPR [ 'step' EXPR ] 'do' BLOCK

Loops, both terminated by the `do` keyword. The range form of the `for` loop (`for i in 1 <= i < 100 do`) is parsed as regular expression and its validity (chained comparison with the induction variable in the middle) is checked semantically.

    MATCH_STMT := 'match' EXPR indent CASE { newline CASE } [ newline 'else' BLOCK ] dedent
    CASE := 'is' PATTERN 'then' BLOCK
    PATTERN := LITERAL | '-' number | ident { '.' ident } 
            |= '(' LIST(ident [ '=' PATTERN ]) ')' | '[' LIST(PATTERN) ']'

The `match` header does not need any terminating keyword as it is always followed by the cases, and `is` cannot continue an expression. Each case is terminated by `then`. Patterns are restricted to literals, identifiers (and enum values), and tuple and array patterns. Whether an identifier binds a new value, or matches an existing one is decided semantically.




## Types

    TYPE := POD_TYPE | ARRAY_TYPE | TUPLE_TYPE | MAP_TYPE | FUN_TYPE | EVENT_TYPE
    POD_TYPE := 'num' | 'bool' | 'text' | ident

A type is either number, boolean, text, or user defined type, which is parsed as identifier. Furthermore specific type literals exist for the built-in types:

    ARRAY_TYPE := '[' TYPE ']'

Array type is simply a type encosed in square brackets. Nested arrays are thus supported.

    TUPLE_TYPE := '(' LIST(FIELD_TYPE) ')'
    FIELD_TYPE := ident [ ':' TYPE ]

Tuple is list of comma separated fields defined inside parentheses. Field has name and may specify a type.

    MAP_TYPE := '{' POD_TYPE '->' TYPE '}'

Map type is defined by the key type followed by arrow and the value type. Only POD types and objects can be keys in the map. 

    FUN_TYPE := 'fun' '(' LIST(ARG) ')' [ '->' [ 'optional' ] TYPE ]
    ARG := ident ':' TYPE [ '=' EXPR ] 

Function types start with the `fun` keyword followed by the argument lists and optional return type after arror operator, or the function returns void. Return type may be prefixed with `optional` keyword. Function arguments are comma separated and they may contain default values.

    EVENT_TYPE := 'event' '(' LIST(ARG) ')'

Event types are linke function types without return value, and start withe `event` keyword instead.

## Expressions

    EXPR := INLINE_IF | OR_EXPR

An expression is either inline `if`, or it recurses into standard expressions based on operator precedence: 

    INLINE_IF := 'if' EXPR 'then' EXPR 'else' EXPR

The inline `if` expression is equivalent to a ternary operator in C++. Both `then` and `else` are mandatory and both then and else case return values must be of same type. 

    OR_EXPR := AND_EXPR { 'or' AND_EXPR }
    AND_EXPR := NOT_EXPR { 'and' NOT_EXPR }
    NOT_EXPR := 'not' NOT_EXPR | EQ_EXPR

Logical operators, no surprise here. `not` has lower precedence than the binary ones, so `not a and b` is equivalent to `(not a) and b`. 

    EQ_EXPR := CMP_EXPR { '==' CMP_EXPR }
            |= CMP_EXPR '!=' CMP_EXPR

Equality comparisons are as expected from other languages. Note that only `==` can be chained, which is ensured at the grammar level. Shortcircuiting works based on the underlying traslation.  

    CMP_EXPR := ADD_EXPR { ( '<' | '<=' | '>' | '>=' ) ADD_EXPR }

Note that relation operators can be chained. And the semantics require that each element is evaluated *once* and then for the inner ones might be compared twice. Shortcircuiting works based on the underlying traslation.

    ADD_EXPR := MUL_EXPR { ( '+' | '-' ) MUL_EXPR }
    MUL_EXPR := UNARY_EXPR { ( '*' | '/' ) UNARY_EXPR }
    UNARY_EXPR := ( '-' | 'clone' ) UNARY_EXPR | POSTFIX_EXPR

Arithmetic operators, all left associative, and the prefix operators, which apply to the entire postfix expression that follows them.

    POSTFIX_EXPR := PRIMARY { '.' ident | '[' EXPR ']' | CALL_EXPR }

Field access (or enum value selection), indexing and function call all share the same precedence and are applied left to right. Function call arguments use the same rule as parenthesized expressions and tuple literals, see below.

    PRIMARY := LITERAL | ident | 'self' | 'none' | '(' EXPR ')' 
    LITERAL := number | string | 'true' | 'false'
            |= ARRAY_LITERAL | MAP_LITERAL | FUN_LITERAL

The primary expressions are literals of the primitive types, identifiers (variables, functions, types when used as constructors and enum values in their shorthand form), `self` inside methods, `none` inside functions returning `optional` values and the compound literals.

    PAREN_EXPR := '(' LIST(NAMED_EXPR) ')'
    NAMED_EXPR := EXPR [ '=' EXPR ]

Parentheses serve multiple purposes: grouping of expressions (`(a + b)`), tuple literals (`(foo = 1, bar = 56)`) and function call arguments (`foo(a, 56)` or `inc(x = 56)`). All are parsed by the same rule and differentiated afterwards: a single item without `=` is a grouping, otherwise all items must be in the `ident '=' EXPR` form to form a tuple literal. When used as call arguments, both forms can be used. This way the grammar stays LL(1) even though we only know which form is used after the first expression has been parsed. Note that the `=` here is not an assignment operator. 

    ARRAY_LITERAL := '[' LIST(EXPR) ']'
    MAP_LITERAL := '{' LIST(MAP_ITEM) '}'
    MAP_ITEM := EXPR '->' EXPR

Array literals are list of values in square brackets, map literals are list of key-value pairs separated by the arrow operator in curly brackets. Empty `[]` and `{}` are valid, but their types must be known from the context.

    FUN_LITERAL := 'fun' [ '(' LIST(LAMBDA_ARG) ')' [ '->' [ 'optional' ] TYPE ] ] BLOCK
    LAMBDA_ARG := ident [ ':' TYPE ] [ '=' EXPR ] | LITERAL

Anonymous function starts with the `fun` keyword, optionally followed by the signature (arguments and optional return type) and then the function body, which is a `BLOCK` of statements starting on a new line (and therefore with `indent`). The two forms are differentiated with a single token lookahead after `fun`: `(` starts the explicit signature, `indent` starts the body. There is no `=` between the signature and the body. 

When the signature is omitted, it is taken from the function type known from the context and the arguments are bound under their names from the type. When the signature is given, argument types may be omitted if the function type is known from the context, but all arguments must always be listed. Literal arguments are only valid for event handlers where they are used for the value matching. Whether the function type is known from the context, and therefore which parts of the signature may be omitted, is decided semantically.

Outside of parentheses, the function body ends with `dedent`, so an anonymous function must always be the last part of the statement it appears in, and the `dedent` also terminates the statement. Inside parentheses, the body ends at the first line indented less than the body, after which the list continues as usual (with the comma being optional).


# Appendix B - Runtime

There is no main function per se.

# Appendix C - Future Extensions

- int type and bitwise operators

