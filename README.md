# Laskenta Library

Laskenta is a C++ library for multivariable differential calculus.  It is used to construct 'Expression' objects by combining references to 'Variable' objects with other Expressions and numeric literals, using elementary functions and arithmetic operators.  A numeric value can be assigned to each Variable.  An Expression can be evaluated and the result depends on the momentary values of its containing Variables.  When the value of a Variable changes, the change is reflected by subsequent evaluations of any Expression that contains references to it.  An Expression can be derived using any of the Variables it references, where the derivative indicates the ratio of how much the value of the Expression would change when the corresponding variable changes by an infinitesimal amount.  And because the derivative is just another Expression, there is no restriction against deriving it again if needed or further combining it with other Expressions.

The Finnish word for 'calculus' is 'laskenta', hence the name of the library.

## Installing
Currently there is no automatic installer, but putting the files in their right places is very straighforward:

- `laskenta.cpp` -- Laskenta Library implementation
- `laskenta.h` -- Laskenta API declarations used by `laskenta.cpp` and your application source file(s)
- `tools.h` -- General purpose development helpers used by `laskenta.cpp`
- `demo.cpp` -- Small demo application about Laskenta Library capabilities

I recommend copying the header files (`laskenta.h` and `tools.h`) either into the directory of your common shared global header files, or into the same directory in which your project resides.  The implementation file (`laskenta.cpp`) should be copied into the same directory with your project source files.  The demo application can serve as a starting point for learning and exploration in which case it should be the main source file in its own project directory.

## Getting started
TODO

## Applicability
Calculating differentials of even just moderately complex mathematical expressions by hand is very tedious, time consuming, and error prone.  Using a tool such as MATLAB or Mathematica for the tedious part still costs time and leaves room for errors when incorporating the resultant formulae into an application program.  One other method known as Automatic Differentiation (a.k.a *autodiff*) is currently perhaps the most common solution because it can compute derivatives automatically alongside the formula being computed and hence requires very little effort from the application programmer.  What it lacks is versatility when it comes to dealing with multiple variables or higher order derivatives.

Laskenta Library can manipulate highly complex mathematical expressions and it does this very efficiently.  While it suits particularly well for rapid prototyping because it can operate regardless of the complexity of the underlying math, it is also more than sufficiently efficient for real time applications.  It performs computations very efficiently because it has a separate optimizing precompilation phase with common subexpression detection and sharing.  Common subexpression sharing not only saves memory, but it facilitates extensive intermediate value caching during expression evaluation.  Even multiple separate expressions operating on the same set of variables are interlaced so that evaluating one expression will often partially precompute intermediate values for the other expressions and communicate them thru value caching.

Finally, because of its versatility the Laskenta Library is an enabler to many algorithms that currently appear only on paper.  It can easily and efficiently compute Jacobian vectors and Hessian matrices of multivariable expressions.  It can easily compute second or third or any higher derivative of an expression making it possible to chain various mathematical methods even if they all need derivatives.  For example, consider solving a *differential* equation (=1st order derivative) using *gradient descent* algorithm with a variable descend rate (=2nd order derivatives) finding the descend rate that *minimizes* the value along the current gradient (=3rd order derivative) using *Newton-Raphson* iteration (=4th order derivative).

#### Some application areas:
* Mathematics
  * dynamic system modeling and analysis
  * numerical solving and optimization
  * approximating difficult inverse functions
  * approximating difficult integrals
* Navigation
  * sensor fusion and kalman filter
  * simultaneous locating and mapping by tracking scenery artifacts
* Neural networks
  * easily implement unconventional activation or objective functions
  * easily implement irregular network topologies
  * removes the need to develop explicit back-propagation algorithms
* Photogrammetry
  * bundle adjustment
* Prediction
  * curve fitting and extrapolating
* Robotics
  * inverse kinematics
  * machine vision

## Under the hood
TODO

#### Caveats
TODO

## Future plans
TODO

## Links & footnotes
TODO
