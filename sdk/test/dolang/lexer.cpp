#include <platform/tests.h>
#include <rckid/dolang/lexer.h>

using namespace rckid;
using namespace rckid::dolang;

TEST(dolang, lexer_isIdentifierStart) {
    EXPECT(Lexer::isIdentifierStart('a'));
    EXPECT(Lexer::isIdentifierStart('Z'));
    EXPECT(Lexer::isIdentifierStart('_'));
    EXPECT(!Lexer::isIdentifierStart('1'));
    EXPECT(!Lexer::isIdentifierStart('-'));
}


