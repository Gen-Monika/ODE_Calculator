#pragma once

#include "OdeSolver.h"

#include <QString>

struct EquationParseResult {
    bool ok = false;
    QString error;
    SolverInput input;
    QString normalizedText;
    QString latex;
};

class ExpressionParser {
public:
    EquationParseResult parseEquation(const QString& rawText) const;

private:
    struct Term {
        Rational coefficient = Rational(0);
        int xPower = 0;
    };

    using Polynomial = std::vector<Rational>;

    bool parseLeftSide(const QString& text, SolverInput& input, QString& error) const;
    bool parseRightSide(const QString& text, SolverInput& input, QString& error) const;

    bool parsePolynomialExp(const QString& text, SolverInput& input, QString& error) const;
    bool parseExpTrig(const QString& text, SolverInput& input, QString& error) const;

    bool parsePolynomial(const QString& text, Polynomial& polynomial, QString& error) const;
    bool parseTerm(const QString& text, Term& term, QString& error) const;
    bool parseRationalToken(const QString& text, Rational& value, QString& error) const;
    bool parseLinearArgument(const QString& text, Rational& coefficient, QString& error) const;
    bool extractExponentialFactor(QString& expr, Rational& lambda, QString& error) const;

    QString normalize(QString text) const;
    QString stripOuterParens(QString text) const;
    QString removeSpaces(QString text) const;
    QString toLatex(const SolverInput& input) const;
    QString leftSideLatex(const SolverInput& input) const;
    QString coefficientVariableLatex(
        const Rational& coefficient,
        const QString& variable,
        bool first) const;
    QString linearArgumentLatex(const Rational& coefficient) const;
    QString polynomialLatex(const Polynomial& polynomial) const;
    QString rationalLatex(const Rational& value) const;
};
