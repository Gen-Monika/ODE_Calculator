#include "ExactNumber.h"

#include <QStringList>
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <stdexcept>

namespace {
long long checkedCast(__int128 value)
{
    if (value > std::numeric_limits<long long>::max()
        || value < std::numeric_limits<long long>::min()) {
        throw std::overflow_error("精确分数运算溢出，请暂时使用较小的整数系数。");
    }
    return static_cast<long long>(value);
}

long long gcdAbs(long long a, long long b)
{
    if (a < 0) {
        a = -a;
    }
    if (b < 0) {
        b = -b;
    }
    return std::gcd(a, b);
}

long long integerSqrt(long long value)
{
    if (value < 0) {
        return -1;
    }
    long long root = static_cast<long long>(std::sqrt(static_cast<long double>(value)));
    while ((root + 1) > 0 && (root + 1) <= value / (root + 1)) {
        ++root;
    }
    while (root > 0 && root > value / root) {
        --root;
    }
    return root;
}

bool parseInteger(const QString& text, long long& value)
{
    bool ok = false;
    value = text.toLongLong(&ok);
    return ok;
}

QString normalizeInput(QString text)
{
    text = text.trimmed();
    text.replace(QChar(0x2212), "-");
    text.remove(' ');
    return text;
}
}

Rational::Rational(long long numerator, long long denominator)
    : numerator_(numerator)
    , denominator_(denominator)
{
    normalize();
}

bool Rational::parse(const QString& rawText, Rational& out, QString* error)
{
    const QString text = normalizeInput(rawText);
    if (text.isEmpty()) {
        if (error) {
            *error = "请输入一个有理数。";
        }
        return false;
    }

    if (text.contains('/')) {
        const QStringList parts = text.split('/');
        if (parts.size() != 2) {
            if (error) {
                *error = "分数格式应为 a/b。";
            }
            return false;
        }
        long long numerator = 0;
        long long denominator = 1;
        if (!parseInteger(parts[0], numerator) || !parseInteger(parts[1], denominator)) {
            if (error) {
                *error = "分数的分子和分母需要是整数。";
            }
            return false;
        }
        if (denominator == 0) {
            if (error) {
                *error = "分母不能为 0。";
            }
            return false;
        }
        out = Rational(numerator, denominator);
        return true;
    }

    if (text.contains('.')) {
        bool negative = text.startsWith('-');
        QString body = negative ? text.mid(1) : text;
        const QStringList parts = body.split('.');
        if (parts.size() != 2 || parts[0].isEmpty() && parts[1].isEmpty()) {
            if (error) {
                *error = "小数格式不正确。";
            }
            return false;
        }
        const QString integerPart = parts[0].isEmpty() ? "0" : parts[0];
        const QString decimalPart = parts[1];
        long long integerValue = 0;
        long long decimalValue = 0;
        if (!parseInteger(integerPart, integerValue)
            || (!decimalPart.isEmpty() && !parseInteger(decimalPart, decimalValue))) {
            if (error) {
                *error = "小数格式不正确。";
            }
            return false;
        }
        long long denominator = 1;
        for (int i = 0; i < decimalPart.size(); ++i) {
            denominator *= 10;
        }
        long long numerator = checkedCast(
            static_cast<__int128>(integerValue) * denominator + decimalValue);
        if (negative) {
            numerator = -numerator;
        }
        out = Rational(numerator, denominator);
        return true;
    }

    long long integer = 0;
    if (!parseInteger(text, integer)) {
        if (error) {
            *error = "请输入整数、小数或 a/b 形式的分数。";
        }
        return false;
    }
    out = Rational(integer, 1);
    return true;
}

double Rational::toDouble() const
{
    return static_cast<double>(numerator_) / static_cast<double>(denominator_);
}

QString Rational::toString() const
{
    if (denominator_ == 1) {
        return QString::number(numerator_);
    }
    return QString::number(numerator_) + "/" + QString::number(denominator_);
}

QString Rational::toHtml() const
{
    return toString().toHtmlEscaped();
}

Rational Rational::operator-() const
{
    return Rational(-numerator_, denominator_);
}

void Rational::normalize()
{
    if (denominator_ == 0) {
        throw std::runtime_error("分母不能为 0。");
    }
    if (denominator_ < 0) {
        numerator_ = -numerator_;
        denominator_ = -denominator_;
    }
    if (numerator_ == 0) {
        denominator_ = 1;
        return;
    }
    const long long divisor = gcdAbs(numerator_, denominator_);
    numerator_ /= divisor;
    denominator_ /= divisor;
}

Rational operator+(const Rational& lhs, const Rational& rhs)
{
    const long long g = gcdAbs(lhs.denominator_, rhs.denominator_);
    const __int128 lcmLeft = lhs.denominator_ / g;
    const __int128 lcmRight = rhs.denominator_ / g;
    const __int128 numerator = static_cast<__int128>(lhs.numerator_) * lcmRight
        + static_cast<__int128>(rhs.numerator_) * lcmLeft;
    const __int128 denominator = static_cast<__int128>(lhs.denominator_) * lcmRight;
    return Rational(checkedCast(numerator), checkedCast(denominator));
}

Rational operator-(const Rational& lhs, const Rational& rhs)
{
    return lhs + (-rhs);
}

Rational operator*(const Rational& lhs, const Rational& rhs)
{
    const long long g1 = gcdAbs(lhs.numerator_, rhs.denominator_);
    const long long g2 = gcdAbs(rhs.numerator_, lhs.denominator_);
    const __int128 numerator = static_cast<__int128>(lhs.numerator_ / g1) * (rhs.numerator_ / g2);
    const __int128 denominator = static_cast<__int128>(lhs.denominator_ / g2) * (rhs.denominator_ / g1);
    return Rational(checkedCast(numerator), checkedCast(denominator));
}

Rational operator/(const Rational& lhs, const Rational& rhs)
{
    if (rhs.numerator_ == 0) {
        throw std::runtime_error("除数不能为 0。");
    }
    return lhs * Rational(rhs.denominator_, rhs.numerator_);
}

bool operator==(const Rational& lhs, const Rational& rhs)
{
    return lhs.numerator_ == rhs.numerator_ && lhs.denominator_ == rhs.denominator_;
}

bool operator!=(const Rational& lhs, const Rational& rhs)
{
    return !(lhs == rhs);
}

bool operator<(const Rational& lhs, const Rational& rhs)
{
    return static_cast<__int128>(lhs.numerator_) * rhs.denominator_
        < static_cast<__int128>(rhs.numerator_) * lhs.denominator_;
}

bool operator>(const Rational& lhs, const Rational& rhs)
{
    return rhs < lhs;
}

bool operator<=(const Rational& lhs, const Rational& rhs)
{
    return !(rhs < lhs);
}

bool operator>=(const Rational& lhs, const Rational& rhs)
{
    return !(lhs < rhs);
}

ComplexRational::ComplexRational(Rational real, Rational imag)
    : real_(real)
    , imag_(imag)
{
}

double ComplexRational::magnitudeSquared() const
{
    return real_.toDouble() * real_.toDouble() + imag_.toDouble() * imag_.toDouble();
}

QString ComplexRational::toString() const
{
    if (imag_.isZero()) {
        return real_.toString();
    }
    if (real_.isZero()) {
        if (imag_.isOne()) {
            return "i";
        }
        if (imag_.isMinusOne()) {
            return "-i";
        }
        return imag_.toString() + "i";
    }
    return "(" + real_.toString() + (imag_ < Rational(0) ? " - " : " + ")
        + absValue(imag_).toString() + "i)";
}

QString ComplexRational::toHtml() const
{
    return toString().toHtmlEscaped();
}

ComplexRational ComplexRational::operator-() const
{
    return ComplexRational(-real_, -imag_);
}

ComplexRational operator+(const ComplexRational& lhs, const ComplexRational& rhs)
{
    return ComplexRational(lhs.real_ + rhs.real_, lhs.imag_ + rhs.imag_);
}

ComplexRational operator-(const ComplexRational& lhs, const ComplexRational& rhs)
{
    return lhs + (-rhs);
}

ComplexRational operator*(const ComplexRational& lhs, const ComplexRational& rhs)
{
    return ComplexRational(
        lhs.real_ * rhs.real_ - lhs.imag_ * rhs.imag_,
        lhs.real_ * rhs.imag_ + lhs.imag_ * rhs.real_);
}

ComplexRational operator/(const ComplexRational& lhs, const ComplexRational& rhs)
{
    const Rational denominator = rhs.real_ * rhs.real_ + rhs.imag_ * rhs.imag_;
    if (denominator.isZero()) {
        throw std::runtime_error("复数除数不能为 0。");
    }
    return ComplexRational(
        (lhs.real_ * rhs.real_ + lhs.imag_ * rhs.imag_) / denominator,
        (lhs.imag_ * rhs.real_ - lhs.real_ * rhs.imag_) / denominator);
}

bool operator==(const ComplexRational& lhs, const ComplexRational& rhs)
{
    return lhs.real_ == rhs.real_ && lhs.imag_ == rhs.imag_;
}

bool operator!=(const ComplexRational& lhs, const ComplexRational& rhs)
{
    return !(lhs == rhs);
}

Rational absValue(const Rational& value)
{
    return value < Rational(0) ? -value : value;
}

ComplexRational pow(const ComplexRational& base, int exponent)
{
    ComplexRational result(Rational(1), Rational(0));
    for (int i = 0; i < exponent; ++i) {
        result = result * base;
    }
    return result;
}

bool isPerfectSquare(const Rational& value, Rational& root)
{
    if (value < Rational(0)) {
        return false;
    }
    const long long numeratorRoot = integerSqrt(value.numerator());
    const long long denominatorRoot = integerSqrt(value.denominator());
    if (numeratorRoot >= 0 && denominatorRoot >= 0
        && numeratorRoot * numeratorRoot == value.numerator()
        && denominatorRoot * denominatorRoot == value.denominator()) {
        root = Rational(numeratorRoot, denominatorRoot);
        return true;
    }
    return false;
}
