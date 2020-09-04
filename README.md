# Laskenta Library

Laskenta is a C++ library for multivariable differential calculus.  It is used to construct 'Expression' objects by combining references to 'Variable' objects with other Expressions and numeric literals, using elementary functions and arithmetic operators.  A numeric value can be assigned to every Variable.  An Expression can be evaluated and the result depends on the current values of its containing Variables.  When the value of a Variable changes, the change is reflected by a subsequent evaluation of all Expressions that contain references to it.  Any Expression can be derived using any of the Variables it references, where the derivative indicates the ratio of how much the value of the Expression would change when the corresponding variable changes by an infinitesimal amount.  And because the derivative is just another Expression, there is no restriction to deriving it again when needed.

The Finnish word for 'calculus' is 'laskenta', hence the name of the library.

## Installing
TODO

## Getting started
TODO

## Applicability
TODO
* Rapid prototyping
* Optimization
* Numerical solutions to math problems that are otherwise hard to crack
  * Inverse functions, e.g. using Newton-Raphson for problems such as: given 'y' solve 'x' in 'y = x * log(x)'
* Photogrammetry
* Visual SLAM
* Sensor Fusion
* Robotics, inverse kinematics
* Modelling the real world
* Curve fitting
  * Test
- Test
  - Test

## Under the hood
TODO

#### Caveats
TODO

## Future plans
TODO

## Links & footnotes
TODO
