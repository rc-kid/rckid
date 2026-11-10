#pragma once

#include <rckid/dolang/ast.h>

namespace rckid::dolang::ast {

    /** Prints the AST as source code, i.e. parsing the output gives the same AST.

        The indent is the indentation of the line the node starts on. Nodes do not print the indentation of their first line, as they may start in the middle of a line, but use the indent for their nested lines (blocks, multi-line lists, etc.). There is no newline after the node, except for the Program, which as the root ends with a newline.
     */
    class PrettyPrinter {
    public:
        static void print(Writer & w, Node const * node, uint32_t indent = 0) {
            PrettyPrinter{w}.visit(node, indent);
        }

        static void print(Writer && w, Node const * node, uint32_t indent = 0) {
            print(static_cast<Writer &>(w), node, indent);
        }

    private:
        explicit PrettyPrinter(Writer & w): w_{w} { }

        void visit(Node const * node, uint32_t indent);

        /** Starts a new line with the given indentation.
         */
        void newLine(uint32_t indent);

        void spaces(uint32_t n);

        /** Prints the items of a list, see List for the details of the single and multi-line forms.
         */
        void listItems(List const * list, uint32_t indent);

        /** Prints the list items surrounded by the open and close brackets.
         */
        void bracketed(List const * list, char open, char close, uint32_t indent);

        /** Prints the arguments in parentheses (if any) and the return type (if any).
         */
        void signature(FunType const * node, uint32_t indent);

        void text(TextLiteral const & text);

        Writer & w_;
    };

} // namespace rckid::dolang::ast
