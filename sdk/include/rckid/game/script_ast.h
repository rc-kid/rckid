#pragma once

#include <vector>

#include <rckid/string.h>
#include <rckid/game/descriptors.h>
#include <rckid/game/runtime.h>
#include <rckid/game/casts.h>

/** Game Engine Script
 */
namespace rckid::game::ast {

    class Node;
    class ObjectNode;
    class IntegerNode;
    class PointNode;
    class MethodCallNode;

    class Visitor {
    public:
        virtual void visit(Node * node) { UNREACHABLE; }
        virtual void visit(ObjectNode * node) = 0;
        virtual void visit(IntegerNode * node) = 0;
        virtual void visit(PointNode * node) = 0;
        virtual void visit(MethodCallNode * node) = 0;
    }; // ast::Visitor

    /** Basic class for all ast nodes, which simply defines the common API, such as the eval method. 
     */
    class Node {
    public:
        virtual ~Node() = default;

        virtual void accept(Visitor & visitor) { visitor.visit(this);}

    }; 

    /** Node pointing to an object.
     
        Evaluates to the object itself. Since the objects are essentially static, the object is stored directly in the value and not via String name. This means that when project is deleted from the game, the AST has to be traversed to determine any affected nodes and they must be dealt with.
     */
    class ObjectNode : public Node {
    public:
        ObjectNode(Value object): object_{object} { }

        ObjectNode(Object * object): object_{Value{object}} { }

        Value object() const { return object_; }

        void accept(Visitor & visitor) override { visitor.visit(this);}

    protected:
        Value object_;    
    }; 

    /** Integer value. 
     */
    class IntegerNode : public Node {
    public:
        IntegerNode(Integer value): value_{value} {}

        Integer value() const { return value_; }

        void accept(Visitor & visitor) override { visitor.visit(this);}

        protected:
        Integer value_;
    };

    /** Point value.
     */
    class PointNode : public Node {
    public:
        PointNode(unique_ptr<Node> x, unique_ptr<Node> y):
            x_{std::move(x)}, y_{std::move(y)} {
        }

        Node * x() const { return x_.get(); }

        Node * y() const { return y_.get(); }

        void accept(Visitor & visitor) override { visitor.visit(this);}

    protected:
        unique_ptr<Node> x_;
        unique_ptr<Node> y_;
    }; 

    /** Method call. 
     
        Consists of an expression evaluating to the object on which the method is called, the method descriptor itself and the list of arguments. 

        TODO more asserts in the evaluation method
     */
    class MethodCallNode : public Node {
    public:

        MethodCallNode(unique_ptr<Node> object, MethodDescriptor const * method):
            object_{std::move(object)}, method_{method} {
        }

        void addArgument(unique_ptr<Node> argument) {
            arguments_.push_back(std::move(argument));
        }

        Node * object() const { return object_.get(); }
        MethodDescriptor const * method() const { return method_; }
        uint32_t numArgs() const { return arguments_.size(); }
        Node * arg(uint32_t index) const { return arguments_[index].get(); }

        void accept(Visitor & visitor) override { visitor.visit(this);}

    private:
        unique_ptr<Node> object_;
        MethodDescriptor const * method_;
        std::vector<unique_ptr<Node>> arguments_;
    };

    /** Function implementation. 
     
        TODO add function arguments for the block editor. For the visual event editor, all functions take no arguments.
     */
    class Function : public Node {
    public:
        Function(unique_ptr<Node> body): body_{std::move(body)} {
        }

        void accept(Visitor & visitor) override { visitor.visit(this);}

    private:
        unique_ptr<Node> body_;
    }; 


    // TODO how to represent the program so that we can serialize & deserialize the game





} // namespace rckid