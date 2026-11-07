#pragma once

#include <rckid/rckid.h>
#include <rckid/dolang/lexer.h>

namespace rckid::dolang::ast {
    
    enum class Kind {
        Error,
        Program,
        Block,
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
        BinaryOp,
        UnaryOp,

        Break,
        Continue,
        Test,
        Write,
        Return,

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
            bool first = true;
            for (auto const & child : children_) {
                if (first)
                    first = false;
                else
                    w.putChar('\n');
                for (uint32_t i = 0; i < indent; ++i)
                    w.putChar(' ');
                child->prettyPrint(w, indent + 2);
            }
        }

        void debugPrint(Writer & w, uint32_t indent = 0) const override {
            Node::debugPrint(w, indent);
            w << "\n";
            for (auto const & child : children_) {
                child->debugPrint(w, indent + 2);
                w << "\n";
            }
        }

    protected:
        
        std::vector<std::unique_ptr<Node>> children_;
    }; 

    class Program : public SequenceNode {
    public:
        Program(Location loc): SequenceNode{Kind::Program, loc} { }

        void prettyPrint(Writer & w, uint32_t indent = 0) const override {
            SequenceNode::prettyPrint(w, indent);
            w << "\n";
        }
    };

    class Block : public SequenceNode {
    public:
        Block(Location loc): SequenceNode{Kind::Block, loc} { }
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
            w << name.c_str();
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
            w << binding << " " << name.c_str();
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
            w << "\n";
            body_->prettyPrint(w, indent + 2);
        }

        void debugPrint(Writer & w, uint32_t indent = 0) const override {
            Node::debugPrint(w, indent);
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

        void prettyPrint(Writer & w, uint32_t indent = 0) const override {
            UNIMPLEMENTED;
        }
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

    class TupleType : public SequenceNode {
    public:
        TupleType(Location loc): SequenceNode{Kind::TupleType, loc} { }

        void prettyPrint(Writer & w, uint32_t indent = 0) const override {
            UNIMPLEMENTED;
        }
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
            w << op_ << "\n";
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
            w << op_ << "\n";
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
            w << op_ << "\n";
            operand_->debugPrint(w, indent + 2);
        }

    protected:
        TokenKind op_;
        unique_ptr<Node> operand_;
    }; 



    class Break : public Node {
    public:
        Break(Location loc): Node{Kind::Break, loc} { }

        void prettyPrint(Writer & w, uint32_t indent = 0) const override {
            w << "break";
        }
    };

    class Continue : public Node {
    public:
        Continue(Location loc): Node{Kind::Continue, loc} { }

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
        case Kind::Test: w << "Test"; break;
        case Kind::TupleType: w << "TupleType"; break;
        case Kind::TypeDecl: w << "TypeDecl"; break;
        case Kind::UnaryOp: w << "UnaryOp"; break;
        case Kind::UseDecl: w << "UseDecl"; break;
        case Kind::VarDecl: w << "VarDecl"; break;
        case Kind::Write: w << "Write"; break;
        default: w << "unknown (id " << static_cast<int>(kind) << ")"; break;
    }
}

inline void write(Writer & w, rckid::dolang::ast::Node const * node) {
    using namespace rckid::dolang::ast;
    node->debugPrint(w);
}

