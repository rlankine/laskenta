
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

#include <iostream>
#include <string>

//**********************************************************************************************************************

#define STACK_LIMIT 10000

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
    friend Expression sign(Expression const&);
    friend Expression sqrt(Expression const&);
    friend Expression cbrt(Expression const&);
    friend Expression exp(Expression const&);
    friend Expression log(Expression const&);
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

    friend Expression operator+(Expression const&);
    friend Expression operator-(Expression const&);
    friend Expression operator+(Expression const&, Expression const&);
    friend Expression operator-(Expression const&, Expression const&);
    friend Expression operator*(Expression const&, Expression const&);
    friend Expression operator/(Expression const&, Expression const&);
    friend Expression pow(Expression const&, Expression const&);

    friend std::ostream& operator<<(std::ostream&, Expression const&);

    enum class Attribute
    {
        DEFINED, NONZERO, POSITIVE, NEGATIVE, NONPOSITIVE, NONNEGATIVE,
        UNITRANGE, ANTIUNITRANGE, OPENUNITRANGE, ANTIOPENUNITRANGE,
        CONTINUOUS, INCREASING, DECREASING, NONINCREASING, NONDECREASING,
        BOUNDEDABOVE, BOUNDEDBELOW
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

namespace std
{
inline double sign(double x)
{
    return double(x > 0) - double(x < 0);
}
} // end-of-namespace std

//**********************************************************************************************************************
