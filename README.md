# Laskenta Library

Laskenta is a stand-alone C++ library for multivariable differential calculus.  It constructs 'Expression' objects by combining references to 'Variable' objects with other Expressions and numeric literals, using elementary functions and arithmetic operators.  A numeric value can be assigned to each Variable.  An Expressions can be evaluated and the result depends on the current values of its containing Variables.  When the value of a Variable changes, the change is reflected by a subsequent evaluation of an Expression that contains references to it.  Any Expression can be derived using any of the Variables it references, and the derivative is - again - an Expression object that indicates the ratio of how much the value of the expression changes when the corresponding variable is changed by an infinitesimal amount.  And because the derivative is just another Expression, there is no restriction to deriving it again if needed.

The 'derivative' is often also called 'differential'.  The Finnish word 'laskenta' means 'calculus' in English, hence the name of the library.

