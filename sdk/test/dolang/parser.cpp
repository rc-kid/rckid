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

// ------------------------------------------------------------------------------------------------
// precedence (via debug prints, as the pretty print does not show the tree structure)
// ------------------------------------------------------------------------------------------------

TEST(dolang, parser_precedenceMulOverAdd) {
    ClearPools clear_;
    EXPECT(debugPrints("global x = a + b * c\n",
        "Program@(1:1)\n"
        "  VarDecl@(1:1)global x\n"
        "    BinaryOp@(1:14)+\n"
        "      SymbolName@(1:12)a\n"
        "      BinaryOp@(1:18)*\n"
        "        SymbolName@(1:16)b\n"
        "        SymbolName@(1:20)c\n"
    ));
    EXPECT(debugPrints("global x = a * b + c\n",
        "Program@(1:1)\n"
        "  VarDecl@(1:1)global x\n"
        "    BinaryOp@(1:18)+\n"
        "      BinaryOp@(1:14)*\n"
        "        SymbolName@(1:12)a\n"
        "        SymbolName@(1:16)b\n"
        "      SymbolName@(1:20)c\n"
    ));
}

TEST(dolang, parser_precedenceLeftAssociative) {
    ClearPools clear_;
    // a - b - c is (a - b) - c
    EXPECT(debugPrints("global x = a - b - c\n",
        "Program@(1:1)\n"
        "  VarDecl@(1:1)global x\n"
        "    BinaryOp@(1:18)-\n"
        "      BinaryOp@(1:14)-\n"
        "        SymbolName@(1:12)a\n"
        "        SymbolName@(1:16)b\n"
        "      SymbolName@(1:20)c\n"
    ));
    // a / b / c is (a / b) / c
    EXPECT(debugPrints("global x = a / b / c\n",
        "Program@(1:1)\n"
        "  VarDecl@(1:1)global x\n"
        "    BinaryOp@(1:18)/\n"
        "      BinaryOp@(1:14)/\n"
        "        SymbolName@(1:12)a\n"
        "        SymbolName@(1:16)b\n"
        "      SymbolName@(1:20)c\n"
    ));
}

TEST(dolang, parser_precedenceAndOverOr) {
    ClearPools clear_;
    EXPECT(debugPrints("global x = a or b and c\n",
        "Program@(1:1)\n"
        "  VarDecl@(1:1)global x\n"
        "    BinaryOp@(1:14)or\n"
        "      SymbolName@(1:12)a\n"
        "      BinaryOp@(1:19)and\n"
        "        SymbolName@(1:17)b\n"
        "        SymbolName@(1:23)c\n"
    ));
}

TEST(dolang, parser_precedenceNot) {
    ClearPools clear_;
    // not is below comparisons, so not a == b is not (a == b)
    EXPECT(debugPrints("global x = not a == b\n",
        "Program@(1:1)\n"
        "  VarDecl@(1:1)global x\n"
        "    UnaryOp@(1:12)not\n"
        "      BinaryOp@(1:18)==\n"
        "        SymbolName@(1:16)a\n"
        "        SymbolName@(1:21)b\n"
    ));
    // but above and, so not a and b is (not a) and b
    EXPECT(debugPrints("global x = not a and b\n",
        "Program@(1:1)\n"
        "  VarDecl@(1:1)global x\n"
        "    BinaryOp@(1:18)and\n"
        "      UnaryOp@(1:12)not\n"
        "        SymbolName@(1:16)a\n"
        "      SymbolName@(1:22)b\n"
    ));
}

TEST(dolang, parser_precedenceComparisons) {
    ClearPools clear_;
    // arithmetic binds tighter than comparison
    EXPECT(debugPrints("global x = a < b + c\n",
        "Program@(1:1)\n"
        "  VarDecl@(1:1)global x\n"
        "    BinaryOp@(1:14)<\n"
        "      SymbolName@(1:12)a\n"
        "      BinaryOp@(1:18)+\n"
        "        SymbolName@(1:16)b\n"
        "        SymbolName@(1:20)c\n"
    ));
    // comparison binds tighter than equality
    EXPECT(debugPrints("global x = a == b < c\n",
        "Program@(1:1)\n"
        "  VarDecl@(1:1)global x\n"
        "    BinaryOp@(1:14)==\n"
        "      SymbolName@(1:12)a\n"
        "      BinaryOp@(1:19)<\n"
        "        SymbolName@(1:17)b\n"
        "        SymbolName@(1:21)c\n"
    ));
}

TEST(dolang, parser_precedenceUnary) {
    ClearPools clear_;
    // prefix operators bind tighter than binary ones
    EXPECT(debugPrints("global x = -a * b\n",
        "Program@(1:1)\n"
        "  VarDecl@(1:1)global x\n"
        "    BinaryOp@(1:15)*\n"
        "      UnaryOp@(1:12)-\n"
        "        SymbolName@(1:13)a\n"
        "      SymbolName@(1:17)b\n"
    ));
    EXPECT(debugPrints("global x = clone a + b\n",
        "Program@(1:1)\n"
        "  VarDecl@(1:1)global x\n"
        "    BinaryOp@(1:20)+\n"
        "      UnaryOp@(1:12)clone\n"
        "        SymbolName@(1:18)a\n"
        "      SymbolName@(1:22)b\n"
    ));
}

TEST(dolang, parser_precedenceAssignment) {
    ClearPools clear_;
    // assignment is the root of the statement
    EXPECT(debugPrints("fun f()\n    a = b + c\n",
        "Program@(1:1)\n"
        "  FunDecl@(1:1)f\n"
        "    Block@(2:5)\n"
        "      Assign@(2:7)=\n"
        "        SymbolName@(2:5)a\n"
        "        BinaryOp@(2:11)+\n"
        "          SymbolName@(2:9)b\n"
        "          SymbolName@(2:13)c\n"
        "\n"
    ));
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
    // not a top level declaration
    EXPECT(fails("12"));
    // let is only allowed in function bodies
    EXPECT(fails("let x : num"));
}
