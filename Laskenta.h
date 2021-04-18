
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
#include <vector>

//**********************************************************************************************************************

using Bindings = std::vector<std::pair<struct Variable, struct Expression>>;

/***********************************************************************************************************************
*** Variable
***********************************************************************************************************************/

struct Variable final
{
    Variable(double = 0);
    Variable(Variable const&) noexcept;
    ~Variable() noexcept;

    Variable& operator=(double);
    double operator()() const noexcept;
    explicit operator double() const noexcept;

    struct data;
    size_t id() const;
    std::string Name() const;
    void Name(std::string const&);

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
    friend Expression erfc(Expression const&);

    friend Expression sgn(Expression const&);
    friend Expression Li2(Expression const&);
    friend Expression Spp(Expression const&);

    friend Expression operator+(Expression const&);
    friend Expression operator-(Expression const&);
    friend Expression operator+(Expression const&, Expression const&);
    friend Expression operator-(Expression const&, Expression const&);
    friend Expression operator*(Expression const&, Expression const&);
    friend Expression operator/(Expression const&, Expression const&);
    friend Expression pow(Expression const&, Expression const&);

    friend std::ostream& operator<<(std::ostream&, Expression const&);

    double operator()() const noexcept;
    explicit operator double() const;

    enum class Attribute
    {
        DEFINED, NONZERO, POSITIVE, NEGATIVE, NONPOSITIVE, NONNEGATIVE,
        UNITRANGE, ANTIUNITRANGE, OPENUNITRANGE, ANTIOPENUNITRANGE,
        CONTINUOUS, INCREASING, DECREASING, NONINCREASING, NONDECREASING,
        BOUNDEDABOVE, BOUNDEDBELOW
    };

    friend void AtomicAssign(Bindings&);
    Expression AtomicBind(Bindings const&) const;
    Expression Bind(Variable const&, double) const;
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

//**********************************************************************************************************************

inline double sgn(double x) { return double(x > 0) - double(x < 0); }

double Li2(double);  // Polylog2
double Spp(double);  // Integral of Softplus

//**********************************************************************************************************************
