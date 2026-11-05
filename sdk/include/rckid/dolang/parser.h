#pragma once

#include <rckid/rckid.h>
#include <rckid/dolang/lexer.h>

namespace rckid::dolang {

    namespace ast {
        enum class Kind {
            Program,
            SymbolName,
            UseDecl,
            VarDecl,
            FunDecl,
            TypeDecl,
            EnumDecl,


            ArrayType,
            TupleType,
            MapType,
            FunType,
            EventType,



            Assign,
        };
        
        class Node {
        public:
            Node(Kind kind, Location loc): kind{kind}, loc{loc} { }
            Kind kind;
            Location loc;

            virtual ~Node() = default;

            Node * parent() const { return parent_; }

        protected:

            void markAsChild(Node * child) {
                if (child == nullptr)
                    return;
                ASSERT(child->parent_ == nullptr);
                child->parent_ = this;
            }

            Node * parent_ = nullptr;

        };

        class SequenceNode : public Node {
        public:
            SequenceNode(Kind kind, Location loc): Node{kind, loc} { }

            void addChild(unique_ptr<Node> child) {
                markAsChild(child.get());
                children_.push_back(std::move(child));
            }

        protected:
            
            std::vector<std::unique_ptr<Node>> children_;
        }; 

        class Program : public SequenceNode {
        public:
            Program(Location loc): SequenceNode{Kind::Program, loc} { }
        };

        class SymbolName : public Node {
        public:
            SymbolName(Location loc, Symbol name): Node{Kind::SymbolName, loc}, name{name} { }
            Symbol name;
        };

        class UseDecl : public Node {
        public:
            UseDecl(Location loc, Symbol module): Node{Kind::UseDecl, loc}, module{module} { }
            Symbol module;
        };

        class VarDecl : public Node {
        public:
            VarDecl(TokenKind binding, Location loc, Symbol name): 
                Node{Kind::VarDecl, loc}, 
                binding{binding}, 
                name{name} {
            }

            TokenKind binding;
            Symbol name;

            Node * type() const { return type_.get(); }
            void setType(unique_ptr<Node> child) {
                markAsChild(child.get());
                type_ = std::move(child);
            }

            Node * value() const { return value_.get(); }
            void setValue(unique_ptr<Node> child) {
                markAsChild(child.get());
                value_ = std::move(child);
            }

        private:
            unique_ptr<Node> type_;
            unique_ptr<Node> value_;
        };

        class FunDecl : public Node {
        public:
            FunDecl(Location loc): Node{Kind::FunDecl, loc} { }
        };

        class TypeDecl : public Node {
        public:
            TypeDecl(Location loc): Node{Kind::TypeDecl, loc} { } 
        };

        class EnumDecl : public Node {
        public:
            EnumDecl(Location loc): Node{Kind::EnumDecl, loc} { }
        };

        class ArrayType : public Node {
        public:
            ArrayType(Location loc): Node{Kind::ArrayType, loc} { }

            Node * elementType() const { return elementType_.get(); }
            void setElementType(unique_ptr<Node> child) {
                markAsChild(child.get());
                elementType_ = std::move(child);
            }

        private:
            unique_ptr<Node> elementType_;
        };

        class TupleType : public SequenceNode {
        public:
            TupleType(Location loc): SequenceNode{Kind::TupleType, loc} { }
        };

        class MapType : public Node {
        public:
            MapType(Location loc): Node{Kind::MapType, loc} { }

            Node * keyType() const { return keyType_.get(); }
            void setKeyType(unique_ptr<Node> child) {
                markAsChild(child.get());
                keyType_ = std::move(child);
            }

            Node * valueType() const { return valueType_.get(); }
            void setValueType(unique_ptr<Node> child) {
                markAsChild(child.get());
                valueType_ = std::move(child);
            }

        private:
            unique_ptr<Node> keyType_;
            unique_ptr<Node> valueType_;
        };

        class FunType : public Node {

        };

        class EventType : public Node {

        };



        class Assign : public Node {
        public:
            Assign(Location loc): Node{Kind::Assign, loc} { }
        };

    }

    class Parser {
    public:
        Parser(ReadStream & from): lexer_{from} {
            // contract from lexer is first token is new line which we use to initialize the indent stack 
            Token t = lexer_.next();
            ASSERT(t == TokenKind::Newline);
            indentStack_.push_back(t.payload);
        }

        std::unique_ptr<ast::Program> parse() {
            return PROGRAM();
        }

    private:

/** A macro for propagating parser errors further up the parse stack. NOTE this is gcc specific.
 */
#define TRY(X) ({ auto result = X; if (! result) return nullptr; std::move(result); })

#define POP(TOKEN_KIND) ({ auto result = pop(); if (result.kind != TOKEN_KIND) return nullptr; std::move(result); })


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
                    // TODO error
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
            return nullptr;
        }

        /** TYPE_DECL := 'type' ident [ 'extends' ident ] '=' '(' LIST(FIELD_DECL) ')'
         */
        unique_ptr<ast::TypeDecl> TYPE_DECL() {
            return nullptr;
        }

        /** ENUM_DECL := 'enum' ident '=' ident { '|' ident }
         */
        unique_ptr<ast::EnumDecl> ENUM_DECL() {
            return nullptr;
        }

        /** EVENT_ASSIGN := ident { '.' ident } ( '=' | '+=' ) EXPR
         */
        unique_ptr<ast::Assign> EVENT_ASSIGN() {
            return nullptr;
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
            FIELD_TYPE := ident [ ':' TYPE ]
         */
        unique_ptr<ast::TupleType> TUPLE_TYPE() {
            return nullptr;
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

        unique_ptr<ast::FunType> FUN_TYPE() {
            return nullptr;
        }

        unique_ptr<ast::EventType> EVENT_TYPE() {
            return nullptr;
        }

        unique_ptr<ast::Node> EXPR() {
            return nullptr;
        }

#undef POP
#undef TRY


        /** \name Token handling and newline translation

            The parser provides the basic eof(), top(), pop() and condPop() interface for querying the tokenized sources. For the doLang this is complicated by the off side rule, which means that we also need to translate the newline tokens emited by the lexer into indent/dedent/newline variants, or swallow them.
            
            This translation is relatively simple though: 



         */
        //@{

        bool atBlockStart_ = false;

        Token next_; 

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
                        next_ = lexer_.next();
                        continue;
                    } 
                // if the the indentation is less than the current indentation, we are converting it to dedents. The issue is that multiple dedents can be generated for a single newline, so we need to manufacture the enough number of dedents
                } else if (next_.payload < indentStack_.back()) {
                    // first check we can actually dedent
                    if (indentStack_.size() == 1) {
                        // we are at the top level, so this is an error
                        next_.kind = TokenKind::Error;
                        return next_;
                    }
                    Token result{next_};
                    result.kind = TokenKind::Dedent;
                    result.payload =  indentStack_[indentStack_.size() - 2];
                    // if the level to which we are dedenting will be smaller than the newline indentation, its an error
                    if (next_.payload > result.payload) {
                        next_.kind = TokenKind::Error;
                        return next_;
                    }
                    return result;
                }
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
            // so that next top will reload
            next_.kind = TokenKind::EndOfFile;
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
        //@}
      
        Lexer lexer_;

    }; // dolang::Parser

} // namespace rckid::dolang