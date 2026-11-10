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
        Function,
        NamedFunction,
        Arg,
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
        Member,
        Call,
        NamedValue,
        TupleLiteral,
        Orelse,
        Paren,
        Is,
        TuplePattern,
        ArrayPattern,
        TypePattern,
        AnyPattern,
        RestPattern,
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

    protected:

        void markAsChild(Node * child) {
            if (child == nullptr)
                return;
            ASSERT(child->parent_ == nullptr);
            child->parent_ = this;
        }

        /** Clears the parent of a child that is being moved out of this node, so that it can be adopted by another one. 
         */
        void unmarkAsChild(Node * child) {
            if (child == nullptr)
                return;
            ASSERT(child->parent_ == this);
            child->parent_ = nullptr;
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
    }; 

    class SequenceNode : public Node {
    public:
        SequenceNode(Kind kind, Location loc): Node{kind, loc} { }

        void addChild(unique_ptr<Node> child) {
            markAsChild(child.get());
            children_.push_back(std::move(child));
        }

        uint32_t size() const { return static_cast<uint32_t>(children_.size()); }

        Node * child(uint32_t i) const { return children_[i].get(); }

        /** Removes the i-th child from the sequence and returns it, without a parent.
         */
        unique_ptr<Node> releaseChild(uint32_t i) {
            unique_ptr<Node> result = std::move(children_[i]);
            children_.erase(children_.begin() + i);
            unmarkAsChild(result.get());
            return result;
        }

    protected:
        
        std::vector<std::unique_ptr<Node>> children_;
    }; 

    class Program : public SequenceNode {
    public:
        Program(Location loc): SequenceNode{Kind::Program, loc} { }
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
    };

    class Number : public Node {
    public:
        
        Number(Location loc, Num num): Node{Kind::Number, loc}, num{num} { }

        Number(Token const & t): 
            Node{Kind::Number, t.loc}, 
            num{t.asNum()} {
        }

        Num num;
    };         

    class Text : public Node {
    public:
        
        Text(Location loc, TextLiteral text): Node{Kind::Text, loc}, text{text} { }

        Text(Token const & t): 
            Node{Kind::Text, t.loc}, 
            text{t.asText()} {
        }
        
        TextLiteral text;
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

    protected:

        unique_ptr<Node> type_;
        unique_ptr<Node> value_;
    };

    /** Function type, i.e. the signature of a function without a body: arguments (a List of Arg nodes) and optional return type. Base class for function literals, named functions and event types, which all print and store the signature the same way. 

        The arguments list is null for function literals with implicit signature (just `fun` followed by the body), where the whole signature comes from the expected type.
     */
    class FunType : public Node {
    public:
        FunType(Location loc): Node{Kind::FunType, loc} { }

        Node * returnType() const { return returnType_.get(); }
        void setReturnType(unique_ptr<Node> child) {
            markAsChild(child.get());
            returnType_ = std::move(child);
        }

        List * args() const { return args_.get(); }
        void setArgs(unique_ptr<List> child) {
            markAsChild(child.get());
            args_ = std::move(child);
        }

        bool optionalReturn() const { return optionalReturn_; }
        void setOptionalReturn(bool optional) { optionalReturn_ = optional; }

    protected:

        FunType(Location loc, Kind kind): Node{kind, loc} { }

        unique_ptr<List> args_;
        unique_ptr<Node> returnType_;
        bool optionalReturn_ = false;
    };

    /** Function literal (lambda), i.e. function type with body. 
     */
    class Function : public FunType {
    public:
        Function(Location loc): FunType{loc, Kind::Function} { }

        Block * body() const { return body_.get(); }
        void setBody(unique_ptr<Block> child) {
            markAsChild(child.get());
            body_ = std::move(child);
        }

    protected:

        Function(Location loc, Kind kind): FunType{loc, kind} { }

        unique_ptr<Block> body_;
    };

    /** Named function, or method declaration. Unlike function literals, always has the arguments list.
     */
    class NamedFunction : public Function {
    public:
        NamedFunction(Location loc, Symbol name): Function{loc, Kind::NamedFunction}, name_{name} {}

        Symbol name() const { return name_; }
        Symbol className() const { return className_; }

        bool isMethod() const { return className_ != Symbol{}; }

        void setMethodName(Symbol methodName) {
            className_ = name_;
            name_ = methodName; 
        }

    protected:
        Symbol name_;
        Symbol className_;
    };

    /** Named argument (or field). Used for arguments of named functions, function and event types (name, type and default value), arguments of function literals (name, type and the pattern of event handlers after `is`) and fields of type declarations (name, type and default value). Which of the parts can be present is decided by the parser rule that creates the argument. 
     */
    class Arg : public Node {
    public:
        Arg(Token const & name): Node{Kind::Arg, name.loc}, name_{name.asSymbol()} { }
        Arg(Location loc, Symbol name): Node{Kind::Arg, loc}, name_{name} { }

        Symbol name() const { return name_; }

        Node * type() const { return type_.get(); }
        void setType(unique_ptr<Node> child) {
            markAsChild(child.get());
            type_ = std::move(child);
        }

        /** Default value. 
         */
        Node * value() const { return value_.get(); }
        void setValue(unique_ptr<Node> child) {
            markAsChild(child.get());
            value_ = std::move(child);
        }

        /** Pattern the argument must match for event handlers. 
         */
        Node * pattern() const { return pattern_.get(); }
        void setPattern(unique_ptr<Node> child) {
            markAsChild(child.get());
            pattern_ = std::move(child);
        }

    protected:
        Symbol name_;
        unique_ptr<Node> type_;
        unique_ptr<Node> value_;
        unique_ptr<Node> pattern_;
    };

    /** Type declaration, i.e. the name, the optional base type it extends and the list of fields (Arg nodes). The constructor is also in the fields list, as an Arg named `constructor` whose value is the function literal. As constructor is a keyword, it cannot clash with a field name.
     */
    class TypeDecl : public Node {
    public:
        TypeDecl(Location loc, Symbol name): Node{Kind::TypeDecl, loc}, name_{name} { } 

        Symbol name() const { return name_; }

        /** Name of the type this one extends, or empty symbol if none. 
         */
        Symbol base() const { return base_; }
        void setBase(Symbol base) { base_ = base; }
        bool hasBase() const { return base_ != Symbol{}; }

        List * fields() const { return fields_.get(); }
        void setFields(unique_ptr<List> child) {
            markAsChild(child.get());
            fields_ = std::move(child);
        }

    private:
        Symbol name_;
        Symbol base_;
        unique_ptr<List> fields_;
    };

    /** Enum declaration, i.e. the name and the list of values (SymbolName nodes). The values list has no brackets, its multi-line form is printed as a block of values, one per line. 
     */
    class EnumDecl : public Node {
    public:
        EnumDecl(Location loc, Symbol name): Node{Kind::EnumDecl, loc}, name_{name} { }

        Symbol name() const { return name_; }

        List * values() const { return values_.get(); }
        void setValues(unique_ptr<List> child) {
            markAsChild(child.get());
            values_ = std::move(child);
        }

    private:
        Symbol name_;
        unique_ptr<List> values_;
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
    };

    class TupleType : public List {
    public:
        TupleType(Location loc): List{Kind::TupleType, loc} { }
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

    protected:
        unique_ptr<Node> keyType_;
        unique_ptr<Node> valueType_;
    };

    /** Event type is a function type without the return value, printed with the `event` keyword (see FunType).
     */
    class EventType : public FunType {
    public:
        EventType(Location loc): FunType{loc, Kind::EventType} { }
    };

    class Assign : public Node {
    public:
        Assign(Token const & op): 
            Node{Kind::Assign, op.loc},
            op_{op.kind}
        {
            ASSERT(op.isAssignmentOperator());
        }

        TokenKind op() const { return op_; }

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

        BinaryOp(Token const & op, unique_ptr<Node> left): 
            Node{Kind::BinaryOp, op.loc},
            op_{op.kind},
            left_{std::move(left)}
        { 
            ASSERT(op.isBinaryOperator());
            markAsChild(left_.get());
        }

        TokenKind op() const { return op_; }

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

        TokenKind op() const { return op_; }

        Node * operand() const { return operand_.get(); }
        void setOperand(unique_ptr<Node> child) {
            markAsChild(child.get());
            operand_ = std::move(child);
        }

    protected:
        TokenKind op_;
        unique_ptr<Node> operand_;
    }; 

    class Member : public Node {
    public:
        Member(Location loc, unique_ptr<Node> object, Symbol memberName): Node{Kind::Member, loc}, object_(std::move(object)), memberName_(memberName) { 
            markAsChild(object_.get());
        }

        Node * object() const { return object_.get(); }
        void setObject(unique_ptr<Node> child) {
            markAsChild(child.get());
            object_ = std::move(child);
        }

        Symbol memberName() const { return memberName_; }
        void setMemberName(Symbol name) { memberName_ = name; }

    private:
        unique_ptr<Node> object_;
        Symbol memberName_;
    }; 

    /** Call of the callee with the arguments, which are the list items. Positional arguments are plain expressions, named arguments are NamedValue nodes. 
     */
    class Call : public List {
    public:
        Call(Location loc, unique_ptr<Node> callee): List{Kind::Call, loc}, callee_{std::move(callee)} { 
            markAsChild(callee_.get());
        }

        Node * callee() const { return callee_.get(); }
        void setCallee(unique_ptr<Node> child) {
            markAsChild(child.get());
            callee_ = std::move(child);
        }

    private:
        unique_ptr<Node> callee_;
    };

    /** `name = value`, used for named call arguments and for the items of tuple literals.
     */
    class NamedValue : public Node {
    public:
        NamedValue(Location loc, Symbol name): Node{Kind::NamedValue, loc}, name_{name} { }

        Symbol name() const { return name_; }

        Node * value() const { return value_.get(); }
        void setValue(unique_ptr<Node> child) {
            markAsChild(child.get());
            value_ = std::move(child);
        }

    private:
        Symbol name_;
        unique_ptr<Node> value_;
    };

    /** Tuple literal, i.e. list of NamedValue items in parentheses. 
     */
    class TupleLiteral : public List {
    public:
        TupleLiteral(Location loc): List{Kind::TupleLiteral, loc} { }
    };

    /** `CALL orelse VALUE`, the value is used when the call returns no value. 
     */
    class Orelse : public Node {
    public:
        Orelse(Location loc, unique_ptr<Node> call): Node{Kind::Orelse, loc}, call_{std::move(call)} { 
            markAsChild(call_.get());
        }

        Node * call() const { return call_.get(); }

        Node * value() const { return value_.get(); }
        void setValue(unique_ptr<Node> child) {
            markAsChild(child.get());
            value_ = std::move(child);
        }

    private:
        unique_ptr<Node> call_;
        unique_ptr<Node> value_;
    };

    /** Parenthesized expression. Kept in the tree so that the pretty print preserves the grouping (and with it the meaning) of the source. 
     */
    class Paren : public Node {
    public:
        Paren(Location loc): Node{Kind::Paren, loc} { }

        Node * value() const { return value_.get(); }
        void setValue(unique_ptr<Node> child) {
            markAsChild(child.get());
            value_ = std::move(child);
        }

    private:
        unique_ptr<Node> value_;
    };

    /** The `X is PATTERN` test. Evaluates to bool. The negated form (`X is not PATTERN`) is the test against negated pattern, i.e. the `not` is part of the pattern.
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

    protected:
        unique_ptr<Node> value_;
        unique_ptr<Node> pattern_;
    };

    /** Tuple pattern, i.e. list of fields (Arg nodes with name and pattern) in parentheses, optionally ending with the rest pattern (`*`). Also used for the fields of the type pattern.
     */
    class TuplePattern : public List {
    public:
        TuplePattern(Location loc): List{Kind::TuplePattern, loc} { }
    };

    /** Array pattern, i.e. list of patterns matching the array elements by their positions, optionally ending with the rest pattern (`*`). 
     */
    class ArrayPattern : public List {
    public:
        ArrayPattern(Location loc): List{Kind::ArrayPattern, loc} { }
    };

    /** Type pattern, i.e. the (qualified) name of an object type (SymbolName, or Member) followed by the tuple pattern of its fields (`Point(x is 1, *)`). 
     */
    class TypePattern : public Node {
    public:
        TypePattern(Location loc, unique_ptr<Node> typeName): Node{Kind::TypePattern, loc}, typeName_{std::move(typeName)} {
            markAsChild(typeName_.get());
        }

        Node * typeName() const { return typeName_.get(); }

        TuplePattern * fields() const { return fields_.get(); }
        void setFields(unique_ptr<TuplePattern> child) {
            markAsChild(child.get());
            fields_ = std::move(child);
        }

    private:
        unique_ptr<Node> typeName_;
        unique_ptr<TuplePattern> fields_;
    };

    /** The `_` (any value, Kind::AnyPattern) and `*` (rest of the tuple, or array, Kind::RestPattern) patterns. 
     */
    class Wildcard : public Node {
    public:
        Wildcard(Kind kind, Location loc): Node{kind, loc} {
            ASSERT(kind == Kind::AnyPattern || kind == Kind::RestPattern);
        }
    };

    class ArrayLiteral : public List {
    public:
        ArrayLiteral(Location loc): List{Kind::ArrayLiteral, loc} { }
    };

    class MapLiteral : public List {
    public:
        MapLiteral(Location loc): List{Kind::MapLiteral, loc} { }
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
    };

    class Continue : public Node {
    public:
        Continue(Location loc): Node{Kind::Continue, loc} { }
        Continue(Token const & t): 
            Node{Kind::Continue, t.loc} 
        {
            ASSERT(t == TokenKind::kwContinue);
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

    protected:
        unique_ptr<Node> value_;
    };

    class If : public Node {
    public:
        If(Location loc): Node{Kind::If, loc} { }

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

        uint32_t numCases() const { return static_cast<uint32_t>(cases_.size()); }
        Node * caseAt(uint32_t i) const { return cases_[i].get(); }

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

    protected:
    
        unique_ptr<Node> value_;
    }; 

    /** Prints the AST as an indented tree, one node per line with its kind, location and the attributes not represented by the children, which follow on their own lines indented from the parent. Used for tests and debugging where the structure of the tree matters. 

        There is no newline after the last child, so that the nodes can nest, except for the Program, which as the root ends with a newline. 
     */
    class DebugPrinter {
    public:
        static void print(Writer & w, Node const * node, uint32_t indent = 0) {
            DebugPrinter{w}.visit(node, indent);
        }

        static void print(Writer && w, Node const * node, uint32_t indent = 0) {
            print(static_cast<Writer &>(w), node, indent);
        }

    private:
        explicit DebugPrinter(Writer & w): w_{w} { }

        void visit(Node const * node, uint32_t indent);

        /** Prints the indentation, kind and location of the node. 
         */
        void header(Node const * node, uint32_t indent);

        /** Prints the child on a new line, indented from the parent. 
         */
        void child(Node const * node, uint32_t indent);

        void children(SequenceNode const * node, uint32_t indent);

        /** Prints the optional return flag on the node's line, followed by the arguments and the return type as children. 
         */
        void signature(FunType const * node, uint32_t indent);

        Writer & w_;
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
        case Kind::Function: w << "Function"; break;
        case Kind::NamedFunction: w << "NamedFunction"; break;
        case Kind::Arg: w << "Arg"; break;
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
        case Kind::Member: w << "Member"; break;
        case Kind::Call: w << "Call"; break;
        case Kind::NamedValue: w << "NamedValue"; break;
        case Kind::TupleLiteral: w << "TupleLiteral"; break;
        case Kind::Orelse: w << "Orelse"; break;
        case Kind::Paren: w << "Paren"; break;
        case Kind::Is: w << "Is"; break;
        case Kind::TuplePattern: w << "TuplePattern"; break;
        case Kind::ArrayPattern: w << "ArrayPattern"; break;
        case Kind::TypePattern: w << "TypePattern"; break;
        case Kind::AnyPattern: w << "AnyPattern"; break;
        case Kind::RestPattern: w << "RestPattern"; break;
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
    rckid::dolang::ast::DebugPrinter::print(w, node);
}
