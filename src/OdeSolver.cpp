#include "OdeSolver.h"

#include <QStringList>
#include <algorithm>
#include <stdexcept>

namespace {
QString joinLines(const QStringList& lines)
{
    return lines.join("");
}

QString texEscape(const QString& body)
{
    return body.toHtmlEscaped();
}

QString inlineFormula(const QString& body)
{
    return "<span class='tex-inline'>" + texEscape(body) + "</span>";
}

QString displayFormula(const QString& body)
{
    return "<div class='equation tex-display'>" + texEscape(body) + "</div>";
}

QString summaryRow(const QString& label, const QString& formula)
{
    return "<div class='summary-row'><div class='summary-label'>" + label
        + "</div><div>" + inlineFormula(formula) + "</div></div>";
}

QString rootExpression(const Rational& numerator, const Rational& discriminant, bool plus)
{
    Rational sqrtValue;
    if (isPerfectSquare(discriminant, sqrtValue)) {
        return (plus ? numerator + sqrtValue : numerator - sqrtValue).toLatex();
    }

    QString sign = plus ? " + " : " - ";
    return numerator.toLatex() + sign + "\\sqrt{" + discriminant.toLatex() + "}";
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
        return coefficient.toLatex() + "x";
    }
    return coefficient.toLatex() + "x";
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

QString powerLatex(const QString& base, int power)
{
    if (power == 0) {
        return "1";
    }
    if (power == 1) {
        return base;
    }
    return base + "^{" + QString::number(power) + "}";
}

QString formatPolynomialDescendingLatex(
    const std::vector<Rational>& coeffsDescending,
    const QString& variable)
{
    QStringList terms;
    const int degree = static_cast<int>(coeffsDescending.size()) - 1;
    for (int index = 0; index < static_cast<int>(coeffsDescending.size()); ++index) {
        const Rational coeff = coeffsDescending[index];
        if (coeff.isZero()) {
            continue;
        }

        const int power = degree - index;
        const Rational absCoeff = absValue(coeff);
        QString term;
        if (power == 0) {
            term = absCoeff.toLatex();
        } else {
            if (absCoeff != Rational(1)) {
                term = absCoeff.toLatex();
            }
            term += powerLatex(variable, power);
        }

        if (terms.isEmpty()) {
            terms << (coeff < Rational(0) ? "-" + term : term);
        } else {
            terms << (coeff < Rational(0) ? "- " + term : "+ " + term);
        }
    }
    return terms.isEmpty() ? "0" : terms.join(" ");
}

QString columnVectorLatex(const QStringList& entries)
{
    return "\\begin{bmatrix}" + entries.join("\\\\") + "\\end{bmatrix}";
}

QString unknownVectorLatex(int degree)
{
    QStringList entries;
    for (int k = degree; k >= 0; --k) {
        entries << "q_{" + QString::number(k) + "}";
    }
    return columnVectorLatex(entries);
}

QString realImagUnknownVectorLatex(int degree)
{
    QStringList entries;
    for (int k = degree; k >= 0; --k) {
        entries << "q_{" + QString::number(k) + ",R}";
    }
    for (int k = degree; k >= 0; --k) {
        entries << "q_{" + QString::number(k) + ",I}";
    }
    return columnVectorLatex(entries);
}

QString complexVectorLatex(const std::vector<ComplexRational>& valuesAscending)
{
    QStringList entries;
    for (int k = static_cast<int>(valuesAscending.size()) - 1; k >= 0; --k) {
        entries << valuesAscending[k].toLatex();
    }
    return columnVectorLatex(entries);
}

QString realImagRhsVectorLatex(const std::vector<ComplexRational>& valuesAscending)
{
    QStringList entries;
    for (int k = static_cast<int>(valuesAscending.size()) - 1; k >= 0; --k) {
        entries << valuesAscending[k].real().toLatex();
    }
    for (int k = static_cast<int>(valuesAscending.size()) - 1; k >= 0; --k) {
        entries << valuesAscending[k].imag().toLatex();
    }
    return columnVectorLatex(entries);
}

QString realVectorLatex(const std::vector<ComplexRational>& valuesAscending, bool imaginary)
{
    QStringList entries;
    for (int k = static_cast<int>(valuesAscending.size()) - 1; k >= 0; --k) {
        entries << (imaginary ? valuesAscending[k].imag() : valuesAscending[k].real()).toLatex();
    }
    return columnVectorLatex(entries);
}

QString complexMatrixLatex(const std::vector<std::vector<ComplexRational>>& matrixAscending)
{
    const int degree = static_cast<int>(matrixAscending.size()) - 1;
    QStringList rows;
    for (int power = degree; power >= 0; --power) {
        QStringList columns;
        for (int k = degree; k >= 0; --k) {
            columns << matrixAscending[power][k].toLatex();
        }
        rows << columns.join(" & ");
    }
    return "\\begin{bmatrix}" + rows.join("\\\\") + "\\end{bmatrix}";
}

QString realMatrixLatex(
    const std::vector<std::vector<ComplexRational>>& matrixAscending,
    bool imaginary,
    bool negate)
{
    const int degree = static_cast<int>(matrixAscending.size()) - 1;
    QStringList rows;
    for (int power = degree; power >= 0; --power) {
        QStringList columns;
        for (int k = degree; k >= 0; --k) {
            Rational value = imaginary ? matrixAscending[power][k].imag() : matrixAscending[power][k].real();
            if (negate) {
                value = -value;
            }
            columns << value.toLatex();
        }
        rows << columns.join(" & ");
    }
    return "\\begin{bmatrix}" + rows.join("\\\\") + "\\end{bmatrix}";
}

QString realImagBlockMatrixLatex(const std::vector<std::vector<ComplexRational>>& matrixAscending)
{
    const int degree = static_cast<int>(matrixAscending.size()) - 1;
    const int size = degree + 1;
    QString leftSpec(size, 'c');
    QString rightSpec(size, 'c');
    QStringList upperRows;
    QStringList lowerRows;

    for (int power = degree; power >= 0; --power) {
        QStringList columns;
        for (int k = degree; k >= 0; --k) {
            columns << matrixAscending[power][k].real().toLatex();
        }
        for (int k = degree; k >= 0; --k) {
            columns << (-matrixAscending[power][k].imag()).toLatex();
        }
        upperRows << columns.join(" & ");
    }
    for (int power = degree; power >= 0; --power) {
        QStringList columns;
        for (int k = degree; k >= 0; --k) {
            columns << matrixAscending[power][k].imag().toLatex();
        }
        for (int k = degree; k >= 0; --k) {
            columns << matrixAscending[power][k].real().toLatex();
        }
        lowerRows << columns.join(" & ");
    }

    return "\\left[\\begin{array}{" + leftSpec + "|" + rightSpec + "}"
        + upperRows.join("\\\\")
        + "\\\\" "\\hline "
        + lowerRows.join("\\\\")
        + "\\end{array}\\right]";
}

QString coefficientTermLatex(const ComplexRational& coefficient, const QString& variable, bool first)
{
    if (coefficient.isZero()) {
        return (first ? "" : "+ ") + QString("0\\cdot ") + variable;
    }

    if (coefficient.isReal()) {
        const Rational absCoeff = absValue(coefficient.real());
        const QString body = absCoeff == Rational(1)
            ? variable
            : absCoeff.toLatex() + variable;
        if (coefficient.real() < Rational(0)) {
            return first ? "-" + body : "- " + body;
        }
        return first ? body : "+ " + body;
    }

    return (first ? "" : "+ ") + QString("\\left(") + coefficient.toLatex() + "\\right)" + variable;
}

QString realCoefficientTermLatex(const Rational& coefficient, const QString& variable, bool first)
{
    if (coefficient.isZero()) {
        return (first ? "" : "+ ") + QString("0\\cdot ") + variable;
    }

    const Rational absCoeff = absValue(coefficient);
    const QString body = absCoeff == Rational(1)
        ? variable
        : absCoeff.toLatex() + variable;
    if (coefficient < Rational(0)) {
        return first ? "-" + body : "- " + body;
    }
    return first ? body : "+ " + body;
}

QString expandedCoefficientSystemLatex(
    const std::vector<std::vector<ComplexRational>>& matrixAscending,
    const std::vector<ComplexRational>& rhsAscending)
{
    const int degree = static_cast<int>(rhsAscending.size()) - 1;
    QStringList rows;
    for (int power = degree; power >= 0; --power) {
        QStringList terms;
        bool first = true;
        for (int k = degree; k >= 0; --k) {
            terms << coefficientTermLatex(
                matrixAscending[power][k],
                "q_{" + QString::number(k) + "}",
                first);
            first = false;
        }
        rows << "\\left[x^{" + QString::number(power) + "}\\right]\\quad & "
            + terms.join(" ") + " = " + rhsAscending[power].toLatex();
    }
    return "\\begin{aligned}" + rows.join("\\\\") + "\\end{aligned}";
}

QString expandedRealImagSystemLatex(
    const std::vector<std::vector<ComplexRational>>& matrixAscending,
    const std::vector<ComplexRational>& rhsAscending)
{
    const int degree = static_cast<int>(rhsAscending.size()) - 1;
    QStringList rows;
    for (int power = degree; power >= 0; --power) {
        QStringList realTerms;
        bool first = true;
        for (int k = degree; k >= 0; --k) {
            realTerms << realCoefficientTermLatex(
                matrixAscending[power][k].real(),
                "q_{" + QString::number(k) + ",R}",
                first);
            first = false;
        }
        for (int k = degree; k >= 0; --k) {
            realTerms << realCoefficientTermLatex(
                -matrixAscending[power][k].imag(),
                "q_{" + QString::number(k) + ",I}",
                first);
            first = false;
        }
        rows << "\\left[x^{" + QString::number(power) + "}\\right]_R\\quad & "
            + realTerms.join(" ") + " = " + rhsAscending[power].real().toLatex();

        QStringList imagTerms;
        first = true;
        for (int k = degree; k >= 0; --k) {
            imagTerms << realCoefficientTermLatex(
                matrixAscending[power][k].imag(),
                "q_{" + QString::number(k) + ",R}",
                first);
            first = false;
        }
        for (int k = degree; k >= 0; --k) {
            imagTerms << realCoefficientTermLatex(
                matrixAscending[power][k].real(),
                "q_{" + QString::number(k) + ",I}",
                first);
            first = false;
        }
        rows << "\\left[x^{" + QString::number(power) + "}\\right]_I\\quad & "
            + imagTerms.join(" ") + " = " + rhsAscending[power].imag().toLatex();
    }
    return "\\begin{aligned}" + rows.join("\\\\") + "\\end{aligned}";
}

QString coefficientPolynomialLatex(int degree, const QString& coefficientSymbol)
{
    QStringList terms;
    for (int k = degree; k >= 0; --k) {
        QString term = coefficientSymbol + "_{" + QString::number(k) + "}";
        if (k > 0) {
            term += powerLatex("x", k);
        }
        terms << term;
    }
    return terms.join(" + ");
}

QString jOffsetLatex(int offset)
{
    if (offset == 0) {
        return "j";
    }
    return "j+" + QString::number(offset);
}

QString symbolicZEntryLatex(int coefficientDegree, int targetPower)
{
    if (targetPower > coefficientDegree) {
        return "0";
    }

    const int derivativeOffset = coefficientDegree - targetPower;
    const QString derivativeOrder = jOffsetLatex(derivativeOffset);
    const QString sourcePower = jOffsetLatex(coefficientDegree);
    return "F^{(" + derivativeOrder + ")}(\\lambda)C_{"
        + sourcePower + "}^{" + derivativeOrder + "}";
}

QString symbolicLowerTriangularMatrixLatex(int degree)
{
    QStringList rows;
    for (int power = degree; power >= 0; --power) {
        QStringList columns;
        for (int k = degree; k >= 0; --k) {
            columns << symbolicZEntryLatex(k, power);
        }
        rows << columns.join(" & ");
    }
    return "\\begin{bmatrix}" + rows.join("\\\\") + "\\end{bmatrix}";
}

QString symbolicLowerTriangularTemplateLatex()
{
    return "\\begin{bmatrix}"
        "F^{(j)}(\\lambda)C_{j+m}^{j} & 0 & 0 & \\cdots & 0\\\\"
        "F^{(j+1)}(\\lambda)C_{j+m}^{j+1} & F^{(j)}(\\lambda)C_{j+m-1}^{j} & 0 & \\cdots & 0\\\\"
        "F^{(j+2)}(\\lambda)C_{j+m}^{j+2} & F^{(j+1)}(\\lambda)C_{j+m-1}^{j+1} & F^{(j)}(\\lambda)C_{j+m-2}^{j} & \\cdots & 0\\\\"
        "\\vdots & \\vdots & \\vdots & \\ddots & \\vdots\\\\"
        "F^{(j+m)}(\\lambda)C_{j+m}^{j+m} & F^{(j+m-1)}(\\lambda)C_{j+m-1}^{j+m-1} & F^{(j+m-2)}(\\lambda)C_{j+m-2}^{j+m-2} & \\cdots & F^{(j)}(\\lambda)C_{j}^{j}"
        "\\end{bmatrix}";
}

QString coefficientVectorTemplateLatex(const QString& coefficientSymbol)
{
    return "\\begin{bmatrix}"
        + coefficientSymbol + "_m\\\\"
        + coefficientSymbol + "_{m-1}\\\\"
        "\\vdots\\\\"
        + coefficientSymbol + "_0"
        "\\end{bmatrix}";
}

QString coefficientSystemHtml(
    const std::vector<std::vector<ComplexRational>>& matrixAscending,
    const std::vector<ComplexRational>& rhsAscending,
    int multiplicity,
    ComplexRational lambda,
    const QString& forcingName,
    bool includeRealImagBlock)
{
    const int degree = static_cast<int>(rhsAscending.size()) - 1;
    QString html;
    html += "<h3>4. 构造待定系数方程组</h3>";
    html += "<p>为了和论文例题中的矩阵写法对应，先把特解中的待定多项式和右端多项式因子都写成系数列：</p>";
    html += displayFormula(
        "Q(x)=" + coefficientPolynomialLatex(degree, "q")
        + ",\\qquad "
        + forcingName + "(x)=" + coefficientPolynomialLatex(degree, "c"));
    html += "<p>于是从多项式到向量的过程就是把系数按高次到低次排成列向量：</p>";
    html += displayFormula(
        "\\mathbf{q}=" + unknownVectorLatex(degree)
        + ",\\qquad \\mathbf{c}=" + complexVectorLatex(rhsAscending));
    html += "<p>论文中的完整下三角模板是下面这个样子；本题中 "
        + inlineFormula("m=" + QString::number(degree))
        + "，也就是右端多项式的次数：</p>";
    html += displayFormula(
        "Z\\mathbf{q}="
        + symbolicLowerTriangularTemplateLatex()
        + coefficientVectorTemplateLatex("q")
        + "=" + coefficientVectorTemplateLatex("c"));
    html += "<p>把这个模板按当前题目的次数写成实际大小，得到符号矩阵：</p>";
    html += displayFormula("Z=" + symbolicLowerTriangularMatrixLatex(degree));
    html += "<p>其中每个非零元素都来自同一个规则：对 "
        + inlineFormula("q_kx^k")
        + " 代入核心公式后，比较 " + inlineFormula("x^p")
        + " 的系数，得到：</p>";
    html += displayFormula(
        "Z_{p,k}=\\begin{cases}"
        "F^{(j+k-p)}(\\lambda)C_{j+k}^{j+k-p},&0\\le p\\le k,\\\\"
        "0,&p>k,"
        "\\end{cases}"
        "\\quad p,k=0,1,\\ldots," + QString::number(degree)
        + ",\\quad j=" + QString::number(multiplicity)
        + ",\\quad \\lambda=" + lambda.toLatex());
    html += "<p>把当前题目的 " + inlineFormula("F^{(i)}(\\lambda)")
        + " 和组合数代入，得到实际矩阵方程：</p>";
    html += displayFormula(
        complexMatrixLatex(matrixAscending)
        + unknownVectorLatex(degree)
        + "=" + complexVectorLatex(rhsAscending));
    html += "<p>把同一个矩阵方程按幂次逐块展开，就是：</p>";
    html += displayFormula(expandedCoefficientSystemLatex(matrixAscending, rhsAscending));

    if (includeRealImagBlock) {
        html += "<h3>5. 实部与虚部分块</h3>";
        html += "<p>程序内部先解上面的复有理数小系统 "
            + inlineFormula("Z\\mathbf{q}=\\mathbf{c}")
            + "，这样维数较低；而答题卡上为了让 "
            + inlineFormula("\\cos bx")
            + " 与 " + inlineFormula("\\sin bx")
            + " 前的系数对齐，可以把它等价展开成较大的实系数块系统。令 "
            + inlineFormula("Z=Z_R+iZ_I")
            + "，" + inlineFormula("\\mathbf{q}=\\mathbf{q}_R+i\\mathbf{q}_I")
            + "，" + inlineFormula("\\mathbf{c}=\\mathbf{c}_R+i\\mathbf{c}_I")
            + "，则：</p>";
        html += displayFormula(
            "\\left[\\begin{array}{c|c}Z_R&-Z_I" "\\\\" "\\hline Z_I&Z_R\\end{array}\\right]"
            "\\begin{bmatrix}\\mathbf{q}_R\\\\\\mathbf{q}_I\\end{bmatrix}"
            "="
            "\\begin{bmatrix}\\mathbf{c}_R\\\\\\mathbf{c}_I\\end{bmatrix}");
        html += displayFormula(
            "Z_R=" + realMatrixLatex(matrixAscending, false, false)
            + ",\\qquad Z_I=" + realMatrixLatex(matrixAscending, true, false));
        html += displayFormula(
            "\\mathbf{c}_R=" + realVectorLatex(rhsAscending, false)
            + ",\\qquad \\mathbf{c}_I=" + realVectorLatex(rhsAscending, true));
        html += "<p>把上面三个块直接代入，得到真正用于对齐实部、虚部系数的大矩阵：</p>";
        html += displayFormula(
            realImagBlockMatrixLatex(matrixAscending)
            + realImagUnknownVectorLatex(degree)
            + "=" + realImagRhsVectorLatex(rhsAscending));
        html += "<p>等价地，它分成下面两块实线性方程组：</p>";
        html += displayFormula(
            "\\begin{aligned}"
            "Z_R\\mathbf{q}_R-Z_I\\mathbf{q}_I&=\\mathbf{c}_R\\\\"
            "Z_I\\mathbf{q}_R+Z_R\\mathbf{q}_I&=\\mathbf{c}_I"
            "\\end{aligned}");
        html += "<p>逐行展开后，每一行就对应同一次幂下的实部或虚部系数：</p>";
        html += displayFormula(expandedRealImagSystemLatex(matrixAscending, rhsAscending));
    }

    return html;
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
    const auto particular = solveComplexPolynomialExp(input.order, characteristic, lambda, forcing, false);
    const QString yh = homogeneousSolution(input);

    QStringList lines;
    lines << "<h2>求解结果</h2>";
    lines << "<div class='summary'>"
        + summaryRow("齐次通解", "y_h = " + yh)
        + summaryRow("特解", "y_p = " + particular.particular)
        + summaryRow("通解", "y = " + joinGeneralSolution(yh, particular.particular))
        + "</div>";

    lines << "<h2>推导步骤</h2>";
    lines << "<h3>1. 标准化与特征多项式</h3>";
    lines << "<p>这里 " + inlineFormula("D=\\frac{d}{dx}")
        + "，" + inlineFormula("L(D)")
        + " 表示左端对应的线性微分算子：一阶时是 "
        + inlineFormula("D+a_0")
        + "，二阶时是 " + inlineFormula("D^2+a_1D+a_0")
        + "；把它作用在 " + inlineFormula("y")
        + " 上，就写成 " + inlineFormula("L(D)y=f(x)") + "。</p>";
    lines << "<p>把这个算子里的 " + inlineFormula("D")
        + " 换成普通变量 " + inlineFormula("\\lambda")
        + "，就得到特征多项式 "
        + inlineFormula("F(\\lambda)=" + formatPolynomialDescendingLatex(characteristic, "\\lambda"))
        + "。</p>";
    lines << "<h3>2. 识别非齐次项与根重数</h3>";
    lines << "<p>本项为 " + inlineFormula("P(x)e^{rx}")
        + "，其中 " + inlineFormula("r = " + formatReal(input.expR))
        + "，" + inlineFormula("P(x) = " + formatPolynomialReal(input.polynomialAscending))
        + "。</p>";
    lines << "<p>精确计算 "
        + inlineFormula("F(r), F'(r), \\ldots")
        + " 后，" + inlineFormula("r")
        + " 的特征根重数 "
        + inlineFormula("j = " + QString::number(particular.multiplicity))
        + "。</p>";
    lines << "<h3>3. 设置特解形式</h3>";
    lines << "<p>按论文结论，设 " + inlineFormula(particular.ansatz) + "。</p>";
    lines << "<p>利用公式：</p>";
    lines << displayFormula(
        "L(D)(x^{q}e^{rx}) = "
        "e^{rx}\\sum_{i=0}^{q} C(q,i)F^{(i)}(r)x^{q-i}");
    lines << particular.matrixHtml;
    lines << "<h3>5. 求解待定系数并合成</h3>";
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
    const auto particular = solveComplexPolynomialExp(input.order, characteristic, lambda, forcingAscending, true);
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
    const QString yp = expFactor.isEmpty() ? trigPart : expFactor + "\\left[" + trigPart + "\\right]";

    QStringList lines;
    lines << "<h2>求解结果</h2>";
    lines << "<div class='summary'>"
        + summaryRow("齐次通解", "y_h = " + yh)
        + summaryRow("特解", "y_p = " + yp)
        + summaryRow("通解", "y = " + joinGeneralSolution(yh, yp))
        + "</div>";

    lines << "<h2>推导步骤</h2>";
    lines << "<h3>1. 标准化与特征多项式</h3>";
    lines << "<p>这里 " + inlineFormula("D=\\frac{d}{dx}")
        + "，" + inlineFormula("L(D)")
        + " 表示左端对应的线性微分算子：一阶时是 "
        + inlineFormula("D+a_0")
        + "，二阶时是 " + inlineFormula("D^2+a_1D+a_0")
        + "；把它作用在 " + inlineFormula("y")
        + " 上，就写成 " + inlineFormula("L(D)y=f(x)") + "。</p>";
    lines << "<p>把这个算子里的 " + inlineFormula("D")
        + " 换成普通变量 " + inlineFormula("\\lambda")
        + "，就得到特征多项式 "
        + inlineFormula("F(\\lambda)=" + formatPolynomialDescendingLatex(characteristic, "\\lambda"))
        + "。</p>";
    lines << "<h3>2. 复指数化右端项</h3>";
    lines << "<p>三角型右端项写作 "
        + inlineFormula("e^{ax}(R(x)\\cos bx + I(x)\\sin bx)")
        + "。</p>";
    lines << "<p>令 " + inlineFormula("z = a + bi = " + formatComplex(lambda))
        + "，并构造复多项式 " + inlineFormula("C(x)=R(x)-iI(x)") + "。</p>";
    lines << "<p>这里 "
        + inlineFormula("R(x) = " + formatPolynomialReal(input.cosinePolynomialAscending))
        + "，" + inlineFormula("I(x) = " + formatPolynomialReal(input.sinePolynomialAscending))
        + "，所以 "
        + inlineFormula("C(x) = " + formatPolynomialComplex(forcingAscending)) + "。</p>";
    lines << "<p>先解复指数问题 "
        + inlineFormula("L(D)Y = C(x)e^{zx}")
        + "，再取实部回到原三角函数。</p>";
    lines << "<h3>3. 设置复特解形式</h3>";
    lines << "<p>" + inlineFormula("z") + " 的特征根重数 "
        + inlineFormula("j = " + QString::number(particular.multiplicity))
        + "，设 " + inlineFormula(particular.ansatz) + "。</p>";
    lines << particular.matrixHtml;
    lines << "<h3>6. 求解待定系数并回到实特解</h3>";
    lines << "<p>用复有理数高斯消元，解得 Q(x) = "
        + inlineFormula(formatPolynomialComplex(particular.coefficientsAscending)) + "。</p>";
    lines << "<p>若 " + inlineFormula("Q(x)=U(x)+iV(x)")
        + "，则 "
        + inlineFormula("\\operatorname{Re}(Q(x)e^{ibx}) = U(x)\\cos bx - V(x)\\sin bx")
        + "。</p>";
    lines << "<p>因此 " + inlineFormula("U(x) = " + u)
        + "，" + inlineFormula("-V(x) = " + v)
        + "；再与共振因子 " + inlineFormula("x^j")
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
    const std::vector<ComplexRational>& forcingAscending,
    bool includeRealImagBlock) const
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
    const QString coefficientSystem = coefficientSystemHtml(
        matrix,
        rhs,
        multiplicity,
        lambda,
        includeRealImagBlock ? "C" : "P",
        includeRealImagBlock);
    const auto shiftedCoefficients = shiftPolynomial(coefficients, multiplicity);
    const QString mergedPolynomial = formatPolynomialFactor(shiftedCoefficients);
    QString ansatz = "Y = ";
    if (multiplicity > 0) {
        ansatz += "x";
        if (multiplicity > 1) {
            ansatz += "^{" + QString::number(multiplicity) + "}";
        }
    }
    if (!lambda.isZero()) {
        if (lambda.imag().isZero()) {
            ansatz += formatExponentialFactor(lambda.real());
        } else {
            ansatz += "e^{(" + formatComplex(lambda) + ")x}";
        }
    }
    ansatz += "Q(x)";

    QString expFactor;
    if (!lambda.isZero()) {
        if (lambda.imag().isZero()) {
            expFactor = formatExponentialFactor(lambda.real());
        } else {
            expFactor = "e^{(" + formatComplex(lambda) + ")x}";
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
    result.matrixHtml = coefficientSystem;
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
        return expFactor.isEmpty() ? "C_1" : "C_1" + expFactor;
    }

    const Rational discriminant = input.a1 * input.a1 - Rational(4) * input.a0;
    if (discriminant > Rational(0)) {
        Rational sqrtValue;
        if (isPerfectSquare(discriminant, sqrtValue)) {
            const Rational root1 = (-input.a1 + sqrtValue) / Rational(2);
            const Rational root2 = (-input.a1 - sqrtValue) / Rational(2);
            const QString exp1 = formatExponentialFactor(root1);
            const QString exp2 = formatExponentialFactor(root2);
            return "C_1" + exp1 + " + C_2" + exp2;
        }
        return "C_1e^{\\frac{" + rootExpression(-input.a1, discriminant, true)
            + "}{2}x} + C_2e^{\\frac{"
            + rootExpression(-input.a1, discriminant, false) + "}{2}x}";
    }

    if (discriminant == Rational(0)) {
        const Rational root = -input.a1 / Rational(2);
        const QString expFactor = formatExponentialFactor(root);
        return expFactor.isEmpty()
            ? "\\left(C_1 + C_2x\\right)"
            : "\\left(C_1 + C_2x\\right)" + expFactor;
    }

    const Rational alpha = -input.a1 / Rational(2);
    const Rational betaSquared = -discriminant;
    Rational beta;
    const QString betaText = isPerfectSquare(betaSquared, beta)
        ? formatTrigArgument(beta / Rational(2))
        : "\\frac{\\sqrt{" + betaSquared.toLatex() + "}}{2}x";
    const QString prefix = formatExponentialFactor(alpha);
    return prefix + "\\left(C_1\\cos\\left(" + betaText
        + "\\right) + C_2\\sin\\left(" + betaText + "\\right)\\right)";
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
    return value.toLatex();
}

QString OdeSolver::formatComplex(const ComplexRational& value)
{
    return value.toLatex();
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
            term = absValue(coeff.real()).toLatex();
        } else if (coeff.isReal()) {
            term = absValue(coeff.real()).toLatex();
        } else {
            term = coeff.toLatex();
            if (power > 0) {
                term = "\\left(" + term + "\\right)";
            }
        }

        if (power == 1) {
            term += "x";
        } else if (power > 1) {
            term += "x^{" + QString::number(power) + "}";
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
    return nonZeroTermCount(coeffsAscending) > 1 ? "\\left(" + polynomial + "\\right)" : polynomial;
}

QString OdeSolver::formatExponentialFactor(const Rational& lambda)
{
    if (lambda.isZero()) {
        return "";
    }
    return "e^{" + coefficientTimesX(lambda) + "}";
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
    const QString functionPart = "\\" + functionName + "\\left(" + formatTrigArgument(frequency) + "\\right)";
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
    return base + "^{" + QString::number(power) + "}";
}

QString OdeSolver::htmlEscape(const QString& value)
{
    return value.toHtmlEscaped();
}
