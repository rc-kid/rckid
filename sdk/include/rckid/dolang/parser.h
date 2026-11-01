#pragma once

#include <rckid.h>

namespace rckid::dolang {

    enum class TokenKind {
        EndOfFile, 
        Newline,

        Identifier, 
        Number,
        Text,

        opAdd,
        opSub,
        opMul,
        opDiv,
        opPipe,
        opAssign,
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

        kwAnd, 
        kwBool, 
        kwBreak, 
        kwCase, 
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
        kwLet, 
        kwMatch, 
        kwNone, 
        kwNot, 
        kwNum, 
        kwOptional, 
        kwOr, 
        kwReturn, 
        kwSelf, 
        kwStep, 
        kwTest, 
        kwText, 
        kwTrue, 
        kwType, 
        kwUse, 
        kwWhile, 
        kwWrite, 
        kwXor,
    };

    class Parser {
    public:

        class Location {
            uint32_t line;
            uint32_t col;
        }; 

        class Token {
        public:
            TokenKind kind; 
            int32_t payload;
            Location loc;
        }; // dolang::Parser::Token

    }; // dolang::Parser

} // namespace rckid::doLang