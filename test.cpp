#include "calculator.h"

#include <iostream>
#include <iomanip>
#include <exception>
#include <vector>

int main() {
    std::vector<std::string> inputs;

    // -------------------- VALID INPUTS START HERE --------------------------
    inputs.emplace_back("1+2");                                  //  1
    inputs.emplace_back("4*21/(8+3) - (7)*450");                 //  2
    inputs.emplace_back("((15))");                               //  3
    inputs.emplace_back("3*-5");                                 //  4
    inputs.emplace_back("3---5");                                //  5
    inputs.emplace_back("10+2-3");                               //  6
    inputs.emplace_back("1000000*3000000 - 999999999");          //  7
    inputs.emplace_back("(1+(2*(3+(4*(5+6)))))");                //  8
    inputs.emplace_back("((3+5)*((7-2)))");                      //  9
    inputs.emplace_back("42");                                   // 10
    inputs.emplace_back("0+0-0*0");                              // 11
    inputs.emplace_back("5/--+-5");                              // 12
    inputs.emplace_back("8*(3-(2-(1-(0))))");                    // 13
    inputs.emplace_back("999999999999 + 888888888888");          // 14
    inputs.emplace_back("(1234567890/5) - (987654321/3)");       // 15
    inputs.emplace_back("7*(6/(5-(4-(3-(2-1)))))");              // 16
    inputs.emplace_back("56/(-7+9)");                            // 17
    inputs.emplace_back("(4+3)*((9/3)+2)");                      // 18
    inputs.emplace_back("-----5");                               // 19
    inputs.emplace_back("5--------3");                           // 20
    inputs.emplace_back("10/----2");                             // 21
    inputs.emplace_back("6*---+-2");                             // 22
    inputs.emplace_back("20/--+-5");                             // 23
    inputs.emplace_back("7+-+--3");                              // 24
    inputs.emplace_back("9/++++3");                              // 25
    inputs.emplace_back("8*-+-2");                               // 26
    inputs.emplace_back("((((((((((5))))))))))");                // 27
    inputs.emplace_back("((((3*2)+1)/(9-(3+(2-(1))))) - 4)");    // 28
    // ---------- VALID INPUTS END HERE / INVALID INPUTS START HERE ----------
    inputs.emplace_back("100/(25/-5)");                          // 29
    inputs.emplace_back("");                                     // 30
    inputs.emplace_back("()");                                   // 31
    inputs.emplace_back("( )");                                  // 32
    inputs.emplace_back("(");                                    // 33
    inputs.emplace_back(")");                                    // 34
    inputs.emplace_back("5++*3");                                // 35
    inputs.emplace_back("*3+5");                                 // 36
    inputs.emplace_back("((1+(2+(3+(4+(5+(6+(7))))))))))");      // 37
    inputs.emplace_back("3+5-");                                 // 38
    inputs.emplace_back("10/**2");                               // 39
    inputs.emplace_back("8+/-2");                                // 40
    inputs.emplace_back("7+()");                                 // 41
    inputs.emplace_back("4+5)");                                 // 42
    inputs.emplace_back("(4+5");                                 // 43
    inputs.emplace_back("5 / 0");                                // 44
    inputs.emplace_back("20/(10-10)");                           // 45
    inputs.emplace_back("3 + * 4");                              // 46
    inputs.emplace_back("5 + - * 7");                            // 47
    inputs.emplace_back("100000000000000000000000000000000000"); // 48

    Calculator calc;
    std::cout << std::setprecision(15);
    unsigned int testCaseNumber = 0;
    for (const std::string& input : inputs) {
        ++testCaseNumber;
        std::cout << testCaseNumber << ".)  ";
        try {
            std::cout << std::setprecision(15) << calc.calculate(input) << "\n";
        } catch (const std::exception& e) { std::cout << e.what() << "\n"; }
    }
}