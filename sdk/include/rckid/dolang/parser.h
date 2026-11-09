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
            // nothing but the end of file can follow the last declaration
            if (! eof()) {
                error_ = Token::error(top().loc, STR("Expected end of line, got " << top().kind));
                return nullptr;
            }
            return result;
        }

        /** TOP_DECL := USE_DECL | GLOBAL_DECL | CONST_DECL | FUN_DECL | TYPE_DECL | ENUM_DECL | EXPR_STMT

            Anything that does not start with a declaration keyword is parsed as expression statement. Only event and function variable assignments are valid at the top level, but that is a semantic check.
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
                default:
                    return EXPR_STMT();
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

        /** FUN_DECL := 'fun' ident [ '.' ident ] SIGNATURE(PARAM) BLOCK
         */
        unique_ptr<ast::NamedFunction> FUN_DECL() {
            Token fun = POP(TokenKind::kwFun);
            Token name = POP(TokenKind::Identifier);
            auto result = std::make_unique<ast::NamedFunction>(fun.loc, name.asSymbol());
            if (condPop(TokenKind::opDot))
                result->setMethodName(POP(TokenKind::Identifier).asSymbol());
            if (! SIGNATURE(result.get(), &Parser::PARAM))
                return nullptr;
            result->setBody(TRY(BLOCK()));
            return result;
        }

        /** SIGNATURE(X) := ARGS(X) [ '->' [ 'optional' ] TYPE ]

            Parses the arguments and the optional return type into the given function type (or function). Returns the function type on success and nullptr on error, so callers must check the result explicitly, same as with LIST.
         */
        ast::FunType * SIGNATURE(ast::FunType * f, unique_ptr<ast::Node> (Parser::*item)()) {
            f->setArgs(TRY(ARGS(item)));
            if (condPop(TokenKind::opArrow)) {
                if (condPop(TokenKind::kwOptional))
                    f->setOptionalReturn(true);
                f->setReturnType(TRY(TYPE()));
            }
            return f;
        }

        /** ARGS(X) := '(' LIST(X) ')'

            Arguments of named functions, function and event types and function literals. They are all lists of arguments, only the item rule (X) differs. 
         */
        unique_ptr<ast::List> ARGS(unique_ptr<ast::Node> (Parser::*item)()) {
            auto result = std::make_unique<ast::List>(POP(TokenKind::opParOpen).loc);
            if (! LIST(result.get(), TokenKind::opParClose, item))
                return nullptr;
            POP(TokenKind::opParClose);
            return result;
        }

        /** PARAM := ident ':' TYPE [ '=' EXPR ]

            Argument of named functions, function types and event types. The type is mandatory, the default value is optional.
         */
        unique_ptr<ast::Node> PARAM() {
            auto result = std::make_unique<ast::Arg>(POP(TokenKind::Identifier));
            POP(TokenKind::opColon);
            result->setType(TRY(TYPE()));
            if (condPop(TokenKind::opAssign))
                result->setValue(TRY(EXPR()));
            return result;
        }

        /** LAMBDA_ARG := ident [ ':' TYPE ] [ 'is' PATTERN ]

            Argument of function literals. The type is optional (taken from the expected type when missing), there is no default value. The pattern is only valid for event handlers, which is a semantic check.
         */
        unique_ptr<ast::Node> LAMBDA_ARG() {
            auto result = std::make_unique<ast::Arg>(POP(TokenKind::Identifier));
            if (condPop(TokenKind::opColon))
                result->setType(TRY(TYPE()));
            if (condPop(TokenKind::kwIs))
                result->setPattern(TRY(PATTERN()));
            return result;
        }

        /** TYPE_DECL := 'type' ident [ 'extends' ident ] '=' '(' LIST(FIELD_DECL) ')'
         */
        unique_ptr<ast::TypeDecl> TYPE_DECL() {
            Location l = POP(TokenKind::kwType).loc;
            auto result = std::make_unique<ast::TypeDecl>(l, POP(TokenKind::Identifier).asSymbol());
            if (condPop(TokenKind::kwExtends))
                result->setBase(POP(TokenKind::Identifier).asSymbol());
            POP(TokenKind::opAssign);
            auto fields = std::make_unique<ast::List>(POP(TokenKind::opParOpen).loc);
            if (! LIST(fields.get(), TokenKind::opParClose, &Parser::FIELD_DECL))
                return nullptr;
            POP(TokenKind::opParClose);
            result->setFields(std::move(fields));
            return result;
        }

        /** FIELD_DECL := ident [ ':' TYPE ] [ '=' EXPR ]
                       |= 'constructor' '=' FUN_LITERAL

            Unlike function arguments, the field type is optional when it can be inferred from the default value. The constructor is stored as an Arg named `constructor` with the function literal as its value.
         */
        unique_ptr<ast::Node> FIELD_DECL() {
            if (top() == TokenKind::kwConstructor) {
                auto result = std::make_unique<ast::Arg>(pop());
                POP(TokenKind::opAssign);
                result->setValue(TRY(FUN_LITERAL()));
                return result;
            }
            auto result = std::make_unique<ast::Arg>(POP(TokenKind::Identifier));
            if (condPop(TokenKind::opColon))
                result->setType(TRY(TYPE()));
            if (condPop(TokenKind::opAssign))
                result->setValue(TRY(EXPR()));
            return result;
        }

        /** ENUM_DECL := 'enum' ident '=' ident { '|' ident }
         */
        unique_ptr<ast::EnumDecl> ENUM_DECL() {
            auto result = std::make_unique<ast::EnumDecl>(POP(TokenKind::kwEnum).loc);
            result->name = POP(TokenKind::Identifier).asSymbol();
            POP(TokenKind::opAssign);
            result->addValue(POP(TokenKind::Identifier).asSymbol());
            while (condPop(TokenKind::opPipe))
                result->addValue(POP(TokenKind::Identifier).asSymbol());
            return result;
        }

        /** BLOCK := indent STATEMENT { newline STATEMENT } dedent

            Sets the block start flag itself, so that the next line indented more is the indent of the block. This must happen before the first top() call as that is where the newline is translated.

            Blocks are not allowed inside single line lists (layoutAllowed_ is false there).
         */
        unique_ptr<ast::Block> BLOCK() {
            if (! layoutAllowed_) {
                error_ = Token::error(top().loc, "Function body is only allowed in a multi-line list");
                return nullptr;
            }
            atBlockStart();
            auto result = std::make_unique<ast::Block>(top().loc);
            POP(TokenKind::Indent);
            result->addChild(TRY(STATEMENT()));
            while (condPop(TokenKind::Newline))
                result->addChild(TRY(STATEMENT()));
            POP(TokenKind::Dedent);
            return result;
        }

        /** LIST(X) := [ X { ',' X } [ ',' ] ]
                     |= indent LINE(X) { [ ',' ] newline LINE(X) } [ ',' ] dedent
            LINE(X) := X { ',' X }

            Parses the items of a bracketed list into the given list node (either plain ast::List, or a node inheriting from it), calling the item parser method to parse each X, and remembers the form used in the list. Returns true on success, false on error (with error_ set), so callers must check the result explicitly instead of using TRY. The opening bracket is expected to be already popped and the closing bracket (close) is left to the caller, but the single line form must know it to tell when the items end.

            The form is decided right after the opening bracket: if the next token is indent, the list is multi-line, otherwise single line. 
            
            Single line form: items separated by commas, optional trailing comma. The list is a single logical line, so no layout is allowed inside (blocks and nested multi-line lists are errors) and all other line breaks are continuations. The closing bracket on its own line at the current indentation produces no newline (see top()).
            
            Multi-line form: block rules, items on the same line separated by commas, at the end of the line the comma is optional. After an item and optional comma, newline starts the next line of items, dedent ends the list and anything else is the next item on the same line, which requires the comma. Blocks are allowed in the items, but as the block ends with a line break, no comma can follow it. The closing bracket follows the dedent directly (see top()).
         */
        bool LIST(ast::List * list, TokenKind close, unique_ptr<ast::Node> (Parser::*item)()) {
            // always check for the multi-line form, so that we can report it as an error inside single line lists instead of silently treating the line break as a continuation
            atBlockStart();
            if (top() == TokenKind::Indent) {
                if (! layoutAllowed_) {
                    error_ = Token::error(top().loc, "Multi-line list is not allowed inside a single line list");
                    return false;
                }
                list->multiLine = true;
                pop();
                while (true) {
                    auto x = (this->*item)();
                    if (x == nullptr)
                        return false;
                    list->addChild(std::move(x));
                    bool comma = condPop(TokenKind::opComma);
                    if (condPop(TokenKind::Newline))
                        continue;
                    if (condPop(TokenKind::Dedent))
                        return true;
                    if (! comma) {
                        error_ = Token::error(top().loc, STR("Expected , or new line, got " << top().kind));
                        return false;
                    }
                }
            } else {
                LayoutGuard g_{*this, false};
                if (top() == close)
                    return true;
                while (true) {
                    auto x = (this->*item)();
                    if (x == nullptr)
                        return false;
                    list->addChild(std::move(x));
                    if (! condPop(TokenKind::opComma))
                        return true;
                    if (top() == close)
                        return true;
                }
            }
        }

        /** STATEMENT := LET_STMT | IF_STMT | WHILE_STMT | FOR_STMT | MATCH_STMT | RETURN_STMT
                       |= 'break' | 'continue' | 'write' EXPR | 'test' EXPR | EXPR_STMT

            LET_STMT is parsed by VAR_DECL. A statement starting with `if`, or `match` is always the IF_STMT, or MATCH_STMT. The IF_EXPR and MATCH_EXPR forms are only parsed when they appear inside an expression.
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
            result->setThenCase(TRY(BLOCK()));
            if (condPop(TokenKind::kwElse)) {
                result->setElseCase(TRY(BLOCK()));
            }
            return result;
        }
        
        /** IF_EXPR := 'if' EXPR 'then' EXPR 'else' EXPR

            Cases are expressions only. As the block start flag is not set, line breaks inside are continuations.
         */
        unique_ptr<ast::Node> IF_EXPR() {
            auto result = std::make_unique<ast::If>(POP(TokenKind::kwIf).loc);
            result->setCondition(TRY(EXPR()));
            POP(TokenKind::kwThen);
            result->setThenCase(TRY(EXPR()));
            POP(TokenKind::kwElse);
            result->setElseCase(TRY(EXPR()));
            return result;
        }

        /** WHILE_STMT := 'while' EXPR 'do' BLOCK
         */
        unique_ptr<ast::Node> WHILE_STMT() {
            auto result = std::make_unique<ast::While>(POP(TokenKind::kwWhile).loc);
            result->setCondition(TRY(EXPR()));
            POP(TokenKind::kwDo);
            result->setBody(TRY(BLOCK()));
            return result;
        }

        /** FOR_STMT := 'for' ident [ ',' ident ] 'in' EXPR [ 'step' EXPR ] 'do' BLOCK
         */
        unique_ptr<ast::Node> FOR_STMT() {
            auto result = std::make_unique<ast::For>(POP(TokenKind::kwFor).loc);
            result->iter = POP(TokenKind::Identifier).asSymbol();
            if (condPop(TokenKind::opComma))
                result->iter2 = POP(TokenKind::Identifier).asSymbol();
            POP(TokenKind::kwIn);
            result->setContainer(TRY(EXPR()));
            if (condPop(TokenKind::kwStep))
                result->setStep(TRY(EXPR()));
            POP(TokenKind::kwDo);
            result->setBody(TRY(BLOCK()));
            return result;
        }

        /** MATCH_STMT := 'match' EXPR 'when' indent MATCH_STMT_CASES dedent
            MATCH_STMT_CASES := CASE_STMT { newline CASE_STMT } [ 'else' BLOCK ]
                             |= 'else' BLOCK

            There must be at least one case, which can be the else case. A line starting with `else` produces no newline, so the else case follows the last case directly.
         */
        unique_ptr<ast::Node> MATCH_STMT() {
            auto result = std::make_unique<ast::Match>(POP(TokenKind::kwMatch).loc);
            result->setValue(TRY(EXPR()));
            POP(TokenKind::kwWhen);
            atBlockStart();
            POP(TokenKind::Indent);
            if (top() != TokenKind::kwElse) {
                result->addCase(TRY(CASE_STMT()));
                while (condPop(TokenKind::Newline))
                    result->addCase(TRY(CASE_STMT()));
            }
            if (condPop(TokenKind::kwElse)) {
                result->setElseCase(TRY(BLOCK()));
            }
            POP(TokenKind::Dedent);
            return result;
        }

        /** MATCH_EXPR := 'match' EXPR 'when' MATCH_EXPR_CASES
            MATCH_EXPR_CASES := CASE_EXPR { CASE_EXPR } [ 'else' EXPR ]
                             |= 'else' EXPR
            CASE_EXPR := 'is' PATTERN 'then' OR_EXPR

            There must be at least one case, which can be the else case. Cases are expressions only. As the block start flag is not set, line breaks inside are continuations. Exhaustiveness (or the else case) is a semantic check.

            The case values are OR_EXPR, not EXPR, so that they cannot contain the `is` operator (unless in parentheses) and the next `is` therefore always starts the next case. The else value is last, so it can be any EXPR.
         */
        unique_ptr<ast::Node> MATCH_EXPR() {
            auto result = std::make_unique<ast::Match>(POP(TokenKind::kwMatch).loc);
            result->setValue(TRY(EXPR()));
            POP(TokenKind::kwWhen);
            if (top() != TokenKind::kwElse) {
                do {
                    auto matchCase = std::make_unique<ast::MatchCase>(POP(TokenKind::kwIs).loc);
                    matchCase->setPattern(TRY(PATTERN()));
                    POP(TokenKind::kwThen);
                    matchCase->setBody(TRY(OR_EXPR()));
                    result->addCase(std::move(matchCase));
                } while (top() == TokenKind::kwIs);
            }
            if (condPop(TokenKind::kwElse))
                result->setElseCase(TRY(EXPR()));
            return result;
        }

        /** CASE_STMT := 'is' PATTERN 'then' BLOCK
         */
        unique_ptr<ast::Node> CASE_STMT() {
            auto result = std::make_unique<ast::MatchCase>(POP(TokenKind::kwIs).loc);
            result->setPattern(TRY(PATTERN()));
            POP(TokenKind::kwThen);
            result->setBody(TRY(BLOCK()));
            return result;
        }

        /** PATTERN := ident
           
           Old ones:
           
           LITERAL | '-' number | ident { '.' ident }
                     |= '(' LIST(ident [ '=' PATTERN ]) ')' | '[' LIST(PATTERN) ']'
         */
        unique_ptr<ast::Node> PATTERN() {
            // TODO implement proper pattern matching and parsing, for now we just assume a name in the pattern which as a name of a type is always valid
            return std::make_unique<ast::SymbolName>(POP(TokenKind::Identifier));
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
            auto result = std::make_unique<ast::TupleType>(POP(TokenKind::opParOpen).loc);
            if (! LIST(result.get(), TokenKind::opParClose, &Parser::FIELD_TYPE))
                return nullptr;
            POP(TokenKind::opParClose);
            return result;
        }

        /** FIELD_TYPE := ident [ ':' TYPE ]
         */
        unique_ptr<ast::Node> FIELD_TYPE() {
            auto result = std::make_unique<ast::FieldType>(POP(TokenKind::Identifier));
            if (condPop(TokenKind::opColon))
                result->setType(TRY(TYPE()));
            return result;
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

        /** FUN_TYPE := 'fun' SIGNATURE(PARAM)
         */
        unique_ptr<ast::FunType> FUN_TYPE() {
            auto result = std::make_unique<ast::FunType>(POP(TokenKind::kwFun).loc);
            if (! SIGNATURE(result.get(), &Parser::PARAM))
                return nullptr;
            return result;
        }

        /** EVENT_TYPE := 'event' ARGS(PARAM)

            Events never return values, so there is no return type.
         */
        unique_ptr<ast::EventType> EVENT_TYPE() {
            auto result = std::make_unique<ast::EventType>(POP(TokenKind::kwEvent).loc);
            result->setArgs(TRY(ARGS(&Parser::PARAM)));
            return result;
        }

        /** EXPR := OR_EXPR [ 'is' [ 'not' ] PATTERN ]

            The `is` operator has the lowest precedence and is non-associative, so it can only appear at the top of an expression, or inside parentheses. It evaluates to bool. When it is the whole condition of an if, or while, the pattern may also bind values visible in the then case, or the loop body, everywhere else (and in the negated form) binding patterns are a semantic error.

            The if, match, return, break and continue expressions are parsed as PRIMARY.
         */
        unique_ptr<ast::Node> EXPR() {
            auto result = TRY(OR_EXPR());
            if (top() != TokenKind::kwIs)
                return result;
            auto isExpr = std::make_unique<ast::Is>(pop());
            isExpr->setValue(std::move(result));
            if (condPop(TokenKind::kwNot))
                isExpr->setNegated(true);
            isExpr->setPattern(TRY(PATTERN()));
            return isExpr;
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

        /** POSTFIX_EXPR := PRIMARY { '.' ident | '[' EXPR ']' | CALL_EXPR [ 'orelse' EXPR ] }
            CALL_EXPR := '(' LIST(CALL_ARG) ')'

            orelse can only follow a call. Its right hand side takes everything, so the postfix expression always ends with it. That the call returns an optional value is a semantic check.
         */
        unique_ptr<ast::Node> POSTFIX_EXPR() {
            auto result = TRY(PRIMARY());
            while (true) {
                switch (top().kind) {
                    case TokenKind::opDot:
                    {
                        Location loc = pop().loc;
                        Symbol name = POP(TokenKind::Identifier).asSymbol();
                        result = std::make_unique<ast::Member>(loc, std::move(result), name);
                        break;
                    }
                    case TokenKind::opSquareOpen: {
                        auto index = std::make_unique<ast::BinaryOp>(pop(), std::move(result));
                        index->setRight(TRY(EXPR()));
                        POP(TokenKind::opSquareClose);
                        result = std::move(index);
                        break;
                    }
                    case TokenKind::opParOpen: {
                        auto call = std::make_unique<ast::Call>(pop().loc, std::move(result));
                        if (! LIST(call.get(), TokenKind::opParClose, &Parser::CALL_ARG))
                            return nullptr;
                        POP(TokenKind::opParClose);
                        // the EXPR eats everything so no need to recurse, we can return the orelse immediately
                        if (top() == TokenKind::kwOrElse) {
                            auto orelse = std::make_unique<ast::Orelse>(pop().loc, std::move(call));
                            orelse->setValue(TRY(EXPR()));
                            return orelse;
                        } 
                        result = std::move(call);
                        break;
                    }
                    default:
                        return result;
                }
            }
        }

        /** PRIMARY := LITERAL | ident | 'self' | 'none'
                    |= PAREN_OR_TUPLE | IF_EXPR | MATCH_EXPR | RETURN_STMT | 'break' | 'continue'
            LITERAL := number | string | 'true' | 'false'
                    |= ARRAY_LITERAL | MAP_LITERAL | FUN_LITERAL

            Parenthesized expression and tuple literal both start with '(' so they are parsed together, see PAREN_OR_TUPLE.

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
                    return PAREN_OR_TUPLE();
                case TokenKind::opSquareOpen:
                    return ARRAY_LITERAL();
                case TokenKind::opCurlyOpen:
                    return MAP_LITERAL();
                case TokenKind::kwIf:
                    return IF_EXPR();
                case TokenKind::kwMatch:
                    return MATCH_EXPR();
                case TokenKind::kwReturn:
                    return RETURN_STMT();
                case TokenKind::kwBreak:
                    return std::make_unique<ast::Break>(pop());
                case TokenKind::kwContinue:
                    return std::make_unique<ast::Continue>(pop());
                case TokenKind::kwFun:
                    return FUN_LITERAL();
                default:
                    error_ = Token::error(top().loc, STR("Expected primary expression, got " << top().kind));
                    return nullptr;
            }
        }

        /** PAREN_OR_TUPLE := '(' LIST(CALL_ARG) ')'

            Parses the parenthesized expression, or tuple literal. When all items are named values, this is a tuple literal (including the empty one). Otherwise there must be only one item, which is the expression in parentheses. Mixing named and unnamed items, or multiple unnamed items is an error.

            This works because assignment has no value in dolang, so `(a = b)` cannot be an assignment in parentheses.
         */
        unique_ptr<ast::Node> PAREN_OR_TUPLE() {
            Location l = POP(TokenKind::opParOpen).loc;
            auto tuple = std::make_unique<ast::TupleLiteral>(l);
            if (! LIST(tuple.get(), TokenKind::opParClose, &Parser::CALL_ARG))
                return nullptr;
            POP(TokenKind::opParClose);
            for (uint32_t i = 0, e = tuple->size(); i < e; ++i) {
                if (tuple->child(i)->kind == ast::Kind::NamedValue)
                    continue;
                if (e != 1) {
                    error_ = Token::error(tuple->child(i)->loc, "Tuple literal items must be in the name = value form");
                    return nullptr;
                }
                auto result = std::make_unique<ast::Paren>(l);
                result->setValue(tuple->releaseChild(0));
                return result;
            }
            return tuple;
        }

        /** CALL_ARG := EXPR [ '=' EXPR ]

            Argument of calls and item of tuple literals. Without the '=', the expression itself is returned. With the '=', the left hand side must be a name and the NamedValue is returned. Only '=' is allowed, compound assignments are errors.
         */
        unique_ptr<ast::Node> CALL_ARG() {
            // keywords such as self or true parse as SymbolName as well, so check the token itself, this ensures the assignment itself is a user defined name, not a keyword or a value.
            bool startsWithIdentifier = top() == TokenKind::Identifier;
            auto first = TRY(EXPR());
            if (top() != TokenKind::opAssign)
                return first;
            if (! startsWithIdentifier || first->kind != ast::Kind::SymbolName) {
                error_ = Token::error(first->loc, "Expected name before =");
                return nullptr;
            }
            pop();
            auto result = std::make_unique<ast::NamedValue>(first->loc, static_cast<ast::SymbolName *>(first.get())->name);
            result->setValue(TRY(EXPR()));
            return result;
        }

        /** ARRAY_LITERAL := '[' LIST(EXPR) ']'
         */
        unique_ptr<ast::Node> ARRAY_LITERAL() {
            auto result = std::make_unique<ast::ArrayLiteral>(POP(TokenKind::opSquareOpen).loc);
            if (! LIST(result.get(), TokenKind::opSquareClose, &Parser::EXPR))
                return nullptr;
            POP(TokenKind::opSquareClose);
            return result;
        }

        /** MAP_LITERAL := '{' LIST(MAP_ITEM) '}'
         */
        unique_ptr<ast::Node> MAP_LITERAL() {
            auto result = std::make_unique<ast::MapLiteral>(POP(TokenKind::opCurlyOpen).loc);
            if (! LIST(result.get(), TokenKind::opCurlyClose, &Parser::MAP_ITEM))
                return nullptr;
            POP(TokenKind::opCurlyClose);
            return result;
        }

        /** MAP_ITEM := EXPR '->' EXPR
         */
        unique_ptr<ast::Node> MAP_ITEM() {
            auto key = TRY(EXPR());
            auto result = std::make_unique<ast::MapItem>(POP(TokenKind::opArrow).loc);
            result->setKey(std::move(key));
            result->setValue(TRY(EXPR()));
            return result;
        }

        /** FUN_LITERAL := 'fun' [ SIGNATURE(LAMBDA_ARG) ] BLOCK

            The signature is optional, `(` starts it and `indent` starts the body. When the signature is missing, the function has no arguments list (args() is nullptr), which is different from the empty one (`fun()`).
         */
        unique_ptr<ast::Node> FUN_LITERAL() {
            auto result = std::make_unique<ast::Function>(POP(TokenKind::kwFun).loc);
            if (top() == TokenKind::opParOpen && ! SIGNATURE(result.get(), &Parser::LAMBDA_ARG))
                return nullptr;
            result->setBody(TRY(BLOCK()));
            return result;
        }

#undef POP
#undef TRY

        /** \name Token handling and newline translation

            The parser provides the basic eof(), top(), pop() and condPop() interface for querying the tokenized sources. For the doLang this is complicated by the off side rule, which means that we also need to translate the newline tokens emited by the lexer into indent/dedent/newline variants, or swallow them.
            
            This translation is relatively simple though: 

            - every block of statements has its *indentation*, the column at which all its statements start. A line starting exactly at this column starts a new statement
            - a line indented *more* continues the current statement
            - except when the line follows a keyword that starts a block (`then`, `else`, `do`, `when`), the `fun` keyword, or a function signature. Then the more indented line starts a new block whose indentation is the column of the line
            - a line indented *less* ends the block (or blocks) and must start exactly at the indentation of one of the enclosing blocks

            We also need the ability to tell the parser that we are at the beginning of a block, which means that the next newline will be converted to an indent instead of swallowed. This is done by calling atBlockStart(), always explicitly by the grammar rules that introduce a block (and not automatically when one of the keywords is popped, as the same keywords are also used by the if and match expressions, which cannot contain blocks). The flag is cleared by the next pop(). But when we do, there could have already been a newline token analyzed and ignored (because we were *not* at the beginning of a block). To fight this, when a newline is swallowed, we keep that newline in pending_. 

            When atBlockStart() is called, we check pending_. If it is a newline, we need to recalculate the next token and we swap the pending and next tokens. This means next top will process the newline into the indent. When we pop the indent, the token in pending_ is moved back into next_ so that the next top will process it as the next token after the newline.

            A line at the current indentation is a newline, unless it starts with `else`, or a closing bracket, which can never start a statement. The `else` therefore continues the if statement (or the match statement), while the closing bracket closes the list (after the dedent of a multi-line list, or as the continuation of a single line list). To tell, we read the token after the newline into pending_. If it is `else`, or a closing bracket, it replaces the newline in next_, otherwise the newline is returned and the token in pending_ is moved to next_ when the newline is popped.

            Finally, layoutAllowed_ is false inside single line lists, where both BLOCK() and LIST() report an error when they would start a block, or a multi-line list. Single line lists set (and restore) the flag via LayoutGuard.

            So pending_ is either empty (EndOfFile), a swallowed newline (and next_ is the token after it), or the token that follows the newline (or indent) in next_.
         */
        //@{

        bool atBlockStart_ = false;

        /** False inside single line lists, where blocks and multi-line lists are not allowed. */
        bool layoutAllowed_ = true;

        /** Sets layoutAllowed_ for its lifetime and restores the previous value afterwards. */
        class LayoutGuard {
        public:
            LayoutGuard(Parser & p, bool allowed): p_{p}, old_{p.layoutAllowed_} { p_.layoutAllowed_ = allowed; }
            ~LayoutGuard() { p_.layoutAllowed_ = old_; }
        private:
            Parser & p_;
            bool old_;
        };

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
                // if the indentation of the newline is the same as the current indentation, we emit it as it is the end of statement, unless the next line starts with else, which continues the if (or match) statement, or a closing bracket, which closes the list
                if (next_.payload == indentStack_.back()) {
                    if (pending_ == TokenKind::EndOfFile)
                        pending_ = lexer_.next();
                    if (pending_ == TokenKind::kwElse || pending_ == TokenKind::opParClose || pending_ == TokenKind::opSquareClose || pending_ == TokenKind::opCurlyClose) {
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