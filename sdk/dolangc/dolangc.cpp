#include <iostream>

#include <rckid/rckid.h>
#include <rckid/dolang/parser.h>
#include <rckid/dolang/pretty_printer.h>

namespace rckid::hal::device {
    void initializeNoWindow();
}

using namespace rckid;

unique_ptr<uint8_t> readFile(char const * path, size_t & size) {
    std::ifstream f{path, std::ios::binary | std::ios::ate};
    if (!f)
        return nullptr;
    size = static_cast<size_t>(f.tellg());
    f.seekg(0, std::ios::beg);
    unique_ptr<uint8_t> data = unique_ptr<uint8_t>(new uint8_t[size]);
    if (!f.read(reinterpret_cast<char *>(data.get()), size)) {
        return nullptr;
    }
    return data;
}

/** doLang standalone compiler for language experiments & testing.
 
    Usage: dolangc <source file>
 */
int main(int argc, char * argv[]) {
    hal::device::initializeNoWindow();
    std::cout << "dolang compiler version 0.1.0" << std::endl;
    if (argc != 2) {
        std::cerr << "Usage: dolangc <source file>" << std::endl;
        return 1;
    }
    // process the source file - first load the entire file into memory, then create memory stream for the dolang frontend to read from
    size_t size = 0;
    auto file = readFile(argv[1], size);
    if (file == nullptr) {
        std::cerr << "Error: Could not open file " << argv[1] << std::endl;
        return 1;
    }
    MemoryStream s{std::move(file), static_cast<uint32_t>(size)};

    dolang::Parser parser{s};
    
    auto ast = parser.parse();
    if (ast->kind == dolang::ast::Kind::Error) {
        StringBuilder sb;
        sb.writer() << "Parse error " << ast->loc << ": " << static_cast<dolang::ast::Error *>(ast.get())->msg.c_str();
        std::cerr << sb.str().c_str() << std::endl;        
        return 1;
    }

    StringBuilder sb;
    dolang::ast::PrettyPrinter::print(sb.writer(), ast.get());
    std::cout << sb.str().c_str() << std::endl;

    return 0;
}    
