#pragma once

#include <QString>

class Rational {
public:
    Rational(long long numerator = 0, long long denominator = 1);

    static bool parse(const QString& text, Rational& out, QString* error = nullptr);

    long long numerator() const { return numerator_; }
    long long denominator() const { return denominator_; }

    bool isZero() const { return numerator_ == 0; }
    bool isOne() const { return numerator_ == denominator_; }
    bool isMinusOne() const { return numerator_ == -denominator_; }
    bool isInteger() const { return denominator_ == 1; }

    double toDouble() const;
    QString toString() const;
    QString toHtml() const;

    Rational operator-() const;

    friend Rational operator+(const Rational& lhs, const Rational& rhs);
    friend Rational operator-(const Rational& lhs, const Rational& rhs);
    friend Rational operator*(const Rational& lhs, const Rational& rhs);
    friend Rational operator/(const Rational& lhs, const Rational& rhs);

    friend bool operator==(const Rational& lhs, const Rational& rhs);
    friend bool operator!=(const Rational& lhs, const Rational& rhs);
    friend bool operator<(const Rational& lhs, const Rational& rhs);
    friend bool operator>(const Rational& lhs, const Rational& rhs);
    friend bool operator<=(const Rational& lhs, const Rational& rhs);
    friend bool operator>=(const Rational& lhs, const Rational& rhs);

private:
    long long numerator_ = 0;
    long long denominator_ = 1;
    void normalize();
};

class ComplexRational {
public:
    ComplexRational(Rational real = Rational(), Rational imag = Rational());

    const Rational& real() const { return real_; }
    const Rational& imag() const { return imag_; }

    bool isZero() const { return real_.isZero() && imag_.isZero(); }
    bool isReal() const { return imag_.isZero(); }
    double magnitudeSquared() const;

    QString toString() const;
    QString toHtml() const;

    ComplexRational operator-() const;

    friend ComplexRational operator+(const ComplexRational& lhs, const ComplexRational& rhs);
    friend ComplexRational operator-(const ComplexRational& lhs, const ComplexRational& rhs);
    friend ComplexRational operator*(const ComplexRational& lhs, const ComplexRational& rhs);
    friend ComplexRational operator/(const ComplexRational& lhs, const ComplexRational& rhs);

    friend bool operator==(const ComplexRational& lhs, const ComplexRational& rhs);
    friend bool operator!=(const ComplexRational& lhs, const ComplexRational& rhs);

private:
    Rational real_;
    Rational imag_;
};

Rational absValue(const Rational& value);
ComplexRational pow(const ComplexRational& base, int exponent);
bool isPerfectSquare(const Rational& value, Rational& root);
