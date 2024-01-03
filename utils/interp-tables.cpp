#include <cstdio>
#include <iostream>
#include <cmath>

int main(int argc, char * argv[]) {
    for (int i = 0; i <= 50; ++i)
        std::cout << static_cast<int>(sin(i / 100.0 * M_PI) * 100) << ", ";  
    return EXIT_SUCCESS;
}