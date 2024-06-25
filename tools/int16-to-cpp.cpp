#include <cstdio>
#include <iostream>
#include <fstream>
#include <vector>

/** Takes raw int16_t and converts them to a sequence of numbers that can easily be embedded as C array of uint16_t.
 */
int main(int argc, char * argv[]) {
    
    std::ifstream input(argv[1], std::ios::binary);

    int i = 0; 
    while (! input.eof()) {
        int16_t value;
        input.read(reinterpret_cast<char*>(&value), sizeof(value));
        unsigned x = (int)value + 32768;
        //unsigned x = *reinterpret_cast<unsigned char*>(& c);
        std::cout << x << ",";
        if (++i % 16 == 0)
            std::cout << std::endl;
    }
    return EXIT_SUCCESS;
}