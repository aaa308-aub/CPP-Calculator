#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <string>
#include <string_view>

class Calculator {
public:

    double calculate(const std::string& exp) {
        ASTNode* ASTree = parse(exp);
        auto totalFraction = ASTree->evaluate();
        auto tryAnswer = totalFraction->calculate();
        lastExpression = exp;
        lastAnswer = tryAnswer;
        delete ASTree;
        delete totalFraction;
        return tryAnswer;
    }

    [[nodiscard]] auto getMaxDigits() const { return MAX_DIGITS; }
    [[nodiscard]] auto getLastExpression() const { return lastExpression; }
    [[nodiscard]] auto getLastAnswer() const { return lastAnswer; }

private:

    using ull_t = unsigned long long int;
    static inline constexpr unsigned short int MAX_DIGITS = 15; // for computing double, needs to be <= 15

    static std::string overflowErrorMessage() {
        return "Max number of digits (currently set to " + std::to_string(MAX_DIGITS) +") exceeded";
    }

    static bool isBounded(const ull_t& a) {
        return std::floor( std::log10(a) ) + 1 <= MAX_DIGITS;
    }

    static bool isProductBounded(const ull_t& a, const ull_t& b) {
        return std::floor( std::log10(a) + std::log10(b) ) + 1 <= MAX_DIGITS;
    }


    struct Fraction;
    struct ASTNode {
        virtual Fraction* evaluate() = 0;
        virtual ~ASTNode() = default;
    };

    struct Fraction : public ASTNode {
        ull_t numerator;
        ull_t denominator;
        bool isNegative;

        explicit Fraction(const ull_t& n, const ull_t& d, const bool& neg = false):
            numerator(n), denominator(d), isNegative(neg) {}

        Fraction(const Fraction& other) {
            this->numerator = other.numerator;
            this->denominator = other.denominator;
            this->isNegative = other.isNegative;
        }

        Fraction* evaluate() override { return new Fraction(*this); }
        // must copy in order to mimic logic of other operators' .evaluate() method

        [[nodiscard]] double calculate() const {
            double result = static_cast<double>(numerator) / static_cast<double>(denominator);
            if (isNegative) result = -result;
            return result;
        }

        ~Fraction() override = default;
    };

    struct Negation : public ASTNode {
        ASTNode* operand;

        explicit Negation(ASTNode* o): operand(o) {}

        Fraction* evaluate() override {
            Fraction* node = operand->evaluate();
            delete operand; operand = nullptr;
            node->isNegative = !node->isNegative;
            return node;
        }

        ~Negation() override { delete operand; };
    };

    struct Addition : public ASTNode {
        ASTNode* left;
        ASTNode* right;

        explicit Addition(ASTNode* l, ASTNode* r): left(l), right(r) {}

        Fraction* evaluate() override {
            Fraction* node_left = left->evaluate();
            Fraction* node_right = right->evaluate();
            delete left; left = nullptr;
            delete right; right = nullptr;
            bool isNegative = false;


            if (!node_left->isNegative && node_right->isNegative) {
                node_right->isNegative = false;
                return Subtraction(node_left, node_right).evaluate();
            }

            if (node_left->isNegative && !node_right->isNegative) {
                node_left->isNegative = false;
                return Subtraction(node_right, node_left).evaluate();
            }

            if (node_left->isNegative && node_right->isNegative)
                isNegative = true;


            if (!isProductBounded(node_left->denominator, node_right->denominator)
                || !isProductBounded(node_left->numerator, node_right->denominator)
                || !isProductBounded(node_left->denominator, node_right->numerator)
                ) {
                delete node_left; node_left = nullptr;
                delete node_right; node_right = nullptr;
                throw std::overflow_error(overflowErrorMessage());
            }

            auto numerator = node_left->numerator * node_right->denominator;
            numerator += node_left->denominator * node_right->numerator;
            // Sum is at most MAX_DIGITS + 1 and MAX_DIGITS is far away from ull_t's max, so it is safe to compute and compare
            auto denominator = node_left->denominator * node_right->denominator;

            delete node_left; node_left = nullptr;
            delete node_right; node_right = nullptr;

            if (!isBounded(numerator))
                throw std::overflow_error(overflowErrorMessage());


            return new Fraction(numerator, denominator, isNegative);
        }

        ~Addition() override { delete left; delete right; }
    };

    struct Subtraction : public ASTNode {
        ASTNode* left;
        ASTNode* right;

        explicit Subtraction(ASTNode* l, ASTNode* r): left(l), right(r) {}

        Fraction* evaluate() override {
            Fraction* node_left = left->evaluate();
            Fraction* node_right = right->evaluate();
            delete left; left = nullptr;
            delete right; right = nullptr;
            bool isNegative = false;


            if (node_left->isNegative && !node_right->isNegative) {
                node_right->isNegative = true;
                return Addition(node_left, node_right).evaluate();
            }

            if (!node_left->isNegative && node_right->isNegative) {
                node_right->isNegative = false;
                return Addition(node_left, node_right).evaluate();
            }

            if (node_left->isNegative && node_right->isNegative)
                isNegative = true;


            if (!isProductBounded(node_left->denominator, node_right->denominator)
                || !isProductBounded(node_left->numerator, node_right->denominator)
                || !isProductBounded(node_left->denominator, node_right->numerator)
                ) {
                delete node_left; node_left = nullptr;
                delete node_right; node_right = nullptr;
                throw std::overflow_error(overflowErrorMessage());
                }

            auto new_num_left = node_left->numerator * node_right->denominator;
            auto new_num_right = node_left->denominator * node_right->numerator;
            auto denominator = node_left->denominator * node_right->denominator;

            delete node_left; node_left = nullptr;
            delete node_right; node_right = nullptr;

            ull_t numerator;
            if (new_num_right <= new_num_left) {
                numerator = new_num_left - new_num_right;
            }
            else {
                numerator = new_num_right - new_num_left;
                isNegative = !isNegative;
            }
            // Subtraction here will always be bounded by MAX_DIGITS

            return new Fraction(numerator, denominator, isNegative);
        }

        ~Subtraction() override { delete left; delete right; }
    };

    struct Product : public ASTNode {
        ASTNode* left;
        ASTNode* right;

        explicit Product(ASTNode* l, ASTNode* r): left(l), right(r) {}

        Fraction* evaluate() override {
            Fraction* node_left = left->evaluate();
            Fraction* node_right = right->evaluate();
            delete left; left = nullptr;
            delete right; right = nullptr;
            bool isNegative = false;


            if (!isProductBounded(node_left->numerator, node_right->numerator)
                || !isProductBounded(node_left->denominator, node_right->denominator)) {
                delete node_left; node_left = nullptr;
                delete node_right; node_right = nullptr;
                throw std::overflow_error(overflowErrorMessage());
            }

            if (node_left->isNegative ^ /*XOR*/ node_right->isNegative)
                isNegative = true;


            ull_t numerator = node_left->numerator * node_right->numerator;
            ull_t denominator = node_right->denominator * node_left->denominator;

            delete node_left; node_left = nullptr;
            delete node_right; node_right = nullptr;

            return new Fraction(numerator, denominator, isNegative);
        }

        ~Product() override { delete left; delete right; }
    };

    struct Quotient : public ASTNode {
        ASTNode* left;
        ASTNode* right;

        explicit Quotient(ASTNode* l, ASTNode* r): left(l), right(r) {
            if (dynamic_cast<Fraction*>(right)) {
                auto den = dynamic_cast<Fraction*>(right);
                if (den->numerator == 0) {
                    throw std::domain_error("Division by zero detected during parsing");
                }
            }
        }

        Fraction* evaluate() override {
            Fraction* node_left = left->evaluate();
            Fraction* node_right = right->evaluate();
            delete left; left = nullptr;
            delete right; right = nullptr;

            if (node_right->numerator == 0) {
                delete node_left; node_left = nullptr;
                delete node_right; node_right = nullptr;
                throw std::domain_error("Division by zero detected during evaluation");
            }

            std::swap(node_right->numerator, node_right->denominator);

            return Product(node_left, node_right).evaluate();
        }

        ~Quotient() override { delete left; delete right; }
    };


    std::string lastExpression = "0";
    double lastAnswer = 0;


    static bool isDigit(const char& c) {
        constexpr std::string_view NUMS = "0123456789";
        return NUMS.find(c) != std::string::npos;
    }

    static bool isOperator(const char& c) {
        constexpr std::string_view OPS = "+-*/";
        return OPS.find(c) != std::string::npos;
    }

    class Lexer {
    private:

        char SENTINEL_CHAR = ' '; // unused sentinel value to silence warnings about uninitialized members
        const std::string& expression;
        unsigned int idx;
        unsigned int numOpenPars;
        bool isDelayed; // purpose is to return '*' in expressions like "...+4)7-..." before returning '7' here for example
        char current;
        char last;
        char delayed;

    public:

        explicit Lexer(const std::string& e)
            : expression(e)
            , numOpenPars(0)
            , isDelayed(false)
            , current(SENTINEL_CHAR)
            , last(SENTINEL_CHAR)
            , delayed(SENTINEL_CHAR)
            {
            for (idx = 0; idx < expression.length(); ++idx) { // point Lexer to first valid token
                switch (expression[idx]) {
                    case ' ': continue;

                    case '*':
                    case '/':
                        throw std::invalid_argument("Invalid unary * or / found");

                    case ')':
                        throw std::invalid_argument("Closed parenthesis with no open match found");

                    case '(':
                        ++numOpenPars;
                        [[fallthrough]];
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                    case '+':
                    case '-':
                        current = expression[idx];
                        return;

                    default: throw std::invalid_argument("Invalid character found");
                }
            }

            throw std::invalid_argument("Expression is empty");
        }

        [[nodiscard]] char operator*() const { return current; }

        void operator++() {
            if (isDelayed) {
                isDelayed = false;
                current = delayed;
            }

            do ++idx; while (idx < expression.length() && expression[idx] == ' ');

            last = current;
            if (idx >= expression.length()) {
                if (numOpenPars != 0) throw std::invalid_argument("Unmatched open parenthesis found");
                if (isOperator(last)) throw std::invalid_argument("Leading operator found");

                current = ')'; // for first iteration of parseExpression() to stop at ')'
                return;
            }

            current = expression[idx];
            switch (current) {
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    if (last == ')') {
                        isDelayed = true;
                        delayed = current;
                        current = '*';
                    }
                    break;

                case '+':
                case '-':
                    break;

                case '*':
                case '/':
                    if (last == '(')
                        throw std::invalid_argument("Invalid unary * or / found");
                    if (isOperator(last))
                        throw std::invalid_argument("Invalid adjacent operators found");
                    break;

                case '(':
                    if (last == ')' || isDigit(last)) {
                        isDelayed = true;
                        delayed = current;
                        current = '*';
                    }
                    ++numOpenPars;
                    break;

                case ')':
                    if (numOpenPars == 0)
                        throw std::invalid_argument("Closed parenthesis with no open match found");
                    if (isOperator(last)) throw std::invalid_argument("Leading operator found");
                    if (last == '(') throw std::invalid_argument("Empty parentheses found");
                    --numOpenPars;
                    break;

                default: throw std::invalid_argument("Invalid character found");
            }
        }
    };


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

    [[nodiscard]] static ASTNode* parse(const std::string& expression) {
        Lexer lex(expression);
        return parseExpression(lex);
    };

    static void operateOnLeft(ASTNode*& left, Lexer& lex) {
        switch (*lex) {
            case '+':
                ++lex;
                left = new Addition(left, parseTerm(lex));
                break;

            case '-':
                ++lex;
                left = new Subtraction(left, parseTerm(lex));
                break;

            case '*':
                ++lex;
                left = new Product(left, parseTerm(lex));
                break;

            case '/':
                ++lex;
                left = new Quotient(left, parseTerm(lex));
                break;

            default: break;
        }
    }

    [[nodiscard]] static ASTNode* parseExpression(Lexer& lex) {
        auto left = parseTerm(lex);

        while (*lex == '+' || *lex == '-') // slight redundancy here, but this is otherwise the best way to do it
                operateOnLeft(left, lex);

        return left;
    }

    [[nodiscard]] static ASTNode* parseTerm(Lexer& lex) {
        auto left = parseOperand(lex);

        while (*lex == '*' || *lex == '/')
            operateOnLeft(left, lex);

        return left;
    };

    [[nodiscard]] static ASTNode* parseOperand(Lexer& lex) {
        bool isNegative = false;

        while (true) {
            if (*lex == '-') isNegative = !isNegative;
            else if (*lex != '+') break;
            ++lex;
        }

        if (*lex == '(') {
            ++lex;
            auto node = parseExpression(lex);
            ++lex;
            if (isNegative) return new Negation(node);
            return node;
        }

        ull_t operand = 0;
        while ( isDigit(*lex) ) {
            operand *= 10;
            operand += static_cast<ull_t>(*lex) - static_cast<ull_t>('0');
            if (!isBounded(operand))
                throw std::overflow_error(overflowErrorMessage());
            ++lex;
        }


        return new Fraction(operand, 1, isNegative);
    };

};

#endif //CALCULATOR_H
