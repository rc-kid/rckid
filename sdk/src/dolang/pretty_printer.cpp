#include <rckid/dolang/pretty_printer.h>

namespace rckid::dolang::ast {

    void PrettyPrinter::spaces(uint32_t n) {
        for (uint32_t i = 0; i < n; ++i)
            w_.putChar(' ');
    }

    void PrettyPrinter::newLine(uint32_t indent) {
        w_.putChar('\n');
        spaces(indent);
    }

    void PrettyPrinter::listItems(List const * list, uint32_t indent) {
        if (list->multiLine) {
            for (uint32_t i = 0, e = list->size(); i < e; ++i) {
                newLine(indent + 2);
                visit(list->child(i), indent + 2);
            }
            newLine(indent);
        } else {
            for (uint32_t i = 0, e = list->size(); i < e; ++i) {
                if (i != 0)
                    w_ << ", ";
                visit(list->child(i), indent);
            }
        }
    }

    void PrettyPrinter::bracketed(List const * list, char open, char close, uint32_t indent) {
        w_.putChar(open);
        listItems(list, indent);
        w_.putChar(close);
    }

    void PrettyPrinter::signature(FunType const * node, uint32_t indent) {
        if (node->args != nullptr)
            bracketed(node->args.get(), '(', ')', indent);
        if (node->returnType != nullptr) {
            w_ << " -> ";
            if (node->optionalReturn())
                w_ << "optional ";
            visit(node->returnType.get(), indent);
        }
    }

    void PrettyPrinter::text(TextLiteral const & text) {
        w_ << "\"";
        for (char const * p = text.c_str(); *p != '\0'; ++p) {
            switch (*p) {
                case '\n':
                    w_ << "\\n";
                    break;
                case '\r':
                    w_ << "\\r";
                    break;
                case '\t':
                    w_ << "\\t";
                    break;
                case '"':
                    w_ << "\\\"";
                    break;
                case '\\':
                    w_ << "\\\\";
                    break;
                default:
                    w_.putChar(*p);
            }
        }
        w_ << "\"";
    }

    void PrettyPrinter::visit(Node const * node, uint32_t indent) {
        switch (node->kind) {
            case Kind::Error: {
                auto n = static_cast<Error const *>(node);
                w_ << "ERROR at (" << n->loc.line << ":" << n->loc.col << "): " << n->msg.c_str();
                break;
            }
            case Kind::Program: {
                // program does not extra indent and puts each child on its own line
                auto n = static_cast<Program const *>(node);
                for (uint32_t i = 0, e = n->size(); i < e; ++i) {
                    spaces(indent);
                    visit(n->child(i), indent);
                    w_.putChar('\n');
                }
                if (n->size() == 0)
                    w_.putChar('\n');
                break;
            }
            case Kind::Block: {
                auto n = static_cast<Block const *>(node);
                for (uint32_t i = 0, e = n->size(); i < e; ++i) {
                    newLine(indent + 2);
                    visit(n->child(i), indent + 2);
                }
                break;
            }
            case Kind::List:
                listItems(static_cast<List const *>(node), indent);
                break;
            case Kind::TupleType:
            case Kind::TupleLiteral:
            case Kind::TuplePattern:
                bracketed(static_cast<List const *>(node), '(', ')', indent);
                break;
            case Kind::ArrayPattern:
            case Kind::ArrayLiteral:
                bracketed(static_cast<List const *>(node), '[', ']', indent);
                break;
            case Kind::MapLiteral:
                bracketed(static_cast<List const *>(node), '{', '}', indent);
                break;
            case Kind::SymbolName:
                w_ << static_cast<SymbolName const *>(node)->name.c_str();
                break;
            case Kind::Number: {
                auto n = static_cast<Number const *>(node);
                // TODO we want a lot better printing here (floats, etc.)
                w_ << n->num.n();
                if (! n->num.isInteger())
                    w_ << "/" << n->num.d();
                break;
            }
            case Kind::Text:
                text(static_cast<Text const *>(node)->text);
                break;
            case Kind::UseDecl:
                w_ << "use " << static_cast<UseDecl const *>(node)->module.c_str();
                break;
            case Kind::VarDecl: {
                auto n = static_cast<VarDecl const *>(node);
                switch (n->binding) {
                    case TokenKind::kwGlobal:
                        w_ << "global ";
                        break;
                    case TokenKind::kwConst:
                        w_ << "const ";
                        break;
                    case TokenKind::kwLet:
                        w_ << "let ";
                        break;
                    default:
                        UNREACHABLE;
                }
                w_ << n->name.c_str();
                if (n->type != nullptr) {
                    w_ << " : ";
                    visit(n->type.get(), indent);
                }
                if (n->value != nullptr) {
                    w_ << " = ";
                    visit(n->value.get(), indent);
                }
                break;
            }
            case Kind::FunType:
                w_ << "fun";
                signature(static_cast<FunType const *>(node), indent);
                break;
            case Kind::EventType:
                w_ << "event";
                signature(static_cast<FunType const *>(node), indent);
                break;
            case Kind::Function: {
                auto n = static_cast<Function const *>(node);
                w_ << "fun";
                signature(n, indent);
                ASSERT(n->body != nullptr);
                visit(n->body.get(), indent);
                break;
            }
            case Kind::NamedFunction: {
                auto n = static_cast<NamedFunction const *>(node);
                w_ << "fun ";
                if (n->isMethod())
                    w_ << n->className().c_str() << ".";
                w_ << n->name().c_str();
                signature(n, indent);
                ASSERT(n->body != nullptr);
                visit(n->body.get(), indent);
                break;
            }
            case Kind::Arg: {
                auto n = static_cast<Arg const *>(node);
                w_ << n->name().c_str();
                if (n->type != nullptr) {
                    w_ << " : ";
                    visit(n->type.get(), indent);
                }
                if (n->value != nullptr) {
                    w_ << " = ";
                    visit(n->value.get(), indent);
                }
                if (n->pattern != nullptr) {
                    w_ << " is ";
                    visit(n->pattern.get(), indent);
                }
                break;
            }
            case Kind::TypeDecl: {
                auto n = static_cast<TypeDecl const *>(node);
                w_ << "type " << n->name().c_str();
                if (n->hasBase())
                    w_ << " extends " << n->base().c_str();
                w_ << " = ";
                bracketed(n->fields.get(), '(', ')', indent);
                break;
            }
            case Kind::EnumDecl: {
                // the values have no brackets, the multi-line form is a block of values, one per line
                auto n = static_cast<EnumDecl const *>(node);
                w_ << "enum " << n->name().c_str() << " =";
                List const * values = n->values.get();
                if (values->multiLine) {
                    for (uint32_t i = 0, e = values->size(); i < e; ++i) {
                        newLine(indent + 2);
                        visit(values->child(i), indent + 2);
                    }
                } else {
                    w_ << " ";
                    listItems(values, indent);
                }
                break;
            }
            case Kind::ArrayType:
                w_ << "[";
                visit(static_cast<ArrayType const *>(node)->elementType.get(), indent);
                w_ << "]";
                break;
            case Kind::FieldType: {
                auto n = static_cast<FieldType const *>(node);
                w_ << n->name.c_str();
                if (n->type != nullptr) {
                    w_ << " : ";
                    visit(n->type.get(), indent);
                }
                break;
            }
            case Kind::MapType: {
                auto n = static_cast<MapType const *>(node);
                w_ << "{";
                visit(n->keyType.get(), indent);
                w_ << " -> ";
                visit(n->valueType.get(), indent);
                w_ << "}";
                break;
            }
            case Kind::Assign: {
                auto n = static_cast<Assign const *>(node);
                visit(n->left.get(), indent);
                w_ << " " << n->op() << " ";
                visit(n->right.get(), indent);
                break;
            }
            case Kind::BinaryOp: {
                auto n = static_cast<BinaryOp const *>(node);
                visit(n->left.get(), indent);
                if (n->op() == TokenKind::opSquareOpen) {
                    w_ << "[";
                    visit(n->right.get(), indent);
                    w_ << "]";
                } else {
                    w_ << " " << n->op() << " ";
                    visit(n->right.get(), indent);
                }
                break;
            }
            case Kind::UnaryOp: {
                auto n = static_cast<UnaryOp const *>(node);
                w_ << n->op();
                if (n->op() != TokenKind::opSub) // no space for minus sign
                    w_ << " ";
                visit(n->operand.get(), indent);
                break;
            }
            case Kind::Member: {
                auto n = static_cast<Member const *>(node);
                visit(n->object.get(), indent);
                w_ << "." << n->memberName().c_str();
                break;
            }
            case Kind::Call: {
                auto n = static_cast<Call const *>(node);
                visit(n->callee.get(), indent);
                bracketed(n, '(', ')', indent);
                break;
            }
            case Kind::NamedValue: {
                auto n = static_cast<NamedValue const *>(node);
                w_ << n->name().c_str() << " = ";
                visit(n->value.get(), indent);
                break;
            }
            case Kind::Orelse: {
                auto n = static_cast<Orelse const *>(node);
                visit(n->call.get(), indent);
                w_ << " orelse ";
                visit(n->value.get(), indent);
                break;
            }
            case Kind::Paren:
                w_ << "(";
                visit(static_cast<Paren const *>(node)->value.get(), indent);
                w_ << ")";
                break;
            case Kind::Is: {
                auto n = static_cast<Is const *>(node);
                visit(n->value.get(), indent);
                w_ << " is ";
                visit(n->pattern.get(), indent);
                break;
            }
            case Kind::As: {
                auto n = static_cast<As const *>(node);
                visit(n->source.get(), indent);
                w_ << " as ";
                visit(n->targetType.get(), indent);
                if (n->orElse != nullptr) {
                    w_ << " orelse ";
                    visit(n->orElse.get(), indent);
                }
                break;
            }
            case Kind::TypePattern: {
                auto n = static_cast<TypePattern const *>(node);
                visit(n->typeName.get(), indent);
                visit(n->fields.get(), indent);
                break;
            }
            case Kind::AnyPattern:
                w_ << "_";
                break;
            case Kind::RestPattern:
                w_ << "*";
                break;
            case Kind::MapItem: {
                auto n = static_cast<MapItem const *>(node);
                visit(n->key.get(), indent);
                w_ << " -> ";
                visit(n->value.get(), indent);
                break;
            }
            case Kind::Break:
                w_ << "break";
                break;
            case Kind::Continue:
                w_ << "continue";
                break;
            case Kind::Return: {
                auto n = static_cast<Return const *>(node);
                w_ << "return";
                if (n->value != nullptr) {
                    w_ << " ";
                    visit(n->value.get(), indent);
                }
                break;
            }
            case Kind::If: {
                auto n = static_cast<If const *>(node);
                w_ << "if ";
                visit(n->condition.get(), indent);
                w_ << " then";
                visit(n->thenCase.get(), indent);
                if (n->elseCase != nullptr) {
                    // else case is on a new line, indented same as the if
                    newLine(indent);
                    w_ << "else";
                    visit(n->elseCase.get(), indent);
                }
                break;
            }
            case Kind::Match: {
                auto n = static_cast<Match const *>(node);
                w_ << "match ";
                visit(n->value.get(), indent);
                w_ << " when";
                for (uint32_t i = 0, e = n->numCases(); i < e; ++i)
                    visit(n->caseAt(i), indent);
                if (n->elseCase != nullptr) {
                    // match statement has the else case on its own line, at the indentation of the cases, while the expression is all on a single line
                    if (n->elseCase->kind == Kind::Block) {
                        newLine(indent + 2);
                        w_ << "else";
                        visit(n->elseCase.get(), indent + 2);
                    } else {
                        w_ << " else ";
                        visit(n->elseCase.get(), indent);
                    }
                }
                break;
            }
            case Kind::MatchCase: {
                // case of the match statement is on its own line, indented from the match, while the expression is all on a single line
                auto n = static_cast<MatchCase const *>(node);
                if (n->body->kind == Kind::Block) {
                    newLine(indent + 2);
                    w_ << "is ";
                    visit(n->pattern.get(), indent);
                    w_ << " then";
                    visit(n->body.get(), indent + 2);
                } else {
                    w_ << " is ";
                    visit(n->pattern.get(), indent);
                    w_ << " then ";
                    visit(n->body.get(), indent);
                }
                break;
            }
            case Kind::While: {
                auto n = static_cast<While const *>(node);
                w_ << "while ";
                visit(n->condition.get(), indent);
                w_ << " do";
                visit(n->body.get(), indent);
                break;
            }
            case Kind::For: {
                auto n = static_cast<For const *>(node);
                w_ << "for " << n->iter.c_str();
                if (n->iter2 != Symbol{})
                    w_ << ", " << n->iter2.c_str();
                w_ << " in ";
                visit(n->container.get(), indent);
                if (n->step != nullptr) {
                    w_ << " step ";
                    visit(n->step.get(), indent);
                }
                w_ << " do";
                visit(n->body.get(), indent);
                break;
            }
            case Kind::Test:
                w_ << "test ";
                visit(static_cast<Test const *>(node)->value.get(), indent);
                break;
            case Kind::Write:
                w_ << "write ";
                visit(static_cast<Write const *>(node)->value.get(), indent);
                break;
            default:
                UNREACHABLE;
        }
    }

} // namespace rckid::dolang::ast
