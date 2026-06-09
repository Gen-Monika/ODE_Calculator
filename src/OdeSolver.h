#pragma once

#include "ExactNumber.h"

#include <QString>
#include <vector>

enum class NonHomogeneousKind {
    PolynomialExp,
    ExpTrig
};

struct SolverInput {
    int order = 2;
    Rational a1 = Rational(0);
    Rational a0 = Rational(0);
    NonHomogeneousKind kind = NonHomogeneousKind::PolynomialExp;

    Rational expR = Rational(0);
    std::vector<Rational> polynomialAscending;

    Rational trigA = Rational(0);
    Rational trigB = Rational(1);
    std::vector<Rational> cosinePolynomialAscending;
    std::vector<Rational> sinePolynomialAscending;
};

struct SolverResult {
    bool ok = false;
    QString error;
    QString plainSolution;
    QString html;
};

class OdeSolver {
public:
    SolverResult solve(const SolverInput& input) const;

private:
    struct ParticularResult {
        int multiplicity = 0;
        ComplexRational lambda;
        std::vector<ComplexRational> forcingAscending;
        std::vector<ComplexRational> coefficientsAscending;
        QString matrixHtml;
        QString ansatz;
        QString particular;
    };

    SolverResult solvePolynomialExp(const SolverInput& input) const;
    SolverResult solveExpTrig(const SolverInput& input) const;

    ParticularResult solveComplexPolynomialExp(
        int order,
        const std::vector<Rational>& characteristicDescending,
        ComplexRational lambda,
        const std::vector<ComplexRational>& forcingAscending) const;

    std::vector<Rational> characteristicDescending(const SolverInput& input) const;
    QString homogeneousSolution(const SolverInput& input) const;

    ComplexRational evaluateDerivative(
        const std::vector<Rational>& descending,
        int derivativeOrder,
        ComplexRational z) const;

    int rootMultiplicity(
        const std::vector<Rational>& descending,
        int order,
        ComplexRational z) const;

    std::vector<ComplexRational> solveLinearSystem(
        std::vector<std::vector<ComplexRational>> matrix,
        std::vector<ComplexRational> rhs) const;

    static Rational binomial(int n, int k);

    static QString formatReal(const Rational& value);
    static QString formatComplex(const ComplexRational& value);
    static QString formatPolynomialReal(const std::vector<Rational>& coeffsAscending);
    static QString formatPolynomialComplex(const std::vector<ComplexRational>& coeffsAscending);
    static QString formatPolynomialFactor(const std::vector<ComplexRational>& coeffsAscending);
    static QString formatExponentialFactor(const Rational& lambda);
    static QString formatTrigArgument(const Rational& frequency);
    static QString formatTrigTerm(
        const std::vector<Rational>& coeffsAscending,
        const QString& functionName,
        const Rational& frequency);
    static std::vector<Rational> shiftPolynomial(
        const std::vector<Rational>& coeffsAscending,
        int shift);
    static std::vector<ComplexRational> shiftPolynomial(
        const std::vector<ComplexRational>& coeffsAscending,
        int shift);
    static bool isZeroPolynomial(const std::vector<Rational>& coeffsAscending);
    static int nonZeroTermCount(const std::vector<ComplexRational>& coeffsAscending);
    static QString formatPower(const QString& base, int power);
    static QString htmlEscape(const QString& value);
};
