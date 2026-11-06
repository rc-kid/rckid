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

    /** True if the source does not parse. 
     */
    bool fails(char const * source) {
        auto result = parse(source);
        return result->kind == ast::Kind::Error;
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
    EXPECT(roundTrips("fun f()\n    break\n"));
    EXPECT(roundTrips("fun f() -> num\n    break\n"));
    EXPECT(roundTrips("fun f() -> optional num\n    break\n"));
}

TEST(dolang, parser_twoFunctions) {
    ClearPools clear_;
    EXPECT(roundTrips("fun f()\n    break\n    continue\nfun g() -> num\n    break\n    continue\n"));
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
