#include "OdeSolver.h"

#include <QStringList>
#include <algorithm>
#include <stdexcept>

namespace {
QString joinLines(const QStringList& lines)
{
    return lines.join("");
}

QString inlineFormula(const QString& body)
{
    return "<span class='inline-math'>" + body + "</span>";
}

QString displayFormula(const QString& body)
{
    return "<div class='equation'><span class='math'>" + body + "</span></div>";
}

QString summaryRow(const QString& label, const QString& formula)
{
    return "<div class='summary-row'><div class='summary-label'>" + label
        + "</div><div><span class='math'>" + formula + "</span></div></div>";
}

QString rootExpression(const Rational& numerator, const Rational& discriminant, bool plus)
{
    Rational sqrtValue;
    if (isPerfectSquare(discriminant, sqrtValue)) {
        return (plus ? numerator + sqrtValue : numerator - sqrtValue).toHtml();
    }

    QString sign = plus ? " + " : " - ";
    return "(" + numerator.toHtml() + sign + "&radic;(" + discriminant.toHtml() + "))";
}

QString coefficientTimesX(const Rational& coefficient)
{
    if (coefficient == Rational(1)) {
        return "x";
    }
    if (coefficient == Rational(-1)) {
        return "-x";
    }
    if (coefficient.denominator() == 1) {
        return coefficient.toHtml() + "x";
    }
    return "(" + coefficient.toHtml() + ")x";
}

QString joinSignedTerms(const QStringList& terms)
{
    QString text = terms.join(" + ");
    text.replace("+ -", "- ");
    return text;
}

QString joinGeneralSolution(const QString& homogeneous, const QString& particular)
{
    if (particular == "0") {
        return homogeneous;
    }
    if (particular.startsWith('-')) {
        return homogeneous + " - " + particular.mid(1);
    }
    return homogeneous + " + " + particular;
}
}

SolverResult OdeSolver::solve(const SolverInput& input) const
{
    if (input.order != 1 && input.order != 2) {
        return {false, "当前原型先支持一阶和二阶常系数线性方程。", {}, {}};
    }

    try {
        if (input.kind == NonHomogeneousKind::PolynomialExp) {
            return solvePolynomialExp(input);
        }
        return solveExpTrig(input);
    } catch (const std::exception& ex) {
        return {false, QString::fromUtf8(ex.what()), {}, {}};
    }
}

SolverResult OdeSolver::solvePolynomialExp(const SolverInput& input) const
{
    if (input.polynomialAscending.empty()) {
        return {false, "请输入 P(x) 的系数。", {}, {}};
    }

    std::vector<ComplexRational> forcing;
    forcing.reserve(input.polynomialAscending.size());
    for (const Rational& value : input.polynomialAscending) {
        forcing.emplace_back(value, Rational(0));
    }

    const auto characteristic = characteristicDescending(input);
    const ComplexRational lambda(input.expR, Rational(0));
    const auto particular = solveComplexPolynomialExp(input.order, characteristic, lambda, forcing);
    const QString yh = homogeneousSolution(input);

    QStringList lines;
    lines << "<h2>求解结果</h2>";
    lines << "<div class='summary'>"
        + summaryRow("齐次通解", "y<sub>h</sub> = " + yh)
        + summaryRow("特解", "y<sub>p</sub> = " + particular.particular)
        + summaryRow("通解", "y = " + joinGeneralSolution(yh, particular.particular))
        + "</div>";

    lines << "<h2>推导步骤</h2>";
    lines << "<p>标准方程写作 "
        + inlineFormula("L(D)y=f(x)")
        + "，特征多项式为 "
        + inlineFormula("F(&lambda;)") + "。</p>";
    lines << "<p>本项为 " + inlineFormula("P(x)e<sup>rx</sup>")
        + "，其中 " + inlineFormula("r = " + formatReal(input.expR))
        + "，" + inlineFormula("P(x) = " + formatPolynomialReal(input.polynomialAscending))
        + "。</p>";
    lines << "<p>精确计算 "
        + inlineFormula("F(r), F&prime;(r), ...")
        + " 后，" + inlineFormula("r")
        + " 的特征根重数 "
        + inlineFormula("j = " + QString::number(particular.multiplicity))
        + "。</p>";
    lines << "<p>按论文结论，设 " + inlineFormula(particular.ansatz) + "。</p>";
    lines << "<p>利用公式：</p>";
    lines << displayFormula(
        "L(D)(x<sup>q</sup>e<sup>rx</sup>) = "
        "e<sup>rx</sup>&sum;<sub>i=0</sub><sup>q</sup>"
        "C(q,i)F<sup>(i)</sup>(r)x<sup>q-i</sup>");
    lines << particular.matrixHtml;
    lines << "<p>用有理数高斯消元，解得 Q(x) = "
        + inlineFormula(formatPolynomialComplex(particular.coefficientsAscending)) + "。</p>";

    SolverResult result;
    result.ok = true;
    result.plainSolution = "y = " + joinGeneralSolution(yh, particular.particular);
    result.html = joinLines(lines);
    return result;
}

SolverResult OdeSolver::solveExpTrig(const SolverInput& input) const
{
    if (input.cosinePolynomialAscending.empty() && input.sinePolynomialAscending.empty()) {
        return {false, "请输入 R(x) 或 I(x) 的系数。", {}, {}};
    }

    const int degree = static_cast<int>(std::max(
        input.cosinePolynomialAscending.size(),
        input.sinePolynomialAscending.size())) - 1;
    std::vector<ComplexRational> forcingAscending(
        degree + 1, ComplexRational(Rational(0), Rational(0)));
    for (int i = 0; i < static_cast<int>(input.cosinePolynomialAscending.size()); ++i) {
        forcingAscending[i] = forcingAscending[i]
            + ComplexRational(input.cosinePolynomialAscending[i], Rational(0));
    }
    for (int i = 0; i < static_cast<int>(input.sinePolynomialAscending.size()); ++i) {
        forcingAscending[i] = forcingAscending[i]
            + ComplexRational(Rational(0), -input.sinePolynomialAscending[i]);
    }

    const auto characteristic = characteristicDescending(input);
    const ComplexRational lambda(input.trigA, input.trigB);
    const auto particular = solveComplexPolynomialExp(input.order, characteristic, lambda, forcingAscending);
    const QString yh = homogeneousSolution(input);

    std::vector<Rational> realPart(particular.coefficientsAscending.size(), Rational(0));
    std::vector<Rational> negativeImagPart(particular.coefficientsAscending.size(), Rational(0));
    for (int i = 0; i < static_cast<int>(particular.coefficientsAscending.size()); ++i) {
        realPart[i] = particular.coefficientsAscending[i].real();
        negativeImagPart[i] = -particular.coefficientsAscending[i].imag();
    }

    const QString u = formatPolynomialReal(realPart);
    const QString v = formatPolynomialReal(negativeImagPart);
    const auto shiftedRealPart = shiftPolynomial(realPart, particular.multiplicity);
    const auto shiftedNegativeImagPart = shiftPolynomial(negativeImagPart, particular.multiplicity);
    QStringList trigTerms;
    const QString cosTerm = formatTrigTerm(shiftedRealPart, "cos", input.trigB);
    const QString sinTerm = formatTrigTerm(shiftedNegativeImagPart, "sin", input.trigB);
    if (!cosTerm.isEmpty()) {
        trigTerms << cosTerm;
    }
    if (!sinTerm.isEmpty()) {
        trigTerms << sinTerm;
    }
    const QString trigPart = trigTerms.isEmpty() ? "0" : joinSignedTerms(trigTerms);
    const QString expFactor = formatExponentialFactor(input.trigA);
    const QString yp = expFactor.isEmpty() ? trigPart : expFactor + "[" + trigPart + "]";

    QStringList lines;
    lines << "<h2>求解结果</h2>";
    lines << "<div class='summary'>"
        + summaryRow("齐次通解", "y<sub>h</sub> = " + yh)
        + summaryRow("特解", "y<sub>p</sub> = " + yp)
        + summaryRow("通解", "y = " + joinGeneralSolution(yh, yp))
        + "</div>";

    lines << "<h2>推导步骤</h2>";
    lines << "<p>三角型右端项写作 "
        + inlineFormula("e<sup>ax</sup>(R(x)<span class='op'>cos</span> bx + I(x)<span class='op'>sin</span> bx)")
        + "。</p>";
    lines << "<p>令 " + inlineFormula("z = a + bi = " + formatComplex(lambda))
        + "，并构造复多项式 " + inlineFormula("C(x)=R(x)-iI(x)") + "。</p>";
    lines << "<p>这里 "
        + inlineFormula("R(x) = " + formatPolynomialReal(input.cosinePolynomialAscending))
        + "，" + inlineFormula("I(x) = " + formatPolynomialReal(input.sinePolynomialAscending))
        + "，所以 "
        + inlineFormula("C(x) = " + formatPolynomialComplex(forcingAscending)) + "。</p>";
    lines << "<p>先解复指数问题 "
        + inlineFormula("L(D)Y = C(x)e<sup>zx</sup>")
        + "，再取实部回到原三角函数。</p>";
    lines << "<p>" + inlineFormula("z") + " 的特征根重数 "
        + inlineFormula("j = " + QString::number(particular.multiplicity))
        + "，设 " + inlineFormula(particular.ansatz) + "。</p>";
    lines << particular.matrixHtml;
    lines << "<p>用复有理数高斯消元，解得 Q(x) = "
        + inlineFormula(formatPolynomialComplex(particular.coefficientsAscending)) + "。</p>";
    lines << "<p>若 " + inlineFormula("Q(x)=U(x)+iV(x)")
        + "，则 "
        + inlineFormula("Re(Q(x)e<sup>ibx</sup>) = U(x)<span class='op'>cos</span> bx - V(x)<span class='op'>sin</span> bx")
        + "。</p>";
    lines << "<p>因此 " + inlineFormula("U(x) = " + u)
        + "，" + inlineFormula("-V(x) = " + v)
        + "；再与共振因子 " + inlineFormula("x<sup>j</sup>")
        + " 合并后得到上面的特解。</p>";

    SolverResult result;
    result.ok = true;
    result.plainSolution = "y = " + joinGeneralSolution(yh, yp);
    result.html = joinLines(lines);
    return result;
}

OdeSolver::ParticularResult OdeSolver::solveComplexPolynomialExp(
    int order,
    const std::vector<Rational>& characteristic,
    ComplexRational lambda,
    const std::vector<ComplexRational>& forcingAscending) const
{
    const int degree = static_cast<int>(forcingAscending.size()) - 1;
    const int multiplicity = rootMultiplicity(characteristic, order, lambda);

    std::vector<std::vector<ComplexRational>> matrix(
        degree + 1,
        std::vector<ComplexRational>(degree + 1, ComplexRational(Rational(0), Rational(0))));
    std::vector<ComplexRational> rhs = forcingAscending;

    for (int unknownDegree = 0; unknownDegree <= degree; ++unknownDegree) {
        const int qPower = multiplicity + unknownDegree;
        for (int derivativeOrder = 0; derivativeOrder <= std::min(qPower, order); ++derivativeOrder) {
            const int targetPower = qPower - derivativeOrder;
            if (targetPower < 0 || targetPower > degree) {
                continue;
            }
            matrix[targetPower][unknownDegree] = matrix[targetPower][unknownDegree]
                + ComplexRational(binomial(qPower, derivativeOrder), Rational(0))
                    * evaluateDerivative(characteristic, derivativeOrder, lambda);
        }
    }

    const auto coefficients = solveLinearSystem(matrix, rhs);

    QStringList rows;
    rows << "<table>";
    rows << "<tr><th>幂次</th><th>系数方程</th></tr>";
    for (int power = degree; power >= 0; --power) {
        QStringList terms;
        for (int col = 0; col <= degree; ++col) {
            if (matrix[power][col].isZero()) {
                continue;
            }
            terms << formatComplex(matrix[power][col]) + " q<sub>"
                + QString::number(col) + "</sub>";
        }
        if (terms.isEmpty()) {
            terms << "0";
        }
        rows << "<tr><td><span class='math'>x<sup>" + QString::number(power)
            + "</sup></span></td><td><span class='math'>"
            + terms.join(" + ") + " = " + formatComplex(rhs[power])
            + "</span></td></tr>";
    }
    rows << "</table>";

    const QString qText = formatPolynomialComplex(coefficients);
    const auto shiftedCoefficients = shiftPolynomial(coefficients, multiplicity);
    const QString mergedPolynomial = formatPolynomialFactor(shiftedCoefficients);
    QString ansatz = "Y = ";
    if (multiplicity > 0) {
        ansatz += "x";
        if (multiplicity > 1) {
            ansatz += "<sup>" + QString::number(multiplicity) + "</sup>";
        }
    }
    if (!lambda.isZero()) {
        if (lambda.imag().isZero()) {
            ansatz += formatExponentialFactor(lambda.real());
        } else {
            ansatz += "e<sup>(" + formatComplex(lambda) + ")x</sup>";
        }
    }
    ansatz += "Q(x)";

    QString expFactor;
    if (!lambda.isZero()) {
        if (lambda.imag().isZero()) {
            expFactor = formatExponentialFactor(lambda.real());
        } else {
            expFactor = "e<sup>(" + formatComplex(lambda) + ")x</sup>";
        }
    }
    QString particular = mergedPolynomial;
    if (particular == "1" && !expFactor.isEmpty()) {
        particular = expFactor;
    } else if (!expFactor.isEmpty()) {
        particular += expFactor;
    }

    ParticularResult result;
    result.multiplicity = multiplicity;
    result.lambda = lambda;
    result.forcingAscending = forcingAscending;
    result.coefficientsAscending = coefficients;
    result.matrixHtml = rows.join("");
    result.ansatz = ansatz;
    result.particular = particular;
    return result;
}

std::vector<Rational> OdeSolver::characteristicDescending(const SolverInput& input) const
{
    if (input.order == 1) {
        return {Rational(1), input.a0};
    }
    return {Rational(1), input.a1, input.a0};
}

QString OdeSolver::homogeneousSolution(const SolverInput& input) const
{
    if (input.order == 1) {
        const QString expFactor = formatExponentialFactor(-input.a0);
        return expFactor.isEmpty() ? "C<sub>1</sub>" : "C<sub>1</sub>" + expFactor;
    }

    const Rational discriminant = input.a1 * input.a1 - Rational(4) * input.a0;
    if (discriminant > Rational(0)) {
        Rational sqrtValue;
        if (isPerfectSquare(discriminant, sqrtValue)) {
            const Rational root1 = (-input.a1 + sqrtValue) / Rational(2);
            const Rational root2 = (-input.a1 - sqrtValue) / Rational(2);
            const QString exp1 = formatExponentialFactor(root1);
            const QString exp2 = formatExponentialFactor(root2);
            return "C<sub>1</sub>" + exp1 + " + C<sub>2</sub>" + exp2;
        }
        return "C<sub>1</sub>e<sup>(" + rootExpression(-input.a1, discriminant, true)
            + "/2)x</sup> + C<sub>2</sub>e<sup>("
            + rootExpression(-input.a1, discriminant, false) + "/2)x</sup>";
    }

    if (discriminant == Rational(0)) {
        const Rational root = -input.a1 / Rational(2);
        const QString expFactor = formatExponentialFactor(root);
        return expFactor.isEmpty()
            ? "(C<sub>1</sub> + C<sub>2</sub>x)"
            : "(C<sub>1</sub> + C<sub>2</sub>x)" + expFactor;
    }

    const Rational alpha = -input.a1 / Rational(2);
    const Rational betaSquared = -discriminant;
    Rational beta;
    const QString betaText = isPerfectSquare(betaSquared, beta)
        ? formatTrigArgument(beta / Rational(2))
        : "(&radic;(" + betaSquared.toHtml() + ")/2)x";
    const QString prefix = formatExponentialFactor(alpha);
    return prefix + "(C<sub>1</sub>cos(" + betaText
        + ") + C<sub>2</sub>sin(" + betaText + "))";
}

ComplexRational OdeSolver::evaluateDerivative(
    const std::vector<Rational>& descending,
    int derivativeOrder,
    ComplexRational z) const
{
    const int degree = static_cast<int>(descending.size()) - 1;
    ComplexRational value(Rational(0), Rational(0));
    for (int index = 0; index < static_cast<int>(descending.size()); ++index) {
        const int power = degree - index;
        if (power < derivativeOrder) {
            continue;
        }
        Rational multiplier(1);
        for (int k = 0; k < derivativeOrder; ++k) {
            multiplier = multiplier * Rational(power - k);
        }
        value = value + ComplexRational(descending[index] * multiplier, Rational(0))
            * pow(z, power - derivativeOrder);
    }
    return value;
}

int OdeSolver::rootMultiplicity(
    const std::vector<Rational>& descending,
    int order,
    ComplexRational z) const
{
    for (int derivativeOrder = 0; derivativeOrder <= order; ++derivativeOrder) {
        if (!evaluateDerivative(descending, derivativeOrder, z).isZero()) {
            return derivativeOrder;
        }
    }
    return order;
}

std::vector<ComplexRational> OdeSolver::solveLinearSystem(
    std::vector<std::vector<ComplexRational>> matrix,
    std::vector<ComplexRational> rhs) const
{
    const int n = static_cast<int>(rhs.size());
    for (int pivot = 0; pivot < n; ++pivot) {
        int best = -1;
        double bestMagnitude = -1.0;
        for (int row = pivot; row < n; ++row) {
            if (!matrix[row][pivot].isZero()
                && matrix[row][pivot].magnitudeSquared() > bestMagnitude) {
                best = row;
                bestMagnitude = matrix[row][pivot].magnitudeSquared();
            }
        }
        if (best < 0) {
            throw std::runtime_error("待定系数方程组奇异，当前输入可能超出了原型支持范围。");
        }
        if (best != pivot) {
            std::swap(matrix[pivot], matrix[best]);
            std::swap(rhs[pivot], rhs[best]);
        }

        const ComplexRational divider = matrix[pivot][pivot];
        for (int col = pivot; col < n; ++col) {
            matrix[pivot][col] = matrix[pivot][col] / divider;
        }
        rhs[pivot] = rhs[pivot] / divider;

        for (int row = 0; row < n; ++row) {
            if (row == pivot || matrix[row][pivot].isZero()) {
                continue;
            }
            const ComplexRational factor = matrix[row][pivot];
            for (int col = pivot; col < n; ++col) {
                matrix[row][col] = matrix[row][col] - factor * matrix[pivot][col];
            }
            rhs[row] = rhs[row] - factor * rhs[pivot];
        }
    }
    return rhs;
}

Rational OdeSolver::binomial(int n, int k)
{
    if (k < 0 || k > n) {
        return Rational(0);
    }
    if (k > n - k) {
        k = n - k;
    }
    long long result = 1;
    for (int i = 1; i <= k; ++i) {
        result = result * (n - k + i) / i;
    }
    return Rational(result);
}

QString OdeSolver::formatReal(const Rational& value)
{
    return value.toHtml();
}

QString OdeSolver::formatComplex(const ComplexRational& value)
{
    return value.toHtml();
}

QString OdeSolver::formatPolynomialReal(const std::vector<Rational>& coeffsAscending)
{
    std::vector<ComplexRational> values;
    values.reserve(coeffsAscending.size());
    for (const Rational& value : coeffsAscending) {
        values.emplace_back(value, Rational(0));
    }
    return formatPolynomialComplex(values);
}

QString OdeSolver::formatPolynomialComplex(const std::vector<ComplexRational>& coeffsAscending)
{
    QStringList terms;
    for (int power = static_cast<int>(coeffsAscending.size()) - 1; power >= 0; --power) {
        const ComplexRational coeff = coeffsAscending[power];
        if (coeff.isZero()) {
            continue;
        }

        QString term;
        if (coeff.isReal() && power > 0 && absValue(coeff.real()) == Rational(1)) {
            term = "";
        } else if (coeff.isReal() && power > 0) {
            term = absValue(coeff.real()).toString();
        } else if (coeff.isReal()) {
            term = absValue(coeff.real()).toString();
        } else {
            term = coeff.toString();
            if (power > 0) {
                term = "(" + term + ")";
            }
        }

        if (power == 1) {
            term += "x";
        } else if (power > 1) {
            term += "x<sup>" + QString::number(power) + "</sup>";
        }

        const bool negativeReal = coeff.isReal() && coeff.real() < Rational(0);
        if (terms.isEmpty()) {
            terms << (negativeReal ? "-" + term : term);
        } else {
            terms << (negativeReal ? "- " + term : "+ " + term);
        }
    }
    if (terms.isEmpty()) {
        return "0";
    }
    return terms.join(" ");
}

QString OdeSolver::formatPolynomialFactor(const std::vector<ComplexRational>& coeffsAscending)
{
    const QString polynomial = formatPolynomialComplex(coeffsAscending);
    if (polynomial == "0") {
        return "0";
    }
    return nonZeroTermCount(coeffsAscending) > 1 ? "(" + polynomial + ")" : polynomial;
}

QString OdeSolver::formatExponentialFactor(const Rational& lambda)
{
    if (lambda.isZero()) {
        return "";
    }
    return "e<sup>" + coefficientTimesX(lambda) + "</sup>";
}

QString OdeSolver::formatTrigArgument(const Rational& frequency)
{
    return coefficientTimesX(frequency);
}

QString OdeSolver::formatTrigTerm(
    const std::vector<Rational>& coeffsAscending,
    const QString& functionName,
    const Rational& frequency)
{
    if (isZeroPolynomial(coeffsAscending)) {
        return "";
    }

    std::vector<ComplexRational> complexCoeffs;
    complexCoeffs.reserve(coeffsAscending.size());
    for (const Rational& coeff : coeffsAscending) {
        complexCoeffs.emplace_back(coeff, Rational(0));
    }

    const QString factor = formatPolynomialFactor(complexCoeffs);
    const QString functionPart = functionName + "(" + formatTrigArgument(frequency) + ")";
    if (factor == "1") {
        return functionPart;
    }
    if (factor == "-1") {
        return "-" + functionPart;
    }
    return factor + functionPart;
}

std::vector<Rational> OdeSolver::shiftPolynomial(
    const std::vector<Rational>& coeffsAscending,
    int shift)
{
    if (shift <= 0 || isZeroPolynomial(coeffsAscending)) {
        return coeffsAscending;
    }
    std::vector<Rational> shifted(shift, Rational(0));
    shifted.insert(shifted.end(), coeffsAscending.begin(), coeffsAscending.end());
    return shifted;
}

std::vector<ComplexRational> OdeSolver::shiftPolynomial(
    const std::vector<ComplexRational>& coeffsAscending,
    int shift)
{
    if (shift <= 0) {
        return coeffsAscending;
    }
    bool zero = true;
    for (const ComplexRational& coeff : coeffsAscending) {
        if (!coeff.isZero()) {
            zero = false;
            break;
        }
    }
    if (zero) {
        return coeffsAscending;
    }
    std::vector<ComplexRational> shifted(
        shift, ComplexRational(Rational(0), Rational(0)));
    shifted.insert(shifted.end(), coeffsAscending.begin(), coeffsAscending.end());
    return shifted;
}

bool OdeSolver::isZeroPolynomial(const std::vector<Rational>& coeffsAscending)
{
    for (const Rational& coeff : coeffsAscending) {
        if (!coeff.isZero()) {
            return false;
        }
    }
    return true;
}

int OdeSolver::nonZeroTermCount(const std::vector<ComplexRational>& coeffsAscending)
{
    int count = 0;
    for (const ComplexRational& coeff : coeffsAscending) {
        if (!coeff.isZero()) {
            ++count;
        }
    }
    return count;
}

QString OdeSolver::formatPower(const QString& base, int power)
{
    if (power == 0) {
        return "1";
    }
    if (power == 1) {
        return base;
    }
    return base + "<sup>" + QString::number(power) + "</sup>";
}

QString OdeSolver::htmlEscape(const QString& value)
{
    return value.toHtmlEscaped();
}
