#pragma once

#include <rckid/game/script_ast.h>
#include <rckid/game/engine.h>

namespace rckid::game {

    /** Evaluates the AST nodes. 
     */
    class Evaluator : public ast::Visitor {
    public:

        static Value eval(ast::Node * node, Engine * engine) {
            Evaluator e{engine};
            e.eval(node);
            return e.result_;
        }

        void visit(ast::ObjectNode * node) override {
            result_ = node->object();
        }
        void visit(ast::IntegerNode * node) override {
            result_ = Value{node->value()};
        }

        void visit(ast::PointNode * node) override {
            Value x = eval(node->x());
            Value y = eval(node->y());
            ASSERT(x.kind() == Type::Kind::Integer);
            ASSERT(y.kind() == Type::Kind::Integer);
            result_ = Value{Point{as<Integer>(x), as<Integer>(y)}};
        }

        void visit(ast::MethodCallNode * node) override {
            Value object = eval(node->object());
            ASSERT(object.type() == Object::descriptor);
            Object * obj = as<Object*>(object);
            MethodDescriptor const * method = node->method();
            ASSERT(method->numArgs() == node->numArgs());
            Value args[method->numArgs()];
            for (size_t i = 0, e = node->numArgs(); i < e; ++i)
                args[i] = eval(node->arg(i));
            result_ = method->call(obj, args);
        }

    private:

        Evaluator(Engine * engine): engine_{engine} { }

        Value eval(ast::Node * node) {
            node->accept(*this);
            return result_;
        }

        Value result_;
        Engine * engine_;

    }; // Evaluator

    /** Serializes the AST nodes. 
     
        TODO the AST nodes will serialize directly to the DSL used for the final pre-SDK tier. That way the serialization and parsing can be shared across all tiers.
     */
    class Serializer : public ast::Visitor {

    }; // Serializer

    /** Parses script into AST nodes. 
     
        TODO
     */
    class Parser {

    }; // Parser

} // namespace rckid::game