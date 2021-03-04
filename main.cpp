
#include "Laskenta.h"
#include "Tools.h"

#include <assert.h>
#include <iomanip>
#include <iostream>
#include <vector>

using std::cout;
using std::endl;

//**********************************************************************************************************************

static size_t const N = 80;  // Size of the Universal Function Approximator (i.e. neural network having topology 1:N:1 and range-unrestricted input and output neurons).

Expression activate(Expression const& r)
{
    return tanh(r);
}

std::vector<std::pair<double, double>> CreateTrainingSet(int const numSamples = 1000)
{
    std::vector<std::pair<double, double>> result;
    double const PI = acos(-1);

    for (int sample = 0; sample <= numSamples; ++sample)  // deliberately off-by-one
    {
        double angle = sample * (PI / numSamples);
        result.push_back({ cos(angle), sin(angle) });
    }

    return result;
}

//**********************************************************************************************************************

int main() try
{
    Variable x;     // Input value

    Variable a[N];  // Middle layer weights
    Variable b[N];  // Middle layer biases
    Variable c[N];  // Output layer weights
    Variable d;     // Output layer bias

    // 1. Construct Universal Function Approximator (UFA, i.e. a neural network to learn the approximation of a function)

    Expression function = d * x;

    for (size_t i = 0; i < N; ++i)
    {
        function = function + activate(x * a[i] + b[i]) * c[i];
    }

    // 2. Derive the UFA so that you can train its differential instead of the function itself

    Expression differential = function.Derive(x);

    // 3. Define the cost function:  Given 'x' compute the distance from value of 'differential' to 'y' >squared<

    Variable y;
    Expression cost = (differential - y) * (differential - y);

    // 4. Create the training material batch

    auto trainingset = CreateTrainingSet();
    Expression batch = 0;
    
    for (auto const& item : trainingset)
    {
        std::vector<std::pair<Variable, Expression>> sample;
        sample.push_back({ x, item.first });
        sample.push_back({ y, item.second });
        batch = batch + cost.Bind(sample);
    }

    // 5. 
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

//**********************************************************************************************************************

template<int N> struct UniversalFunctionApproximator
{
    Variable x;     // Input value

    Variable a[N];  // Middle layer weights
    Variable b[N];  // Middle layer biases
    Variable c[N];  // Output layer weights
    Variable d;     // Output layer bias
    Variable rate;  // Descent rate

    Expression A[N];  // Instrumented middle layer weights
    Expression B[N];  // Instrumented middle layer biases
    Expression C[N];  // Instrumented output layer weights
    Expression D;     // Instrumented output layer bias
    Expression X;     // Instrumented input value

    Expression integral;  // Output evaluator
    Expression derivant;  // The network

#if 0
    double snapshot_a[N];  // Snapshot of middle layer weights
    double snapshot_b[N];  // Snapshot of middle layer biases
    double snapshot_c[N];  // Snapshot of output layer weights
    double snapshot_d;     // Snapshot of output layer bias

    Expression cost;		// Raw cost function
    Expression minimize;	// Instrumented cost funxction
    Expression findzero;	// Find zero to minimize cost function
    Expression converge;	// Find optimal rate for convergence
#endif

    UniversalFunctionApproximator()
    {
        for (int i = 0; i < N; ++i)
        {
            A[i] = a[i];
            B[i] = b[i];
            C[i] = c[i];
        }

        D = d;
        X = x;

        derivant = BuildDerivant();
        integral = BuildIntegral();
    }

    void Initialize()
    {
        for (int i = 0; i < N; ++i)
        {
            a[i] = cos(i + 1);
            b[i] = 0;
            c[i] = sin(2 * i);
        }

        d = 0;
    }

    Expression BuildDerivant()
    {
        Expression result = D;

        for (int i = 0; i < N; ++i)
        {
            result = result + tanh(A[i] * X + B[i]) * C[i];  // <---- activation function
        }

        return result;
    }

    Expression BuildIntegral()
    {
        Expression result = D * X;

        for (int i = 0; i < N; ++i)
        {
            result = result + log(cosh(A[i] * X + B[i])) * C[i] / A[i];  // <---- integral of activation function
        }

        return result;
    }

    double SetBatch(std::vector<std::pair<double, double>> const& batch)
    {
        Expression cost = 0;

        for (auto& item : batch)
        {
            TODO;
        }

        TODO;
    }

    double operator()(double arg) const
    {
        x = arg;

        return integral();
    }
};

//**********************************************************************************************************************

double function(double x)
{
    return sqrt(1 - x * x);
}

double scale(int i, int n, double lo, double hi)
{
    assert(n > 0);
    assert(i >= 0 && i <= n);
    assert(lo < hi);

    return lo + i * (hi - lo) / n;
}

//**********************************************************************************************************************

static double const PI = acos(-1);

//**********************************************************************************************************************

/*
int main() try
{
    UniversalFunctionApproximator<2> ufa;

    //

    std::vector<std::pair<double, double>> batch;

    for (int i = 0; i <= 1000; ++i)
    {
        double angle = scale(i, 1000, 0, PI);

        batch.push_back({ cos(angle), sin(angle) });
    }

    throw 0;

    ufa.train(batch);

    //

    for (int i = 0; i < 1000; ++i)
    {
        double x = scale(i,1000,-1,1);
        cout << function(x) << "\t" << ufa(x) << "\t" << abs(function(x) - ufa(x)) << endl;
    }

    throw nullptr;

    Variable x;
    Expression e = exp(+x);
    Expression f = e.Derive(x);
    Expression g = f.Derive(x);

    for (int i = -999; i <= 999; ++i)
    {
        x = i / 1000.0;
        cout << e() << "\t" << f() << "\t" << g() << endl;
    }

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
*/
