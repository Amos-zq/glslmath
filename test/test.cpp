
#include <iostream>

#include "../include/math.hpp"

#define CHECK(X) if (!(X)) { std::cout << #X << "\n"; return 1; }

int main() {
    using namespace glslmath;
    {
        vec2 a(1, 2);
        CHECK(a[0] == 1);
        CHECK(a[1] == 2);
    }
    {
        mat2 a(1, 2, 3, 4);
        CHECK(a[0] == vec2(1, 2));
        CHECK(a[1] == vec2(3, 4));
    }
    {
        // see https://en.wikibooks.org/wiki/GLSL_Programming/Vector_and_Matrix_Operations
        mat2 a = mat2(1, 2,  3, 4);
        mat2 b = mat2(10, 20,  30, 40);
        mat2 c = a * b;
        CHECK(c == mat2(1 * 10 + 3 * 20, 2 * 10 + 4 * 20, 1 * 30 + 3 * 40, 2 * 30 + 4 * 40));
        std::cout << c << "\n";
    }
    std::cout << "All tests passed\n";
}

