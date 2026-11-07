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

        bool operator == (Location const & other) const { 
            return line == other.line && col == other.col; 
        }
        bool operator != (Location const & other) const { 
            return line != other.line || col != other.col; 
        }
        
    } __attribute__((packed)); // rckid::dolang::Location

    static_assert(sizeof(Location) == 4, "Location should be 4 bytes");

    inline void write(Writer & w, Location const & loc) {
        w << "(" << loc.line << ":" << loc.col << ")";
    }   

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

        Token(): kind{TokenKind::EndOfFile} { }
        Token(Token const & other) = default;

        static Token error(Location loc, String msg) {
            TextLiteral text{std::move(msg)};
            return Token{TokenKind::Error, loc, text};
        }

        Token(TokenKind kind, Location loc, int32_t payload = 0, int32_t payload2 = 0): 
            kind{kind}, payload{payload}, payload2{payload2}, loc{loc} { }

        Token(TokenKind kind, Location loc, Symbol symbol): 
            kind{kind}, payload{static_cast<int32_t>(symbol.id_)}, loc{loc} { }

        Token(TokenKind kind, Location loc, TextLiteral text): 
            kind{kind}, payload{static_cast<int32_t>(text.id_)}, loc{loc} { }

        Token(TokenKind kind, Location loc, Num num): 
            kind{kind}, payload{num.n()}, payload2{num.d()}, loc{loc} { }

        bool operator == (TokenKind k) const { return kind == k; }
        bool operator != (TokenKind k) const { return kind != k; }

        bool isKeyword() const { return isKeyword(kind); }
        bool isBinaryOperator() const { return isBinaryOperator(kind); }
        bool isUnaryOperator() const { return isUnaryOperator(kind); }
        bool isAssignmentOperator() const { return isAssignmentOperator(kind); }

        static bool isKeyword(TokenKind k) { 
            return k >= TokenKind::kwAnd && k <= TokenKind::kwXor; 
        }

        static bool isBinaryOperator(TokenKind k) {
            switch (k) {
                case TokenKind::opAdd:
                case TokenKind::opSub:
                case TokenKind::opMul:
                case TokenKind::opDiv:
                case TokenKind::opPipe:
                case TokenKind::opAssign:
                case TokenKind::opAddAssign:
                case TokenKind::opSubAssign:
                case TokenKind::opMulAssign:
                case TokenKind::opDivAssign:
                case TokenKind::opEquals:
                case TokenKind::opNotEquals:
                case TokenKind::opLess:
                case TokenKind::opLessEquals:
                case TokenKind::opGreater:
                case TokenKind::opGreaterEquals:
                case TokenKind::kwAnd:
                case TokenKind::kwOr:
                    return true;
                default:
                    return false;
            }
        }

        static bool isUnaryOperator(TokenKind k) {
            switch (k) {
                case TokenKind::opNot:
                case TokenKind::opSub:
                case TokenKind::kwNot:
                case TokenKind::kwClone:
                    return true;
                default:
                    return false;
            }
        }

        static bool isAssignmentOperator(TokenKind k) {
            switch (k) {
                case TokenKind::opAssign:
                case TokenKind::opAddAssign:
                case TokenKind::opSubAssign:
                case TokenKind::opMulAssign:
                case TokenKind::opDivAssign:
                    return true;
                default:
                    return false;
            }
        }

        /** Identifiers and keywords carry the symbol of their name. */
        Symbol asSymbol() const {
            ASSERT(kind == TokenKind::Identifier || isKeyword());
            return Symbol{static_cast<uint32_t>(payload)};
        }

        /** Text literals and comments carry their text (comments including the leading #). */
        char const * asText() const {
            ASSERT(kind == TokenKind::Text || kind == TokenKind::Comment || kind == TokenKind::Error);
            return TextLiteral{static_cast<uint32_t>(payload)}.c_str();
        }

        Num asNum() const {
            ASSERT(kind == TokenKind::Number);
            return Num{payload, payload2};
        }

        /** Indentation (number of leading spaces) of the line that starts with this newline token. */
        int32_t indent() const {
            ASSERT(kind == TokenKind::Newline);
            return payload;
        }

    }; 

    class Lexer {
    public:
        Lexer(ReadStream & from): from_{from} {}

        Token next() {
            // if we are at the beginning of the input, always parse newline telling the initial indent
            if (loc_.line == 0) {
                loc_.line = 1;
                return parseNewline(true);
            }
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
                        return parseText(l);
                    // comment
                    case '#':
                        return parseComment(l);
                    // newline 
                    case '\n':
                        return parseNewline();
                    // whitespace, which we simply ignore (indentation whitespace is handled in parseNewline)
                    case ' ':
                    case '\t':
                        l = loc_;
                        break;
                    // otherwise it can be either numeric literal, identifier, or keyword
                    default:
                        if (isDigit(c))
                           return parseNumber(c, l); 
                        else if (isIdentifierStart(c))
                            return parseIdentifier(c, l);
                        else
                            return Token::error(l, STR("Invalid token start  - char " << c));
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

        char readNext() {
            auto c = from_.readByte();
            if (!c)
                return '\0';
            return c.value();
        }

        char pop() {
            char result = last_;
            last_ = '\0';
            if (result == '\0')
                result = readNext();
            if (result == '\r') {
                last_ = readNext();
                if (last_ == '\n')
                    last_ = '\0';
                result = '\n';
            }
            if (result == '\n') {
                loc_.line++;
                loc_.col = 1;
            } else {
                loc_.col++;
            }
            return result;
        }

        /** Returns the next character withouut consuming it
         */
        char top() {
            if (last_ == '\0')
                last_ = readNext();
            // convert all newlines to single \n character
            return last_ == '\r' ? '\n' : last_;
        }

        bool condPop(char expected) {
            if (top() == expected) {
                pop();
                return true;
            }
            return false;
        }

        /** Parses the newline character. 
         
            The trick is that newline followed by an indentation (spaces) has different meaning based on the context of the lexer. 
         */
        Token parseNewline(bool force = false) {
            while (! eof()) {
                // we should be at the beginning of a line, so column should be 1
                ASSERT(loc_.col == 1);
                // determine the indent, i.e. the number of continuous spaces before anything else
                int32_t indent = 0;
                while (condPop(' '))
                    indent++;
                // a line with only spaces at the end of input is not a new line
                if (eof())
                    break;
                // if the next character is newline, we ignore the empty line and parse the next one
                if (condPop('\n'))
                    continue;
                // if next character is whitespace, its an error as only spaces can be used to indent
                if (isWhitespace(top()))
                    return Token::error(loc_, STR("Invalid whitespace character " << top() << " at the beginning of a line"));
                // if next character is # (comment start), we just parse the comment, or ignore the line
                if ((top() == '#')  && ! force) {
                    if (parseComments_) {
                        Location l = loc_;
                        pop();
                        return parseComment(l);
                    }
                    // skip the comment line here instead of in parseComment() to avoid recursion over consecutive comment lines
                    while (! eof() && top() != '\n')
                        pop();
                    condPop('\n');
                    continue;
                }
                // otherwise simply emit the newline token with indent as payload - the parser will then translate those tokens into the indent/dedent pairs, etc.
                return Token{TokenKind::Newline, loc_, indent};
            }
            if (force)
                return Token{TokenKind::Newline, loc_, loc_.col - 1};
            else
                return Token{TokenKind::EndOfFile, loc_};
        }

        Token parseComment(Location loc) {
            if (parseComments_) {
                StringBuilder str;
                str.appendChar('#');
                while (! eof() && top() != '\n')
                    str.appendChar(pop());
                return Token{TokenKind::Comment, loc, TextLiteral{str.str()}};
            } else {
                while (! eof() && top() != '\n') {
                    pop();
                }
                return next();
            }
        }

        Token parseText(Location loc) {
            StringBuilder str;
            while (! eof()) {
                char c = pop();
                switch (c) {
                    case '\\':
                        if (eof())
                            return Token::error(loc_, "unterminated escape sequence");
                        c = pop();
                        switch (c) {
                            case 'n':
                                str.appendChar('\n');
                                break;
                            case 'r':
                                str.appendChar('\r');
                                break;
                            case 't':
                                str.appendChar('\t');
                                break;
                            case '"':
                                str.appendChar('"');
                                break;
                            case '\\':
                                str.appendChar('\\');
                                break;
                            default:
                                return Token::error(loc_, STR("Invalid escape sequence \\" << c));
                        }
                        break;
                    case '\n':
                        return Token::error(loc_, "Unexpected newline in text literal");
                    case '"':
                        return Token{TokenKind::Text, loc, TextLiteral{str.str()}};
                    default:
                        str.appendChar(c);
                }
            }
            return Token::error(loc_, "Unterminated text literal");
        }

        Token parseNumber(char first, Location loc) {
            int32_t value = first - '0';
            while (! eof() && (isDigit(top()) || top() == '_')) {
                if (condPop('_'))
                    continue;
                // check if we would have overflowed (must be done before the signed overflow UB)
                if (value > (Num::MAX - (top() - '0')) / 10)
                    return Token::error(loc, "Number literal is too large");
                value = value * 10 + (pop() - '0');
            }
            if (condPop('.')) {
                if (! isDigit(top()))
                    return Token::error(loc, "Expected digit after decimal point");
                int32_t frac = 0;
                int64_t fracDivider = 1;
                while (! eof() && (isDigit(top()) || top() == '_')) {
                    if (condPop('_'))
                        continue;
                    // if fracDiider
                    fracDivider *= 10;
                    if (fracDivider > Num::MAX)
                        return Token::error(loc, "Number literal is too large");
                    frac = frac * 10 + (pop() - '0');
                }
                // now we need to decrease
                return Token{TokenKind::Number, loc, Num{value * fracDivider + frac, fracDivider}};
            } else {
                return Token{TokenKind::Number, loc, Num{value}};
            }
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
        Location loc_{0, 1};
        char last_ = '\0';
        bool parseComments_ = true;

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

    inline void write(Writer & w, TokenKind kind) {
        switch (kind) {
            case TokenKind::EndOfFile: w << "EOF"; break;
            case TokenKind::Newline: w << "Newline"; break;
            case TokenKind::Indent: w << "Indent"; break;
            case TokenKind::Dedent: w << "Dedent"; break;
            case TokenKind::Error: w << "Error"; break;
            case TokenKind::Identifier: w << "Identifier"; break;
            case TokenKind::Number: w << "Number"; break;
            case TokenKind::Text: w << "Text"; break;
            case TokenKind::Comment: w << "Comment"; break;
            case TokenKind::opAdd: w << '+'; break;
            case TokenKind::opSub: w << '-'; break;
            case TokenKind::opMul: w << '*'; break;
            case TokenKind::opDiv: w << '/'; break;
            case TokenKind::opPipe: w << '|'; break;
            case TokenKind::opAssign: w << '='; break;
            case TokenKind::opAddAssign: w << "+="; break;
            case TokenKind::opSubAssign: w << "-="; break;
            case TokenKind::opMulAssign: w << "*="; break;
            case TokenKind::opDivAssign: w << "/="; break;
            case TokenKind::opColon: w << ':'; break;
            case TokenKind::opComma: w << ','; break;
            case TokenKind::opDot: w << '.'; break;
            case TokenKind::opArrow: w << "->"; break;
            case TokenKind::opParOpen: w << '('; break;
            case TokenKind::opParClose: w << ')'; break;
            case TokenKind::opSquareOpen: w << '['; break;
            case TokenKind::opSquareClose: w << ']'; break;
            case TokenKind::opCurlyOpen: w << '{'; break;
            case TokenKind::opCurlyClose: w << '}'; break;
            case TokenKind::opEquals: w << "=="; break;
            case TokenKind::opNotEquals: w << "!="; break;
            case TokenKind::opLess: w << '<'; break;
            case TokenKind::opLessEquals: w << "<="; break;
            case TokenKind::opGreater: w << '>'; break;
            case TokenKind::opGreaterEquals: w << ">="; break;
            case TokenKind::opNot: w << '!'; break;
            case TokenKind::kwAnd: w << "and"; break;
            case TokenKind::kwBool: w << "bool"; break;
            case TokenKind::kwBreak: w << "break"; break;
            case TokenKind::kwClone: w << "clone"; break;
            case TokenKind::kwConst: w << "const"; break;
            case TokenKind::kwConstructor: w << "constructor"; break;
            case TokenKind::kwContinue: w << "continue"; break;
            case TokenKind::kwDo: w << "do"; break;
            case TokenKind::kwElse: w << "else"; break;
            case TokenKind::kwEnum: w << "enum"; break;
            case TokenKind::kwEvent: w << "event"; break;
            case TokenKind::kwExtends: w << "extends"; break;
            case TokenKind::kwFalse: w << "false"; break;
            case TokenKind::kwFor: w << "for"; break;
            case TokenKind::kwFun: w << "fun"; break;
            case TokenKind::kwGlobal: w << "global"; break;
            case TokenKind::kwIf: w << "if"; break;
            case TokenKind::kwIn: w << "in"; break;
            case TokenKind::kwIs: w << "is"; break;
            case TokenKind::kwLet: w << "let"; break;
            case TokenKind::kwMatch: w << "match"; break;
            case TokenKind::kwNone: w << "none"; break;
            case TokenKind::kwNot: w << "not"; break;
            case TokenKind::kwNum: w << "num"; break;
            case TokenKind::kwOptional: w << "optional"; break;
            case TokenKind::kwOr: w << "or"; break;
            case TokenKind::kwOrElse: w << "orelse"; break;
            case TokenKind::kwReturn: w << "return"; break;
            case TokenKind::kwSelf: w << "self"; break;
            case TokenKind::kwStep: w << "step"; break;
            case TokenKind::kwTest: w << "test"; break;
            case TokenKind::kwText: w << "text"; break;
            case TokenKind::kwThen: w << "then"; break;
            case TokenKind::kwTrue: w << "true"; break;
            case TokenKind::kwType: w << "type"; break;
            case TokenKind::kwUse: w << "use"; break;
            case TokenKind::kwWhile: w << "while"; break;
            case TokenKind::kwWith: w << "with"; break;
            case TokenKind::kwWrite: w << "write"; break;
            case TokenKind::kwXor: w << "xor"; break;
        }
    }

    inline void write(Writer & w, Token const & t) {
        w << "Token(" << t.kind << ", " << t.payload << ", " << t.payload2 << ", (" << t.loc.line << ":" << t.loc.col << "))";
    }

} // namespace rckid::dolang