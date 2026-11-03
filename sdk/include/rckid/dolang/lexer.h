#pragma once 

#include <rckid/rckid.h>
#include <rckid/stream.h>

#include <rckid/dolang/symbol.h>
#include <rckid/dolang/num.h>

namespace rckid::dolang {

    class Location {
    public:
        uint16_t line;
        uint16_t col;
    } __attribute__((packed)); // rckid::dolang::Location

    static_assert(sizeof(Location) == 4, "Location should be 4 bytes");

    enum class TokenKind {
        EndOfFile,
        Newline,
        Indent,
        Dedent,
        Error,

        Identifier, 
        Number,
        Text,
        Comment,

        opAdd,
        opSub,
        opMul,
        opDiv,
        opPipe,
        opAssign,
        opAddAssign,
        opSubAssign,
        opMulAssign,
        opDivAssign,
        opColon,
        opComma,
        opDot,
        opArrow,
        opParOpen,
        opParClose,
        opSquareOpen,
        opSquareClose,
        opCurlyOpen,
        opCurlyClose,
        opEquals,
        opNotEquals,
        opLess,
        opLessEquals,
        opGreater,
        opGreaterEquals,
        opNot, // reserved, not used in language

        kwAnd, 
        kwBool, 
        kwBreak, 
        kwClone, 
        kwConst, 
        kwConstructor, 
        kwContinue, 
        kwDo, 
        kwElse, 
        kwEnum, 
        kwEvent, 
        kwExtends, 
        kwFalse, 
        kwFor, 
        kwFun, 
        kwGlobal, 
        kwIf, 
        kwIn, 
        kwIs, 
        kwLet, 
        kwMatch, 
        kwNone, 
        kwNot, 
        kwNum, 
        kwOptional, 
        kwOr, 
        kwOrElse,
        kwReturn, 
        kwSelf, 
        kwStep, 
        kwTest, 
        kwText, 
        kwThen, 
        kwTrue, 
        kwType, 
        kwUse, 
        kwWhile, 
        kwWith, 
        kwWrite, 
        kwXor, // reserved
    }; // rckid::dolang::TokenKind

    class Token {
    public:
        TokenKind kind; 
        int32_t payload = 0;
        int32_t payload2 = 0;
        Location loc;

        Token(TokenKind kind, Location loc, int32_t payload = 0, int32_t payload2 = 0): 
            kind{kind}, payload{payload}, payload2{payload2}, loc{loc} { }

        Token(TokenKind kind, Location loc, Symbol symbol): 
            kind{kind}, payload{static_cast<int32_t>(symbol.id_)}, loc{loc} { }

        Symbol asSymbol() const {
            return Symbol{static_cast<uint32_t>(payload)};
        }

        Num asNum() const {
            return Num{payload, payload2};
        }

    }; 


    class Lexer {
    public:
        Lexer(ReadStream & from): from_{from} {}

        Token next() {
            Location l = loc_;
            if (eof())
                return Token{TokenKind::EndOfFile, l};
            while (! eof()) {
                char c = pop();
                ASSERT(c != '\0');
                switch (c) {
                    // operators
                    case '+':
                        return condPop('=') ? Token{TokenKind::opAddAssign, l} : Token{TokenKind::opAdd, l};
                    case '-':
                        if (condPop('>'))
                            return Token{TokenKind::opArrow, l};
                        else if (condPop('='))
                            return Token{TokenKind::opSubAssign, l};
                        else
                            return Token{TokenKind::opSub, l};
                    case '*':
                        return condPop('=') ? Token{TokenKind::opMulAssign, l} : Token{TokenKind::opMul, l};
                    case '/':
                        return condPop('=') ? Token{TokenKind::opDivAssign, l} : Token{TokenKind::opDiv, l};
                    case '|':
                        return Token{TokenKind::opPipe, l};
                    case '=':
                        return condPop('=') ? Token{TokenKind::opEquals, l} : Token{TokenKind::opAssign, l};
                    case ':':
                        return Token{TokenKind::opColon, l};
                    case ',':
                        return Token{TokenKind::opComma, l};
                    case '.':
                        return Token{TokenKind::opDot, l};
                    case '(':
                        return Token{TokenKind::opParOpen, l};
                    case ')':
                        return Token{TokenKind::opParClose, l};
                    case '[':
                        return Token{TokenKind::opSquareOpen, l};
                    case ']':
                        return Token{TokenKind::opSquareClose, l};
                    case '{':
                        return Token{TokenKind::opCurlyOpen, l};
                    case '}':
                        return Token{TokenKind::opCurlyClose, l};
                    case '!':
                        return condPop('=') ? Token{TokenKind::opNotEquals, l} : Token{TokenKind::opNot, l};
                    case '<':
                        return condPop('=') ? Token{TokenKind::opLessEquals, l} : Token{TokenKind::opLess, l};
                    case '>':
                        return condPop('=') ? Token{TokenKind::opGreaterEquals, l} : Token{TokenKind::opGreater, l};
                    // text literal  
                    case '"':
                        UNIMPLEMENTED;
                    // comment
                    case '#':
                        UNIMPLEMENTED;
                    // newline 
                    case '\n':
                        return parseNewline();
                    // whitespace, which we simply ignore (indentation whitespace is handled in parseNewline)
                    case ' ':
                    case '\t':
                    case '\r':
                        l = loc_;
                        break;
                    // otherwise it can be either numeric literal, identifier, or keyword
                    default:
                        if (isDigit(c))
                           return parseNumber(c, l); 
                        else if (isIdentifierStart(c))
                            return parseIdentifier(c, l);
                        else
                            UNIMPLEMENTED;
                }
            }
            return Token{TokenKind::EndOfFile, l};
        }

        static bool isIdentifierStart(char c) {
            return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
        }

        static bool isIdentifierPart(char c) {
            return isIdentifierStart(c) || isDigit(c);
        }

        static bool isDigit(char c) {
            return (c >= '0' && c <= '9');
        }

        static bool isWhitespace(char c) {
            return c == ' ' || c == '\t' || c == '\r' || c == '\n';
        }

    private:

        /** End of input is only reached when there is no lookahead character stored either. */
        bool eof() { return last_ == '\0' && from_.eof(); }

        char pop() {
            // TODO update loc
            if (last_ != '\0') {
                char result = last_;
                last_ = '\0';
                return result;
            }
            auto c = from_.readByte();
            if (!c)
                return '\0';
            return c.value();
        }

        char top() {
            if (last_ == '\0')
                last_ = pop();
            return last_;
        }

        bool condPop(char expected) {
            if (top() == expected) {
                last_ = '\0';
                return true;
            }
            return false;
        }

        /** Parses the newline character. 
         
            The trick is that newline followed by an indentation (spaces) has different meaning based on the context of the lexer. 
         */
        Token parseNewline() {
            while (! eof()) {
                // we should be at the beginning of a line, so column should be 1
                ASSERT(loc_.col == 1);
                // determine the indent, i.e. the number of continuous spaces before anything else
                int32_t indent = 0;
                while (condPop(' '))
                    indent++;
                // if the next character is newline, we ignore the empty line and parse the next one (handles CRLF too)
                condPop('\r');
                if (condPop('\n'))
                    continue;
                // if next character is whitespace, its an error as only spaces can be used to indent
                if (isWhitespace(top()))
                    return Token{TokenKind::Error, loc_};
                // if next character is # (comment start), we just parse the comment ?
                // TODO
                // otherwise simply emit the newline token with indent as payload - the parser will then translate those tokens into the indent/dedent pairs, etc.
                return Token{TokenKind::Newline, loc_, indent};
            }
            return Token{TokenKind::EndOfFile, loc_};
        }

        Token parseNumber(char first, Location loc) {
            UNIMPLEMENTED;
        }

        Token parseIdentifier(char first, Location loc) {
            StringBuilder str;
            str.appendChar(first);
            while (isIdentifierPart(top()))
                str.appendChar(pop());
            // check if the identifier is a keyword
            auto i = keywords_.find(str.str());
            if (i != keywords_.end())
                return Token{i->second, loc, Symbol{str.str()}};
            // it's identifier
            return Token{TokenKind::Identifier, loc, Symbol{str.str()}};
        }

        ReadStream & from_;
        Location loc_{1, 1};
        char last_ = '\0';

        std::unordered_map<String, TokenKind> keywords_ = {
            {"and", TokenKind::kwAnd},
            {"bool", TokenKind::kwBool},
            {"break", TokenKind::kwBreak},
            {"clone", TokenKind::kwClone},
            {"const", TokenKind::kwConst},
            {"constructor", TokenKind::kwConstructor},
            {"continue", TokenKind::kwContinue},
            {"do", TokenKind::kwDo},
            {"else", TokenKind::kwElse},
            {"enum", TokenKind::kwEnum},
            {"event", TokenKind::kwEvent},
            {"extends", TokenKind::kwExtends},
            {"false", TokenKind::kwFalse},
            {"for", TokenKind::kwFor},
            {"fun", TokenKind::kwFun},
            {"global", TokenKind::kwGlobal},
            {"if", TokenKind::kwIf},
            {"in", TokenKind::kwIn},
            {"is", TokenKind::kwIs},
            {"let", TokenKind::kwLet},
            {"match", TokenKind::kwMatch},
            {"none", TokenKind::kwNone},
            {"not", TokenKind::kwNot},
            {"num", TokenKind::kwNum},
            {"optional", TokenKind::kwOptional},
            {"or", TokenKind::kwOr},
            {"orelse", TokenKind::kwOrElse},
            {"return", TokenKind::kwReturn},
            {"self", TokenKind::kwSelf},
            {"step", TokenKind::kwStep},
            {"test", TokenKind::kwTest},
            {"text", TokenKind::kwText},
            {"then", TokenKind::kwThen},
            {"true", TokenKind::kwTrue},
            {"type", TokenKind::kwType},
            {"use", TokenKind::kwUse},
            {"while", TokenKind::kwWhile},
            {"with", TokenKind::kwWith},
            {"write", TokenKind::kwWrite},
            {"xor", TokenKind::kwXor}
        };

    }; // rckid::dolang::Lexer

} // namespace rckid::dolang