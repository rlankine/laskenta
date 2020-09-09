# Laskenta Library

Laskenta is a C++ library for multivariable differential calculus.  It is used to construct 'Expression' objects by combining references to 'Variable' objects with other Expressions and numeric literals, using elementary functions and arithmetic operators.  A numeric value can be assigned to each Variable.  An Expression can be evaluated and the result depends on the momentary values of its contained Variables.  When the value of a Variable changes, the change is reflected by subsequent evaluations of any Expression that contains references to it.  An Expression can be derived using any of the Variables it references, where the derivative indicates the ratio of how much the value of the Expression would change when the corresponding variable changes by an infinitesimal amount.  And because the derivative is just another Expression, there is no restriction against deriving it again if needed or further combining it with other Expressions.

The Finnish word for 'calculus' is 'laskenta', hence the name of the library.

## Installing
Currently there is no automatic installer, but putting the files in their places is very straighforward:

- `laskenta.cpp` -- Laskenta Library implementation
- `laskenta.h` -- Laskenta API declarations used by `laskenta.cpp` and your application source file(s)
- `tools.h` -- General purpose development helpers used by `laskenta.cpp`
- `demo.cpp` -- Small demo application about Laskenta Library capabilities

I recommend copying the header files (`laskenta.h` and `tools.h`) either into the directory of your common shared global header files, or into the same directory in which your project resides.  The implementation file (`laskenta.cpp`) should be copied into the same directory with your project source files.  The demo application can serve as a starting point for learning and exploration in which case it should be the main source file in its own project directory.

#### Getting started
After installing you may want to take a look at the `demo.cpp` application.  It demonstrates typical uses of the Laskenta Library in C++ source code.  A more detailed manual is planned to be written.

## Applicability
Calculating differentials of even just moderately complex mathematical expressions by hand is very tedious, time consuming, and error prone.  Using a tool such as MATLAB or Mathematica for the tedious part still costs time and leaves room for errors when incorporating the resultant formulae into an application program.  One other method known as Automatic Differentiation (a.k.a *autodiff*) is perhaps the most widely used solution because it can compute derivatives automatically alongside the formula being computed and hence requires very little effort from the application programmer.  What it still lacks is versatility when it comes to dealing with multiple variables or higher order derivatives.

Laskenta Library can manipulate highly complex mathematical expressions and it does this very efficiently.  It suits particularly well for rapid prototyping because it is many times quicker than traditional methods in generating correct expressions, but it certainly is not lacking power for actual applications as well.  It performs computations very efficiently because of its extensive common subexpression detection and sharing.  Sharing common subexpression not only saves memory, but more importantly it facilitates intermediate value caching during expression evaluation.  Even multiple separate expressions operating on the same set of variables are interlaced so that a side effect of evaluating one expression is the reduction of time needed to evaluate other expressions because they can now utilize the precomputed intermediate values from cache.

Because of its versatility the Laskenta Library is an enabler to many algorithms that currently appear only on paper or in a researcher's mind.

#### Some application areas:
* Mathematics
  * numerical solving and optimization
  * dynamic system modeling and analysis
  * efficiently computing Jacobians and Hessians
  * approximating difficult inverse functions
  * approximating difficult integrals as Expression objects where the range can be freely varied after construction (not as plain numerical value of a fixed range the way trapezoid or Monte Carlo methods would do)
* Navigation
  * sensor fusion and kalman filter
  * simultaneous locating and mapping by tracking scenery artifacts
* Neural networks
  * easily experiment with arbitrary activation functions and objective functions
  * easily implement irregular network topologies
  * eliminates the need to develop explicit back-propagation algorithms
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

If you wish to contact the author you may send email to: laskenta.library@gmail.com
