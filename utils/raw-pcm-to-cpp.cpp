#include <cstdio>
#include <iostream>
#include <fstream>
#include <vector>

int main(int argc, char * argv[]) {
    
    std::ifstream input(argv[1], std::ios::binary);

    std::vector<char> bytes(
         (std::istreambuf_iterator<char>(input)),
         (std::istreambuf_iterator<char>()));
    input.close();   
    int i = 0; 
    for (char c : bytes) {
        std::cout << (unsigned)c << ", 0, ";
        if (++i % 16 == 0)
            std::cout << std::endl;
    }
    return EXIT_SUCCESS;
}