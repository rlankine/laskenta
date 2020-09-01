
#include "Laskenta.h"

#include <iomanip>
#include <iostream>

using std::cout;
using std::endl;

//**********************************************************************************************************************

int main() try
{
    Variable a;
    Variable b;
    Variable c;
    Variable x;

    a.Name("a");
    b.Name("b");
    c.Name("c");
    x.Name("x");

    Expression quadratic = a * x * x + b * x + c;
    Expression discriminant = b * b - 4 * a * c;
    Expression root0 = (-b - sqrt(discriminant)) / (2 * a);
    Expression root1 = (-b + sqrt(discriminant)) / (2 * a);

    //**********************************************************************************************************************

    cout << endl << "-------------- Quadratic formula:" << endl << endl;

    cout << "F(x,a,b,c) = " << quadratic << endl;

    //**********************************************************************************************************************

    cout << endl << "-------------- Roots:" << endl << endl;

    a = 1;   ///////////////////////////////////////////
    b = -5;  // Experiment!!!! Change the coefficients!
    c = 4;   ///////////////////////////////////////////

    cout << "Let a = " << double(a) << ", b = " << double(b) << " and c = " << double(c) << endl;
    if (discriminant.Evaluate() < 0)
    {
        cout << "Complex root: " << (-b / (2 * a)).Evaluate() << " + " << (sqrt(-discriminant) / (2 * a)).Evaluate() << "i" << endl;
        cout << "Complex root: " << (-b / (2 * a)).Evaluate() << " - " << (sqrt(-discriminant) / (2 * a)).Evaluate() << "i" << endl;
    }
    else
    {
        cout << root0 << " = " << root0.Evaluate() << endl;
        cout << root1 << " = " << root1.Evaluate() << endl;
    }

    //**********************************************************************************************************************

    cout << endl << "-------------- Evaluation:" << endl << endl;

    cout << "Let a = " << double(a) << ", b = " << double(b) << " and c = " << double(c) << endl;
    for (int i = 0; i < 8; ++i)
    {
        x = i;
        cout << "x = " << i << " ---> " << quadratic << " = " << quadratic.Evaluate() << endl;
    }

    //**********************************************************************************************************************

    cout << endl << "-------------- Derivative:" << endl << endl;

    Expression derivative = quadratic.Derive(x);

    cout << "F(x) = " << quadratic << " ---> F'(x) = " << derivative << endl;

    //**********************************************************************************************************************

    cout << endl << "-------------- Derivative is an Expression object like any other:" << endl << endl;

    cout << "G(x) = x - F(x)/F'(x) = " << x - quadratic / derivative << endl;
    cout << "G'(x) = " << (x - quadratic / derivative).Derive(x) << endl;
    x = 5;
    cout << "G'(5) = " << (x - quadratic / derivative).Derive(x).Evaluate() << endl;

    //**********************************************************************************************************************

    cout << endl << "-------------- Expression objects can be derived again and again and again:" << endl << endl;

    cout << "F(x) = " << quadratic << endl;
    cout << "F'(x) = " << quadratic.Derive(x) << endl;
    cout << "F''(x) = " << quadratic.Derive(x).Derive(x) << endl;
    cout << "F'''(x) = " << quadratic.Derive(x).Derive(x).Derive(x) << endl;

    //**********************************************************************************************************************

    cout << endl << "-------------- Expression objects can be derived wrt/ any variable:" << endl << endl;

    cout << "F'(a) = " << quadratic.Derive(a) << endl;
    cout << "F'(b) = " << quadratic.Derive(b) << endl;
    cout << "F'(c) = " << quadratic.Derive(c) << endl;

    //**********************************************************************************************************************

    cout << endl << "-------------- All elementary functions are supported:" << endl << endl;

    Expression e = log(sin(exp(tanh(sqrt(quadratic.Derive(x))))));

    cout << e << " --- Derive(x) ---> " << e.Derive(x) << endl;

    //**********************************************************************************************************************

    return EXIT_SUCCESS;
}
catch (std::exception e)
{
    cout << endl << e.what() << endl << endl;
}
catch (char const* p)
{
    cout << endl << p << endl << endl;
}
catch (...)
{
    cout << endl << "Diva tantrum!!!!" << endl << endl;
}

//----------------------------------------------------------------------------------------------------------------------
