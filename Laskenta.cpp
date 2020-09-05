
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

#include "Laskenta.h"

// #define VERBOSE
#include "Tools.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <unordered_map>

using std::cout;
using std::endl;

#define OPTIMIZATION PROFILER

//**********************************************************************************************************************

struct ConstantNode;

/***********************************************************************************************************************
*** Expression::data
***********************************************************************************************************************/

struct Expression::data : public Shared
{
    // Primitives

    static Expression::data const* constant(double);
    static Expression::data const* variable(Variable const&);

    // Functions

    virtual Expression::data const* abs() const;
    virtual Expression::data const* sign() const;
    virtual Expression::data const* sqrt() const;
    virtual Expression::data const* cbrt() const;
    virtual Expression::data const* exp() const;
    virtual Expression::data const* log() const;
    virtual Expression::data const* sin() const;
    virtual Expression::data const* cos() const;
    virtual Expression::data const* tan() const;
    virtual Expression::data const* sec() const;
    virtual Expression::data const* asin() const;
    virtual Expression::data const* acos() const;
    virtual Expression::data const* atan() const;
    virtual Expression::data const* sinh() const;
    virtual Expression::data const* cosh() const;
    virtual Expression::data const* tanh() const;
    virtual Expression::data const* asinh() const;
    virtual Expression::data const* acosh() const;
    virtual Expression::data const* atanh() const;
    virtual Expression::data const* erf() const;
    virtual Expression::data const* invert() const;
    virtual Expression::data const* negate() const;
    virtual Expression::data const* square() const;
    virtual Expression::data const* xconic() const;
    virtual Expression::data const* yconic() const;
    virtual Expression::data const* zconic() const;

    // Operators

    virtual Expression::data const* add(Expression::data const*) const;
    virtual Expression::data const* commutative_add(Expression::data const*) const;
    virtual Expression::data const* mul(Expression::data const*) const;
    virtual Expression::data const* commutative_mul(Expression::data const*) const;
    virtual Expression::data const* pow(Expression::data const*) const;

    // Evaluation and derivation

    Expression::data const* derive(Variable const& r) const { return Clone(cachedNode ? cachedNode : cachedNode = derivative(r)); }
    double evaluate() const { if (cleanLevel != dirtyLevel) { valueCache = value(); cleanLevel = dirtyLevel; } return valueCache; }

    // Analysis tools

    virtual bool guaranteed(Expression::Attribute) const = 0; // { return false; }

    enum class NodeType
    {
        ABS, SIGN, SQRT, CBRT, EXP, LOG, SIN, COS, TAN, SEC, ASIN, ACOS, ATAN, SINH, COSH, TANH, ASINH, ACOSH, ATANH, ERF,
        INVERT, NEGATE, SQUARE, XCONIC, YCONIC, ZCONIC, CONSTANT, VARIABLE, ADD, MUL, POW
    };

    virtual bool is(NodeType) const = 0;
    virtual bool is(NodeType, Expression::data const*) const = 0;

    virtual bool easyInvert() const { return false; }
    virtual bool easyNegate() const { return false; }

    // Cache management

    mutable std::map<NodeType, Expression::data const*> functionNode;
    mutable std::map<Expression::data const*, Expression::data const*> addNode;
    mutable std::map<Expression::data const*, Expression::data const*> mulNode;
    mutable std::map<Expression::data const*, Expression::data const*> powNode;

    // Miscellaneous

    int32_t const depth;
    static size_t dirtyLevel;
    mutable Expression::data const* cachedNode;
    virtual void purge() const { if (cachedNode) { Erase(cachedNode); cachedNode = nullptr; } }

    operator Expression::data const* () const;

    virtual void print(std::ostream& r) const = 0;

protected:
    explicit data(int32_t n) : depth(n), cachedNode(nullptr), cleanLevel(0), valueCache(0) { }
    virtual ~data() { }

    static std::unordered_map<double, Expression::data const*> constantNode;
    static std::unordered_map<Variable::data const*, Expression::data const*> variableNode;

    static ConstantNode const literal0;
    static ConstantNode const literal1;
    static ConstantNode const literal2;
    static ConstantNode const literal3;
    static ConstantNode const literal2Inv;
    static ConstantNode const literal3Inv;
    static ConstantNode const literal1Neg;
    static ConstantNode const erfDiffConst;

private:
    mutable size_t cleanLevel;
    mutable double valueCache;

    virtual Expression::data const* derivative(Variable const&) const = 0;
    virtual double value() const = 0;

    Expression::data const* function(NodeType n) const;

    void* operator new(size_t n) { return ::operator new(n); }
};

//----------------------------------------------------------------------------------------------------------------------

Expression::data::operator Expression::data const* () const
{
    return this;
}

//----------------------------------------------------------------------------------------------------------------------

size_t Expression::data::dirtyLevel = 1LL;
std::unordered_map<double, Expression::data const*> Expression::data::constantNode;
std::unordered_map<Variable::data const*, Expression::data const*> Expression::data::variableNode;

/***********************************************************************************************************************
*** FunctionNode
***********************************************************************************************************************/

struct FunctionNode : public Expression::data
{
    FunctionNode(Expression::data const*, NodeType);
    virtual ~FunctionNode();

    bool is(NodeType) const override final;
    bool is(NodeType, Expression::data const*) const override final;

    void purge() const override final;

protected:
    Expression::data const* const f_x;

private:
    NodeType const fn;
};

//----------------------------------------------------------------------------------------------------------------------

FunctionNode::FunctionNode(Expression::data const* p, NodeType n) : Expression::data(p->depth + 1), f_x(p), fn(n)
{
    assert(f_x->functionNode.find(fn) == f_x->functionNode.end());

    f_x->functionNode[fn] = this;
}

FunctionNode::~FunctionNode()
{
    assert(f_x->functionNode.find(fn) != f_x->functionNode.end() && f_x->functionNode[fn] == this);

    f_x->functionNode.erase(fn);
    Erase(f_x);
}

bool FunctionNode::is(NodeType t) const
{
    return t == fn;
}

bool FunctionNode::is(NodeType t, Expression::data const* p) const
{
    return t == fn && p == f_x;
}

void FunctionNode::purge() const
{
    if (cachedNode)
    {
        Expression::data::purge();
        f_x->purge();
    }
}

/***********************************************************************************************************************
*** OperatorNode
***********************************************************************************************************************/

struct OperatorNode : public Expression::data
{
    OperatorNode(Expression::data const* p, Expression::data const* q) : Expression::data(std::max(p->depth, q->depth) + 1), f_x(p), g_x(q) { }
    virtual ~OperatorNode();

    bool is(NodeType, Expression::data const*) const override final;

    void purge() const override final;

protected:
    Expression::data const* const f_x;
    Expression::data const* const g_x;
};

//----------------------------------------------------------------------------------------------------------------------

OperatorNode::~OperatorNode()
{
    Erase(f_x);
    Erase(g_x);
}

bool OperatorNode::is(NodeType t, Expression::data const* p) const
{
    return false;
}

void OperatorNode::purge() const
{
    if (cachedNode)
    {
        Expression::data::purge();
        f_x->purge();
        g_x->purge();
    }
}

/***********************************************************************************************************************
*** NullExpression
***********************************************************************************************************************/

struct NullExpression final : public Expression::data
{
    static NullExpression const instance;

    // Empty expression starts in 'quantum state' -- It behaves like '0' if added anything or '1' if multiplied by anything:

    Expression::data const* add(Expression::data const* p) const override final { return Clone(p); }
    Expression::data const* mul(Expression::data const* p) const override final { return Clone(p); }

    bool guaranteed(Expression::Attribute) const override final { return false; }

    bool is(NodeType) const override final { return false; }
    bool is(NodeType, Expression::data const*) const override final { return false; }

    Expression::data const* derivative(Variable const&) const override final { return Clone(this); }
    double value() const override final { FAIL("Attempt was made to evaluate uninitialized Expression object"); }

    void print(std::ostream& out) const override final {}

private:
    NullExpression() : Expression::data(0) { }
};

//----------------------------------------------------------------------------------------------------------------------

NullExpression const NullExpression::instance;

/***********************************************************************************************************************
*** ConstantNode
***********************************************************************************************************************/

struct ConstantNode final : public Expression::data, private ObjectGuard<ConstantNode>
{
    explicit ConstantNode(double);
    virtual ~ConstantNode();

    Expression::data const* abs() const override final;
    Expression::data const* sign() const override final;
    Expression::data const* sqrt() const override final;
    Expression::data const* cbrt() const override final;
    Expression::data const* exp() const override final;
    Expression::data const* log() const override final;
    Expression::data const* sin() const override final;
    Expression::data const* cos() const override final;
    Expression::data const* tan() const override final;
    Expression::data const* sec() const override final;
    Expression::data const* asin() const override final;
    Expression::data const* acos() const override final;
    Expression::data const* atan() const override final;
    Expression::data const* sinh() const override final;
    Expression::data const* cosh() const override final;
    Expression::data const* tanh() const override final;
    Expression::data const* asinh() const override final;
    Expression::data const* acosh() const override final;
    Expression::data const* atanh() const override final;
    Expression::data const* erf() const override final;
    Expression::data const* invert() const override final;
    Expression::data const* negate() const override final;
    Expression::data const* square() const override final;
    Expression::data const* xconic() const override final;
    Expression::data const* yconic() const override final;
    Expression::data const* zconic() const override final;

    Expression::data const* add(Expression::data const*) const override final;
    Expression::data const* commutative_add(Expression::data const*) const override final;
    Expression::data const* mul(Expression::data const*) const override final;
    Expression::data const* commutative_mul(Expression::data const*) const override final;
    Expression::data const* pow(Expression::data const*) const override final;

    bool guaranteed(Expression::Attribute) const override final;

    bool is(NodeType t) const override final { return t == NodeType::CONSTANT; }
    bool is(NodeType t, Expression::data const* p) const override final { return t == NodeType::CONSTANT && p == this; }

    bool easyInvert() const override final { return n != 0; }
    bool easyNegate() const override final { return true; }

    Expression::data const* derivative(Variable const&) const override final;
    double value() const override final;

    void print(std::ostream&) const override final;

private:
    double const n;
};

//----------------------------------------------------------------------------------------------------------------------

ConstantNode const Expression::data::literal0(0);
ConstantNode const Expression::data::literal1(1);
ConstantNode const Expression::data::literal3(3);
ConstantNode const Expression::data::literal2Inv(1.0 / 2);
ConstantNode const Expression::data::literal3Inv(1.0 / 3);
ConstantNode const Expression::data::literal1Neg(-1);
ConstantNode const Expression::data::erfDiffConst(1.0 / std::sqrt(std::atan(1)));

//----------------------------------------------------------------------------------------------------------------------

ConstantNode::ConstantNode(double d) : Expression::data(0), n(d)
{
    assert(constantNode.find(n) == constantNode.end());

    constantNode[n] = this;
}

ConstantNode::~ConstantNode()
{
    assert(constantNode.find(n) != constantNode.end() && constantNode[n] == this);

    constantNode.erase(n);
}

Expression::data const* Expression::data::constant(double d)
{
    auto node = constantNode.find(d);

    return node == constantNode.end() ? new ConstantNode(d) : Clone(node->second);
}

/***********************************************************************************************************************
*** VariableNode
***********************************************************************************************************************/

struct VariableNode final : public Expression::data, private ObjectGuard<VariableNode>
{
    explicit VariableNode(Variable const&);
    virtual ~VariableNode();

    bool guaranteed(Expression::Attribute) const override final;

    bool is(NodeType t) const override final { return t == NodeType::VARIABLE; }
    bool is(NodeType t, Expression::data const* p) const override final { return t == NodeType::VARIABLE && p == this; }

    Expression::data const* derivative(Variable const&) const override final;
    double value() const override final;

    void print(std::ostream&) const override final;

private:
    Variable const x;
};

//----------------------------------------------------------------------------------------------------------------------

VariableNode::VariableNode(Variable const& r) : Expression::data(1), x(r)
{
    assert(variableNode.find(x.id()) == variableNode.end());

    variableNode[x.id()] = this;
}

VariableNode::~VariableNode()
{
    assert(variableNode.find(x.id()) != variableNode.end() && variableNode[x.id()] == this);

    variableNode.erase(x.id());
}

Expression::data const* Expression::data::variable(Variable const& r)
{
    auto node = variableNode.find(r.id());

    return node == variableNode.end() ? new VariableNode(r) : Clone(node->second);
}

/***********************************************************************************************************************
*** Abs
***********************************************************************************************************************/

struct Abs final : public FunctionNode, private ObjectGuard<Abs>
{
    Abs(Expression::data const* p) : FunctionNode(p, NodeType::ABS) { }

    Expression::data const* abs() const override final;
    Expression::data const* sign() const override final;
    Expression::data const* cos() const override final;
    Expression::data const* cosh() const override final;
    Expression::data const* square() const override final;
    Expression::data const* yconic() const override final;
    Expression::data const* zconic() const override final;

    bool guaranteed(Expression::Attribute) const override final;

    Expression::data const* derivative(Variable const&) const override final;
    double value() const override final;

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** Sign
***********************************************************************************************************************/

struct Sign final : public FunctionNode, private ObjectGuard<Sign>
{
    Sign(Expression::data const* p) : FunctionNode(p, NodeType::SIGN) { }

    Expression::data const* sign() const override final;
    Expression::data const* sqrt() const override final;
    Expression::data const* cbrt() const override final;

    bool guaranteed(Expression::Attribute) const override final;

    Expression::data const* derivative(Variable const&) const override final;
    double value() const override final;

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** Sqrt
***********************************************************************************************************************/

struct Sqrt final : public FunctionNode, private ObjectGuard<Sqrt>
{
    Sqrt(Expression::data const* p) : FunctionNode(p, NodeType::SQRT) { }

    Expression::data const* abs() const override final;
    Expression::data const* square() const override final;
    Expression::data const* pow(Expression::data const*) const override final;

    bool guaranteed(Expression::Attribute) const override final;

    Expression::data const* derivative(Variable const&) const override final;
    double value() const override final;

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** Cbrt
***********************************************************************************************************************/

struct Cbrt final : public FunctionNode, private ObjectGuard<Cbrt>
{
    Cbrt(Expression::data const* p) : FunctionNode(p, NodeType::CBRT) { }

    Expression::data const* sign() const override final;
    Expression::data const* pow(Expression::data const*) const override final;

    bool guaranteed(Expression::Attribute) const override final;

    Expression::data const* derivative(Variable const&) const override final;
    double value() const override final;

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** Exp
***********************************************************************************************************************/

struct Exp final : public FunctionNode, private ObjectGuard<Exp>
{
    Exp(Expression::data const* p) : FunctionNode(p, NodeType::EXP) { }

    Expression::data const* abs() const override final;
    Expression::data const* sign() const override final;
    Expression::data const* log() const override final;
    Expression::data const* pow(Expression::data const*) const override final;

    bool guaranteed(Expression::Attribute) const override final;

    Expression::data const* derivative(Variable const&) const override final;
    double value() const override final;

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** Log
***********************************************************************************************************************/

struct Log final : public FunctionNode, private ObjectGuard<Log>
{
    Log(Expression::data const* p) : FunctionNode(p, NodeType::LOG) { }

    Expression::data const* exp() const override final;

    bool guaranteed(Expression::Attribute) const override final;

    Expression::data const* derivative(Variable const&) const override final;
    double value() const override final;

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** Sin
***********************************************************************************************************************/

struct Sin final : public FunctionNode, private ObjectGuard<Sin>
{
    Sin(Expression::data const* p) : FunctionNode(p, NodeType::SIN) { }

    Expression::data const* zconic() const override final;

    bool guaranteed(Expression::Attribute) const override final;

    Expression::data const* derivative(Variable const&) const override final;
    double value() const override final;

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** Cos
***********************************************************************************************************************/

struct Cos final : public FunctionNode, private ObjectGuard<Cos>
{
    Cos(Expression::data const* p) : FunctionNode(p, NodeType::COS) { }

    Expression::data const* invert() const;
    Expression::data const* zconic() const override final;

    bool guaranteed(Expression::Attribute) const override final;

    Expression::data const* derivative(Variable const&) const override final;
    double value() const override final;

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** Tan
***********************************************************************************************************************/

struct Tan final : public FunctionNode, private ObjectGuard<Tan>
{
    Tan(Expression::data const* p) : FunctionNode(p, NodeType::TAN) { }

    bool guaranteed(Expression::Attribute) const override final;

    Expression::data const* derivative(Variable const&) const override final;
    double value() const override final;

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** Sec
***********************************************************************************************************************/

struct Sec final : public FunctionNode, private ObjectGuard<Sec>
{
    Sec(Expression::data const* p) : FunctionNode(p, NodeType::SEC) { }

    bool guaranteed(Expression::Attribute) const override final;

    Expression::data const* invert() const;

    Expression::data const* derivative(Variable const&) const override final;
    double value() const override final;

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** ASin
***********************************************************************************************************************/

struct ASin final : public FunctionNode, private ObjectGuard<ASin>
{
    ASin(Expression::data const* p) : FunctionNode(p, NodeType::ASIN) { }

    Expression::data const* sign() const override final;
    Expression::data const* sin() const override final;
    Expression::data const* cos() const override final;

    bool guaranteed(Expression::Attribute) const override final;

    Expression::data const* derivative(Variable const&) const override final;
    double value() const override final;

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** ACos
***********************************************************************************************************************/

struct ACos final : public FunctionNode, private ObjectGuard<ACos>
{
    ACos(Expression::data const* p) : FunctionNode(p, NodeType::ACOS) { }

    Expression::data const* abs() const override final;
    Expression::data const* sin() const override final;
    Expression::data const* cos() const override final;

    bool guaranteed(Expression::Attribute) const override final;

    Expression::data const* derivative(Variable const&) const override final;
    double value() const override final;

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** ATan
***********************************************************************************************************************/

struct ATan final : public FunctionNode, private ObjectGuard<ATan>
{
    ATan(Expression::data const* p) : FunctionNode(p, NodeType::ATAN) { }

    Expression::data const* sign() const override final;
    Expression::data const* cos() const override final;
    Expression::data const* tan() const override final;

    bool guaranteed(Expression::Attribute) const override final;

    Expression::data const* derivative(Variable const&) const override final;
    double value() const override final;

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** SinH
***********************************************************************************************************************/

struct SinH final : public FunctionNode, private ObjectGuard<SinH>
{
    SinH(Expression::data const* p) : FunctionNode(p, NodeType::SINH) { }

    Expression::data const* sign() const override final;
    Expression::data const* asinh() const override final;
    Expression::data const* yconic() const override final;

    bool guaranteed(Expression::Attribute) const override final;

    Expression::data const* derivative(Variable const&) const override final;
    double value() const override final;

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** CosH
***********************************************************************************************************************/

struct CosH final : public FunctionNode, private ObjectGuard<CosH>
{
    CosH(Expression::data const* p) : FunctionNode(p, NodeType::COSH) { }

    Expression::data const* abs() const override final;
    Expression::data const* sign() const override final;
    Expression::data const* acosh() const override final;
    Expression::data const* xconic() const override final;

    bool guaranteed(Expression::Attribute) const override final;

    Expression::data const* derivative(Variable const&) const override final;
    double value() const override final;

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** TanH
***********************************************************************************************************************/

struct TanH final : public FunctionNode, private ObjectGuard<TanH>
{
    TanH(Expression::data const* p) : FunctionNode(p, NodeType::TANH) { ; }

    Expression::data const* sign() const override final;
    Expression::data const* atanh() const override final;

    bool guaranteed(Expression::Attribute) const override final;

    Expression::data const* derivative(Variable const&) const override final;
    double value() const override final;

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** ASinH
***********************************************************************************************************************/

struct ASinH final : public FunctionNode, private ObjectGuard<ASinH>
{
    ASinH(Expression::data const* p) : FunctionNode(p, NodeType::ASINH) { }

    Expression::data const* sign() const override final;
    Expression::data const* exp() const override final;
    Expression::data const* sinh() const override final;
    Expression::data const* cosh() const override final;

    bool guaranteed(Expression::Attribute) const override final;

    Expression::data const* derivative(Variable const&) const override final;
    double value() const override final;

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** ACosH
***********************************************************************************************************************/

struct ACosH final : public FunctionNode, private ObjectGuard<ACosH>
{
    ACosH(Expression::data const* p) : FunctionNode(p, NodeType::ACOSH) { }

    Expression::data const* abs() const override final;
    Expression::data const* sinh() const override final;
    Expression::data const* cosh() const override final;

    bool guaranteed(Expression::Attribute) const override final;

    Expression::data const* derivative(Variable const&) const override final;
    double value() const override final;

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** ATanH
***********************************************************************************************************************/

struct ATanH final : public FunctionNode, private ObjectGuard<ATanH>
{
    ATanH(Expression::data const* p) : FunctionNode(p, NodeType::ATANH) { }

    Expression::data const* sign() const override final;
    Expression::data const* cosh() const override final;
    Expression::data const* tanh() const override final;

    bool guaranteed(Expression::Attribute) const override final;

    Expression::data const* derivative(Variable const&) const override final;
    double value() const override final;

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** Erf
***********************************************************************************************************************/

struct Erf final : public FunctionNode, private ObjectGuard<Erf>
{
    Erf(Expression::data const* p) : FunctionNode(p, NodeType::ERF) { }

    Expression::data const* sign() const override final;

    bool guaranteed(Expression::Attribute) const override final;

    Expression::data const* derivative(Variable const&) const override final;
    double value() const override final;

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** Invert
***********************************************************************************************************************/

struct Invert final : public FunctionNode, private ObjectGuard<Invert>
{
    Invert(Expression::data const* p) : FunctionNode(p, NodeType::INVERT) { }

    Expression::data const* abs() const override final;
    Expression::data const* sign() const override final;
    Expression::data const* sqrt() const override final;
    Expression::data const* cbrt() const override final;
    Expression::data const* log() const override final;
    Expression::data const* invert() const override final;
    Expression::data const* square() const override final;
    Expression::data const* mul(Expression::data const*) const override final;
    Expression::data const* pow(Expression::data const*) const override final;

    bool easyInvert() const override final { return true; }
    bool easyNegate() const override final { if (f_x->easyNegate()) UNREACHABLE; return false; }

    bool guaranteed(Expression::Attribute) const override final;

    Expression::data const* derivative(Variable const&) const override final;
    double value() const override final;

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** Negate
***********************************************************************************************************************/

struct Negate final : public FunctionNode, private ObjectGuard<Negate>
{
    Negate(Expression::data const* p) : FunctionNode(p, NodeType::NEGATE) { }

    Expression::data const* abs() const override final;
    Expression::data const* sign() const override final;
    Expression::data const* cbrt() const override final;
    Expression::data const* exp() const override final;
    Expression::data const* sin() const override final;
    Expression::data const* cos() const override final;
    Expression::data const* tan() const override final;
    Expression::data const* sec() const override final;
    Expression::data const* asin() const override final;
    Expression::data const* atan() const override final;
    Expression::data const* sinh() const override final;
    Expression::data const* cosh() const override final;
    Expression::data const* tanh() const override final;
    Expression::data const* asinh() const override final;
    Expression::data const* atanh() const override final;
    Expression::data const* erf() const override final;
    Expression::data const* invert() const override final;
    Expression::data const* negate() const override final;
    Expression::data const* square() const override final;
    Expression::data const* yconic() const override final;
    Expression::data const* zconic() const override final;

    Expression::data const* add(Expression::data const*) const override final;
    Expression::data const* mul(Expression::data const*) const override final;

    bool easyInvert() const override final { return f_x->easyInvert(); }
    bool easyNegate() const override final { return true; }

    bool guaranteed(Expression::Attribute) const override final;

    Expression::data const* derivative(Variable const&) const override final;
    double value() const override final;

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** Square
***********************************************************************************************************************/

struct Square final : public FunctionNode, private ObjectGuard<Square>
{
    Square(Expression::data const* p) : FunctionNode(p, NodeType::SQUARE) { }

    Expression::data const* abs() const override final;
    Expression::data const* sqrt() const override final;
    Expression::data const* mul(Expression::data const*) const override final;
    Expression::data const* commutative_mul(Expression::data const*) const override final;
    Expression::data const* pow(Expression::data const*) const override final;

    bool guaranteed(Expression::Attribute) const override final;

    Expression::data const* derivative(Variable const&) const override final;
    double value() const override final;

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** XConic
***********************************************************************************************************************/

struct XConic final : public FunctionNode, private ObjectGuard<XConic>
{
    XConic(Expression::data const* p) : FunctionNode(p, NodeType::XCONIC) { }

    Expression::data const* abs() const override final;
    Expression::data const* asinh() const override final;
    Expression::data const* yconic() const override final;

    bool guaranteed(Expression::Attribute) const override final;

    Expression::data const* derivative(Variable const&) const override final;
    double value() const override final;

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** YConic
***********************************************************************************************************************/

struct YConic final : public FunctionNode, private ObjectGuard<YConic>
{
    YConic(Expression::data const* p) : FunctionNode(p, NodeType::YCONIC) { }

    Expression::data const* abs() const override final;
    Expression::data const* acosh() const override final;
    Expression::data const* xconic() const override final;

    bool guaranteed(Expression::Attribute) const override final;

    Expression::data const* derivative(Variable const&) const override final;
    double value() const override final;

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** ZConic
***********************************************************************************************************************/

struct ZConic final : public FunctionNode, private ObjectGuard<ZConic>
{
    ZConic(Expression::data const* p) : FunctionNode(p, NodeType::ZCONIC) { }

    Expression::data const* abs() const override final;
    Expression::data const* asin() const override final;
    Expression::data const* acos() const override final;
    Expression::data const* zconic() const override final;

    bool guaranteed(Expression::Attribute) const override final;

    Expression::data const* derivative(Variable const&) const override final;
    double value() const override final;

    void print(std::ostream&) const override final;
};

/***********************************************************************************************************************
*** function()
***********************************************************************************************************************/

Expression::data const* Expression::data::function(NodeType n) const
{
    if (functionNode.find(n) != functionNode.end()) { return Clone(functionNode[n]); }

    switch (n)
    {
    case NodeType::ABS:
        return new Abs(Clone(this));

    case NodeType::SIGN:
        return new Sign(Clone(this));

    case NodeType::SQRT:
        return new Sqrt(Clone(this));

    case NodeType::CBRT:
        return new Cbrt(Clone(this));

    case NodeType::EXP:
        return new Exp(Clone(this));

    case NodeType::LOG:
        return new Log(Clone(this));

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

    case NodeType::ASINH:
        return new ASinH(Clone(this));

    case NodeType::ACOSH:
        return new ACosH(Clone(this));

    case NodeType::ATANH:
        return new ATanH(Clone(this));

    case NodeType::ERF:
        return new Erf(Clone(this));

    case NodeType::INVERT:
        return new Invert(Clone(this));

    case NodeType::NEGATE:
        return new Negate(Clone(this));

    case NodeType::SQUARE:
        return new Square(Clone(this));

    case NodeType::XCONIC:
        return new XConic(Clone(this));

    case NodeType::YCONIC:
        return new YConic(Clone(this));

    case NodeType::ZCONIC:
        return new ZConic(Clone(this));
    }

    FAIL("Attempt was made to create a function node of an unknown type.");
}

/***********************************************************************************************************************
*** Add
***********************************************************************************************************************/

struct Add final : public OperatorNode, private ObjectGuard<Add>
{
    Add(Expression::data const*, Expression::data const*);
    virtual ~Add();

    double value() const override final;

    Expression::data const* add(Expression::data const*) const override final;
    Expression::data const* commutative_add(Expression::data const*) const override final;
    Expression::data const* mul(Expression::data const*) const override final;
    Expression::data const* commutative_mul(Expression::data const*) const override final;

    bool is(NodeType t) const override final { return t == NodeType::ADD; }

    bool guaranteed(Expression::Attribute) const override final;

    Expression::data const* derivative(Variable const&) const override final;

    void print(std::ostream&) const override final;
};

//----------------------------------------------------------------------------------------------------------------------

Add::Add(Expression::data const* p, Expression::data const* q) : OperatorNode(p, q)
{
    assert(f_x->addNode.find(g_x) == f_x->addNode.end());
    assert(g_x->addNode.find(f_x) == g_x->addNode.end());

    f_x->addNode[g_x] = this;
    g_x->addNode[f_x] = this;
}

Add::~Add()
{
    assert(f_x->addNode.find(g_x) != f_x->addNode.end() && f_x->addNode[g_x] == this);
    assert(g_x->addNode.find(f_x) != g_x->addNode.end() && g_x->addNode[f_x] == this);

    f_x->addNode.erase(g_x);
    g_x->addNode.erase(f_x);
}

/***********************************************************************************************************************
*** Mul
***********************************************************************************************************************/

struct Mul final : public OperatorNode, private ObjectGuard<Mul>
{
    Mul(Expression::data const* p, Expression::data const* q);
    virtual ~Mul();

    Expression::data const* mul(Expression::data const*) const override final;
    Expression::data const* commutative_mul(Expression::data const*) const override final;

    bool is(NodeType t) const override final { return t == NodeType::MUL; }

    bool guaranteed(Expression::Attribute) const override final;

    Expression::data const* derivative(Variable const&) const override final;
    double value() const override final;

    void print(std::ostream&) const override final;
};

//----------------------------------------------------------------------------------------------------------------------

Mul::Mul(Expression::data const* p, Expression::data const* q) : OperatorNode(p, q)
{
    assert(f_x->mulNode.find(g_x) == f_x->mulNode.end());
    assert(g_x->mulNode.find(f_x) == g_x->mulNode.end());

    f_x->mulNode[g_x] = this;
    g_x->mulNode[f_x] = this;
}

Mul::~Mul()
{
    assert(f_x->mulNode.find(g_x) != f_x->mulNode.end() && f_x->mulNode[g_x] == this);
    assert(g_x->mulNode.find(f_x) != g_x->mulNode.end() && g_x->mulNode[f_x] == this);

    f_x->mulNode.erase(g_x);
    g_x->mulNode.erase(f_x);
}

/***********************************************************************************************************************
*** Pow
***********************************************************************************************************************/

struct Pow final : public OperatorNode, private ObjectGuard<Pow>
{
    Pow(Expression::data const* p, Expression::data const* q);
    virtual ~Pow();

    Expression::data const* sqrt() const override final;
    Expression::data const* cbrt() const override final;
    Expression::data const* invert() const override final;
    Expression::data const* square() const override final;
    Expression::data const* mul(Expression::data const*) const override final;
    Expression::data const* commutative_mul(Expression::data const*) const override final;
    Expression::data const* pow(Expression::data const*) const override final;

    bool is(NodeType t) const override final { return t == NodeType::POW; }

    bool guaranteed(Expression::Attribute) const override final;

    Expression::data const* derivative(Variable const&) const override final;
    double value() const override final;

    void print(std::ostream&) const override final;
};

//----------------------------------------------------------------------------------------------------------------------

Pow::Pow(Expression::data const* p, Expression::data const* q) : OperatorNode(p, q)
{
    assert(f_x->powNode.find(g_x) == f_x->powNode.end());

    f_x->powNode[g_x] = this;
}

Pow::~Pow()
{
    assert(f_x->powNode.find(g_x) != f_x->powNode.end() && f_x->powNode[g_x] == this);

    f_x->powNode.erase(g_x);
}

/***********************************************************************************************************************
*** abs()
***********************************************************************************************************************/

Expression::data const* Expression::data::abs() const
{
    if (guaranteed(Expression::Attribute::NONNEGATIVE)) return Clone(this);
    if (guaranteed(Expression::Attribute::NONPOSITIVE)) return negate();

    return function(NodeType::ABS);
}

Expression::data const* ConstantNode::abs() const
{
    return constant(std::abs(n));
}

Expression::data const* Abs::abs() const  // abs(abs(x)) ----> abs(x)
{
    return Clone(this);
}

Expression::data const* Sqrt::abs() const  // abs(sqrt(x)) ----> sqrt(x)
{
    return Clone(this);
}

Expression::data const* Exp::abs() const  // abs(exp(x)) ----> exp(x)
{
    return Clone(this);
}

Expression::data const* ACos::abs() const  // abs(acos(x)) ----> acos(x)
{
    return Clone(this);
}

Expression::data const* CosH::abs() const  // abs(cosh(x)) ----> cosh(x)
{
    return Clone(this);
}

Expression::data const* ACosH::abs() const  // abs(acosh(x)) ----> acosh(x)
{
    return Clone(this);
}

Expression::data const* Invert::abs() const  // abs(1/x) ----> 1/abs(x)
{
    auto step0 = f_x->abs();
    auto step1 = step0->invert();

    Erase(step0);

    return step1;
}

Expression::data const* Negate::abs() const  // abs(-x) ----> abs(x)
{
    return f_x->abs();
}

Expression::data const* Square::abs() const  // abs(x^2) ----> x^2
{
    return Clone(this);
}

Expression::data const* XConic::abs() const  // abs(sqrt(x*x-1)) ----> sqrt(x*x-1)
{
    return Clone(this);
}

Expression::data const* YConic::abs() const  // abs(sqrt(x*x+1)) ----> sqrt(x*x+1)
{
    return Clone(this);
}

Expression::data const* ZConic::abs() const  // abs(sqrt(1-x*x)) ----> sqrt(1-x*x)
{
    return Clone(this);
}

/***********************************************************************************************************************
*** sign()
***********************************************************************************************************************/

Expression::data const* Expression::data::sign() const
{
    if (guaranteed(Expression::Attribute::POSITIVE)) return Clone(literal1);
    if (guaranteed(Expression::Attribute::NEGATIVE)) return Clone(literal1Neg);
    return function(NodeType::SIGN);
}

Expression::data const* ConstantNode::sign() const
{
    return constant(double(n > 0) - (n < 0));
}

Expression::data const* Abs::sign() const  // sign(abs(x)) ----> abs(sign(x))
{
    auto step0 = f_x->sign();
    auto step1 = step0->abs();

    Erase(step0);

    return step1;
}

Expression::data const* Sign::sign() const  // sign(sign(x)) ----> sign(x)
{
    return Clone(this);
}

Expression::data const* Cbrt::sign() const  // sign(cbrt(x)) ----> sign(x)
{
    return f_x->sign();
}

Expression::data const* Exp::sign() const  // sign(exp(x)) ----> 1
{
    return Clone(literal1);
}

Expression::data const* ASin::sign() const  // sign(asin(x)) ----> sign(x)  ; iff -1 <= x <= 1
{
    return f_x->guaranteed(Expression::Attribute::UNITRANGE) ? f_x->sign() : Expression::data::sign();
}

Expression::data const* ATan::sign() const  // sign(atan(x)) ----> sign(x)
{
    return f_x->sign();
}

Expression::data const* SinH::sign() const  // sign(sinh(x)) ----> sign(x)
{
    return f_x->sign();
}

Expression::data const* CosH::sign() const  // sign(cosh(x)) ----> 1
{
    return Clone(literal1);
}

Expression::data const* TanH::sign() const  // sign(tanh(x)) ----> sign(x)
{
    return f_x->sign();
}

Expression::data const* ASinH::sign() const  // sign(asinh(x)) ----> sign(x)
{
    return f_x->sign();
}

Expression::data const* ATanH::sign() const  // sign(atanh(x)) -----> sign(x)  ; iff -1 < x < 1
{
    return f_x->guaranteed(Expression::Attribute::OPENUNITRANGE) ? f_x->sign() : Expression::data::sign();
}

Expression::data const* Erf::sign() const  // sign(erf(x)) ----> sign(x)
{
    return f_x->sign();
}

Expression::data const* Invert::sign() const  // sign(1/x) ----> 1/sign(x)
{
    auto step0 = f_x->sign();
    auto step1 = step0->invert();

    Erase(step0);

    return step1;
}

Expression::data const* Negate::sign() const  // sign(-x) ----> -sign(x)
{
    auto step0 = f_x->sign();
    auto step1 = step0->negate();

    Erase(step0);

    return step1;
}

/***********************************************************************************************************************
*** sqrt()
***********************************************************************************************************************/

Expression::data const* Expression::data::sqrt() const
{
    return function(NodeType::SQRT);
}

Expression::data const* ConstantNode::sqrt() const
{
    return constant(std::sqrt(n));
}

Expression::data const* Sign::sqrt() const  // sqrt(sign(x)) ----> sign(x)  ; iff x >= 0
{
    return f_x->guaranteed(Expression::Attribute::NONNEGATIVE) ? Clone(this) : Expression::data::sqrt();
}

Expression::data const* Invert::sqrt() const  // sqrt(1/x) ----> 1/sqrt(x)
{
    auto step0 = f_x->sqrt();
    auto step1 = step0->invert();

    Erase(step0);

    return step1;
}

Expression::data const* Square::sqrt() const  // sqrt(x^2) ----> abs(x)
{
    return f_x->abs();
}

Expression::data const* Pow::sqrt() const  // sqrt(x^y) ----> x^(y/2)
{
    auto step0 = g_x->mul(literal2Inv);
    auto step1 = f_x->pow(step0);

    Erase(step0);

    return step1;
}

/***********************************************************************************************************************
*** cbrt()
***********************************************************************************************************************/

Expression::data const* Expression::data::cbrt() const
{
    return function(NodeType::CBRT);
}

Expression::data const* ConstantNode::cbrt() const
{
    return constant(std::cbrt(n));
}

Expression::data const* Sign::cbrt() const  // cbrt(sign(x)) ----> sign(x)
{
    return Clone(this);
}

Expression::data const* Invert::cbrt() const  // cbrt(1/x) ----> 1/cbrt(x)
{
    auto step0 = f_x->cbrt();
    auto step1 = step0->invert();

    Erase(step0);

    return step1;
}

Expression::data const* Negate::cbrt() const  // cbrt(-x) ----> -cbrt(x)
{
    auto step0 = f_x->cbrt();
    auto step1 = step0->negate();

    Erase(step0);

    return step1;
}

Expression::data const* Pow::cbrt() const  // sqrt(x^y) ----> x^(y/3)
{
    auto step0 = g_x->mul(literal3Inv);
    auto step1 = f_x->pow(step0);

    Erase(step0);

    return step1;
}

/***********************************************************************************************************************
*** exp()
***********************************************************************************************************************/

Expression::data const* Expression::data::exp() const
{
    return function(NodeType::EXP);
}

Expression::data const* ConstantNode::exp() const
{
    return constant(std::exp(n));
}

Expression::data const* Log::exp() const  // exp(log(x)) ----> x  ; iff x > 0
{
    return f_x->guaranteed(Expression::Attribute::POSITIVE) ? Clone(f_x) : Expression::data::exp();
}

Expression::data const* ASinH::exp() const  // exp(asinh(f_x)) ----> sqrt(1+x*x) + x
{
    auto step0 = f_x->yconic();
    auto step1 = f_x->add(step0);

    Erase(step0);

    return step1;
}

Expression::data const* Negate::exp() const  // exp(-x) ----> 1/exp(x)
{
    auto step0 = f_x->exp();
    auto step1 = step0->invert();

    Erase(step0);

    return step1;
}

/***********************************************************************************************************************
*** log()
***********************************************************************************************************************/

Expression::data const* Expression::data::log() const
{
    return function(NodeType::LOG);
}

Expression::data const* ConstantNode::log() const
{
    return constant(std::log(n));
}

Expression::data const* Exp::log() const  // log(exp(x)) ----> x
{
    return Clone(f_x);
}

Expression::data const* Invert::log() const  // log(1/x) ----> -log(x)
{
    auto step0 = f_x->log();
    auto step1 = step0->negate();

    Erase(step0);

    return step1;
}

/***********************************************************************************************************************
*** sin()
***********************************************************************************************************************/

Expression::data const* Expression::data::sin() const
{
    return function(NodeType::SIN);
}

Expression::data const* ConstantNode::sin() const
{
    return constant(std::sin(n));
}

Expression::data const* ASin::sin() const  // sin(asin(x)) = x  ; iff -1 <= x <= 1
{
    return f_x->guaranteed(Expression::Attribute::UNITRANGE) ? Clone(f_x) : Expression::data::sin();
}

Expression::data const* ACos::sin() const
{
    return f_x->zconic();
}

Expression::data const* Negate::sin() const  // sin(-x) ----> -sin(x)
{
    auto step0 = f_x->sin();
    auto step1 = step0->negate();

    Erase(step0);

    return step1;
}

/***********************************************************************************************************************
*** cos()
***********************************************************************************************************************/

Expression::data const* Expression::data::cos() const
{
    return function(NodeType::COS);
}

Expression::data const* ConstantNode::cos() const
{
    return constant(std::cos(n));
}

Expression::data const* Abs::cos() const  // cos(abs(x)) ----> cos(x)
{
    return f_x->cos();
}

Expression::data const* ASin::cos() const  // cos(asin(x)) ----> sqrt(1-x^2)
{
    return f_x->zconic();
}

Expression::data const* ACos::cos() const  // cos(acos(x)) ----> x  ; iff -1 <= x <= 1
{
    return f_x->guaranteed(Expression::Attribute::UNITRANGE) ? Clone(f_x) : Expression::data::cos();
}

Expression::data const* ATan::cos() const  // cos(atan(x)) ----> 1 / sqrt(1+x*x)
{
    auto step0 = f_x->yconic();
    auto step1 = step0->invert();

    Erase(step0);

    return step1;
}

Expression::data const* Negate::cos() const  // cos(-x) ----> cos(x)
{
    return f_x->cos();
}

/***********************************************************************************************************************
*** tan()
***********************************************************************************************************************/

Expression::data const* Expression::data::tan() const
{
    return function(NodeType::TAN);
}

Expression::data const* ConstantNode::tan() const
{
    return constant(std::tan(n));
}

Expression::data const* ATan::tan() const  // tan(atan(x)) ----> x
{
    return Clone(f_x);
}

Expression::data const* Negate::tan() const  // tan(-x) ----> -tan(x)
{
    auto step0 = f_x->tan();
    auto step1 = step0->negate();

    Erase(step0);

    return step1;
}

/***********************************************************************************************************************
*** sec()
***********************************************************************************************************************/

Expression::data const* Expression::data::sec() const
{
    return function(NodeType::SEC);
}

Expression::data const* ConstantNode::sec() const
{
    return constant(1 / std::cos(n));
}

Expression::data const* Negate::sec() const  // sec(-x) ----> sec(x)
{
    return f_x->sec();
}

/***********************************************************************************************************************
*** asin()
***********************************************************************************************************************/

Expression::data const* Expression::data::asin() const
{
    return function(NodeType::ASIN);
}

Expression::data const* ConstantNode::asin() const
{
    return constant(std::asin(n));
}

Expression::data const* Negate::asin() const  // asin(-x) ----> -asin(x)
{
    auto step0 = f_x->asin();
    auto step1 = step0->negate();

    Erase(step0);

    return step1;
}

Expression::data const* ZConic::asin() const  // asin(sqrt(1-x*x)) ----> acos(abs(x))
{
    auto step0 = f_x->abs();
    auto step1 = step0->acos();

    Erase(step0);

    return step1;
}

/***********************************************************************************************************************
*** acos()
***********************************************************************************************************************/

Expression::data const* Expression::data::acos() const
{
    return function(NodeType::ACOS);
}

Expression::data const* ConstantNode::acos() const
{
    return constant(std::acos(n));
}

Expression::data const* ZConic::acos() const  // acos(sqrt(1-x*x)) ----> abs(asin(x))
{
    auto step0 = f_x->asin();
    auto step1 = step0->abs();

    Erase(step0);

    return step1;
}

/***********************************************************************************************************************
*** atan()
***********************************************************************************************************************/

Expression::data const* Expression::data::atan() const
{
    return function(NodeType::ATAN);
}

Expression::data const* ConstantNode::atan() const
{
    return constant(std::atan(n));
}

Expression::data const* Negate::atan() const  // atan(-x) ----> -atan(x)
{
    auto step0 = f_x->atan();
    auto step1 = step0->negate();

    Erase(step0);

    return step1;
}

/***********************************************************************************************************************
*** sinh()
***********************************************************************************************************************/

Expression::data const* Expression::data::sinh() const
{
    return function(NodeType::SINH);
}

Expression::data const* ConstantNode::sinh() const
{
    return constant(std::sinh(n));
}

Expression::data const* ASinH::sinh() const  // sinh(asinh(x)) ----> x
{
    return Clone(f_x);
}

Expression::data const* ACosH::sinh() const  // sinh(acosh(x)) ----> sqrt(1-x^2)
{
    return f_x->zconic();
}

Expression::data const* Negate::sinh() const  // sinh(-x) ----> -sinh(x)
{
    auto step0 = f_x->sinh();
    auto step1 = step0->negate();

    Erase(step0);

    return step1;
}

/***********************************************************************************************************************
*** cosh()
***********************************************************************************************************************/

Expression::data const* Expression::data::cosh() const
{
    return function(NodeType::COSH);
}

Expression::data const* ConstantNode::cosh() const
{
    return constant(std::cosh(n));
}

Expression::data const* Abs::cosh() const  // cosh(abs(x)) ----> cosh(x)
{
    return f_x->cosh();
}

Expression::data const* ASinH::cosh() const  // cosh(asinh(x)) ----> sqrt(1+x^2)
{
    return f_x->yconic();
}

Expression::data const* ACosH::cosh() const  // cosh(acosh(x)) ----> x  ; iff x >= 1
{
    return f_x->guaranteed(Expression::Attribute::POSITIVE) && f_x->guaranteed(Expression::Attribute::ANTIOPENUNITRANGE) ? Clone(f_x) : Expression::data::cosh();
}

Expression::data const* ATanH::cosh() const  // cosh(atanh(x)) ----> 1/sqrt(1-x*x)
{
    auto step0 = f_x->zconic();
    auto step1 = step0->invert();

    Erase(step0);

    return step1;
}

Expression::data const* Negate::cosh() const  // cosh(-x) ----> cosh(x)
{
    return f_x->cosh();
}

/***********************************************************************************************************************
*** tanh()
***********************************************************************************************************************/

Expression::data const* Expression::data::tanh() const
{
    return function(NodeType::TANH);
}

Expression::data const* ConstantNode::tanh() const
{
    return constant(std::tanh(n));
}

Expression::data const* ATanH::tanh() const  // tanh(atanh(x)) ----> x  ; iff -1 < x < 1
{
    return f_x->guaranteed(Expression::Attribute::OPENUNITRANGE) ? Clone(f_x) : Expression::data::tanh();
}

Expression::data const* Negate::tanh() const  // tanh(-x) ----> -tanh(x)
{
    auto step0 = f_x->tanh();
    auto step1 = step0->negate();

    Erase(step0);

    return step1;
}

/***********************************************************************************************************************
*** asinh()
***********************************************************************************************************************/

Expression::data const* Expression::data::asinh() const
{
    return function(NodeType::ASINH);
}

Expression::data const* ConstantNode::asinh() const
{
    return constant(std::asinh(n));
}

Expression::data const* SinH::asinh() const  // asinh(sinh(x)) ----> x
{
    return Clone(f_x);
}

Expression::data const* Negate::asinh() const  // asinh(-x) ----> -asinh(x)
{
    auto step0 = f_x->asinh();
    auto step1 = step0->negate();

    Erase(step0);

    return step1;
}

Expression::data const* XConic::asinh() const  // asinh(sqrt(x^2-1)) ----> acosh(abs(x))
{
    auto step0 = f_x->abs();
    auto step1 = step0->acosh();

    Erase(step0);

    return step1;
}

/***********************************************************************************************************************
*** acosh()
***********************************************************************************************************************/

Expression::data const* Expression::data::acosh() const
{
    return function(NodeType::ACOSH);
}

Expression::data const* ConstantNode::acosh() const
{
    return constant(std::acosh(n));
}

Expression::data const* CosH::acosh() const  // acosh(cosh(x)) ----> abs(x)
{
    return f_x->abs();
}

Expression::data const* YConic::acosh() const  // acosh(sqrt(1+x^2)) ----> abs(asinh(x))
{
    auto step0 = f_x->asinh();
    auto step1 = step0->abs();

    Erase(step0);

    return step1;
}

/***********************************************************************************************************************
*** atanh()
***********************************************************************************************************************/

Expression::data const* Expression::data::atanh() const
{
    return function(NodeType::ATANH);
}

Expression::data const* ConstantNode::atanh() const
{
    return constant(std::atanh(n));
}

Expression::data const* TanH::atanh() const  // atanh(tanh(x)) ----> x
{
    return Clone(f_x);
}

Expression::data const* Negate::atanh() const  // atanh(-x) ----> -atanh(x)
{
    auto step0 = f_x->atanh();
    auto step1 = step0->negate();

    Erase(step0);

    return step1;
}

/***********************************************************************************************************************
*** erf()
***********************************************************************************************************************/

Expression::data const* Expression::data::erf() const
{
    return function(NodeType::ERF);
}

Expression::data const* ConstantNode::erf() const
{
    return constant(std::erf(n));
}

Expression::data const* Negate::erf() const  // erf(-x) ----> -erf(x)
{
    auto step0 = f_x->erf();
    auto step1 = step0->negate();

    Erase(step0);

    return step1;
}

/***********************************************************************************************************************
*** invert()
***********************************************************************************************************************/

Expression::data const* Expression::data::invert() const
{
    return function(NodeType::INVERT);
}

Expression::data const* ConstantNode::invert() const
{
    return constant(1 / n);
}

Expression::data const* Cos::invert() const  // 1/cos(x) ----> sec(x)
{
    return f_x->sec();
}

Expression::data const* Sec::invert() const  // 1/sec(x) ----> cos(x)
{
    return f_x->cos();
}

Expression::data const* Invert::invert() const  // 1/(1/x) ----> x  ; iff x != 0
{
    return f_x->guaranteed(Expression::Attribute::NONZERO) ? Clone(f_x) : Expression::data::invert();
}

Expression::data const* Negate::invert() const  // 1/(-x) ----> -(1/x)
{
    auto step0 = f_x->invert();
    auto step1 = step0->negate();

    Erase(step0);

    return step1;
}

Expression::data const* Pow::invert() const  // 1/(x^y) ---> x ^ -y
{
    auto step0 = g_x->negate();
    auto step1 = f_x->pow(step0);

    Erase(step0);

    return step1;
}

/***********************************************************************************************************************
*** negate()
***********************************************************************************************************************/

Expression::data const* Expression::data::negate() const
{
    return function(NodeType::NEGATE);
}

Expression::data const* ConstantNode::negate() const
{
    return constant(-n);
}

Expression::data const* Negate::negate() const  // -(-x) ----> x
{
    return Clone(f_x);
}

/***********************************************************************************************************************
*** square()
***********************************************************************************************************************/

Expression::data const* Expression::data::square() const
{
    return function(NodeType::SQUARE);
}

Expression::data const* ConstantNode::square() const
{
    return constant(n * n);
}

Expression::data const* Abs::square() const  // abs(x)^2 ----> x^2
{
    return f_x->square();
}

Expression::data const* Sqrt::square() const  // sqrt(x)^2 ----> x  ; iff x >= 0
{
    return f_x->guaranteed(Expression::Attribute::NONNEGATIVE) ? Clone(f_x) : Expression::data::square();
}

Expression::data const* Invert::square() const  // (1/x)^2 ----> 1/(x^2)
{
    auto step0 = f_x->square();
    auto step1 = step0->invert();

    Erase(step0);

    return step1;
}

Expression::data const* Negate::square() const  // (-x)^2 ----> x^2
{
    return f_x->square();
}

Expression::data const* Pow::square() const  // (x^y)^2 ---> x ^ (2*y)
{
    auto step0 = g_x->add(g_x);
    auto step1 = f_x->pow(step0);

    Erase(step0);

    return step1;
}

/***********************************************************************************************************************
*** xconic()
***********************************************************************************************************************/

Expression::data const* Expression::data::xconic() const
{
    return function(NodeType::XCONIC);
}

Expression::data const* ConstantNode::xconic() const
{
    return constant(std::sqrt(n > 0 ? n * n - 1 : -1));
}

Expression::data const* YConic::xconic() const  // sqrt(sqrt(1+x^2)^2-1) ----> abs(x)
{
    return f_x->abs();
}

Expression::data const* CosH::xconic() const  // sqrt((cosh(x)^2-1) ----> abs(sinh(x))
{
    auto step0 = f_x->sinh();
    auto step1 = step0->abs();

    Erase(step0);

    return step1;
}

/***********************************************************************************************************************
*** yconic()
***********************************************************************************************************************/

Expression::data const* Expression::data::yconic() const
{
    return function(NodeType::YCONIC);
}

Expression::data const* ConstantNode::yconic() const
{
    return constant(std::sqrt(1 + n * n));
}

Expression::data const* Abs::yconic() const  // abs(sqrt(x*x+1)) ----> sqrt(x*x+1)
{
    return f_x->yconic();
}

Expression::data const* Negate::yconic() const  // sqrt(1+(-x)^2) ----> sqrt(1+x^2)
{
    return f_x->yconic();
}

Expression::data const* SinH::yconic() const  // sqrt(1+sinh(x)^2) ----> cosh(x)
{
    return f_x->cosh();
}

Expression::data const* XConic::yconic() const  // sqrt(1+sqrt(x^2-1)^2) ----> abs(x)  ; iff x >= 1 || x <= -1
{
    return f_x->guaranteed(Expression::Attribute::ANTIOPENUNITRANGE) ? f_x->abs() : Expression::data::yconic();
}

/***********************************************************************************************************************
*** zconic()
***********************************************************************************************************************/

Expression::data const* Expression::data::zconic() const
{
    return function(NodeType::ZCONIC);
}

Expression::data const* ConstantNode::zconic() const
{
    return constant(std::sqrt(1 - n * n));
}

Expression::data const* Abs::zconic() const  // abs(sqrt(1-x*x)) ----> sqrt(1-x*x)
{
    return f_x->zconic();
}

Expression::data const* Sin::zconic() const  // sqrt(1-sin(x)^2) ----> abs(cos(x))
{
    auto step0 = f_x->cos();
    auto step1 = step0->abs();

    Erase(step0);

    return step1;
}

Expression::data const* Cos::zconic() const  // sqrt(1-cos(x)^2) ----> abs(sin(x))
{
    auto step0 = f_x->sin();
    auto step1 = step0->abs();

    Erase(step0);

    return step1;
}

Expression::data const* Negate::zconic() const  // sqrt(1-(-x)^2) ----> sqrt(1-x^2)
{
    return f_x->zconic();
}

Expression::data const* ZConic::zconic() const  // sqrt(1-sqrt(1-x^2)^2) ----> abs(x)  ; iff -1 <= x <= 1
{
    return f_x->guaranteed(Expression::Attribute::UNITRANGE) ? f_x->abs() : Expression::data::zconic();
}

/***********************************************************************************************************************
*** add()
***********************************************************************************************************************/

Expression::data const* Expression::data::add(Expression::data const* p) const
{
    return p->commutative_add(this);
}

Expression::data const* Expression::data::commutative_add(Expression::data const* p) const
{
    if (addNode.find(p) != addNode.end()) return Clone(addNode[p]);
    return new Add(Clone(p), Clone(this));
}

Expression::data const* ConstantNode::add(Expression::data const* p) const
{
    if (p->is(NodeType::CONSTANT)) return constant(n + p->evaluate());
    if (p->is(NodeType::NEGATE, this)) return Clone(literal0);
    if (n == 0) return Clone(p);
    return Expression::data::add(p);
}

Expression::data const* ConstantNode::commutative_add(Expression::data const* p) const
{
    if (p->is(NodeType::CONSTANT)) return constant(n + p->evaluate());
    if (p->is(NodeType::NEGATE, this)) return Clone(literal0);
    if (n == 0) return Clone(p);
    return Expression::data::commutative_add(p);
}

Expression::data const* Negate::add(Expression::data const* p) const
{
    return Expression::data::add(p);
}

Expression::data const* Add::add(Expression::data const* p) const
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

    return Expression::data::add(p);
}

Expression::data const* Add::commutative_add(Expression::data const* p) const
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

    return Expression::data::commutative_add(p);
}

/***********************************************************************************************************************
*** mul()
***********************************************************************************************************************/

Expression::data const* Expression::data::mul(Expression::data const* p) const
{
    if (p == this) return square();
    return p->commutative_mul(this);
}

Expression::data const* Expression::data::commutative_mul(Expression::data const* p) const
{
    if (mulNode.find(p) != mulNode.end()) return Clone(mulNode[p]);
    return new Mul(Clone(p), Clone(this));
}

Expression::data const* ConstantNode::mul(Expression::data const* p) const
{
    if (p->is(NodeType::CONSTANT)) return constant(n * p->evaluate());
    if (n == 0) return Clone(this);
    if (n == 1) return Clone(p);
    if (n == 2) return p->add(p);
    if (n == -1) return p->negate();
    return Expression::data::mul(p);
}

Expression::data const* ConstantNode::commutative_mul(Expression::data const* p) const
{
    if (p->is(NodeType::CONSTANT)) return constant(n * p->evaluate());
    if (n == 0) return Clone(this);
    if (n == 1) return Clone(p);
    if (n == 2) return p->add(p);
    if (n == -1) return p->negate();
    return Expression::data::commutative_mul(p);
}

Expression::data const* Invert::mul(Expression::data const* p) const
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

    return Expression::data::add(p);
}

Expression::data const* Negate::mul(Expression::data const* p) const
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

Expression::data const* Square::mul(Expression::data const* p) const
{
    if (f_x == p) return f_x->pow(literal3);
    return Expression::data::mul(p);
}

Expression::data const* Square::commutative_mul(Expression::data const* p) const
{
    if(f_x == p) return f_x->pow(literal3);
    return Expression::data::commutative_mul(p);
}

Expression::data const* Add::mul(Expression::data const* p) const
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

    return Expression::data::mul(p);
}

Expression::data const* Add::commutative_mul(Expression::data const* p) const
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

    return Expression::data::commutative_mul(p);
}

Expression::data const* Mul::mul(Expression::data const* p) const
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

    return Expression::data::mul(p);
}

Expression::data const* Mul::commutative_mul(Expression::data const* p) const
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

    return Expression::data::commutative_mul(p);
}

Expression::data const* Pow::mul(Expression::data const* p) const
{
    if (f_x == p)
    {
        auto step0 = g_x->add(literal1);
        auto step1 = f_x->pow(step0);

        Erase(step0);

        return step1;
    }

    return Expression::data::mul(p);
}

Expression::data const* Pow::commutative_mul(Expression::data const* p) const
{
    if (f_x == p)
    {
        auto step0 = g_x->add(literal1);
        auto step1 = f_x->pow(step0);

        Erase(step0);

        return step1;
    }

    return Expression::data::commutative_mul(p);
}

/***********************************************************************************************************************
*** pow()
***********************************************************************************************************************/

Expression::data const* Expression::data::pow(Expression::data const* p) const
{
    if (p->is(NodeType::CONSTANT))
    {
        auto n = p->evaluate();
        if (n == 0) return Clone(literal1);
        if (n == 1) return Clone(this);
        if (n == 2) return square();
        if (n == -1) return invert();
        if (n == 1.0 / 2.0) return sqrt();
        if (n == 1.0 / 3.0) return cbrt();
    }

    if (powNode.find(p) != powNode.end()) return Clone(powNode[p]);

    return new Pow(Clone(this), Clone(p));
}

Expression::data const* ConstantNode::pow(Expression::data const* p) const
{
    if (p->is(NodeType::CONSTANT)) return constant(std::pow(n, p->evaluate()));
    if (n == 0 && p->guaranteed(Expression::Attribute::NONZERO)) return Clone(this);
    if (n == 1) return Clone(this);
    if (n == std::exp(1)) return p->exp();
    return Expression::data::pow(p);
}

Expression::data const* Sqrt::pow(Expression::data const* p) const  // sqrt(f_x) ^ p  ---->  f_x ^ (p/2)
{
    auto step0 = p->mul(literal2Inv);
    auto step1 = f_x->pow(step0);

    Erase(step0);

    return step1;
}

Expression::data const* Cbrt::pow(Expression::data const* p) const  // cbrt(f_x) ^ p  ---->  f_x ^ (p/3)
{
    auto step0 = p->mul(literal3Inv);
    auto step1 = f_x->pow(step0);

    Erase(step0);

    return step1;
}

Expression::data const* Exp::pow(Expression::data const* p) const  // exp(f_x) ^ p  ---->  exp(f_x*p)
{
    auto step0 = f_x->mul(p);
    auto step1 = step0->exp();

    Erase(step0);

    return step1;
}

Expression::data const* Invert::pow(Expression::data const* p) const  // (1/f_x) ^ p  ---->  1/(f_x^p)
{
    auto step0 = f_x->pow(p);
    auto step1 = step0->invert();

    Erase(step0);

    return step1;
}

Expression::data const* Square::pow(Expression::data const* p) const  // (f_x^2) ^ p  ---->  f_x ^ (2*p)
{
    auto step0 = p->add(p);
    auto step1 = f_x->pow(step0);

    Erase(step0);

    return step1;
}

Expression::data const* Pow::pow(Expression::data const* p) const  // (f_x^g_x) ^ p  ---->  f_x ^ (g_x*p)
{
    auto step0 = g_x->mul(p);
    auto step1 = f_x->pow(step0);

    Erase(step0);

    return step1;
}

/***********************************************************************************************************************
*** derivative()
***********************************************************************************************************************/

Expression::data const* ConstantNode::derivative(Variable const&) const
{
    // D(C) = 0

    return Clone(literal0);
}

Expression::data const* VariableNode::derivative(Variable const& r) const
{
    // D(x) = 1 , D(?) = 0

    return constant(r.id() == x.id());
}

Expression::data const* Abs::derivative(Variable const& r) const
{
    // D(abs(f_x)) = D(f_x) * sign(f_x)

    auto step0 = f_x->derive(r);
    auto step1 = f_x->sign();
    auto step2 = step0->mul(step1);

    Erase(step0);
    Erase(step1);

    return step2;
}

Expression::data const* Sign::derivative(Variable const& r) const
{
    // D(sign(f_x)) = D(f_x) * 0 = 0

    return Clone(literal0);
}

Expression::data const* Sqrt::derivative(Variable const& r) const
{
    // D(sqrt(f_x)) = D(f_x) * 1/(2*sqrt(f_x))

    auto step0 = f_x->derive(r);
    auto step1 = add(this);
    auto step2 = step1->invert();
    auto step3 = step0->mul(step2);

    Erase(step0);
    Erase(step1);
    Erase(step2);

    return step3;
}

Expression::data const* Cbrt::derivative(Variable const& r) const
{
    // D(cbrt(f_x)) = D(f_x) * 1/(3*cbrt(f_x)^2)

    auto step0 = f_x->derive(r);
    auto step1 = square();
    auto step2 = step1->mul(literal3);
    auto step3 = step2->invert();
    auto step4 = step0->mul(step3);

    Erase(step0);
    Erase(step1);
    Erase(step2);
    Erase(step3);

    return step4;
}

Expression::data const* Exp::derivative(Variable const& r) const
{
    // D(exp(f_x)) = D(f_x) * exp(f_x)

    auto step0 = f_x->derive(r);
    auto step1 = mul(step0);

    Erase(step0);

    return step1;
}

Expression::data const* Log::derivative(Variable const& r) const
{
    // D(log(f_x)) = D(f_x) * 1/f_x

    auto step0 = f_x->derive(r);
    auto step1 = f_x->invert();
    auto step2 = step0->mul(step1);

    Erase(step0);
    Erase(step1);

    return step2;
}

Expression::data const* Sin::derivative(Variable const& r) const
{
    // D(sin(f_x)) = D(f_x) * cos(f_x)

    auto step0 = f_x->derive(r);
    auto step1 = f_x->cos();
    auto step2 = step0->mul(step1);

    Erase(step0);
    Erase(step1);

    return step2;
}

Expression::data const* Cos::derivative(Variable const& r) const
{
    // D(cos(f_x)) = D(f_x) * -sin(f_x)

    auto step0 = f_x->derive(r);
    auto step1 = f_x->sin();
    auto step2 = step0->mul(step1);
    auto step3 = step2->negate();

    Erase(step0);
    Erase(step1);
    Erase(step2);

    return step3;
}

Expression::data const* Tan::derivative(Variable const& r) const
{
    // D(tan(f_x)) = D(f_x) * 1/cos(f_x)^2

    auto step0 = f_x->derive(r);
    auto step1 = f_x->sec();
    auto step2 = step1->square();
    auto step3 = step0->mul(step2);

    Erase(step0);
    Erase(step1);
    Erase(step2);

    return step3;
}

Expression::data const* Sec::derivative(Variable const& r) const
{
    // D(sec(f_x)) = D(f_x) * tan(f_x)*sec(f_x)

    auto step0 = f_x->derive(r);
    auto step1 = f_x->tan();
    auto step2 = mul(step1);
    auto step3 = step0->mul(step2);

    Erase(step0);
    Erase(step1);
    Erase(step2);

    return step3;
}

Expression::data const* ASin::derivative(Variable const& r) const
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

Expression::data const* ACos::derivative(Variable const& r) const
{
    // D(acos(f_x)) = D(f_x) * -1/sqrt(1-f_x^2)

    auto step0 = f_x->derive(r);
    auto step1 = f_x->zconic();
    auto step2 = step1->invert();
    auto step3 = step0->mul(step2);
    auto step4 = step3->negate();

    Erase(step0);
    Erase(step1);
    Erase(step2);
    Erase(step3);

    return step4;
}

Expression::data const* ATan::derivative(Variable const& r) const
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

Expression::data const* SinH::derivative(Variable const& r) const
{
    // D(sinh(f_x)) = D(f_x) * cosh(f_x)

    auto step0 = f_x->derive(r);
    auto step1 = f_x->cosh();
    auto step2 = step0->mul(step1);

    Erase(step0);
    Erase(step1);

    return step2;
}

Expression::data const* CosH::derivative(Variable const& r) const
{
    // D(cosh(f_x)) = D(f_x) * sinh(f_x)

    auto step0 = f_x->derive(r);
    auto step1 = f_x->sinh();
    auto step2 = step0->mul(step1);

    Erase(step0);
    Erase(step1);

    return step2;
}

Expression::data const* TanH::derivative(Variable const& r) const
{
    // D(tanh(f_x)) = D(f_x) * 1/cosh(f_x)^2

    auto step0 = f_x->derive(r);
    auto step1 = f_x->cosh();
    auto step2 = step1->square();
    auto step3 = step2->invert();
    auto step4 = step0->mul(step3);

    Erase(step0);
    Erase(step1);
    Erase(step2);
    Erase(step3);

    return step4;
}

Expression::data const* ASinH::derivative(Variable const& r) const
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

Expression::data const* ACosH::derivative(Variable const& r) const
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

Expression::data const* ATanH::derivative(Variable const& r) const
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

Expression::data const* Erf::derivative(Variable const& r) const
{
    // D(erf(f_x)) = D(f_x) * 1/exp(f_x^2) * 1/sqrt(atan(1))

    auto step0 = f_x->derive(r);
    auto step1 = f_x->square();
    auto step2 = step1->exp();
    auto step3 = step2->invert();
    auto step4 = step3->mul(erfDiffConst);
    auto step5 = step0->mul(step4);

    Erase(step0);
    Erase(step1);
    Erase(step2);
    Erase(step3);
    Erase(step4);

    return step5;
}

Expression::data const* Invert::derivative(Variable const& r) const
{
    // D(1/f_x) = D(f_x) * -(1/f_x)^2

    auto step0 = f_x->derive(r);
    auto step1 = square();
    auto step2 = step0->mul(step1);
    auto step3 = step2->negate();

    Erase(step0);
    Erase(step1);
    Erase(step2);

    return step3;
}

Expression::data const* Negate::derivative(Variable const& r) const
{
    // D(-f_x) = D(f_x) * -1

    auto step0 = f_x->derive(r);
    auto step1 = step0->negate();

    Erase(step0);

    return step1;
}

Expression::data const* Square::derivative(Variable const& r) const
{
    // D(f_x^2) = D(f_x) * 2*f_x

    auto step0 = f_x->derive(r);
    auto step1 = f_x->add(f_x);
    auto step2 = step0->mul(step1);

    Erase(step0);
    Erase(step1);

    return step2;
}

Expression::data const* XConic::derivative(Variable const& r) const
{
    // D(sqrt(f_x^2-1)) = D(f_x) * f_x/sqrt(f_x^2-1)

    auto step0 = f_x->derive(r);
    auto step1 = invert();
    auto step2 = step1->mul(f_x);
    auto step3 = step0->mul(step2);

    Erase(step0);
    Erase(step1);
    Erase(step2);

    return step3;
}

Expression::data const* YConic::derivative(Variable const& r) const
{
    // D(sqrt(1+f_x^2)) = D(f_x) * f_x / sqrt(1+f_x^2)

    auto step0 = f_x->derive(r);
    auto step1 = invert();
    auto step2 = step1->mul(f_x);
    auto step3 = step0->mul(step2);

    Erase(step0);
    Erase(step1);
    Erase(step2);

    return step3;
}

Expression::data const* ZConic::derivative(Variable const& r) const
{
    // D(sqrt(1-f_x^2)) = D(f_x) * -f_x / sqrt(1-f_x^2)

    auto step0 = f_x->derive(r);
    auto step1 = invert();
    auto step2 = step1->mul(f_x);
    auto step3 = step0->mul(step2);
    auto step4 = step3->negate();

    Erase(step0);
    Erase(step1);
    Erase(step2);
    Erase(step3);

    return step4;
}

Expression::data const* Add::derivative(Variable const& r) const
{
    // D(f_x+g_x) = D(f_x) + D(g_x)

    auto step0 = f_x->derive(r);
    auto step1 = g_x->derive(r);
    auto step2 = step1->add(step0);

    Erase(step0);
    Erase(step1);

    return step2;
}

Expression::data const* Mul::derivative(Variable const& r) const
{
    // D(f_x*g_x) = D(f_x) * g_x + D(g_x) * f_x

    auto step0 = f_x->derive(r);
    auto step1 = g_x->derive(r);
    auto step2 = step1->mul(f_x);
    auto step3 = step0->mul(g_x);
    auto step4 = step3->add(step2);

    Erase(step0);
    Erase(step1);
    Erase(step2);
    Erase(step3);

    return step4;
}

Expression::data const* Pow::derivative(Variable const& r) const
{
    // D(f_x^g_x) = D(f_x) * g_x*f_x^(g_x-1) + D(g_x) * f_x^g_x*log(f_x)

    auto step0 = f_x->derive(r);
    auto step1 = g_x->derive(r);
    auto step2 = f_x->log();
    auto step3 = g_x->add(literal1Neg);
    auto step4 = f_x->pow(step3);
    auto step5 = g_x->mul(step4);
    auto step6 = mul(step2);
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

double ConstantNode::value() const
{
    return n;
}

double VariableNode::value() const
{
    return double(x);
}

double Abs::value() const
{
    return std::abs(f_x->evaluate());
}

double Sign::value() const
{
    return std::sign(f_x->evaluate());
}

double Sqrt::value() const
{
    return std::sqrt(f_x->evaluate());
}

double Cbrt::value() const
{
    return std::cbrt(f_x->evaluate());
}

double Exp::value() const
{
    return std::exp(f_x->evaluate());
}

double Log::value() const
{
    return std::log(f_x->evaluate());
}

double Sin::value() const
{
    return std::sin(f_x->evaluate());
}

double Cos::value() const
{
    return std::cos(f_x->evaluate());
}

double Tan::value() const
{
    return std::tan(f_x->evaluate());
}

double Sec::value() const
{
    return 1 / std::cos(f_x->evaluate());
}

double ASin::value() const
{
    return std::asin(f_x->evaluate());
}

double ACos::value() const
{
    return std::acos(f_x->evaluate());
}

double ATan::value() const
{
    return std::atan(f_x->evaluate());
}

double SinH::value() const
{
    return std::sinh(f_x->evaluate());
}

double CosH::value() const
{
    return std::cosh(f_x->evaluate());
}

double TanH::value() const
{
    return std::tanh(f_x->evaluate());
}

double ASinH::value() const
{
    return std::asinh(f_x->evaluate());
}

double ACosH::value() const
{
    return std::acosh(f_x->evaluate());
}

double ATanH::value() const
{
    return std::atanh(f_x->evaluate());
}

double Erf::value() const
{
    return std::erf(f_x->evaluate());
}

double Invert::value() const
{
    return 1 / f_x->evaluate();
}

double Negate::value() const
{
    return -f_x->evaluate();
}

double Square::value() const
{
    auto x = f_x->evaluate();
    return x * x;
}

double XConic::value() const
{
    auto x = f_x->evaluate();
    return std::sqrt(x * x - 1);
}

double YConic::value() const
{
    auto x = f_x->evaluate();
    return std::sqrt(1 + x * x);
}

double ZConic::value() const
{
    auto x = f_x->evaluate();
    return std::sqrt(1 - x * x);
}

double Add::value() const
{
    return f_x->evaluate() + g_x->evaluate();
}

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

double Pow::value() const
{
    return std::pow(f_x->evaluate(), g_x->evaluate());
}

/***********************************************************************************************************************
*** guaranteed()
***********************************************************************************************************************/

bool ConstantNode::guaranteed(Expression::Attribute a) const
{
    if (!isnan(n) && !isinf(n)) switch (a)
    {
    case Expression::Attribute::DEFINED:
    case Expression::Attribute::CONTINUOUS:
    case Expression::Attribute::NONINCREASING:
    case Expression::Attribute::NONDECREASING:
    case Expression::Attribute::BOUNDEDABOVE:
    case Expression::Attribute::BOUNDEDBELOW:
        return true;

    case Expression::Attribute::NONZERO:
        return n != 0;

    case Expression::Attribute::POSITIVE:
        return n > 0;

    case Expression::Attribute::NEGATIVE:
        return n < 0;

    case Expression::Attribute::NONPOSITIVE:
        return n <= 0;

    case Expression::Attribute::NONNEGATIVE:
        return n >= 0;

    case Expression::Attribute::UNITRANGE:
        return n >= -1 && n <= 1;

    case Expression::Attribute::ANTIUNITRANGE:
        return n < -1 || n > 1;

    case Expression::Attribute::OPENUNITRANGE:
        return n > -1 && n < 1;

    case Expression::Attribute::ANTIOPENUNITRANGE:
        return n <= -1 || n >= 1;
    }

    return false;
}

bool VariableNode::guaranteed(Expression::Attribute a) const
{
    switch (a)
    {
    case Expression::Attribute::DEFINED:
    case Expression::Attribute::CONTINUOUS:
    case Expression::Attribute::INCREASING:
    case Expression::Attribute::NONDECREASING:
        return true;
    }

    return false;
}

bool Abs::guaranteed(Expression::Attribute a) const
{
    if (f_x->guaranteed(Expression::Attribute::DEFINED)) switch (a)
    {
    case Expression::Attribute::DEFINED:
    case Expression::Attribute::NONNEGATIVE:
    case Expression::Attribute::BOUNDEDBELOW:
        return true;

    case Expression::Attribute::NONZERO:
    case Expression::Attribute::UNITRANGE:
    case Expression::Attribute::ANTIUNITRANGE:
    case Expression::Attribute::OPENUNITRANGE:
    case Expression::Attribute::ANTIOPENUNITRANGE:
    case Expression::Attribute::CONTINUOUS:
        return f_x->guaranteed(a);

    case Expression::Attribute::POSITIVE:
        return f_x->guaranteed(Expression::Attribute::NONZERO);

    case Expression::Attribute::INCREASING:
        if (f_x->guaranteed(Expression::Attribute::INCREASING) && f_x->guaranteed(Expression::Attribute::POSITIVE)) return true;
        if (f_x->guaranteed(Expression::Attribute::DECREASING) && f_x->guaranteed(Expression::Attribute::NEGATIVE)) return true;
        return false;

    case Expression::Attribute::DECREASING:
        if (f_x->guaranteed(Expression::Attribute::DECREASING) && f_x->guaranteed(Expression::Attribute::POSITIVE)) return true;
        if (f_x->guaranteed(Expression::Attribute::INCREASING) && f_x->guaranteed(Expression::Attribute::NEGATIVE)) return true;
        return false;

    case Expression::Attribute::NONINCREASING:
        if (f_x->guaranteed(Expression::Attribute::NONINCREASING) && f_x->guaranteed(Expression::Attribute::NONNEGATIVE)) return true;
        if (f_x->guaranteed(Expression::Attribute::NONDECREASING) && f_x->guaranteed(Expression::Attribute::NONPOSITIVE)) return true;
        return false;

    case Expression::Attribute::NONDECREASING:
        if (f_x->guaranteed(Expression::Attribute::NONDECREASING) && f_x->guaranteed(Expression::Attribute::NONNEGATIVE)) return true;
        if (f_x->guaranteed(Expression::Attribute::NONINCREASING) && f_x->guaranteed(Expression::Attribute::NONPOSITIVE)) return true;
        return false;

    case Expression::Attribute::BOUNDEDABOVE:
        return f_x->guaranteed(Expression::Attribute::BOUNDEDABOVE) && f_x->guaranteed(Expression::Attribute::BOUNDEDBELOW);
    }

    return false;
}

bool Sign::guaranteed(Expression::Attribute a) const
{
    if (f_x->guaranteed(Expression::Attribute::DEFINED)) switch (a)
    {
    case Expression::Attribute::DEFINED:
    case Expression::Attribute::UNITRANGE:
    case Expression::Attribute::BOUNDEDABOVE:
    case Expression::Attribute::BOUNDEDBELOW:
        return true;

    case Expression::Attribute::NONZERO:
    case Expression::Attribute::POSITIVE:
    case Expression::Attribute::NEGATIVE:
    case Expression::Attribute::NONPOSITIVE:
    case Expression::Attribute::NONNEGATIVE:
        return f_x->guaranteed(a);

    case Expression::Attribute::ANTIOPENUNITRANGE:
        return f_x->guaranteed(Expression::Attribute::NONZERO);

    case Expression::Attribute::CONTINUOUS:
        return f_x->guaranteed(Expression::Attribute::POSITIVE) || f_x->guaranteed(Expression::Attribute::NEGATIVE);

    case Expression::Attribute::NONINCREASING:
        return f_x->guaranteed(Expression::Attribute::NONINCREASING) || f_x->guaranteed(Expression::Attribute::POSITIVE) || f_x->guaranteed(Expression::Attribute::NEGATIVE);

    case Expression::Attribute::NONDECREASING:
        return f_x->guaranteed(Expression::Attribute::NONDECREASING) || f_x->guaranteed(Expression::Attribute::POSITIVE) || f_x->guaranteed(Expression::Attribute::NEGATIVE);
    }

    return false;
}

bool Sqrt::guaranteed(Expression::Attribute a) const
{
    if (f_x->guaranteed(Expression::Attribute::NONNEGATIVE)) switch (a)
    {
    case Expression::Attribute::DEFINED:
    case Expression::Attribute::NONNEGATIVE:
    case Expression::Attribute::BOUNDEDBELOW:
        return true;

    case Expression::Attribute::NONZERO:
    case Expression::Attribute::POSITIVE:
    case Expression::Attribute::UNITRANGE:
    case Expression::Attribute::ANTIUNITRANGE:
    case Expression::Attribute::OPENUNITRANGE:
    case Expression::Attribute::ANTIOPENUNITRANGE:
    case Expression::Attribute::CONTINUOUS:
    case Expression::Attribute::INCREASING:
    case Expression::Attribute::DECREASING:
    case Expression::Attribute::NONINCREASING:
    case Expression::Attribute::NONDECREASING:
    case Expression::Attribute::BOUNDEDABOVE:
        return f_x->guaranteed(a);
    }

    return false;
}

bool Cbrt::guaranteed(Expression::Attribute a) const
{
    if (f_x->guaranteed(Expression::Attribute::DEFINED)) switch (a)
    {
    case Expression::Attribute::DEFINED:
        return true;

    case Expression::Attribute::NONZERO:
    case Expression::Attribute::POSITIVE:
    case Expression::Attribute::NEGATIVE:
    case Expression::Attribute::NONPOSITIVE:
    case Expression::Attribute::NONNEGATIVE:
    case Expression::Attribute::UNITRANGE:
    case Expression::Attribute::ANTIUNITRANGE:
    case Expression::Attribute::OPENUNITRANGE:
    case Expression::Attribute::ANTIOPENUNITRANGE:
    case Expression::Attribute::CONTINUOUS:
    case Expression::Attribute::INCREASING:
    case Expression::Attribute::DECREASING:
    case Expression::Attribute::NONINCREASING:
    case Expression::Attribute::NONDECREASING:
    case Expression::Attribute::BOUNDEDABOVE:
    case Expression::Attribute::BOUNDEDBELOW:
        return f_x->guaranteed(a);
    }

    return false;
}

bool Exp::guaranteed(Expression::Attribute a) const
{
    if (f_x->guaranteed(Expression::Attribute::DEFINED)) switch (a)
    {
    case Expression::Attribute::DEFINED:
    case Expression::Attribute::NONZERO:
    case Expression::Attribute::POSITIVE:
    case Expression::Attribute::NONNEGATIVE:
    case Expression::Attribute::BOUNDEDBELOW:
        return true;

    case Expression::Attribute::CONTINUOUS:
    case Expression::Attribute::INCREASING:
    case Expression::Attribute::DECREASING:
    case Expression::Attribute::NONINCREASING:
    case Expression::Attribute::NONDECREASING:
    case Expression::Attribute::BOUNDEDABOVE:
        return f_x->guaranteed(a);

    case Expression::Attribute::UNITRANGE:
        return f_x->guaranteed(Expression::Attribute::NONPOSITIVE);

    case Expression::Attribute::ANTIUNITRANGE:
        return f_x->guaranteed(Expression::Attribute::POSITIVE);

    case Expression::Attribute::OPENUNITRANGE:
        return f_x->guaranteed(Expression::Attribute::NEGATIVE);

    case Expression::Attribute::ANTIOPENUNITRANGE:
        return f_x->guaranteed(Expression::Attribute::NONNEGATIVE);
    }

    return false;
}

bool Log::guaranteed(Expression::Attribute a) const
{
    if (f_x->guaranteed(Expression::Attribute::POSITIVE)) switch (a)
    {
    case Expression::Attribute::DEFINED:
        return true;

    case Expression::Attribute::CONTINUOUS:
    case Expression::Attribute::INCREASING:
    case Expression::Attribute::DECREASING:
    case Expression::Attribute::NONINCREASING:
    case Expression::Attribute::NONDECREASING:
    case Expression::Attribute::BOUNDEDABOVE:
        return f_x->guaranteed(a);

    case Expression::Attribute::NONZERO:
        return f_x->guaranteed(Expression::Attribute::ANTIUNITRANGE) || f_x->guaranteed(Expression::Attribute::OPENUNITRANGE);

    case Expression::Attribute::POSITIVE:
        return f_x->guaranteed(Expression::Attribute::ANTIUNITRANGE);

    case Expression::Attribute::NEGATIVE:
        return f_x->guaranteed(Expression::Attribute::OPENUNITRANGE);

    case Expression::Attribute::NONPOSITIVE:
        return f_x->guaranteed(Expression::Attribute::UNITRANGE);

    case Expression::Attribute::NONNEGATIVE:
        return f_x->guaranteed(Expression::Attribute::ANTIOPENUNITRANGE);
    }

    return false;
}

bool Sin::guaranteed(Expression::Attribute a) const
{
    if (f_x->guaranteed(Expression::Attribute::DEFINED)) switch (a)
    {
    case Expression::Attribute::DEFINED:
    case Expression::Attribute::UNITRANGE:
    case Expression::Attribute::BOUNDEDABOVE:
    case Expression::Attribute::BOUNDEDBELOW:
        return true;

    case Expression::Attribute::CONTINUOUS:
        return f_x->guaranteed(a);
    }

    return false;
}

bool Cos::guaranteed(Expression::Attribute a) const
{
    if (f_x->guaranteed(Expression::Attribute::DEFINED)) switch (a)
    {
    case Expression::Attribute::DEFINED:
    case Expression::Attribute::UNITRANGE:
    case Expression::Attribute::BOUNDEDABOVE:
    case Expression::Attribute::BOUNDEDBELOW:
        return true;

    case Expression::Attribute::CONTINUOUS:
        return f_x->guaranteed(a);
    }

    return false;
}

bool Tan::guaranteed(Expression::Attribute a) const
{
    if (f_x->guaranteed(Expression::Attribute::DEFINED)) switch (a)
    {
    case Expression::Attribute::DEFINED:
        return true;
    }

    return false;
}

bool Sec::guaranteed(Expression::Attribute a) const
{
    return false;
}

bool ASin::guaranteed(Expression::Attribute a) const
{
    if (f_x->guaranteed(Expression::Attribute::UNITRANGE)) switch (a)
    {
    case Expression::Attribute::DEFINED:
    case Expression::Attribute::BOUNDEDABOVE:
    case Expression::Attribute::BOUNDEDBELOW:
        return true;

    case Expression::Attribute::NONZERO:
    case Expression::Attribute::POSITIVE:
    case Expression::Attribute::NEGATIVE:
    case Expression::Attribute::NONPOSITIVE:
    case Expression::Attribute::NONNEGATIVE:
    case Expression::Attribute::CONTINUOUS:
    case Expression::Attribute::INCREASING:
    case Expression::Attribute::DECREASING:
    case Expression::Attribute::NONINCREASING:
    case Expression::Attribute::NONDECREASING:
        return f_x->guaranteed(a);
    }

    return false;
}

bool ACos::guaranteed(Expression::Attribute a) const
{
    if (f_x->guaranteed(Expression::Attribute::UNITRANGE)) switch (a)
    {
    case Expression::Attribute::DEFINED:
    case Expression::Attribute::NONNEGATIVE:
    case Expression::Attribute::BOUNDEDABOVE:
    case Expression::Attribute::BOUNDEDBELOW:
        return true;

    case Expression::Attribute::CONTINUOUS:
        return f_x->guaranteed(a);

    case Expression::Attribute::NONZERO:
    case Expression::Attribute::POSITIVE:
        return f_x->guaranteed(Expression::Attribute::OPENUNITRANGE);

    case Expression::Attribute::NONPOSITIVE:
        return f_x->guaranteed(Expression::Attribute::POSITIVE) && f_x->guaranteed(Expression::Attribute::ANTIOPENUNITRANGE);

    case Expression::Attribute::INCREASING:
        return f_x->guaranteed(Expression::Attribute::DECREASING);

    case Expression::Attribute::DECREASING:
        return f_x->guaranteed(Expression::Attribute::INCREASING);

    case Expression::Attribute::NONINCREASING:
        return f_x->guaranteed(Expression::Attribute::NONDECREASING);

    case Expression::Attribute::NONDECREASING:
        return f_x->guaranteed(Expression::Attribute::NONINCREASING);
    }

    return false;
}

bool ATan::guaranteed(Expression::Attribute a) const
{
    if (f_x->guaranteed(Expression::Attribute::DEFINED)) switch (a)
    {
    case Expression::Attribute::DEFINED:
    case Expression::Attribute::BOUNDEDABOVE:
    case Expression::Attribute::BOUNDEDBELOW:
        return true;

    case Expression::Attribute::NONZERO:
    case Expression::Attribute::POSITIVE:
    case Expression::Attribute::NEGATIVE:
    case Expression::Attribute::NONPOSITIVE:
    case Expression::Attribute::NONNEGATIVE:
    case Expression::Attribute::CONTINUOUS:
    case Expression::Attribute::INCREASING:
    case Expression::Attribute::DECREASING:
    case Expression::Attribute::NONINCREASING:
    case Expression::Attribute::NONDECREASING:
        return f_x->guaranteed(a);
    }

    return false;
}

bool SinH::guaranteed(Expression::Attribute a) const
{
    if (f_x->guaranteed(Expression::Attribute::DEFINED)) switch (a)
    {
    case Expression::Attribute::DEFINED:
        return true;

    case Expression::Attribute::NONZERO:
    case Expression::Attribute::POSITIVE:
    case Expression::Attribute::NEGATIVE:
    case Expression::Attribute::NONPOSITIVE:
    case Expression::Attribute::NONNEGATIVE:
    case Expression::Attribute::CONTINUOUS:
    case Expression::Attribute::INCREASING:
    case Expression::Attribute::DECREASING:
    case Expression::Attribute::NONINCREASING:
    case Expression::Attribute::NONDECREASING:
    case Expression::Attribute::BOUNDEDABOVE:
    case Expression::Attribute::BOUNDEDBELOW:
        return f_x->guaranteed(a);
    }

    return false;
}

bool CosH::guaranteed(Expression::Attribute a) const
{
    if (f_x->guaranteed(Expression::Attribute::DEFINED)) switch (a)
    {
    case Expression::Attribute::DEFINED:
    case Expression::Attribute::NONZERO:
    case Expression::Attribute::POSITIVE:
    case Expression::Attribute::NONNEGATIVE:
    case Expression::Attribute::ANTIOPENUNITRANGE:
    case Expression::Attribute::BOUNDEDBELOW:
        return true;

    case Expression::Attribute::CONTINUOUS:
        return f_x->guaranteed(a);

    case Expression::Attribute::ANTIUNITRANGE:
        return f_x->guaranteed(Expression::Attribute::NONZERO);

    case Expression::Attribute::INCREASING:
        if (f_x->guaranteed(Expression::Attribute::INCREASING) && f_x->guaranteed(Expression::Attribute::POSITIVE)) return true;
        if (f_x->guaranteed(Expression::Attribute::DECREASING) && f_x->guaranteed(Expression::Attribute::NEGATIVE)) return true;
        return false;

    case Expression::Attribute::DECREASING:
        if (f_x->guaranteed(Expression::Attribute::DECREASING) && f_x->guaranteed(Expression::Attribute::POSITIVE)) return true;
        if (f_x->guaranteed(Expression::Attribute::INCREASING) && f_x->guaranteed(Expression::Attribute::NEGATIVE)) return true;
        return false;

    case Expression::Attribute::NONINCREASING:
        if (f_x->guaranteed(Expression::Attribute::NONINCREASING) && f_x->guaranteed(Expression::Attribute::NONNEGATIVE)) return true;
        if (f_x->guaranteed(Expression::Attribute::NONDECREASING) && f_x->guaranteed(Expression::Attribute::NONPOSITIVE)) return true;
        return false;

    case Expression::Attribute::NONDECREASING:
        if (f_x->guaranteed(Expression::Attribute::NONDECREASING) && f_x->guaranteed(Expression::Attribute::NONNEGATIVE)) return true;
        if (f_x->guaranteed(Expression::Attribute::NONINCREASING) && f_x->guaranteed(Expression::Attribute::NONPOSITIVE)) return true;
        return false;

    case Expression::Attribute::BOUNDEDABOVE:
        return f_x->guaranteed(Expression::Attribute::BOUNDEDABOVE) && f_x->guaranteed(Expression::Attribute::BOUNDEDBELOW);
    }

    return false;
}

bool TanH::guaranteed(Expression::Attribute a) const
{
    if (f_x->guaranteed(Expression::Attribute::DEFINED)) switch (a)
    {
    case Expression::Attribute::DEFINED:
    case Expression::Attribute::UNITRANGE:
    case Expression::Attribute::OPENUNITRANGE:
    case Expression::Attribute::BOUNDEDABOVE:
    case Expression::Attribute::BOUNDEDBELOW:
        return true;

    case Expression::Attribute::NONZERO:
    case Expression::Attribute::POSITIVE:
    case Expression::Attribute::NEGATIVE:
    case Expression::Attribute::NONPOSITIVE:
    case Expression::Attribute::NONNEGATIVE:
    case Expression::Attribute::CONTINUOUS:
    case Expression::Attribute::INCREASING:
    case Expression::Attribute::DECREASING:
    case Expression::Attribute::NONINCREASING:
    case Expression::Attribute::NONDECREASING:
        return f_x->guaranteed(a);
    }

    return false;
}

bool ASinH::guaranteed(Expression::Attribute a) const
{
    if (f_x->guaranteed(Expression::Attribute::DEFINED)) switch (a)
    {
    case Expression::Attribute::DEFINED:
        return true;

    case Expression::Attribute::NONZERO:
    case Expression::Attribute::POSITIVE:
    case Expression::Attribute::NEGATIVE:
    case Expression::Attribute::NONPOSITIVE:
    case Expression::Attribute::NONNEGATIVE:
    case Expression::Attribute::CONTINUOUS:
    case Expression::Attribute::INCREASING:
    case Expression::Attribute::DECREASING:
    case Expression::Attribute::NONINCREASING:
    case Expression::Attribute::NONDECREASING:
    case Expression::Attribute::BOUNDEDABOVE:
    case Expression::Attribute::BOUNDEDBELOW:
        return f_x->guaranteed(a);
    }

    return false;
}

bool ACosH::guaranteed(Expression::Attribute a) const
{
    if (f_x->guaranteed(Expression::Attribute::POSITIVE) && f_x->guaranteed(Expression::Attribute::ANTIOPENUNITRANGE)) switch (a)
    {
    case Expression::Attribute::DEFINED:
    case Expression::Attribute::NONNEGATIVE:
    case Expression::Attribute::BOUNDEDBELOW:
        return true;

    case Expression::Attribute::CONTINUOUS:
    case Expression::Attribute::INCREASING:
    case Expression::Attribute::DECREASING:
    case Expression::Attribute::NONINCREASING:
    case Expression::Attribute::NONDECREASING:
    case Expression::Attribute::BOUNDEDABOVE:
        return f_x->guaranteed(a);

    case Expression::Attribute::NONZERO:
    case Expression::Attribute::POSITIVE:
        return f_x->guaranteed(Expression::Attribute::ANTIUNITRANGE);
    }

    return false;
}

bool ATanH::guaranteed(Expression::Attribute a) const
{
    if (f_x->guaranteed(Expression::Attribute::OPENUNITRANGE)) switch (a)
    {
    case Expression::Attribute::DEFINED:
        return true;

    case Expression::Attribute::NONZERO:
    case Expression::Attribute::POSITIVE:
    case Expression::Attribute::NEGATIVE:
    case Expression::Attribute::NONPOSITIVE:
    case Expression::Attribute::NONNEGATIVE:
    case Expression::Attribute::CONTINUOUS:
    case Expression::Attribute::INCREASING:
    case Expression::Attribute::DECREASING:
    case Expression::Attribute::NONINCREASING:
    case Expression::Attribute::NONDECREASING:
        return f_x->guaranteed(a);
    }

    return false;
}

bool Erf::guaranteed(Expression::Attribute a) const
{
    if (f_x->guaranteed(Expression::Attribute::DEFINED)) switch (a)
    {
    case Expression::Attribute::DEFINED:
    case Expression::Attribute::UNITRANGE:
    case Expression::Attribute::OPENUNITRANGE:
    case Expression::Attribute::BOUNDEDABOVE:
    case Expression::Attribute::BOUNDEDBELOW:
        return true;

    case Expression::Attribute::NONZERO:
    case Expression::Attribute::POSITIVE:
    case Expression::Attribute::NEGATIVE:
    case Expression::Attribute::NONPOSITIVE:
    case Expression::Attribute::NONNEGATIVE:
    case Expression::Attribute::CONTINUOUS:
    case Expression::Attribute::INCREASING:
    case Expression::Attribute::DECREASING:
    case Expression::Attribute::NONINCREASING:
    case Expression::Attribute::NONDECREASING:
        return f_x->guaranteed(a);
    }

    return false;
}

bool Invert::guaranteed(Expression::Attribute a) const
{
    if (f_x->guaranteed(Expression::Attribute::NONZERO)) switch (a)
    {
    case Expression::Attribute::DEFINED:
    case Expression::Attribute::NONZERO:
        return true;

    case Expression::Attribute::POSITIVE:
    case Expression::Attribute::NEGATIVE:
    case Expression::Attribute::NONPOSITIVE:
    case Expression::Attribute::NONNEGATIVE:
        return f_x->guaranteed(a);

    case Expression::Attribute::UNITRANGE:
        return f_x->guaranteed(Expression::Attribute::ANTIOPENUNITRANGE);

    case Expression::Attribute::ANTIUNITRANGE:
        return f_x->guaranteed(Expression::Attribute::OPENUNITRANGE);

    case Expression::Attribute::OPENUNITRANGE:
        return f_x->guaranteed(Expression::Attribute::ANTIUNITRANGE);

    case Expression::Attribute::ANTIOPENUNITRANGE:
        return f_x->guaranteed(Expression::Attribute::UNITRANGE);

    case Expression::Attribute::CONTINUOUS:
        return f_x->guaranteed(Expression::Attribute::POSITIVE) || f_x->guaranteed(Expression::Attribute::NEGATIVE);

    case Expression::Attribute::INCREASING:
        return f_x->guaranteed(Expression::Attribute::DECREASING) && (f_x->guaranteed(Expression::Attribute::POSITIVE) || f_x->guaranteed(Expression::Attribute::NEGATIVE));

    case Expression::Attribute::DECREASING:
        return f_x->guaranteed(Expression::Attribute::INCREASING) && (f_x->guaranteed(Expression::Attribute::POSITIVE) || f_x->guaranteed(Expression::Attribute::NEGATIVE));

    case Expression::Attribute::NONINCREASING:
        return f_x->guaranteed(Expression::Attribute::NONDECREASING) && (f_x->guaranteed(Expression::Attribute::POSITIVE) || f_x->guaranteed(Expression::Attribute::NEGATIVE));

    case Expression::Attribute::NONDECREASING:
        return f_x->guaranteed(Expression::Attribute::NONINCREASING) && (f_x->guaranteed(Expression::Attribute::POSITIVE) || f_x->guaranteed(Expression::Attribute::NEGATIVE));
    }

    return false;
}

bool Negate::guaranteed(Expression::Attribute a) const
{
    if (f_x->guaranteed(Expression::Attribute::DEFINED)) switch (a)
    {
    case Expression::Attribute::DEFINED:
        return true;

    case Expression::Attribute::NONZERO:
    case Expression::Attribute::UNITRANGE:
    case Expression::Attribute::ANTIUNITRANGE:
    case Expression::Attribute::OPENUNITRANGE:
    case Expression::Attribute::ANTIOPENUNITRANGE:
    case Expression::Attribute::CONTINUOUS:
        return f_x->guaranteed(a);

    case Expression::Attribute::POSITIVE:
        return f_x->guaranteed(Expression::Attribute::NEGATIVE);

    case Expression::Attribute::NEGATIVE:
        return f_x->guaranteed(Expression::Attribute::POSITIVE);

    case Expression::Attribute::NONPOSITIVE:
        return f_x->guaranteed(Expression::Attribute::NONNEGATIVE);

    case Expression::Attribute::NONNEGATIVE:
        return f_x->guaranteed(Expression::Attribute::NONPOSITIVE);

    case Expression::Attribute::INCREASING:
        return f_x->guaranteed(Expression::Attribute::DECREASING);

    case Expression::Attribute::DECREASING:
        return f_x->guaranteed(Expression::Attribute::INCREASING);

    case Expression::Attribute::NONINCREASING:
        return f_x->guaranteed(Expression::Attribute::NONDECREASING);

    case Expression::Attribute::NONDECREASING:
        return f_x->guaranteed(Expression::Attribute::NONINCREASING);

    case Expression::Attribute::BOUNDEDABOVE:
        return f_x->guaranteed(Expression::Attribute::BOUNDEDBELOW);

    case Expression::Attribute::BOUNDEDBELOW:
        return f_x->guaranteed(Expression::Attribute::BOUNDEDABOVE);
    }

    return false;
}

bool Square::guaranteed(Expression::Attribute a) const
{
    if (f_x->guaranteed(Expression::Attribute::DEFINED)) switch (a)
    {
    case Expression::Attribute::DEFINED:
    case Expression::Attribute::NONNEGATIVE:
    case Expression::Attribute::BOUNDEDBELOW:
        return true;

    case Expression::Attribute::NONZERO:
    case Expression::Attribute::UNITRANGE:
    case Expression::Attribute::ANTIUNITRANGE:
    case Expression::Attribute::OPENUNITRANGE:
    case Expression::Attribute::ANTIOPENUNITRANGE:
    case Expression::Attribute::CONTINUOUS:
        return f_x->guaranteed(a);

    case Expression::Attribute::POSITIVE:
        return f_x->guaranteed(Expression::Attribute::NONZERO);

    case Expression::Attribute::INCREASING:
        if (f_x->guaranteed(Expression::Attribute::INCREASING) && f_x->guaranteed(Expression::Attribute::POSITIVE)) return true;
        if (f_x->guaranteed(Expression::Attribute::DECREASING) && f_x->guaranteed(Expression::Attribute::NEGATIVE)) return true;
        return false;

    case Expression::Attribute::DECREASING:
        if (f_x->guaranteed(Expression::Attribute::DECREASING) && f_x->guaranteed(Expression::Attribute::POSITIVE)) return true;
        if (f_x->guaranteed(Expression::Attribute::INCREASING) && f_x->guaranteed(Expression::Attribute::NEGATIVE)) return true;
        return false;

    case Expression::Attribute::NONINCREASING:
        if (f_x->guaranteed(Expression::Attribute::NONINCREASING) && f_x->guaranteed(Expression::Attribute::NONNEGATIVE)) return true;
        if (f_x->guaranteed(Expression::Attribute::NONDECREASING) && f_x->guaranteed(Expression::Attribute::NONPOSITIVE)) return true;
        return false;

    case Expression::Attribute::NONDECREASING:
        if (f_x->guaranteed(Expression::Attribute::NONDECREASING) && f_x->guaranteed(Expression::Attribute::NONNEGATIVE)) return true;
        if (f_x->guaranteed(Expression::Attribute::NONINCREASING) && f_x->guaranteed(Expression::Attribute::NONPOSITIVE)) return true;
        return false;

    case Expression::Attribute::BOUNDEDABOVE:
        return f_x->guaranteed(Expression::Attribute::BOUNDEDABOVE) && f_x->guaranteed(Expression::Attribute::BOUNDEDBELOW);
    }

    return false;
}

bool XConic::guaranteed(Expression::Attribute a) const
{
    if (f_x->guaranteed(Expression::Attribute::POSITIVE) && f_x->guaranteed(Expression::Attribute::ANTIOPENUNITRANGE)) switch (a)
    {
    case Expression::Attribute::DEFINED:
    case Expression::Attribute::NONNEGATIVE:
    case Expression::Attribute::BOUNDEDBELOW:
        return true;

    case Expression::Attribute::CONTINUOUS:
    case Expression::Attribute::INCREASING:
    case Expression::Attribute::DECREASING:
    case Expression::Attribute::NONINCREASING:
    case Expression::Attribute::NONDECREASING:
    case Expression::Attribute::BOUNDEDABOVE:
        return f_x->guaranteed(a);

    case Expression::Attribute::NONZERO:
    case Expression::Attribute::POSITIVE:
        return f_x->guaranteed(Expression::Attribute::ANTIUNITRANGE);
    }

    return false;
}

bool YConic::guaranteed(Expression::Attribute a) const
{
    if (f_x->guaranteed(Expression::Attribute::DEFINED)) switch (a)
    {
    case Expression::Attribute::DEFINED:
    case Expression::Attribute::NONZERO:
    case Expression::Attribute::POSITIVE:
    case Expression::Attribute::NONNEGATIVE:
    case Expression::Attribute::ANTIOPENUNITRANGE:
    case Expression::Attribute::BOUNDEDBELOW:
        return true;

    case Expression::Attribute::CONTINUOUS:
        return f_x->guaranteed(a);

    case Expression::Attribute::ANTIUNITRANGE:
        return f_x->guaranteed(Expression::Attribute::NONZERO);

    case Expression::Attribute::INCREASING:
        if (f_x->guaranteed(Expression::Attribute::INCREASING) && f_x->guaranteed(Expression::Attribute::POSITIVE)) return true;
        if (f_x->guaranteed(Expression::Attribute::DECREASING) && f_x->guaranteed(Expression::Attribute::NEGATIVE)) return true;
        return false;

    case Expression::Attribute::DECREASING:
        if (f_x->guaranteed(Expression::Attribute::DECREASING) && f_x->guaranteed(Expression::Attribute::POSITIVE)) return true;
        if (f_x->guaranteed(Expression::Attribute::INCREASING) && f_x->guaranteed(Expression::Attribute::NEGATIVE)) return true;
        return false;

    case Expression::Attribute::NONINCREASING:
        if (f_x->guaranteed(Expression::Attribute::NONINCREASING) && f_x->guaranteed(Expression::Attribute::NONNEGATIVE)) return true;
        if (f_x->guaranteed(Expression::Attribute::NONDECREASING) && f_x->guaranteed(Expression::Attribute::NONPOSITIVE)) return true;
        return false;

    case Expression::Attribute::NONDECREASING:
        if (f_x->guaranteed(Expression::Attribute::NONDECREASING) && f_x->guaranteed(Expression::Attribute::NONNEGATIVE)) return true;
        if (f_x->guaranteed(Expression::Attribute::NONINCREASING) && f_x->guaranteed(Expression::Attribute::NONPOSITIVE)) return true;
        return false;

    case Expression::Attribute::BOUNDEDABOVE:
        return f_x->guaranteed(Expression::Attribute::BOUNDEDABOVE) && f_x->guaranteed(Expression::Attribute::BOUNDEDBELOW);
    }

    return false;
}

bool ZConic::guaranteed(Expression::Attribute a) const
{
    if (f_x->guaranteed(Expression::Attribute::UNITRANGE)) switch (a)
    {
    case Expression::Attribute::DEFINED:
    case Expression::Attribute::NONNEGATIVE:
    case Expression::Attribute::UNITRANGE:
    case Expression::Attribute::BOUNDEDABOVE:
    case Expression::Attribute::BOUNDEDBELOW:
        return true;

    case Expression::Attribute::CONTINUOUS:
        return f_x->guaranteed(a);

    case Expression::Attribute::NONZERO:
    case Expression::Attribute::POSITIVE:
        return f_x->guaranteed(Expression::Attribute::OPENUNITRANGE);

    case Expression::Attribute::NONPOSITIVE:
        return f_x->guaranteed(Expression::Attribute::ANTIOPENUNITRANGE);

    case Expression::Attribute::OPENUNITRANGE:
        return f_x->guaranteed(Expression::Attribute::NONZERO);

    case Expression::Attribute::INCREASING:
        if (f_x->guaranteed(Expression::Attribute::INCREASING) && f_x->guaranteed(Expression::Attribute::NEGATIVE)) return true;
        if (f_x->guaranteed(Expression::Attribute::DECREASING) && f_x->guaranteed(Expression::Attribute::POSITIVE)) return true;
        return false;

    case Expression::Attribute::DECREASING:
        if (f_x->guaranteed(Expression::Attribute::DECREASING) && f_x->guaranteed(Expression::Attribute::NEGATIVE)) return true;
        if (f_x->guaranteed(Expression::Attribute::INCREASING) && f_x->guaranteed(Expression::Attribute::POSITIVE)) return true;
        return false;

    case Expression::Attribute::NONINCREASING:
        if (f_x->guaranteed(Expression::Attribute::NONINCREASING) && f_x->guaranteed(Expression::Attribute::NONPOSITIVE)) return true;
        if (f_x->guaranteed(Expression::Attribute::NONDECREASING) && f_x->guaranteed(Expression::Attribute::NONNEGATIVE)) return true;
        return false;

    case Expression::Attribute::NONDECREASING:
        if (f_x->guaranteed(Expression::Attribute::NONDECREASING) && f_x->guaranteed(Expression::Attribute::NONPOSITIVE)) return true;
        if (f_x->guaranteed(Expression::Attribute::NONINCREASING) && f_x->guaranteed(Expression::Attribute::NONNEGATIVE)) return true;
        return false;
    }

    return false;
}

bool Add::guaranteed(Expression::Attribute a) const
{
    if (f_x->guaranteed(Expression::Attribute::DEFINED) && g_x->guaranteed(Expression::Attribute::DEFINED)) switch (a)
    {
    case Expression::Attribute::DEFINED:
        return true;

    case Expression::Attribute::NONZERO:
        if (f_x->guaranteed(Expression::Attribute::POSITIVE) && g_x->guaranteed(Expression::Attribute::NONNEGATIVE)) return true;
        if (f_x->guaranteed(Expression::Attribute::NEGATIVE) && g_x->guaranteed(Expression::Attribute::NONPOSITIVE)) return true;
        if (f_x->guaranteed(Expression::Attribute::NONPOSITIVE) && g_x->guaranteed(Expression::Attribute::NEGATIVE)) return true;
        if (f_x->guaranteed(Expression::Attribute::NONNEGATIVE) && g_x->guaranteed(Expression::Attribute::POSITIVE)) return true;
        return false;

    case Expression::Attribute::POSITIVE:
        if (f_x->guaranteed(Expression::Attribute::POSITIVE) && g_x->guaranteed(Expression::Attribute::NONNEGATIVE)) return true;
        if (f_x->guaranteed(Expression::Attribute::NONNEGATIVE) && g_x->guaranteed(Expression::Attribute::POSITIVE)) return true;
        return false;

    case Expression::Attribute::NEGATIVE:
        if (f_x->guaranteed(Expression::Attribute::NEGATIVE) && g_x->guaranteed(Expression::Attribute::NONPOSITIVE)) return true;
        if (f_x->guaranteed(Expression::Attribute::NONPOSITIVE) && g_x->guaranteed(Expression::Attribute::NEGATIVE)) return true;
        return false;

    case Expression::Attribute::NONPOSITIVE:
        return f_x->guaranteed(a) && g_x->guaranteed(a);

    case Expression::Attribute::NONNEGATIVE:
        return f_x->guaranteed(a) && g_x->guaranteed(a);

    case Expression::Attribute::UNITRANGE:
        break;

    case Expression::Attribute::ANTIUNITRANGE:
        break;

    case Expression::Attribute::OPENUNITRANGE:
        break;

    case Expression::Attribute::ANTIOPENUNITRANGE:
        break;

    case Expression::Attribute::CONTINUOUS:
        return f_x->guaranteed(a) && g_x->guaranteed(a);

    case Expression::Attribute::INCREASING:
        if (f_x->guaranteed(Expression::Attribute::INCREASING) && g_x->guaranteed(Expression::Attribute::NONDECREASING)) return true;
        if (f_x->guaranteed(Expression::Attribute::NONDECREASING) && g_x->guaranteed(Expression::Attribute::INCREASING)) return true;
        return false;

    case Expression::Attribute::DECREASING:
        if (f_x->guaranteed(Expression::Attribute::DECREASING) && g_x->guaranteed(Expression::Attribute::NONINCREASING)) return true;
        if (f_x->guaranteed(Expression::Attribute::NONINCREASING) && g_x->guaranteed(Expression::Attribute::DECREASING)) return true;
        return false;

    case Expression::Attribute::NONINCREASING:
        return f_x->guaranteed(a) && g_x->guaranteed(a);

    case Expression::Attribute::NONDECREASING:
        return f_x->guaranteed(a) && g_x->guaranteed(a);

    case Expression::Attribute::BOUNDEDABOVE:
        return f_x->guaranteed(a) && g_x->guaranteed(a);

    case Expression::Attribute::BOUNDEDBELOW:
        return f_x->guaranteed(a) && g_x->guaranteed(a);
    }

    return false;
}

bool Mul::guaranteed(Expression::Attribute a) const
{
    if (f_x->guaranteed(Expression::Attribute::DEFINED) && g_x->guaranteed(Expression::Attribute::DEFINED)) switch (a)
    {
    case Expression::Attribute::DEFINED:
        return true;

    case Expression::Attribute::NONZERO:
        return f_x->guaranteed(a) && g_x->guaranteed(a);

    case Expression::Attribute::POSITIVE:
        if (f_x->guaranteed(Expression::Attribute::POSITIVE) && g_x->guaranteed(Expression::Attribute::POSITIVE)) return true;
        if (f_x->guaranteed(Expression::Attribute::NEGATIVE) && g_x->guaranteed(Expression::Attribute::NEGATIVE)) return true;
        return false;

    case Expression::Attribute::NEGATIVE:
        if (f_x->guaranteed(Expression::Attribute::POSITIVE) && g_x->guaranteed(Expression::Attribute::NEGATIVE)) return true;
        if (f_x->guaranteed(Expression::Attribute::NEGATIVE) && g_x->guaranteed(Expression::Attribute::POSITIVE)) return true;
        return false;

    case Expression::Attribute::NONPOSITIVE:
        break;

    case Expression::Attribute::NONNEGATIVE:
        break;

    case Expression::Attribute::UNITRANGE:
        return f_x->guaranteed(a) && g_x->guaranteed(a);

    case Expression::Attribute::ANTIUNITRANGE:
        return f_x->guaranteed(a) && g_x->guaranteed(a);

    case Expression::Attribute::OPENUNITRANGE:
        return f_x->guaranteed(a) && g_x->guaranteed(a);

    case Expression::Attribute::ANTIOPENUNITRANGE:
        return f_x->guaranteed(a) && g_x->guaranteed(a);

    case Expression::Attribute::CONTINUOUS:
        return f_x->guaranteed(a) && g_x->guaranteed(a);

    case Expression::Attribute::INCREASING:
        break;

    case Expression::Attribute::DECREASING:
        break;

    case Expression::Attribute::NONINCREASING:
        break;

    case Expression::Attribute::NONDECREASING:
        break;

    case Expression::Attribute::BOUNDEDABOVE:
        break;

    case Expression::Attribute::BOUNDEDBELOW:
        break;
    }

    return false;
}

bool Pow::guaranteed(Expression::Attribute a) const
{
    if (f_x->guaranteed(Expression::Attribute::POSITIVE) && g_x->guaranteed(Expression::Attribute::DEFINED)) switch (a)
    {
    case Expression::Attribute::DEFINED:
    case Expression::Attribute::NONZERO:
    case Expression::Attribute::POSITIVE:
    case Expression::Attribute::NONNEGATIVE:
        return true;

    case Expression::Attribute::UNITRANGE:
        break;

    case Expression::Attribute::ANTIUNITRANGE:
        break;

    case Expression::Attribute::OPENUNITRANGE:
        break;

    case Expression::Attribute::ANTIOPENUNITRANGE:
        break;

    case Expression::Attribute::CONTINUOUS:
        return f_x->guaranteed(a) && g_x->guaranteed(a);

    case Expression::Attribute::INCREASING:
        break;

    case Expression::Attribute::DECREASING:
        break;

    case Expression::Attribute::NONINCREASING:
        break;

    case Expression::Attribute::NONDECREASING:
        break;

    case Expression::Attribute::BOUNDEDABOVE:
        break;

    case Expression::Attribute::BOUNDEDBELOW:
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

void Sign::print(std::ostream& out) const
{
    out << "sign(";
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

void Log::print(std::ostream& out) const
{
    out << "log(";
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

Expression::Expression() : pData(Shared::Clone(NullExpression::instance))
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

Expression sign(Expression const& r)
{
    return r.pData->sign();
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

Expression log(Expression const& r)
{
    return r.pData->log();
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

Expression pow(Expression const& r, Expression const& s)
{
    return r.pData->pow(s.pData);
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
    ++Expression::data::dirtyLevel;
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
    data(double d, std::string const& r) : value(d), name(r) { }

    mutable double value;
    mutable std::string name;
};

/***********************************************************************************************************************
*** Variable
***********************************************************************************************************************/

Variable::Variable(Variable const& r) noexcept : pData(Shared::Clone(r.pData))
{
}

Variable::Variable(char const* p, double d) : pData(p ? new data(d, p) : new data(d))
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

std::string Variable::Name() const
{
    return pData->name;
}

void Variable::Name(std::string const& s)
{
    pData->name = s;
}

Variable::data const* Variable::id() const
{
    return pData;
}

//**********************************************************************************************************************

