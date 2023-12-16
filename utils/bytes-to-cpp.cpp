#include <cstdio>
#include <iostream>
#include <fstream>
#include <vector>

/** Takes raw bytes and converts them to a sequence of numbers that can easily be embedded as C array of uint8_t.
 */
int main(int argc, char * argv[]) {
    
    std::ifstream input(argv[1], std::ios::binary);

    std::vector<char> bytes(
         (std::istreambuf_iterator<char>(input)),
         (std::istreambuf_iterator<char>()));
    input.close();   
    int i = 0; 
    for (char c : bytes) {
        std::cout << (unsigned)c << ",";
        if (++i % 16 == 0)
            std::cout << std::endl;
    }
    return EXIT_SUCCESS;
}