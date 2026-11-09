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

    and, bool, break, clone, const, constructor, continue, do, else, enum, event, extends, false, for, fun, global, if, in, is, let, match, none, not, num, optional, or, orelse, return, self, step, test, text, then, true, type, use, when, while, write, xor

> TODO and others. Note that `xor` is reserved for the future bitwise operators.

### Indentation

_doLang_ uses indentation to denote nesting. Only spaces can be used for indentation. The rules are simple:

1. Every block of statements has its *indentation*, the column at which all its statements start. A line starting exactly at this column starts a new statement.
2. A line indented *more* continues the current statement.
3. Except when the line follows a keyword that starts a block (`then`, `else`, `do`, `when`), the `fun` keyword, or a function signature. Then the more indented line starts a new block whose indentation is the column of the line.
4. A line indented *less* ends the block (or blocks) and must start exactly at the indentation of one of the enclosing blocks.

So statements can span multiple lines, as long as the continuation lines are indented more than the beginning of the statement:

    let ok = lives > 0 and not gameOver
        and score < limit
    foo() # starts at the same column, so it is a new statement

This means that a line starting at the same column as the statement always starts a new statement, so there is no confusion whether the line belongs to the previous statement or not. The indentation of the top level is the indentation of the first line of the file.

Blocks always start on a new line that is indented more than the line that introduced them. The statements that have a block end with a keyword (`if ... then`, `while ... do`, `for ... do`, `match ... when`, `is ... then`), so their headers can span multiple lines as well. Function bodies always follow the `fun` keyword, either directly, or after the function signature (`fun(...)`). A block is only ever introduced by these keywords, or the function signature, never by an operator. In particular, a newline after `=` (or any other assignment operator) is always a continuation of the statement:

    d =
        1     # continuation of the statement, d is assigned 43
        + 42
    n = fun
        1     # function body, block of 2 statements (1 and + 42)
        + 42

Going back to a column that is not the indentation of any of the enclosing blocks is an error:

    if a then
            foo()
        bar()     # error, column 4 is neither the if block, nor the top level

### Brackets

Lists of items inside `()`, `[]` and `{}` brackets can be written in two forms. When the first item is on the same line as the opening bracket, the list is in its *single line* form. All items are separated by commas and a trailing comma after the last item is allowed. The whole list is a single logical line, i.e. the standard rules for an expression spanning multiple lines apply and any line break inside it is a continuation, which must be indented more than the statement. For the same reason, a single line list cannot contain function bodies, or other lists in the multi-line form:

    foo(a, b,
        c)           # three arguments
    foo(a, b,
        c,
    )                # the closing bracket can be on its own line too

When the opening bracket is the last token on its line, the list is in its *multi-line* form and the items behave the same as statements in a block: the indentation of the first item is the indentation of the list, a line starting at this column starts a new item and a line indented more continues the current item. Items on the same line are separated by commas, while at the end of the line the comma is optional (and a trailing comma after the last item is allowed). The closing bracket is on its own line, at the indentation of the statement (or of the enclosing list item) the list is part of:

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
    # or, with multiple items per line
    let m = [
        1, 2, 3
        4, 5, 6
    ]

Since the items follow the same rules as statements, a line at the item column is always a new item:

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

Function bodies can appear inside brackets too, but only in the multi-line form. As any other block, a function body ends with the first line indented less than the body, i.e. the next item, or the line with the closing bracket. The line break is thus the only separator after a function body. A comma at the end of the body's last line would be part of its last statement, which is an error:

    type Player = (
        onHit = fun(damage : num)
            if damage > 10 then
                playSound(ouch)
            self.lives -= damage
        lives = 3
        constructor = fun
            test self.lives > 0
    )

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

or anonymous (function literals, or lambdas):

    let a = fun(arg : num)
        ...

A function _body_ is always an indented block of statements starting on a new line, and always follows the `fun` keyword, either directly, or after the signature. There is no `=` between the signature and the body.

If the function returns value, the `->` notation is used, e.g.:

    fun min(a : num, b : num) -> num
        if a <= b then
            return a
        else
            return b

When the `->` part is omitted, the function does not return anything (`void` in C/C++).

Function variables can be assigned a lambda, existing global function, or method of a particular object as long as their types are identical (see below). Function variable must *always* point to a valid function and hence be callable. For potentially empty function-like variables one can use events instead.

### Function types and signatures

Contrary to most mainstream languages, argument names are *not* part of the function type. We distinguish:

- the function _type_, which consists of the number of arguments (arity), their types and the return type. Only the type decides whether functions are compatible (i.e. whether a function can be assigned to a function variable) and whether two functions are different overloads.
- the function _signature_, which is the type together with the argument names and their default values.

Signatures are written in named function declarations, function types (`fun(a : num, b : num = 1) -> num`) and event types. In all of these, every argument must have its type, and may have a default value:

    fun inc(x : num, by : num = 1) -> num
        return x + by

    let f : fun(x : num, by : num = 1) -> num = inc

Calls always use the signature of what is being called, i.e. of the named function, of the declared type of the function variable, or of the event. That is where the names for keyword arguments and the default values come from (see Calling Functions). So a function with default values can be assigned to a function variable whose type has no defaults, or different ones, as their types are the same. Calls of the variable then use the variable's signature.

### Function literals

Function literals (lambdas) are almost always used in a context in which their type is known, such as when assigned to a function variable, or an event, or passed as an argument to a function. This is the _expected type_. Therefore all parts of the lambda's signature are optional and the missing ones are taken from the expected type:

- `fun` followed directly by the body takes the whole signature from the expected type and the arguments are available in the body under their names from the expected type (see Argument names below)
- `fun(...)` with argument names renames the arguments. All arguments must always be listed
- argument types and the return type can be given, or left out, in which case they are taken from the expected type

The following are all equivalent ways of creating a function variable `a`:

    let a : fun(arg : num) = fun
        ...

    let a : fun(arg : num) = fun(arg)
        ...

    let a : fun(arg : num) = fun(arg : num)
        ...

When the expected type is not known, as in the example below, the lambda itself gives the type of the variable and must therefore specify all argument types. If the return type is missing, the function does not return anything, the same as named functions:

    let a = fun(arg : num)
        ...

Any part of the signature given explicitly must match the expected type, only the argument names can differ. A missing part that cannot be taken from the expected type, or a lambda without the signature with no expected type at all, is an error.

Lambdas cannot specify default values for their arguments. If a function variable needs default values, they must be specified in its type:

    let a : fun(x : num, by : num = 1) -> num = fun
        return x + by

The expected type comes from:

- the type of the variable in assignments and `let`, `const` and `global` declarations
- the event or function in event assignments (`button.onPress += fun`)
- the argument type when the lambda is passed as an argument of a function call
- the field type when used as the default value of a field in type declaration
- the return type when returned from a function
- the element type of an array, or the value type of a map literal, if known

### Argument names

Argument names affect keyword calls, editor visualizaton and implicit bindings: When a body is given with the implicit signature (`fun` without parentheses), the arguments are automatically bound under the names from the expected type. This means that the following code is valid:

    let inc : fun(x : num) -> num
    inc = fun
       return x + 1 # x is bound from inc's type

This is very useful for function variables and events, especially those declared elsewhere (such as in objects or libraries), as the handler can be written without repeating the signature:

    button.onPress = fun
        ... # arguments of onPress available under their declared names

However, it is also possible to rename the arguments by providing the signature explicitly in parentheses. So continuing the example from above, one can write:

    inc = fun(a : num) -> num
        return a + 1

or, without the parts known from `inc`'s type:

    inc = fun(a)
        return a + 1

Implicitly bound arguments must not hide any other variable visible at the place of the body. For example, if there is a global variable `x`, the `inc = fun` body from above is an error and the arguments must be renamed using the explicit signature. This prevents the confusing situation where `x` in the body looks like the global variable, but is in fact the argument.

> The editors should always display the implicitly bound arguments, e.g. as a greyed out signature above the body, so that the argument names are visible even if they are not part of the code.

Note that renaming the arguments in the lambda does not change the signature of the variable it is assigned to, i.e. all of the `inc` functions above are called with `inc(x = 56)`.

### Optional return types

A function return type can be declared as `optional`, which means the function may either return value of the given type, or `none`. Optional is a primitive maybe type - functions (including user defined ones) can return it and the caller is forced to handle the `none` case right at the call site, but the optional values themselves cannot be stored, or passed around. This makes the feature a lot simpler to understand:

    fun safeDivide(a : num, d : num) -> optional num
        if d != 0 then
            return a / d
        else 
            return none

Every call of a function returning `optional` must be immediately followed by the `orelse` operator, which provides the value to be used when the function returns `none`:

    let x = safeDivide(a, b) orelse 0

The fallback value must be of the same type as the optional value and it is only evaluated when the function returned `none`. The `orelse` is part of the call: it attaches to the call right before it and takes *everything* after it. When the result is used in a larger expression, the call with its `orelse` must be put in parentheses:

    let z = safeDivide(a, b) orelse c + d        # safeDivide(a, b) orelse (c + d)
    let y = a + safeDivide(a, b) orelse 1 + 2    # a + (safeDivide(a, b) orelse (1 + 2))
    let y = a + (safeDivide(a, b) orelse 1) + 2  # use parentheses to stop the fallback
    let w = safeDivide(a, b) orelse safeDivide(a, c) orelse 0 # orelse can be chained

As `orelse` can only follow a call, something like `foo() + 3 orelse 4` is a syntax error. Note that the fallback taking everything also applies to the uses of the value, i.e. `m(k) orelse 0 > 5` is `m(k) orelse (0 > 5)`, which is a type error, while `(m(k) orelse 0) > 5` is the comparison of the value. Similarly, nothing can follow the fallback, so to access a field of the result, parentheses are needed: `(foo() orelse bar).x`.

Instead of a value, the fallback can also be `return` (with optional value), `break` or `continue`, in which case the execution leaves the expression when `none` was returned. This is particularly useful for early returns and `none` propagation:

    fun average(values : {text -> num}, a : text, b : text) -> optional num
        let x = values(a) orelse return none
        let y = values(b) orelse return none
        return (x + y) / 2

The fallback cannot be a block of statements. When more statements are needed to handle the `none` case, an `if` statement, or a function call can be used instead.

### Calling Functions

When a function is called, all arguments without default values must be given value. Arguments can be specified either positionally, or via their names from the signature of what is called (see Function types and signatures). Argument matching can start positional and then shift to keyword based:

    foo(1, 2, bar = 67)

The positional arguments must come first, i.e. once an argument is given by name, all arguments after it must be given by name as well. Each argument can only be given once, i.e. specifying an argument both positionally and by name is an error. Note that the call arguments look the same as the tuple literal, i.e. the call can be thought of as passing a tuple of values to the function's arguments.

### Capture

Unnamed function is a closure that can capture local variables. Local variables are captured is if by creating identically named variables inside the closure and assigning to them the outside variables, i.e. primitive types and tuples by value, objects, arrays and maps by reference.

Global variables do not need to be captured and can be referenced directly.

### Overloading

Functions can be overloaded, i.e. single function name can be used with different argument types and arities. Similar to mainstream programming languages, overloading on the return type alone is *not* allowed. Overloading on the argument names (or default values) alone is not allowed either, as those are not part of the function type.

When an overloaded function is called, the overloads are first selected by the number of arguments and by the names of the keyword arguments and then by the argument types. When a lambda is passed as an argument and more than one overload has a function argument at its position, the expected type of the lambda is not known and the lambda must specify all argument types itself. Similarly, an overloaded function can only be assigned to a function variable whose type is known, i.e. `let f = foo` is an error if `foo` is overloaded.

## Events

Events are like functions in the sense that they can be called like a function and they can be assigned functions. Events are particularly useful as fields of objects:

    type Object = (
        field1 : num,
        onSomething : event(a : num)
    )

Unlike functions, events may not return values (`->` missing), but they may provide default values for their arguments. Same as function types, every event argument must have its type. To trigger an event, the event should be called as if it were a function, i.e. the arguments must begiven. Events also implicitly cast to boolean values. An event with no handler is `false`, all other events are `true`

When an event is assigned a function, that function becomes its sole handler. But unlike functions, events also support the `+=` assignment, in which case the handler is *added* to the event. When multiple handlers for the event exist, they are called in the order in which they were added to the event:

    let a : event(a : num, b : num) 
    a += fun
        ... # first
    a += fun
        ... # second

    a(1, 2) # both first and second will be called, in this order

All event handlers can be cleared if they are assigned `none` (`+=` assignment of `none` is syntax error). An event can also be queried about the number of its handlers vioa the `numHandlers() -> num` method. 
    
Event handlers can use pattern matching to allow for conditional dispatch. Any argument of the handler's signature can be followed by `is` and a pattern (the same patterns as used by `match` and the `is` operator), in which case the handler is only called when the argument matches the pattern, i.e. unlike the example above, consider the following:

    let a : event(a : num, b : num) 
    a += fun
        ... # first
    a += fun(a, b is 56)
        ... # second

    a(1, 2) # only first will be called
    a(1, 56) # first and second will be called

This is particularly useful for enums, such as:

    button.onPress += fun(btn is A, pressed)
        ...

Pattern matching requires the explicit signature and patterns are only allowed in handlers assigned to events, not in lambdas assigned to function variables. Note that the `=` is never used for matching in the handler's signature, as it would look like a default value, which lambdas cannot have.

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

- postfix operators: field access `.` (or enum value selection), indexing `[]` and function call `()`, optionally followed by `orelse` (see Optional return types)
- prefix operators: unary `-` and `clone`
- multiplication and division (`*`, `/`)
- addition and subtraction (`+`, `-`), text concatenation (also `+`)
- comparison operators (`<`, `<=`, `>`, `>=`)
- equality comparisons (`==`, `!=`)
- logical `not`
- logical `and`
- logical `or`
- pattern test `is` (and `is not`)

All postfix operators share the same precedence and are applied left to right, so that expressions such as `a[i].x` or `foo().bar[2]` work as expected. Prefix operators apply to the whole postfix expression that follows them, i.e. `-a.x` negates `a.x` and `clone a.items[2]` clones the element of `a.items`, not `a` itself.

Unlike the arithmetic negation, logical `not` has lower precedence than comparisons, so that `not a == b` means `not (a == b)`, which is how it reads in English.

### `is`

The `is` operator tests whether a value matches a pattern (the same patterns as used by `match`, see below) and evaluates to `bool`:

    let isOrigin = point is (x = 0, y = 0)
    if direction is not Left then
        ...

`is` has the lowest precedence of all operators and cannot be chained, so its left hand side is always the whole expression before it and it can only be combined with other operators inside parentheses: `a + b is 3` is `(a + b) is 3`, while `(a is 1) and (b is 2)` needs the parentheses. Note that this also means `not x is P` is `(not x) is P`, use `x is not P` instead.

When the `is` test is the whole condition of an `if` (statement or expression), or a `while` loop, the pattern may also bind values, which are then available in the `then` case, or in the loop body:

    if complexTuple is (re = 1, im) then
        ... # im is in scope here
    while it.next() is (key, value) do
        ...

Anywhere else, including the `is not` form, patterns that bind values are an error.

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

When the case is a single statement, it can be written on the same line as the keyword:

    if a < b then foo() else bar()

`if` can also be used as an expression, i.e. one can write for instance:

    fun min(a : num, b : num) -> num
        return if a <= b then a else b

Unlike the statement, the `else` case of the `if` expression is mandatory, both cases must have the same type and they can only be expressions, not blocks of statements. Same as the fallback of `orelse`, the `else` case takes *everything* after it, i.e. `if c then 1 else 2 + 3` is `if c then 1 else (2 + 3)`, and the whole `if` expression must be put in parentheses to be used as an operand: `(if c then 1 else 2) + 3`. As the `if` expression cannot contain blocks, any line break inside it is a continuation and long expressions can thus be broken into multiple lines:

    let x = if a < b then a else b
    let y = if a < b then a
        else if a < c then c
        else b

A statement starting with `if` is always the `if` statement, so to use the `if` expression as a statement, it must be put in parentheses.

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

This is _doLang_'s switch on pattern-matching steroids. No fallback between different cases is allowed and each case is its own block. The matched value is terminated by the `when` keyword, followed by a block of cases. Each case starts with the `is` keyword followed by a pattern and is terminated by the `then` keyword:

    match value when
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

    match complexTuple when
        is (re = 1, im) then
            ... # we now have re and im in scope

    match array when
        is [1, a, b] then
            ... # we have a and b in scope

A case may also have a guard, a condition after `and`, which must hold for the case to apply. The guard sees the values bound by the pattern:

    match point when
        is (x, y) and x == y then
            ... # on the diagonal

Same as `if`, `match` can be used as an expression, whose cases (including `else`) can only be expressions. All cases must have the same type and the `else` case is mandatory, unless the cases cover all possible values (such as all values of an enum). As the `match` expression cannot contain blocks, line breaks inside it are continuations, so the cases can be written on the same line, or each on its own line:

    let name = match direction when is Left then "left" is Right then "right"
    let x = match y when
        is (re = 1, im) then "real is one"
        else "real is not one"

The case values (and guards) of the `match` expression cannot use the `is` operator, unless in parentheses, so that `is` always starts the next case. A nested `match` expression in a case value must be put in parentheses too, otherwise it would take the remaining cases.

## `return`

Returns from the function, optionally returning value of required type. `return` (as well as `break` and `continue`) is an expression, so it can also be used as the fallback of the `orelse` operator, or a case of the `if` and `match` expressions. This is particularly useful for early returns and `none` propagation:

    let x = map("key") orelse return none
    # do some more with x

## `write`

`write` is a simple statement that allows debug printing to the output (serial to usb on the device usually). It is followed by a text expression.

## `test`

`test` followed by a boolean evaluating expression is _doLang_'s version of assertion. Upon test failure, the program is paused and user is informed of the invariant problem.

## Type Inference

While _doLang_ uses static typing and every binding as well as function can be typed, type inference is heavily used to avoid the need for writing the types. Crucially, let bindings and tuple and object fields do not have to be explicitly typed if their type can be inferred from the assignment (which is most cases, except an empty array or map). 

When declaring new functions, the functions themselves must be typed explicitly, i.e. all arguments of named functions must have types, but note that function definition is fairly complex task already so the need for explicit typing is likely ok. Lambdas on the other hand take the missing parts of their signature from the expected type (see Function literals).

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

The lexer turns the source into a stream of tokens. Line breaks are reported as `newline` tokens that carry the indentation (number of leading spaces) of the next line. The parser then translates them into `indent`, `dedent` and `newline` tokens of the grammar (see Layout below), so that the grammar rules never need to look at columns.

## Tokens

- *identifiers* start with a letter or `_`, followed by letters, digits and `_`. Identifiers are case sensitive. Keywords (see Keywords) are not identifiers.
- *numbers* are decimal (`123`, `1_000`, `1.567`), hexadecimal (`0x7f`), binary (`0b101`) or octal (`0o17`). Floating point requires digits on both sides of the `.`. A `-` is never part of the number, negative values are the unary minus.
- *text* is enclosed in `"` and may contain the escapes described in Literal Values. Text cannot span multiple lines.
- *operators and punctuation*, longest match wins (i.e. `->` is never `-` followed by `>`):

        +  -  *  /  =  +=  -=  *=  /=  ==  !=  <  <=  >  >=  ->  .  ,  :  |  (  )  [  ]  {  }

Whitespace between tokens is ignored. Comments (`#` to the end of the line), empty lines and lines with only a comment are ignored entirely and do not affect the layout, with one exception: if the first non-empty line of the file is a comment, its indentation is the indentation of the top level. Only spaces may be used for indentation, a tab in the indentation is an error. Since columns are only ever measured on the first token of a line, they are simply the number of leading spaces.

## Layout

The lexer always starts with a `newline` token that gives the indentation of the first non-empty line, which is the indentation of the top level. After that, a `newline` token is emitted for every line break (empty lines and lines with only comments do not count), with the indentation of the line that follows.

The parser keeps a stack of indentations, starting with the indentation of the top level. The top of the stack, `I`, is the indentation of the current block. It also keeps a *block start* flag, which the grammar rules that introduce a block set explicitly after the keyword that starts it (`then`, `else`, `do`, `when` in the statements, `fun` and the function signature), or after an opening bracket, and which is cleared when the next token is consumed. The flag is never set by the keyword alone, so the `if` and `match` expressions, which use the same keywords but cannot contain blocks, simply do not set it. Inside a single line list, an `indent` after a block keyword, or an opening bracket is an error, as single line lists cannot contain blocks, or multi-line lists.

A `newline` with indentation `c` is translated as follows:

1. `c == I`: `newline` is emitted, i.e. a new statement (or a new line of items) starts. The only exceptions are lines starting with `else`, or a closing bracket (`)`, `]`, or `}`), which never start a statement, so no `newline` is emitted and the `else` continues the `if` (or `match`) statement, while the closing bracket closes the list.
2. `c > I`: if the block start flag is set, `indent` is emitted and `c` is pushed on the stack. Otherwise the line is a continuation and nothing is emitted.
3. `c < I`: `dedent` is emitted and `I` is popped from the stack. This is repeated as long as `c < I`, after which rule 1 applies. If `c` falls between two indentations on the stack, it is an error.

At the end of file, `dedent` is emitted for every indentation on the stack above the top level.

Note that `indent` is only emitted after the block keywords, function signatures and opening brackets, never just because a line is indented more. Any other line indented more is a continuation.

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

- statements, as well as lines of items in multi-line lists, are separated by `newline`
- multi-line lists are `indent` ... `dedent` inside the bracket tokens
- single line lists contain no layout tokens at all
- `else` after a block arrives as `dedent 'else'`, and the closing bracket after a multi-line list as `dedent ')'`, without `newline`

# Appendix B - Grammar

The grammar works on the tokens produced by the lexer (see Appendix A). Nonterminals are uppercase, terminals are either quoted (`'if'`, `'('`), or lowercase: `ident`, `number`, `string` and the layout tokens `newline`, `indent` and `dedent`. `[ X ]` is optional, `{ X }` is repeated zero or more times and `|` separates alternatives (`|=` continues the alternatives on the next line).

The grammar is LL(1). Note that a line starting with `else`, or a closing bracket never produces `newline` (see Appendix A), so `else` after a block, or the closing bracket after a multi-line list follows its `dedent` directly.

## Common Rules

    BLOCK := indent STATEMENT { newline STATEMENT } dedent
           |= STATEMENT

A block is a sequence of statements, each starting on its own line. As `newline` is only emitted for lines at the indentation of the block, statements continued on further lines need no special handling. Statements ending with a nested `BLOCK` are followed by `newline` as any other statement, since it is emitted after the `dedent` tokens. A block consisting of a single statement can also be written on the same line as the keyword that introduces it (e.g. `if a then foo()`).

    LIST(X) := [ X { ',' X } [ ',' ] ]
             | indent LINE(X) { [ ',' ] newline LINE(X) } [ ',' ] dedent
    LINE(X) := X { ',' X }

Items inside brackets are either in the single line form, separated by commas with optional trailing comma, or in the multi-line form (the first item is on a new line), in which `indent` is emitted after the opening bracket, `newline` between lines and `dedent` before the closing bracket. Items on the same line are separated by commas, the comma at the end of a line is optional. After an item and an optional comma, `newline` starts the next line of items, `dedent` ends the list and any other token is the next item on the same line, which requires the comma. An item that continues on further lines, or ends with a block, needs no special handling. Blocks (function bodies) and multi-line lists cannot appear inside a single line list, the parser reports both as errors. The closing bracket is part of the rules that use `LIST`.

Blocks are only ever introduced by a keyword (`then`, `else`, `do`, `when`, `fun`), or a function signature, never by an operator. The right hand side of `=` (and other assignment operators) is therefore always an `EXPR`, function bodies on the right hand side are function literals (see `FUN_LITERAL`). The grammar is thus context free and the parser never needs to know the type of the left hand side.

## Top Level Statements

    PROGRAM := [ TOP_DECL { newline TOP_DECL } ]
    TOP_DECL := USE_DECL | GLOBAL_DECL | CONST_DECL | FUN_DECL | TYPE_DECL | ENUM_DECL | EXPR_STMT

Program consists of `use` declarations, global or constant declarations, function, type and enum declarations and event assignments, all starting at column `0`. That the `use` declarations come before all others is a semantic check.

Anything that does not start with a declaration keyword is parsed as `EXPR_STMT` (see Statements). Only assignments to events and function variables with `=`, or `+=`, such as `button.onPress += fun` followed by the handler, are valid at the top level, but that is a semantic check. The parser thus accepts any expression statement, including standalone expressions and other assignments, which are reported as errors by the type checker. The grammar stays LL(1), as the declaration keywords are checked first. A top level line starting with `fun` is always the `FUN_DECL`, never a function literal.

    USE_DECL := 'use' ident

`use` declaration is simply name of library to be used. Each library must be on separate line and with its own `use` keyword.

    GLOBAL_DECL := 'global' ident [ ':' TYPE ] [ '=' EXPR ]
    CONST_DECL := 'const' ident [ ':' TYPE ] [ '=' EXPR ]

Global variable declaration starts with `global` keyword and has name, optional type and initialization value. The value is optional and when omitted default constructor is used. When the global variable is a function or event, the value can be a function literal. Constant declaration is the same, just starts with `const` keyword. As constants can only be primitive values, they cannot be initialized with function literals.

    FUN_DECL := 'fun' ident [ '.' ident ] SIGNATURE(PARAM) BLOCK

Function declaration has a name, which for methods is prefixed with the object type name, the signature (arguments and optional return type) and the body. Same as anonymous functions, there is no `=` between the signature and the body. The `SIGNATURE` and `PARAM` rules are described with the function types below.

    TYPE_DECL := 'type' ident [ 'extends' ident ] '=' '(' LIST(FIELD_DECL) ')'
    FIELD_DECL := ident [ ':' TYPE ] [ '=' EXPR ] 
               |= 'constructor' '=' FUN_LITERAL

Object type declaration is a list of fields, each with name, optional type and optional default value, and optionally the constructor. Unlike function arguments, the field type can be omitted when it can be inferred from the default value. Both the constructor and function or event fields may use function literals inside the parentheses. The constructor must use the implicit signature form, i.e. `constructor = fun`.

    ENUM_DECL := 'enum' ident '=' ident { '|' ident }

Enum declaration is a list of its values separated by `|`. As a line break after `=` is a continuation, the values may start on the next line.

## Statements

    STATEMENT := LET_STMT | IF_STMT | WHILE_STMT | FOR_STMT | MATCH_STMT | RETURN_STMT 
              |= 'break' | 'continue' | 'write' EXPR | 'test' EXPR | EXPR_STMT

Statements that can appear inside function bodies. A statement starting with `if`, or `match` is always the `IF_STMT`, or `MATCH_STMT`, the `if` and `match` expressions are only parsed inside an expression.

    LET_STMT := ( 'let' | 'const' ) ident [ ':' TYPE ] [ '=' EXPR ]
    EXPR_STMT := EXPR [ ( '=' | '+=' | '-=' | '*=' | '/=' ) EXPR ]
    RETURN_STMT := 'return' [ EXPR ]

Local variable declarations, assignments and standalone expressions. Assignment is only allowed at the root of the expression statement. Whether the left hand side can be assigned to is a semantic check. `return` value is optional, there is no value when the next token is `newline`, or `dedent`. As `return` is also an expression (see `PRIMARY`), the value is in general absent whenever the next token cannot start an expression, such as `)`, `]`, `}`, `,`, `then`, or `else`.

    IF_STMT := 'if' EXPR 'then' BLOCK [ 'else' BLOCK ]

The `if` statement. The condition is terminated by `then`, so it can span multiple lines. `else if` chains need no special rule, as the single statement form of `BLOCK` can be another `IF_STMT`. As a line starting with `else` produces no `newline`, the `else` follows the `dedent` of the block directly. The `if` expression is described under expressions.

    WHILE_STMT := 'while' EXPR 'do' BLOCK
    FOR_STMT := 'for' ident [ ',' ident ] 'in' EXPR [ 'step' EXPR ] 'do' BLOCK

Loops, both terminated by the `do` keyword. The range form of the `for` loop (`for i in 1 <= i < 100 do`) is parsed as regular expression and its validity (chained comparison with the induction variable in the middle) is checked semantically.

    MATCH_STMT := 'match' EXPR 'when' indent MATCH_STMT_CASES dedent
    MATCH_STMT_CASES := CASE_STMT { newline CASE_STMT } [ 'else' BLOCK ]
                     |= 'else' BLOCK
    CASE_STMT := 'is' PATTERN [ 'and' EXPR ] 'then' BLOCK
    PATTERN := LITERAL | '-' number | ident { '.' ident } 
            |= '(' LIST(ident [ '=' PATTERN ]) ')' | '[' LIST(PATTERN) ']'

The `match` statement. The matched value is terminated by `when`, which is followed by a block of cases, each on its own line. There must be at least one case, which can be the `else` case alone. The `else` case is the last one and, as a line starting with `else` produces no `newline`, follows the last case directly. Each case is terminated by `then` and may have a guard after `and`, which sees the values bound by the pattern. Patterns are restricted to literals, identifiers (and enum values), and tuple and array patterns. Whether an identifier binds a new value, or matches an existing one is decided semantically. The `match` expression is described under expressions.

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

    FUN_TYPE := 'fun' SIGNATURE(PARAM)
    SIGNATURE(X) := ARGS(X) [ '->' [ 'optional' ] TYPE ]
    ARGS(X) := '(' LIST(X) ')'
    PARAM := ident ':' TYPE [ '=' EXPR ]

Function types start with the `fun` keyword followed by the signature, i.e. the argument list and optional return type after arrow operator, or the function returns void. Return type may be prefixed with `optional` keyword. All arguments lists (of named functions, function and event types and function literals) are the same list, only the rule for its items differs. Arguments of function types (and of named functions and event types) must have types and may have default values.

    EVENT_TYPE := 'event' ARGS(PARAM)

Event types are like function types without return value, and start with the `event` keyword instead.

## Expressions

    EXPR := OR_EXPR [ 'is' [ 'not' ] PATTERN ]

An expression recurses into standard expressions based on operator precedence. The `is` operator has the lowest precedence and is not associative, so it can only appear at the top of an expression (or inside parentheses). Patterns may bind values only when the `is` is the whole condition of `IF_STMT`, `IF_EXPR`, or `WHILE_STMT` and is not negated, which is a semantic check.

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

    POSTFIX_EXPR := PRIMARY { '.' ident | '[' EXPR ']' | PAREN_EXPR [ 'orelse' EXPR ] }

Field access (or enum value selection), indexing and function call all share the same precedence and are applied left to right. Function call arguments use the same rule as parenthesized expressions and tuple literals, see below.

The function call can be followed by `orelse`, so that only a call (including the map lookup, which is a call as well) can have the fallback. The right hand side of `orelse` takes everything, i.e. `foo() orelse 4 + 2` is `foo() orelse (4 + 2)`, `a + foo() orelse 1 + 2` is `a + (foo() orelse (1 + 2))` and `foo() orelse return 2 + 3` returns `2 + 3`. Chained `orelse` is right associative, i.e. `foo() orelse bar() orelse 0` is `foo() orelse (bar() orelse 0)`. As the right hand side takes everything, the postfix expression always ends with the `orelse`. That the call returns an optional value (and that every such call has the `orelse`) is a semantic check.

    PRIMARY := LITERAL | ident | 'self' | 'none'
            |= PAREN_EXPR | IF_EXPR | MATCH_EXPR | RETURN_STMT | 'break' | 'continue'
    LITERAL := number | string | 'true' | 'false'
            |= ARRAY_LITERAL | MAP_LITERAL | FUN_LITERAL

The primary expressions are literals of the primitive types, identifiers (variables, functions, types when used as constructors and enum values in their shorthand form), `self` inside methods, `none`, parenthesized expressions and the `if`, `match`, `return`, `break` and `continue` expressions. The last five take everything after them, the same as the fallback of `orelse`. That `return`, `break` and `continue` only appear where leaving the expression makes sense is a semantic check.

    PAREN_EXPR := '(' LIST(NAMED_VALUE) ')'
    NAMED_VALUE := EXPR [ '=' EXPR ]

Parentheses serve multiple purposes: grouping of expressions (`(a + b)`), tuple literals (`(foo = 1, bar = 56)`) and function call arguments (`foo(a, 56)` or `inc(x = 56)`). All are parsed by the same rule and differentiated afterwards: in `PRIMARY` position, a single item without `=` is a grouping, otherwise all items must be in the `ident '=' EXPR` form to form a tuple literal. In `POSTFIX_EXPR` position, it is a function call. When used as call arguments, both forms can be used. This way the grammar stays LL(1) even though we only know which form is used after the first expression has been parsed. Note that the `=` here is not an assignment operator. 

The `if` and `match` expressions:

    IF_EXPR := 'if' EXPR 'then' EXPR 'else' EXPR
    MATCH_EXPR := 'match' EXPR 'when' MATCH_EXPR_CASES
    MATCH_EXPR_CASES := CASE_EXPR { CASE_EXPR } [ 'else' EXPR ]
                     |= 'else' EXPR
    CASE_EXPR := 'is' PATTERN [ 'and' OR_EXPR ] 'then' OR_EXPR

The cases of `if` and `match` expressions can only be expressions, never blocks. These rules do not set the block start flag, so any line break inside them is a continuation. `else if` chains need no special rule as `IF_EXPR` is an `EXPR`. The guards and values of the `match` cases are `OR_EXPR`, so that they cannot contain `is` (unless in parentheses) and `is` always starts the next case. The `else` value is always last, so it can be any `EXPR`. There must be at least one case, which can be the `else` case alone. That the `match` expression is exhaustive, or has the `else` case is a semantic check.

    ARRAY_LITERAL := '[' LIST(EXPR) ']'
    MAP_LITERAL := '{' LIST(MAP_ITEM) '}'
    MAP_ITEM := EXPR '->' EXPR

Array literals are list of values in square brackets, map literals are list of key-value pairs separated by the arrow operator in curly brackets. Empty `[]` and `{}` are valid (an opening bracket immediately followed by a line with the closing bracket is an empty list as well), but their types must be known from the context.

    FUN_LITERAL := 'fun' [ SIGNATURE(LAMBDA_ARG) ] BLOCK
    LAMBDA_ARG := ident [ ':' TYPE ] [ 'is' PATTERN ]

Anonymous function starts with the `fun` keyword, optionally followed by the signature (arguments and optional return type) and then the function body, which is a `BLOCK`. The two forms are differentiated with a single token lookahead after `fun`: `(` starts the explicit signature, `indent` starts the body. There is no `=` between the signature and the body. 

When the signature is omitted, it is taken from the expected type and the arguments are bound under their names from the type. When the signature is given, all arguments must always be listed, but their types and the return type may be omitted and are then taken from the expected type. Lambda arguments never have default values. The pattern after `is` is only valid for event handlers where it is used for the conditional dispatch. Whether the expected type is known, and therefore which parts of the signature may be omitted, as well as whether patterns are allowed, is decided semantically.

Outside of brackets, the function body is followed by `newline` (or `dedent`), so an anonymous function must always be the last part of the statement it appears in. Inside brackets, which must be in the multi-line form, the body ends at the next line of items, or at the line with the closing bracket, after which the list continues as usual. As only line breaks end blocks, neither a comma, nor the closing bracket can follow the body on the same line.


# Appendix C - Runtime

There is no main function per se.

# Appendix D - Future Extensions

- int type and bitwise operators

