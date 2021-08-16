#include <iostream>
#include "parsec.hpp"

int main() {
    using namespace Parsec;
    
    auto parser = make_parser("([" << repeat<3>(value<int>() << ",")
                              << "] " << value<std::string>().then(")") );

    auto [x,y] = parser.parse("([11,22,33,] a string)");
    std::cout << x[2] << ", "<< y << "\n";
}