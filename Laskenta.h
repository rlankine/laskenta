
/*
MIT License

Copyright (c) 2020 Risto Lankinen

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <string>

/***********************************************************************************************************************
*** Variable
***********************************************************************************************************************/

struct Variable final
{
    Variable(char const* = nullptr, double = 0);
    Variable(Variable const&) noexcept;
    ~Variable() noexcept;

    Variable& operator=(double);
    explicit operator double() const noexcept;
    double operator()() const noexcept;

    std::string Name() const;
    void Name(std::string const&);

    struct data;

    data const* id() const;

private:
    mutable data const* pData;
};

/***********************************************************************************************************************
*** Expression
***********************************************************************************************************************/

struct Expression final
{
    Expression();
    Expression(Expression const&);
    Expression(Variable const&);
    Expression(double);
    Expression(int);
    ~Expression();

    Expression& operator=(Expression const&) noexcept;

    friend Expression abs(Expression const&);
    friend Expression sqrt(Expression const&);
    friend Expression cbrt(Expression const&);
    friend Expression exp(Expression const&);
    friend Expression expm1(Expression const&);
    friend Expression log(Expression const&);
    friend Expression log1p(Expression const&);
    friend Expression sin(Expression const&);
    friend Expression cos(Expression const&);
    friend Expression tan(Expression const&);
    friend Expression asin(Expression const&);
    friend Expression acos(Expression const&);
    friend Expression atan(Expression const&);
    friend Expression sinh(Expression const&);
    friend Expression cosh(Expression const&);
    friend Expression tanh(Expression const&);
    friend Expression asinh(Expression const&);
    friend Expression acosh(Expression const&);
    friend Expression atanh(Expression const&);
    friend Expression erf(Expression const&);

    friend Expression sgn(Expression const&);
    friend Expression Li2(Expression const&);  // Polylog2 a.k.a. dilogarithm
    friend Expression ISp(Expression const&);  // Integral of Softplus function

    friend Expression operator+(Expression const&);
    friend Expression operator-(Expression const&);
    friend Expression operator+(Expression const&, Expression const&);
    friend Expression operator-(Expression const&, Expression const&);
    friend Expression operator*(Expression const&, Expression const&);
    friend Expression operator/(Expression const&, Expression const&);
    friend Expression pow(Expression const&, Expression const&);

    friend std::ostream& operator<<(std::ostream&, Expression const&);

    double operator()() const noexcept;

    enum class Attribute
    {
        DEFINED, NONZERO, POSITIVE, NEGATIVE, NONPOSITIVE, NONNEGATIVE, UNITRANGE, ANTIUNITRANGE, OPENUNITRANGE, ANTIOPENUNITRANGE,
        CONTINUOUS, INCREASING, DECREASING, NONINCREASING, NONDECREASING, BOUNDEDABOVE, BOUNDEDBELOW
    };

    Expression Derive(Variable const&) const;
    double Evaluate() const;
    bool Guaranteed(Attribute) const;
    static void Touch();

    int32_t Depth() const noexcept;

    struct data;

private:
    Expression(data const*);
    mutable data const* pData;
};

//**********************************************************************************************************************

inline Expression operator+(Variable const& r) { return +Expression(r); }
inline Expression operator-(Variable const& r) { return -Expression(r); }

inline Expression operator+(double r, Variable const& s) { return Expression(r) + s; }
inline Expression operator-(double r, Variable const& s) { return Expression(r) - s; }
inline Expression operator*(double r, Variable const& s) { return Expression(r) * s; }
inline Expression operator/(double r, Variable const& s) { return Expression(r) / s; }
inline Expression pow(double r, Variable const& s) { return pow(Expression(r), s); }

inline Expression operator+(Variable const& r, double s) { return Expression(r) + s; }
inline Expression operator-(Variable const& r, double s) { return Expression(r) - s; }
inline Expression operator*(Variable const& r, double s) { return Expression(r) * s; }
inline Expression operator/(Variable const& r, double s) { return Expression(r) / s; }
inline Expression pow(Variable const& r, double s) { return pow(Expression(r), s); }

inline Expression operator+(Variable const& r, Variable const& s) { return Expression(r) + s; }
inline Expression operator-(Variable const& r, Variable const& s) { return Expression(r) - s; }
inline Expression operator*(Variable const& r, Variable const& s) { return Expression(r) * s; }
inline Expression operator/(Variable const& r, Variable const& s) { return Expression(r) / s; }
inline Expression pow(Variable const& r, Variable const& s) { return pow(Expression(r), s); }

//**********************************************************************************************************************

inline Expression exp2(Expression const& x) { return exp(x * log(2)); }
inline Expression log2(Expression const& x) { return log(x) / log(2); }
inline Expression log10(Expression const& x) { return log(x) / log(10); }
inline Expression erfc(Expression const& x) { return 1 - erf(x); }

//**********************************************************************************************************************

inline double sgn(double x) { return double(x > 0) - double(x < 0); }

double Li2(double);  // Polylog2
double ISp(double);  // Integral of Softplus

//**********************************************************************************************************************

template <typename T> T max(T const& x, T const& y) { return (abs(x + y) + abs(x - y)) / 2; }
template <typename T> T min(T const& x, T const& y) { return (abs(x + y) - abs(x - y)) / 2; }
template <typename T> T within(T const& x, T const& a, T const& b) { return (abs(x - min(a, b)) - abs(x - max(a, b)) + a + b) / 2; }

template <typename T> T logistic(T const& x) { return 1 / (1 + exp(-x)); }
template <typename T> T softplus(T const& x) { return log(1 + exp(x)); }
template <typename T> T ReLU(T const& x) { return (x + abs(x)) / 2; }

//**********************************************************************************************************************
