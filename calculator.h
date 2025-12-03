#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <cmath>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

class Calculator {
public:

    double calculate(const std::string& expression) {
        auto ASTree = parse(expression);
        ScientificValue answer = ASTree->evaluate();
        answer = makeScientific(answer.rawValue(),MAX_DIGITS-1);
        // must round to MAX_DIGITS - 1 to precisely represent repeating doubles like 0.9999999... = 1
        lastAnswer = answer.rawValue();
        lastExpression = expression;
        return lastAnswer;
    }

    [[nodiscard]] auto getMaxDigits() const { return MAX_DIGITS; }
    [[nodiscard]] auto getLastExpression() const { return lastExpression; }
    [[nodiscard]] auto getLastAnswer() const { return lastAnswer; }

private:

    static inline constexpr std::uint8_t MAX_DIGITS = 13;
    // for precision in double, MAX_DIGITS needs to be <= 15 . It is currently set to 13 to create a large safety
    // net against floating-point errors of double.
    static inline constexpr std::uint16_t MAX_MAGNITUDE = 300; // double can store an exponent up to (plus-or-minus) 308


    static int getScientificMagnitude(const double& value) {
        if (value == 0.0) return 0;
        return std::floor(std::log10(std::abs(value)));
    }

    static std::string overflowErrorMessage() {
        return "Value limit exceeded (currently set to 10 ^ " + std::to_string(MAX_MAGNITUDE) + ")";
    }

    static bool isBounded(const double& value) {
        return std::abs( getScientificMagnitude(value) ) <= MAX_MAGNITUDE;
    }

    static bool isProductBounded(const int& magnitude1, const int& magnitude2) {
        return std::abs(magnitude1 + magnitude2) <= MAX_MAGNITUDE;
        // magnitude of product isn't always equal to sum of magnitudes, but this works fine when MAX_MAGNITUDE is so large
    }


    struct ScientificValue;
    struct ASTNode {
        virtual ScientificValue evaluate() = 0;
        virtual ~ASTNode() = default;
    };

    struct ScientificValue : ASTNode {
        double value; int magnitude;

        ScientificValue(const double& val, const int& m): value(val), magnitude(m) {}

        ScientificValue evaluate() override { return *this; }

        [[nodiscard]] double rawValue() const {
            return this->value * std::pow(10.0, this->magnitude);
        }
    };

    static ScientificValue makeScientific(double value, const std::uint8_t& lastDigit = MAX_DIGITS) {
        if (value == 0.0) return {0.0, 0};
        int magnitude = getScientificMagnitude(value);
        int rounded = lastDigit - 1 - magnitude;
        double exponent = std::pow(10, rounded);
        value = std::round(value * exponent) / exponent;
        value = value / std::pow(10, magnitude);
        return {value, magnitude};
    }

    struct Negation : public ASTNode {
        std::unique_ptr<ASTNode> operand;

        explicit Negation(std::unique_ptr<ASTNode>&& o): operand(std::move(o)) {}

        ScientificValue evaluate() override {
            ScientificValue val = operand->evaluate();
            val.value = -val.value;
            return val;
        }

        ~Negation() override = default;
    };

    struct AddOrSubtract : public ASTNode {
        std::unique_ptr<ASTNode> left;
        std::unique_ptr<ASTNode> right;
        bool isSub;

        explicit AddOrSubtract(std::unique_ptr<ASTNode>&& l, std::unique_ptr<ASTNode>&& r, const bool& b = false)
            : left(std::move(l)), right(std::move(r)), isSub(b) {}

        ScientificValue evaluate() override {
            const ScientificValue left_val = left->evaluate();
            const ScientificValue right_val = right->evaluate();

            const double raw_left = left_val.rawValue();
            const double raw_right = right_val.rawValue();
            const double raw_sum = isSub ? raw_left - raw_right : raw_left + raw_right ;

            if (!isBounded(raw_sum))
                throw std::overflow_error(overflowErrorMessage());

            return makeScientific(raw_sum);
        }

        ~AddOrSubtract() override = default;
    };

    struct MultiplyOrDivide : public ASTNode {
        std::unique_ptr<ASTNode> left;
        std::unique_ptr<ASTNode> right;
        bool isDiv;

        explicit MultiplyOrDivide(std::unique_ptr<ASTNode>&& l, std::unique_ptr<ASTNode>&& r, const bool& b = false)
            : left(std::move(l)), right(std::move(r)), isDiv(b) {}

        ScientificValue evaluate() override {
            const ScientificValue left_val = left->evaluate();
            const ScientificValue right_val = right->evaluate();


            if (isDiv) {
                if (right_val.value == 0.0)
                    throw std::domain_error("Division by zero detected");

                int right_mag_inverse = -right_val.magnitude;
                if (!isProductBounded(left_val.magnitude, right_mag_inverse))
                    throw std::overflow_error(overflowErrorMessage());

                const double quotient = left_val.rawValue() / right_val.rawValue();
                return makeScientific(quotient);
            }


            if (!isProductBounded(left_val.magnitude, right_val.magnitude))
                throw std::overflow_error(overflowErrorMessage());

            const double product = left_val.rawValue() * right_val.rawValue();
            return makeScientific(product);
        }

        ~MultiplyOrDivide() override = default;
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

        static inline constexpr char SENTINEL_CHAR = ' '; // unused sentinel value to silence warnings about uninitialized members
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

    [[nodiscard]] static std::unique_ptr<ASTNode> parse(const std::string& expression) {
        Lexer lex(expression);
        return parseExpression(lex);
    };

    static void operateOnLeft(std::unique_ptr<ASTNode>& left, Lexer& lex) {
        char op = *lex;
        ++lex;
        std::unique_ptr<ASTNode> right;
        switch (op) {
            case '+':
                right = parseTerm(lex);
                left = std::make_unique<AddOrSubtract>(std::move(left), std::move(right));
                break;

            case '-':
                right = parseTerm(lex);
                left = std::make_unique<AddOrSubtract>(std::move(left), std::move(right), true);
                break;

            case '*':
                right = parseOperand(lex);
                left = std::make_unique<MultiplyOrDivide>(std::move(left), std::move(right));
                break;

            case '/':
                right = parseOperand(lex);
                left = std::make_unique<MultiplyOrDivide>(std::move(left), std::move(right), true);
                break;

            default: throw std::logic_error("Unexpected operator in operateOnLeft method");
        }
    }

    [[nodiscard]] static std::unique_ptr<ASTNode> parseExpression(Lexer& lex) {
        auto left = parseTerm(lex);

        while (*lex == '+' || *lex == '-') // slight redundancy here, but this is otherwise the best way to do it
            operateOnLeft(left, lex);

        return left;
    }

    [[nodiscard]] static std::unique_ptr<ASTNode> parseTerm(Lexer& lex) {
        auto left = parseOperand(lex);

        while (*lex == '*' || *lex == '/')
            operateOnLeft(left, lex);

        return left;
    };

    [[nodiscard]] static std::unique_ptr<ASTNode> parseOperand(Lexer& lex) {
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
            if (isNegative) return std::make_unique<Negation>(std::move(node));
            return node;
        }

        double operand = 0;
        while ( isDigit(*lex) ) {
            operand *= 10;
            operand += static_cast<double>(*lex) - static_cast<double>('0');
            if (!isBounded(operand))
                throw std::overflow_error(overflowErrorMessage());
            ++lex;
        }

        if (isNegative) operand = -operand;
        return std::make_unique<ScientificValue>( makeScientific(operand) );
    };

};

#endif //CALCULATOR_H