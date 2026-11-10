#include <rckid/dolang/ast.h>
#include <rckid/dolang/pretty_printer.h>

namespace rckid::dolang::ast {

    void DebugPrinter::header(Node const * node, uint32_t indent) {
        for (uint32_t i = 0; i < indent; ++i)
            w_.putChar(' ');
        w_ << node->kind << "@" << node->loc;
    }

    void DebugPrinter::child(Node const * node, uint32_t indent) {
        w_ << "\n";
        visit(node, indent + 2);
    }

    void DebugPrinter::children(SequenceNode const * node, uint32_t indent) {
        for (uint32_t i = 0, e = node->size(); i < e; ++i)
            child(node->child(i), indent);
    }

    void DebugPrinter::signature(FunType const * node, uint32_t indent) {
        if (node->optionalReturn())
            w_ << " optional";
        if (node->args() != nullptr)
            child(node->args(), indent);
        if (node->returnType() != nullptr)
            child(node->returnType(), indent);
    }

    void DebugPrinter::visit(Node const * node, uint32_t indent) {
        header(node, indent);
        switch (node->kind) {
            // nodes that only print the header
            case Kind::Error:
            case Kind::AnyPattern:
            case Kind::RestPattern:
            case Kind::Break:
            case Kind::Continue:
                break;
            case Kind::UseDecl: {
                w_ << " " << static_cast<UseDecl const *>(node)->module.c_str();
                break;
            }
            case Kind::ArrayType: {
                child(static_cast<ArrayType const *>(node)->elementType(), indent);
                break;
            }
            case Kind::MapType: {
                auto n = static_cast<MapType const *>(node);
                child(n->keyType(), indent);
                child(n->valueType(), indent);
                break;
            }
            case Kind::Test: {
                child(static_cast<Test const *>(node)->value(), indent);
                break;
            }
            case Kind::Write: {
                child(static_cast<Write const *>(node)->value(), indent);
                break;
            }
            case Kind::Program: {
                // the whole program ends with a newline, as it is the root
                children(static_cast<Program const *>(node), indent);
                w_ << "\n";
                break;
            }
            case Kind::Block: {
                children(static_cast<Block const *>(node), indent);
                break;
            }
            case Kind::List:
            case Kind::TupleType:
            case Kind::TupleLiteral:
            case Kind::TuplePattern:
            case Kind::ArrayPattern:
            case Kind::ArrayLiteral:
            case Kind::MapLiteral: {
                auto n = static_cast<List const *>(node);
                if (n->multiLine)
                    w_ << " multi-line";
                children(n, indent);
                break;
            }
            case Kind::Call: {
                // the callee is the first child, followed by the arguments
                auto n = static_cast<Call const *>(node);
                if (n->multiLine)
                    w_ << " multi-line";
                child(n->callee(), indent);
                children(n, indent);
                break;
            }
            case Kind::SymbolName: {
                w_ << " " << static_cast<SymbolName const *>(node)->name.c_str();
                break;
            }
            case Kind::Number: {
                auto n = static_cast<Number const *>(node);
                w_ << " " << n->num.n();
                if (! n->num.isInteger())
                    w_ << "/" << n->num.d();
                break;
            }
            case Kind::Text: {
                w_ << " ";
                PrettyPrinter::print(w_, node, indent);
                break;
            }
            case Kind::VarDecl: {
                auto n = static_cast<VarDecl const *>(node);
                w_ << " " << n->binding << " " << n->name.c_str();
                if (n->type() != nullptr)
                    child(n->type(), indent);
                if (n->value() != nullptr)
                    child(n->value(), indent);
                break;
            }
            case Kind::FunType:
            case Kind::EventType: {
                signature(static_cast<FunType const *>(node), indent);
                break;
            }
            case Kind::Function: {
                auto n = static_cast<Function const *>(node);
                signature(n, indent);
                child(n->body(), indent);
                break;
            }
            case Kind::NamedFunction: {
                auto n = static_cast<NamedFunction const *>(node);
                w_ << " ";
                if (n->isMethod())
                    w_ << n->className().c_str() << ".";
                w_ << n->name().c_str();
                signature(n, indent);
                child(n->body(), indent);
                break;
            }
            case Kind::Arg: {
                auto n = static_cast<Arg const *>(node);
                w_ << " " << n->name().c_str();
                if (n->type() != nullptr)
                    child(n->type(), indent);
                if (n->value() != nullptr)
                    child(n->value(), indent);
                if (n->pattern() != nullptr)
                    child(n->pattern(), indent);
                break;
            }
            case Kind::TypeDecl: {
                auto n = static_cast<TypeDecl const *>(node);
                w_ << " " << n->name().c_str();
                if (n->hasBase())
                    w_ << " extends " << n->base().c_str();
                child(n->fields(), indent);
                break;
            }
            case Kind::EnumDecl: {
                auto n = static_cast<EnumDecl const *>(node);
                w_ << " " << n->name().c_str();
                child(n->values(), indent);
                break;
            }
            case Kind::FieldType: {
                auto n = static_cast<FieldType const *>(node);
                w_ << " " << n->name.c_str();
                if (n->type() != nullptr)
                    child(n->type(), indent);
                break;
            }
            case Kind::Assign: {
                auto n = static_cast<Assign const *>(node);
                w_ << " " << n->op();
                child(n->left(), indent);
                child(n->right(), indent);
                break;
            }
            case Kind::BinaryOp: {
                auto n = static_cast<BinaryOp const *>(node);
                w_ << " " << n->op();
                child(n->left(), indent);
                child(n->right(), indent);
                break;
            }
            case Kind::UnaryOp: {
                auto n = static_cast<UnaryOp const *>(node);
                w_ << " " << n->op();
                child(n->operand(), indent);
                break;
            }
            case Kind::Member: {
                auto n = static_cast<Member const *>(node);
                child(n->object(), indent);
                w_ << "\n";
                for (uint32_t i = 0; i < indent + 2; ++i)
                    w_.putChar(' ');
                w_ << "member: " << n->memberName().c_str();
                break;
            }
            case Kind::NamedValue: {
                auto n = static_cast<NamedValue const *>(node);
                w_ << " " << n->name().c_str();
                child(n->value(), indent);
                break;
            }
            case Kind::Orelse: {
                auto n = static_cast<Orelse const *>(node);
                child(n->call(), indent);
                child(n->value(), indent);
                break;
            }
            case Kind::Paren: {
                child(static_cast<Paren const *>(node)->value(), indent);
                break;
            }
            case Kind::Is: {
                auto n = static_cast<Is const *>(node);
                child(n->value(), indent);
                child(n->pattern(), indent);
                break;
            }
            case Kind::TypePattern: {
                auto n = static_cast<TypePattern const *>(node);
                child(n->typeName(), indent);
                child(n->fields(), indent);
                break;
            }
            case Kind::MapItem: {
                auto n = static_cast<MapItem const *>(node);
                child(n->key(), indent);
                child(n->value(), indent);
                break;
            }
            case Kind::Return: {
                auto n = static_cast<Return const *>(node);
                if (n->value() != nullptr)
                    child(n->value(), indent);
                break;
            }
            case Kind::If: {
                auto n = static_cast<If const *>(node);
                if (n->condition() != nullptr)
                    child(n->condition(), indent);
                if (n->thenCase() != nullptr)
                    child(n->thenCase(), indent);
                if (n->elseCase() != nullptr)
                    child(n->elseCase(), indent);
                break;
            }
            case Kind::While: {
                auto n = static_cast<While const *>(node);
                child(n->condition(), indent);
                child(n->body(), indent);
                break;
            }
            case Kind::For: {
                auto n = static_cast<For const *>(node);
                w_ << " " << n->iter.c_str();
                if (n->iter2 != Symbol{})
                    w_ << ", " << n->iter2.c_str();
                child(n->container(), indent);
                if (n->step() != nullptr)
                    child(n->step(), indent);
                child(n->body(), indent);
                break;
            }
            case Kind::Match: {
                auto n = static_cast<Match const *>(node);
                if (n->value() != nullptr)
                    child(n->value(), indent);
                for (uint32_t i = 0, e = n->numCases(); i < e; ++i)
                    child(n->caseAt(i), indent);
                if (n->elseCase() != nullptr)
                    child(n->elseCase(), indent);
                break;
            }
            case Kind::MatchCase: {
                auto n = static_cast<MatchCase const *>(node);
                child(n->pattern(), indent);
                child(n->body(), indent);
                break;
            }
            default:
                UNREACHABLE;
        }
    }

} // namespace rckid::dolang::ast
