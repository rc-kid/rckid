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

    enum, let, const, fun, bool, num, true, false

> TODO and others

## Built-in Types

_doLang_ supports rational numbers, boolean values, text strings and simple enumerations as primitive types. Generic arrays, maps and tuples are supported as well. Contrary to mainstream languages, floating point values are *not* supported. 

### `num`

The basic numeric types. All numbers in the language are rational numbers underneath. Uses the `/` (division) character to differentiate between numerator and denominator. This works nicely as the division is *exactly* what constructs the rational number. 

When the numbers are printed or visualized, the same notion is used, i.e. `3/4`. Note that this conflicts with the operator precedence, i.e. `1 + 3/4` read from left to right would evaluate to `1.0` instead of the "obvious" `1.75`. In the chevron based editors, this can be fixed by having the `3/4` as immediate value inside a single chevron, while at the token level and above, parentheses must be added around the value.

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

### `tuple`

Tuple is sequence of *named* values:

    let t : ( foo : num, bar : num) = ( foo = 1, bar = 56)

### `array`

A simple array. Array is generic so we can have arrays of other types and even arrays of arrays. Arrays are created with `[]`, i.e. :

    let a : [num]

All types in the array have to be identical. Type inference can be used where the type of the first item will be the type all types of the array. Arrays are growable and shrinkable and support a few methods themselves.

Inline array initialization is possible with the `[]` parentheses as well:

    let a = [1, 2, 3]

### `map`

This is the associative array. Generic on key and value. Maps are created with `{}`, i.e. the following is a map from number to text:

    let m : {num, text}

All types in the map have to be identical. Type inference can be used from the first type. The `{}` parentheses can also be used for inline initialization, such as:

    let m = {
        1 = "foo",
        2 = "bar",
    }

### Special value types

_doLang_ supports a variety of special purpose value type, but object-like types, such as `Point`, `Color`, etc. They can be used the same way user defined objects can be, but they are *not* user defined objects, cannot be inherited from, cannot have extra methods created, etc. And unlike the user defined types, those are value types, i.e. passed, returned and assigned by value.

## Functions

Functions can be either named:

    fun myFirstFunction(arg : num) =
        ...

or anonymous:

    let a : fun(arg : num) =
        ...

If the function returns value, the `->` notation is used, e.g.:

    fun min(a : num, b: num) -> num = 
        if a <= b
            return a
        else
            return b

When the `->` part is omitted the function type is expected to not return anything (`void` in C/C++). Note that contrary to most mainstream languages, names of the arguments are technically part of the function _type_. This means that the following code is valid:

    let inc : fun(x : num) -> num
    inc = 
       return x + 1

However, it is also possible to rename the argument values by providing new function definition as part of the assignment, as long as the function definitionis compatible. So continuing the example from above, one can write:

    inc = fun(a : num) -> num
        return a + 1

Since the function type is already known from `a`'s definition, type inference can be used and a shorter version can be typed:

    inc = fun(a)
        return a + 1

Functions can have default values for some of their arguments, in which case those do not have to be specified when calling a function. The default value is part of the _type_. When a function is called, arguments can also be provided using their names as per the type, i.e. we can call all of the `inc` functions from above using this:

    inc(x = 56)

A function variable can be set to nothing by calling its `clear()` method. It also automatically casts to a boolean returning true if containing a handler, and false otherwise.

### Optional return types

A function return type can be declared as `optional`, which means the function may either return value of the given type, or `None`. Functions like this can only even be called using the `if let =` syntax and they serve as a primitive maybe type - i.e. functions (including user defined ones) can return it and we force the matching and check, but the optional values cannot be propagated, or stored. This makes the feature a lot simpler to understand:

    fun safeDivide(a : num, d : num) -> optional num = 
        if d != 0
            return a / d
        else 
            return None

### Capture

Unnamed function is a closure that can capture local variables. Local variables are captured is if by creating identically named variables inside the closure and assigning to them the outside variables, i.e. primitive types and pairs by value, objects, arrays and maps by reference.

Global variables do not need to be captured and can be referenced directly.

### Overloading

Functions can be overloaded, i.e. single function name can be used with different argument types and arities. Similar to mainstream programming languages, overloading on the return type alone is *not* allowed. 

## Events

Events are like functions in the sense that they can be called like a function and they can be assigned functions. The difference is that an event can be assigned multiple functions. When the event is called, *all* the associated functions (handlers) will be called as well, in the order in which they were added to the event.

Events can either be used as types, i.e.:

    let a : event(a : num, b : num) 

Or as named events inside objects. Note that named events are onky available in object definitions, not in anonymous tuples:

    type Object = (
        field1 : num,
        event onSomething(a : num) -> num
    )

Like functions, events can return values, or nothing (`->` missing) and they may even provide default values for their arguments. 

When an event is assigned a function, that function becomes is sole handler. But unlike functions, events also support the `+=` assignment, in which case the handler is *added* to the event:

    let a : event(a : num, b : num) 
    a +=
        ... # first
    a += 
        ... # second

    a(1, 2) # both first and second will be called
    
Events use value matching to allow for conditional dispatch, i.e. unlike the example above, consider the following:

    let a : event(a : num, b : num) 
    a +=
        ... # first
    a += fun(a, 56)
        ... # second

    a(1, 2) # only first will be called
    a(1, 56) # first and second will be called

### API

Event can be called (fires the event), assigned to (adds the handler) or have its object-like methods called:

- `event.clear()` clears all handlers registerd for the event.
- `event.numHandlers() -> num` returns the number of handlers associated with the event

Events implicitly cast to booleans, `false` if no handler, `true` if at least one handler is assigned.

## Objects

User defined object types, which must always be named and may contain fields. Note that an object is, in its simplest form just a named tuple, which is reflected in the declaration syntax:

    type ObjectName = (
        field1 : num = 67,
    )

When the field is event, the canonical declaration is using the `event` keyword, which is a shorthand for actual type definition as shown below

    type ObjectWithEvent = (
        field1 : num = 67,
        event onDone(status : num), # shorthand
        eventField : event(status : num), # long form
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

    fun Complex.increaseBy(other : Complex) =
        self.re += other.re
        self.im += other.im

The out-of-declaration syntax is useful so that functionality of objects can be extended without touching the objects themselves, i.e. a child may extend say a sprite with method functionality at any point.

### Inheritance

Object can inherit from others. Onky single inheritance is supported and _doLang_ does *not* support virtual methods, meaning that while objects can be extended, their base functionality cannot be changed. A child object can however be used in place of its parent object types:

    type FancyComplex extends Complex = (
        fancy : num,
    )

### Constructors

To create an object, the name of the object type is used as if it were a function. Empty arguments will create the object using the default initialization defined for its fields (either via `=`, or if absent via the default initialization for their respective type). Alternatively, a keyword style arguments can be provided as if the type was a function call. Only keyword style arguments are allowed:

    let x = Complex() # re == 0, im == 0
    let i = Complex(im = 1) # re == 0, im == 1

User defined constructors are not supported, instead free functions can be defined with intended maning, such as:

    fun Im(a : num) -> Complex =
        return Complex(im = a)

    let a = Im(56)

However, this simple arcitecture makes construction of invalid object states possible. Canonical example of this is the rational number - and even though it makes no sense in _doLang_ context where rational is built-in - we will thus use it to illustrate as well. The problem is that certain values are invalid without fallback (denominator being zero), while some must be normalized. This can be done by specifying a `constructor` block in the type definition, where the block is a function that is guaranteed to run after the object is created:

    type Rational = (
        n = 0,
        d = 1,
        constructor = 
            test self.d != 0
            if self.d < 0
                self.d *= -1
                self.n *= -1
            self.normalize()
    )

    fun Rational.normalize() =
        ...

Constructor blocks can only be used in named objects, not anonymous pairs.

## Binding

_doLang_ supports three binding types: `let`, `const` and `global`. For assignments, function arguments and return values the following hold true and *cannot* be changed in the language:

- objects, arrays and maps are passed by reference, 
- the other types are passed by value.

### `global`

Declares new global mutable variable. Only valid at the top level. Syntactically `global` is identical to `let`, e.g.:
    
    global foo : num = 67 # type is optional if deducible from the assignment rhs


### `let`

`let` creates new local variable, only available inside functions and methods. When a let bound variable is assigned, the effect depends on the type. Primitive values and pairs are copied, while arrays, maps and objects will be referenced.

    let mySprite = globalSprite
    # mySprite and globalSprite are two different sprite objects

### `const`

Declares constant primitive value (anything but object, array or map). Can be used in both global and local scopes (i.e. whenever `let` or `global` can be used). Value of a constant variable cannot be changed after the initial assignment.

    const pi = 3.14

_doLang_ does not support constant objects, maps and arrays. 

### Cloning

Because objects, arrays and maps are always passed by reference, any assignment creates an alias. In cases this is not wanted, and we do want a copy of the object, the `clone` keyword can be used, which instead of by reference assignment triggers a deep copy:

    let a = clone b # assuming b is an object array or map

## Assignments

The following assignment operators are supported:

    =, +=, -=, *=, /=, %=

Assignment operators work as expected, but restrictions are placed on their placement as they can only be at the root position of standalone expressions or inside let and const statements. This also enforces only one assignment operator per expression. 

> Note that the assignment inside `fun`, `enum` and `type` definitions is not really assignment operator, but part of the specific rules with distinct semantic meaning.

## Operators

_doLang_ follows standard operator precedence so as not to confuse kids with specific evaluation rules, namely:

- field access operator `.` (or enum value selection)
- indexing operator `[]`
- unary `-` and unary `not`
- multiplication and division (`*`, `/`)
- addition and subtraction (`+`, `-`), text concatenation (also `+`)
- comparison operators (`<`, `<=`, `>`, `>=`)
- equality comparisons (`==`, `!=`)
- logical `or`
- logical `and`

Short-circuiting of logical operators is supported. 

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

The `if` statement supports conditional execution and can be optionally followed by else case. There is a special case of the statement that deals with the `optional` returning functions and map index operator:

    if let a = map["key"]
        ... # a is bound to value of key in map, if one existed
    else
        ... # key not in map (the index operator returned None)

This is the only `if` form where assignment operator is allowed inside the if expression.

`if` statement can also be used as an expression, i.e. one can write for instance:

    fun min(a : num, b : num) -> num =
        return if a <= b a else b

In the expression form the `if` statemet *must* include the `else` clause and only a single expression is allowed in either true or false case. Both expressions must return same type. 

### `while` and `do-while`

Those are simple loops available in the language. Their syntax is fairly straightforward:

    while a < 10
        a += 1

    do
        a += 1
    while a < 20

The loops also support `break` and `continue` statements, which do what they are expected to, i.e. stop the loop immediately and move to the next iteration respectively. 

### `for` loop

The `for` loop serves a double duty. First the sequence iterator, which automatically creates let binding of the induction variable. 

    for i in 1 <= i < 100
        ... # simple loop with induction variable

    for i in 1 <= i < 100 step 2
        ... # custom step

This notation neatly solves the inclusivity or exclusivity of the bounds, but has limitations such as both lower and upper bounds must be present and the middle value *must* be the induction variable itself.

Then a for-each variant for arrays and maps:

    for value in array
        ...

    for value, index in array:
        ...

    for key in map.keys
        ...

    for value in map
        ...

    for key, value in map

During the for each loop variants, any access to the arry or map itself other than the element is *not* allowed.

For loop supports the `break` and `continue` statements as well.

### `match`

This is _doLang_'s switch on pattern-matching steroids. No fallback between different cases is allowed and each case is its own block:

    match value
        case a
            ...
        case b
            ...
        else
            ...

The `else` case is executed if *none* of the cases above applies. The case statements must be disjunctive, i.e. for any value, only one can apply. Case descriptors support extremely simple form of pattern matching to existing values, similar to that of the event handlers:

- primitive values can be matched by their values, including enums
- arrays and tuples can be matched by their contents

As an example:

    match complexTuple
        case (re : 1, im):
            ... # we now have re and im in scope

    match array
        case [1, a, b]:
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

_doLang_ does not suppport namespaces. The reason for this is that namespaces complicate things and force kids thing about ambiguity resolution. Instead _doLang_ does not allow ambiguity to exist at all, which is probably fine as the projects in the language are not expected to be enormous.

## Standard Library

- mod(x, y)
- pow(x, y)
- sqrt(x)


# Appendix A - Program Execution

There is no main function per se.


# Appendix B - Future Extensions

- byte type and bitwise operators

