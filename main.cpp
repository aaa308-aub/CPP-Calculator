#include "calculator.h"

#include <iostream>
#include <iomanip>
#include <exception>

void displayFeatures() {
    std::cout << "------------------------------------- Calculator rules and features -------------------------------------" << "\n";
    std::cout << "* Supports basics operations: addition (+), subtraction (-), multiplication (*), and division (/), while" << "\n";
    std::cout << "   following their rules of associativity and precedence. Example: \"4-3*5\" is evaluated as 4-(3*5)" << "\n";
    std::cout << "* Supports nesting with parentheses, and negation with unary minus, such as in \"-5\"" << "\n";
    std::cout << "* Supports adjacent operators as long as the leading operators are not * or / . The left-most" << "\n";
    std::cout << "    operator can be anything. Examples:" << "\n";
    std::cout << "      \"+--+-5\" : evaluates to -5" << "\n";
    std::cout << "      \"-8*+--4\" : evaluates to -8*4" << "\n";
    std::cout << "      \"+++9/+(--7)\" : evaluates to 9/7" << "\n";
    std::cout << "      \"4*/6\" : invalid because / as a leading operator is not allowed" << "\n";
    std::cout << "* No trailing operators without right operands allowed, such as in \"7+\" or \"(7+)6\" or \"4/4*4*\"" << "\n";
    std::cout << "* Parentheses adjacent to numbers or other parentheses are padded by * . Example:" << "\n";
    std::cout << "      \"2(1/2)4(5-7)(0+1)\" is evaluated as \"2*(1/2)*4*(5-7)*(0+1)\"" << "\n";
    std::cout << "* Supports numbers with great orders of magnitude (up to around 10 ^ 300)" << "\n";
    std::cout << "---------------------------------------------------------------------------------------------------------" << "\n";
    std::cout << "\n";
}

int main() {
    Calculator calc;
    std::cout << std::setprecision(calc.getMaxDigits());

    displayFeatures();

    std::string input;
    while (true) {
        std::cout << "Please enter your expression, or \"d\" to display features again, or \"e\" to exit: " << "\n";
        std::getline(std::cin, input);
        std::cout << "\n";

        if (input == "d") { displayFeatures(); continue; }
        else if (input == "e") break;

        try {
            calc.calculate(input);
            std::cout << "Answer: " << calc.getLastAnswer() << "\n\n";
        } catch (const std::exception& e) {
            std::cout << "ERROR / INVALID INPUT : " << e.what() << "\n\n";
        }
    }
}