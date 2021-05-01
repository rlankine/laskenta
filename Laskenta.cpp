
/*
MIT License

Copyright (c) 2021 Risto Lankinen

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

#include "Laskenta.h"
// #define VERBOSE
#include "Tools.h"

#include <map>
#include <unordered_map>

//**********************************************************************************************************************

auto const STACK_LIMIT = 10000;

using Attr = Expression::Attribute;
using Expr = Expression::data;

/***********************************************************************************************************************
*** Expression::data
***********************************************************************************************************************/

struct Expression::data : public Shared
{
    // Primitives

    static Expr const* constant(double);
    static Expr const* variable(Variable const&);

    // Functions

    virtual Expr const* abs() const;
    virtual Expr const* sgn() const;
    virtual Expr const* sqrt() const { return function(NodeType::SQRT); }
    virtual Expr const* cbrt() const { return function(NodeType::CBRT); }
    virtual Expr const* exp() const { return function(NodeType::EXP); }
    virtual Expr const* expm1() const { return function(NodeType::EXPM1); }
    virtual Expr const* log() const { return function(NodeType::LOG); }
    virtual Expr const* log1p() const { return function(NodeType::LOG1P); }
    virtual Expr const* sin() const { return function(NodeType::SIN); }
    virtual Expr const* cos() const { return function(NodeType::COS); }
    virtual Expr const* tan() const { return function(NodeType::TAN); }
    virtual Expr const* sec() const { return function(NodeType::SEC); }
    virtual Expr const* asin() const { return function(NodeType::ASIN); }
    virtual Expr const* acos() const { return function(NodeType::ACOS); }
    virtual Expr const* atan() const { return function(NodeType::ATAN); }
    virtual Expr const* sinh() const { return function(NodeType::SINH); }
    virtual Expr const* cosh() const { return function(NodeType::COSH); }
    virtual Expr const* tanh() const { return function(NodeType::TANH); }
    virtual Expr const* sech() const { return function(NodeType::SECH); }
    virtual Expr const* asinh() const { return function(NodeType::ASINH); }
    virtual Expr const* acosh() const { return function(NodeType::ACOSH); }
    virtual Expr const* atanh() const { return function(NodeType::ATANH); }
    virtual Expr const* erf() const { return function(NodeType::ERF); }
    virtual Expr const* erfc() const { return function(NodeType::ERFC); }

    virtual Expr const* invert() const { return function(NodeType::INVERT); }
    virtual Expr const* negate() const { return function(NodeType::NEGATE); }
    virtual Expr const* softpp() const { return function(NodeType::SOFTPP); }
    virtual Expr const* spence() const { return function(NodeType::SPENCE); }
    virtual Expr const* square() const { return function(NodeType::SQUARE); }
    virtual Expr const* xconic() const { return function(NodeType::XCONIC); }
    virtual Expr const* yconic() const { return function(NodeType::YCONIC); }
    virtual Expr const* zconic() const { return function(NodeType::ZCONIC); }

    // Operators

    virtual Expr const* add(Expr const*) const;
    virtual Expr const* commutative_add(Expr const*) const;
    virtual Expr const* mul(Expr const*) const;
    virtual Expr const* commutative_mul(Expr const*) const;
    virtual Expr const* pow(Expr const*) const;

    // Evaluation and derivation

    virtual Expr const* bind(std::vector<std::pair<Variable, Expr const*>> const&) const = 0;

    Expr const* derive(Variable const& r) const { return Clone(cachedNode ? cachedNode : cachedNode = derivative(r)); }
    double evaluate() const { if (cleanLevel != dirtyLevel) { valueCache = value(); cleanLevel = dirtyLevel; } return valueCache; }

    // Analysis tools

    virtual bool guaranteed(Attr) const = 0;

    enum class NodeType
    {
        ABS, SGN, SQRT, CBRT, EXP, EXPM1, LOG, LOG1P, SIN, COS, TAN, SEC, ASIN, ACOS, ATAN, SINH, COSH, TANH, SECH, ASINH, ACOSH, ATANH, ERF, ERFC,
        INVERT, NEGATE, SOFTPP, SPENCE, SQUARE, XCONIC, YCONIC, ZCONIC, CONSTANT, VARIABLE, ADD, MUL, POW
    };

    virtual bool is(NodeType) const = 0;
    virtual bool is(NodeType, Expr const*) const = 0;

    virtual bool easyInvert() const { return false; }
    virtual bool easyNegate() const { return false; }

    // Cache management

    mutable std::map<NodeType, Expr const*> functionNode;
    mutable std::map<Expr const*, Expr const*> addNode;
    mutable std::map<Expr const*, Expr const*> mulNode;
    mutable std::map<Expr const*, Expr const*> powNode;

    // Lazy evaluation, etc.

    int32_t const depth;
    static size_t dirtyLevel;
    mutable Expr const* cachedNode;
    virtual void purge() const { if (cachedNode) { Erase(cachedNode); cachedNode = nullptr; } }

    operator Expr const* () const;

    virtual void print(std::ostream& r) const = 0;

protected:
    explicit data(int32_t n) : depth(n), cachedNode(nullptr), cleanLevel(0), valueCache(0) { }
    virtual ~data() { }

    static std::unordered_map<double, Expr const*> constantNode;
    static std::unordered_map<size_t, Expr const*> variableNode;

private:
    mutable size_t cleanLevel;
    mutable double valueCache;

    virtual Expr const* derivative(Variable const&) const = 0;
    virtual double value() const = 0;

    Expr const* function(NodeType n) const;

    void* operator new(size_t n) { return ::operator new(n); }
    void* operator new[](size_t) = delete;
};

//----------------------------------------------------------------------------------------------------------------------

inline Expression::data::operator Expr const* () const
{
    return this;
}

//----------------------------------------------------------------------------------------------------------------------

size_t Expression::data::dirtyLevel = 1LL;
std::unordered_map<double, Expr const*> Expression::data::constantNode;
std::unordered_map<size_t, Expr const*> Expression::data::variableNode;

/***********************************************************************************************************************
*** FunctionNode
***********************************************************************************************************************/

struct FunctionNode : public Expr
{
    FunctionNode(Expr const* p, NodeType n) : Expr(p->depth + 1), f_x(p), fn(n)
    {
        assert(f_x->functionNode.find(fn) == f_x->functionNode.end());
        f_x->functionNode[fn] = this;
    }

    bool is(NodeType t) const override final { return t == fn; }
    bool is(NodeType t, Expr const* p) const override final { return t == fn && p == f_x; }

    void purge() const override final { if (cachedNode) { Expr::purge(); f_x->purge(); } }

protected:
    virtual ~FunctionNode()
    {
        assert(f_x->functionNode.find(fn) != f_x->functionNode.end() && f_x->functionNode[fn] == this);
        f_x->functionNode.erase(fn);
        Erase(f_x);
    }

    Expr const* const f_x;

private:
    NodeType const fn;
};

/***********************************************************************************************************************
*** OperatorNode
***********************************************************************************************************************/

struct OperatorNode : public Expr
{
    OperatorNode(Expr const* p, Expr const* q) : Expr(std::max(p->depth, q->depth) + 1), f_x(p), g_x(q) { }

    bool is(NodeType, Expr const*) const override final { return false; }

    void purge() const override final { if (cachedNode) { Expr::purge(); f_x->purge(); g_x->purge(); } }

protected:
    virtual ~OperatorNode()
    {
        Erase(f_x);
        Erase(g_x);
    }

    Expr const* const f_x;
    Expr const* const g_x;
};

/***********************************************************************************************************************
*** Nan
***********************************************************************************************************************/

struct Nan final : public Expr
{
    static Nan const instance;

    Expr const* abs() const override final { return Clone(this); }
    Expr const* sgn() const override final { return Clone(this); }
    Expr const* sqrt() const override final { return Clone(this); }
    Expr const* cbrt() const override final { return Clone(this); }
    Expr const* exp() const override final { return Clone(this); }
    Expr const* expm1() const override final { return Clone(this); }
    Expr const* log() const override final { return Clone(this); }
    Expr const* log1p() const override final { return Clone(this); }
    Expr const* sin() const override final { return Clone(this); }
    Expr const* cos() const override final { return Clone(this); }
    Expr const* tan() const override final { return Clone(this); }
    Expr const* sec() const override final { return Clone(this); }
    Expr const* asin() const override final { return Clone(this); }
    Expr const* acos() const override final { return Clone(this); }
    Expr const* atan() const override final { return Clone(this); }
    Expr const* sinh() const override final { return Clone(this); }
    Expr const* cosh() const override final { return Clone(this); }
    Expr const* tanh() const override final { return Clone(this); }
    Expr const* sech() const override final { return Clone(this); }
    Expr const* asinh() const override final { return Clone(this); }
    Expr const* acosh() const override final { return Clone(this); }
    Expr const* atanh() const override final { return Clone(this); }
    Expr const* erf() const override final { return Clone(this); }
    Expr const* erfc() const override final { return Clone(this); }

    Expr const* invert() const override final { return Clone(this); }
    Expr const* negate() const override final { return Clone(this); }
    Expr const* softpp() const override final { return Clone(this); }
    Expr const* spence() const override final { return Clone(this); }
    Expr const* square() const override final { return Clone(this); }
    Expr const* xconic() const override final { return Clone(this); }
    Expr const* yconic() const override final { return Clone(this); }
    Expr const* zconic() const override final { return Clone(this); }

    Expr const* add(Expr const*) const override final { return Clone(this); }
    Expr const* commutative_add(Expr const*) const override final { return Clone(this); }
    Expr const* mul(Expr const*) const override final { return Clone(this); }
    Expr const* commutative_mul(Expr const*) const override final { return Clone(this); }
    Expr const* pow(Expr const*) const override final { return Clone(this); }

    Expr const* bind(std::vector<std::pair<Variable, Expr const*>> const&) const override final { return Clone(this); }

    bool guaranteed(Attr) const override final { return false; }

    bool is(NodeType) const override final { return false; }
    bool is(NodeType, Expr const*) const override final { return false; }

    Expr const* derivative(Variable const&) const override final { return Clone(this); }
    double value() const override final { return nan(__FUNCTION__); }

    void print(std::ostream& out) const override final { out << "nan"; }

private:
    Nan() : Expr(0) { }
};

//----------------------------------------------------------------------------------------------------------------------

Nan const Nan::instance;

/***********************************************************************************************************************
*** ConstantNode
***********************************************************************************************************************/

struct ConstantNode final : public Expr, private ObjectGuard<ConstantNode>
{
    explicit ConstantNode(double d) : Expr(0), n(d)
    {
        assert(constantNode.find(n) == constantNode.end());
        constantNode[n] = this;
    }

    Expr const* abs() const override final { return constant(std::abs(n)); }
    Expr const* sgn() const override final { return constant(double(n > 0) - (n < 0)); }
    Expr const* sqrt() const override final { return constant(std::sqrt(n)); }
    Expr const* cbrt() const override final { return constant(std::cbrt(n)); }
    Expr const* exp() const override final { return constant(std::exp(n)); }
    Expr const* log() const override final { return constant(std::log(n)); }
    Expr const* sin() const override final { return constant(std::sin(n)); }
    Expr const* cos() const override final { return constant(std::cos(n)); }
    Expr const* tan() const override final { return constant(std::tan(n)); }
    Expr const* sec() const override final { return constant(1 / std::cos(n)); }
    Expr const* asin() const override final { return constant(std::asin(n)); }
    Expr const* acos() const override final { return constant(std::acos(n)); }
    Expr const* atan() const override final { return constant(std::atan(n)); }
    Expr const* sinh() const override final { return constant(std::sinh(n)); }
    Expr const* cosh() const override final { return constant(std::cosh(n)); }
    Expr const* tanh() const override final { return constant(std::tanh(n)); }
    Expr const* sech() const override final { return constant(1 / std::cosh(n)); }
    Expr const* asinh() const override final { return constant(std::asinh(n)); }
    Expr const* acosh() const override final { return constant(std::acosh(n)); }
    Expr const* atanh() const override final { return constant(std::atanh(n)); }
    Expr const* erf() const override final { return constant(std::erf(n)); }
    Expr const* erfc() const override final { return constant(std::erfc(n)); }

    Expr const* invert() const override final { return constant(1 / n); }
    Expr const* negate() const override final { return constant(-n); }
    Expr const* softpp() const override final { return constant(::Spp(n)); }
    Expr const* spence() const override final { return constant(::Li2(n)); }
    Expr const* square() const override final { return constant(n * n); }
    Expr const* xconic() const override final { return constant(std::sqrt(n * n - 1)); }
    Expr const* yconic() const override final { return constant(std::sqrt(n * n + 1)); }
    Expr const* zconic() const override final { return constant(std::sqrt(1 - n * n)); }

    Expr const* add(Expr const*) const override final;
    Expr const* commutative_add(Expr const*) const override final;
    Expr const* mul(Expr const*) const override final;
    Expr const* commutative_mul(Expr const*) const override final;
    Expr const* pow(Expr const*) const override final;

    Expr const* bind(std::vector<std::pair<Variable, Expr const*>> const&) const override final { return Clone(this); }

    bool guaranteed(Attr) const override final;

    bool is(NodeType t) const override final { return t == NodeType::CONSTANT; }
    bool is(NodeType t, Expr const* p) const override final { return t == NodeType::CONSTANT && p == this; }

    bool easyInvert() const override final { return n != 0; }
    bool easyNegate() const override final { return true; }

    Expr const* derivative(Variable const&) const override final;
    double value() const override final { return n; }

    void print(std::ostream&) const override final;

    virtual ~ConstantNode()
    {
        assert(constantNode.find(n) != constantNode.end() && constantNode[n] == this);
        constantNode.erase(n);
    }

private:
    double const n;
};

//----------------------------------------------------------------------------------------------------------------------

Expr const* Expression::data::constant(double d)
{
    if (isnan(d)) return Clone(Nan::instance);

    auto node = constantNode.find(d);
    return node != constantNode.end() ? Clone(node->second) : new ConstantNode(d);
}

/***********************************************************************************************************************
*** VariableNode
***********************************************************************************************************************/

struct VariableNode final : public Expr, private ObjectGuard<VariableNode>
{
    explicit VariableNode(Variable const& r) : Expr(1), x(r)
    {
        assert(variableNode.find(x.id()) == variableNode.end());
        variableNode[x.id()] = this;
    }

    Expr const* bind(std::vector<std::pair<Variable, Expr const*>> const& r) const override final
    {
        for (auto& item : r)
        {
            if (item.first.id() == x.id())
            {
                return Clone(item.second);
            }
        }

        return Clone(this);
    }

    bool guaranteed(Attr) const override final;

    bool is(NodeType t) const override final { return t == NodeType::VARIABLE; }
    bool is(NodeType t, Expr const* p) const override final { return t == NodeType::VARIABLE && p == this; }

    Expr const* derivative(Variable const&) const override final;
    double value() const override final { return double(x); }

    void print(std::ostream&) const override final;

private:
    virtual ~VariableNode()
    {
        assert(variableNode.find(x.id()) != variableNode.end() && variableNode[x.id()] == this);
        variableNode.erase(x.id());
    }

    Variable const x;
};

//----------------------------------------------------------------------------------------------------------------------

Expr const* Expression::data::variable(Variable const& r)
{
    auto node = variableNode.find(r.id());
    return node != variableNode.end() ? Clone(node->second) : new VariableNode(r);
}

/***********************************************************************************************************************
*** Abs
***********************************************************************************************************************/

struct Abs final : public FunctionNode, private ObjectGuard<Abs>
{
    Abs(Expr const* p) : FunctionNode(p, NodeType::ABS) { }

    Expr const* abs() const override final { return Clone(this); }
    Expr const* sgn() const override final { auto step0 = f_x->sgn(); auto step1 = step0->abs(); Erase(step0); return step1; }
    Expr const* cbrt() const override final { auto step0 = f_x->cbrt(); auto step1 = step0->abs(); Erase(step0); return step1; }
    Expr const* cos() const override final { return f_x->cos(); }
    Expr const* sec() const override final { return f_x->sec(); }
    Expr const* asin() const override final { auto step0 = f_x->asin(); auto step1 = step0->abs(); Erase(step0); return step1; }
    Expr const* atan() const override final { auto step0 = f_x->atan(); auto step1 = step0->abs(); Erase(step0); return step1; }
    Expr const* sinh() const override final { auto step0 = f_x->sinh(); auto step1 = step0->abs(); Erase(step0); return step1; }
    Expr const* cosh() const override final { return f_x->cosh(); }
    Expr const* tanh() const override final { auto step0 = f_x->tanh(); auto step1 = step0->abs(); Erase(step0); return step1; }
    Expr const* sech() const override final { return f_x->sech(); }
    Expr const* asinh() const override final { auto step0 = f_x->asinh(); auto step1 = step0->abs(); Erase(step0); return step1; }
    Expr const* atanh() const override final { auto step0 = f_x->atanh(); auto step1 = step0->abs(); Erase(step0); return step1; }
    Expr const* erf() const override final { auto step0 = f_x->erf(); auto step1 = step0->abs(); Erase(step0); return step1; }
    Expr const* square() const override final { return f_x->square(); }
    Expr const* xconic() const override final { return f_x->xconic(); }
    Expr const* yconic() const override final { return f_x->yconic(); }
    Expr const* zconic() const override final { return f_x->zconic(); }

    Expr const* bind(std::vector<std::pair<Variable, Expr const*>> const& r) const override final { auto step0 = f_x->bind(r); auto step1 = step0->abs(); Erase(step0); return step1; }

    bool guaranteed(Attr) const override final;

    Expr const* derivative(Variable const&) const override final;
    double value() const override final { return std::abs(f_x->evaluate()); }

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** Sgn
***********************************************************************************************************************/

struct Sgn final : public FunctionNode, private ObjectGuard<Sgn>
{
    Sgn(Expr const* p) : FunctionNode(p, NodeType::SGN) { }

    Expr const* sgn() const override final { return Clone(this); }
    Expr const* cbrt() const override final { return Clone(this); }
    Expr const* square() const override final { auto step0 = f_x->square(); auto step1 = step0->sgn(); Erase(step0); return step1; }

    Expr const* bind(std::vector<std::pair<Variable, Expr const*>> const& r) const override final { auto step0 = f_x->bind(r); auto step1 = step0->sgn(); Erase(step0); return step1; }

    bool guaranteed(Attr) const override final;

    Expr const* derivative(Variable const&) const override final;
    double value() const override final { auto x = f_x->evaluate(); return double(x > 0) - (x < 0); }

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** Sqrt
***********************************************************************************************************************/

struct Sqrt final : public FunctionNode, private ObjectGuard<Sqrt>
{
    Sqrt(Expr const* p) : FunctionNode(p, NodeType::SQRT) { }

    Expr const* abs() const override final { return Clone(this); }
    Expr const* square() const override final { return Clone(f_x); }
    Expr const* pow(Expr const* p) const override final;

    Expr const* bind(std::vector<std::pair<Variable, Expr const*>> const& r) const override final { auto step0 = f_x->bind(r); auto step1 = step0->sqrt(); Erase(step0); return step1; }

    bool guaranteed(Attr) const override final;

    Expr const* derivative(Variable const&) const override final;
    double value() const override final { return std::sqrt(f_x->evaluate()); }

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** Cbrt
***********************************************************************************************************************/

struct Cbrt final : public FunctionNode, private ObjectGuard<Cbrt>
{
    Cbrt(Expr const* p) : FunctionNode(p, NodeType::CBRT) { }

    Expr const* sgn() const override final { return f_x->sgn(); }
    Expr const* pow(Expr const*) const override final;

    Expr const* bind(std::vector<std::pair<Variable, Expr const*>> const& r) const override final { auto step0 = f_x->bind(r); auto step1 = step0->cbrt(); Erase(step0); return step1; }

    bool guaranteed(Attr) const override final;

    Expr const* derivative(Variable const&) const override final;
    double value() const override final { return std::cbrt(f_x->evaluate()); }

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** Exp
***********************************************************************************************************************/

struct Exp final : public FunctionNode, private ObjectGuard<Exp>
{
    Exp(Expr const* p) : FunctionNode(p, NodeType::EXP) { }

    Expr const* abs() const override final { return Clone(this); }
    Expr const* sgn() const override final { return constant(1); }
    Expr const* log() const override final { return Clone(f_x); }
    Expr const* pow(Expr const* p) const override final { auto step0 = f_x->mul(p); auto step1 = step0->exp();  Erase(step0); return step1; }

    Expr const* bind(std::vector<std::pair<Variable, Expr const*>> const& r) const override final { auto step0 = f_x->bind(r); auto step1 = step0->exp(); Erase(step0); return step1; }

    bool guaranteed(Attr) const override final;

    Expr const* derivative(Variable const&) const override final;
    double value() const override final { return std::exp(f_x->evaluate()); }

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** ExpM1
***********************************************************************************************************************/

struct ExpM1 final : public FunctionNode, private ObjectGuard<ExpM1>
{
    ExpM1(Expr const* p) : FunctionNode(p, NodeType::EXPM1) { }

    Expr const* bind(std::vector<std::pair<Variable, Expr const*>> const& r) const override final { auto step0 = f_x->bind(r); auto step1 = step0->expm1(); Erase(step0); return step1; }

    bool guaranteed(Attr) const override final;

    Expr const* derivative(Variable const&) const override final;
    double value() const override final { return std::expm1(f_x->evaluate()); }

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** Log
***********************************************************************************************************************/

struct Log final : public FunctionNode, private ObjectGuard<Log>
{
    Log(Expr const* p) : FunctionNode(p, NodeType::LOG) { }

    Expr const* exp() const override final { return Clone(f_x); }

    Expr const* bind(std::vector<std::pair<Variable, Expr const*>> const& r) const override final { auto step0 = f_x->bind(r); auto step1 = step0->log(); Erase(step0); return step1; }

    bool guaranteed(Attr) const override final;

    Expr const* derivative(Variable const&) const override final;
    double value() const override final { return std::log(f_x->evaluate()); }

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** Log1P
***********************************************************************************************************************/

struct Log1P final : public FunctionNode, private ObjectGuard<Log1P>
{
    Log1P(Expr const* p) : FunctionNode(p, NodeType::LOG1P) { }

    Expr const* bind(std::vector<std::pair<Variable, Expr const*>> const& r) const override final { auto step0 = f_x->bind(r); auto step1 = step0->log1p(); Erase(step0); return step1; }

    bool guaranteed(Attr) const override final;

    Expr const* derivative(Variable const&) const override final;
    double value() const override final { return std::log1p(f_x->evaluate()); }

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** Sin
***********************************************************************************************************************/

struct Sin final : public FunctionNode, private ObjectGuard<Sin>
{
    Sin(Expr const* p) : FunctionNode(p, NodeType::SIN) { }

    Expr const* zconic() const override final { auto step0 = f_x->cos(); auto step1 = step0->abs(); Erase(step0); return step1; }

    Expr const* bind(std::vector<std::pair<Variable, Expr const*>> const& r) const override final { auto step0 = f_x->bind(r); auto step1 = step0->sin(); Erase(step0); return step1; }

    bool guaranteed(Attr) const override final;

    Expr const* derivative(Variable const&) const override final;
    double value() const override final { return std::sin(f_x->evaluate()); }

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** Cos
***********************************************************************************************************************/

struct Cos final : public FunctionNode, private ObjectGuard<Cos>
{
    Cos(Expr const* p) : FunctionNode(p, NodeType::COS) { }

    Expr const* invert() const override final { return f_x->sec(); }
    Expr const* zconic() const override final { auto step0 = f_x->sin(); auto step1 = step0->abs(); Erase(step0); return step1; }

    Expr const* bind(std::vector<std::pair<Variable, Expr const*>> const& r) const override final { auto step0 = f_x->bind(r); auto step1 = step0->cos(); Erase(step0); return step1; }

    bool guaranteed(Attr) const override final;

    Expr const* derivative(Variable const&) const override final;
    double value() const override final { return std::cos(f_x->evaluate()); }

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** Tan
***********************************************************************************************************************/

struct Tan final : public FunctionNode, private ObjectGuard<Tan>
{
    Tan(Expr const* p) : FunctionNode(p, NodeType::TAN) { }

    Expr const* bind(std::vector<std::pair<Variable, Expr const*>> const& r) const override final { auto step0 = f_x->bind(r); auto step1 = step0->tan(); Erase(step0); return step1; }

    bool guaranteed(Attr) const override final;

    Expr const* derivative(Variable const&) const override final;
    double value() const override final { return std::tan(f_x->evaluate()); }

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** Sec
***********************************************************************************************************************/

struct Sec final : public FunctionNode, private ObjectGuard<Sec>
{
    Sec(Expr const* p) : FunctionNode(p, NodeType::SEC) { }

    Expr const* invert() const override final { return f_x->cos(); }

    Expr const* bind(std::vector<std::pair<Variable, Expr const*>> const& r) const override final { auto step0 = f_x->bind(r); auto step1 = step0->sec(); Erase(step0); return step1; }

    bool guaranteed(Attr) const override final;

    Expr const* derivative(Variable const&) const override final;
    double value() const override final { return 1 / std::cos(f_x->evaluate()); }

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** ASin
***********************************************************************************************************************/

struct ASin final : public FunctionNode, private ObjectGuard<ASin>
{
    ASin(Expr const* p) : FunctionNode(p, NodeType::ASIN) { }

    Expr const* sgn() const override final { return f_x->sgn(); }
    Expr const* sin() const override final { return Clone(f_x); }
    Expr const* cos() const override final { return f_x->zconic(); }
    Expr const* sec() const override final { auto step0 = f_x->zconic(); auto step1 = step0->invert(); Erase(step0); return step1; }

    Expr const* bind(std::vector<std::pair<Variable, Expr const*>> const& r) const override final { auto step0 = f_x->bind(r); auto step1 = step0->asin(); Erase(step0); return step1; }

    bool guaranteed(Attr) const override final;

    Expr const* derivative(Variable const&) const override final;
    double value() const override final { return std::asin(f_x->evaluate()); }

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** ACos
***********************************************************************************************************************/

struct ACos final : public FunctionNode, private ObjectGuard<ACos>
{
    ACos(Expr const* p) : FunctionNode(p, NodeType::ACOS) { }

    Expr const* abs() const override final { return Clone(this); }
    Expr const* sin() const override final { return f_x->zconic(); }
    Expr const* cos() const override final { return Clone(f_x); }
    Expr const* sec() const override final { return f_x->invert(); }

    Expr const* bind(std::vector<std::pair<Variable, Expr const*>> const& r) const override final { auto step0 = f_x->bind(r); auto step1 = step0->acos(); Erase(step0); return step1; }

    bool guaranteed(Attr) const override final;

    Expr const* derivative(Variable const&) const override final;
    double value() const override final { return std::acos(f_x->evaluate()); }

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** ATan
***********************************************************************************************************************/

struct ATan final : public FunctionNode, private ObjectGuard<ATan>
{
    ATan(Expr const* p) : FunctionNode(p, NodeType::ATAN) { }

    Expr const* sgn() const override final { return f_x->sgn(); }
    Expr const* cos() const override final { auto step0 = f_x->yconic(); auto step1 = step0->invert(); Erase(step0); return step1; }
    Expr const* tan() const override final { return Clone(f_x); }
    Expr const* sec() const override final { return f_x->yconic(); }

    Expr const* bind(std::vector<std::pair<Variable, Expr const*>> const& r) const override final { auto step0 = f_x->bind(r); auto step1 = step0->atan(); Erase(step0); return step1; }

    bool guaranteed(Attr) const override final;

    Expr const* derivative(Variable const&) const override final;
    double value() const override final { return std::atan(f_x->evaluate()); }

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** SinH
***********************************************************************************************************************/

struct SinH final : public FunctionNode, private ObjectGuard<SinH>
{
    SinH(Expr const* p) : FunctionNode(p, NodeType::SINH) { }

    Expr const* sgn() const override final { return f_x->sgn(); }
    Expr const* asinh() const override final { return Clone(f_x); }
    Expr const* yconic() const override final { return f_x->cosh(); }

    Expr const* bind(std::vector<std::pair<Variable, Expr const*>> const& r) const override final { auto step0 = f_x->bind(r); auto step1 = step0->sinh(); Erase(step0); return step1; }

    bool guaranteed(Attr) const override final;

    Expr const* derivative(Variable const&) const override final;
    double value() const override final { return std::sinh(f_x->evaluate()); }

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** CosH
***********************************************************************************************************************/

struct CosH final : public FunctionNode, private ObjectGuard<CosH>
{
    CosH(Expr const* p) : FunctionNode(p, NodeType::COSH) { }

    Expr const* abs() const override final { return Clone(this); }
    Expr const* sgn() const override final { return constant(1); }
    Expr const* acosh() const override final { return f_x->abs(); }
    Expr const* invert() const override final { return f_x->sech(); }
    Expr const* xconic() const override final { auto step0 = f_x->sinh(); auto step1 = step0->abs(); Erase(step0); return step1; }

    Expr const* bind(std::vector<std::pair<Variable, Expr const*>> const& r) const override final { auto step0 = f_x->bind(r); auto step1 = step0->cosh(); Erase(step0); return step1; }

    bool guaranteed(Attr) const override final;

    Expr const* derivative(Variable const&) const override final;
    double value() const override final { return std::cosh(f_x->evaluate()); }

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** TanH
***********************************************************************************************************************/

struct TanH final : public FunctionNode, private ObjectGuard<TanH>
{
    TanH(Expr const* p) : FunctionNode(p, NodeType::TANH) { ; }

    Expr const* sgn() const override final { return f_x->sgn(); }
    Expr const* atanh() const override final { return Clone(f_x); }

    Expr const* bind(std::vector<std::pair<Variable, Expr const*>> const& r) const override final { auto step0 = f_x->bind(r); auto step1 = step0->tanh(); Erase(step0); return step1; }

    bool guaranteed(Attr) const override final;

    Expr const* derivative(Variable const&) const override final;
    double value() const override final { return std::tanh(f_x->evaluate()); }

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** SecH
***********************************************************************************************************************/

struct SecH final : public FunctionNode, private ObjectGuard<SecH>
{
    SecH(Expr const* p) : FunctionNode(p, NodeType::SECH) { }

    Expr const* invert() const override final { return f_x->cosh(); }

    Expr const* bind(std::vector<std::pair<Variable, Expr const*>> const& r) const override final { auto step0 = f_x->bind(r); auto step1 = step0->sech(); Erase(step0); return step1; }

    bool guaranteed(Attr) const override final;

    Expr const* derivative(Variable const&) const override final;
    double value() const override final { return 1 / std::cosh(f_x->evaluate()); }

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** ASinH
***********************************************************************************************************************/

struct ASinH final : public FunctionNode, private ObjectGuard<ASinH>
{
    ASinH(Expr const* p) : FunctionNode(p, NodeType::ASINH) { }

    Expr const* sgn() const override final { return f_x->sgn(); }
    Expr const* exp() const override final { auto step0 = f_x->yconic(); auto step1 = f_x->add(step0); Erase(step0); return step1; }
    Expr const* sinh() const override final { return Clone(f_x); }
    Expr const* cosh() const override final { return f_x->yconic(); }

    Expr const* bind(std::vector<std::pair<Variable, Expr const*>> const& r) const override final { auto step0 = f_x->bind(r); auto step1 = step0->asinh(); Erase(step0); return step1; }

    bool guaranteed(Attr) const override final;

    Expr const* derivative(Variable const&) const override final;
    double value() const override final { return std::asinh(f_x->evaluate()); }

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** ACosH
***********************************************************************************************************************/

struct ACosH final : public FunctionNode, private ObjectGuard<ACosH>
{
    ACosH(Expr const* p) : FunctionNode(p, NodeType::ACOSH) { }

    Expr const* abs() const override final { return Clone(this); }
    Expr const* sinh() const override final { return f_x->zconic(); }
    Expr const* cosh() const override final { return Clone(f_x); }

    Expr const* bind(std::vector<std::pair<Variable, Expr const*>> const& r) const override final { auto step0 = f_x->bind(r); auto step1 = step0->acosh(); Erase(step0); return step1; }

    bool guaranteed(Attr) const override final;

    Expr const* derivative(Variable const&) const override final;
    double value() const override final { return std::acosh(f_x->evaluate()); }

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** ATanH
***********************************************************************************************************************/

struct ATanH final : public FunctionNode, private ObjectGuard<ATanH>
{
    ATanH(Expr const* p) : FunctionNode(p, NodeType::ATANH) { }

    Expr const* sgn() const override final { return f_x->sgn(); }
    Expr const* cosh() const override final { auto step0 = f_x->zconic(); auto step1 = step0->invert(); Erase(step0); return step1; }
    Expr const* tanh() const override final { return Clone(f_x); }

    Expr const* bind(std::vector<std::pair<Variable, Expr const*>> const& r) const override final { auto step0 = f_x->bind(r); auto step1 = step0->atanh(); Erase(step0); return step1; }

    bool guaranteed(Attr) const override final;

    Expr const* derivative(Variable const&) const override final;
    double value() const override final { return std::atanh(f_x->evaluate()); }

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** Erf
***********************************************************************************************************************/

struct Erf final : public FunctionNode, private ObjectGuard<Erf>
{
    Erf(Expr const* p) : FunctionNode(p, NodeType::ERF) { }

    Expr const* sgn() const override final { return f_x->sgn(); }

    Expr const* bind(std::vector<std::pair<Variable, Expr const*>> const& r) const override final { auto step0 = f_x->bind(r); auto step1 = step0->erf(); Erase(step0); return step1; }

    bool guaranteed(Attr) const override final;

    Expr const* derivative(Variable const&) const override final;
    double value() const override final { return std::erf(f_x->evaluate()); }

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** ErfC
***********************************************************************************************************************/

struct ErfC final : public FunctionNode, private ObjectGuard<ErfC>
{
    ErfC(Expr const* p) : FunctionNode(p, NodeType::ERF) { }

    // TODO: Expr const* sgn() const override final { return f_x->sgn(); }

    Expr const* bind(std::vector<std::pair<Variable, Expr const*>> const& r) const override final { auto step0 = f_x->bind(r); auto step1 = step0->erfc(); Erase(step0); return step1; }

    bool guaranteed(Attr) const override final;

    Expr const* derivative(Variable const&) const override final;
    double value() const override final { return std::erfc(f_x->evaluate()); }

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** Invert
***********************************************************************************************************************/

struct Invert final : public FunctionNode, private ObjectGuard<Invert>
{
    Invert(Expr const* p) : FunctionNode(p, NodeType::INVERT) { }

    Expr const* abs() const override final { auto step0 = f_x->abs(); auto step1 = step0->invert(); Erase(step0); return step1; }
    Expr const* sgn() const override final { auto step0 = f_x->sgn(); auto step1 = step0->invert(); Erase(step0); return step1; }
    Expr const* sqrt() const override final { auto step0 = f_x->sqrt(); auto step1 = step0->invert(); Erase(step0); return step1; }
    Expr const* cbrt() const override final { auto step0 = f_x->cbrt(); auto step1 = step0->invert(); Erase(step0); return step1; }
    Expr const* log() const override final { auto step0 = f_x->log(); auto step1 = step0->negate(); Erase(step0); return step1; }
    Expr const* invert() const override final { return Clone(f_x); }
    Expr const* square() const override final { auto step0 = f_x->square(); auto step1 = step0->invert(); Erase(step0); return step1; }

    Expr const* mul(Expr const*) const override final;
    Expr const* pow(Expr const* p) const override final { auto step0 = f_x->pow(p); auto step1 = step0->invert(); Erase(step0); return step1; }

    Expr const* bind(std::vector<std::pair<Variable, Expr const*>> const& r) const override final { auto step0 = f_x->bind(r); auto step1 = step0->invert(); Erase(step0); return step1; }

    bool easyInvert() const override final { return true; }
    bool easyNegate() const override final { return f_x->easyNegate(); }

    bool guaranteed(Attr) const override final;

    Expr const* derivative(Variable const&) const override final;
    double value() const override final { return 1 / f_x->evaluate(); }

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** Negate
***********************************************************************************************************************/

struct Negate final : public FunctionNode, private ObjectGuard<Negate>
{
    Negate(Expr const* p) : FunctionNode(p, NodeType::NEGATE) { }

    Expr const* abs() const override final { return f_x->abs(); }
    Expr const* sgn() const override final { auto step0 = f_x->sgn(); auto step1 = step0->negate(); Erase(step0); return step1; }
    Expr const* cbrt() const override final { auto step0 = f_x->cbrt(); auto step1 = step0->negate(); Erase(step0); return step1; }
    Expr const* exp() const override final { auto step0 = f_x->exp(); auto step1 = step0->invert(); Erase(step0); return step1; }
    Expr const* sin() const override final { auto step0 = f_x->sin(); auto step1 = step0->negate(); Erase(step0); return step1; }
    Expr const* cos() const override final { return f_x->cos(); }
    Expr const* tan() const override final { auto step0 = f_x->tan(); auto step1 = step0->negate(); Erase(step0); return step1; }
    Expr const* sec() const override final { return f_x->sec(); }
    Expr const* asin() const override final { auto step0 = f_x->asin(); auto step1 = step0->negate(); Erase(step0); return step1; }
    Expr const* atan() const override final { auto step0 = f_x->atan(); auto step1 = step0->negate(); Erase(step0); return step1; }
    Expr const* sinh() const override final { auto step0 = f_x->sinh(); auto step1 = step0->negate(); Erase(step0); return step1; }
    Expr const* cosh() const override final { return f_x->cosh(); }
    Expr const* tanh() const override final { auto step0 = f_x->tanh(); auto step1 = step0->negate(); Erase(step0); return step1; }
    Expr const* sech() const override final { return f_x->sech(); }
    Expr const* asinh() const override final { auto step0 = f_x->asinh(); auto step1 = step0->negate(); Erase(step0); return step1; }
    Expr const* atanh() const override final { auto step0 = f_x->atanh(); auto step1 = step0->negate(); Erase(step0); return step1; }
    Expr const* erf() const override final { auto step0 = f_x->erf(); auto step1 = step0->negate(); Erase(step0); return step1; }
    Expr const* invert() const override final { auto step0 = f_x->invert(); auto step1 = step0->negate(); Erase(step0); return step1; }
    Expr const* negate() const override final { return Clone(f_x); }
    Expr const* square() const override final { return f_x->square(); }
    Expr const* xconic() const override final { return f_x->xconic(); }
    Expr const* yconic() const override final { return f_x->yconic(); }
    Expr const* zconic() const override final { return f_x->zconic(); }

    Expr const* add(Expr const*) const override final;
    Expr const* mul(Expr const*) const override final;

    Expr const* bind(std::vector<std::pair<Variable, Expr const*>> const& r) const override final { auto step0 = f_x->bind(r); auto step1 = step0->negate(); Erase(step0); return step1; }

    bool easyInvert() const override final { return f_x->easyInvert(); }
    bool easyNegate() const override final { return true; }

    bool guaranteed(Attr) const override final;

    Expr const* derivative(Variable const&) const override final;
    double value() const override final { return -f_x->evaluate(); }

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** SoftPP
***********************************************************************************************************************/

struct SoftPP final : public FunctionNode, private ObjectGuard<SoftPP>
{
    SoftPP(Expr const* p) : FunctionNode(p, NodeType::SOFTPP) { }

    Expr const* bind(std::vector<std::pair<Variable, Expr const*>> const& r) const override final { auto step0 = f_x->bind(r); auto step1 = step0->softpp(); Erase(step0); return step1; }

    bool guaranteed(Attr) const override final;

    Expr const* derivative(Variable const&) const override final;
    double value() const override final { return Spp(f_x->evaluate()); }

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** Spence
***********************************************************************************************************************/

struct Spence final : public FunctionNode, private ObjectGuard<Spence>
{
    Spence(Expr const* p) : FunctionNode(p, NodeType::SPENCE) { }

    Expr const* bind(std::vector<std::pair<Variable, Expr const*>> const& r) const override final { auto step0 = f_x->bind(r); auto step1 = step0->spence(); Erase(step0); return step1; }

    bool guaranteed(Attr) const override final;

    Expr const* derivative(Variable const&) const override final;
    double value() const override final { return Li2(f_x->evaluate()); }

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** Square
***********************************************************************************************************************/

struct Square final : public FunctionNode, private ObjectGuard<Square>
{
    Square(Expr const* p) : FunctionNode(p, NodeType::SQUARE) { }

    Expr const* abs() const override final { return Clone(this); }
    Expr const* sqrt() const override final { return f_x->abs(); }

    Expr const* pow(Expr const* p) const override final;

    Expr const* bind(std::vector<std::pair<Variable, Expr const*>> const& r) const override final { auto step0 = f_x->bind(r); auto step1 = step0->square(); Erase(step0); return step1; }

    bool guaranteed(Attr) const override final;

    Expr const* derivative(Variable const&) const override final;
    double value() const override final { auto x = f_x->evaluate(); return x * x; }

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** XConic
***********************************************************************************************************************/

struct XConic final : public FunctionNode, private ObjectGuard<XConic>
{
    XConic(Expr const* p) : FunctionNode(p, NodeType::XCONIC) { }

    Expr const* abs() const override final { return Clone(this); }
    Expr const* asinh() const override final { auto step0 = f_x->abs(); auto step1 = step0->acosh(); Erase(step0); return step1; }
    Expr const* yconic() const override final { return f_x->abs(); }

    Expr const* bind(std::vector<std::pair<Variable, Expr const*>> const& r) const override final { auto step0 = f_x->bind(r); auto step1 = step0->xconic(); Erase(step0); return step1; }

    bool guaranteed(Attr) const override final;

    Expr const* derivative(Variable const&) const override final;
    double value() const override final { auto x = f_x->evaluate(); return std::sqrt(x * x - 1); }

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** YConic
***********************************************************************************************************************/

struct YConic final : public FunctionNode, private ObjectGuard<YConic>
{
    YConic(Expr const* p) : FunctionNode(p, NodeType::YCONIC) { }

    Expr const* abs() const override final { return Clone(this); }
    Expr const* acosh() const override final { auto step0 = f_x->asinh(); auto step1 = step0->abs(); Erase(step0); return step1; }
    Expr const* xconic() const override final { return f_x->abs(); }

    Expr const* bind(std::vector<std::pair<Variable, Expr const*>> const& r) const override final { auto step0 = f_x->bind(r); auto step1 = step0->yconic(); Erase(step0); return step1; }

    bool guaranteed(Attr) const override final;

    Expr const* derivative(Variable const&) const override final;
    double value() const override final { auto x = f_x->evaluate(); return std::sqrt(x * x + 1); }

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** ZConic
***********************************************************************************************************************/

struct ZConic final : public FunctionNode, private ObjectGuard<ZConic>
{
    ZConic(Expr const* p) : FunctionNode(p, NodeType::ZCONIC) { }

    Expr const* abs() const override final { return Clone(this); }
    Expr const* asin() const override final { auto step0 = f_x->abs(); auto step1 = step0->acos(); Erase(step0); return step1; }
    Expr const* acos() const override final { auto step0 = f_x->asin(); auto step1 = step0->abs(); Erase(step0); return step1; }
    Expr const* zconic() const override final { return f_x->abs(); }

    Expr const* bind(std::vector<std::pair<Variable, Expr const*>> const& r) const override final { auto step0 = f_x->bind(r); auto step1 = step0->zconic(); Erase(step0); return step1; }

    bool guaranteed(Attr) const override final;

    Expr const* derivative(Variable const&) const override final;
    double value() const override final { auto x = f_x->evaluate(); return std::sqrt(1 - x * x); }

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** function()
***********************************************************************************************************************/

Expr const* Expression::data::function(NodeType n) const
{
    auto node = functionNode.find(n);
    if (node != functionNode.end()) { return Clone(node->second); }

    switch (n)
    {
    case NodeType::ABS:
        return new Abs(Clone(this));

    case NodeType::SGN:
        return new Sgn(Clone(this));

    case NodeType::SQRT:
        return new Sqrt(Clone(this));

    case NodeType::CBRT:
        return new Cbrt(Clone(this));

    case NodeType::EXP:
        return new Exp(Clone(this));

    case NodeType::EXPM1:
        return new ExpM1(Clone(this));

    case NodeType::LOG:
        return new Log(Clone(this));

    case NodeType::LOG1P:
        return new Log1P(Clone(this));

    case NodeType::SIN:
        return new Sin(Clone(this));

    case NodeType::COS:
        return new Cos(Clone(this));

    case NodeType::TAN:
        return new Tan(Clone(this));

    case NodeType::SEC:
        return new Sec(Clone(this));

    case NodeType::ASIN:
        return new ASin(Clone(this));

    case NodeType::ACOS:
        return new ACos(Clone(this));

    case NodeType::ATAN:
        return new ATan(Clone(this));

    case NodeType::SINH:
        return new SinH(Clone(this));

    case NodeType::COSH:
        return new CosH(Clone(this));

    case NodeType::TANH:
        return new TanH(Clone(this));

    case NodeType::SECH:
        return new SecH(Clone(this));

    case NodeType::ASINH:
        return new ASinH(Clone(this));

    case NodeType::ACOSH:
        return new ACosH(Clone(this));

    case NodeType::ATANH:
        return new ATanH(Clone(this));

    case NodeType::ERF:
        return new Erf(Clone(this));

    case NodeType::ERFC:
        return new ErfC(Clone(this));

    case NodeType::INVERT:
        return new Invert(Clone(this));

    case NodeType::NEGATE:
        return new Negate(Clone(this));

    case NodeType::SOFTPP:
        return new SoftPP(Clone(this));

    case NodeType::SPENCE:
        return new Spence(Clone(this));

    case NodeType::SQUARE:
        return new Square(Clone(this));

    case NodeType::XCONIC:
        return new XConic(Clone(this));

    case NodeType::YCONIC:
        return new YConic(Clone(this));

    case NodeType::ZCONIC:
        return new ZConic(Clone(this));
    }

    return Clone(Nan::instance);
}

/***********************************************************************************************************************
*** Add
***********************************************************************************************************************/

struct Add final : public OperatorNode, private ObjectGuard<Add>
{
    Add(Expr const* p, Expr const* q) : OperatorNode(p, q)
    {
        assert(f_x->addNode.find(g_x) == f_x->addNode.end());
        assert(g_x->addNode.find(f_x) == g_x->addNode.end());

        f_x->addNode[g_x] = this;
        g_x->addNode[f_x] = this;
    }

    double value() const override final { return f_x->evaluate() + g_x->evaluate(); }

    Expr const* add(Expr const*) const override final;
    Expr const* commutative_add(Expr const*) const override final;
    Expr const* mul(Expr const*) const override final;
    Expr const* commutative_mul(Expr const*) const override final;

    Expr const* bind(std::vector<std::pair<Variable, Expr const*>> const& r) const override final
    {
        auto step0 = f_x->bind(r);
        auto step1 = g_x->bind(r);
        auto step2 = step0->add(step1);

        Erase(step0);
        Erase(step1);

        return step2;
    }

    bool is(NodeType t) const override final { return t == NodeType::ADD; }

    bool guaranteed(Attr) const override final;

    Expr const* derivative(Variable const&) const override final;

    void print(std::ostream&) const override final;

private:
    virtual ~Add()
    {
        assert(f_x->addNode.find(g_x) != f_x->addNode.end() && f_x->addNode[g_x] == this);
        assert(g_x->addNode.find(f_x) != g_x->addNode.end() && g_x->addNode[f_x] == this);

        f_x->addNode.erase(g_x);
        g_x->addNode.erase(f_x);
    }
};

/***********************************************************************************************************************
*** Mul
***********************************************************************************************************************/

struct Mul final : public OperatorNode, private ObjectGuard<Mul>
{
    Mul(Expr const* p, Expr const* q) : OperatorNode(p, q)
    {
        assert(f_x->mulNode.find(g_x) == f_x->mulNode.end());
        assert(g_x->mulNode.find(f_x) == g_x->mulNode.end());

        f_x->mulNode[g_x] = this;
        g_x->mulNode[f_x] = this;
    }

    Expr const* mul(Expr const*) const override final;
    Expr const* commutative_mul(Expr const*) const override final;

    Expr const* bind(std::vector<std::pair<Variable, Expr const*>> const& r) const override final
    {
        auto step0 = f_x->bind(r);
        auto step1 = g_x->bind(r);
        auto step2 = step0->mul(step1);

        Erase(step0);
        Erase(step1);

        return step2;
    }

    bool is(NodeType t) const override final { return t == NodeType::MUL; }

    bool guaranteed(Attr) const override final;

    Expr const* derivative(Variable const&) const override final;
    double value() const override final;

    void print(std::ostream&) const override final;

private:
    virtual ~Mul()
    {
        assert(f_x->mulNode.find(g_x) != f_x->mulNode.end() && f_x->mulNode[g_x] == this);
        assert(g_x->mulNode.find(f_x) != g_x->mulNode.end() && g_x->mulNode[f_x] == this);

        f_x->mulNode.erase(g_x);
        g_x->mulNode.erase(f_x);
    }
};

/***********************************************************************************************************************
*** Pow
***********************************************************************************************************************/

struct Pow final : public OperatorNode, private ObjectGuard<Pow>
{
    Pow(Expr const* p, Expr const* q) : OperatorNode(p, q)
    {
        assert(f_x->powNode.find(g_x) == f_x->powNode.end());

        f_x->powNode[g_x] = this;
    }

    virtual ~Pow()
    {
        assert(f_x->powNode.find(g_x) != f_x->powNode.end() && f_x->powNode[g_x] == this);

        f_x->powNode.erase(g_x);
    }

    Expr const* sqrt() const override final;
    Expr const* cbrt() const override final;
    Expr const* invert() const override final { auto step0 = g_x->negate(); auto step1 = f_x->pow(step0); Erase(step0); return step1; }
    Expr const* square() const override final;

    Expr const* mul(Expr const*) const override final;
    Expr const* commutative_mul(Expr const*) const override final;
    Expr const* pow(Expr const* p) const override final { auto step0 = g_x->mul(p); auto step1 = f_x->pow(step0); Erase(step0); return step1; }

    Expr const* bind(std::vector<std::pair<Variable, Expr const*>> const& r) const override final
    {
        auto step0 = f_x->bind(r);
        auto step1 = g_x->bind(r);
        auto step2 = step0->pow(step1);

        Erase(step0);
        Erase(step1);

        return step2;
    }

    bool is(NodeType t) const override final { return t == NodeType::POW; }

    bool guaranteed(Attr) const override final;

    Expr const* derivative(Variable const&) const override final;
    double value() const override final { return std::pow(f_x->evaluate(), g_x->evaluate()); }

    void print(std::ostream&) const override final;

private:
    virtual ~Pow()
    {
        assert(f_x->powNode.find(g_x) != f_x->powNode.end() && f_x->powNode[g_x] == this);

        f_x->powNode.erase(g_x);
    }
};

/***********************************************************************************************************************
*** abs()
***********************************************************************************************************************/

Expr const* Expression::data::abs() const
{
    if (guaranteed(Attr::NONNEGATIVE)) return Clone(this);
    if (guaranteed(Attr::NONPOSITIVE)) return negate();
    return function(NodeType::ABS);
}

/***********************************************************************************************************************
*** sgn()
***********************************************************************************************************************/

Expr const* Expression::data::sgn() const
{
    if (guaranteed(Attr::POSITIVE)) return constant(1);
    if (guaranteed(Attr::NEGATIVE)) return constant(-1);
    return function(NodeType::SGN);
}

/***********************************************************************************************************************
*** sqrt()
***********************************************************************************************************************/

Expr const* Pow::sqrt() const
{
    static ConstantNode inv2(1.0 / 2);

    auto step0 = g_x->mul(inv2);
    auto step1 = f_x->pow(step0);

    Erase(step0);

    return step1;
}

/***********************************************************************************************************************
*** cbrt()
***********************************************************************************************************************/

Expr const* Pow::cbrt() const
{
    static ConstantNode inv3(1.0 / 3);

    auto step0 = g_x->mul(inv3);
    auto step1 = f_x->pow(step0);

    Erase(step0);

    return step1;
}

/***********************************************************************************************************************
*** square()
***********************************************************************************************************************/

Expr const* Pow::square() const
{
    static ConstantNode num2(2);

    auto step0 = g_x->mul(num2);
    auto step1 = f_x->pow(step0);

    Erase(step0);

    return step1;
}

/***********************************************************************************************************************
*** add()
***********************************************************************************************************************/

Expr const* Expression::data::add(Expr const* p) const
{
    return p->commutative_add(this);
}

Expr const* Expression::data::commutative_add(Expr const* p) const
{
    auto node = addNode.find(p);
    return node != addNode.end() ? Clone(node->second) : new Add(Clone(p), Clone(this));
}

Expr const* ConstantNode::add(Expr const* p) const
{
    if (p->is(NodeType::CONSTANT)) return constant(n + p->evaluate());
    if (n == 0) return Clone(p);
    return Expr::add(p);
}

Expr const* ConstantNode::commutative_add(Expr const* p) const
{
    if (p->is(NodeType::CONSTANT)) return constant(n + p->evaluate());
    if (n == 0) return Clone(p);
    return Expr::commutative_add(p);
}

Expr const* Negate::add(Expr const* p) const
{
    return Expr::add(p);
}

Expr const* Add::add(Expr const* p) const
{
    if (depth > STACK_LIMIT)
    {
        if (f_x->depth < g_x->depth)
        {
            auto step0 = f_x->add(p);
            auto step1 = g_x->add(step0);
            Erase(step0);
            return step1;
        }

        if (f_x->depth > g_x->depth)
        {
            auto step0 = g_x->add(p);
            auto step1 = f_x->add(step0);
            Erase(step0);
            return step1;
        }
    }

    return Expr::add(p);
}

Expr const* Add::commutative_add(Expr const* p) const
{
    if (depth > STACK_LIMIT)
    {
        if (f_x->depth < g_x->depth)
        {
            auto step0 = f_x->commutative_add(p);
            auto step1 = g_x->commutative_add(step0);
            Erase(step0);
            return step1;
        }

        if (f_x->depth > g_x->depth)
        {
            auto step0 = g_x->commutative_add(p);
            auto step1 = f_x->commutative_add(step0);
            Erase(step0);
            return step1;
        }
    }

    return Expr::commutative_add(p);
}

/***********************************************************************************************************************
*** mul()
***********************************************************************************************************************/

Expr const* Expression::data::mul(Expr const* p) const
{
    if (p == this) return square();
    return p->commutative_mul(this);
}

Expr const* Expression::data::commutative_mul(Expr const* p) const
{
    auto node = mulNode.find(p);
    return node != mulNode.end() ? Clone(node->second) : new Mul(Clone(p), Clone(this));
}

Expr const* ConstantNode::mul(Expr const* p) const
{
    if (p->is(NodeType::CONSTANT)) return constant(n * p->evaluate());
    if (n == 0) return Clone(this);
    if (n == 1) return Clone(p);
    if (n == -1) return p->negate();
    return Expr::mul(p);
}

Expr const* ConstantNode::commutative_mul(Expr const* p) const
{
    if (p->is(NodeType::CONSTANT)) return constant(n * p->evaluate());
    if (n == 0) return Clone(this);
    if (n == 1) return Clone(p);
    if (n == -1) return p->negate();
    return Expr::commutative_mul(p);
}

Expr const* Invert::mul(Expr const* p) const
{
    if (p->easyInvert())  // 1/x * 1/p  ---->  1/(x * p)
    {
        auto step0 = p->invert();
        auto step1 = f_x->mul(step0);
        auto step2 = step1->invert();
        Erase(step0);
        Erase(step1);
        return step2;
    }

    return Expr::mul(p);
}

Expr const* Negate::mul(Expr const* p) const
{
    if (p->easyNegate())  // -x * -p  ---->  x * p
    {
        auto step0 = p->negate();
        auto step1 = f_x->mul(step0);
        Erase(step0);
        return step1;
    }
    else  // -x * p  ---->  -(x * p)
    {
        auto step0 = f_x->mul(p);
        auto step1 = step0->negate();
        Erase(step0);
        return step1;
    }
}

Expr const* Add::mul(Expr const* p) const
{
    if (depth > STACK_LIMIT)
    {
        auto step0 = f_x->mul(p);
        auto step1 = g_x->mul(p);
        auto step2 = step0->add(step1);
        Erase(step0);
        Erase(step1);
        return step2;
    }

    return Expr::mul(p);
}

Expr const* Add::commutative_mul(Expr const* p) const
{
    if (depth > STACK_LIMIT)
    {
        auto step0 = p->commutative_mul(f_x);
        auto step1 = p->commutative_mul(g_x);
        auto step2 = step0->add(step1);
        Erase(step0);
        Erase(step1);
        return step2;
    }

    return Expr::commutative_mul(p);
}

Expr const* Mul::mul(Expr const* p) const
{
    if (depth > STACK_LIMIT)
    {
        if (f_x->depth < g_x->depth)
        {
            auto step0 = f_x->mul(p);
            auto step1 = g_x->mul(step0);
            Erase(step0);
            return step1;
        }

        if (f_x->depth > g_x->depth)
        {
            auto step0 = g_x->mul(p);
            auto step1 = f_x->mul(step0);
            Erase(step0);
            return step1;
        }
    }

    return Expr::mul(p);
}

Expr const* Mul::commutative_mul(Expr const* p) const
{
    if (depth > STACK_LIMIT)
    {
        if (f_x->depth < g_x->depth)
        {
            auto step0 = f_x->commutative_mul(p);
            auto step1 = g_x->commutative_mul(step0);
            Erase(step0);
            return step1;
        }

        if (f_x->depth > g_x->depth)
        {
            auto step0 = g_x->commutative_mul(p);
            auto step1 = f_x->commutative_mul(step0);
            Erase(step0);
            return step1;
        }
    }

    return Expr::commutative_mul(p);
}

Expr const* Pow::mul(Expr const* p) const
{
    static ConstantNode num1(1);

    if (f_x == p)
    {
        auto step0 = g_x->add(num1);
        auto step1 = f_x->pow(step0);

        Erase(step0);

        return step1;
    }

    return Expr::mul(p);
}

Expr const* Pow::commutative_mul(Expr const* p) const
{
    static ConstantNode num1(1);

    if (f_x == p)
    {
        auto step0 = g_x->add(num1);
        auto step1 = f_x->pow(step0);

        Erase(step0);

        return step1;
    }

    return Expr::commutative_mul(p);
}

/***********************************************************************************************************************
*** pow()
***********************************************************************************************************************/

Expr const* Expression::data::pow(Expr const* p) const
{
    if (p->is(NodeType::CONSTANT))
    {
        double n = p->evaluate();
        if (n == 0) return constant(1);
        if (n == 1) return Clone(this);
        if (n == 2) return square();
        if (n == -1) return invert();
        if (n == 1.0 / 2.0) return sqrt();
        if (n == 1.0 / 3.0) return cbrt();
    }

    auto node = powNode.find(p);
    return node != powNode.end() ? Clone(node->second) : new Pow(Clone(this), Clone(p));
}

Expr const* ConstantNode::pow(Expr const* p) const
{
    if (p->is(NodeType::CONSTANT)) return constant(std::pow(n, p->evaluate()));
    if (n == 0 && p->guaranteed(Attr::NONZERO)) return Clone(this);
    if (n == 1) return Clone(this);
    if (n == std::exp(1)) return p->exp();
    return Expr::pow(p);
}

Expr const* Sqrt::pow(Expr const* p) const
{
    static ConstantNode inv2(1.0 / 2);

    auto step0 = p->mul(inv2);
    auto step1 = f_x->pow(step0);

    Erase(step0);

    return step1;
}

Expr const* Cbrt::pow(Expr const* p) const
{
    static ConstantNode inv3(1.0 / 3);

    auto step0 = p->mul(inv3);
    auto step1 = f_x->pow(step0);

    Erase(step0);

    return step1;
}

Expr const* Square::pow(Expr const* p) const
{
    static ConstantNode num2(2);

    auto step0 = p->mul(num2);
    auto step1 = f_x->pow(step0);

    Erase(step0);

    return step1;
}

/***********************************************************************************************************************
*** derivative()
***********************************************************************************************************************/

Expr const* ConstantNode::derivative(Variable const&) const
{
    // D(n) = 0

    return constant(0);
}

Expr const* VariableNode::derivative(Variable const& r) const
{
    // D(x) = 1 , D(?) = 0

    return constant(r.id() == x.id());
}

Expr const* Abs::derivative(Variable const& r) const
{
    // D(abs(f_x)) = D(f_x) * sgn(f_x)

    auto step0 = f_x->sgn();
    auto step1 = f_x->derive(r);
    auto step2 = step0->mul(step1);


    Erase(step0);
    Erase(step1);

    return step2;
}

Expr const* Sgn::derivative(Variable const& r) const
{
    // D(sgn(f_x)) = D(f_x) * 0

    return constant(0);
}

Expr const* Sqrt::derivative(Variable const& r) const
{
    // D(sqrt(f_x)) = D(f_x) * 1/2 * 1/sqrt(f_x)

    static ConstantNode inv2(1.0 / 2);

    auto step0 = f_x->derive(r);
    auto step1 = this->invert();
    auto step2 = step1->mul(inv2);
    auto step3 = step0->mul(step2);

    Erase(step0);
    Erase(step1);
    Erase(step2);

    return step3;
}

Expr const* Cbrt::derivative(Variable const& r) const
{
    // D(cbrt(f_x)) = D(f_x) * 1/3 * 1/cbrt(f_x)^2

    static ConstantNode inv3(1.0 / 3);

    auto step0 = f_x->derive(r);
    auto step1 = this->square();
    auto step2 = step1->invert();
    auto step3 = step2->mul(inv3);
    auto step4 = step0->mul(step3);

    Erase(step0);
    Erase(step1);
    Erase(step2);
    Erase(step3);

    return step4;
}

Expr const* Exp::derivative(Variable const& r) const
{
    // D(exp(f_x)) = D(f_x) * exp(f_x)

    auto step0 = f_x->derive(r);
    auto step1 = step0->mul(this);

    Erase(step0);

    return step1;
}

Expr const* ExpM1::derivative(Variable const& r) const
{
    // D(exp(f_x)-1) = D(f_x) * exp(f_x)

    auto step0 = f_x->derive(r);
    auto step1 = f_x->exp();
    auto step2 = step0->mul(step1);

    Erase(step0);
    Erase(step1);

    return step2;
}

Expr const* Log::derivative(Variable const& r) const
{
    // D(log(f_x)) = D(f_x) * 1/f_x

    auto step0 = f_x->derive(r);
    auto step1 = f_x->invert();
    auto step2 = step0->mul(step1);

    Erase(step0);
    Erase(step1);

    return step2;
}

Expr const* Log1P::derivative(Variable const& r) const
{
    // D(log(f_x+1)) = D(f_x) * 1/(f_x+1)

    static ConstantNode num1(1);

    auto step0 = f_x->derive(r);
    auto step1 = f_x->add(num1);
    auto step2 = step1->invert();
    auto step3 = step0->mul(step2);

    Erase(step0);
    Erase(step1);
    Erase(step2);

    return step3;
}

Expr const* Sin::derivative(Variable const& r) const
{
    // D(sin(f_x)) = D(f_x) * cos(f_x)

    auto step0 = f_x->derive(r);
    auto step1 = f_x->cos();
    auto step2 = step0->mul(step1);

    Erase(step0);
    Erase(step1);

    return step2;
}

Expr const* Cos::derivative(Variable const& r) const
{
    // D(cos(f_x)) = D(f_x) * -sin(f_x)

    auto step0 = f_x->derive(r);
    auto step1 = f_x->sin();
    auto step2 = step1->negate();
    auto step3 = step0->mul(step2);

    Erase(step0);
    Erase(step1);
    Erase(step2);

    return step3;
}

Expr const* Tan::derivative(Variable const& r) const
{
    // D(tan(f_x)) = D(f_x) * sec(f_x)^2

    auto step0 = f_x->derive(r);
    auto step1 = f_x->sec();
    auto step2 = step1->square();
    auto step3 = step0->mul(step2);

    Erase(step0);
    Erase(step1);
    Erase(step2);

    return step3;
}

Expr const* Sec::derivative(Variable const& r) const
{
    // D(sec(f_x)) = D(f_x) * tan(f_x)*sec(f_x)

    auto step0 = f_x->derive(r);
    auto step1 = f_x->tan();
    auto step2 = step1->mul(this);
    auto step3 = step0->mul(step2);

    Erase(step0);
    Erase(step1);
    Erase(step2);

    return step3;
}

Expr const* ASin::derivative(Variable const& r) const
{
    // D(asin(f_x)) = D(f_x) * 1/sqrt(1-f_x^2)

    auto step0 = f_x->derive(r);
    auto step1 = f_x->zconic();
    auto step2 = step1->invert();
    auto step3 = step0->mul(step2);

    Erase(step0);
    Erase(step1);
    Erase(step2);

    return step3;
}

Expr const* ACos::derivative(Variable const& r) const
{
    // D(acos(f_x)) = D(f_x) * -1/sqrt(1-f_x^2)

    auto step0 = f_x->derive(r);
    auto step1 = f_x->zconic();
    auto step2 = step1->invert();
    auto step3 = step2->negate();
    auto step4 = step0->mul(step3);

    Erase(step0);
    Erase(step1);
    Erase(step2);
    Erase(step3);

    return step4;
}

Expr const* ATan::derivative(Variable const& r) const
{
    // D(atan(f_x)) = D(f_x) * 1/(f_x^2+1)

    auto step0 = f_x->derive(r);
    auto step1 = f_x->yconic();
    auto step2 = step1->square();
    auto step3 = step2->invert();
    auto step4 = step0->mul(step3);

    Erase(step0);
    Erase(step1);
    Erase(step2);
    Erase(step3);

    return step4;
}

Expr const* SinH::derivative(Variable const& r) const
{
    // D(sinh(f_x)) = D(f_x) * cosh(f_x)

    auto step0 = f_x->derive(r);
    auto step1 = f_x->cosh();
    auto step2 = step0->mul(step1);

    Erase(step0);
    Erase(step1);

    return step2;
}

Expr const* CosH::derivative(Variable const& r) const
{
    // D(cosh(f_x)) = D(f_x) * sinh(f_x)

    auto step0 = f_x->derive(r);
    auto step1 = f_x->sinh();
    auto step2 = step0->mul(step1);

    Erase(step0);
    Erase(step1);

    return step2;
}

Expr const* TanH::derivative(Variable const& r) const
{
    // D(tanh(f_x)) = D(f_x) * sech(f_x)^2

    auto step0 = f_x->derive(r);
    auto step1 = f_x->sech();
    auto step2 = step1->square();
    auto step3 = step0->mul(step2);

    Erase(step0);
    Erase(step1);
    Erase(step2);

    return step3;
}

Expr const* SecH::derivative(Variable const& r) const
{
    // D(sech(f_x)) = D(f_x) * -tanh(f_x)*sech(f_x)

    auto step0 = f_x->derive(r);
    auto step1 = f_x->tanh();
    auto step2 = step1->mul(this);
    auto step3 = step2->negate();
    auto step4 = step0->mul(step3);

    Erase(step0);
    Erase(step1);
    Erase(step2);
    Erase(step3);

    return step4;
}

Expr const* ASinH::derivative(Variable const& r) const
{
    // D(asinh(f_x)) = D(f_x) * 1/sqrt(f_x^2+1)

    auto step0 = f_x->derive(r);
    auto step1 = f_x->yconic();
    auto step2 = step1->invert();
    auto step3 = step0->mul(step2);

    Erase(step0);
    Erase(step1);
    Erase(step2);

    return step3;
}

Expr const* ACosH::derivative(Variable const& r) const
{
    // D(acosh(f_x)) = D(f_x) * 1/sqrt(f_x^2-1)

    auto step0 = f_x->derive(r);
    auto step1 = f_x->xconic();
    auto step2 = step1->invert();
    auto step3 = step0->mul(step2);

    Erase(step0);
    Erase(step1);
    Erase(step2);

    return step3;
}

Expr const* ATanH::derivative(Variable const& r) const
{
    // D(atanh(f_x)) = D(f_x) * 1/(1-f_x^2)

    auto step0 = f_x->derive(r);
    auto step1 = f_x->zconic();
    auto step2 = step1->square();
    auto step3 = step2->invert();
    auto step4 = step0->mul(step3);

    Erase(step0);
    Erase(step1);
    Erase(step2);
    Erase(step3);

    return step4;
}

Expr const* Erf::derivative(Variable const& r) const
{
    ConstantNode InvSqrtAtan1(1 / std::sqrt(std::atan(1)));

    // D(erf(f_x)) = D(f_x) * 1/exp(f_x^2) * 1/sqrt(atan(1))

    auto step0 = f_x->derive(r);
    auto step1 = f_x->square();
    auto step2 = step1->exp();
    auto step3 = step2->invert();
    auto step4 = step3->mul(InvSqrtAtan1);
    auto step5 = step0->mul(step4);

    Erase(step0);
    Erase(step1);
    Erase(step2);
    Erase(step3);
    Erase(step4);

    return step5;
}

Expr const* ErfC::derivative(Variable const& r) const
{
    ConstantNode NegInvSqrtAtan1(-1 / std::sqrt(std::atan(1)));

    // D(erfc(f_x)) = D(f_x) * 1/exp(f_x^2) * -1/sqrt(atan(1))

    auto step0 = f_x->derive(r);
    auto step1 = f_x->square();
    auto step2 = step1->exp();
    auto step3 = step2->invert();
    auto step4 = step3->mul(NegInvSqrtAtan1);
    auto step5 = step0->mul(step4);

    Erase(step0);
    Erase(step1);
    Erase(step2);
    Erase(step3);
    Erase(step4);

    return step5;
}

Expr const* Invert::derivative(Variable const& r) const
{
    // D(1/f_x) = D(f_x) * -(1/f_x)^2

    auto step0 = f_x->derive(r);
    auto step1 = this->square();
    auto step2 = step1->negate();
    auto step3 = step0->mul(step2);

    Erase(step0);
    Erase(step1);
    Erase(step2);

    return step3;
}

Expr const* Negate::derivative(Variable const& r) const
{
    // D(-f_x) = D(f_x) * -1

    auto step0 = f_x->derive(r);
    auto step1 = step0->negate();

    Erase(step0);

    return step1;
}

Expr const* SoftPP::derivative(Variable const& r) const
{
    // D(-Li2(-exp(f_x))) = D(f_x) * log(1+exp(f_x))

    auto step0 = f_x->derive(r);
    auto step1 = f_x->exp();
    auto step2 = step1->log1p();
    auto step3 = step0->mul(step2);

    Erase(step0);
    Erase(step1);
    Erase(step2);

    return step3;
}

Expr const* Spence::derivative(Variable const& r) const
{
    // D(Li2(f_x)) = D(f_x) * log(1-f_x)/(-f_x)

    auto step0 = f_x->derive(r);
    auto step1 = f_x->negate();
    auto step2 = step1->log1p();
    auto step3 = step1->invert();
    auto step4 = step2->mul(step3);
    auto step5 = step0->mul(step4);

    Erase(step0);
    Erase(step1);
    Erase(step2);
    Erase(step3);
    Erase(step4);

    return step5;
}

Expr const* Square::derivative(Variable const& r) const
{
    // D(f_x^2) = D(f_x) * 2*f_x

    static ConstantNode num2(2);

    auto step0 = f_x->derive(r);
    auto step1 = f_x->mul(num2);
    auto step2 = step0->mul(step1);

    Erase(step0);
    Erase(step1);

    return step2;
}

Expr const* XConic::derivative(Variable const& r) const
{
    // D(sqrt(f_x^2-1)) = D(f_x) * f_x / sqrt(f_x^2-1)

    auto step0 = f_x->derive(r);
    auto step1 = this->invert();
    auto step2 = step1->mul(f_x);
    auto step3 = step0->mul(step2);

    Erase(step0);
    Erase(step1);
    Erase(step2);

    return step3;
}

Expr const* YConic::derivative(Variable const& r) const
{
    // D(sqrt(f_x^2+1)) = D(f_x) * f_x / sqrt(f_x^2+1)

    auto step0 = f_x->derive(r);
    auto step1 = this->invert();
    auto step2 = step1->mul(f_x);
    auto step3 = step0->mul(step2);

    Erase(step0);
    Erase(step1);
    Erase(step2);

    return step3;
}

Expr const* ZConic::derivative(Variable const& r) const
{
    // D(sqrt(1-f_x^2)) = D(f_x) * -f_x / sqrt(1-f_x^2)

    auto step0 = f_x->derive(r);
    auto step1 = this->invert();
    auto step2 = step1->mul(f_x);
    auto step3 = step2->negate();
    auto step4 = step0->mul(step3);

    Erase(step0);
    Erase(step1);
    Erase(step2);
    Erase(step3);

    return step4;
}

Expr const* Add::derivative(Variable const& r) const
{
    // D(f_x+g_x) = D(f_x) + D(g_x)

    auto step0 = f_x->derive(r);
    auto step1 = g_x->derive(r);
    auto step2 = step0->add(step1);

    Erase(step0);
    Erase(step1);

    return step2;
}

Expr const* Mul::derivative(Variable const& r) const
{
    // D(f_x*g_x) = D(f_x) * g_x + D(g_x) * f_x

    auto step0 = f_x->derive(r);
    auto step1 = g_x->derive(r);
    auto step2 = f_x->mul(step1);
    auto step3 = g_x->mul(step0);
    auto step4 = step2->add(step3);

    Erase(step0);
    Erase(step1);
    Erase(step2);
    Erase(step3);

    return step4;
}

Expr const* Pow::derivative(Variable const& r) const
{
    // D(f_x^g_x) = D(f_x) * g_x*f_x^(g_x-1) + D(g_x) * f_x^g_x*log(f_x)

    static ConstantNode neg1(-1);

    auto step0 = f_x->derive(r);
    auto step1 = g_x->derive(r);
    auto step2 = f_x->log();
    auto step3 = g_x->add(neg1);
    auto step4 = f_x->pow(step3);
    auto step5 = g_x->mul(step4);
    auto step6 = this->mul(step2);
    auto step7 = step0->mul(step5);
    auto step8 = step1->mul(step6);
    auto step9 = step7->add(step8);

    Erase(step0);
    Erase(step1);
    Erase(step2);
    Erase(step3);
    Erase(step4);
    Erase(step5);
    Erase(step6);
    Erase(step7);
    Erase(step8);

    return step9;
}

/***********************************************************************************************************************
*** value()
***********************************************************************************************************************/

double Mul::value() const
{
    // NOTE: All of '0*inf', '0*nan', 'inf*0', 'nan*0' deliberately evaluate as '0'
    // The purpose is to be able to prune [possibly undefined] branches of the expression tree by using a variable
    // and giving it a value '1' (=use the multiplicand) or '0' (=prune the expression contained by multiplicand)

    auto x = f_x->evaluate();
    if (x == 0) return 0;
    auto y = g_x->evaluate();
    if (y == 0) return 0;
    return x * y;
}

/***********************************************************************************************************************
*** guaranteed()
***********************************************************************************************************************/

/*
bool <generic>::guaranteed(Attr a) const
{
    if (f_x->guaranteed(Attr::DEFINED)) switch (a)
    {
    case Attr::DEFINED:
    case Attr::NONZERO:
    case Attr::POSITIVE:
    case Attr::NEGATIVE:
    case Attr::NONPOSITIVE:
    case Attr::NONNEGATIVE:
    case Attr::UNITRANGE:
    case Attr::ANTIUNITRANGE:
    case Attr::OPENUNITRANGE:
    case Attr::ANTIOPENUNITRANGE:
    case Attr::CONTINUOUS:
    case Attr::INCREASING:
    case Attr::DECREASING:
    case Attr::NONINCREASING:
    case Attr::NONDECREASING:
    case Attr::BOUNDEDABOVE:
    case Attr::BOUNDEDBELOW:
        ;
    }

    return false;
}
*/

bool ConstantNode::guaranteed(Attr a) const
{
    if (!isnan(n) && !isinf(n)) switch (a)
    {
    case Attr::DEFINED:
    case Attr::CONTINUOUS:
    case Attr::NONINCREASING:
    case Attr::NONDECREASING:
    case Attr::BOUNDEDABOVE:
    case Attr::BOUNDEDBELOW:
        return true;

    case Attr::NONZERO:
        return n != 0;

    case Attr::POSITIVE:
        return n > 0;

    case Attr::NEGATIVE:
        return n < 0;

    case Attr::NONPOSITIVE:
        return n <= 0;

    case Attr::NONNEGATIVE:
        return n >= 0;

    case Attr::UNITRANGE:
        return n >= -1 && n <= 1;

    case Attr::ANTIUNITRANGE:
        return n < -1 || n > 1;

    case Attr::OPENUNITRANGE:
        return n > -1 && n < 1;

    case Attr::ANTIOPENUNITRANGE:
        return n <= -1 || n >= 1;
    }

    return false;
}

bool VariableNode::guaranteed(Attr a) const
{
    switch (a)
    {
    case Attr::DEFINED:
    case Attr::CONTINUOUS:
    case Attr::INCREASING:
    case Attr::NONDECREASING:
        return true;
    }

    return false;
}

bool Abs::guaranteed(Attr a) const
{
    if (f_x->guaranteed(Attr::DEFINED)) switch (a)
    {
    case Attr::DEFINED:
    case Attr::NONNEGATIVE:
    case Attr::BOUNDEDBELOW:
        return true;

    case Attr::NONZERO:
    case Attr::UNITRANGE:
    case Attr::ANTIUNITRANGE:
    case Attr::OPENUNITRANGE:
    case Attr::ANTIOPENUNITRANGE:
    case Attr::CONTINUOUS:
        return f_x->guaranteed(a);

    case Attr::POSITIVE:
        return f_x->guaranteed(Attr::NONZERO);

    case Attr::INCREASING:
        if (f_x->guaranteed(Attr::INCREASING) && f_x->guaranteed(Attr::POSITIVE)) return true;
        if (f_x->guaranteed(Attr::DECREASING) && f_x->guaranteed(Attr::NEGATIVE)) return true;
        return false;

    case Attr::DECREASING:
        if (f_x->guaranteed(Attr::DECREASING) && f_x->guaranteed(Attr::POSITIVE)) return true;
        if (f_x->guaranteed(Attr::INCREASING) && f_x->guaranteed(Attr::NEGATIVE)) return true;
        return false;

    case Attr::NONINCREASING:
        if (f_x->guaranteed(Attr::NONINCREASING) && f_x->guaranteed(Attr::NONNEGATIVE)) return true;
        if (f_x->guaranteed(Attr::NONDECREASING) && f_x->guaranteed(Attr::NONPOSITIVE)) return true;
        return false;

    case Attr::NONDECREASING:
        if (f_x->guaranteed(Attr::NONDECREASING) && f_x->guaranteed(Attr::NONNEGATIVE)) return true;
        if (f_x->guaranteed(Attr::NONINCREASING) && f_x->guaranteed(Attr::NONPOSITIVE)) return true;
        return false;

    case Attr::BOUNDEDABOVE:
        return f_x->guaranteed(Attr::BOUNDEDABOVE) && f_x->guaranteed(Attr::BOUNDEDBELOW);
    }

    return false;
}

bool Sgn::guaranteed(Attr a) const
{
    if (f_x->guaranteed(Attr::DEFINED)) switch (a)
    {
    case Attr::DEFINED:
    case Attr::UNITRANGE:
    case Attr::BOUNDEDABOVE:
    case Attr::BOUNDEDBELOW:
        return true;

    case Attr::NONZERO:
    case Attr::POSITIVE:
    case Attr::NEGATIVE:
    case Attr::NONPOSITIVE:
    case Attr::NONNEGATIVE:
        return f_x->guaranteed(a);

    case Attr::ANTIOPENUNITRANGE:
        return f_x->guaranteed(Attr::NONZERO);

    case Attr::CONTINUOUS:
        return f_x->guaranteed(Attr::POSITIVE) || f_x->guaranteed(Attr::NEGATIVE);

    case Attr::NONINCREASING:
        return f_x->guaranteed(Attr::NONINCREASING) || f_x->guaranteed(Attr::POSITIVE) || f_x->guaranteed(Attr::NEGATIVE);

    case Attr::NONDECREASING:
        return f_x->guaranteed(Attr::NONDECREASING) || f_x->guaranteed(Attr::POSITIVE) || f_x->guaranteed(Attr::NEGATIVE);
    }

    return false;
}

bool Sqrt::guaranteed(Attr a) const
{
    if (f_x->guaranteed(Attr::NONNEGATIVE)) switch (a)
    {
    case Attr::DEFINED:
    case Attr::NONNEGATIVE:
    case Attr::BOUNDEDBELOW:
        return true;

    case Attr::NONZERO:
    case Attr::POSITIVE:
    case Attr::UNITRANGE:
    case Attr::ANTIUNITRANGE:
    case Attr::OPENUNITRANGE:
    case Attr::ANTIOPENUNITRANGE:
    case Attr::CONTINUOUS:
    case Attr::INCREASING:
    case Attr::DECREASING:
    case Attr::NONINCREASING:
    case Attr::NONDECREASING:
    case Attr::BOUNDEDABOVE:
        return f_x->guaranteed(a);
    }

    return false;
}

bool Cbrt::guaranteed(Attr a) const
{
    if (f_x->guaranteed(Attr::DEFINED)) switch (a)
    {
    case Attr::DEFINED:
        return true;

    case Attr::NONZERO:
    case Attr::POSITIVE:
    case Attr::NEGATIVE:
    case Attr::NONPOSITIVE:
    case Attr::NONNEGATIVE:
    case Attr::UNITRANGE:
    case Attr::ANTIUNITRANGE:
    case Attr::OPENUNITRANGE:
    case Attr::ANTIOPENUNITRANGE:
    case Attr::CONTINUOUS:
    case Attr::INCREASING:
    case Attr::DECREASING:
    case Attr::NONINCREASING:
    case Attr::NONDECREASING:
    case Attr::BOUNDEDABOVE:
    case Attr::BOUNDEDBELOW:
        return f_x->guaranteed(a);
    }

    return false;
}

bool Exp::guaranteed(Attr a) const
{
    if (f_x->guaranteed(Attr::DEFINED)) switch (a)
    {
    case Attr::DEFINED:
    case Attr::NONZERO:
    case Attr::POSITIVE:
    case Attr::NONNEGATIVE:
    case Attr::BOUNDEDBELOW:
        return true;

    case Attr::CONTINUOUS:
    case Attr::INCREASING:
    case Attr::DECREASING:
    case Attr::NONINCREASING:
    case Attr::NONDECREASING:
    case Attr::BOUNDEDABOVE:
        return f_x->guaranteed(a);

    case Attr::UNITRANGE:
        return f_x->guaranteed(Attr::NONPOSITIVE);

    case Attr::ANTIUNITRANGE:
        return f_x->guaranteed(Attr::POSITIVE);

    case Attr::OPENUNITRANGE:
        return f_x->guaranteed(Attr::NEGATIVE);

    case Attr::ANTIOPENUNITRANGE:
        return f_x->guaranteed(Attr::NONNEGATIVE);
    }

    return false;
}

bool ExpM1::guaranteed(Attr a) const
{
    if (f_x->guaranteed(Attr::DEFINED)) switch (a)
    {
    case Attr::DEFINED:
    case Attr::BOUNDEDBELOW:
        return true;

    case Attr::NONZERO:
    case Attr::POSITIVE:
    case Attr::NEGATIVE:
    case Attr::NONPOSITIVE:
    case Attr::NONNEGATIVE:
    case Attr::CONTINUOUS:
    case Attr::INCREASING:
    case Attr::DECREASING:
    case Attr::NONINCREASING:
    case Attr::NONDECREASING:
    case Attr::BOUNDEDABOVE:
        return f_x->guaranteed(a);

    case Attr::UNITRANGE:
    case Attr::OPENUNITRANGE:
        return f_x->guaranteed(Attr::NONPOSITIVE);
    }

    return false;
}

bool Log::guaranteed(Attr a) const
{
    if (f_x->guaranteed(Attr::POSITIVE)) switch (a)
    {
    case Attr::DEFINED:
        return true;

    case Attr::CONTINUOUS:
    case Attr::INCREASING:
    case Attr::DECREASING:
    case Attr::NONINCREASING:
    case Attr::NONDECREASING:
    case Attr::BOUNDEDABOVE:
        return f_x->guaranteed(a);

    case Attr::NONZERO:
        return f_x->guaranteed(Attr::ANTIUNITRANGE) || f_x->guaranteed(Attr::OPENUNITRANGE);

    case Attr::POSITIVE:
        return f_x->guaranteed(Attr::ANTIUNITRANGE);

    case Attr::NEGATIVE:
        return f_x->guaranteed(Attr::OPENUNITRANGE);

    case Attr::NONPOSITIVE:
        return f_x->guaranteed(Attr::UNITRANGE);

    case Attr::NONNEGATIVE:
        return f_x->guaranteed(Attr::ANTIOPENUNITRANGE);
    }

    return false;
}

bool Log1P::guaranteed(Attr a) const
{
    if (f_x->guaranteed(Attr::OPENUNITRANGE) || f_x->guaranteed(Attr::POSITIVE)) switch (a)
    {
    case Attr::DEFINED:
        return true;

    case Attr::NONZERO:
    case Attr::POSITIVE:
    case Attr::NEGATIVE:
    case Attr::NONPOSITIVE:
    case Attr::NONNEGATIVE:
    case Attr::CONTINUOUS:
    case Attr::INCREASING:
    case Attr::DECREASING:
    case Attr::NONINCREASING:
    case Attr::NONDECREASING:
    case Attr::BOUNDEDABOVE:
    case Attr::BOUNDEDBELOW:
        return f_x->guaranteed(a);
    }

    return false;
}

bool Sin::guaranteed(Attr a) const
{
    if (f_x->guaranteed(Attr::DEFINED)) switch (a)
    {
    case Attr::DEFINED:
    case Attr::UNITRANGE:
    case Attr::BOUNDEDABOVE:
    case Attr::BOUNDEDBELOW:
        return true;

    case Attr::CONTINUOUS:
        return f_x->guaranteed(a);
    }

    return false;
}

bool Cos::guaranteed(Attr a) const
{
    if (f_x->guaranteed(Attr::DEFINED)) switch (a)
    {
    case Attr::DEFINED:
    case Attr::UNITRANGE:
    case Attr::BOUNDEDABOVE:
    case Attr::BOUNDEDBELOW:
        return true;

    case Attr::CONTINUOUS:
        return f_x->guaranteed(a);
    }

    return false;
}

bool Tan::guaranteed(Attr a) const
{
    return false;
}

bool Sec::guaranteed(Attr a) const
{
    return false;
}

bool ASin::guaranteed(Attr a) const
{
    if (f_x->guaranteed(Attr::UNITRANGE)) switch (a)
    {
    case Attr::DEFINED:
    case Attr::BOUNDEDABOVE:
    case Attr::BOUNDEDBELOW:
        return true;

    case Attr::NONZERO:
    case Attr::POSITIVE:
    case Attr::NEGATIVE:
    case Attr::NONPOSITIVE:
    case Attr::NONNEGATIVE:
    case Attr::CONTINUOUS:
    case Attr::INCREASING:
    case Attr::DECREASING:
    case Attr::NONINCREASING:
    case Attr::NONDECREASING:
        return f_x->guaranteed(a);
    }

    return false;
}

bool ACos::guaranteed(Attr a) const
{
    if (f_x->guaranteed(Attr::UNITRANGE)) switch (a)
    {
    case Attr::DEFINED:
    case Attr::NONNEGATIVE:
    case Attr::BOUNDEDABOVE:
    case Attr::BOUNDEDBELOW:
        return true;

    case Attr::CONTINUOUS:
        return f_x->guaranteed(a);

    case Attr::NONZERO:
    case Attr::POSITIVE:
        return f_x->guaranteed(Attr::OPENUNITRANGE);

    case Attr::NONPOSITIVE:
        return f_x->guaranteed(Attr::POSITIVE) && f_x->guaranteed(Attr::ANTIOPENUNITRANGE);

    case Attr::INCREASING:
        return f_x->guaranteed(Attr::DECREASING);

    case Attr::DECREASING:
        return f_x->guaranteed(Attr::INCREASING);

    case Attr::NONINCREASING:
        return f_x->guaranteed(Attr::NONDECREASING);

    case Attr::NONDECREASING:
        return f_x->guaranteed(Attr::NONINCREASING);
    }

    return false;
}

bool ATan::guaranteed(Attr a) const
{
    if (f_x->guaranteed(Attr::DEFINED)) switch (a)
    {
    case Attr::DEFINED:
    case Attr::BOUNDEDABOVE:
    case Attr::BOUNDEDBELOW:
        return true;

    case Attr::NONZERO:
    case Attr::POSITIVE:
    case Attr::NEGATIVE:
    case Attr::NONPOSITIVE:
    case Attr::NONNEGATIVE:
    case Attr::CONTINUOUS:
    case Attr::INCREASING:
    case Attr::DECREASING:
    case Attr::NONINCREASING:
    case Attr::NONDECREASING:
        return f_x->guaranteed(a);
    }

    return false;
}

bool SinH::guaranteed(Attr a) const
{
    if (f_x->guaranteed(Attr::DEFINED)) switch (a)
    {
    case Attr::DEFINED:
        return true;

    case Attr::NONZERO:
    case Attr::POSITIVE:
    case Attr::NEGATIVE:
    case Attr::NONPOSITIVE:
    case Attr::NONNEGATIVE:
    case Attr::CONTINUOUS:
    case Attr::INCREASING:
    case Attr::DECREASING:
    case Attr::NONINCREASING:
    case Attr::NONDECREASING:
    case Attr::BOUNDEDABOVE:
    case Attr::BOUNDEDBELOW:
        return f_x->guaranteed(a);
    }

    return false;
}

bool CosH::guaranteed(Attr a) const
{
    if (f_x->guaranteed(Attr::DEFINED)) switch (a)
    {
    case Attr::DEFINED:
    case Attr::NONZERO:
    case Attr::POSITIVE:
    case Attr::NONNEGATIVE:
    case Attr::ANTIOPENUNITRANGE:
    case Attr::BOUNDEDBELOW:
        return true;

    case Attr::CONTINUOUS:
        return f_x->guaranteed(a);

    case Attr::ANTIUNITRANGE:
        return f_x->guaranteed(Attr::NONZERO);

    case Attr::INCREASING:
        if (f_x->guaranteed(Attr::INCREASING) && f_x->guaranteed(Attr::POSITIVE)) return true;
        if (f_x->guaranteed(Attr::DECREASING) && f_x->guaranteed(Attr::NEGATIVE)) return true;
        return false;

    case Attr::DECREASING:
        if (f_x->guaranteed(Attr::DECREASING) && f_x->guaranteed(Attr::POSITIVE)) return true;
        if (f_x->guaranteed(Attr::INCREASING) && f_x->guaranteed(Attr::NEGATIVE)) return true;
        return false;

    case Attr::NONINCREASING:
        if (f_x->guaranteed(Attr::NONINCREASING) && f_x->guaranteed(Attr::NONNEGATIVE)) return true;
        if (f_x->guaranteed(Attr::NONDECREASING) && f_x->guaranteed(Attr::NONPOSITIVE)) return true;
        return false;

    case Attr::NONDECREASING:
        if (f_x->guaranteed(Attr::NONDECREASING) && f_x->guaranteed(Attr::NONNEGATIVE)) return true;
        if (f_x->guaranteed(Attr::NONINCREASING) && f_x->guaranteed(Attr::NONPOSITIVE)) return true;
        return false;

    case Attr::BOUNDEDABOVE:
        return f_x->guaranteed(Attr::BOUNDEDABOVE) && f_x->guaranteed(Attr::BOUNDEDBELOW);
    }

    return false;
}

bool TanH::guaranteed(Attr a) const
{
    if (f_x->guaranteed(Attr::DEFINED)) switch (a)
    {
    case Attr::DEFINED:
    case Attr::UNITRANGE:
    case Attr::OPENUNITRANGE:
    case Attr::BOUNDEDABOVE:
    case Attr::BOUNDEDBELOW:
        return true;

    case Attr::NONZERO:
    case Attr::POSITIVE:
    case Attr::NEGATIVE:
    case Attr::NONPOSITIVE:
    case Attr::NONNEGATIVE:
    case Attr::CONTINUOUS:
    case Attr::INCREASING:
    case Attr::DECREASING:
    case Attr::NONINCREASING:
    case Attr::NONDECREASING:
        return f_x->guaranteed(a);
    }

    return false;
}

bool SecH::guaranteed(Attr a) const
{
    return false;
}

bool ASinH::guaranteed(Attr a) const
{
    if (f_x->guaranteed(Attr::DEFINED)) switch (a)
    {
    case Attr::DEFINED:
        return true;

    case Attr::NONZERO:
    case Attr::POSITIVE:
    case Attr::NEGATIVE:
    case Attr::NONPOSITIVE:
    case Attr::NONNEGATIVE:
    case Attr::CONTINUOUS:
    case Attr::INCREASING:
    case Attr::DECREASING:
    case Attr::NONINCREASING:
    case Attr::NONDECREASING:
    case Attr::BOUNDEDABOVE:
    case Attr::BOUNDEDBELOW:
        return f_x->guaranteed(a);
    }

    return false;
}

bool ACosH::guaranteed(Attr a) const
{
    if (f_x->guaranteed(Attr::POSITIVE) && f_x->guaranteed(Attr::ANTIOPENUNITRANGE)) switch (a)
    {
    case Attr::DEFINED:
    case Attr::NONNEGATIVE:
    case Attr::BOUNDEDBELOW:
        return true;

    case Attr::CONTINUOUS:
    case Attr::INCREASING:
    case Attr::DECREASING:
    case Attr::NONINCREASING:
    case Attr::NONDECREASING:
    case Attr::BOUNDEDABOVE:
        return f_x->guaranteed(a);

    case Attr::NONZERO:
    case Attr::POSITIVE:
        return f_x->guaranteed(Attr::ANTIUNITRANGE);
    }

    return false;
}

bool ATanH::guaranteed(Attr a) const
{
    if (f_x->guaranteed(Attr::OPENUNITRANGE)) switch (a)
    {
    case Attr::DEFINED:
        return true;

    case Attr::NONZERO:
    case Attr::POSITIVE:
    case Attr::NEGATIVE:
    case Attr::NONPOSITIVE:
    case Attr::NONNEGATIVE:
    case Attr::CONTINUOUS:
    case Attr::INCREASING:
    case Attr::DECREASING:
    case Attr::NONINCREASING:
    case Attr::NONDECREASING:
        return f_x->guaranteed(a);
    }

    return false;
}

bool Erf::guaranteed(Attr a) const
{
    if (f_x->guaranteed(Attr::DEFINED)) switch (a)
    {
    case Attr::DEFINED:
    case Attr::UNITRANGE:
    case Attr::OPENUNITRANGE:
    case Attr::BOUNDEDABOVE:
    case Attr::BOUNDEDBELOW:
        return true;

    case Attr::NONZERO:
    case Attr::POSITIVE:
    case Attr::NEGATIVE:
    case Attr::NONPOSITIVE:
    case Attr::NONNEGATIVE:
    case Attr::CONTINUOUS:
    case Attr::INCREASING:
    case Attr::DECREASING:
    case Attr::NONINCREASING:
    case Attr::NONDECREASING:
        return f_x->guaranteed(a);
    }

    return false;
}

bool ErfC::guaranteed(Attr a) const
{
    return false;
}

bool Invert::guaranteed(Attr a) const
{
    if (f_x->guaranteed(Attr::NONZERO)) switch (a)
    {
    case Attr::DEFINED:
    case Attr::NONZERO:
        return true;

    case Attr::POSITIVE:
    case Attr::NEGATIVE:
    case Attr::NONPOSITIVE:
    case Attr::NONNEGATIVE:
        return f_x->guaranteed(a);

    case Attr::UNITRANGE:
        return f_x->guaranteed(Attr::ANTIOPENUNITRANGE);

    case Attr::ANTIUNITRANGE:
        return f_x->guaranteed(Attr::OPENUNITRANGE);

    case Attr::OPENUNITRANGE:
        return f_x->guaranteed(Attr::ANTIUNITRANGE);

    case Attr::ANTIOPENUNITRANGE:
        return f_x->guaranteed(Attr::UNITRANGE);

    case Attr::CONTINUOUS:
        return f_x->guaranteed(Attr::POSITIVE) || f_x->guaranteed(Attr::NEGATIVE);

    case Attr::INCREASING:
        return f_x->guaranteed(Attr::DECREASING) && (f_x->guaranteed(Attr::POSITIVE) || f_x->guaranteed(Attr::NEGATIVE));

    case Attr::DECREASING:
        return f_x->guaranteed(Attr::INCREASING) && (f_x->guaranteed(Attr::POSITIVE) || f_x->guaranteed(Attr::NEGATIVE));

    case Attr::NONINCREASING:
        return f_x->guaranteed(Attr::NONDECREASING) && (f_x->guaranteed(Attr::POSITIVE) || f_x->guaranteed(Attr::NEGATIVE));

    case Attr::NONDECREASING:
        return f_x->guaranteed(Attr::NONINCREASING) && (f_x->guaranteed(Attr::POSITIVE) || f_x->guaranteed(Attr::NEGATIVE));
    }

    return false;
}

bool Negate::guaranteed(Attr a) const
{
    if (f_x->guaranteed(Attr::DEFINED)) switch (a)
    {
    case Attr::DEFINED:
        return true;

    case Attr::NONZERO:
    case Attr::UNITRANGE:
    case Attr::ANTIUNITRANGE:
    case Attr::OPENUNITRANGE:
    case Attr::ANTIOPENUNITRANGE:
    case Attr::CONTINUOUS:
        return f_x->guaranteed(a);

    case Attr::POSITIVE:
        return f_x->guaranteed(Attr::NEGATIVE);

    case Attr::NEGATIVE:
        return f_x->guaranteed(Attr::POSITIVE);

    case Attr::NONPOSITIVE:
        return f_x->guaranteed(Attr::NONNEGATIVE);

    case Attr::NONNEGATIVE:
        return f_x->guaranteed(Attr::NONPOSITIVE);

    case Attr::INCREASING:
        return f_x->guaranteed(Attr::DECREASING);

    case Attr::DECREASING:
        return f_x->guaranteed(Attr::INCREASING);

    case Attr::NONINCREASING:
        return f_x->guaranteed(Attr::NONDECREASING);

    case Attr::NONDECREASING:
        return f_x->guaranteed(Attr::NONINCREASING);

    case Attr::BOUNDEDABOVE:
        return f_x->guaranteed(Attr::BOUNDEDBELOW);

    case Attr::BOUNDEDBELOW:
        return f_x->guaranteed(Attr::BOUNDEDABOVE);
    }

    return false;
}

bool SoftPP::guaranteed(Attr a) const
{
    if (f_x->guaranteed(Attr::DEFINED)) switch (a)
    {
    case Attr::DEFINED:
    case Attr::NONZERO:
    case Attr::POSITIVE:
    case Attr::NEGATIVE:
    case Attr::NONPOSITIVE:
    case Attr::NONNEGATIVE:
    case Attr::UNITRANGE:
    case Attr::ANTIUNITRANGE:
    case Attr::OPENUNITRANGE:
    case Attr::ANTIOPENUNITRANGE:
    case Attr::CONTINUOUS:
    case Attr::INCREASING:
    case Attr::DECREASING:
    case Attr::NONINCREASING:
    case Attr::NONDECREASING:
    case Attr::BOUNDEDABOVE:
    case Attr::BOUNDEDBELOW:
        ;
    }

    return false;
}

bool Spence::guaranteed(Attr a) const
{
    if (f_x->guaranteed(Attr::UNITRANGE) || f_x->guaranteed(Attr::NEGATIVE)) switch (a)
    {
    case Attr::DEFINED:
    case Attr::BOUNDEDABOVE:
        return true;

    case Attr::NONZERO:
    case Attr::POSITIVE:
    case Attr::NEGATIVE:
    case Attr::NONPOSITIVE:
    case Attr::NONNEGATIVE:
    case Attr::CONTINUOUS:
    case Attr::INCREASING:
    case Attr::DECREASING:
    case Attr::NONINCREASING:
    case Attr::NONDECREASING:
    case Attr::BOUNDEDBELOW:
        return f_x->guaranteed(a);
    }

    return false;
}

bool Square::guaranteed(Attr a) const
{
    if (f_x->guaranteed(Attr::DEFINED)) switch (a)
    {
    case Attr::DEFINED:
    case Attr::NONNEGATIVE:
    case Attr::BOUNDEDBELOW:
        return true;

    case Attr::NONZERO:
    case Attr::UNITRANGE:
    case Attr::ANTIUNITRANGE:
    case Attr::OPENUNITRANGE:
    case Attr::ANTIOPENUNITRANGE:
    case Attr::CONTINUOUS:
        return f_x->guaranteed(a);

    case Attr::POSITIVE:
        return f_x->guaranteed(Attr::NONZERO);

    case Attr::INCREASING:
        if (f_x->guaranteed(Attr::INCREASING) && f_x->guaranteed(Attr::POSITIVE)) return true;
        if (f_x->guaranteed(Attr::DECREASING) && f_x->guaranteed(Attr::NEGATIVE)) return true;
        return false;

    case Attr::DECREASING:
        if (f_x->guaranteed(Attr::DECREASING) && f_x->guaranteed(Attr::POSITIVE)) return true;
        if (f_x->guaranteed(Attr::INCREASING) && f_x->guaranteed(Attr::NEGATIVE)) return true;
        return false;

    case Attr::NONINCREASING:
        if (f_x->guaranteed(Attr::NONINCREASING) && f_x->guaranteed(Attr::NONNEGATIVE)) return true;
        if (f_x->guaranteed(Attr::NONDECREASING) && f_x->guaranteed(Attr::NONPOSITIVE)) return true;
        return false;

    case Attr::NONDECREASING:
        if (f_x->guaranteed(Attr::NONDECREASING) && f_x->guaranteed(Attr::NONNEGATIVE)) return true;
        if (f_x->guaranteed(Attr::NONINCREASING) && f_x->guaranteed(Attr::NONPOSITIVE)) return true;
        return false;

    case Attr::BOUNDEDABOVE:
        return f_x->guaranteed(Attr::BOUNDEDABOVE) && f_x->guaranteed(Attr::BOUNDEDBELOW);
    }

    return false;
}

bool XConic::guaranteed(Attr a) const
{
    if (f_x->guaranteed(Attr::POSITIVE) && f_x->guaranteed(Attr::ANTIOPENUNITRANGE)) switch (a)
    {
    case Attr::DEFINED:
    case Attr::NONNEGATIVE:
    case Attr::BOUNDEDBELOW:
        return true;

    case Attr::CONTINUOUS:
    case Attr::INCREASING:
    case Attr::DECREASING:
    case Attr::NONINCREASING:
    case Attr::NONDECREASING:
    case Attr::BOUNDEDABOVE:
        return f_x->guaranteed(a);

    case Attr::NONZERO:
    case Attr::POSITIVE:
        return f_x->guaranteed(Attr::ANTIUNITRANGE);
    }

    return false;
}

bool YConic::guaranteed(Attr a) const
{
    if (f_x->guaranteed(Attr::DEFINED)) switch (a)
    {
    case Attr::DEFINED:
    case Attr::NONZERO:
    case Attr::POSITIVE:
    case Attr::NONNEGATIVE:
    case Attr::ANTIOPENUNITRANGE:
    case Attr::BOUNDEDBELOW:
        return true;

    case Attr::CONTINUOUS:
        return f_x->guaranteed(a);

    case Attr::ANTIUNITRANGE:
        return f_x->guaranteed(Attr::NONZERO);

    case Attr::INCREASING:
        if (f_x->guaranteed(Attr::INCREASING) && f_x->guaranteed(Attr::POSITIVE)) return true;
        if (f_x->guaranteed(Attr::DECREASING) && f_x->guaranteed(Attr::NEGATIVE)) return true;
        return false;

    case Attr::DECREASING:
        if (f_x->guaranteed(Attr::DECREASING) && f_x->guaranteed(Attr::POSITIVE)) return true;
        if (f_x->guaranteed(Attr::INCREASING) && f_x->guaranteed(Attr::NEGATIVE)) return true;
        return false;

    case Attr::NONINCREASING:
        if (f_x->guaranteed(Attr::NONINCREASING) && f_x->guaranteed(Attr::NONNEGATIVE)) return true;
        if (f_x->guaranteed(Attr::NONDECREASING) && f_x->guaranteed(Attr::NONPOSITIVE)) return true;
        return false;

    case Attr::NONDECREASING:
        if (f_x->guaranteed(Attr::NONDECREASING) && f_x->guaranteed(Attr::NONNEGATIVE)) return true;
        if (f_x->guaranteed(Attr::NONINCREASING) && f_x->guaranteed(Attr::NONPOSITIVE)) return true;
        return false;

    case Attr::BOUNDEDABOVE:
        return f_x->guaranteed(Attr::BOUNDEDABOVE) && f_x->guaranteed(Attr::BOUNDEDBELOW);
    }

    return false;
}

bool ZConic::guaranteed(Attr a) const
{
    if (f_x->guaranteed(Attr::UNITRANGE)) switch (a)
    {
    case Attr::DEFINED:
    case Attr::NONNEGATIVE:
    case Attr::UNITRANGE:
    case Attr::BOUNDEDABOVE:
    case Attr::BOUNDEDBELOW:
        return true;

    case Attr::CONTINUOUS:
        return f_x->guaranteed(a);

    case Attr::NONZERO:
    case Attr::POSITIVE:
        return f_x->guaranteed(Attr::OPENUNITRANGE);

    case Attr::NONPOSITIVE:
        return f_x->guaranteed(Attr::ANTIOPENUNITRANGE);

    case Attr::OPENUNITRANGE:
        return f_x->guaranteed(Attr::NONZERO);

    case Attr::INCREASING:
        if (f_x->guaranteed(Attr::INCREASING) && f_x->guaranteed(Attr::NEGATIVE)) return true;
        if (f_x->guaranteed(Attr::DECREASING) && f_x->guaranteed(Attr::POSITIVE)) return true;
        return false;

    case Attr::DECREASING:
        if (f_x->guaranteed(Attr::DECREASING) && f_x->guaranteed(Attr::NEGATIVE)) return true;
        if (f_x->guaranteed(Attr::INCREASING) && f_x->guaranteed(Attr::POSITIVE)) return true;
        return false;

    case Attr::NONINCREASING:
        if (f_x->guaranteed(Attr::NONINCREASING) && f_x->guaranteed(Attr::NONPOSITIVE)) return true;
        if (f_x->guaranteed(Attr::NONDECREASING) && f_x->guaranteed(Attr::NONNEGATIVE)) return true;
        return false;

    case Attr::NONDECREASING:
        if (f_x->guaranteed(Attr::NONDECREASING) && f_x->guaranteed(Attr::NONPOSITIVE)) return true;
        if (f_x->guaranteed(Attr::NONINCREASING) && f_x->guaranteed(Attr::NONNEGATIVE)) return true;
        return false;
    }

    return false;
}

bool Add::guaranteed(Attr a) const
{
    if (f_x->guaranteed(Attr::DEFINED) && g_x->guaranteed(Attr::DEFINED)) switch (a)
    {
    case Attr::DEFINED:
        return true;

    case Attr::NONZERO:
        if (f_x->guaranteed(Attr::POSITIVE) && g_x->guaranteed(Attr::NONNEGATIVE)) return true;
        if (f_x->guaranteed(Attr::NEGATIVE) && g_x->guaranteed(Attr::NONPOSITIVE)) return true;
        if (f_x->guaranteed(Attr::NONPOSITIVE) && g_x->guaranteed(Attr::NEGATIVE)) return true;
        if (f_x->guaranteed(Attr::NONNEGATIVE) && g_x->guaranteed(Attr::POSITIVE)) return true;
        return false;

    case Attr::POSITIVE:
        if (f_x->guaranteed(Attr::POSITIVE) && g_x->guaranteed(Attr::NONNEGATIVE)) return true;
        if (f_x->guaranteed(Attr::NONNEGATIVE) && g_x->guaranteed(Attr::POSITIVE)) return true;
        return false;

    case Attr::NEGATIVE:
        if (f_x->guaranteed(Attr::NEGATIVE) && g_x->guaranteed(Attr::NONPOSITIVE)) return true;
        if (f_x->guaranteed(Attr::NONPOSITIVE) && g_x->guaranteed(Attr::NEGATIVE)) return true;
        return false;

    case Attr::NONPOSITIVE:
        return f_x->guaranteed(a) && g_x->guaranteed(a);

    case Attr::NONNEGATIVE:
        return f_x->guaranteed(a) && g_x->guaranteed(a);

    case Attr::UNITRANGE:
        break;

    case Attr::ANTIUNITRANGE:
        break;

    case Attr::OPENUNITRANGE:
        break;

    case Attr::ANTIOPENUNITRANGE:
        break;

    case Attr::CONTINUOUS:
        return f_x->guaranteed(a) && g_x->guaranteed(a);

    case Attr::INCREASING:
        if (f_x->guaranteed(Attr::INCREASING) && g_x->guaranteed(Attr::NONDECREASING)) return true;
        if (f_x->guaranteed(Attr::NONDECREASING) && g_x->guaranteed(Attr::INCREASING)) return true;
        return false;

    case Attr::DECREASING:
        if (f_x->guaranteed(Attr::DECREASING) && g_x->guaranteed(Attr::NONINCREASING)) return true;
        if (f_x->guaranteed(Attr::NONINCREASING) && g_x->guaranteed(Attr::DECREASING)) return true;
        return false;

    case Attr::NONINCREASING:
        return f_x->guaranteed(a) && g_x->guaranteed(a);

    case Attr::NONDECREASING:
        return f_x->guaranteed(a) && g_x->guaranteed(a);

    case Attr::BOUNDEDABOVE:
        return f_x->guaranteed(a) && g_x->guaranteed(a);

    case Attr::BOUNDEDBELOW:
        return f_x->guaranteed(a) && g_x->guaranteed(a);
    }

    return false;
}

bool Mul::guaranteed(Attr a) const
{
    if (f_x->guaranteed(Attr::DEFINED) && g_x->guaranteed(Attr::DEFINED)) switch (a)
    {
    case Attr::DEFINED:
        return true;

    case Attr::NONZERO:
        return f_x->guaranteed(a) && g_x->guaranteed(a);

    case Attr::POSITIVE:
        if (f_x->guaranteed(Attr::POSITIVE) && g_x->guaranteed(Attr::POSITIVE)) return true;
        if (f_x->guaranteed(Attr::NEGATIVE) && g_x->guaranteed(Attr::NEGATIVE)) return true;
        return false;

    case Attr::NEGATIVE:
        if (f_x->guaranteed(Attr::POSITIVE) && g_x->guaranteed(Attr::NEGATIVE)) return true;
        if (f_x->guaranteed(Attr::NEGATIVE) && g_x->guaranteed(Attr::POSITIVE)) return true;
        return false;

    case Attr::NONPOSITIVE:
        break;

    case Attr::NONNEGATIVE:
        break;

    case Attr::UNITRANGE:
        return f_x->guaranteed(a) && g_x->guaranteed(a);

    case Attr::ANTIUNITRANGE:
        return f_x->guaranteed(a) && g_x->guaranteed(a);

    case Attr::OPENUNITRANGE:
        return f_x->guaranteed(a) && g_x->guaranteed(a);

    case Attr::ANTIOPENUNITRANGE:
        return f_x->guaranteed(a) && g_x->guaranteed(a);

    case Attr::CONTINUOUS:
        return f_x->guaranteed(a) && g_x->guaranteed(a);

    case Attr::INCREASING:
        break;

    case Attr::DECREASING:
        break;

    case Attr::NONINCREASING:
        break;

    case Attr::NONDECREASING:
        break;

    case Attr::BOUNDEDABOVE:
        break;

    case Attr::BOUNDEDBELOW:
        break;
    }

    return false;
}

bool Pow::guaranteed(Attr a) const
{
    if (f_x->guaranteed(Attr::POSITIVE) && g_x->guaranteed(Attr::DEFINED)) switch (a)
    {
    case Attr::DEFINED:
    case Attr::NONZERO:
    case Attr::POSITIVE:
    case Attr::NONNEGATIVE:
        return true;

    case Attr::UNITRANGE:
        break;

    case Attr::ANTIUNITRANGE:
        break;

    case Attr::OPENUNITRANGE:
        break;

    case Attr::ANTIOPENUNITRANGE:
        break;

    case Attr::CONTINUOUS:
        return f_x->guaranteed(a) && g_x->guaranteed(a);

    case Attr::INCREASING:
        break;

    case Attr::DECREASING:
        break;

    case Attr::NONINCREASING:
        break;

    case Attr::NONDECREASING:
        break;

    case Attr::BOUNDEDABOVE:
        break;

    case Attr::BOUNDEDBELOW:
        break;
    }

    return false;
}

/***********************************************************************************************************************
*** print()
***********************************************************************************************************************/

void ConstantNode::print(std::ostream& out) const
{
    out << n;
}

void VariableNode::print(std::ostream& out) const
{
    out << x.Name();
}

void Abs::print(std::ostream& out) const
{
    out << "abs(";
    f_x->print(out);
    out << ")";
}

void Sgn::print(std::ostream& out) const
{
    out << "sgn(";
    f_x->print(out);
    out << ")";
}

void Sqrt::print(std::ostream& out) const
{
    out << "sqrt(";
    f_x->print(out);
    out << ")";
}

void Cbrt::print(std::ostream& out) const
{
    out << "cbrt(";
    f_x->print(out);
    out << ")";
}

void Exp::print(std::ostream& out) const
{
    out << "exp(";
    f_x->print(out);
    out << ")";
}

void ExpM1::print(std::ostream& out) const
{
    out << "expm1(";
    f_x->print(out);
    out << ")";
}

void Log::print(std::ostream& out) const
{
    out << "log(";
    f_x->print(out);
    out << ")";
}

void Log1P::print(std::ostream& out) const
{
    out << "log1p(";
    f_x->print(out);
    out << ")";
}

void Sin::print(std::ostream& out) const
{
    out << "sin(";
    f_x->print(out);
    out << ")";
}

void Cos::print(std::ostream& out) const
{
    out << "cos(";
    f_x->print(out);
    out << ")";
}

void Tan::print(std::ostream& out) const
{
    out << "tan(";
    f_x->print(out);
    out << ")";
}

void Sec::print(std::ostream& out) const
{
    out << "sec(";
    f_x->print(out);
    out << ")";
}

void ASin::print(std::ostream& out) const
{
    out << "asin(";
    f_x->print(out);
    out << ")";
}

void ACos::print(std::ostream& out) const
{
    out << "acos(";
    f_x->print(out);
    out << ")";
}

void ATan::print(std::ostream& out) const
{
    out << "atan(";
    f_x->print(out);
    out << ")";
}

void SinH::print(std::ostream& out) const
{
    out << "sinh(";
    f_x->print(out);
    out << ")";
}

void CosH::print(std::ostream& out) const
{
    out << "cosh(";
    f_x->print(out);
    out << ")";
}

void TanH::print(std::ostream& out) const
{
    out << "tanh(";
    f_x->print(out);
    out << ")";
}

void SecH::print(std::ostream& out) const
{
    out << "sech(";
    f_x->print(out);
    out << ")";
}

void ASinH::print(std::ostream& out) const
{
    out << "asinh(";
    f_x->print(out);
    out << ")";
}

void ACosH::print(std::ostream& out) const
{
    out << "acosh(";
    f_x->print(out);
    out << ")";
}

void ATanH::print(std::ostream& out) const
{
    out << "atanh(";
    f_x->print(out);
    out << ")";
}

void Erf::print(std::ostream& out) const
{
    out << "erf(";
    f_x->print(out);
    out << ")";
}

void ErfC::print(std::ostream& out) const
{
    out << "erfc(";
    f_x->print(out);
    out << ")";
}

void Invert::print(std::ostream& out) const
{
    out << "1/(";
    f_x->print(out);
    out << ")";
}

void Negate::print(std::ostream& out) const
{
    out << "-";
    if (f_x->is(NodeType::ADD)) out << "(";
    f_x->print(out);
    if (f_x->is(NodeType::ADD)) out << ")";
}

void SoftPP::print(std::ostream& out) const
{
    out << "softpp(";
    f_x->print(out);
    out << ")";
}

void Spence::print(std::ostream& out) const
{
    out << "Li2(";
    f_x->print(out);
    out << ")";
}

void Square::print(std::ostream& out) const
{
    if (f_x->is(NodeType::ADD) || f_x->is(NodeType::MUL)) out << "(";
    f_x->print(out);
    if (f_x->is(NodeType::ADD) || f_x->is(NodeType::MUL)) out << ")";
    out << "^2";
}

void XConic::print(std::ostream& out) const
{
    out << "xconic(";
    f_x->print(out);
    out << ")";
}

void YConic::print(std::ostream& out) const
{
    out << "yconic(";
    f_x->print(out);
    out << ")";
}

void ZConic::print(std::ostream& out) const
{
    out << "zconic(";
    f_x->print(out);
    out << ")";
}

void Add::print(std::ostream& out) const
{
    f_x->print(out);
    out << "+";
    g_x->print(out);
}

void Mul::print(std::ostream& out) const
{
    if (f_x->is(NodeType::ADD) || f_x->is(NodeType::POW)) out << "(";
    f_x->print(out);
    if (f_x->is(NodeType::ADD) || f_x->is(NodeType::POW)) out << ")";
    out << "*";
    if (g_x->is(NodeType::ADD) || g_x->is(NodeType::POW)) out << "(";
    g_x->print(out);
    if (g_x->is(NodeType::ADD) || g_x->is(NodeType::POW)) out << ")";
}

void Pow::print(std::ostream& out) const
{
    if (f_x->is(NodeType::ADD) || f_x->is(NodeType::MUL) || f_x->is(NodeType::POW)) out << "(";
    f_x->print(out);
    if (f_x->is(NodeType::ADD) || f_x->is(NodeType::MUL) || f_x->is(NodeType::POW)) out << ")";
    out << "^";
    if (g_x->is(NodeType::ADD) || g_x->is(NodeType::MUL) || g_x->is(NodeType::POW)) out << "(";
    g_x->print(out);
    if (g_x->is(NodeType::ADD) || g_x->is(NodeType::MUL) || g_x->is(NodeType::POW)) out << ")";
}

/***********************************************************************************************************************
*** Expression
***********************************************************************************************************************/

Expression::Expression() : pData(Shared::Clone(Nan::instance))
{
}

Expression::Expression(Expression const& r) : pData(Shared::Clone(r.pData))
{
}

Expression::Expression(Variable const& r) : pData(data::variable(r))
{
}

Expression::Expression(double d) : pData(data::constant(d))
{
}

Expression::Expression(int i) : pData(data::constant(i))
{
}

Expression::Expression(data const* p) : pData(p)
{
}

Expression::~Expression()
{
    Shared::Erase(pData);
}

Expression& Expression::operator=(Expression const& r) noexcept
{
    Shared::Clone(r.pData);
    Shared::Erase(pData);
    pData = r.pData;
    return *this;
}

//----------------------------------------------------------------------------------------------------------------------

Expression abs(Expression const& r)
{
    return r.pData->abs();
}

Expression sgn(Expression const& r)
{
    return r.pData->sgn();
}

Expression sqrt(Expression const& r)
{
    return r.pData->sqrt();
}

Expression cbrt(Expression const& r)
{
    return r.pData->cbrt();
}

Expression exp(Expression const& r)
{
    return r.pData->exp();
}

Expression expm1(Expression const& r)
{
    return r.pData->expm1();
}

Expression log(Expression const& r)
{
    return r.pData->log();
}

Expression log1p(Expression const& r)
{
    return r.pData->log1p();
}

Expression sin(Expression const& r)
{
    return r.pData->sin();
}

Expression cos(Expression const& r)
{
    return r.pData->cos();
}

Expression tan(Expression const& r)
{
    return r.pData->tan();
}

Expression asin(Expression const& r)
{
    return r.pData->asin();
}

Expression acos(Expression const& r)
{
    return r.pData->acos();
}

Expression atan(Expression const& r)
{
    return r.pData->atan();
}

Expression sinh(Expression const& r)
{
    return r.pData->sinh();
}

Expression cosh(Expression const& r)
{
    return r.pData->cosh();
}

Expression tanh(Expression const& r)
{
    return r.pData->tanh();
}

Expression asinh(Expression const& r)
{
    return r.pData->asinh();
}

Expression acosh(Expression const& r)
{
    return r.pData->acosh();
}

Expression atanh(Expression const& r)
{
    return r.pData->atanh();
}

Expression erf(Expression const& r)
{
    return r.pData->erf();
}

Expression erfc(Expression const& r)
{
    return r.pData->erfc();
}

Expression Li2(Expression const& r)
{
    return r.pData->spence();
}

Expression Spp(Expression const& r)
{
    return r.pData->softpp();
}

Expression operator+(Expression const& r)
{
    return r;
}

Expression operator-(Expression const& r)
{
    return r.pData->negate();
}

Expression operator+(Expression const& r, Expression const& s)
{
    return r.pData->add(s.pData);
}

Expression operator-(Expression const& r, Expression const& s)
{
    // f_x-g_x = f_x + -g_x

    auto step0 = s.pData->negate();
    auto step1 = r.pData->add(step0);

    Shared::Erase(step0);

    return step1;
}

Expression operator*(Expression const& r, Expression const& s)
{
    return r.pData->mul(s.pData);
}

Expression operator/(Expression const& r, Expression const& s)
{
    // f_x/g_x = f_x * 1/g_x

    auto step0 = s.pData->invert();
    auto step1 = r.pData->mul(step0);

    Shared::Erase(step0);

    return step1;
}

double Expression::operator()() const noexcept
{
    return pData->evaluate();
}

Expression::operator double() const
{
    return pData->evaluate();
}

Expression pow(Expression const& r, Expression const& s)
{
    return r.pData->pow(s.pData);
}

void AtomicAssign(Bindings& r)
{
    auto const N = r.size();
    double* p = new double[N];

    for (int i = 0; i < N; ++i) p[i] = r[i].second();
    for (int i = 0; i < N; ++i) r[i].first = p[i];

    delete[] p;
}

Expression Expression::AtomicBind(Bindings const& r) const
{
    std::vector<std::pair<Variable, Expression::data const*> > t;
    for (auto& s : r) t.emplace_back(s.first, s.second.pData);
    return pData->bind(t);
}

Expression Expression::Bind(Variable const& r, double d) const
{
    Expression s(d);
    std::vector<std::pair<Variable, Expression::data const*>> t;
    t.emplace_back(r, s.pData);
    return pData->bind(t);
}

Expression Expression::Derive(Variable const& r) const
{
    auto result = pData->derive(r);
    pData->purge();
    return result;
}

double Expression::Evaluate() const
{
    return pData->evaluate();
}

bool Expression::Guaranteed(Attribute a) const
{
    return pData->guaranteed(a);
}

void Expression::Touch()
{
    ++Expr::dirtyLevel;
}

int32_t Expression::Depth() const noexcept
{
    return pData->depth;
}

std::ostream& operator<<(std::ostream& r, Expression const& s)
{
    s.pData->print(r);
    return r;
}

/***********************************************************************************************************************
*** Variable::data
***********************************************************************************************************************/

struct Variable::data : public Shared
{
    data(double d) : value(d), name("[&" + std::to_string(size_t(this) / sizeof(*this)) + "]") { }

    mutable double value;
    mutable std::string name;
};

/***********************************************************************************************************************
*** Variable
***********************************************************************************************************************/

Variable::Variable(double d) : pData(new data(d))
{
}

Variable::Variable(Variable const& r) noexcept : pData(Shared::Clone(r.pData))
{
}

Variable::~Variable() noexcept
{
    Shared::Erase(pData);
}

Variable& Variable::operator=(double d)
{
    assert(!isinf(d));
    assert(!isnan(d));

    pData->value = d;

    Expression::Touch();

    return *this;
}

Variable::operator double() const noexcept
{
    return pData->value;
}

double Variable::operator()() const noexcept
{
    return pData->value;
}

std::string Variable::Name() const
{
    return pData->name;
}

void Variable::Name(std::string const& s)
{
    pData->name = s;
}

size_t Variable::id() const
{
    return size_t(pData);
}

/***********************************************************************************************************************
*** Additional functions
***********************************************************************************************************************/

static inline double sq(double d) { return d * d; }
static double const PiPiDiv6 = 1.64493406684822644e-00;

static double bernoulli(double x)
{
    assert(abs(x) <= log(2));

    double const x2(x * x);
    double power[8];

    power[0] = x2 * x;
    power[1] = x2 * power[0];
    power[2] = x2 * power[1];
    power[3] = x2 * power[2];
    power[4] = x2 * power[3];
    power[5] = x2 * power[4];
    power[6] = x2 * power[5];
    power[7] = x2 * power[6];

    double total;

    total = -power[7] * 1.99392958607210757e-14;
    total += power[6] * 8.92169102045645256e-13;
    total -= power[5] * 4.06476164514422553e-11;
    total += power[4] * 1.89788699889709991e-09;
    total -= power[3] * 9.18577307466196355e-08;
    total += power[2] * 4.72411186696900983e-06;
    total -= power[1] * 2.77777777777777778e-04;
    total += power[0] * 2.77777777777777778e-02;

    return total - x2 / 4 + x;
}

double Li2(double x)
{
    if (x < -1) return -Li2(1 / x) - PiPiDiv6 - sq(log(-x)) / 2;
    if (x <= 0.5) return bernoulli(-log1p(-x));
    if (x < 1) return -Li2(1 - x) + PiPiDiv6 - log(x) * log1p(-x);
    if (x == 1) return PiPiDiv6;
    return nan(__FUNCTION__);  // Reals only!
}

double Spp(double x)
{
    if (x > 0) return x * x / 2 - Spp(-x) + PiPiDiv6;
    return -bernoulli(-log1p(exp(x)));
}

//**********************************************************************************************************************
