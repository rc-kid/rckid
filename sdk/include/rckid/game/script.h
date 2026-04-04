#pragma once

#include <vector>

#include <rckid/string.h>
#include <rckid/game/descriptors.h>
#include <rckid/game/runtime.h>
#include <rckid/game/casts.h>

/** Game Engine Script
 */
namespace rckid::game::ast {

    /** Basic class for all ast nodes, which simply defines the common API, such as the eval method. 
     */
    class Node {
    public:
        virtual ~Node() = default;

        virtual Value eval() const = 0;

    }; 

    /** Node pointing to an object.
     
        Evaluates to the object itself. Since the objects are essentially static, the object is stored directly in the value and not via String name. This means that when project is deleted from the game, the AST has to be traversed to determine any affected nodes and they must be dealt with.
     */
    class ObjectNode : public Node {
    public:
        ObjectNode(Value result);

        Value eval() const {
            return result_;
        }

    protected:
        Value result_;    
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

        Value eval() const override {
            Value object = object_->eval();
            ASSERT(object.type() == Object::descriptor);
            Object * obj = as<Object*>(object);
            ASSERT(method_->numArgs() == arguments_.size());
            Value args[method_->numArgs()];
            for (size_t i = 0; i < arguments_.size(); ++i)
                args[i] = arguments_[i]->eval();
            return method_->call(obj, args);
        }

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

        Value eval() const override {
            return body_->eval();
        }

    private:
        unique_ptr<Node> body_;
    }; 


    // TODO how to represent the program so that we can serialize & deserialize the game


} // namespace rckid