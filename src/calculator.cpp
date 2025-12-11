#include "calculator.h"

#include <cmath>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>


// Utilities
int calc::utils::getScientificMagnitude(double value) {
	if (value == 0.0) return 0;
	return std::floor(std::log10(std::abs(value)));
}

std::string calc::utils::overflowErrorMessage() {
	return "Value limit exceeded (currently set to 10 ^ " + std::to_string(MAX_MAGNITUDE) + ")";
}

bool calc::utils::isBounded(double value) {
	return static_cast<unsigned int>( std::abs( getScientificMagnitude(value) ) ) <= MAX_MAGNITUDE;
}

bool calc::utils::isProductBounded(int magnitude1, int magnitude2) {
	return static_cast<unsigned int>( std::abs(magnitude1 + magnitude2) ) <= MAX_MAGNITUDE;
	// magnitude of product isn't always equal to sum of magnitudes, but this is fine when MAX_MAGNITUDE is so large
}

bool calc::utils::isDigit(char c) {
	constexpr std::string_view NUMS = "0123456789";
	return NUMS.find(c) != std::string::npos;
}

bool calc::utils::isOperator(char c) {
	constexpr std::string_view OPS = "+-*/";
	return OPS.find(c) != std::string::npos;
}

calc::classes::ScientificValue calc::utils::makeScientific(double value, unsigned int lastDigit) {
	// function rounds to MAX_DIGITS + 1 by default. After last evaluation, calculate() will call makeScientific() to
	// round to MAX_DIGITS. This will guarantee that repeating values are rounded properly, like 0.9999999... = 1
	if (value == 0.0) return {0.0, 0};
	int magnitude = getScientificMagnitude(value);
	int rounded = static_cast<int>(lastDigit) - 1 - magnitude;
	double exponent = std::pow(10, rounded);
	value = std::round(value * exponent) / exponent;
	value = value / std::pow(10, magnitude);
	return {value, magnitude};
}

// Calculator Inner Classes
// defined in a top-level namespace for easier implementation
// AST
calc::classes::ScientificValue::ScientificValue(double val, int m): value(val), magnitude(m) {}

calc::classes::ScientificValue calc::classes::ScientificValue::evaluate() { return *this; }

double calc::classes::ScientificValue::rawValue() const {
	return value * std::pow(10.0, magnitude);
}

calc::classes::Negation::Negation(std::unique_ptr<ASTNode>&& o): operand(std::move(o)) {}

calc::classes::AddOrSubtract::AddOrSubtract(
	std::unique_ptr<ASTNode>&& l, std::unique_ptr<ASTNode>&& r, bool b)
	: left(std::move(l)), right(std::move(r)), isSub(b) {}

calc::classes::ScientificValue calc::classes::Negation::evaluate() {
	ScientificValue val = operand->evaluate();
	val.value = -val.value;
	return val;
}

calc::classes::ScientificValue calc::classes::AddOrSubtract::evaluate() {
	using namespace utils;

	const ScientificValue left_val = left->evaluate();
	const ScientificValue right_val = right->evaluate();

	const double raw_left = left_val.rawValue();
	const double raw_right = right_val.rawValue();
	const double raw_sum = isSub ? raw_left - raw_right : raw_left + raw_right ;

	if (!isBounded(raw_sum))
		throw std::overflow_error(overflowErrorMessage());

	return makeScientific(raw_sum);
}

calc::classes::MultiplyOrDivide::MultiplyOrDivide(
	std::unique_ptr<ASTNode>&& l, std::unique_ptr<ASTNode>&& r, bool b)
	: left(std::move(l)), right(std::move(r)), isDiv(b) {}

calc::classes::ScientificValue calc::classes::MultiplyOrDivide::evaluate() {
	using namespace utils;

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


// Lexer
calc::classes::Lexer::Lexer(const std::string& e)
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

void calc::classes::Lexer::operator++() {
	using namespace utils;

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


// Calculator
double Calculator::calculate(const std::string& expression) {
	auto ASTree = parse(expression);
	ScientificValue answer = ASTree->evaluate();
	answer = calc::utils::makeScientific(answer.rawValue(),calc::MAX_DIGITS);
	lastAnswer = answer.rawValue();
	lastExpression = expression;
	return lastAnswer;
}


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

std::unique_ptr<calc::classes::ASTNode> Calculator::parse(const std::string& expression) {
	Lexer lex(expression);
	return parseExpression(lex);
};

void Calculator::operateOnLeft(std::unique_ptr<ASTNode>& left, Lexer& lex) {
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

std::unique_ptr<calc::classes::ASTNode> Calculator::parseExpression(Lexer& lex) {
	auto left = parseTerm(lex);

	while (*lex == '+' || *lex == '-') // slight redundancy here, but this is otherwise the best way to do it
		operateOnLeft(left, lex);

	return left;
}

std::unique_ptr<calc::classes::ASTNode> Calculator::parseTerm(Lexer& lex) {
	auto left = parseOperand(lex);

	while (*lex == '*' || *lex == '/')
		operateOnLeft(left, lex);

	return left;
};

std::unique_ptr<calc::classes::ASTNode> Calculator::parseOperand(Lexer& lex) {
	using namespace calc::utils;

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
		++lex;
	}

	if (operand == std::numeric_limits<double>::infinity() || !isBounded(operand))
		throw std::overflow_error(overflowErrorMessage());

	if (isNegative) operand = -operand;
	return std::make_unique<ScientificValue>( makeScientific(operand) );
};