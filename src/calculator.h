#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <memory>
#include <string>


/* Namespace Structure
 * namespace calc
 * ----constants
 * ----namespace utils
 * ----namespace classes
 */


// Namespace Constants
namespace calc {
    inline constexpr unsigned int MAX_DIGITS = 12;
    // for precision in double, MAX_DIGITS needs to be <= 14 . It is currently set to 12 to create a large safety
    // net against floating-point errors with double.
    inline constexpr unsigned int MAX_MAGNITUDE = 300; // double can store an exponent up to (plus-or-minus) 308
}


// Utilities
namespace calc::classes { struct ScientificValue; } // forward-declaration
namespace calc::utils {

    int getScientificMagnitude(double value);

    std::string overflowErrorMessage();

    bool isBounded(double value);

    bool isProductBounded(int magnitude1, int magnitude2);

    bool isDigit(char c);

    bool isOperator(char c);

    classes::ScientificValue makeScientific(double value, unsigned int lastDigit = MAX_DIGITS + 1);
    // lastDigit represents digit that's been rounded, set to MAX_DIGITS + 1 by default. Then Calculator will
    // call makeScientific in last evaluation with lastDigit = MAX_DIGITS . This ensures that repeating values
    // will be properly rounded like 0.99999999... = 1

}


// Calculator Inner Classes
// instead of making nested classes in Calculator, I defined them in a top-level namespace for easier implementation
namespace calc::classes {

    // AST
    struct ScientificValue;
    struct ASTNode {
        virtual ScientificValue evaluate() = 0;
        virtual ~ASTNode() = default;
    };

    struct ScientificValue : public ASTNode {
        double value;
        int magnitude;

        ScientificValue(double val, int m);

        ScientificValue evaluate() override;

        [[nodiscard]] double rawValue() const;
    };

    struct Negation : public ASTNode {
        std::unique_ptr<ASTNode> operand;

        explicit Negation(std::unique_ptr<ASTNode>&& o);

        ScientificValue evaluate() override;
    };

    struct AddOrSubtract : public ASTNode {
        std::unique_ptr<ASTNode> left;
        std::unique_ptr<ASTNode> right;
        bool isSub;

        AddOrSubtract(std::unique_ptr<ASTNode>&& l, std::unique_ptr<ASTNode>&& r, bool b = false);

        ScientificValue evaluate() override;
    };

    struct MultiplyOrDivide : public ASTNode {
        std::unique_ptr<ASTNode> left;
        std::unique_ptr<ASTNode> right;
        bool isDiv;

        MultiplyOrDivide(std::unique_ptr<ASTNode>&& l, std::unique_ptr<ASTNode>&& r, bool b = false);

        ScientificValue evaluate() override;
    };

    // Lexer
    class Lexer {
    private:

        static inline constexpr char SENTINEL_CHAR = ' '; // unused sentinel value to silence warnings about uninitialized members
        const std::string& expression;
        unsigned int idx;
        unsigned int numOpenPars;
        bool isDelayed; // purpose is to return '*' in expressions like "...+4)7-..." before returning '7' here for example
        char current;
        char last;
        char delayed;

    public:

        explicit Lexer(const std::string& e);

        char operator*() const { return current; }

        void operator++();
    };

}


// Calculator
class Calculator {
public:

    double calculate(const std::string& expression);

    std::string getLastExpression() const { return lastExpression; }
    double getLastAnswer() const { return lastAnswer; }

private:

    using ASTNode = calc::classes::ASTNode;
    using ScientificValue = calc::classes::ScientificValue;
    using Negation = calc::classes::Negation;
    using AddOrSubtract = calc::classes::AddOrSubtract;
    using MultiplyOrDivide = calc::classes::MultiplyOrDivide;
    using Lexer = calc::classes::Lexer;


    // Data Members
    std::string lastExpression = "0";
    double lastAnswer = 0;


    // Parser
    /* Parser Language:
     * An operand (O) is an integer (int) or expression (E), possibly negated with a unary minus operator:
     * O := {-}int || {-}E
     *
     * A term (T) is a product/quotient of operands, or a single operand:
     * T := O { (* || /) O }
     *
     * An expression (E) is the sum/difference of terms, or a single term:
     * E := T { (+ || -) T }
     */

    static std::unique_ptr<ASTNode> parse(const std::string& expression);

    static void operateOnLeft(std::unique_ptr<ASTNode>& left, Lexer& lex);

    static std::unique_ptr<ASTNode> parseExpression(Lexer& lex);

    static std::unique_ptr<ASTNode> parseTerm(Lexer& lex);

    static std::unique_ptr<ASTNode> parseOperand(Lexer& lex);

};

#endif //CALCULATOR_H