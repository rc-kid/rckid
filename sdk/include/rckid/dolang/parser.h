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
                  |= STATEMENT
         */
        unique_ptr<ast::Block> BLOCK() {
            auto result = std::make_unique<ast::Block>(top().loc);
            if (condPop(TokenKind::Indent)) {
                result->addChild(TRY(STATEMENT()));
                while (condPop(TokenKind::Newline))
                    result->addChild(TRY(STATEMENT()));
                POP(TokenKind::Dedent);
            } else {
                result->addChild(TRY(STATEMENT()));
            }
            return result;
        }

        /** BODY := BLOCK | EXPR

            Decided by the next token, indent starts the block, anything else is the expression.
         */
        unique_ptr<ast::Node> BODY() {
            UNIMPLEMENTED;
        }

        /** LIST(X) := indent LINE(X) { newline LINE(X) } dedent
                     | LINE(X)
            LINE(X) := [ X { ',' X } [ ',' ] ]

            Parses the items of a bracketed list, calling item() to parse each X. The opening bracket is expected to be already popped and the closing bracket is left to the caller. Returns false on error.
         */
        template<typename ITEM>
        bool LIST(ITEM item) {
            UNIMPLEMENTED;
        }

        /** STATEMENT := LET_STMT | IF | WHILE_STMT | FOR_STMT | MATCH | RETURN_STMT
                       |= 'break' | 'continue' | 'write' EXPR | 'test' EXPR | EXPR_STMT

            LET_STMT is parsed by VAR_DECL. At the beginning of a statement, `if` and `match` are always the IF and MATCH statements.
         */
        unique_ptr<ast::Node> STATEMENT() {
            switch (top().kind) {
                case TokenKind::kwLet:
                case TokenKind::kwConst:
                    return VAR_DECL(top().kind);
                case TokenKind::kwIf:
                    return IF();
                case TokenKind::kwWhile:
                    return WHILE_STMT();
                case TokenKind::kwFor:
                    return FOR_STMT();
                case TokenKind::kwMatch:
                    return MATCH();
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
            UNIMPLEMENTED;
        }

        /** RETURN_STMT := 'return' [ EXPR ]

            There is no value when the next token is newline, or dedent.
         */
        unique_ptr<ast::Node> RETURN_STMT() {
            UNIMPLEMENTED;
        }

        /** IF := 'if' EXPR 'then' ( BLOCK [ [ newline ] 'else' BODY ] | EXPR [ 'else' BODY ] )

            Shared by the if statement and the if expression.
         */
        unique_ptr<ast::Node> IF() {
            UNIMPLEMENTED;
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

        /** MATCH := 'match' EXPR 'with' ( indent CASE { newline ( CASE | 'else' BODY ) } dedent
                                         | INLINE_CASE { INLINE_CASE } [ 'else' EXPR ] )

            Shared by the match statement and the match expression.
         */
        unique_ptr<ast::Node> MATCH() {
            UNIMPLEMENTED;
        }

        /** CASE := 'is' PATTERN 'then' BODY
         */
        unique_ptr<ast::Node> CASE() {
            UNIMPLEMENTED;
        }

        /** INLINE_CASE := 'is' PATTERN 'then' EXPR
         */
        unique_ptr<ast::Node> INLINE_CASE() {
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

        /** EXPR := IF | MATCH | OR_EXPR
         */
        unique_ptr<ast::Node> EXPR() {
            UNIMPLEMENTED;
        }

        /** OR_EXPR := AND_EXPR { 'or' AND_EXPR }
         */
        unique_ptr<ast::Node> OR_EXPR() {
            UNIMPLEMENTED;
        }

        /** AND_EXPR := NOT_EXPR { 'and' NOT_EXPR }
         */
        unique_ptr<ast::Node> AND_EXPR() {
            UNIMPLEMENTED;
        }

        /** NOT_EXPR := 'not' NOT_EXPR | EQ_EXPR
         */
        unique_ptr<ast::Node> NOT_EXPR() {
            UNIMPLEMENTED;
        }

        /** EQ_EXPR := CMP_EXPR { '==' CMP_EXPR }
                     |= CMP_EXPR { '!=' CMP_EXPR }

            Mixing == and != in a single chain is an error.
         */
        unique_ptr<ast::Node> EQ_EXPR() {
            UNIMPLEMENTED;
        }

        /** CMP_EXPR := ADD_EXPR { ( '<' | '<=' | '>' | '>=' ) ADD_EXPR }
         */
        unique_ptr<ast::Node> CMP_EXPR() {
            UNIMPLEMENTED;
        }

        /** ADD_EXPR := MUL_EXPR { ( '+' | '-' ) MUL_EXPR }
         */
        unique_ptr<ast::Node> ADD_EXPR() {
            UNIMPLEMENTED;
        }

        /** MUL_EXPR := UNARY_EXPR { ( '*' | '/' ) UNARY_EXPR }
         */
        unique_ptr<ast::Node> MUL_EXPR() {
            UNIMPLEMENTED;
        }

        /** UNARY_EXPR := ( '-' | 'clone' ) UNARY_EXPR | POSTFIX_EXPR
         */
        unique_ptr<ast::Node> UNARY_EXPR() {
            UNIMPLEMENTED;
        }

        /** POSTFIX_EXPR := PRIMARY { '.' ident | '[' EXPR ']' | PAREN_EXPR | 'orelse' ORELSE_RHS }
         */
        unique_ptr<ast::Node> POSTFIX_EXPR() {
            UNIMPLEMENTED;
        }

        /** ORELSE_RHS := UNARY_EXPR | 'return' [ EXPR ] | 'break' | 'continue' | BLOCK
         */
        unique_ptr<ast::Node> ORELSE_RHS() {
            UNIMPLEMENTED;
        }

        /** PRIMARY := LITERAL | ident | 'self' | 'none' | PAREN_EXPR
         */
        unique_ptr<ast::Node> PRIMARY() {
            UNIMPLEMENTED;
        }

        /** LITERAL := number | string | 'true' | 'false'
                     |= ARRAY_LITERAL | MAP_LITERAL | FUN_LITERAL
         */
        unique_ptr<ast::Node> LITERAL() {
            UNIMPLEMENTED;
        }

        /** PAREN_EXPR := '(' LIST(NAMED_EXPR) ')'

            Grouping, tuple literal, or call arguments, differentiated after parsing.
         */
        unique_ptr<ast::Node> PAREN_EXPR() {
            UNIMPLEMENTED;
        }

        /** NAMED_EXPR := EXPR [ '=' EXPR ]
         */
        unique_ptr<ast::Node> NAMED_EXPR() {
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


            We also need the ability to tell the parser that we are at the beginning of a block, which means that the next newline will be converted to an indent instead of swallowed. This is done by calling atBlockStart(). But when we do, there could have already been a newline token analyzed and ignored (because we were *not* at the beginning of a block). To fight this, when a newline is swallowed, we add that newline to special variable newlineBackup_. 

            When adBlockStart() is called, we check newlineBackup_. If it is a newline, we need to recalculate the next token and we swap the backup and next tokens. This means next top will process the newline into the indent. When we pop indent, if there is something in the newlineBackup_, we will swap it back into next_ so that the next top will process it as the next token after the newline.

         */
        //@{

        bool atBlockStart_ = false;

        Token next_; 
        Token newlineBackup_;

        std::vector<int32_t> indentStack_;

        bool eof() {
            return top() == TokenKind::EndOfFile;
        }

        Token top() {
            if (next_.kind == TokenKind::EndOfFile)
                    next_ = lexer_.next();
            while (next_.kind == TokenKind::Newline) {
                // if the indentation of the newline is the same as the current indentation, we emit it as it is the end of statement
                if (next_.payload == indentStack_.back()) {
                    return next_;
                // if the indentation is greater than the current indentation, we are either converting the newline to indent if we are at the beginning of a block expecting statement, or we are swallowing the newline as it is a continuation of the previous statement
                } else if (next_.payload > indentStack_.back()) {
                    if (atBlockStart_) {
                        next_.kind = TokenKind::Indent;
                        return next_;
                    } else {
                        // swallow the newline
                        newlineBackup_ = next_;
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
                case TokenKind::kwOrElse:
                case TokenKind::kwWith:
                    atBlockStart_ = true;
                    break;
                default:
                    break;
            }
            // so that next top will reload fresh
            if (next_ == TokenKind::Indent && newlineBackup_ != TokenKind::EndOfFile)
                next_ = newlineBackup_;
            else
                next_.kind = TokenKind::EndOfFile;
            newlineBackup_.kind = TokenKind::EndOfFile;
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
            if (newlineBackup_ == TokenKind::Newline) {
                // swap the backup and next tokens so that next top will process the newline into indent
                std::swap(newlineBackup_, next_);
            }
        }
        //@}
      
        Lexer lexer_;

        Token error_;

    }; // dolang::Parser

} // namespace rckid::dolang