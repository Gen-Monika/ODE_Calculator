#include "ExpressionParser.h"

#include <QRegularExpression>
#include <QStringList>
#include <algorithm>

namespace {
QString joinSigned(const QStringList& parts)
{
    QString text = parts.join("+");
    text.replace("+-", "-");
    return text;
}

bool startsWithSign(const QString& text)
{
    return text.startsWith('+') || text.startsWith('-');
}

QString addLeadingPlus(QString text)
{
    if (!startsWithSign(text)) {
        text.prepend('+');
    }
    return text;
}

QString dropOptionalMul(QString text)
{
    text.replace("*", "");
    return text;
}

bool containsTopLevel(const QString& text, QChar ch)
{
    int depth = 0;
    for (QChar c : text) {
        if (c == '(') {
            ++depth;
        } else if (c == ')') {
            --depth;
        } else if (c == ch && depth == 0) {
            return true;
        }
    }
    return false;
}

QStringList splitTopLevelSigned(QString text)
{
    text = addLeadingPlus(text);
    QStringList parts;
    int depth = 0;
    int start = 0;
    for (int i = 1; i < text.size(); ++i) {
        const QChar c = text[i];
        if (c == '(') {
            ++depth;
        } else if (c == ')') {
            --depth;
        } else if ((c == '+' || c == '-') && depth == 0) {
            parts << text.mid(start, i - start);
            start = i;
        }
    }
    parts << text.mid(start);
    return parts;
}
}

EquationParseResult ExpressionParser::parseEquation(const QString& rawText) const
{
    EquationParseResult result;
    QString text = normalize(rawText);
    if (text.isEmpty()) {
        result.error = "请输入方程，例如 y''+y=x*cos(2x)。";
        return result;
    }

    const QStringList sides = text.split('=');
    if (sides.size() != 2) {
        result.error = "方程需要且只能包含一个等号。";
        return result;
    }

    SolverInput input;
    QString error;
    Rational leadingCoefficient(1);
    if (!parseLeftSide(sides[0], input, leadingCoefficient, error)) {
        result.error = error;
        return result;
    }
    if (!parseRightSide(sides[1], input, error)) {
        result.error = error;
        return result;
    }
    normalizeRightSide(input, leadingCoefficient);

    result.ok = true;
    result.input = input;
    result.normalizedText = text;
    result.latex = toLatex(input);
    return result;
}

bool ExpressionParser::parseLeftSide(
    const QString& text,
    SolverInput& input,
    Rational& leadingCoefficient,
    QString& error) const
{
    Rational y2(0);
    Rational y1(0);
    Rational y0(0);

    for (QString part : splitTopLevelSigned(text)) {
        if (part.isEmpty()) {
            continue;
        }

        Rational coeff;
        int derivativeOrder = 0;
        if (!parseLeftTerm(part, coeff, derivativeOrder, error)) {
            return false;
        }

        if (derivativeOrder == 2) {
            y2 = y2 + coeff;
        } else if (derivativeOrder == 1) {
            y1 = y1 + coeff;
        } else {
            y0 = y0 + coeff;
        }
    }

    if (!y2.isZero()) {
        input.order = 2;
        leadingCoefficient = y2;
        input.a1 = y1 / y2;
        input.a0 = y0 / y2;
        return true;
    }

    if (!y1.isZero()) {
        input.order = 1;
        leadingCoefficient = y1;
        input.a1 = Rational(0);
        input.a0 = y0 / y1;
        return true;
    }

    error = "左端至少需要包含 y' 或 y''。";
    return false;
}

bool ExpressionParser::parseLeftTerm(
    const QString& rawText,
    Rational& coefficient,
    int& derivativeOrder,
    QString& error) const
{
    QString text = rawText;
    int sign = 1;
    if (text.startsWith('+')) {
        text = text.mid(1);
    } else if (text.startsWith('-')) {
        text = text.mid(1);
        sign = -1;
    }

    text = stripOuterParens(text);
    if (text.isEmpty()) {
        error = "左端存在空项。";
        return false;
    }

    struct DerivativeToken {
        const char* token;
        int order;
    };
    const DerivativeToken tokens[] = {
        {"y''", 2},
        {"y2", 2},
        {"y'", 1},
        {"y1", 1},
        {"y0", 0},
        {"y", 0},
    };

    bool matched = false;
    QString coeffText;
    for (const DerivativeToken& candidate : tokens) {
        const QString token = QString::fromLatin1(candidate.token);
        if (text.endsWith(token)) {
            derivativeOrder = candidate.order;
            coeffText = text.left(text.size() - token.size());
            matched = true;
            break;
        }
    }

    if (!matched) {
        if (text.contains('y')) {
            error = "左端项 \"" + rawText + "\" 无效：只支持 y、y'、y'' 及其常数倍。";
        } else {
            error = "左端只支持 y、y'、y'' 及其常数倍。";
        }
        return false;
    }

    const bool hasSeparator = coeffText.endsWith('*');
    if (hasSeparator) {
        coeffText.chop(1);
    }
    if (coeffText.contains('*')) {
        error = "左端项 \"" + rawText + "\" 的系数只能写在 y、y' 或 y'' 前面。";
        return false;
    }
    if (coeffText.isEmpty()) {
        if (hasSeparator) {
            error = "左端项 \"" + rawText + "\" 的乘号前缺少系数。";
            return false;
        }
        coeffText = "1";
    }

    Rational parsedCoefficient;
    if (!parseRationalToken(coeffText, parsedCoefficient, error)) {
        error = "无法解析左端系数 \"" + coeffText + "\"：" + error;
        return false;
    }
    coefficient = sign == 1 ? parsedCoefficient : -parsedCoefficient;
    return true;
}

void ExpressionParser::normalizeRightSide(SolverInput& input, const Rational& leadingCoefficient) const
{
    if (leadingCoefficient == Rational(1)) {
        return;
    }

    const auto scalePolynomial = [leadingCoefficient](std::vector<Rational>& polynomial) {
        for (Rational& coefficient : polynomial) {
            coefficient = coefficient / leadingCoefficient;
        }
    };

    if (input.kind == NonHomogeneousKind::PolynomialExp) {
        scalePolynomial(input.polynomialAscending);
    } else {
        scalePolynomial(input.cosinePolynomialAscending);
        scalePolynomial(input.sinePolynomialAscending);
    }
}

bool ExpressionParser::parseRightSide(const QString& text, SolverInput& input, QString& error) const
{
    if (text.contains("sin") || text.contains("cos")) {
        return parseExpTrig(text, input, error);
    }
    return parsePolynomialExp(text, input, error);
}

bool ExpressionParser::parsePolynomialExp(const QString& text, SolverInput& input, QString& error) const
{
    QString expr = stripOuterParens(text);
    input.kind = NonHomogeneousKind::PolynomialExp;
    input.expR = Rational(0);

    QString polynomialText = expr;
    if (!extractExponentialFactor(polynomialText, input.expR, error)) {
        return false;
    }

    polynomialText = stripOuterParens(polynomialText);
    if (!parsePolynomial(polynomialText, input.polynomialAscending, error)) {
        error = "无法解析 P(x)：" + error;
        return false;
    }
    return true;
}

bool ExpressionParser::parseExpTrig(const QString& text, SolverInput& input, QString& error) const
{
    QString expr = stripOuterParens(text);
    input.kind = NonHomogeneousKind::ExpTrig;
    input.trigA = Rational(0);

    if (!extractExponentialFactor(expr, input.trigA, error)) {
        return false;
    }

    expr = stripOuterParens(expr);
    std::vector<Rational> cosine;
    std::vector<Rational> sine;
    bool foundTrig = false;
    Rational frequency;
    bool hasFrequency = false;

    for (QString part : splitTopLevelSigned(expr)) {
        if (part.isEmpty()) {
            continue;
        }
        const QString sign = part.left(1);
        part = part.mid(1);
        const bool negative = sign == "-";

        const bool isCos = part.contains("cos(");
        const bool isSin = part.contains("sin(");
        if (isCos == isSin) {
            error = "三角型右端每一项需要恰好包含 sin(...) 或 cos(...)。";
            return false;
        }

        const QString functionName = isCos ? "cos(" : "sin(";
        const int fnIndex = part.indexOf(functionName);
        const int argStart = fnIndex + functionName.size();
        int depth = 1;
        int pos = argStart;
        for (; pos < part.size(); ++pos) {
            if (part[pos] == '(') {
                ++depth;
            } else if (part[pos] == ')') {
                --depth;
                if (depth == 0) {
                    break;
                }
            }
        }
        if (pos >= part.size()) {
            error = "三角函数缺少右括号。";
            return false;
        }

        Rational currentFrequency;
        if (!parseLinearArgument(part.mid(argStart, pos - argStart), currentFrequency, error)) {
            error = "无法解析三角函数频率：" + error;
            return false;
        }
        if (!hasFrequency) {
            frequency = currentFrequency;
            hasFrequency = true;
        } else if (frequency != currentFrequency) {
            error = "当前原型要求同一右端中的 sin/cos 使用相同频率。";
            return false;
        }

        QString polyText = part.left(fnIndex) + part.mid(pos + 1);
        if (polyText.endsWith('*')) {
            polyText.chop(1);
        }
        if (polyText.startsWith('*')) {
            polyText = polyText.mid(1);
        }
        if (polyText.isEmpty()) {
            polyText = "1";
        }
        polyText = stripOuterParens(polyText);

        Polynomial poly;
        if (!parsePolynomial(polyText, poly, error)) {
            error = "无法解析三角项前的多项式：" + error;
            return false;
        }
        if (negative) {
            for (Rational& coeff : poly) {
                coeff = -coeff;
            }
        }

        Polynomial& target = isCos ? cosine : sine;
        if (target.size() < poly.size()) {
            target.resize(poly.size(), Rational(0));
        }
        for (int i = 0; i < static_cast<int>(poly.size()); ++i) {
            target[i] = target[i] + poly[i];
        }
        foundTrig = true;
    }

    if (!foundTrig) {
        error = "未识别到 sin 或 cos 项。";
        return false;
    }
    if (cosine.empty()) {
        cosine = {Rational(0)};
    }
    if (sine.empty()) {
        sine = {Rational(0)};
    }
    input.trigB = frequency;
    input.cosinePolynomialAscending = cosine;
    input.sinePolynomialAscending = sine;
    return true;
}

bool ExpressionParser::parsePolynomial(const QString& rawText, Polynomial& polynomial, QString& error) const
{
    QString text = stripOuterParens(rawText);
    text = text.replace("**", "^");
    text = addLeadingPlus(text);

    polynomial.clear();
    for (QString part : splitTopLevelSigned(text)) {
        if (part.isEmpty()) {
            continue;
        }
        Term term;
        if (!parseTerm(part, term, error)) {
            return false;
        }
        if (polynomial.size() <= static_cast<size_t>(term.xPower)) {
            polynomial.resize(term.xPower + 1, Rational(0));
        }
        polynomial[term.xPower] = polynomial[term.xPower] + term.coefficient;
    }
    if (polynomial.empty()) {
        polynomial = {Rational(0)};
    }
    while (polynomial.size() > 1 && polynomial.back().isZero()) {
        polynomial.pop_back();
    }
    return true;
}

bool ExpressionParser::parseTerm(const QString& rawText, Term& term, QString& error) const
{
    QString text = rawText;
    int sign = 1;
    if (text.startsWith('+')) {
        text = text.mid(1);
    } else if (text.startsWith('-')) {
        text = text.mid(1);
        sign = -1;
    }

    text = stripOuterParens(text);
    text = dropOptionalMul(text);
    term.coefficient = Rational(sign);
    term.xPower = 0;

    const int xIndex = text.indexOf('x');
    if (xIndex < 0) {
        Rational value;
        if (!parseRationalToken(text, value, error)) {
            return false;
        }
        term.coefficient = sign == 1 ? value : -value;
        return true;
    }

    QString coeffText = text.left(xIndex);
    if (coeffText.isEmpty()) {
        coeffText = "1";
    }
    Rational coeff;
    if (!parseRationalToken(coeffText, coeff, error)) {
        return false;
    }
    term.coefficient = sign == 1 ? coeff : -coeff;
    term.xPower = 1;

    if (xIndex + 1 < text.size()) {
        QString rest = text.mid(xIndex + 1);
        if (!rest.startsWith('^')) {
            error = "x 后只支持 ^n 次幂。";
            return false;
        }
        bool ok = false;
        const int power = rest.mid(1).toInt(&ok);
        if (!ok || power < 0) {
            error = "x 的次数需要是非负整数。";
            return false;
        }
        term.xPower = power;
    }
    return true;
}

bool ExpressionParser::parseRationalToken(const QString& rawText, Rational& value, QString& error) const
{
    QString text = stripOuterParens(rawText);
    if (text.isEmpty() || text == "+") {
        text = "1";
    } else if (text == "-") {
        text = "-1";
    }
    return Rational::parse(text, value, &error);
}

bool ExpressionParser::extractExponentialFactor(QString& expr, Rational& lambda, QString& error) const
{
    int expIndex = expr.indexOf("e^(");
    QString argument;
    int removeStart = -1;
    int removeEnd = -1;

    if (expIndex >= 0) {
        const int argStart = expIndex + 3;
        int depth = 1;
        int pos = argStart;
        for (; pos < expr.size(); ++pos) {
            if (expr[pos] == '(') {
                ++depth;
            } else if (expr[pos] == ')') {
                --depth;
                if (depth == 0) {
                    break;
                }
            }
        }
        if (pos >= expr.size()) {
            error = "指数函数缺少右括号。";
            return false;
        }
        argument = expr.mid(argStart, pos - argStart);
        removeStart = expIndex;
        removeEnd = pos + 1;
    } else {
        expIndex = expr.indexOf("exp(");
        if (expIndex >= 0) {
            const int argStart = expIndex + 4;
            int depth = 1;
            int pos = argStart;
            for (; pos < expr.size(); ++pos) {
                if (expr[pos] == '(') {
                    ++depth;
                } else if (expr[pos] == ')') {
                    --depth;
                    if (depth == 0) {
                        break;
                    }
                }
            }
            if (pos >= expr.size()) {
                error = "exp 函数缺少右括号。";
                return false;
            }
            argument = expr.mid(argStart, pos - argStart);
            removeStart = expIndex;
            removeEnd = pos + 1;
        } else {
            expIndex = expr.indexOf("e^");
            if (expIndex < 0) {
                return true;
            }
            const int argStart = expIndex + 2;
            int pos = argStart;
            while (pos < expr.size()) {
                const QChar c = expr[pos];
                if (c == '*' || c == '+' || c == '-' || c == ')' || c == '(') {
                    if (pos == argStart && (c == '+' || c == '-')) {
                        ++pos;
                        continue;
                    }
                    break;
                }
                ++pos;
            }
            argument = expr.mid(argStart, pos - argStart);
            removeStart = expIndex;
            removeEnd = pos;
        }
    }

    if (!parseLinearArgument(argument, lambda, error)) {
        error = "无法解析指数参数：" + error;
        return false;
    }

    expr = expr.left(removeStart) + expr.mid(removeEnd);
    if (expr.endsWith('*')) {
        expr.chop(1);
    }
    if (expr.startsWith('*')) {
        expr = expr.mid(1);
    }
    if (expr.isEmpty()) {
        expr = "1";
    }
    return true;
}

bool ExpressionParser::parseLinearArgument(const QString& rawText, Rational& coefficient, QString& error) const
{
    QString text = stripOuterParens(rawText);
    text = dropOptionalMul(text);
    const int xIndex = text.indexOf('x');
    if (xIndex < 0) {
        error = "参数需要形如 kx。";
        return false;
    }
    if (text.indexOf('x', xIndex + 1) >= 0) {
        error = "参数中只能出现一个 x。";
        return false;
    }
    QString coeffText = text.left(xIndex);
    if (coeffText.isEmpty() || coeffText == "+") {
        coeffText = "1";
    } else if (coeffText == "-") {
        coeffText = "-1";
    }
    if (xIndex + 1 != text.size()) {
        error = "当前只支持 kx 形式，不支持平移相位。";
        return false;
    }
    return parseRationalToken(coeffText, coefficient, error);
}

QString ExpressionParser::normalize(QString text) const
{
    text = text.trimmed();
    text.replace(QChar(0x2212), "-");
    text.replace("（", "(");
    text.replace("）", ")");
    text.replace("×", "*");
    text.replace("·", "*");
    text.replace(" ", "");
    text.replace("exp", "exp");
    return text;
}

QString ExpressionParser::stripOuterParens(QString text) const
{
    text = removeSpaces(text);
    bool changed = true;
    while (changed && text.startsWith('(') && text.endsWith(')')) {
        changed = false;
        int depth = 0;
        bool enclosesAll = true;
        for (int i = 0; i < text.size(); ++i) {
            if (text[i] == '(') {
                ++depth;
            } else if (text[i] == ')') {
                --depth;
                if (depth == 0 && i != text.size() - 1) {
                    enclosesAll = false;
                    break;
                }
            }
        }
        if (enclosesAll) {
            text = text.mid(1, text.size() - 2);
            changed = true;
        }
    }
    return text;
}

QString ExpressionParser::removeSpaces(QString text) const
{
    text.remove(QRegularExpression("\\s+"));
    return text;
}

QString ExpressionParser::toLatex(const SolverInput& input) const
{
    const QString lhs = leftSideLatex(input);

    QString rhs;
    if (input.kind == NonHomogeneousKind::PolynomialExp) {
        rhs = polynomialLatex(input.polynomialAscending);
        if (!input.expR.isZero()) {
            rhs += "e^{" + linearArgumentLatex(input.expR) + "}";
        }
    } else {
        if (!input.trigA.isZero()) {
            rhs += "e^{" + linearArgumentLatex(input.trigA) + "}";
        }
        QStringList trigParts;
        if (!(input.cosinePolynomialAscending.size() == 1 && input.cosinePolynomialAscending[0].isZero())) {
            const QString coeff = polynomialLatex(input.cosinePolynomialAscending);
            trigParts << (coeff == "1" ? "" : coeff) + "\\cos(" + linearArgumentLatex(input.trigB) + ")";
        }
        if (!(input.sinePolynomialAscending.size() == 1 && input.sinePolynomialAscending[0].isZero())) {
            const QString coeff = polynomialLatex(input.sinePolynomialAscending);
            trigParts << (coeff == "1" ? "" : coeff) + "\\sin(" + linearArgumentLatex(input.trigB) + ")";
        }
        rhs += "\\left(" + joinSigned(trigParts) + "\\right)";
    }
    return lhs + "=" + rhs;
}

QString ExpressionParser::leftSideLatex(const SolverInput& input) const
{
    QStringList parts;
    if (input.order == 2) {
        parts << "y''";
        if (!input.a1.isZero()) {
            parts << coefficientVariableLatex(input.a1, "y'", false);
        }
        if (!input.a0.isZero()) {
            parts << coefficientVariableLatex(input.a0, "y", false);
        }
    } else {
        parts << "y'";
        if (!input.a0.isZero()) {
            parts << coefficientVariableLatex(input.a0, "y", false);
        }
    }
    return parts.join("");
}

QString ExpressionParser::coefficientVariableLatex(
    const Rational& coefficient,
    const QString& variable,
    bool first) const
{
    const Rational absCoeff = absValue(coefficient);
    QString term;
    if (absCoeff == Rational(1)) {
        term = variable;
    } else {
        term = rationalLatex(absCoeff) + variable;
    }
    if (coefficient < Rational(0)) {
        return "-" + term;
    }
    return first ? term : "+" + term;
}

QString ExpressionParser::linearArgumentLatex(const Rational& coefficient) const
{
    if (coefficient == Rational(1)) {
        return "x";
    }
    if (coefficient == Rational(-1)) {
        return "-x";
    }
    return rationalLatex(coefficient) + "x";
}

QString ExpressionParser::polynomialLatex(const Polynomial& polynomial) const
{
    QStringList parts;
    for (int power = static_cast<int>(polynomial.size()) - 1; power >= 0; --power) {
        const Rational coeff = polynomial[power];
        if (coeff.isZero()) {
            continue;
        }
        QString term;
        const Rational absCoeff = absValue(coeff);
        if (power == 0) {
            term = rationalLatex(absCoeff);
        } else {
            if (absCoeff != Rational(1)) {
                term = rationalLatex(absCoeff);
            }
            term += "x";
            if (power > 1) {
                term += "^{" + QString::number(power) + "}";
            }
        }
        if (parts.isEmpty()) {
            parts << (coeff < Rational(0) ? "-" + term : term);
        } else {
            parts << (coeff < Rational(0) ? "-" + term : "+" + term);
        }
    }
    return parts.isEmpty() ? "0" : parts.join("");
}

QString ExpressionParser::rationalLatex(const Rational& value) const
{
    if (value.denominator() == 1) {
        return QString::number(value.numerator());
    }
    return "\\frac{" + QString::number(value.numerator()) + "}{"
        + QString::number(value.denominator()) + "}";
}
