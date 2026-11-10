#include <cstring>
#include <vector>

#include <platform/tests.h>
#include <rckid/dolang/lexer.h>

using namespace rckid;
using namespace rckid::dolang;

namespace {

    /** Lexes the given source until the end of file and returns all tokens (without the final EndOfFile), including the initial newline.
     */
    std::vector<Token> lexAll(char const * source) {
        MemoryReadStream stream{immutable_ptr<uint8_t>{reinterpret_cast<uint8_t const *>(source), static_cast<uint32_t>(std::strlen(source))}};
        Lexer lexer{stream};
        std::vector<Token> result;
        // the limit guards against a lexer stuck in an infinite loop
        while (result.size() < 1000) {
            Token t = lexer.next();
            if (t == TokenKind::EndOfFile)
                break;
            result.push_back(t);
        }
        return result;
    }

    /** Lexes the given source and returns all tokens after the initial newline, which the lexer always emits first (it gives the indentation of the first line).

        If the initial newline is missing, returns a single Error token instead, so that any test using the helper fails.
     */
    std::vector<Token> lex(char const * source) {
        std::vector<Token> result = lexAll(source);
        if (result.empty() || result[0] != TokenKind::Newline)
            return { Token{TokenKind::Error, Location{0, 0}} };
        result.erase(result.begin());
        return result;
    }

    /** Returns the initial newline token of the source (or EndOfFile if there are no tokens at all).
     */
    Token initial(char const * source) {
        auto tokens = lexAll(source);
        return tokens.empty() ? Token{TokenKind::EndOfFile, Location{0, 0}} : tokens[0];
    }

    /** Returns the kinds of all tokens in the source, for checking token sequences.
     */
    std::vector<TokenKind> kinds(char const * source) {
        std::vector<TokenKind> result;
        for (Token const & t : lex(source))
            result.push_back(t.kind);
        return result;
    }

    /** Returns the first token of the source.
     */
    Token first(char const * source) {
        auto tokens = lex(source);
        return tokens.empty() ? Token{TokenKind::EndOfFile, Location{0, 0}} : tokens[0];
    }

    /** True if the source consists of exactly one token of given kind.
     */
    bool single(char const * source, TokenKind kind) {
        auto tokens = lex(source);
        return tokens.size() == 1 && tokens[0] == kind;
    }

    bool isIdentifier(Token const & t, char const * name) {
        return t == TokenKind::Identifier && t.asSymbol() == Symbol{name};
    }

    bool isNumber(Token const & t, Num value) {
        return t == TokenKind::Number && t.asNum() == value;
    }

    bool isText(Token const & t, char const * text) {
        return t == TokenKind::Text && std::strcmp(t.asText(), text) == 0;
    }

    bool isComment(Token const & t, char const * text) {
        return t == TokenKind::Comment && std::strcmp(t.asText(), text) == 0;
    }

    bool isNewline(Token const & t, int32_t indent) {
        return t == TokenKind::Newline && t.indent() == indent;
    }

    bool isAt(Token const & t, uint16_t line, uint16_t col) {
        return t.loc == Location{line, col};
    }

    /** Clears the interned symbols and texts at the end of a test, so that the heap is left as it was found (tests of other modules check exact heap usage).
     */
    class ClearPools {
    public:
        ~ClearPools() {
            Symbol::clear();
            TextLiteral::clear();
        }
    };

} // anonymous namespace

TEST(dolang, lexer_isIdentifierStart) {
    EXPECT(Lexer::isIdentifierStart('a'));
    EXPECT(Lexer::isIdentifierStart('Z'));
    EXPECT(Lexer::isIdentifierStart('_'));
    EXPECT(!Lexer::isIdentifierStart('1'));
    EXPECT(!Lexer::isIdentifierStart('-'));
}

// ------------------------------------------------------------------------------------------------
// single tokens
// ------------------------------------------------------------------------------------------------

TEST(dolang, lexer_empty) {
    ClearPools clear_;
    EXPECT(lex("").empty());
    EXPECT(lex("   ").empty());
}

TEST(dolang, lexer_operators) {
    ClearPools clear_;
    EXPECT(single("+", TokenKind::opAdd));
    EXPECT(single("-", TokenKind::opSub));
    EXPECT(single("*", TokenKind::opMul));
    EXPECT(single("/", TokenKind::opDiv));
    EXPECT(single("=", TokenKind::opAssign));
    EXPECT(single("+=", TokenKind::opAddAssign));
    EXPECT(single("-=", TokenKind::opSubAssign));
    EXPECT(single("*=", TokenKind::opMulAssign));
    EXPECT(single("/=", TokenKind::opDivAssign));
    EXPECT(single(":", TokenKind::opColon));
    EXPECT(single(",", TokenKind::opComma));
    EXPECT(single(".", TokenKind::opDot));
    EXPECT(single("->", TokenKind::opArrow));
    EXPECT(single("==", TokenKind::opEquals));
    EXPECT(single("!=", TokenKind::opNotEquals));
    EXPECT(single("<", TokenKind::opLess));
    EXPECT(single("<=", TokenKind::opLessEquals));
    EXPECT(single(">", TokenKind::opGreater));
    EXPECT(single(">=", TokenKind::opGreaterEquals));
    EXPECT(single("!", TokenKind::opNot));
}

TEST(dolang, lexer_brackets) {
    ClearPools clear_;
    EXPECT(single("(", TokenKind::opParOpen));
    EXPECT(single(")", TokenKind::opParClose));
    EXPECT(single("[", TokenKind::opSquareOpen));
    EXPECT(single("]", TokenKind::opSquareClose));
    EXPECT(single("{", TokenKind::opCurlyOpen));
    EXPECT(single("}", TokenKind::opCurlyClose));
}

TEST(dolang, lexer_identifiers) {
    ClearPools clear_;
    EXPECT(isIdentifier(first("foo"), "foo"));
    EXPECT(isIdentifier(first("_bar"), "_bar"));
    EXPECT(isIdentifier(first("a1_b2"), "a1_b2"));
    EXPECT(isIdentifier(first("CamelCase"), "CamelCase"));
    // the same name is always the same symbol
    EXPECT(first("foo").asSymbol() == first("foo").asSymbol());
    EXPECT(first("foo").asSymbol() != first("bar").asSymbol());
}

TEST(dolang, lexer_keywords) {
    ClearPools clear_;
    EXPECT(single("if", TokenKind::kwIf));
    EXPECT(single("then", TokenKind::kwThen));
    EXPECT(single("else", TokenKind::kwElse));
    EXPECT(single("let", TokenKind::kwLet));
    EXPECT(single("fun", TokenKind::kwFun));
    EXPECT(single("match", TokenKind::kwMatch));
    EXPECT(single("when", TokenKind::kwWhen));
    EXPECT(single("is", TokenKind::kwIs));
    EXPECT(single("orelse", TokenKind::kwOrElse));
    EXPECT(single("xor", TokenKind::kwXor));
    // keywords carry their name as symbol as well
    EXPECT(first("while").asSymbol() == Symbol{"while"});
}

TEST(dolang, lexer_keywordLookalikes) {
    ClearPools clear_;
    // keywords are case sensitive and must match the whole identifier
    EXPECT(isIdentifier(first("orElse"), "orElse"));
    EXPECT(isIdentifier(first("If"), "If"));
    EXPECT(isIdentifier(first("iffy"), "iffy"));
    EXPECT(isIdentifier(first("if_"), "if_"));
}

TEST(dolang, lexer_integers) {
    ClearPools clear_;
    EXPECT(isNumber(first("0"), Num{0}));
    EXPECT(isNumber(first("7"), Num{7}));
    EXPECT(isNumber(first("42"), Num{42}));
    EXPECT(isNumber(first("1_000"), Num{1000}));
    EXPECT(isNumber(first("2147483647"), Num{2147483647}));
}

TEST(dolang, lexer_integerOverflow) {
    ClearPools clear_;
    EXPECT(first("2147483648") == TokenKind::Error);
}

TEST(dolang, lexer_decimals) {
    ClearPools clear_;
    EXPECT(isNumber(first("1.5"), Num{3, 2}));
    EXPECT(isNumber(first("0.25"), Num{1, 4}));
    EXPECT(isNumber(first("1.567"), Num{1567, 1000}));
    EXPECT(isNumber(first("0.001"), Num{1, 1000}));
    EXPECT(isNumber(first("2.0"), Num{2}));
    EXPECT(isNumber(first("1_000.000_1"), Num{10000001, 10000}));
}

TEST(dolang, lexer_decimalNeedsDigitsAfterDot) {
    ClearPools clear_;
    EXPECT(first("1.") == TokenKind::Error);
    EXPECT(first("1.x") == TokenKind::Error);
}

TEST(dolang, lexer_text) {
    ClearPools clear_;
    EXPECT(isText(first("\"hello\""), "hello"));
    EXPECT(isText(first("\"\""), ""));
    EXPECT(isText(first("\"with spaces and # not a comment\""), "with spaces and # not a comment"));
}

TEST(dolang, lexer_textEscapes) {
    ClearPools clear_;
    EXPECT(isText(first("\"a\\nb\""), "a\nb"));
    EXPECT(isText(first("\"a\\rb\""), "a\rb"));
    EXPECT(isText(first("\"a\\tb\""), "a\tb"));
    EXPECT(isText(first("\"say \\\"hi\\\"\""), "say \"hi\""));
    EXPECT(isText(first("\"back\\\\slash\""), "back\\slash"));
}

TEST(dolang, lexer_textErrors) {
    ClearPools clear_;
    // unterminated
    EXPECT(first("\"abc") == TokenKind::Error);
    // text cannot span multiple lines
    EXPECT(first("\"ab\ncd\"") == TokenKind::Error);
    EXPECT(first("\"ab\r\ncd\"") == TokenKind::Error);
    // unknown escape
    EXPECT(first("\"\\q\"") == TokenKind::Error);
}

TEST(dolang, lexer_comments) {
    ClearPools clear_;
    EXPECT(isComment(first("# hello"), "# hello"));
    EXPECT(isComment(first("#"), "#"));
}

TEST(dolang, lexer_invalidCharacters) {
    ClearPools clear_;
    EXPECT(first("@") == TokenKind::Error);
    EXPECT(first("$") == TokenKind::Error);
    // pipe is not used, enum values are a list
    EXPECT(first("|") == TokenKind::Error);
}

// ------------------------------------------------------------------------------------------------
// whitespace, newlines & locations
// ------------------------------------------------------------------------------------------------

TEST(dolang, lexer_whitespaceIsIgnored) {
    ClearPools clear_;
    EXPECT(kinds("a   +\tb") == std::vector<TokenKind>{TokenKind::Identifier, TokenKind::opAdd, TokenKind::Identifier});
    EXPECT(kinds("a+b") == std::vector<TokenKind>{TokenKind::Identifier, TokenKind::opAdd, TokenKind::Identifier});
}

TEST(dolang, lexer_locationsOnLine) {
    ClearPools clear_;
    auto t = lex("foo += 12 \"x\"");
    CHECK(t.size() == 4);
    EXPECT(isAt(t[0], 1, 1));
    EXPECT(isAt(t[1], 1, 5));
    EXPECT(isAt(t[2], 1, 8));
    EXPECT(isAt(t[3], 1, 11));
}

TEST(dolang, lexer_newline) {
    ClearPools clear_;
    auto t = lex("a\nb");
    CHECK(t.size() == 3);
    EXPECT(isIdentifier(t[0], "a"));
    EXPECT(isNewline(t[1], 0));
    EXPECT(isIdentifier(t[2], "b"));
    EXPECT(isAt(t[2], 2, 1));
}

TEST(dolang, lexer_newlineIndent) {
    ClearPools clear_;
    auto t = lex("a\n    b");
    CHECK(t.size() == 3);
    EXPECT(isNewline(t[1], 4));
    // the newline token is located at the first token of the new line
    EXPECT(isAt(t[1], 2, 5));
    EXPECT(isAt(t[2], 2, 5));
}

TEST(dolang, lexer_emptyLinesAreIgnored) {
    ClearPools clear_;
    auto t = lex("a\n\n\nb");
    CHECK(t.size() == 3);
    EXPECT(isNewline(t[1], 0));
    EXPECT(isAt(t[2], 4, 1));
}

TEST(dolang, lexer_linesWithOnlySpacesAreIgnored) {
    ClearPools clear_;
    auto t = lex("a\n      \n  b");
    CHECK(t.size() == 3);
    EXPECT(isNewline(t[1], 2));
    EXPECT(isAt(t[2], 3, 3));
}

TEST(dolang, lexer_crlf) {
    ClearPools clear_;
    auto t = lex("a\r\nb\r\n\r\nc");
    CHECK(t.size() == 5);
    EXPECT(isNewline(t[1], 0));
    EXPECT(isAt(t[2], 2, 1));
    EXPECT(isNewline(t[3], 0));
    EXPECT(isAt(t[4], 4, 1));
}

TEST(dolang, lexer_crOnly) {
    ClearPools clear_;
    auto t = lex("a\rb\r\rc");
    CHECK(t.size() == 5);
    EXPECT(isAt(t[2], 2, 1));
    EXPECT(isAt(t[4], 4, 1));
}

TEST(dolang, lexer_mixedLineEndings) {
    ClearPools clear_;
    auto t = lex("a\n\r\n\rb");
    CHECK(t.size() == 3);
    EXPECT(isNewline(t[1], 0));
    EXPECT(isAt(t[2], 4, 1));
}

TEST(dolang, lexer_trailingNewlines) {
    ClearPools clear_;
    // newlines at the end of input do not produce newline tokens
    EXPECT(kinds("a\n") == std::vector<TokenKind>{TokenKind::Identifier});
    EXPECT(kinds("a\n\n\n") == std::vector<TokenKind>{TokenKind::Identifier});
    EXPECT(kinds("a\n    ") == std::vector<TokenKind>{TokenKind::Identifier});
    EXPECT(kinds("a\r\n") == std::vector<TokenKind>{TokenKind::Identifier});
}

// ------------------------------------------------------------------------------------------------
// initial newline
// ------------------------------------------------------------------------------------------------

TEST(dolang, lexer_initialNewline) {
    ClearPools clear_;
    // the first token is always a newline with the indentation of the first line, located at its first token
    EXPECT(isNewline(initial("a"), 0));
    EXPECT(isAt(initial("a"), 1, 1));
    EXPECT(isNewline(initial("  a"), 2));
    EXPECT(isAt(initial("  a"), 1, 3));
    // the first token after it is where it actually is
    auto t = lex("  a");
    CHECK(t.size() == 1);
    EXPECT(isAt(t[0], 1, 3));
}

TEST(dolang, lexer_initialNewlineOfEmptyInput) {
    ClearPools clear_;
    EXPECT(initial("") == TokenKind::Newline);
    EXPECT(initial("   ") == TokenKind::Newline);
    EXPECT(initial("\n\n") == TokenKind::Newline);
    EXPECT(lexAll("").size() == 1);
    EXPECT(lexAll("   ").size() == 1);
    EXPECT(lexAll("\n\n").size() == 1);
}

TEST(dolang, lexer_leadingEmptyLines) {
    ClearPools clear_;
    // empty lines before the first one do not count, the initial newline gives the indentation of the first non-empty line
    EXPECT(isNewline(initial("\n\n  a"), 2));
    EXPECT(isAt(initial("\n\n  a"), 3, 3));
    EXPECT(isNewline(initial("    \n    a"), 4));
    auto t = lex("\n\n  a");
    CHECK(t.size() == 1);
    EXPECT(isIdentifier(t[0], "a"));
    EXPECT(isAt(t[0], 3, 3));
}

TEST(dolang, lexer_leadingCommentDefinesIndent) {
    ClearPools clear_;
    // if the first non-empty line is a comment, its indentation is the top level indentation
    EXPECT(isNewline(initial("    # c\n    a"), 4));
    EXPECT(isNewline(initial("\n  # c\na"), 2));
    auto t = lex("  # c\n  a");
    CHECK(t.size() == 3);
    EXPECT(isComment(t[0], "# c"));
    EXPECT(isNewline(t[1], 2));
    EXPECT(isIdentifier(t[2], "a"));
}

TEST(dolang, lexer_tabIndentIsError) {
    ClearPools clear_;
    EXPECT(initial("\ta") == TokenKind::Error);
    EXPECT(initial("  \ta") == TokenKind::Error);
    EXPECT(kinds("a\n\tb") == std::vector<TokenKind>{TokenKind::Identifier, TokenKind::Error, TokenKind::Identifier});
    EXPECT(kinds("a\n  \tb") == std::vector<TokenKind>{TokenKind::Identifier, TokenKind::Error, TokenKind::Identifier});
}

TEST(dolang, lexer_commentAfterCode) {
    ClearPools clear_;
    auto t = lex("a # note\nb");
    CHECK(t.size() == 4);
    EXPECT(isIdentifier(t[0], "a"));
    EXPECT(isComment(t[1], "# note"));
    EXPECT(isAt(t[1], 1, 3));
    EXPECT(isNewline(t[2], 0));
    EXPECT(isAt(t[3], 2, 1));
}

TEST(dolang, lexer_commentLinesDoNotAffectLayout) {
    ClearPools clear_;
    // the comment line has its own indentation, but only the line after it produces the newline token
    auto t = lex("a\n        # note\n  b");
    CHECK(t.size() == 4);
    EXPECT(isComment(t[1], "# note"));
    EXPECT(isAt(t[1], 2, 9));
    EXPECT(isNewline(t[2], 2));
    EXPECT(isAt(t[3], 3, 3));
}

TEST(dolang, lexer_lineCountingAcrossEverything) {
    ClearPools clear_;
    // text, comments, empty lines and CRLF all keep the line count right
    auto t = lex("\"x\" # c\r\n\n# only comment\nlet a = 1\r\n    b");
    CHECK(t.size() == 10);
    EXPECT(isText(t[0], "x"));
    EXPECT(isAt(t[0], 1, 1));
    EXPECT(isComment(t[1], "# c"));
    EXPECT(isAt(t[1], 1, 5));
    EXPECT(isComment(t[2], "# only comment"));
    EXPECT(isAt(t[2], 3, 1));
    EXPECT(isNewline(t[3], 0));
    EXPECT(t[4] == TokenKind::kwLet);
    EXPECT(isAt(t[4], 4, 1));
    EXPECT(isNumber(t[7], Num{1}));
    EXPECT(isAt(t[7], 4, 9));
    EXPECT(isNewline(t[8], 4));
    EXPECT(isIdentifier(t[9], "b"));
    EXPECT(isAt(t[9], 5, 5));
}

// ------------------------------------------------------------------------------------------------
// token sequences
// ------------------------------------------------------------------------------------------------

TEST(dolang, lexer_letStatement) {
    ClearPools clear_;
    EXPECT(kinds("let x = 1 + 2") == std::vector<TokenKind>{
        TokenKind::kwLet, TokenKind::Identifier, TokenKind::opAssign, TokenKind::Number, TokenKind::opAdd, TokenKind::Number
    });
}

TEST(dolang, lexer_longestOperatorWins) {
    ClearPools clear_;
    EXPECT(kinds("a->b") == std::vector<TokenKind>{TokenKind::Identifier, TokenKind::opArrow, TokenKind::Identifier});
    EXPECT(kinds("a-b") == std::vector<TokenKind>{TokenKind::Identifier, TokenKind::opSub, TokenKind::Identifier});
    EXPECT(kinds("a- >b") == std::vector<TokenKind>{TokenKind::Identifier, TokenKind::opSub, TokenKind::opGreater, TokenKind::Identifier});
    EXPECT(kinds("a<=b") == std::vector<TokenKind>{TokenKind::Identifier, TokenKind::opLessEquals, TokenKind::Identifier});
    EXPECT(kinds("a!=b") == std::vector<TokenKind>{TokenKind::Identifier, TokenKind::opNotEquals, TokenKind::Identifier});
    EXPECT(kinds("a==b") == std::vector<TokenKind>{TokenKind::Identifier, TokenKind::opEquals, TokenKind::Identifier});
    EXPECT(kinds("a===b") == std::vector<TokenKind>{TokenKind::Identifier, TokenKind::opEquals, TokenKind::opAssign, TokenKind::Identifier});
}

TEST(dolang, lexer_rational) {
    ClearPools clear_;
    auto t = lex("x += 2/3");
    CHECK(t.size() == 5);
    EXPECT(t[1] == TokenKind::opAddAssign);
    EXPECT(isNumber(t[2], Num{2}));
    EXPECT(t[3] == TokenKind::opDiv);
    EXPECT(isNumber(t[4], Num{3}));
}

TEST(dolang, lexer_memberAccess) {
    ClearPools clear_;
    EXPECT(kinds("a.b.c") == std::vector<TokenKind>{TokenKind::Identifier, TokenKind::opDot, TokenKind::Identifier, TokenKind::opDot, TokenKind::Identifier});
}

TEST(dolang, lexer_functionSignature) {
    ClearPools clear_;
    EXPECT(kinds("fun(a : num, b : text) -> bool") == std::vector<TokenKind>{
        TokenKind::kwFun, TokenKind::opParOpen, TokenKind::Identifier, TokenKind::opColon, TokenKind::kwNum, TokenKind::opComma,
        TokenKind::Identifier, TokenKind::opColon, TokenKind::kwText, TokenKind::opParClose, TokenKind::opArrow, TokenKind::kwBool
    });
}

TEST(dolang, lexer_mapLookup) {
    ClearPools clear_;
    auto t = lex("m(1) orelse \"none\"");
    CHECK(t.size() == 6);
    EXPECT(isIdentifier(t[0], "m"));
    EXPECT(t[1] == TokenKind::opParOpen);
    EXPECT(isNumber(t[2], Num{1}));
    EXPECT(t[3] == TokenKind::opParClose);
    EXPECT(t[4] == TokenKind::kwOrElse);
    EXPECT(isText(t[5], "none"));
}

TEST(dolang, lexer_ifElseBlock) {
    ClearPools clear_;
    auto t = lex(
        "if a <= 10 then\n"
        "    write \"small\"\n"
        "else\n"
        "    write \"big\"\n"
    );
    CHECK(t.size() == 13);
    EXPECT(t[0] == TokenKind::kwIf);
    EXPECT(isIdentifier(t[1], "a"));
    EXPECT(t[2] == TokenKind::opLessEquals);
    EXPECT(isNumber(t[3], Num{10}));
    EXPECT(t[4] == TokenKind::kwThen);
    EXPECT(isNewline(t[5], 4));
    EXPECT(t[6] == TokenKind::kwWrite);
    EXPECT(isText(t[7], "small"));
    EXPECT(isNewline(t[8], 0));
    EXPECT(t[9] == TokenKind::kwElse);
    EXPECT(isNewline(t[10], 4));
    EXPECT(t[11] == TokenKind::kwWrite);
    EXPECT(isText(t[12], "big"));
    EXPECT(isAt(t[12], 4, 11));
}

TEST(dolang, lexer_mapLiteral) {
    ClearPools clear_;
    EXPECT(kinds(
        "let m = {\n"
        "    1 -> \"foo\",\n"
        "    2 -> \"bar\"\n"
        "}"
    ) == std::vector<TokenKind>{
        TokenKind::kwLet, TokenKind::Identifier, TokenKind::opAssign, TokenKind::opCurlyOpen, TokenKind::Newline,
        TokenKind::Number, TokenKind::opArrow, TokenKind::Text, TokenKind::opComma, TokenKind::Newline,
        TokenKind::Number, TokenKind::opArrow, TokenKind::Text, TokenKind::Newline,
        TokenKind::opCurlyClose
    });
}
