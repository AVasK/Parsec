#include <iostream>
#include "parsec.hpp"

int main() {
    using namespace Parsec;
    
    // auto parser = make_parser("([" << repeat<3>(value<int>() << ",")
    //                           << "] " << value<std::string>().then(")") );

    auto parser = make_parser(value<float>() << ", " << value<std::string>());

    //auto [x,y] = parser.parse("([11,22,33,] a string)");
    auto [x,y] = parser.parse("0.1415, some string");
    std::cout << x << ", "<< y << "\n";
}