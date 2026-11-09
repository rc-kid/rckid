#include <cstdio>
#include <cstring>
#include <string>

#include <platform/tests.h>
#include <rckid/dolang/parser.h>

using namespace rckid;
using namespace rckid::dolang;

namespace {

    /** Parses the source
     */
    unique_ptr<ast::Node> parse(char const * source) {
        MemoryReadStream stream{immutable_ptr<uint8_t>{reinterpret_cast<uint8_t const *>(source), static_cast<uint32_t>(std::strlen(source))}};
        Parser parser{stream};
        return parser.parse();
    }

    /** Returns the text with newlines and tabs escaped, so that the whole of it fits on a single line of the test output. 
     */
    String escaped(String const & text) {
        return text.replace("\n", "\\n").replace("\t", "\\t");
    }

    /** True if parsing the source and printing it back gives the same source. If not, prints both, so that the failure can be seen in the test output. 
     */
    bool roundTrips(char const * source) {
        StringBuilder sb;
        auto parsed = parse(source);
        parsed->prettyPrint(sb.writer());
        String result{sb.str()};
        if (result == source)
            return true;
        std::cout << std::endl << "    expected: " << escaped(source).c_str() << std::endl << "    actual:   " << escaped(result).c_str() << std::endl;
        StringBuilder dbg;
        parsed->debugPrint(dbg.writer());
        std::cout << dbg.str().c_str();
        return false;
    }

    /** True if parsing the source and printing it back gives the expected text. For sources that have multiple valid layouts which print the same, such as optional commas in lists.
     */
    bool printsAs(char const * source, char const * expected) {
        StringBuilder sb;
        parse(source)->prettyPrint(sb.writer());
        String result{sb.str()};
        if (result == expected)
            return true;
        std::cout << std::endl << "    expected: " << escaped(expected).c_str() << std::endl << "    actual:   " << escaped(result).c_str() << std::endl;
        return false;
    }

    /** True if the debug print of the parsed source is the expected text. If not, prints both, so that the failure can be seen in the test output.

        Pretty printing should be used for most tests as eventually it should keep the formatting of the source. Debug prints are for the tests where the structure of the tree matters, such as operator precedence, which the pretty print does not show.
     */
    bool debugPrints(char const * source, char const * expected) {
        StringBuilder sb;
        parse(source)->debugPrint(sb.writer());
        String result{sb.str()};
        if (result == expected)
            return true;
        std::cout << std::endl << "    expected:" << std::endl << expected << "    actual:" << std::endl << result.c_str();
        return false;
    }

    /** True if the source does not parse. 
     */
    bool fails(char const * source) {
        auto result = parse(source);
        if (result->kind == ast::Kind::Error)
            return true;
        StringBuilder sb;
        result->prettyPrint(sb.writer());
        std::cout << std::endl << "    expected error, parsed as: " << escaped(sb.str()).c_str() << std::endl;
        return false;
    }

    /** Clears the interned symbols and texts at the end of a test, so that the heap is left as it was found. 
    */
    class ClearPools {
    public:
        ~ClearPools() {
            Symbol::clear();
            TextLiteral::clear();
        }
    };

} // anonymous namespace

// ------------------------------------------------------------------------------------------------
// top level declarations
// ------------------------------------------------------------------------------------------------

TEST(dolang, parser_empty) {
    ClearPools clear_;
    EXPECT(roundTrips("\n"));
}

TEST(dolang, parser_use) {
    ClearPools clear_;
    EXPECT(roundTrips("use foo\n"));
    EXPECT(roundTrips("use std_lib2\n"));
}

TEST(dolang, parser_varDeclWithoutValue) {
    ClearPools clear_;
    EXPECT(roundTrips("global x\n"));
    EXPECT(roundTrips("global x : num\n"));
    EXPECT(roundTrips("const x : bool\n"));
}

TEST(dolang, parser_multipleDeclarations) {
    ClearPools clear_;
    EXPECT(roundTrips("use foo\nuse bar\n"));
    EXPECT(roundTrips("use foo\nglobal x : num\nconst y : text\n"));
}

// ------------------------------------------------------------------------------------------------
// types
// ------------------------------------------------------------------------------------------------

TEST(dolang, parser_podTypes) {
    ClearPools clear_;
    EXPECT(roundTrips("global x : num\n"));
    EXPECT(roundTrips("global x : bool\n"));
    EXPECT(roundTrips("global x : text\n"));
    // user defined types are identifiers
    EXPECT(roundTrips("global x : Player\n"));
}

TEST(dolang, parser_arrayTypes) {
    ClearPools clear_;
    EXPECT(roundTrips("global a : [num]\n"));
    EXPECT(roundTrips("global a : [Player]\n"));
    EXPECT(roundTrips("global a : [[text]]\n"));
}

TEST(dolang, parser_tupleTypes) {
    ClearPools clear_;
    EXPECT(roundTrips("global x : ()\n"));
    EXPECT(roundTrips("global x : (a)\n"));
    EXPECT(roundTrips("global x : (a : num)\n"));
    EXPECT(roundTrips("global x : (a : num, b : text)\n"));
    // field types are optional
    EXPECT(roundTrips("global x : (a, b : bool, c)\n"));
    // any type can be a field type, tuples can be part of other types
    EXPECT(roundTrips("global x : (a : [num], b : {num -> text}, c : Player)\n"));
    EXPECT(roundTrips("global x : (a : (b : num, c), d)\n"));
    EXPECT(roundTrips("global x : [(a : num)]\n"));
    EXPECT(roundTrips("global m : {num -> (a : num)}\n"));
    // with value and in functions
    EXPECT(roundTrips("global x : (a : num) = y\n"));
    EXPECT(roundTrips("fun f()\n  let x : (a : num, b) = y\n"));
    // trailing comma
    EXPECT(printsAs("global x : (a : num, b,)\n", "global x : (a : num, b)\n"));
}

TEST(dolang, parser_tupleTypesMultiLine) {
    ClearPools clear_;
    EXPECT(roundTrips("global x : (\n  a : num\n  b : text\n)\n"));
    EXPECT(roundTrips("fun f()\n  let x : (\n    a : num\n    b\n  ) = y\n"));
    // nested multi-line tuple types
    EXPECT(roundTrips("global x : (\n  a : (\n    b : num\n    c\n  )\n  d\n)\n"));
    // commas
    EXPECT(printsAs("global x : (\n  a : num, b\n  c,\n)\n", "global x : (\n  a : num\n  b\n  c\n)\n"));
    // continuation of the field type
    EXPECT(printsAs("global x : (\n  a :\n    num\n  b\n)\n", "global x : (\n  a : num\n  b\n)\n"));
}

TEST(dolang, parser_tupleTypesStructure) {
    ClearPools clear_;
    EXPECT(debugPrints("global x : (a : num, b)\n",
        "Program@(1:1)\n"
        "  VarDecl@(1:1) global x\n"
        "    TupleType@(1:12)\n"
        "      FieldType@(1:13) a\n"
        "        SymbolName@(1:17) num\n"
        "      FieldType@(1:22) b\n"
    ));
    EXPECT(debugPrints("global x : (\n  a : num\n  b\n)\n",
        "Program@(1:1)\n"
        "  VarDecl@(1:1) global x\n"
        "    TupleType@(1:12) multi-line\n"
        "      FieldType@(2:3) a\n"
        "        SymbolName@(2:7) num\n"
        "      FieldType@(3:3) b\n"
    ));
}

TEST(dolang, parser_tupleTypesErrors) {
    ClearPools clear_;
    // unclosed
    EXPECT(fails("global x : (a : num\n"));
    EXPECT(fails("global x : (\n  a : num\n"));
    // field names must be identifiers, types must be present after :
    EXPECT(fails("global x : (1)\n"));
    EXPECT(fails("global x : (num)\n"));
    EXPECT(fails("global x : (a : )\n"));
    EXPECT(fails("global x : (a : 1)\n"));
    // missing or doubled separators
    EXPECT(fails("global x : (a b)\n"));
    EXPECT(fails("global x : (a : num b : num)\n"));
    EXPECT(fails("global x : (a,, b)\n"));
    EXPECT(fails("global x : (, a)\n"));
    EXPECT(fails("global x : (\n  a b\n)\n"));
    // multi-line must close on its own line, and cannot be inside a single line list
    EXPECT(fails("global x : (\n  a\n  b)\n"));
    EXPECT(fails("global x : (a : (\n    b\n  ))\n"));
    // mismatched brackets
    EXPECT(fails("global x : (a]\n"));
}

TEST(dolang, parser_mapTypes) {
    ClearPools clear_;
    EXPECT(roundTrips("global m : {num -> text}\n"));
    EXPECT(roundTrips("global m : {Player -> num}\n"));
    EXPECT(roundTrips("global m : {text -> [num]}\n"));
    EXPECT(roundTrips("global m : {num -> {text -> bool}}\n"));
    EXPECT(roundTrips("global m : [{num -> text}]\n"));
}


// basic function declaration

TEST(dolang, parser_functionDefinition) {
    ClearPools clear_;
    EXPECT(roundTrips("fun f()\n  break\n"));
    EXPECT(roundTrips("fun f() -> num\n  break\n"));
    EXPECT(roundTrips("fun f() -> optional num\n  break\n"));
}

TEST(dolang, parser_twoFunctions) {
    ClearPools clear_;
    EXPECT(roundTrips("fun f()\n  break\n  continue\nfun g() -> num\n  break\n  continue\n"));
}

TEST(dolang, parser_functionArgs) {
    ClearPools clear_;
    EXPECT(roundTrips("fun f(a : num)\n  break\n"));
    EXPECT(roundTrips("fun f(a : num, b : [text]) -> num\n  break\n"));
    EXPECT(roundTrips("fun f(a : num, b : num = c)\n  break\n"));
    EXPECT(printsAs("fun f(a : num, b : num = c,)\n  break\n", "fun f(a : num, b : num = c)\n  break\n"));
    EXPECT(roundTrips("fun Complex.add(other : Complex)\n  break\n"));
    EXPECT(roundTrips("fun f(\n  a : num\n  b : num = c\n)\n  break\n"));
}

TEST(dolang, parser_functionArgsStructure) {
    ClearPools clear_;
    EXPECT(debugPrints("fun f(a : num = b) -> optional num\n  break\n",
        "Program@(1:1)\n"
        "  NamedFunction@(1:1) f optional\n"
        "    List@(1:6)\n"
        "      Arg@(1:7) a\n"
        "        SymbolName@(1:11) num\n"
        "        SymbolName@(1:17) b\n"
        "    SymbolName@(1:32) num\n"
        "    Block@(2:3)\n"
        "      Break@(2:3)\n"
    ));
}

TEST(dolang, parser_functionArgsErrors) {
    ClearPools clear_;
    // named function arguments must have types
    EXPECT(fails("fun f(a)\n  break\n"));
    EXPECT(fails("fun f(a = 1)\n  break\n"));
    // and cannot have patterns
    EXPECT(fails("fun f(a : num is 1)\n  break\n"));
    // named functions must have the arguments list
    EXPECT(fails("fun f\n  break\n"));
}

TEST(dolang, parser_functionTypes) {
    ClearPools clear_;
    EXPECT(roundTrips("global f : fun()\n"));
    EXPECT(roundTrips("global f : fun(a : num) -> num\n"));
    EXPECT(roundTrips("global f : fun(a : num, b : num = 1) -> optional text\n"));
    EXPECT(roundTrips("global f : fun(a : fun(b : num) -> num)\n"));
    // arguments must have types
    EXPECT(fails("global f : fun(a)\n"));
    EXPECT(fails("global f : fun(a = 1)\n"));
    EXPECT(fails("global f : fun\n"));
}

TEST(dolang, parser_eventTypes) {
    ClearPools clear_;
    EXPECT(roundTrips("global e : event()\n"));
    EXPECT(roundTrips("global e : event(a : num, b : num = 1)\n"));
    // events do not return values and arguments must have types
    EXPECT(fails("global e : event(a : num) -> num\n"));
    EXPECT(fails("global e : event(a)\n"));
    EXPECT(fails("global e : event\n"));
}

TEST(dolang, parser_functionLiterals) {
    ClearPools clear_;
    // implicit signature
    EXPECT(roundTrips("global f = fun\n  break\n"));
    // explicit signature, types are optional
    EXPECT(roundTrips("global f = fun()\n  break\n"));
    EXPECT(roundTrips("global f = fun(a)\n  break\n"));
    EXPECT(roundTrips("global f = fun(a : num, b) -> num\n  break\n"));
    // event handler patterns
    EXPECT(roundTrips("global f = fun(a is A, b)\n  break\n"));
    EXPECT(roundTrips("global f = fun(a : Btn is A)\n  break\n"));
    // inside function bodies
    EXPECT(roundTrips("fun f()\n  let g : fun(a : num) = fun\n    break\n"));
    EXPECT(roundTrips("fun f()\n  g = fun(a)\n    break\n"));
}

TEST(dolang, parser_functionLiteralsStructure) {
    ClearPools clear_;
    // implicit signature has no arguments list, while the empty signature has an empty one
    EXPECT(debugPrints("global f = fun\n  break\n",
        "Program@(1:1)\n"
        "  VarDecl@(1:1) global f\n"
        "    Function@(1:12)\n"
        "      Block@(2:3)\n"
        "        Break@(2:3)\n"
    ));
    EXPECT(debugPrints("global f = fun()\n  break\n",
        "Program@(1:1)\n"
        "  VarDecl@(1:1) global f\n"
        "    Function@(1:12)\n"
        "      List@(1:15)\n"
        "      Block@(2:3)\n"
        "        Break@(2:3)\n"
    ));
    EXPECT(debugPrints("global f = fun(a is A)\n  break\n",
        "Program@(1:1)\n"
        "  VarDecl@(1:1) global f\n"
        "    Function@(1:12)\n"
        "      List@(1:15)\n"
        "        Arg@(1:16) a\n"
        "          SymbolName@(1:21) A\n"
        "      Block@(2:3)\n"
        "        Break@(2:3)\n"
    ));
}

TEST(dolang, parser_topLevelAssignments) {
    ClearPools clear_;
    EXPECT(roundTrips("onPress = fun\n  break\n"));
    EXPECT(roundTrips("onPress += fun(btn is A)\n  break\n"));
    EXPECT(roundTrips("global x : num\nonPress += fun\n  break\nfun f()\n  break\n"));
    // any expression statement parses at the top level, only event and function assignments are valid, but that is a semantic check
    EXPECT(roundTrips("12\n"));
    EXPECT(roundTrips("a -= b + c\n"));
}

TEST(dolang, parser_functionLiteralsErrors) {
    ClearPools clear_;
    // function literals cannot have default values
    EXPECT(fails("global f = fun(a = 1)\n  break\n"));
    EXPECT(fails("global f = fun(a : num = 1)\n  break\n"));
    // body is mandatory
    EXPECT(fails("global f = fun(a)\n"));
    EXPECT(fails("global f = fun\n"));
    // and is not allowed in single line lists
    EXPECT(fails("global f = [fun\n  break\n]\n"));
}

TEST(dolang, parser_typeDecl) {
    ClearPools clear_;
    EXPECT(roundTrips("type Point = ()\n"));
    EXPECT(roundTrips("type Point = (x : num, y : num)\n"));
    // types are optional when there is a default value, and defaults are optional
    EXPECT(roundTrips("type Point = (x = 0, y : num = 0, z : num)\n"));
    // any type can be used for the fields
    EXPECT(roundTrips("type T = (a : [num], b : {text -> Point}, c : (x : num), d : fun(a : num) -> num, e : event(x : num))\n"));
    // extends
    EXPECT(roundTrips("type Point3D extends Point = (z : num)\n"));
    // trailing comma is dropped
    EXPECT(printsAs("type Point = (x : num, y : num,)\n", "type Point = (x : num, y : num)\n"));
    // among other declarations
    EXPECT(roundTrips("type A = (x : num)\ntype B extends A = (y : num)\nglobal b : B\n"));
    EXPECT(roundTrips("fun f()\n  break\ntype A = (x : num)\n"));
}

TEST(dolang, parser_typeDeclMultiLine) {
    ClearPools clear_;
    EXPECT(roundTrips("type Point = (\n  x : num\n  y : num\n)\n"));
    EXPECT(printsAs("type Point = (\n  x : num, y : num\n  z : num,\n)\n", "type Point = (\n  x : num\n  y : num\n  z : num\n)\n"));
    // the constructor body is a block, so it needs the multi-line form
    EXPECT(roundTrips("type Point = (\n  x : num\n  constructor = fun(a : num)\n    x = a\n)\n"));
    EXPECT(roundTrips("type Point = (\n  constructor = fun\n    break\n  x : num\n)\n"));
}

TEST(dolang, parser_typeDeclStructure) {
    ClearPools clear_;
    EXPECT(debugPrints("type B extends A = (x : num, y = 1)\n",
        "Program@(1:1)\n"
        "  TypeDecl@(1:1) B extends A\n"
        "    List@(1:20)\n"
        "      Arg@(1:21) x\n"
        "        SymbolName@(1:25) num\n"
        "      Arg@(1:30) y\n"
        "        Number@(1:34) 1\n"
    ));
    // the constructor is an argument named constructor with the function as its value
    EXPECT(debugPrints("type A = (\n  constructor = fun\n    break\n)\n",
        "Program@(1:1)\n"
        "  TypeDecl@(1:1) A\n"
        "    List@(1:10) multi-line\n"
        "      Arg@(2:3) constructor\n"
        "        Function@(2:17)\n"
        "          Block@(3:5)\n"
        "            Break@(3:5)\n"
    ));
}

TEST(dolang, parser_typeDeclErrors) {
    ClearPools clear_;
    // missing parts
    EXPECT(fails("type"));
    EXPECT(fails("type = (x : num)\n"));
    EXPECT(fails("type A (x : num)\n"));
    EXPECT(fails("type A =\n"));
    EXPECT(fails("type A extends = (x : num)\n"));
    // fields must be in parentheses
    EXPECT(fails("type A = x : num\n"));
    EXPECT(fails("type A = [x : num]\n"));
    EXPECT(fails("type A = (x : num\n"));
    // fields must be named, with valid types
    EXPECT(fails("type A = (num)\n"));
    EXPECT(fails("type A = (x : 12)\n"));
    EXPECT(fails("type A = (x :)\n"));
    // only a single base type
    EXPECT(fails("type A extends B, C = (x : num)\n"));
    // the constructor must be a function literal with a body
    EXPECT(fails("type A = (constructor = 1)\n"));
    EXPECT(fails("type A = (constructor fun\n  break\n)\n"));
    EXPECT(fails("type A = (constructor = fun\n  break\n)\n"));
    // types are only allowed at the top level
    EXPECT(fails("fun f()\n  type A = (x : num)\n"));
}

TEST(dolang, parser_enumDecl) {
    ClearPools clear_;
    EXPECT(roundTrips("enum Color = Red\n"));
    EXPECT(roundTrips("enum Color = Red | Green | Blue\n"));
    // among other declarations
    EXPECT(roundTrips("enum A = X | Y\nenum B = Z\nglobal a = X\n"));
    EXPECT(roundTrips("fun f()\n  break\nenum A = X | Y\n"));
}

TEST(dolang, parser_enumDeclMultiline) {
    ClearPools clear_;
    // a line break after = (or anywhere else) is a continuation
    EXPECT(debugPrints("enum Btn =\n    Up | Down\n",
        "Program@(1:1)\n"
        "  EnumDecl@(1:1) Btn\n"
        "    Up\n"
        "    Down\n"
    ));
    EXPECT(debugPrints("enum Btn = Up\n    | Down\n",
        "Program@(1:1)\n"
        "  EnumDecl@(1:1) Btn\n"
        "    Up\n"
        "    Down\n"
    ));
}

TEST(dolang, parser_enumStructure) {
    ClearPools clear_;
    EXPECT(debugPrints("enum Color = Red | Green | Blue\n",
        "Program@(1:1)\n"
        "  EnumDecl@(1:1) Color\n"
        "    Red\n"
        "    Green\n"
        "    Blue\n"
    ));
}

TEST(dolang, parser_enumErrors) {
    ClearPools clear_;
    // missing name, =, or values
    EXPECT(fails("enum"));
    EXPECT(fails("enum Color"));
    EXPECT(fails("enum = Red"));
    EXPECT(fails("enum Color Red"));
    EXPECT(fails("enum Color ="));
    // dangling, doubled or missing separators
    EXPECT(fails("enum Color = Red |"));
    EXPECT(fails("enum Color = | Red"));
    EXPECT(fails("enum Color = Red || Green"));
    EXPECT(fails("enum Color = Red Green"));
    EXPECT(fails("enum Color = Red, Green"));
    // names must be identifiers
    EXPECT(fails("enum 1 = Red"));
    EXPECT(fails("enum Color = 1"));
    EXPECT(fails("enum Color = Red | if"));
    // enums are top level declarations only
    EXPECT(fails("fun f()\n  enum Color = Red\n"));
}

// ------------------------------------------------------------------------------------------------
// statements
// ------------------------------------------------------------------------------------------------

TEST(dolang, parser_globalWithValue) {
    ClearPools clear_;
    EXPECT(roundTrips("global x = a\n"));
    EXPECT(roundTrips("global x : bool = true\n"));
    EXPECT(roundTrips("const x = false\n"));
    EXPECT(roundTrips("global x = a + b\n"));
}

TEST(dolang, parser_letStatement) {
    ClearPools clear_;
    EXPECT(roundTrips("fun f()\n  let x\n"));
    EXPECT(roundTrips("fun f()\n  let x : num\n"));
    EXPECT(roundTrips("fun f()\n  let x = a\n"));
    EXPECT(roundTrips("fun f()\n  let x : bool = a and b\n"));
    EXPECT(roundTrips("fun f()\n  const x = true\n"));
}

TEST(dolang, parser_returnStatement) {
    ClearPools clear_;
    EXPECT(roundTrips("fun f()\n  return\n"));
    EXPECT(roundTrips("fun f()\n  return a\n"));
    EXPECT(roundTrips("fun f()\n  return a + b\n"));
    // return without value followed by another statement, and as the last statement before another function
    EXPECT(roundTrips("fun f()\n  return\n  break\n"));
    EXPECT(roundTrips("fun f()\n  return\nfun g()\n  return\n"));
}

TEST(dolang, parser_writeAndTest) {
    ClearPools clear_;
    EXPECT(roundTrips("fun f()\n  write a\n"));
    EXPECT(roundTrips("fun f()\n  test a\n"));
    EXPECT(roundTrips("fun f()\n  test a == b\n"));
}

TEST(dolang, parser_expressionStatement) {
    ClearPools clear_;
    EXPECT(roundTrips("fun f()\n  a\n"));
    EXPECT(roundTrips("fun f()\n  a + b\n"));
    EXPECT(roundTrips("fun f()\n  a\n  b\n  c\n"));
}

TEST(dolang, parser_assignments) {
    ClearPools clear_;
    EXPECT(roundTrips("fun f()\n  a = b\n"));
    EXPECT(roundTrips("fun f()\n  a += b\n"));
    EXPECT(roundTrips("fun f()\n  a -= b\n"));
    EXPECT(roundTrips("fun f()\n  a *= b\n"));
    EXPECT(roundTrips("fun f()\n  a /= b\n"));
    EXPECT(roundTrips("fun f()\n  a = b + c\n"));
    EXPECT(roundTrips("fun f()\n  a = b\n  c = d\n"));
}

// ------------------------------------------------------------------------------------------------
// expressions
// ------------------------------------------------------------------------------------------------

TEST(dolang, parser_keywordPrimaries) {
    ClearPools clear_;
    EXPECT(roundTrips("fun f()\n  let a = true\n"));
    EXPECT(roundTrips("fun f()\n  let a = false\n"));
    EXPECT(roundTrips("fun f()\n  let a = none\n"));
    EXPECT(roundTrips("fun f()\n  let a = self\n"));
}

TEST(dolang, parser_arithmeticOperators) {
    ClearPools clear_;
    EXPECT(roundTrips("fun f()\n  a + b\n"));
    EXPECT(roundTrips("fun f()\n  a - b\n"));
    EXPECT(roundTrips("fun f()\n  a * b\n"));
    EXPECT(roundTrips("fun f()\n  a / b\n"));
    EXPECT(roundTrips("fun f()\n  a + b - c\n"));
    EXPECT(roundTrips("fun f()\n  a * b / c\n"));
    EXPECT(roundTrips("fun f()\n  a + b * c - d / e\n"));
}

TEST(dolang, parser_comparisonOperators) {
    ClearPools clear_;
    EXPECT(roundTrips("fun f()\n  a < b\n"));
    EXPECT(roundTrips("fun f()\n  a <= b\n"));
    EXPECT(roundTrips("fun f()\n  a > b\n"));
    EXPECT(roundTrips("fun f()\n  a >= b\n"));
    // comparisons can be chained
    EXPECT(roundTrips("fun f()\n  a < b <= c\n"));
    EXPECT(roundTrips("fun f()\n  a + b < c * d\n"));
}

TEST(dolang, parser_equalityOperators) {
    ClearPools clear_;
    EXPECT(roundTrips("fun f()\n  a == b\n"));
    EXPECT(roundTrips("fun f()\n  a != b\n"));
    // equality can be chained, but not mixed
    EXPECT(roundTrips("fun f()\n  a == b == c\n"));
    EXPECT(roundTrips("fun f()\n  a != b != c\n"));
    EXPECT(roundTrips("fun f()\n  a < b == c > d\n"));
}

TEST(dolang, parser_logicalOperators) {
    ClearPools clear_;
    EXPECT(roundTrips("fun f()\n  a and b\n"));
    EXPECT(roundTrips("fun f()\n  a or b\n"));
    EXPECT(roundTrips("fun f()\n  not a\n"));
    EXPECT(roundTrips("fun f()\n  a and b and c\n"));
    EXPECT(roundTrips("fun f()\n  a or b or c\n"));
    EXPECT(roundTrips("fun f()\n  a or b and c\n"));
    EXPECT(roundTrips("fun f()\n  not a and b\n"));
    EXPECT(roundTrips("fun f()\n  not not a\n"));
    EXPECT(roundTrips("fun f()\n  not a == b\n"));
}

TEST(dolang, parser_unaryOperators) {
    ClearPools clear_;
    EXPECT(roundTrips("fun f()\n  -a\n"));
    EXPECT(roundTrips("fun f()\n  clone a\n"));
    EXPECT(roundTrips("fun f()\n  a = -b\n"));
    EXPECT(roundTrips("fun f()\n  a = clone b\n"));
    EXPECT(roundTrips("fun f()\n  -a * b\n"));
    EXPECT(roundTrips("fun f()\n  a - -b\n"));
}

TEST(dolang, parser_mixedPrecedence) {
    ClearPools clear_;
    EXPECT(roundTrips("fun f()\n  a or b and not c == d < e + f * -g\n"));
}

TEST(dolan, parser_ifStatement) {
    ClearPools clear_;
    EXPECT(roundTrips("fun f()\n  if a then\n    b\n    bb\n  c\n"));
    EXPECT(roundTrips("fun f()\n  if a then\n    b\n  else\n    c\n"));
}

TEST(dolang, parser_whileStatement) {
    ClearPools clear_;
    EXPECT(roundTrips("fun f()\n  while a do\n    b\n"));
    EXPECT(roundTrips("fun f()\n  while a do\n    b\n    c\n"));
    EXPECT(roundTrips("fun f()\n  while a < b and not c do\n    b\n"));
    // statement after the loop
    EXPECT(roundTrips("fun f()\n  while a do\n    b\n  c\n"));
    // nested loops
    EXPECT(roundTrips("fun f()\n  while a do\n    while b do\n      c\n    d\n"));
    // break and continue in the body
    EXPECT(roundTrips("fun f()\n  while a do\n    break\n    continue\n"));
}

TEST(dolang, parser_forStatement) {
    ClearPools clear_;
    EXPECT(roundTrips("fun f()\n  for x in a do\n    b\n"));
    EXPECT(roundTrips("fun f()\n  for k, v in a do\n    b\n"));
    EXPECT(roundTrips("fun f()\n  for x in a step b do\n    c\n"));
    EXPECT(roundTrips("fun f()\n  for k, v in a step b do\n    c\n"));
    // range form is an ordinary expression
    EXPECT(roundTrips("fun f()\n  for i in 1 <= i < 10 step 2 do\n    b\n"));
    // statement after the loop, nested loops
    EXPECT(roundTrips("fun f()\n  for x in a do\n    b\n  c\n"));
    EXPECT(roundTrips("fun f()\n  for x in a do\n    for y in x do\n      b\n    c\n"));
}

TEST(dolang, parser_whileStructure) {
    ClearPools clear_;
    EXPECT(debugPrints("fun f()\n  while a do\n    b\n",
        "Program@(1:1)\n"
        "  NamedFunction@(1:1) f\n"
        "    List@(1:6)\n"
        "    Block@(2:3)\n"
        "      While@(2:3)\n"
        "        SymbolName@(2:9) a\n"
        "        Block@(3:5)\n"
        "          SymbolName@(3:5) b\n"
    ));
}

TEST(dolang, parser_forStructure) {
    ClearPools clear_;
    EXPECT(debugPrints("fun f()\n  for x in a do\n    b\n",
        "Program@(1:1)\n"
        "  NamedFunction@(1:1) f\n"
        "    List@(1:6)\n"
        "    Block@(2:3)\n"
        "      For@(2:3) x\n"
        "        SymbolName@(2:12) a\n"
        "        Block@(3:5)\n"
        "          SymbolName@(3:5) b\n"
    ));
    EXPECT(debugPrints("fun f()\n  for k, v in a step b do\n    c\n",
        "Program@(1:1)\n"
        "  NamedFunction@(1:1) f\n"
        "    List@(1:6)\n"
        "    Block@(2:3)\n"
        "      For@(2:3) k, v\n"
        "        SymbolName@(2:15) a\n"
        "        SymbolName@(2:22) b\n"
        "        Block@(3:5)\n"
        "          SymbolName@(3:5) c\n"
    ));
}

TEST(dolang, parser_loopsMultilineHeader) {
    ClearPools clear_;
    // a line break in the header is a continuation, as the block only starts after do
    auto w = parse("fun f()\n  while a and\n      b do\n    c\n");
    EXPECT(w->kind != ast::Kind::Error);
    auto f = parse("fun f()\n  for x in a\n      step b do\n    c\n");
    EXPECT(f->kind != ast::Kind::Error);
}

TEST(dolang, parser_loopErrors) {
    ClearPools clear_;
    // missing do, condition, or body
    EXPECT(fails("fun f()\n  while a\n    b\n"));
    EXPECT(fails("fun f()\n  while do\n    b\n"));
    EXPECT(fails("fun f()\n  while a do\n  b\n"));
    // missing or invalid induction variables
    EXPECT(fails("fun f()\n  for in a do\n    b\n"));
    EXPECT(fails("fun f()\n  for 1 in a do\n    b\n"));
    EXPECT(fails("fun f()\n  for k, in a do\n    b\n"));
    EXPECT(fails("fun f()\n  for a, b, c in d do\n    e\n"));
    // missing in, container, step value, or do
    EXPECT(fails("fun f()\n  for x a do\n    b\n"));
    EXPECT(fails("fun f()\n  for x in do\n    b\n"));
    EXPECT(fails("fun f()\n  for x in a step do\n    b\n"));
    EXPECT(fails("fun f()\n  for x in a\n    b\n"));
}

TEST(dolang, parser_matchStatement) {
    ClearPools clear_;
    EXPECT(roundTrips("fun f()\n  match a when\n    is X then\n      b\n"));
    EXPECT(roundTrips("fun f()\n  match a when\n    is X then\n      b\n      c\n"));
    EXPECT(roundTrips("fun f()\n  match a when\n    is X then\n      b\n    is Y then\n      c\n"));
    EXPECT(roundTrips("fun f()\n  match a when\n    is X then\n      b\n    else\n      c\n"));
    EXPECT(roundTrips("fun f()\n  match a when\n    is X then\n      b\n    is Y then\n      c\n    else\n      d\n"));
    // matched value is any expression
    EXPECT(roundTrips("fun f()\n  match a + b when\n    is X then\n      c\n"));
    // statement after the match
    EXPECT(roundTrips("fun f()\n  match a when\n    is X then\n      b\n  c\n"));
    EXPECT(roundTrips("fun f()\n  match a when\n    is X then\n      b\n    else\n      c\n  d\n"));
    // only the else case
    EXPECT(roundTrips("fun f()\n  match a when\n    else\n      b\n"));
    // nested match
    EXPECT(roundTrips("fun f()\n  match a when\n    is X then\n      match b when\n        is Y then\n          c\n    is Z then\n      d\n"));
}

TEST(dolang, parser_matchStructure) {
    ClearPools clear_;
    EXPECT(debugPrints("fun f()\n  match a when\n    is X then\n      b\n    else\n      c\n",
        "Program@(1:1)\n"
        "  NamedFunction@(1:1) f\n"
        "    List@(1:6)\n"
        "    Block@(2:3)\n"
        "      Match@(2:3)\n"
        "        SymbolName@(2:9) a\n"
        "        MatchCase@(3:5)\n"
        "          SymbolName@(3:8) X\n"
        "          Block@(4:7)\n"
        "            SymbolName@(4:7) b\n"
        "        Block@(6:7)\n"
        "          SymbolName@(6:7) c\n"
    ));
}

TEST(dolang, parser_matchErrors) {
    ClearPools clear_;
    // missing when, is, then, or case body
    EXPECT(fails("fun f()\n  match a\n    is X then\n      b\n"));
    EXPECT(fails("fun f()\n  match a when\n    X then\n      b\n"));
    EXPECT(fails("fun f()\n  match a when\n    is X\n      b\n"));
    EXPECT(fails("fun f()\n  match a when\n    is X then\n    is Y then\n      b\n"));
    // cases must be on their own lines in a block
    EXPECT(fails("fun f()\n  match a when is X then\n    b\n"));
    // at least one case (which can be the else case) is required
    EXPECT(fails("fun f()\n  match a when\n  b\n"));
    EXPECT(fails("fun f()\n  match a when\n"));
    // else is the last case, only once, and at the indentation of the cases
    EXPECT(fails("fun f()\n  match a when\n    else\n      b\n    is X then\n      c\n"));
    EXPECT(fails("fun f()\n  match a when\n    is X then\n      b\n    else\n      c\n    else\n      d\n"));
    EXPECT(fails("fun f()\n  match a when\n    is X then\n      b\n  else\n    c\n"));
    // patterns are identifiers for now
    EXPECT(fails("fun f()\n  match a when\n    is 1 then\n      b\n"));
}

TEST(dolang, parser_matchExpression) {
    ClearPools clear_;
    EXPECT(roundTrips("global x = match a when is X then b\n"));
    EXPECT(roundTrips("global x = match a when is X then b is Y then c\n"));
    EXPECT(roundTrips("global x = match a when is X then b else c\n"));
    EXPECT(roundTrips("global x = match a when else c\n"));
    EXPECT(roundTrips("fun f()\n  let x = match a when is X then b is Y then c else d\n"));
    EXPECT(roundTrips("fun f()\n  return match a when is X then b else c\n"));
    // case values are expressions
    EXPECT(roundTrips("global x = match a when is X then b + 1 is Y then c * 2 else -d\n"));
}

TEST(dolang, parser_matchExpressionStructure) {
    ClearPools clear_;
    EXPECT(debugPrints("global x = match a when is X then b else c\n",
        "Program@(1:1)\n"
        "  VarDecl@(1:1) global x\n"
        "    Match@(1:12)\n"
        "      SymbolName@(1:18) a\n"
        "      MatchCase@(1:25)\n"
        "        SymbolName@(1:28) X\n"
        "        SymbolName@(1:35) b\n"
        "      SymbolName@(1:42) c\n"
    ));
    // is always starts the next case, it does not continue the case value
    EXPECT(debugPrints("global x = match a when is X then b is Y then c\n",
        "Program@(1:1)\n"
        "  VarDecl@(1:1) global x\n"
        "    Match@(1:12)\n"
        "      SymbolName@(1:18) a\n"
        "      MatchCase@(1:25)\n"
        "        SymbolName@(1:28) X\n"
        "        SymbolName@(1:35) b\n"
        "      MatchCase@(1:37)\n"
        "        SymbolName@(1:40) Y\n"
        "        SymbolName@(1:47) c\n"
    ));
}

TEST(dolang, parser_matchExpressionMultiline) {
    ClearPools clear_;
    // line breaks inside the match expression are continuations
    auto a = parse("fun f()\n  let x = match a when\n    is X then b\n    is Y then c\n    else d\n");
    EXPECT(a->kind != ast::Kind::Error);
    auto b = parse("fun f()\n  let x = match a when is X then\n      b\n    else\n      c\n");
    EXPECT(b->kind != ast::Kind::Error);
}

TEST(dolang, parser_matchExpressionErrors) {
    ClearPools clear_;
    // missing when, then, case value, or else value
    EXPECT(fails("global x = match a is X then b\n"));
    EXPECT(fails("global x = match a when is X b\n"));
    EXPECT(fails("global x = match a when is X then\n"));
    EXPECT(fails("global x = match a when is X then b else\n"));
    // at least one case (which can be the else case) is required
    EXPECT(fails("global x = match a when\n"));
    // else is the last case
    EXPECT(fails("global x = match a when else b is X then c\n"));
}

TEST(dolang, parser_isOperator) {
    ClearPools clear_;
    EXPECT(roundTrips("global x = a is B\n"));
    EXPECT(roundTrips("global x = a is not B\n"));
    EXPECT(roundTrips("fun f()\n  if a is B then\n    c\n"));
    EXPECT(roundTrips("fun f()\n  while a is not B do\n    c\n"));
}

TEST(dolang, parser_isOperatorStructure) {
    ClearPools clear_;
    // is has the lowest precedence
    EXPECT(debugPrints("global x = a + b is C\n",
        "Program@(1:1)\n"
        "  VarDecl@(1:1) global x\n"
        "    Is@(1:18)\n"
        "      BinaryOp@(1:14) +\n"
        "        SymbolName@(1:12) a\n"
        "        SymbolName@(1:16) b\n"
        "      SymbolName@(1:21) C\n"
    ));
    // even lower than not, i.e. not a is B is (not a) is B
    EXPECT(debugPrints("global x = not a is B\n",
        "Program@(1:1)\n"
        "  VarDecl@(1:1) global x\n"
        "    Is@(1:18)\n"
        "      UnaryOp@(1:12) not\n"
        "        SymbolName@(1:16) a\n"
        "      SymbolName@(1:21) B\n"
    ));
    EXPECT(debugPrints("global x = a is not B\n",
        "Program@(1:1)\n"
        "  VarDecl@(1:1) global x\n"
        "    Is@(1:14) not\n"
        "      SymbolName@(1:12) a\n"
        "      SymbolName@(1:21) B\n"
    ));
}

TEST(dolang, parser_isOperatorErrors) {
    ClearPools clear_;
    // missing pattern, is is not associative, and cannot be combined without parentheses
    EXPECT(fails("global x = a is\n"));
    EXPECT(fails("global x = a is not\n"));
    EXPECT(fails("global x = a is B is C\n"));
    EXPECT(fails("global x = a is B and c\n"));
    EXPECT(fails("global x = a is B + c\n"));
    EXPECT(fails("global x = is B\n"));
}

// ------------------------------------------------------------------------------------------------
// lists (via array literals)
// ------------------------------------------------------------------------------------------------

TEST(dolang, parser_listSingleLine) {
    ClearPools clear_;
    EXPECT(roundTrips("global x = []\n"));
    EXPECT(roundTrips("global x = [a]\n"));
    EXPECT(roundTrips("global x = [a, b, c]\n"));
    EXPECT(roundTrips("global x = [a + b, -c, [d, e]]\n"));
    EXPECT(roundTrips("fun f()\n  let x = [a, b]\n  c\n"));
    // trailing comma
    EXPECT(printsAs("global x = [a, b,]\n", "global x = [a, b]\n"));
    // line breaks are continuations
    EXPECT(printsAs("global x = [a,\n    b]\n", "global x = [a, b]\n"));
    EXPECT(printsAs("global x = [a\n    , b]\n", "global x = [a, b]\n"));
    EXPECT(printsAs("global x = [a, b +\n    c]\n", "global x = [a, b + c]\n"));
    // closing bracket on its own line at the statement indentation, or as a continuation
    EXPECT(printsAs("global x = [a,\n    b\n]\n", "global x = [a, b]\n"));
    EXPECT(printsAs("global x = [a,\n    b,\n]\n", "global x = [a, b]\n"));
    EXPECT(printsAs("fun f()\n  let x = [a,\n      b\n  ]\n", "fun f()\n  let x = [a, b]\n"));
    EXPECT(printsAs("global x = [a,\n    b\n  ]\n", "global x = [a, b]\n"));
}

TEST(dolang, parser_listMultiLine) {
    ClearPools clear_;
    EXPECT(roundTrips("global x = [\n  a\n]\n"));
    EXPECT(roundTrips("global x = [\n  a\n  b\n  c\n]\n"));
    EXPECT(roundTrips("fun f()\n  let x = [\n    a\n    b\n  ]\n"));
    // statement after the list
    EXPECT(roundTrips("fun f()\n  let x = [\n    a\n  ]\n  b\n"));
    // nested lists
    EXPECT(roundTrips("global x = [\n  [\n    a\n    b\n  ]\n  c\n]\n"));
    EXPECT(roundTrips("global x = [\n  [a, b]\n  c\n]\n"));
    // a line at the item column is always a new item
    EXPECT(roundTrips("global x = [\n  a\n  -b\n]\n"));
    // while a line indented more is a continuation
    EXPECT(printsAs("global x = [\n  a\n    + b\n  c\n]\n", "global x = [\n  a + b\n  c\n]\n"));
    EXPECT(printsAs("global x = [\n  a,\n    b\n]\n", "global x = [\n  a\n  b\n]\n"));
}

TEST(dolang, parser_listMultiLineCommas) {
    ClearPools clear_;
    // items on the same line are separated by commas
    EXPECT(printsAs("global x = [\n  a, b\n  c\n]\n", "global x = [\n  a\n  b\n  c\n]\n"));
    EXPECT(printsAs("global x = [\n  a, b, c\n  d, e\n]\n", "global x = [\n  a\n  b\n  c\n  d\n  e\n]\n"));
    // comma at the end of the line is optional, including after the last item
    EXPECT(printsAs("global x = [\n  a,\n  b,\n]\n", "global x = [\n  a\n  b\n]\n"));
    EXPECT(printsAs("global x = [\n  a,\n  b\n]\n", "global x = [\n  a\n  b\n]\n"));
    EXPECT(printsAs("global x = [\n  a, b,\n  c\n]\n", "global x = [\n  a\n  b\n  c\n]\n"));
}

TEST(dolang, parser_listEmpty) {
    ClearPools clear_;
    EXPECT(printsAs("global x = [\n]\n", "global x = []\n"));
    EXPECT(printsAs("fun f()\n  let x = [\n  ]\n", "fun f()\n  let x = []\n"));
}

TEST(dolang, parser_listStructure) {
    ClearPools clear_;
    EXPECT(debugPrints("global x = [a, b]\n",
        "Program@(1:1)\n"
        "  VarDecl@(1:1) global x\n"
        "    ArrayLiteral@(1:12)\n"
        "      SymbolName@(1:13) a\n"
        "      SymbolName@(1:16) b\n"
    ));
    EXPECT(debugPrints("global x = [\n  a, b\n  c\n]\n",
        "Program@(1:1)\n"
        "  VarDecl@(1:1) global x\n"
        "    ArrayLiteral@(1:12) multi-line\n"
        "      SymbolName@(2:3) a\n"
        "      SymbolName@(2:6) b\n"
        "      SymbolName@(3:3) c\n"
    ));
}

TEST(dolang, parser_listErrors) {
    ClearPools clear_;
    // missing commas
    EXPECT(fails("global x = [a b]\n"));
    EXPECT(fails("global x = [\n  a b\n]\n"));
    // missing, or doubled items
    EXPECT(fails("global x = [, a]\n"));
    EXPECT(fails("global x = [a,, b]\n"));
    EXPECT(fails("global x = [\n  a,,\n]\n"));
    EXPECT(fails("global x = [\n  , a\n]\n"));
    // unclosed lists
    EXPECT(fails("global x = [a, b\n"));
    EXPECT(fails("global x = [\n  a\n"));
    EXPECT(fails("global x = [a,\n"));
    // multi-line list must end on its own line
    EXPECT(fails("global x = [\n  a\n  b]\n"));
    // at the statement indentation, not less, or more
    EXPECT(fails("fun f()\n  let x = [\n    a\n]\n"));
    EXPECT(fails("global x = [\n  a\n    ]\n"));
    // continuation of a single line list must be indented more than the statement
    EXPECT(fails("global x = [a,\nb]\n"));
    // single line list cannot contain multi-line lists
    EXPECT(fails("global x = [a, [\n    b\n  ]]\n"));
    EXPECT(fails("global x = [a, [\n    b]]\n"));
    EXPECT(fails("global x = [[\n    b\n  ]]\n"));
}

// ------------------------------------------------------------------------------------------------
// map literals
// ------------------------------------------------------------------------------------------------

TEST(dolang, parser_mapLiteral) {
    ClearPools clear_;
    EXPECT(roundTrips("global x = {}\n"));
    EXPECT(roundTrips("global x = {a -> b}\n"));
    EXPECT(roundTrips("global x = {1 -> \"foo\", 2 -> \"bar\"}\n"));
    EXPECT(roundTrips("global x = {a + b -> c * d, -e -> not f}\n"));
    EXPECT(roundTrips("fun f()\n  let x = {a -> b}\n  c\n"));
    // nested maps and arrays
    EXPECT(roundTrips("global x = {a -> {b -> c}, d -> [e, f]}\n"));
    EXPECT(roundTrips("global x = [{a -> b}, {}]\n"));
    // trailing comma
    EXPECT(printsAs("global x = {a -> b,}\n", "global x = {a -> b}\n"));
    // line breaks are continuations, including around the arrow
    EXPECT(printsAs("global x = {a -> b,\n    c -> d}\n", "global x = {a -> b, c -> d}\n"));
    EXPECT(printsAs("global x = {a\n    -> b}\n", "global x = {a -> b}\n"));
    EXPECT(printsAs("global x = {a ->\n    b}\n", "global x = {a -> b}\n"));
    EXPECT(printsAs("global x = {a -> b,\n    c -> d\n}\n", "global x = {a -> b, c -> d}\n"));
}

TEST(dolang, parser_mapLiteralMultiLine) {
    ClearPools clear_;
    EXPECT(roundTrips("global x = {\n  a -> b\n}\n"));
    EXPECT(roundTrips("global x = {\n  1 -> \"foo\"\n  2 -> \"bar\"\n}\n"));
    EXPECT(roundTrips("fun f()\n  let x = {\n    a -> b\n    c -> d\n  }\n  e\n"));
    // nested multi-line map
    EXPECT(roundTrips("global x = {\n  a -> {\n    b -> c\n  }\n  d -> e\n}\n"));
    // commas
    EXPECT(printsAs("global x = {\n  a -> b, c -> d\n  e -> f,\n}\n", "global x = {\n  a -> b\n  c -> d\n  e -> f\n}\n"));
    // continuation of the item
    EXPECT(printsAs("global x = {\n  a ->\n    b\n  c -> d\n}\n", "global x = {\n  a -> b\n  c -> d\n}\n"));
    // empty
    EXPECT(printsAs("global x = {\n}\n", "global x = {}\n"));
}

TEST(dolang, parser_mapLiteralStructure) {
    ClearPools clear_;
    EXPECT(debugPrints("global x = {a -> b, c -> d}\n",
        "Program@(1:1)\n"
        "  VarDecl@(1:1) global x\n"
        "    MapLiteral@(1:12)\n"
        "      MapItem@(1:15)\n"
        "        SymbolName@(1:13) a\n"
        "        SymbolName@(1:18) b\n"
        "      MapItem@(1:23)\n"
        "        SymbolName@(1:21) c\n"
        "        SymbolName@(1:26) d\n"
    ));
    EXPECT(debugPrints("global x = {\n  a -> b + c\n}\n",
        "Program@(1:1)\n"
        "  VarDecl@(1:1) global x\n"
        "    MapLiteral@(1:12) multi-line\n"
        "      MapItem@(2:5)\n"
        "        SymbolName@(2:3) a\n"
        "        BinaryOp@(2:10) +\n"
        "          SymbolName@(2:8) b\n"
        "          SymbolName@(2:12) c\n"
    ));
}

TEST(dolang, parser_mapLiteralErrors) {
    ClearPools clear_;
    // missing key, value, or arrow
    EXPECT(fails("global x = {a}\n"));
    EXPECT(fails("global x = {a ->}\n"));
    EXPECT(fails("global x = {-> b}\n"));
    EXPECT(fails("global x = {a b}\n"));
    EXPECT(fails("global x = {a -> b -> c}\n"));
    // missing commas
    EXPECT(fails("global x = {a -> b c -> d}\n"));
    EXPECT(fails("global x = {\n  a -> b c -> d\n}\n"));
    // unclosed, or wrongly closed
    EXPECT(fails("global x = {a -> b\n"));
    EXPECT(fails("global x = {\n  a -> b\n"));
    EXPECT(fails("global x = {a -> b]\n"));
    // multi-line map must end on its own line
    EXPECT(fails("global x = {\n  a -> b}\n"));
    // single line map cannot contain multi-line lists
    EXPECT(fails("global x = {a -> {\n    b -> c\n  }}\n"));
}

// ------------------------------------------------------------------------------------------------
// precedence (via debug prints, as the pretty print does not show the tree structure)
// ------------------------------------------------------------------------------------------------

TEST(dolang, parser_precedenceMulOverAdd) {
    ClearPools clear_;
    EXPECT(debugPrints("global x = a + b * c\n",
        "Program@(1:1)\n"
        "  VarDecl@(1:1) global x\n"
        "    BinaryOp@(1:14) +\n"
        "      SymbolName@(1:12) a\n"
        "      BinaryOp@(1:18) *\n"
        "        SymbolName@(1:16) b\n"
        "        SymbolName@(1:20) c\n"
    ));
    EXPECT(debugPrints("global x = a * b + c\n",
        "Program@(1:1)\n"
        "  VarDecl@(1:1) global x\n"
        "    BinaryOp@(1:18) +\n"
        "      BinaryOp@(1:14) *\n"
        "        SymbolName@(1:12) a\n"
        "        SymbolName@(1:16) b\n"
        "      SymbolName@(1:20) c\n"
    ));
}

TEST(dolang, parser_precedenceLeftAssociative) {
    ClearPools clear_;
    // a - b - c is (a - b) - c
    EXPECT(debugPrints("global x = a - b - c\n",
        "Program@(1:1)\n"
        "  VarDecl@(1:1) global x\n"
        "    BinaryOp@(1:18) -\n"
        "      BinaryOp@(1:14) -\n"
        "        SymbolName@(1:12) a\n"
        "        SymbolName@(1:16) b\n"
        "      SymbolName@(1:20) c\n"
    ));
    // a / b / c is (a / b) / c
    EXPECT(debugPrints("global x = a / b / c\n",
        "Program@(1:1)\n"
        "  VarDecl@(1:1) global x\n"
        "    BinaryOp@(1:18) /\n"
        "      BinaryOp@(1:14) /\n"
        "        SymbolName@(1:12) a\n"
        "        SymbolName@(1:16) b\n"
        "      SymbolName@(1:20) c\n"
    ));
}

TEST(dolang, parser_precedenceAndOverOr) {
    ClearPools clear_;
    EXPECT(debugPrints("global x = a or b and c\n",
        "Program@(1:1)\n"
        "  VarDecl@(1:1) global x\n"
        "    BinaryOp@(1:14) or\n"
        "      SymbolName@(1:12) a\n"
        "      BinaryOp@(1:19) and\n"
        "        SymbolName@(1:17) b\n"
        "        SymbolName@(1:23) c\n"
    ));
}

TEST(dolang, parser_precedenceNot) {
    ClearPools clear_;
    // not is below comparisons, so not a == b is not (a == b)
    EXPECT(debugPrints("global x = not a == b\n",
        "Program@(1:1)\n"
        "  VarDecl@(1:1) global x\n"
        "    UnaryOp@(1:12) not\n"
        "      BinaryOp@(1:18) ==\n"
        "        SymbolName@(1:16) a\n"
        "        SymbolName@(1:21) b\n"
    ));
    // but above and, so not a and b is (not a) and b
    EXPECT(debugPrints("global x = not a and b\n",
        "Program@(1:1)\n"
        "  VarDecl@(1:1) global x\n"
        "    BinaryOp@(1:18) and\n"
        "      UnaryOp@(1:12) not\n"
        "        SymbolName@(1:16) a\n"
        "      SymbolName@(1:22) b\n"
    ));
}

TEST(dolang, parser_precedenceComparisons) {
    ClearPools clear_;
    // arithmetic binds tighter than comparison
    EXPECT(debugPrints("global x = a < b + c\n",
        "Program@(1:1)\n"
        "  VarDecl@(1:1) global x\n"
        "    BinaryOp@(1:14) <\n"
        "      SymbolName@(1:12) a\n"
        "      BinaryOp@(1:18) +\n"
        "        SymbolName@(1:16) b\n"
        "        SymbolName@(1:20) c\n"
    ));
    // comparison binds tighter than equality
    EXPECT(debugPrints("global x = a == b < c\n",
        "Program@(1:1)\n"
        "  VarDecl@(1:1) global x\n"
        "    BinaryOp@(1:14) ==\n"
        "      SymbolName@(1:12) a\n"
        "      BinaryOp@(1:19) <\n"
        "        SymbolName@(1:17) b\n"
        "        SymbolName@(1:21) c\n"
    ));
}

TEST(dolang, parser_precedenceUnary) {
    ClearPools clear_;
    // prefix operators bind tighter than binary ones
    EXPECT(debugPrints("global x = -a * b\n",
        "Program@(1:1)\n"
        "  VarDecl@(1:1) global x\n"
        "    BinaryOp@(1:15) *\n"
        "      UnaryOp@(1:12) -\n"
        "        SymbolName@(1:13) a\n"
        "      SymbolName@(1:17) b\n"
    ));
    EXPECT(debugPrints("global x = clone a + b\n",
        "Program@(1:1)\n"
        "  VarDecl@(1:1) global x\n"
        "    BinaryOp@(1:20) +\n"
        "      UnaryOp@(1:12) clone\n"
        "        SymbolName@(1:18) a\n"
        "      SymbolName@(1:22) b\n"
    ));
}

TEST(dolang, parser_precedenceAssignment) {
    ClearPools clear_;
    // assignment is the root of the statement
    EXPECT(debugPrints("fun f()\n    a = b + c\n",
        "Program@(1:1)\n"
        "  NamedFunction@(1:1) f\n"
        "    List@(1:6)\n"
        "    Block@(2:5)\n"
        "      Assign@(2:7) =\n"
        "        SymbolName@(2:5) a\n"
        "        BinaryOp@(2:11) +\n"
        "          SymbolName@(2:9) b\n"
        "          SymbolName@(2:13) c\n"
    ));
}

TEST(dolang, parser_member) {
    ClearPools clear_;
    EXPECT(roundTrips("global x = a.b\n"));
    EXPECT(roundTrips("global x = a.b.c\n"));
    EXPECT(roundTrips("global x = a.b + c.d\n"));
    EXPECT(roundTrips("fun f()\n  a.b = c\n"));
}

TEST(dolang, parser_index) {
    ClearPools clear_;
    EXPECT(roundTrips("global x = a[1]\n"));
    EXPECT(roundTrips("global x = a[1][2]\n"));
    EXPECT(roundTrips("global x = a[i + 1]\n"));
    EXPECT(roundTrips("global x = a[b[c]]\n"));
    EXPECT(roundTrips("global x = [1, 2][0]\n"));
    EXPECT(roundTrips("fun f()\n  a[i] = c\n"));
    // a list on a new line at the item column is a new item, not an index
    EXPECT(roundTrips("global x = [\n  a\n  [b]\n]\n"));
}

TEST(dolang, parser_memberAndIndex) {
    ClearPools clear_;
    EXPECT(roundTrips("global x = a.b[1].c\n"));
    EXPECT(roundTrips("global x = a[1].b[2]\n"));
}

TEST(dolang, parser_memberStructure) {
    ClearPools clear_;
    // member access is left associative
    EXPECT(debugPrints("global x = a.b.c\n",
        "Program@(1:1)\n"
        "  VarDecl@(1:1) global x\n"
        "    Member@(1:15)\n"
        "      Member@(1:13)\n"
        "        SymbolName@(1:12) a\n"
        "        member: b\n"
        "      member: c\n"
    ));
}

TEST(dolang, parser_indexStructure) {
    ClearPools clear_;
    // indexing is left associative
    EXPECT(debugPrints("global x = a[1][2]\n",
        "Program@(1:1)\n"
        "  VarDecl@(1:1) global x\n"
        "    BinaryOp@(1:16) [\n"
        "      BinaryOp@(1:13) [\n"
        "        SymbolName@(1:12) a\n"
        "        Number@(1:14) 1\n"
        "      Number@(1:17) 2\n"
    ));
}

TEST(dolang, parser_precedencePostfix) {
    ClearPools clear_;
    // postfix operators bind tighter than prefix ones
    EXPECT(debugPrints("global x = -a.b\n",
        "Program@(1:1)\n"
        "  VarDecl@(1:1) global x\n"
        "    UnaryOp@(1:12) -\n"
        "      Member@(1:14)\n"
        "        SymbolName@(1:13) a\n"
        "        member: b\n"
    ));
    EXPECT(debugPrints("global x = -a[1]\n",
        "Program@(1:1)\n"
        "  VarDecl@(1:1) global x\n"
        "    UnaryOp@(1:12) -\n"
        "      BinaryOp@(1:14) [\n"
        "        SymbolName@(1:13) a\n"
        "        Number@(1:15) 1\n"
    ));
    // and tighter than binary ones
    EXPECT(debugPrints("global x = a * b.c\n",
        "Program@(1:1)\n"
        "  VarDecl@(1:1) global x\n"
        "    BinaryOp@(1:14) *\n"
        "      SymbolName@(1:12) a\n"
        "      Member@(1:17)\n"
        "        SymbolName@(1:16) b\n"
        "        member: c\n"
    ));
}

TEST(dolang, parser_memberAndIndexErrors) {
    ClearPools clear_;
    // member name must be an identifier
    EXPECT(fails("global x = a."));
    EXPECT(fails("global x = a.1\n"));
    EXPECT(fails("global x = a.[1]\n"));
    // index must be a single expression, closed by ]
    EXPECT(fails("global x = a[]\n"));
    EXPECT(fails("global x = a[1\n"));
    EXPECT(fails("global x = a[1)\n"));
    EXPECT(fails("global x = a[1, 2]\n"));
}

TEST(dolang, parser_expressionErrors) {
    ClearPools clear_;
    // missing operands
    EXPECT(fails("fun f()\n  a +\n"));
    EXPECT(fails("fun f()\n  a = \n"));
    EXPECT(fails("fun f()\n  * a\n"));
    EXPECT(fails("fun f()\n  not\n"));
    EXPECT(fails("fun f()\n  global x =\n"));
    // only one assignment per statement
    EXPECT(fails("fun f()\n  a = b = c\n"));
    EXPECT(fails("fun f()\n  a = b += c\n"));
    // == and != cannot be mixed in a single chain
    EXPECT(fails("fun f()\n  a == b != c\n"));
    EXPECT(fails("fun f()\n  a != b == c\n"));
    // top level value without a declaration
    EXPECT(fails("global x = "));
}

// ------------------------------------------------------------------------------------------------
// errors
// ------------------------------------------------------------------------------------------------

// TODO no need to test errors until we get to proper error messages from the parser
TEST(dolang, parser_errors) {
    ClearPools clear_;
    // missing names
    EXPECT(fails("use"));
    EXPECT(fails("global"));
    EXPECT(fails("global : num"));
    // missing or invalid types
    EXPECT(fails("global x :"));
    EXPECT(fails("global x : 12"));
    // unclosed or mismatched brackets
    EXPECT(fails("global a : [num"));
    EXPECT(fails("global m : {num -> text)"));
    // map keys must be POD types
    EXPECT(fails("global m : {[num] -> text}"));
    // let is only allowed in function bodies
    EXPECT(fails("let x : num"));
}
