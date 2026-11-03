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

    and, bool, break, clone, const, constructor, continue, do, else, enum, event, extends, false, for, fun, global, if, in, is, let, match, none, not, num, optional, or, orelse, return, self, step, test, text, then, true, type, use, while, with, write, xor

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

Blocks (bodies of `if`, loops, functions, etc.) always start on a new line that is indented more than the line that introduced them. The statements that have a block end with a keyword (`if ... then`, `while ... do`, `for ... do`, `match ... with`, `is ... then`), so their headers can span multiple lines as well. A line break directly after a keyword that may introduce a block (`then`, `else`, `do`, `orelse`, `with`, `fun`) therefore always starts a block, never a continuation of an expression. Function bodies always follow the `fun` keyword, either directly, or after the function signature (`fun(...)`). Function bodies use indentation inside parentheses too. Such a body starts on a new line indented more than the line where it was introduced and ends at the first line indented less than the body. As the next item starts at the item column, no comma is needed after the body:

    type Player = (
        onHit = fun(damage : num)
            if damage > 10 then
                playSound(ouch)
            self.lives -= damage
        lives = 3
        constructor = fun
            test self.lives > 0
    )

It is always the indentation that decides where an item ends, the comma is only a separator between items. A comma may be written at the end of a line that continues a multi-line item, but not after a function body (or any other block), as the block only ends on the next line:

    foo(
        a
            + b,     # comma after the multi-line item a + b
        c
    )

When the first item is on the same line as the opening bracket, the list is in its single line form, even if it spans multiple lines. Line breaks inside are then always continuations and the items must be separated by commas:

    foo(a, b,
        c)           # three arguments

A block of statements is always introduced by a keyword (`then`, `else`, `do`, `orelse`, `with`, `fun`, or the function signature following it), never by an operator. In particular, a newline after `=` (or any other assignment operator) is always a continuation of the statement, regardless of the type of the assignment target. Function bodies must always start with the `fun` keyword:

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

Following the `key -> value` notion, a map is read by calling it as if it were a function taking the key and returning `optional` value, i.e. the call must be followed by `orelse` (see Optional return types) as the key might not be in the map. The argument is called `key` and can be used by name as well:

    let a = m(1) orelse "" # "foo"
    let b = m(key = 3) orelse "none" # "none"

Calling the map is a shorthand for its `get(key : K) -> optional V` method, i.e. `m(1)` and `m.get(1)` are the same. All other operations on the map are methods as well:

    m.get(1) orelse "" # same as m(1) orelse ""
    m.set(3, "baz") # inserts, or overwrites the value for the key
    m.has(3) # true if the key is in the map
    m.remove(3) # removes the key from the map, if present

The call is only a shorthand for the lookup and not a function value, i.e. a map cannot be used where a function is expected. When a function is needed, the `get` method can be used instead, as like any other method it can be assigned to function variables (the other map methods can be added to events as well, but `get` cannot, as events do not return values):

    let lookup : fun(key : num) -> optional text = m.get
    let x = lookup(1) orelse ""

As maps are reference types, the function refers to the map itself, not its copy, i.e. later changes of `m` are visible through `lookup`.

Assigning to the call result (`m(1) = "foo"`) is an error, `set` must be used instead. Note that for maps of functions, the fallback of `orelse` would be called with the arguments, i.e. `m(1) orelse foo(2)` calls `foo(2)` when key is missing. To call the value from the map, parentheses must be used: `(m(1) orelse foo)(2)`.

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

A function return type can be declared as `optional`, which means the function may either return value of the given type, or `none`. Optional is a primitive maybe type - functions (including user defined ones) can return it and the caller is forced to handle the `none` case right at the call site, but the optional values themselves cannot be stored, or passed around. This makes the feature a lot simpler to understand:

    fun safeDivide(a : num, d : num) -> optional num
        if d != 0 then
            return a / d
        else 
            return none

Every call of a function returning `optional` must be immediately followed by the `orelse` operator, which provides the value to be used when the function returns `none`:

    let x = safeDivide(a, b) orelse 0

The fallback value must be of the same type as the optional value and it is only evaluated when the function returned `none`. The `orelse` binds as tightly as the call itself and its right hand side is a single unary expression (i.e. a literal, a variable, or a postfix expression such as another call, optionally prefixed with `-` or `clone`). More complex fallbacks must be put in parentheses:

    let y = a + safeDivide(a, b) orelse 1 + 2  # a + (safeDivide(a, b) orelse 1) + 2
    let z = safeDivide(a, b) orelse (c + d)
    let w = safeDivide(a, b) orelse safeDivide(a, c) orelse 0 # orelse can be chained

Instead of a value, the fallback can also be `return`, `break` or `continue`, in which case the execution leaves the expression when `none` was returned. This is particularly useful for early returns and `none` propagation:

    fun average(values : {text -> num}, a : text, b : text) -> optional num
        let x = values(a) orelse return none
        let y = values(b) orelse return none
        return (x + y) / 2

When more statements are needed to handle the `none` case, `orelse` can be followed by a block of statements. As with any block used as an expression (see Blocks as Expressions), the block must either consist of a single expression, or never finish normally, i.e. always end with `return`, `break` or `continue`:

    let x = values("key") orelse
        write "key not found"
        return none

Like with function literals, the block ends with `dedent` and therefore must be the last part of the statement it appears in. To compute the fallback value with multiple statements, an anonymous function can be created and immediately called.

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

- postfix operators: field access `.` (or enum value selection), indexing `[]`, function call `()` and `orelse`
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

The equality (`==`) operator can also be chained i.e.:

    A == B == C
    # is identical to
    let a = A # evaluate the values exactly once
    let b = B
    let c = C  
    a == b and b == c

Inequality operators can be chained too, with the meaning that all values bound by the inequality must be distinct, i.e. :

    A != B != C
    # true for 1 2 3
    # false for 1 2 1

It is not allowed to mix `==` and `!=` chains in a single expression. 

Comparison operators only work on numbers. Equality operators work on all types, comparing the value for value types and the object identity for the reference types (objects, arrays and maps).

### Indexing Operator

Special indexing operator `[]` exists for arrays:

    array[67] # returns 67th element of the array

Maps do not support the indexing operator, values are read by calling the map instead (see `map`).

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

`if` statement can also be used as an expression, i.e. one can write for instance:

    fun min(a : num, b : num) -> num
        return if a <= b then a else b

In the expression form the `else` clause is mandatory and both cases must have the same type. The cases are either written on the same line as the keyword (`then`, or `else`), or as blocks, which must follow the rules for blocks used as expressions (see below). The two forms can be mixed:

    let x = if a < b then a else b
    let y = if a < b then
        a
    else if a < c then
        c
    else
        return none

When the cases are written as blocks, the `else` must be at the same column as the first token of the line on which the `if` is (`let` in the example above). As the last block ends with `dedent`, an `if` expression with blocks must be the last part of the statement it appears in.

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

This is _doLang_'s switch on pattern-matching steroids. No fallback between different cases is allowed and each case is its own block. The matched value is terminated by the `with` keyword. Each case starts with the `is` keyword followed by a pattern and is terminated by the `then` keyword:

    match value with
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

    match complexTuple with
        is (re = 1, im) then
            ... # we now have re and im in scope

    match array with
        is [1, a, b] then
            ... # we have a and b in scope

Match statement can be used inside expression as well, e.g. one can write the following:

    let x = match y with
        is (re = 1, im) then
            "real is one"
        else
            "real is not one"

Similar to the `if` expression, each case can either be written on the same line as its `then` (or `else`), or as a block, in which case it must follow the rules for blocks used as expressions (see below). All cases must have the same type and the `else` case is mandatory, unless the cases cover all possible values (such as all values of an enum). When the cases are on separate lines, they end with `dedent` and the `match` expression must be the last part of the statement it appears in:

    let name = match direction with
        is Direction.Left then "left"
        is Direction.Right then "right"

The `match` expression can also be written on a single line, in which case all case values must be on the same line as their `then`:

    let name = match direction with is Left then "left" is Right then "right"

Since `is` cannot continue an expression, it always starts the next case. When an inline `match` is nested inside a case of another inline `match`, the following cases belong to the inner one, same as an `else` belongs to the nearest `if`, or `match`. Parentheses must be used otherwise.

### Blocks as Expressions

Whenever a block is used as a part of an expression (cases of `if` and `match` expressions and the `orelse` block), it must be one of:

1. a block consisting of a single expression, whose value is the value of the block. The expression can itself be an `if`, or `match` expression, whose blocks follow the same rules.
2. a block that never finishes normally, i.e. all paths through the block end with `return`, `break` or `continue`.

A block that never finishes normally does not produce any value, so it is compatible with any type. This is why the cases may mix values and early returns, e.g. `if a > 0 then a else return none` is of type `num`. Same goes for the `orelse` operator followed by `return`, `break`, or `continue`. Any other block (such as several statements followed by a value) is an error. If a value needs to be computed by multiple statements, an anonymous function can be created and immediately called.

## `return`

Returns from the function, optionally returning value of required type. `return` (as well as `break` and `continue`) can also be used as the fallback of the `orelse` operator. This is particularly useful for early returns and `none` propagation:

    let x = map("key") orelse return none
    # do some more with x

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


# Appendix A - Lexer

The lexer turns the source into a stream of tokens. It also handles all of the layout: indentation is converted to `indent`, `dedent` and `newline` tokens, so that the parser never needs to look at columns.

## Tokens

- *identifiers* start with a letter or `_`, followed by letters, digits and `_`. Identifiers are case sensitive. Keywords (see Keywords) are not identifiers.
- *numbers* are decimal (`123`, `1_000`, `1.567`), hexadecimal (`0x7f`), binary (`0b101`) or octal (`0o17`). Floating point requires digits on both sides of the `.`. A `-` is never part of the number, negative values are the unary minus.
- *text* is enclosed in `"` and may contain the escapes described in Literal Values. Text cannot span multiple lines.
- *operators and punctuation*, longest match wins (i.e. `->` is never `-` followed by `>`):

        +  -  *  /  =  +=  -=  *=  /=  ==  !=  <  <=  >  >=  ->  .  ,  :  |  (  )  [  ]  {  }

Whitespace between tokens is ignored. Comments (`#` to the end of the line), empty lines and lines with only a comment are ignored entirely and do not affect the layout. Only spaces may be used for indentation, a tab in the indentation is an error. Since columns are only ever measured on the first token of a line, they are simply the number of leading spaces.

## Layout

The lexer keeps a stack of layout contexts. A context is either a block of statements, or a bracket (`()`, `[]`, `{}`), and may have a column. Blocks always have a column, brackets only when they are written in the multi-line form (see below). The stack starts with the top level block at column `0`. The reference column `R` is the column of the topmost context that has one.

Only line breaks are interesting, i.e. the lexer only acts when the next token is the first on its line, at column `c`. The rules are checked in the following order:

1. *Closing bracket.* If the line starts with a closing bracket, nothing is emitted for the line break. The bracket token itself is handled as described below.
2. *Opening a context.* If the last token of the previous line is one of the following and `c > R`, `indent` is emitted and a new context with column `c` is pushed:
    - `then`, `else`, `do`, `orelse`, `with`, or `fun` - a block. If `c <= R` it is an error, as a block is expected
    - the end of a function signature, i.e. the `)` closing the bracket that directly follows `fun`, `fun ident`, or `fun ident.ident`, optionally followed by `->` and a type - a block. If `c <= R` nothing happens, as a function type may end the line (e.g. `let inc : fun(x : num) -> num`)
    - an opening bracket - the bracket becomes multi-line and gets column `c`
3. *Same column.* If `c` equals the column of the top context, `newline` is emitted. This separates statements in blocks, and lines of items in brackets.
4. *Continuation.* If `c` is greater than the column of the top context, or the top context is a bracket without a column, nothing is emitted.
5. *Smaller column.* `dedent` is emitted and the top context is popped, as long as it is a block with column greater than `c`. Then rule 3 or 4 applies to the new top context. If the new top is a bracket with column greater than `c`, it is an error (brackets can only be closed by their closing token).

When a closing bracket arrives (anywhere on the line), the lexer pops all contexts down to and including the matching bracket, and emits `dedent` for each popped context that has a column, i.e. for every block inside the bracket, as well as for the bracket itself if it is multi-line. Brackets that do not match are an error.

At the end of file, `dedent` is emitted for every open block except the top level one. An unclosed bracket is an error.

Note that `indent` is only emitted for the block keywords, function signatures and brackets, never just because a line is indented more. Any other line indented more than the column of the top context is a continuation.

### Example

    type Player = (
        onHit = fun(damage : num)
            if damage > 10 then
                playSound(ouch)
            self.lives -= damage
        lives = 3
    )

produces the following tokens:

    'type' ident '=' '('
    indent ident '=' 'fun' '(' ident ':' 'num' ')'
    indent 'if' ident '>' number 'then'
    indent ident '(' ident ')'
    dedent newline 'self' '.' ident '-=' ident
    dedent newline ident '=' number
    dedent ')'

### Consequences for the grammar

- statements, as well as lines of items in multi-line brackets, are separated by `newline`
- `else` after a block arrives as `dedent newline 'else'`, unless it is inside a bracket without a column, where it arrives as `dedent 'else'`. The parser must look one token past the `newline` to decide whether the statement continues
- multi-line brackets are `indent` ... `dedent` inside the bracket tokens, single line brackets contain no layout tokens at all, unless they contain blocks

# Appendix B - Grammar

The grammar works on the tokens produced by the lexer (see Appendix A). Nonterminals are uppercase, terminals are either quoted (`'if'`, `'('`), or lowercase: `ident`, `number`, `string` and the layout tokens `newline`, `indent` and `dedent`. `[ X ]` is optional, `{ X }` is repeated zero or more times and `|` separates alternatives (`|=` continues the alternatives on the next line).

The grammar is LL(1) with a single exception: after an `if` with a `BLOCK` body, `newline 'else'` continues the `if`, while `newline` alone ends the statement (or the line of items). The parser must therefore look at the token after `newline` (see `IF`).

## Common Rules

    BLOCK := indent STATEMENT { newline STATEMENT } dedent

A block is a sequence of statements, each starting on its own line. As the lexer emits `newline` only for lines at the column of the block, statements continued on further lines need no special handling. Statements ending with a nested `BLOCK` are followed by `newline` as any other statement, since the lexer emits it after the `dedent` tokens.

    BODY := BLOCK | EXPR

Where a keyword allows either a block, or an expression (`then`, `else`), the parser decides by the next token: `indent` starts the block, anything else is the expression. Whether a `BLOCK` used as an expression consists of a single expression, or never finishes normally (see Blocks as Expressions) is a semantic check.

    LIST(X) := indent LINE(X) { newline LINE(X) } dedent
             | LINE(X)
    LINE(X) := [ X { ',' X } [ ',' ] ]

Items inside brackets are either in the single line form, separated by commas, or in the multi-line form (the first item is on a new line), in which the lexer emits `indent` after the opening bracket, `newline` between lines and `dedent` before the closing bracket. Each line may contain multiple comma separated items and an optional trailing comma. An item that continues on further lines, or ends with a block, needs no special handling.

Blocks are only ever introduced by a keyword (`then`, `else`, `do`, `orelse`, `with`, `fun`), or a function signature, never by an operator. The right hand side of `=` (and other assignment operators) is therefore always an `EXPR`, function bodies on the right hand side are function literals (see `FUN_LITERAL`). The grammar is thus context free and the parser never needs to know the type of the left hand side.

## Top Level Statements

    PROGRAM := [ TOP_DECL { newline TOP_DECL } ]
    TOP_DECL := USE_DECL | GLOBAL_DECL | CONST_DECL | FUN_DECL | TYPE_DECL | ENUM_DECL | EVENT_ASSIGN

Program consists of `use` declarations, global or constant declarations, function, type and enum declarations and event assignments, all starting at column `0`. That the `use` declarations come before all others is a semantic check.

    USE_DECL := 'use' ident

`use` declaration is simply name of library to be used. Each library must be on separate line and with its own `use` keyword.

    GLOBAL_DECL := 'global' ident [ ':' TYPE ] [ '=' EXPR ]
    CONST_DECL := 'const' ident [ ':' TYPE ] [ '=' EXPR ]

Global variable declaration starts with `global` keyword and has name, optional type and initialization value. The value is optional and when omitted default constructor is used. When the global variable is a function or event, the value can be a function literal. Constant declaration is the same, just starts with `const` keyword. As constants can only be primitive values, they cannot be initialized with function literals.

    FUN_DECL := 'fun' ident [ '.' ident ] '(' LIST(ARG) ')' [ '->' [ 'optional' ] TYPE ] BLOCK

Function declaration has a name, which for methods is prefixed with the object type name, arguments, optional return type and the body. Same as anonymous functions, there is no `=` between the signature and the body.

    EVENT_ASSIGN := ident { '.' ident } ( '=' | '+=' ) EXPR

Event (or function variable) assignment at the top level, such as `button.onPress += fun` followed by the handler body. The handler is usually a function literal, either with implicit signature (arguments bound from the event type), or with explicit signature. The target is restricted to a chain of identifiers, so that all top level declarations can be told apart by their first token.

    TYPE_DECL := 'type' ident [ 'extends' ident ] '=' '(' LIST(FIELD_DECL) ')'
    FIELD_DECL := ident [ ':' TYPE ] [ '=' EXPR ] 
               |= 'constructor' '=' FUN_LITERAL

Object type declaration is a list of fields, each with name, optional type and optional default value, and optionally the constructor. Both the constructor and function or event fields may use function literals inside the parentheses. The constructor must use the implicit signature form, i.e. `constructor = fun`.

    ENUM_DECL := 'enum' ident '=' ident { '|' ident }

Enum declaration is a list of its values separated by `|`. As a line break after `=` is a continuation, the values may start on the next line.

## Statements

    STATEMENT := LET_STMT | IF | WHILE_STMT | FOR_STMT | MATCH | RETURN_STMT 
              |= 'break' | 'continue' | 'write' EXPR | 'test' EXPR | EXPR_STMT

Statements that can appear inside function bodies. Although `EXPR` may start with `if`, or `match` as well, at the beginning of a statement these are always parsed as the `IF` and `MATCH` statements.

    LET_STMT := ( 'let' | 'const' ) ident [ ':' TYPE ] [ '=' EXPR ]
    EXPR_STMT := EXPR [ ( '=' | '+=' | '-=' | '*=' | '/=' ) EXPR ]
    RETURN_STMT := 'return' [ EXPR ]

Local variable declarations, assignments and standalone expressions. Assignment is only allowed at the root of the expression statement. Whether the left hand side can be assigned to is a semantic check. `return` value is optional, there is no value when the next token is `newline`, or `dedent`.

    IF := 'if' EXPR 'then' ( BLOCK [ [ newline ] 'else' BODY ] | EXPR [ 'else' BODY ] )

The same rule is used for both the `if` statement and the `if` expression. The condition is terminated by `then`, so it can span multiple lines. `else if` chains need no special rule, as `BODY` can be an `EXPR`, which can be another `IF`. After a `BLOCK`, the `else` is preceded by `newline` outside of brackets, but not inside them (see Appendix A). An `if` at the beginning of a statement is the `if` statement, whose bodies must be blocks (of any statements) and the `else` is optional. Anywhere else, it is the `if` expression, whose `else` is mandatory and whose blocks must follow the Blocks as Expressions rules. Both are semantic checks.

    WHILE_STMT := 'while' EXPR 'do' BLOCK
    FOR_STMT := 'for' ident [ ',' ident ] 'in' EXPR [ 'step' EXPR ] 'do' BLOCK

Loops, both terminated by the `do` keyword. The range form of the `for` loop (`for i in 1 <= i < 100 do`) is parsed as regular expression and its validity (chained comparison with the induction variable in the middle) is checked semantically.

    MATCH := 'match' EXPR 'with' ( indent CASE { newline ( CASE | 'else' BODY ) } dedent
                                 | INLINE_CASE { INLINE_CASE } [ 'else' EXPR ] )
    CASE := 'is' PATTERN 'then' BODY
    INLINE_CASE := 'is' PATTERN 'then' EXPR
    PATTERN := LITERAL | '-' number | ident { '.' ident } 
            |= '(' LIST(ident [ '=' PATTERN ]) ')' | '[' LIST(PATTERN) ']'

The matched value is terminated by `with`. When it is followed by `indent`, the cases are on separate lines, otherwise all cases are on the same line. The `else` case must be the last one, which is a semantic check. In the single line form, `is` cannot continue the `EXPR` of the previous case, so it always starts a new case, and the `else` binds to the nearest `match`, or `if`. Each case is terminated by `then`. Patterns are restricted to literals, identifiers (and enum values), and tuple and array patterns. Whether an identifier binds a new value, or matches an existing one is decided semantically. Same as with `if`, a `match` at the beginning of a statement is the `match` statement whose case bodies must be blocks, anywhere else it is the `match` expression, whose cases follow the Blocks as Expressions rules and must be exhaustive, or have the `else` case.

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

    EXPR := IF | MATCH | OR_EXPR

An expression is either `if` expression, `match` expression, or it recurses into standard expressions based on operator precedence. The `if` and `match` rules are shared with the statements (see above). The single line `if` expression is equivalent to a ternary operator in C++. Inside brackets, an `if` or `match` expression ending with a `BLOCK` may be followed by further tokens (e.g. `(if c then` ... `else` ... `) + 1`). Outside of brackets this is not possible, as the lexer emits `newline` (or `dedent`) after the block.

    OR_EXPR := AND_EXPR { 'or' AND_EXPR }
    AND_EXPR := NOT_EXPR { 'and' NOT_EXPR }
    NOT_EXPR := 'not' NOT_EXPR | EQ_EXPR

Logical operators, no surprise here. `not` has lower precedence than the binary ones, so `not a and b` is equivalent to `(not a) and b`. 

    EQ_EXPR := CMP_EXPR { '==' CMP_EXPR }
            |= CMP_EXPR { '!=' CMP_EXPR }

Equality comparisons are as expected from other languages. Shortcircuiting works based on the underlying traslation.  

    CMP_EXPR := ADD_EXPR { ( '<' | '<=' | '>' | '>=' ) ADD_EXPR }

Note that relation operators can be chained. And the semantics require that each element is evaluated *once* and then for the inner ones might be compared twice. Shortcircuiting works based on the underlying traslation.

    ADD_EXPR := MUL_EXPR { ( '+' | '-' ) MUL_EXPR }
    MUL_EXPR := UNARY_EXPR { ( '*' | '/' ) UNARY_EXPR }
    UNARY_EXPR := ( '-' | 'clone' ) UNARY_EXPR | POSTFIX_EXPR

Arithmetic operators, all left associative, and the prefix operators, which apply to the entire postfix expression that follows them.

    POSTFIX_EXPR := PRIMARY { '.' ident | '[' EXPR ']' | PAREN_EXPR | 'orelse' ORELSE_RHS }
    ORELSE_RHS := UNARY_EXPR | 'return' [ EXPR ] | 'break' | 'continue' | BLOCK

Field access (or enum value selection), indexing, function call and `orelse` all share the same precedence and are applied left to right. Function call arguments use the same rule as parenthesized expressions and tuple literals, see below.

The right hand side of `orelse` is a unary expression, so that `a + foo() orelse 1 + 2` is `a + (foo() orelse 1) + 2`. Since the unary expression itself contains postfix expressions, chained `orelse` is right associative, i.e. `foo() orelse bar() orelse 0` is `foo() orelse (bar() orelse 0)`, which has the same meaning. When followed by `return`, its value extends to the end of the statement. When `orelse` is followed by `indent`, the right hand side is a `BLOCK`. That the `orelse` is used exactly on optional values (calls of `optional` returning functions and maps) and that the block follows the Blocks as Expressions rules are semantic checks.

    PRIMARY := LITERAL | ident | 'self' | 'none' | PAREN_EXPR
    LITERAL := number | string | 'true' | 'false'
            |= ARRAY_LITERAL | MAP_LITERAL | FUN_LITERAL

The primary expressions are literals of the primitive types, identifiers (variables, functions, types when used as constructors and enum values in their shorthand form), `self` inside methods and `none`.

    PAREN_EXPR := '(' LIST(NAMED_EXPR) ')'
    NAMED_EXPR := EXPR [ '=' EXPR ]

Parentheses serve multiple purposes: grouping of expressions (`(a + b)`), tuple literals (`(foo = 1, bar = 56)`) and function call arguments (`foo(a, 56)` or `inc(x = 56)`). All are parsed by the same rule and differentiated afterwards: in `PRIMARY` position, a single item without `=` is a grouping, otherwise all items must be in the `ident '=' EXPR` form to form a tuple literal. In `POSTFIX_EXPR` position, it is a function call. When used as call arguments, both forms can be used. This way the grammar stays LL(1) even though we only know which form is used after the first expression has been parsed. Note that the `=` here is not an assignment operator. 

    ARRAY_LITERAL := '[' LIST(EXPR) ']'
    MAP_LITERAL := '{' LIST(MAP_ITEM) '}'
    MAP_ITEM := EXPR '->' EXPR

Array literals are list of values in square brackets, map literals are list of key-value pairs separated by the arrow operator in curly brackets. Empty `[]` and `{}` are valid, but their types must be known from the context.

    FUN_LITERAL := 'fun' [ '(' LIST(LAMBDA_ARG) ')' [ '->' [ 'optional' ] TYPE ] ] BLOCK
    LAMBDA_ARG := ident [ ':' TYPE ] [ '=' EXPR ] | LITERAL

Anonymous function starts with the `fun` keyword, optionally followed by the signature (arguments and optional return type) and then the function body, which is a `BLOCK`. The two forms are differentiated with a single token lookahead after `fun`: `(` starts the explicit signature, `indent` starts the body. There is no `=` between the signature and the body. 

When the signature is omitted, it is taken from the function type known from the context and the arguments are bound under their names from the type. When the signature is given, argument types may be omitted if the function type is known from the context, but all arguments must always be listed. Literal arguments are only valid for event handlers where they are used for the value matching. Whether the function type is known from the context, and therefore which parts of the signature may be omitted, is decided semantically.

Outside of brackets, the function body is followed by `newline` (or `dedent`), so an anonymous function must always be the last part of the statement it appears in. Inside brackets, the body is closed by the lexer at the next line of items, or by the closing bracket, after which the list continues as usual.


# Appendix C - Runtime

There is no main function per se.

# Appendix D - Future Extensions

- int type and bitwise operators

