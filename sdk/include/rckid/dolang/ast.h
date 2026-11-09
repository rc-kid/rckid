#pragma once

#include <rckid/rckid.h>
#include <rckid/dolang/lexer.h>

namespace rckid::dolang::ast {
    
    enum class Kind {
        Error,
        Program,
        Block,
        SymbolName,
        Number,
        Text,
        UseDecl,
        VarDecl,
        FunDecl,
        TypeDecl,
        EnumDecl,


        ArrayType,
        TupleType,
        FieldType,
        MapType,
        FunType,
        EventType,



        Assign,
        BinaryOp,
        UnaryOp,
        Is,
        List,
        ArrayLiteral,
        MapLiteral,
        MapItem,

        Break,
        Continue,
        Test,
        Write,
        Return,
        If,
        While,
        For,
        Match,
        MatchCase,

    };
    
    class Node {
    public:
        Node(Kind kind, Location loc): kind{kind}, loc{loc} { }
        Kind kind;
        Location loc;

        virtual ~Node() = default;

        Node * parent() const { return parent_; }

        void prettyPrint(Writer && w) const {
            prettyPrint((Writer &)w);
        }

        /** Prints the node as source code, i.e. parsing the output gives the same AST.
         */
        virtual void prettyPrint(Writer & w, uint32_t indent = 0) const = 0;

        void debugPrint(Writer && w) const {
            debugPrint((Writer &)w);
        }

        virtual void debugPrint(Writer & w, uint32_t indent = 0) const {
            for (uint32_t i = 0; i < indent; ++i)
                w.putChar(' ');
            w << kind << "@" << loc;
        }

    protected:

        void markAsChild(Node * child) {
            if (child == nullptr)
                return;
            ASSERT(child->parent_ == nullptr);
            child->parent_ = this;
        }

        Node * parent_ = nullptr;
    };

    class Error : public Node {
    public:
        Error(Token const & t):
            Node{Kind::Error, t.loc},
            msg{t.asText()} {
        }

        TextLiteral msg;

        void prettyPrint(Writer & w, [[maybe_unused]] uint32_t indent = 0) const override {
            w << "ERROR at (" << loc.line << ":" << loc.col << "): " << msg.c_str();
        }
    }; 

    class SequenceNode : public Node {
    public:
        SequenceNode(Kind kind, Location loc): Node{kind, loc} { }

        void addChild(unique_ptr<Node> child) {
            markAsChild(child.get());
            children_.push_back(std::move(child));
        }

        void prettyPrint(Writer & w, uint32_t indent = 0) const override {
            for (auto const & child : children_) {
                w.putChar('\n');
                for (uint32_t i = 0; i < indent + 2; ++i)
                    w.putChar(' ');
                child->prettyPrint(w, indent + 2);
            }
        }

        /** Children are on their own lines. Same as other nodes, there is no newline after the last child, so that sequences can nest. 
         */
        void debugPrint(Writer & w, uint32_t indent = 0) const override {
            Node::debugPrint(w, indent);
            for (auto const & child : children_) {
                w << "\n";
                child->debugPrint(w, indent + 2);
            }
        }

    protected:
        
        std::vector<std::unique_ptr<Node>> children_;
    }; 

    class Program : public SequenceNode {
    public:
        Program(Location loc): SequenceNode{Kind::Program, loc} { }

        void prettyPrint(Writer & w, uint32_t indent = 0) const override {
            // program does not extra indent
            for (auto const & child : children_) {
                for (uint32_t i = 0; i < indent; ++i)
                    w.putChar(' ');
                child->prettyPrint(w, indent);
                w.putChar('\n');
            }
            if (children_.empty())
                w.putChar('\n');
        }

        /** The whole program ends with a newline, as it is the root.
         */
        void debugPrint(Writer & w, uint32_t indent = 0) const override {
            SequenceNode::debugPrint(w, indent);
            w << "\n";
        }
    };

    class Block : public SequenceNode {
    public:
        Block(Location loc): SequenceNode{Kind::Block, loc} { }
    };

    /** Items of a bracketed list. Remembers whether the list was written in the multi-line form, so that it can be pretty printed in the same form (one item per line). Prints only the items. Nodes that are lists themselves (array literal, tuple type, etc.) inherit from List, provide their own kind and print the brackets around the items. Nodes that contain a list as one of their parts (function arguments, etc.) embed a plain List and print the brackets themselves. In the multi-line form, the items are printed on their own lines indented from the parent and the output ends with a new line at the parent's indentation, so that the closing bracket follows.
     */
    class List : public SequenceNode {
    public:
        List(Location loc): SequenceNode{Kind::List, loc} { }

        bool multiLine = false;

        void prettyPrint(Writer & w, uint32_t indent = 0) const override {
            if (multiLine) {
                for (auto const & child : children_) {
                    w.putChar('\n');
                    for (uint32_t i = 0; i < indent + 2; ++i)
                        w.putChar(' ');
                    child->prettyPrint(w, indent + 2);
                }
                w.putChar('\n');
                for (uint32_t i = 0; i < indent; ++i)
                    w.putChar(' ');
            } else {
                bool first = true;
                for (auto const & child : children_) {
                    if (first)
                        first = false;
                    else
                        w << ", ";
                    child->prettyPrint(w, indent);
                }
            }
        }

        void debugPrint(Writer & w, uint32_t indent = 0) const override {
            Node::debugPrint(w, indent);
            if (multiLine)
                w << " multi-line";
            for (auto const & child : children_) {
                w << "\n";
                child->debugPrint(w, indent + 2);
            }
        }

    protected:
        List(Kind kind, Location loc): SequenceNode{kind, loc} { }
    };

    class SymbolName : public Node {
    public:
        SymbolName(Location loc, Symbol name): Node{Kind::SymbolName, loc}, name{name} { }
        Symbol name;

        SymbolName(Token const & t): 
            Node{Kind::SymbolName, t.loc}, 
            name{t.asSymbol()} {
        }

        void prettyPrint(Writer & w, uint32_t indent = 0) const override {
            w << name.c_str();
        }

        void debugPrint(Writer & w, uint32_t indent = 0) const override {
            Node::debugPrint(w, indent);
            w << " " << name.c_str();
        }
    };

    class Number : public Node {
    public:
        
        Number(Location loc, Num num): Node{Kind::Number, loc}, num{num} { }

        Number(Token const & t): 
            Node{Kind::Number, t.loc}, 
            num{t.asNum()} {
        }

        Num num;

        void prettyPrint(Writer & w, uint32_t indent = 0) const override {
            w << num.n();
            // TODO we want a lot better printing here (floats, etc.)
            if (! num.isInteger())
                w << "/" << num.d();
        } 
        void debugPrint(Writer & w, uint32_t indent = 0) const override {
            Node::debugPrint(w, indent);
            w << " " << num.n();
            if (! num.isInteger())
                w << "/" << num.d();
        }
    };         

    class Text : public Node {
    public:
        
        Text(Location loc, TextLiteral text): Node{Kind::Text, loc}, text{text} { }

        Text(Token const & t): 
            Node{Kind::Text, t.loc}, 
            text{t.asText()} {
        }
        
        TextLiteral text;

        void prettyPrint(Writer & w, uint32_t indent = 0) const override {
            w << "\"";
            for (char const * p = text.c_str(); *p != '\0'; ++p) {
                switch (*p) {
                    case '\n':
                        w << "\\n";
                        break;
                    case '\r':
                        w << "\\r";
                        break;
                    case '\t':
                        w << "\\t";
                        break;
                    case '"':
                        w << "\\\"";
                        break;
                    case '\\':
                        w << "\\\\";
                        break;
                    default:
                        w.putChar(*p);
                }
            }
            w << "\"";
        }

        void debugPrint(Writer & w, uint32_t indent = 0) const override {
            Node::debugPrint(w, indent);
            w << " ";
            prettyPrint(w, indent);
        }
    }; 

    class UseDecl : public Node {
    public:
        UseDecl(Location loc, Symbol module): Node{Kind::UseDecl, loc}, module{module} { }
        Symbol module;

        void prettyPrint(Writer & w, uint32_t indent = 0) const override {
            w << "use " << module.c_str();
        }
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

        void prettyPrint(Writer & w, uint32_t indent = 0) const override {
            switch (binding) {
                case TokenKind::kwGlobal:
                    w << "global ";
                    break;
                case TokenKind::kwConst:
                    w << "const ";
                    break;
                case TokenKind::kwLet:
                    w << "let ";
                    break;
                default:
                    UNREACHABLE;
            }
            w << name.c_str();
            if (type_ != nullptr) {
                w << " : ";
                type_->prettyPrint(w, indent);
            }
            if (value_ != nullptr) {
                w << " = ";
                value_->prettyPrint(w, indent);
            }
        }

        void debugPrint(Writer & w, uint32_t indent = 0) const override {
            Node::debugPrint(w, indent);
            w << " " << binding << " " << name.c_str();
            if (type_ != nullptr) {
                w << "\n";
                type_->debugPrint(w, indent + 2);
            }
            if (value_ != nullptr) {
                w << "\n";
                value_->debugPrint(w, indent + 2);
            }
        }

    protected:

        unique_ptr<Node> type_;
        unique_ptr<Node> value_;
    };

    class FunDecl : public Node {
    public:
        FunDecl(Location loc, Symbol name): Node{Kind::FunDecl, loc}, name_{name} { }

        Symbol name() const { return name_; }
        Symbol className() const { return className_; }

        bool isMethod() const { return className_ != Symbol{}; }

        void setMethodName(Symbol methodName) {
            className_ = name_;
            name_ = methodName; 
        }

        Node * returnType() const { return returnType_.get(); }
        void setReturnType(unique_ptr<Node> child) {
            markAsChild(child.get());
            returnType_ = std::move(child);
        }

        bool optionalReturn() const { return optionalReturn_; }
        void setOptionalReturn(bool optional) { optionalReturn_ = optional; }

        Block * body() const { return body_.get(); }
        void setBody(unique_ptr<Block> child) {
            markAsChild(child.get());
            body_ = std::move(child);
        }

        void prettyPrint(Writer & w, uint32_t indent = 0) const override {
            w << "fun " << name_.c_str() << "(";
            // TODO ags
            w << ")";
            if (returnType_ != nullptr) {
                w << " -> ";
                if (optionalReturn_)
                    w << "optional ";
                returnType_->prettyPrint(w, indent);
            }
            ASSERT(body_ != nullptr);
            body_->prettyPrint(w, indent);
        }

        void debugPrint(Writer & w, uint32_t indent = 0) const override {
            Node::debugPrint(w, indent);
            w << " ";
            if (isMethod())
                w << className_.c_str() << ".";
            w << name_.c_str();
            w << "\n";
            body_->debugPrint(w, indent + 2);
        }

    protected:

        Symbol name_;
        Symbol className_;
        unique_ptr<Node> returnType_;
        bool optionalReturn_ = false;
        unique_ptr<Block> body_;
    };

    class TypeDecl : public Node {
    public:
        TypeDecl(Location loc): Node{Kind::TypeDecl, loc} { } 

        void prettyPrint(Writer & w, uint32_t indent = 0) const override {
            UNIMPLEMENTED;
        }
    };

    class EnumDecl : public Node {
    public:
        EnumDecl(Location loc): Node{Kind::EnumDecl, loc} { }

        void addValue(Symbol value) {
            values_.push_back(value);
        }

        void prettyPrint(Writer & w, [[maybe_unused]] uint32_t indent = 0) const override {
            w << "enum " << name.c_str() << " = ";
            bool first = true;
            for (auto const & value : values_) {
                if (first)
                    first = false;
                else
                    w << " | ";
                w << value.c_str();
            }
        }

        void debugPrint(Writer & w, uint32_t indent = 0) const override {
            Node::debugPrint(w, indent);
            w << " " << name.c_str();
            for (auto const & value : values_) {
                w << "\n";
                for (uint32_t i = 0; i < indent + 2; ++i)
                    w.putChar(' ');
                w << value.c_str();
            }
        }

        Symbol name;

    protected:
        std::vector<Symbol> values_;
    };

    class ArrayType : public Node {
    public:
        ArrayType(Location loc): Node{Kind::ArrayType, loc} { }

        Node * elementType() const { return elementType_.get(); }
        void setElementType(unique_ptr<Node> child) {
            markAsChild(child.get());
            elementType_ = std::move(child);
        }

        void prettyPrint(Writer & w, uint32_t indent = 0) const override {
            w << "[";
            elementType_->prettyPrint(w, indent);
            w << "]";
        }

    protected:
        unique_ptr<Node> elementType_;
    };

    class TupleType : public List {
    public:
        TupleType(Location loc): List{Kind::TupleType, loc} { }

        void prettyPrint(Writer & w, uint32_t indent = 0) const override {
            w << "(";
            List::prettyPrint(w, indent);
            w << ")";
        }
    };

    class FieldType : public Node {
    public:
        FieldType(Token const & name): Node{Kind::FieldType, name.loc}, name{name.asSymbol()} { }
        
        Symbol name;

        Node * type() const { return type_.get(); }
        void setType(unique_ptr<Node> child) {
            markAsChild(child.get());
            type_ = std::move(child);
        }

        void prettyPrint(Writer & w, uint32_t indent = 0) const override {
            w << name.c_str();
            if (type_ != nullptr) {
                w << " : ";
                type_->prettyPrint(w, indent);
            }
        }

        void debugPrint(Writer & w, uint32_t indent = 0) const override {
            Node::debugPrint(w, indent);
            w << " " << name.c_str();
            if (type_ != nullptr) {
                w << "\n";
                type_->debugPrint(w, indent + 2);
            }
        }

    private:

        unique_ptr<Node> type_;

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

        void prettyPrint(Writer & w, uint32_t indent = 0) const override {
            w << "{";
            keyType_->prettyPrint(w, indent);
            w << " -> ";
            valueType_->prettyPrint(w, indent);
            w << "}";
        }

    protected:
        unique_ptr<Node> keyType_;
        unique_ptr<Node> valueType_;
    };

    class FunType : public Node {
    public:

        void prettyPrint(Writer & w, uint32_t indent = 0) const override {
            UNIMPLEMENTED;
        }
    };

    class EventType : public Node {
    public:

        void prettyPrint(Writer & w, uint32_t indent = 0) const override {
            UNIMPLEMENTED;
        }
    };



    class Assign : public Node {
    public:
        Assign(Token const & op): 
            Node{Kind::Assign, op.loc},
            op_{op.kind}
        {
            ASSERT(op.isAssignmentOperator());
        }

        Node * left() const { return left_.get(); }
        void setLeft(unique_ptr<Node> child) {
            markAsChild(child.get());
            left_ = std::move(child);
        }

        Node * right() const { return right_.get(); }
        void setRight(unique_ptr<Node> child) {
            markAsChild(child.get());
            right_ = std::move(child);
        }

        void prettyPrint(Writer & w, uint32_t indent = 0) const override {
            left_->prettyPrint(w, indent);
            w << " " << op_ << " ";
            right_->prettyPrint(w, indent);
        }

        void debugPrint(Writer & w, uint32_t indent = 0) const override {
            Node::debugPrint(w, indent);
            w << " " << op_ << "\n";
            left_->debugPrint(w, indent + 2);
            w << "\n";
            right_->debugPrint(w, indent + 2);
        }

    private:
        TokenKind op_;
        unique_ptr<Node> left_;
        unique_ptr<Node> right_;
    };

    class BinaryOp : public Node {
    public:
        BinaryOp(Token const & op): 
            Node{Kind::BinaryOp, op.loc},
            op_{op.kind} 
        { 
            ASSERT(op.isBinaryOperator());
        }

        Node * left() const { return left_.get(); }
        void setLeft(unique_ptr<Node> child) {
            markAsChild(child.get());
            left_ = std::move(child);
        }

        Node * right() const { return right_.get(); }
        void setRight(unique_ptr<Node> child) {
            markAsChild(child.get());
            right_ = std::move(child);
        }

        void prettyPrint(Writer & w, uint32_t indent = 0) const override {
            left_->prettyPrint(w, indent);
            w << " " << op_ << " ";
            right_->prettyPrint(w, indent);
        }

        void debugPrint(Writer & w, uint32_t indent = 0) const override {
            Node::debugPrint(w, indent);
            w << " " << op_ << "\n";
            left_->debugPrint(w, indent + 2);
            w << "\n";
            right_->debugPrint(w, indent + 2);
        }

    protected:
        TokenKind op_;
        unique_ptr<Node> left_;
        unique_ptr<Node> right_;
    }; 

    class UnaryOp : public Node {
    public:
        UnaryOp(Token const & op): 
            Node{Kind::UnaryOp, op.loc},
            op_{op.kind} 
        { 
            ASSERT(op.isUnaryOperator());
        }

        Node * operand() const { return operand_.get(); }
        void setOperand(unique_ptr<Node> child) {
            markAsChild(child.get());
            operand_ = std::move(child);
        }

        void prettyPrint(Writer & w, uint32_t indent = 0) const override {
            w << op_;
            if (op_ != TokenKind::opSub) // no space for minus sign
                w << " ";
            operand_->prettyPrint(w, indent);
        }

        void debugPrint(Writer & w, uint32_t indent = 0) const override {
            Node::debugPrint(w, indent);
            w << " " << op_ << "\n";
            operand_->debugPrint(w, indent + 2);
        }

    protected:
        TokenKind op_;
        unique_ptr<Node> operand_;
    }; 

    /** The `X is PATTERN` and `X is not PATTERN` test. 
     
        Evaluates to bool. When it is the whole condition of an if, or while, the pattern may also bind values, otherwise binding patterns are a semantic error. The negated form never binds.
     */
    class Is : public Node {
    public:
        Is(Token const & t): 
            Node{Kind::Is, t.loc} 
        {
            ASSERT(t == TokenKind::kwIs);
        }

        Node * value() const { return value_.get(); }
        void setValue(unique_ptr<Node> child) {
            markAsChild(child.get());
            value_ = std::move(child);
        }

        Node * pattern() const { return pattern_.get(); }
        void setPattern(unique_ptr<Node> child) {
            markAsChild(child.get());
            pattern_ = std::move(child);
        }

        bool negated() const { return negated_; }
        void setNegated(bool value) { negated_ = value; }

        void prettyPrint(Writer & w, uint32_t indent = 0) const override {
            value_->prettyPrint(w, indent);
            w << (negated_ ? " is not " : " is ");
            pattern_->prettyPrint(w, indent);
        }

        void debugPrint(Writer & w, uint32_t indent = 0) const override {
            Node::debugPrint(w, indent);
            if (negated_)
                w << " not";
            w << "\n";
            value_->debugPrint(w, indent + 2);
            w << "\n";
            pattern_->debugPrint(w, indent + 2);
        }

    protected:
        unique_ptr<Node> value_;
        unique_ptr<Node> pattern_;
        bool negated_ = false;
    };



    class ArrayLiteral : public List {
    public:
        ArrayLiteral(Location loc): List{Kind::ArrayLiteral, loc} { }

        void prettyPrint(Writer & w, uint32_t indent = 0) const override {
            w << "[";
            List::prettyPrint(w, indent);
            w << "]";
        }
    };

    class MapLiteral : public List {
    public:
        MapLiteral(Location loc): List{Kind::MapLiteral, loc} { }

        void prettyPrint(Writer & w, uint32_t indent = 0) const override {
            w << "{";
            List::prettyPrint(w, indent);
            w << "}";
        }
    };

    /** Single key -> value pair of a map literal. The location is that of the arrow.
     */
    class MapItem : public Node {
    public:
        MapItem(Location loc): Node{Kind::MapItem, loc} { }

        Node * key() const { return key_.get(); }
        void setKey(unique_ptr<Node> child) {
            markAsChild(child.get());
            key_ = std::move(child);
        }

        Node * value() const { return value_.get(); }
        void setValue(unique_ptr<Node> child) {
            markAsChild(child.get());
            value_ = std::move(child);
        }

        void prettyPrint(Writer & w, uint32_t indent = 0) const override {
            key_->prettyPrint(w, indent);
            w << " -> ";
            value_->prettyPrint(w, indent);
        }

        void debugPrint(Writer & w, uint32_t indent = 0) const override {
            Node::debugPrint(w, indent);
            w << "\n";
            key_->debugPrint(w, indent + 2);
            w << "\n";
            value_->debugPrint(w, indent + 2);
        }

    protected:
        unique_ptr<Node> key_;
        unique_ptr<Node> value_;
    };

    class Break : public Node {
    public:
        Break(Location loc): Node{Kind::Break, loc} { }
        Break(Token const & t): 
            Node{Kind::Break, t.loc} 
        {
            ASSERT(t == TokenKind::kwBreak);
        }

        void prettyPrint(Writer & w, uint32_t indent = 0) const override {
            w << "break";
        }
    };

    class Continue : public Node {
    public:
        Continue(Location loc): Node{Kind::Continue, loc} { }
        Continue(Token const & t): 
            Node{Kind::Continue, t.loc} 
        {
            ASSERT(t == TokenKind::kwContinue);
        }

        void prettyPrint(Writer & w, uint32_t indent = 0) const override {
            w << "continue";
        }
    };

    class Return : public Node {
    public:
        Return(Location loc): Node{Kind::Return, loc} { }

        Node * value() const { return value_.get(); }
        void setValue(unique_ptr<Node> child) {
            markAsChild(child.get());
            value_ = std::move(child);
        }

        void prettyPrint(Writer & w, uint32_t indent = 0) const override {
            w << "return";
            if (value_ != nullptr) {
                w << " ";
                value_->prettyPrint(w, indent);
            }
        }

        void debugPrint(Writer & w, uint32_t indent = 0) const override {
            Node::debugPrint(w, indent);
            if (value_ != nullptr) {
                w << "\n";
                value_->debugPrint(w, indent + 2);
            }
        }

    protected:
        unique_ptr<Node> value_;
    };

    class If : public Node {
    public:
        If(Location loc): Node{Kind::If, loc} { }

        void prettyPrint(Writer & w, uint32_t indent = 0) const override {
            w << "if ";
            condition_->prettyPrint(w, indent);
            w << " then";
            thenCase()->prettyPrint(w, indent);
            if (elseCase() != nullptr) {
                // else case is on a new line, indented same as the if
                w.putChar('\n');
                for (uint32_t i = 0; i < indent; ++i)
                    w.putChar(' ');
                w << "else";
                elseCase()->prettyPrint(w, indent);
            }
        }

        void debugPrint(Writer & w, uint32_t indent = 0) const override {
            Node::debugPrint(w, indent);
            if (condition_ != nullptr) {
                w << "\n";
                condition_->debugPrint(w, indent + 2);
            }
            if (then_ != nullptr) {
                w << "\n";
                then_->debugPrint(w, indent + 2);
            }
            if (else_ != nullptr) {
                w << "\n";
                else_->debugPrint(w, indent + 2);
            }
        }

        Node * condition() const { return condition_.get(); }
        void setCondition(unique_ptr<Node> child) {
            markAsChild(child.get());
            condition_ = std::move(child);
        }

        Node * thenCase() const { return then_.get(); }
        void setThenCase(unique_ptr<Node> child) {
            markAsChild(child.get());
            then_ = std::move(child);
        }

        Node * elseCase() const { return else_.get(); }
        void setElseCase(unique_ptr<Node> child) {
            markAsChild(child.get());
            else_ = std::move(child);
        }

    protected:

        unique_ptr<Node> condition_;
        unique_ptr<Node> then_;
        unique_ptr<Node> else_;

    }; 

    class Match : public Node {
    public:
        Match(Location loc): Node{Kind::Match, loc} { }

        Node * value() const { return value_.get(); }
        void setValue(unique_ptr<Node> child) {
            markAsChild(child.get());
            value_ = std::move(child);
        }

        Node * elseCase() const { return else_.get(); }
        void setElseCase(unique_ptr<Node> child) {
            markAsChild(child.get());
            else_ = std::move(child);
        }

        void addCase(unique_ptr<Node> child) {
            markAsChild(child.get());
            cases_.push_back(std::move(child));
        }

        void prettyPrint(Writer & w, uint32_t indent = 0) const override {
            w << "match ";
            value_->prettyPrint(w, indent);
            w << " when";
            for (auto const & c : cases_)
                c->prettyPrint(w, indent);
            if (else_ != nullptr) {
                // match statement has the else case on its own line, at the indentation of the cases, while the expression is all on a single line
                if (else_->kind == Kind::Block) {
                    w.putChar('\n');
                    for (uint32_t i = 0; i < indent + 2; ++i)
                        w.putChar(' ');
                    w << "else";
                    else_->prettyPrint(w, indent + 2);
                } else {
                    w << " else ";
                    else_->prettyPrint(w, indent);
                }
            }
        }

        void debugPrint(Writer & w, uint32_t indent = 0) const override {
            Node::debugPrint(w, indent);
            if (value_ != nullptr) {
                w << "\n";
                value_->debugPrint(w, indent + 2);
            }
            for (auto const & c : cases_) {
                w << "\n";
                c->debugPrint(w, indent + 2);
            }
            if (else_ != nullptr) {
                w << "\n";
                else_->debugPrint(w, indent + 2);
            }
        }

    private:
        unique_ptr<Node> value_;
        unique_ptr<Node> else_;
        std::vector<std::unique_ptr<Node>> cases_;
    }; 

    class MatchCase : public Node {
    public:
        MatchCase(Location loc): Node{Kind::MatchCase, loc} { }

        Node * pattern() const { return pattern_.get(); }
        void setPattern(unique_ptr<Node> child) {
            markAsChild(child.get());
            pattern_ = std::move(child);
        }

        Node * body() const { return body_.get(); }
        void setBody(unique_ptr<Node> child) {
            markAsChild(child.get());
            body_ = std::move(child);
        }

        void prettyPrint(Writer & w, uint32_t indent = 0) const override {
            // case of the match statement is on its own line, indented from the match, while the expression is all on a single line
            if (body_->kind == Kind::Block) {
                w.putChar('\n');
                for (uint32_t i = 0; i < indent + 2; ++i)
                    w.putChar(' ');
                w << "is ";
                pattern_->prettyPrint(w, indent);
                w << " then";
                body_->prettyPrint(w, indent + 2);
            } else {
                w << " is ";
                pattern_->prettyPrint(w, indent);
                w << " then ";
                body_->prettyPrint(w, indent);
            }
        }

        void debugPrint(Writer & w, uint32_t indent = 0) const override {
            Node::debugPrint(w, indent);
            w << "\n";
            pattern_->debugPrint(w, indent + 2);
            w << "\n";
            body_->debugPrint(w, indent + 2);
        }

    private:
        unique_ptr<Node> pattern_;
        unique_ptr<Node> body_;

    };

    class While : public Node {
    public:

        While(Location loc): Node{Kind::While, loc} { }

        Node * condition() const { return condition_.get(); }
        void setCondition(unique_ptr<Node> child) {
            markAsChild(child.get());
            condition_ = std::move(child);
        }

        Node * body() const { return body_.get(); }
        void setBody(unique_ptr<Node> child) {
            markAsChild(child.get());
            body_ = std::move(child);
        }

        void prettyPrint(Writer & w, uint32_t indent = 0) const override {
            w << "while ";
            condition_->prettyPrint(w, indent);
            w << " do";
            body_->prettyPrint(w, indent);
        }

        void debugPrint(Writer & w, uint32_t indent = 0) const override {
            Node::debugPrint(w, indent);
            w << "\n";
            condition_->debugPrint(w, indent + 2);
            w << "\n";
            body_->debugPrint(w, indent + 2);
        }


    protected:
    
        unique_ptr<Node> condition_;
        unique_ptr<Node> body_;
    };

    class For : public Node {
    public:
        For(Location loc): Node{Kind::For, loc} { }
        Node * container() const { return container_.get(); }
        void setContainer(unique_ptr<Node> child) {
            markAsChild(child.get());
            container_ = std::move(child);
        }

        Node * step() const { return step_.get(); }
        void setStep(unique_ptr<Node> child) {
            markAsChild(child.get());
            step_ = std::move(child);
        }

        Node * body() const { return body_.get(); }
        void setBody(unique_ptr<Node> child) {
            markAsChild(child.get());
            body_ = std::move(child);   
        };

        void prettyPrint(Writer & w, uint32_t indent = 0) const override {
            w << "for " << iter.c_str();
            if (iter2 != Symbol{})
                w << ", " << iter2.c_str();
            w << " in ";
            container_->prettyPrint(w, indent);
            if (step_ != nullptr) {
                w << " step ";
                step_->prettyPrint(w, indent);
            }
            w << " do";
            body_->prettyPrint(w, indent);
        }

        void debugPrint(Writer & w, uint32_t indent = 0) const override {
            Node::debugPrint(w, indent);
            w << " " << iter.c_str();
            if (iter2 != Symbol{}) {
                w << ", " << iter2.c_str();
            }
            w << "\n";
            container_->debugPrint(w, indent + 2);
            if (step_ != nullptr) {
                w << "\n";
                step_->debugPrint(w, indent + 2);
            }
            w << "\n";
            body_->debugPrint(w, indent + 2);
        }

        Symbol iter;
        Symbol iter2;

    protected:
        unique_ptr<Node> container_;
        unique_ptr<Node> step_;
        unique_ptr<Node> body_;
    };

    class Test : public Node {
    public:
        Test(Location loc): Node{Kind::Test, loc} { }
        Node * value() const { return value_.get(); }
        void setValue(unique_ptr<Node> child) {
            markAsChild(child.get());
            value_ = std::move(child);
        }

        void prettyPrint(Writer & w, uint32_t indent = 0) const override {
            w << "test ";
            value_->prettyPrint(w, indent);
        }

    protected:
    
        unique_ptr<Node> value_;
    };

    class Write : public Node {
    public:
        Write(Location loc): Node{Kind::Write, loc} { }

        Node * value() const { return value_.get(); }
        void setValue(unique_ptr<Node> child) {
            markAsChild(child.get());
            value_ = std::move(child);
        }

        void prettyPrint(Writer & w, uint32_t indent = 0) const override {
            w << "write ";
            value_->prettyPrint(w, indent);
        }

    protected:
    
        unique_ptr<Node> value_;
    }; 

} // namespace rckid::dolang::ast

inline void write(Writer & w, rckid::dolang::ast::Kind kind) {
    using namespace rckid::dolang::ast;
    switch (kind) {
        case Kind::ArrayType: w << "ArrayType"; break;
        case Kind::Assign: w << "Assign"; break;
        case Kind::BinaryOp: w << "BinaryOp"; break;
        case Kind::Block: w << "Block"; break;
        case Kind::Break: w << "Break"; break;
        case Kind::Continue: w << "Continue"; break;
        case Kind::Error: w << "Error"; break;
        case Kind::EventType: w << "EventType"; break;
        case Kind::FunDecl: w << "FunDecl"; break;
        case Kind::FunType: w << "FunType"; break;
        case Kind::MapType: w << "MapType"; break;
        case Kind::Program: w << "Program"; break;
        case Kind::Return: w << "Return"; break;
        case Kind::SymbolName: w << "SymbolName"; break;
        case Kind::FieldType: w << "FieldType"; break;
        case Kind::Test: w << "Test"; break;
        case Kind::TupleType: w << "TupleType"; break;
        case Kind::TypeDecl: w << "TypeDecl"; break;
        case Kind::UnaryOp: w << "UnaryOp"; break;
        case Kind::Is: w << "Is"; break;
        case Kind::List: w << "List"; break;
        case Kind::ArrayLiteral: w << "ArrayLiteral"; break;
        case Kind::MapLiteral: w << "MapLiteral"; break;
        case Kind::MapItem: w << "MapItem"; break;
        case Kind::UseDecl: w << "UseDecl"; break;
        case Kind::VarDecl: w << "VarDecl"; break;
        case Kind::Write: w << "Write"; break;
        case Kind::If: w << "If"; break;
        case Kind::While: w << "While"; break;
        case Kind::For: w << "For"; break;
        case Kind::Match: w << "Match"; break;
        case Kind::MatchCase: w << "MatchCase"; break;
        case Kind::Number: w << "Number"; break;
        case Kind::Text: w << "Text"; break;
        case Kind::EnumDecl: w << "EnumDecl"; break;
        default: w << "unknown (id " << static_cast<int>(kind) << ")"; break;
    }
}

inline void write(Writer & w, rckid::dolang::ast::Node const * node) {
    using namespace rckid::dolang::ast;
    node->debugPrint(w);
}

