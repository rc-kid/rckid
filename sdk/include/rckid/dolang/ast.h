#pragma once

#include <rckid/rckid.h>
#include <rckid/dolang/lexer.h>

namespace rckid::dolang::ast {
    
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

        void writeChild(Writer & w, Node const * child, uint32_t indent) const {
            child->writeInto(w, indent);
        }

        Node * parent_ = nullptr;

    private:

        virtual void writeInto(Writer & w, uint32_t indent) const = 0;

        friend inline void write(Writer & w, Node const & node) {
            node.writeInto(w, 0);
        }

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
    
    private:

        void writeInto(Writer & w, uint32_t indent) const override {
            for (auto const & child : children_) {
                for (uint32_t i = 0; i < indent; ++i)
                    w.putChar(' ');
                writeChild(w, child.get(), indent + 2);
                w.putChar('\n');
            }
        }

    }; 

    class Program : public SequenceNode {
    public:
        Program(Location loc): SequenceNode{Kind::Program, loc} { }
    };

    class SymbolName : public Node {
    public:
        SymbolName(Location loc, Symbol name): Node{Kind::SymbolName, loc}, name{name} { }
        Symbol name;

    private:

        void writeInto(Writer & w, uint32_t indent) const override {
            w << name.c_str();
        }
    };

    class UseDecl : public Node {
    public:
        UseDecl(Location loc, Symbol module): Node{Kind::UseDecl, loc}, module{module} { }
        Symbol module;
    
    private:
        
    void writeInto(Writer & w, uint32_t indent) const override {
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

    protected:

        unique_ptr<Node> type_;
        unique_ptr<Node> value_;

    private:

        void writeInto(Writer & w, uint32_t indent) const override {
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
                writeChild(w, type_.get(), indent);
            }
            if (value_ != nullptr) {
                w << " = ";
                writeChild(w, value_.get(), indent);
            }
        }

    };

    class FunDecl : public Node {
    public:
        FunDecl(Location loc): Node{Kind::FunDecl, loc} { }

    private:
    
        void writeInto(Writer & w, uint32_t indent) const override {
            UNIMPLEMENTED;
        }
    };

    class TypeDecl : public Node {
    public:
        TypeDecl(Location loc): Node{Kind::TypeDecl, loc} { } 

    private:
    
        void writeInto(Writer & w, uint32_t indent) const override {
            UNIMPLEMENTED;
        }
    };

    class EnumDecl : public Node {
    public:
        EnumDecl(Location loc): Node{Kind::EnumDecl, loc} { }

    private:
    
        void writeInto(Writer & w, uint32_t indent) const override {
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

    protected:
        unique_ptr<Node> elementType_;

    private:
    
        void writeInto(Writer & w, uint32_t indent) const override {
            w << "[";
            writeChild(w, elementType_.get(), indent);
            w << "]";
        }
    };

    class TupleType : public SequenceNode {
    public:
        TupleType(Location loc): SequenceNode{Kind::TupleType, loc} { }

    private:
    
        void writeInto(Writer & w, uint32_t indent) const override {
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

    private:
        unique_ptr<Node> keyType_;
        unique_ptr<Node> valueType_;

    private:
    
        void writeInto(Writer & w, uint32_t indent) const override {
            w << "{";
            writeChild(w, keyType_.get(), indent);
            w << " -> ";
            writeChild(w, valueType_.get(), indent);
            w << "}";
        }

    };

    class FunType : public Node {

    private:
    
        void writeInto(Writer & w, uint32_t indent) const override {
            UNIMPLEMENTED;
        }

    };

    class EventType : public Node {


    private:
    
        void writeInto(Writer & w, uint32_t indent) const override {
            UNIMPLEMENTED;
        }
    };



    class Assign : public Node {
    public:
        Assign(Location loc): Node{Kind::Assign, loc} { }

    private:
    
        void writeInto(Writer & w, uint32_t indent) const override {
            UNIMPLEMENTED;
        }
    };



} // namespace rckid::dolang::ast