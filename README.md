# Laskenta Library

Laskenta is a stand-alone C++ library for multivariable differential calculus.  It constructs 'Expression' objects by combining references to 'Variable' objects with other Expressions and numeric literals, using elementary functions and arithmetic operators.  A numeric value can be assigned to a Variable and an Expressions can be evaluated based on the current values of its containing Variables.  When the value of a Variable changes, the change is reflected in the evaluation result of an Expression that contains references to the Variable.  Finally, an Expression can be derived using any of the Variables it references.  The derivative is - again - an Expression object which indicates the ratio of how much more (or less) the value of the expression changes, when the corresponding variable is changed by an infinitesimal amount.

The name of the library 'laskenta' is Finnish word for 'calculus'.
