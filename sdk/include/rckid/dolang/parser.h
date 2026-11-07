#pragma once

#include <rckid/rckid.h>
#include <rckid/dolang/lexer.h>
#include <rckid/dolang/ast.h>

namespace rckid::dolang {

    class Parser {
    public:
        Parser(ReadStream & from): lexer_{from} {
            // contract from lexer is first token is new line which we use to initialize the indent stack 
            Token t = lexer_.next();
            ASSERT(t == TokenKind::Newline);
            indentStack_.push_back(t.payload);
        }

        std::unique_ptr<ast::Node> parse() {
            auto result = PROGRAM();
            if (result != nullptr)
                return result;
            return  std::make_unique<ast::Error>(error_);
        }

    private:

/** A macro for propagating parser errors further up the parse stack. NOTE this is gcc specific.
 */
#define TRY(X) ({ auto result = X; if (! result) return nullptr; std::move(result); })

#define POP(TOKEN_KIND) ({ auto result = pop(); if (result.kind != TOKEN_KIND) { \
    error_ = Token::error(result.loc, STR("Expected " << TOKEN_KIND << ", got " << result.kind)); \
    return nullptr; } std::move(result); })

        /** PROGRAM := [ TOP_DECL { newline TOP_DECL } ] 
         */
        unique_ptr<ast::Program> PROGRAM() {
            auto result = std::make_unique<ast::Program>(top().loc);
            if (! eof()) {
                result->addChild(TRY(TOP_DECL()));
                while (condPop(TokenKind::Newline))
                    result->addChild(TRY(TOP_DECL()));
            }
            return result;
        }

        /** TOP_DECL := USE_DECL | GLOBAL_DECL | CONST_DECL | FUN_DECL | TYPE_DECL | ENUM_DECL | EVENT_ASSIGN
         */
        unique_ptr<ast::Node> TOP_DECL() {
            switch (top().kind) {
                case TokenKind::kwUse:
                    return USE_DECL();
                case TokenKind::kwGlobal:
                    return VAR_DECL(TokenKind::kwGlobal);
                case TokenKind::kwConst:
                    return VAR_DECL(TokenKind::kwConst);
                case TokenKind::kwFun:
                    return FUN_DECL();
                case TokenKind::kwType:
                    return TYPE_DECL();
                case TokenKind::kwEnum:
                    return ENUM_DECL();
                case TokenKind::Identifier:
                    return EVENT_ASSIGN();
                default:
                    error_ = Token::error(top().loc, STR("Expected top-level declaration, got " << top().kind));
                    return nullptr;
            }
        }

        /** USE_DECL := 'use' ident
         */
        unique_ptr<ast::UseDecl> USE_DECL() {
            Location l = POP(TokenKind::kwUse).loc;
            return std::make_unique<ast::UseDecl>(l, POP(TokenKind::Identifier).asSymbol());
        }

        /** GLOBAL_DECL := 'global' ident [ ':' TYPE ] [ '=' EXPR ]
            CONST_DECL := 'const' ident [ ':' TYPE ] [ '=' EXPR ]
            LET_STMT := ( 'let' | 'const' ) ident [ ':' TYPE ] [ '=' EXPR ]

         */
        unique_ptr<ast::Node> VAR_DECL(TokenKind kind) {
            auto l = POP(kind).loc;
            auto result = std::make_unique<ast::VarDecl>(kind, l, POP(TokenKind::Identifier).asSymbol());
            if (condPop(TokenKind::opColon))
                result->setType(TRY(TYPE()));
            if (condPop(TokenKind::opAssign))
                result->setValue(TRY(EXPR()));
            return result;
        }

        /** FUN_DECL := 'fun' ident [ '.' ident ] '(' LIST(ARG) ')' [ '->' [ 'optional' ] TYPE ] BLOCK
         */
        unique_ptr<ast::FunDecl> FUN_DECL() {
            Token fun = POP(TokenKind::kwFun);
            Token name = POP(TokenKind::Identifier);
            auto result = std::make_unique<ast::FunDecl>(fun.loc, name.asSymbol());
            if (condPop(TokenKind::opDot))
                result->setMethodName(POP(TokenKind::Identifier).asSymbol());
            POP(TokenKind::opParOpen);
            // TODO parse args
            POP(TokenKind::opParClose);
            if (condPop(TokenKind::opArrow)) {
                if (condPop(TokenKind::kwOptional))
                    result->setOptionalReturn(true);
                result->setReturnType(TRY(TYPE()));
            }
            atBlockStart();
            result->setBody(TRY(BLOCK()));
            return result;
        }

        /** TYPE_DECL := 'type' ident [ 'extends' ident ] '=' '(' LIST(FIELD_DECL) ')'
         */
        unique_ptr<ast::TypeDecl> TYPE_DECL() {
            UNIMPLEMENTED;
        }

        /** FIELD_DECL := ident [ ':' TYPE ] [ '=' EXPR ]
                       |= 'constructor' '=' FUN_LITERAL
         */
        unique_ptr<ast::Node> FIELD_DECL() {
            UNIMPLEMENTED;
        }

        /** ENUM_DECL := 'enum' ident '=' ident { '|' ident }
         */
        unique_ptr<ast::EnumDecl> ENUM_DECL() {
            UNIMPLEMENTED;
        }

        /** EVENT_ASSIGN := ident { '.' ident } ( '=' | '+=' ) EXPR
         */
        unique_ptr<ast::Assign> EVENT_ASSIGN() {
            UNIMPLEMENTED;
        }

        /** BLOCK := indent STATEMENT { newline STATEMENT } dedent
         */
        unique_ptr<ast::Block> BLOCK() {
            auto result = std::make_unique<ast::Block>(top().loc);
            POP(TokenKind::Indent);
            result->addChild(TRY(STATEMENT()));
            while (condPop(TokenKind::Newline))
                result->addChild(TRY(STATEMENT()));
            POP(TokenKind::Dedent);
            return result;
        }

        /** LIST(X) := indent LINE(X) { newline LINE(X) } dedent [ newline ]
                     | LINE(X)
            LINE(X) := [ X { ',' X } [ ',' ] ]

            Parses the items of a bracketed list, calling item() to parse each X. The opening bracket is expected to be already popped and the closing bracket is left to the caller. Returns false on error.
         */
        template<typename ITEM>
        bool LIST(ITEM item) {
            UNIMPLEMENTED;
        }

        /** STATEMENT := LET_STMT | IF_STMT | WHILE_STMT | FOR_STMT | MATCH_STMT | RETURN_STMT
                       |= 'break' | 'continue' | 'write' EXPR | 'test' EXPR | EXPR_STMT

            LET_STMT is parsed by VAR_DECL. The if and match expressions can only appear inside parentheses, so a statement starting with `if`, or `match` is always the IF_STMT, or MATCH_STMT.
         */
        unique_ptr<ast::Node> STATEMENT() {
            switch (top().kind) {
                case TokenKind::kwLet:
                case TokenKind::kwConst:
                    return VAR_DECL(top().kind);
                case TokenKind::kwIf:
                    return IF_STMT();
                case TokenKind::kwWhile:
                    return WHILE_STMT();
                case TokenKind::kwFor:
                    return FOR_STMT();
                case TokenKind::kwMatch:
                    return MATCH_STMT();
                case TokenKind::kwReturn:
                    return RETURN_STMT();
                case TokenKind::kwBreak:
                    return std::make_unique<ast::Break>(POP(TokenKind::kwBreak).loc);
                case TokenKind::kwContinue:
                    return std::make_unique<ast::Continue>(POP(TokenKind::kwContinue).loc);
                case TokenKind::kwWrite: {
                    auto result = std::make_unique<ast::Write>(POP(TokenKind::kwWrite).loc);
                    result->setValue(TRY(EXPR()));
                    return result;
                }
                case TokenKind::kwTest: {
                    auto result = std::make_unique<ast::Test>(POP(TokenKind::kwTest).loc);
                    result->setValue(TRY(EXPR()));
                    return result;
                }
                default:
                    return EXPR_STMT();
            }
            UNIMPLEMENTED;
        }

        /** EXPR_STMT := EXPR [ ( '=' | '+=' | '-=' | '*=' | '/=' ) EXPR ]
         */
        unique_ptr<ast::Node> EXPR_STMT() {
            auto result = TRY(EXPR());
            switch (top().kind) {
                case TokenKind::opAssign:
                case TokenKind::opAddAssign:
                case TokenKind::opSubAssign:
                case TokenKind::opMulAssign:
                case TokenKind::opDivAssign: {
                    auto assign = std::make_unique<ast::Assign>(pop());
                    assign->setLeft(std::move(result));
                    assign->setRight(TRY(EXPR()));
                    return assign;
                }
                default:
                    return result;
            }
        }

        /** RETURN_STMT := 'return' [ EXPR ]

            There is no value when the next token is newline, or dedent.
         */
        unique_ptr<ast::Node> RETURN_STMT() {
            auto result = std::make_unique<ast::Return>(POP(TokenKind::kwReturn).loc);
            if (top().kind != TokenKind::Newline && top().kind != TokenKind::Dedent)
                result->setValue(TRY(EXPR()));
            return result;
        }

        /** IF_STMT := 'if' EXPR 'then' BLOCK [ 'else' BLOCK ]

            `else if` chains are the single statement form of BLOCK. A line starting with `else` produces no newline, so the else follows the dedent of the block directly.
         */
        unique_ptr<ast::Node> IF_STMT() {
            auto result = std::make_unique<ast::If>(POP(TokenKind::kwIf).loc);
            result->setCondition(TRY(EXPR()));
            POP(TokenKind::kwThen);
            atBlockStart();
            result->setThenCase(TRY(BLOCK()));
            if (condPop(TokenKind::kwElse)) {
                atBlockStart();
                result->setElseCase(TRY(BLOCK()));
            }
            return result;
        }

        /** WHILE_STMT := 'while' EXPR 'do' BLOCK
         */
        unique_ptr<ast::Node> WHILE_STMT() {
            UNIMPLEMENTED;
        }

        /** FOR_STMT := 'for' ident [ ',' ident ] 'in' EXPR [ 'step' EXPR ] 'do' BLOCK
         */
        unique_ptr<ast::Node> FOR_STMT() {
            UNIMPLEMENTED;
        }

        /** MATCH_STMT := 'match' EXPR 'with' indent CASE_STMT { newline CASE_STMT } [ 'else' BLOCK ] dedent

            A line starting with `else` produces no newline, so the else case follows the last case directly.
         */
        unique_ptr<ast::Node> MATCH_STMT() {
            UNIMPLEMENTED;
        }

        /** CASE_STMT := 'is' PATTERN 'then' BLOCK
         */
        unique_ptr<ast::Node> CASE_STMT() {
            UNIMPLEMENTED;
        }

        /** PATTERN := LITERAL | '-' number | ident { '.' ident }
                     |= '(' LIST(ident [ '=' PATTERN ]) ')' | '[' LIST(PATTERN) ']'
         */
        unique_ptr<ast::Node> PATTERN() {
            UNIMPLEMENTED;
        }

        /** TYPE := POD_TYPE | ARRAY_TYPE | TUPLE_TYPE | MAP_TYPE | FUN_TYPE | EVENT_TYPE
         */
        unique_ptr<ast::Node> TYPE() {
            switch (top().kind) {
                case TokenKind::opSquareOpen:
                    return ARRAY_TYPE();
                case TokenKind::opParOpen:
                    return TUPLE_TYPE();
                case TokenKind::opCurlyOpen:
                    return MAP_TYPE();
                case TokenKind::kwFun:
                    return FUN_TYPE();
                case TokenKind::kwEvent:
                    return EVENT_TYPE();
                default:
                    return POD_TYPE();
            }
        }

        /** POD_TYPE := 'num' | 'bool' | 'text' | ident
         */
        unique_ptr<ast::SymbolName> POD_TYPE() {
            Location l = top().loc;
            switch (top().kind) {
                case TokenKind::kwNum:
                case TokenKind::kwBool:
                case TokenKind::kwText:
                case TokenKind::Identifier:
                    return std::make_unique<ast::SymbolName>(l, pop().asSymbol());
                default:
                    error_ = Token::error(l, STR("Expected POD type, got " << top().kind));
                    return nullptr;
            }
        }

        /** ARRAY_TYPE := '[' TYPE ']'
         */
        unique_ptr<ast::ArrayType> ARRAY_TYPE() {
            auto result = std::make_unique<ast::ArrayType>(POP(TokenKind::opSquareOpen).loc);
            result->setElementType(TRY(TYPE()));
            POP(TokenKind::opSquareClose);
            return result;
        }

        /** TUPLE_TYPE := '(' LIST(FIELD_TYPE) ')'
         */
        unique_ptr<ast::TupleType> TUPLE_TYPE() {
            UNIMPLEMENTED;
        }

        /** FIELD_TYPE := ident [ ':' TYPE ]
         */
        unique_ptr<ast::Node> FIELD_TYPE() {
            UNIMPLEMENTED;
        }

        /** MAP_TYPE := '{' POD_TYPE '->' TYPE '}'
         */
        unique_ptr<ast::MapType> MAP_TYPE() {
            auto result = std::make_unique<ast::MapType>(POP(TokenKind::opCurlyOpen).loc);
            result->setKeyType(TRY(POD_TYPE()));
            POP(TokenKind::opArrow);
            result->setValueType(TRY(TYPE()));
            POP(TokenKind::opCurlyClose);
            return result;
        }

        /** FUN_TYPE := 'fun' '(' LIST(ARG) ')' [ '->' [ 'optional' ] TYPE ]
         */
        unique_ptr<ast::FunType> FUN_TYPE() {
            UNIMPLEMENTED;
        }

        /** ARG := ident ':' TYPE [ '=' EXPR ]
         */
        unique_ptr<ast::Node> ARG() {
            UNIMPLEMENTED;
        }

        /** EVENT_TYPE := 'event' '(' LIST(ARG) ')'
         */
        unique_ptr<ast::EventType> EVENT_TYPE() {
            UNIMPLEMENTED;
        }

        /** EXPR := OR_EXPR

            The if and match expressions are not EXPR, they can only appear inside parentheses (see VALUE).
         */
        unique_ptr<ast::Node> EXPR() {
            return OR_EXPR();
        }

        /** OR_EXPR := AND_EXPR { 'or' AND_EXPR }
         */
        unique_ptr<ast::Node> OR_EXPR() {
            auto result = TRY(AND_EXPR());
            while (top() == TokenKind::kwOr) {
                auto orExpr = std::make_unique<ast::BinaryOp>(pop());
                orExpr->setLeft(std::move(result));
                orExpr->setRight(TRY(AND_EXPR()));
                result = std::move(orExpr);
            }
            return result;
        }

        /** AND_EXPR := NOT_EXPR { 'and' NOT_EXPR }
         */
        unique_ptr<ast::Node> AND_EXPR() {
            auto result = TRY(NOT_EXPR());
            while (top() == TokenKind::kwAnd) {
                auto orExpr = std::make_unique<ast::BinaryOp>(pop());
                orExpr->setLeft(std::move(result));
                orExpr->setRight(TRY(NOT_EXPR()));
                result = std::move(orExpr);
            }
            return result;
        }

        /** NOT_EXPR := 'not' NOT_EXPR | EQ_EXPR
         */
        unique_ptr<ast::Node> NOT_EXPR() {
            if (top() == TokenKind::kwNot) {
                auto notExpr = std::make_unique<ast::UnaryOp>(pop());
                notExpr->setOperand(TRY(NOT_EXPR()));
                return notExpr;
            } else {
                return EQ_EXPR();
            }
        }

        /** EQ_EXPR := CMP_EXPR { '==' CMP_EXPR }
                     |= CMP_EXPR { '!=' CMP_EXPR }

            Mixing == and != in a single chain is an error.
         */
        unique_ptr<ast::Node> EQ_EXPR() {
            auto result = TRY(CMP_EXPR());
            if (top() == TokenKind::opEquals) {
                do {
                    auto eqExpr = std::make_unique<ast::BinaryOp>(pop());
                    eqExpr->setLeft(std::move(result));
                    eqExpr->setRight(TRY(CMP_EXPR()));
                    result = std::move(eqExpr);
                } while (top() == TokenKind::opEquals);
            } else if (top() == TokenKind::opNotEquals) {
                do {
                    auto neqExpr = std::make_unique<ast::BinaryOp>(pop());
                    neqExpr->setLeft(std::move(result));
                    neqExpr->setRight(TRY(CMP_EXPR()));
                    result = std::move(neqExpr);
                } while (top() == TokenKind::opNotEquals);
            } else {
                return result;
            }
            return result;
        }

        /** CMP_EXPR := ADD_EXPR { ( '<' | '<=' | '>' | '>=' ) ADD_EXPR }
         */
        unique_ptr<ast::Node> CMP_EXPR() {
            auto result = TRY(ADD_EXPR());
            while (true) {
                switch (top().kind) {
                    case TokenKind::opLess:
                    case TokenKind::opLessEquals:
                    case TokenKind::opGreater:
                    case TokenKind::opGreaterEquals: {
                        auto cmpExpr = std::make_unique<ast::BinaryOp>(pop());
                        cmpExpr->setLeft(std::move(result));
                        cmpExpr->setRight(TRY(ADD_EXPR()));
                        result = std::move(cmpExpr);
                        break;
                    }
                    default:
                        return result;
                }
            }
        }

        /** ADD_EXPR := MUL_EXPR { ( '+' | '-' ) MUL_EXPR }
         */
        unique_ptr<ast::Node> ADD_EXPR() {
            auto result = TRY(MUL_EXPR());
            while (true) {
                switch (top().kind) {
                    case TokenKind::opAdd:
                    case TokenKind::opSub: { 
                        auto cmpExpr = std::make_unique<ast::BinaryOp>(pop());
                        cmpExpr->setLeft(std::move(result));
                        cmpExpr->setRight(TRY(MUL_EXPR()));
                        result = std::move(cmpExpr);
                        break;
                    }
                    default:
                        return result;
                }
            }
        }

        /** MUL_EXPR := UNARY_EXPR { ( '*' | '/' ) UNARY_EXPR }
         */
        unique_ptr<ast::Node> MUL_EXPR() {
           auto result = TRY(UNARY_EXPR());
            while (true) {
                switch (top().kind) {
                    case TokenKind::opMul:
                    case TokenKind::opDiv: { 
                        auto cmpExpr = std::make_unique<ast::BinaryOp>(pop());
                        cmpExpr->setLeft(std::move(result));
                        cmpExpr->setRight(TRY(UNARY_EXPR()));
                        result = std::move(cmpExpr);
                        break;
                    }
                    default:
                        return result;
                }
            }
         }

        /** UNARY_EXPR := ( '-' | 'clone' ) UNARY_EXPR | POSTFIX_EXPR
         */
        unique_ptr<ast::Node> UNARY_EXPR() {
            switch (top().kind) {
                case TokenKind::opSub:
                case TokenKind::kwClone: {
                    auto unaryExpr = std::make_unique<ast::UnaryOp>(pop());
                    unaryExpr->setOperand(TRY(UNARY_EXPR()));
                    return unaryExpr;
                }
                default:
                    return POSTFIX_EXPR();
            }
        }

        /** POSTFIX_EXPR := PRIMARY { '.' ident | '[' EXPR ']' | PAREN_EXPR [ 'orelse' ORELSE_RHS ] }

            orelse can only follow a call. Its right hand side takes everything, so the postfix expression always ends with it. That the call returns an optional value is a semantic check.
         */
        unique_ptr<ast::Node> POSTFIX_EXPR() {
            auto result = TRY(PRIMARY());
            while (true) {
                switch (top().kind) {
                    case TokenKind::opDot:
                    case TokenKind::opSquareOpen:
                    case TokenKind::opParOpen:
                        UNIMPLEMENTED;
                    default:
                        return result;
                }
            }
        }

        /** ORELSE_RHS := EXPR | 'return' [ EXPR ] | 'break' | 'continue'

            Takes everything, i.e. `foo() orelse 4 + 2` is `foo() orelse (4 + 2)`.

            No value for return when the next token is newline, dedent, or ends the enclosing list.
         */
        unique_ptr<ast::Node> ORELSE_RHS() {
            UNIMPLEMENTED;
        }

        /** PRIMARY := LITERAL | ident | 'self' | 'none' | PAREN_EXPR
            LITERAL := number | string | 'true' | 'false'
                    |= ARRAY_LITERAL | MAP_LITERAL | FUN_LITERAL
         */
        unique_ptr<ast::Node> PRIMARY() {
            switch (top().kind) {
                case TokenKind::Number:
                    return std::make_unique<ast::Number>(pop());
                case TokenKind::Text:
                    return std::make_unique<ast::Text>(pop());
                case TokenKind::kwTrue:
                case TokenKind::kwFalse:
                case TokenKind::kwNone:
                case TokenKind::kwSelf:
                case TokenKind::Identifier:
                    return std::make_unique<ast::SymbolName>(pop());
                case TokenKind::opParOpen:
                    return PAREN_EXPR();
                default:
                    error_ = Token::error(top().loc, STR("Expected primary expression, got " << top().kind));
                    return nullptr;
            }
            UNIMPLEMENTED;
        }

        /** PAREN_EXPR := '(' LIST(NAMED_VALUE) ')'

            Grouping, tuple literal, or call arguments, differentiated after parsing.
         */
        unique_ptr<ast::Node> PAREN_EXPR() {
            UNIMPLEMENTED;
        }

        /** NAMED_VALUE := VALUE [ '=' VALUE ]
         */
        unique_ptr<ast::Node> NAMED_VALUE() {
            UNIMPLEMENTED;
        }

        /** VALUE := IF_EXPR | MATCH_EXPR | EXPR

            The only place where the if and match expressions can appear.
         */
        unique_ptr<ast::Node> VALUE() {
            UNIMPLEMENTED;
        }

        /** IF_EXPR := 'if' EXPR 'then' EXPR 'else' ( IF_EXPR | EXPR )

            Cases are expressions only. The block start flag must be cleared after `then` and `else`, so that line breaks inside are continuations.
         */
        unique_ptr<ast::Node> IF_EXPR() {
            UNIMPLEMENTED;
        }

        /** MATCH_EXPR := 'match' EXPR 'with' CASE_EXPR { CASE_EXPR } [ 'else' EXPR ]

            Cases are expressions only. The block start flag must be cleared after `with`, `then` and `else`, so that line breaks inside are continuations. Exhaustiveness (or the else case) is a semantic check.
         */
        unique_ptr<ast::Node> MATCH_EXPR() {
            UNIMPLEMENTED;
        }

        /** CASE_EXPR := 'is' PATTERN 'then' EXPR
         */
        unique_ptr<ast::Node> CASE_EXPR() {
            UNIMPLEMENTED;
        }

        /** ARRAY_LITERAL := '[' LIST(EXPR) ']'
         */
        unique_ptr<ast::Node> ARRAY_LITERAL() {
            UNIMPLEMENTED;
        }

        /** MAP_LITERAL := '{' LIST(MAP_ITEM) '}'
         */
        unique_ptr<ast::Node> MAP_LITERAL() {
            UNIMPLEMENTED;
        }

        /** MAP_ITEM := EXPR '->' EXPR
         */
        unique_ptr<ast::Node> MAP_ITEM() {
            UNIMPLEMENTED;
        }

        /** FUN_LITERAL := 'fun' [ '(' LIST(LAMBDA_ARG) ')' [ '->' [ 'optional' ] TYPE ] ] BLOCK
         */
        unique_ptr<ast::Node> FUN_LITERAL() {
            UNIMPLEMENTED;
        }

        /** LAMBDA_ARG := ident [ ':' TYPE ] [ '=' EXPR ] | LITERAL
         */
        unique_ptr<ast::Node> LAMBDA_ARG() {
            UNIMPLEMENTED;
        }

#undef POP
#undef TRY

        /** \name Token handling and newline translation

            The parser provides the basic eof(), top(), pop() and condPop() interface for querying the tokenized sources. For the doLang this is complicated by the off side rule, which means that we also need to translate the newline tokens emited by the lexer into indent/dedent/newline variants, or swallow them.
            
            This translation is relatively simple though: 

            - every block of statements has its *indentation*, the column at which all its statements start. A line starting exactly at this column starts a new statement
            - a line indented *more* continues the current statement
            - except when the line follows a keyword that starts a block (`then`, `else`, `do`, `with`), the `fun` keyword, or a function signature. Then the more indented line starts a new block whose indentation is the column of the line
            - a line indented *less* ends the block (or blocks) and must start exactly at the indentation of one of the enclosing blocks

            We also need the ability to tell the parser that we are at the beginning of a block, which means that the next newline will be converted to an indent instead of swallowed. This is done by calling atBlockStart(). But when we do, there could have already been a newline token analyzed and ignored (because we were *not* at the beginning of a block). To fight this, when a newline is swallowed, we keep that newline in pending_. 

            When atBlockStart() is called, we check pending_. If it is a newline, we need to recalculate the next token and we swap the pending and next tokens. This means next top will process the newline into the indent. When we pop the indent, the token in pending_ is moved back into next_ so that the next top will process it as the next token after the newline.

            A line at the current indentation is a newline, unless it starts with `else`, which can never start a statement and therefore continues the if statement (or the match statement). To tell, we read the token after the newline into pending_. If it is `else`, it replaces the newline in next_, otherwise the newline is returned and the token in pending_ is moved to next_ when the newline is popped.

            So pending_ is either empty (EndOfFile), a swallowed newline (and next_ is the token after it), or the token that follows the newline (or indent) in next_.
         */
        //@{

        bool atBlockStart_ = false;

        Token next_; 
        Token pending_;

        std::vector<int32_t> indentStack_;

        bool eof() {
            return top() == TokenKind::EndOfFile;
        }

        Token top() {
            if (next_.kind == TokenKind::EndOfFile)
                    next_ = lexer_.next();
            while (next_.kind == TokenKind::Newline) {
                // if the indentation of the newline is the same as the current indentation, we emit it as it is the end of statement, unless the next line starts with else, which continues the if (or match) statement
                if (next_.payload == indentStack_.back()) {
                    if (pending_ == TokenKind::EndOfFile)
                        pending_ = lexer_.next();
                    if (pending_ == TokenKind::kwElse) {
                        next_ = pending_;
                        pending_.kind = TokenKind::EndOfFile;
                    }
                    return next_;
                // if the indentation is greater than the current indentation, we are either converting the newline to indent if we are at the beginning of a block expecting statement, or we are swallowing the newline as it is a continuation of the previous statement
                } else if (next_.payload > indentStack_.back()) {
                    if (atBlockStart_) {
                        next_.kind = TokenKind::Indent;
                        return next_;
                    } else {
                        // swallow the newline
                        pending_ = next_;
                        next_ = lexer_.next();
                        continue;
                    } 
                // if the the indentation is less than the current indentation, we are converting it to dedents. The issue is that multiple dedents can be generated for a single newline, so we need to manufacture the enough number of dedents
                } else if (next_.payload < indentStack_.back()) {
                    // first check we can actually dedent
                    if (indentStack_.size() == 1) {
                        // we are at the top level, so this is an error
                        next_ = Token::error(next_.loc, "Unexpected dedent");
                        return next_;
                    }
                    Token result{next_};
                    result.kind = TokenKind::Dedent;
                    result.payload =  indentStack_[indentStack_.size() - 2];
                    // if the level to which we are dedenting will be smaller than the newline indentation, its an error
                    if (next_.payload > result.payload) {
                        next_ = Token::error(next_.loc, "Unexpected dedent");
                        return next_;
                    }
                    return result;
                }
            }
            // if next token is end of file *and* there are still indents on the stack, we need to manufacture dedents until we reach the top level
            if (next_.kind == TokenKind::EndOfFile && indentStack_.size() > 1) {
                Token result{next_};
                result.kind = TokenKind::Dedent;
                result.payload =  indentStack_[indentStack_.size() - 2];
                return result;
            }
            // non-newline token, just return it
            return next_;
        }

        Token pop() {
            Token x = top();
            atBlockStart_ = false;
            switch (x.kind) {
                case TokenKind::Indent:
                    indentStack_.push_back(x.payload);
                    break;
                case TokenKind::Dedent:
                    indentStack_.pop_back();
                    // return immediately so that we dn't consume the next token as there might be another dedent coming from it
                    return x;
                    break;
                case TokenKind::opParOpen:
                    // TODO I am not sure if we want this - means after every open paren, including inside an expression we will start with statements and indents and dedents. Not sure I like that, but at this point not sure what to do with it
                case TokenKind::opCurlyOpen:
                case TokenKind::opSquareOpen:
                case TokenKind::kwThen:
                case TokenKind::kwElse:
                case TokenKind::kwDo:
                case TokenKind::kwWith:
                    atBlockStart_ = true;
                    break;
                default:
                    break;
            }
            // if we have popped newline (or indent) and have the token after it in pending, it becomes the next token, otherwise next top will reload fresh
            if ((x == TokenKind::Indent || x == TokenKind::Newline) && pending_ != TokenKind::EndOfFile)
                next_ = pending_;
            else
                next_.kind = TokenKind::EndOfFile;
            pending_.kind = TokenKind::EndOfFile;
            return x;
        }

        bool condPop(TokenKind kind) {
            if (top() == kind) {
                pop();
                return true;
            } else {
                return false;
            }
        }

        void atBlockStart() {
            atBlockStart_ = true;
            if (pending_ == TokenKind::Newline) {
                // swap the pending and next tokens so that next top will process the newline into indent
                std::swap(pending_, next_);
            }
        }
        //@}
      
        Lexer lexer_;

        Token error_;

    }; // dolang::Parser

} // namespace rckid::dolang