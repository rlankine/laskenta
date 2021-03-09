
#include "Laskenta.h"
#include "Tools.h"

#include <assert.h>
#include <iomanip>
#include <iostream>
#include <vector>

using std::cout;
using std::endl;

//**********************************************************************************************************************

static size_t const N = 85;  // Size of the Universal Function Approximator (i.e. neural network having topology 1:N:1 and range-unrestricted input and output neurons).

Expression activation_0(Expression const& r)
{
    return log(1 + exp(r));
}

Expression activation_1(Expression const& r)
{
    return r;
}

std::vector<std::pair<double, double>> CreateTrainingSet(int const numSamples = 100)
{
    std::vector<std::pair<double, double>> result;
    double const PI = acos(-1);

    for (int sample = 0; sample <= numSamples; ++sample)  // deliberately off-by-one (for 'n' intervals 'n+1' samples are needed)
    {
        double angle = sample * (PI / numSamples);
        result.emplace_back(cos(angle), sin(angle));
    }

    return result;
}

//**********************************************************************************************************************

int main() try
{
    Variable x;            // Source value
    Variable y;            // Target value

    Variable weight_0[N];  // Middle layer weights
    Variable bias_0[N];    // Middle layer biases
    Variable weight_1[N];  // Output layer weights
    Variable bias_1;       // Output layer bias

    Expression func;       // The resultant integral after training
    Expression diff;       // Derivative to be trained

    Expression cost;       // Squared difference of the approximation and the expectation (used in training)

    Variable rate;

    Expression gradient_w0[N];
    Expression gradient_b0[N];
    Expression gradient_w1[N];
    Expression gradient_b1;

    for (size_t i = 0; i < N; ++i)
    {
        weight_0[i] = sin(i);
        weight_1[i] = cos(i);
    }

    // 1. Construct Universal Function Approximator (UFA, i.e. a neural network to learn the approximation of a function)

    Expression neuron[N];

    for (size_t i = 0; i < N; ++i)
    {
        neuron[i] = activation_0(bias_0[i] + weight_0[i] * x);
    }

    Expression output = x * bias_1;  // <---- Note that 'x*bias_1' degenerates to plain 'bias_1' in the trained expression

    for (size_t i = 0; i < N; ++i)
    {
        output = output + weight_1[i] * neuron[i];
    }

    func = activation_1(output);

    // 2. Derive the UFA so that you can train its differential instead of the function itself

    diff = func.Derive(x);

    // 3. Construct the cost function:  Given 'x' compute the distance from value of 'differential' to 'y' >squared<

    cost = (diff - y) * (diff - y);

    // 4. Create the training batch

    auto trainingset = CreateTrainingSet();
    Expression batch = 0;

    for (auto const& item : trainingset)
    {
        std::vector<std::pair<Variable, Expression>> sample;
        sample.emplace_back(x, item.first);
        sample.emplace_back(y, item.second);
        batch = batch + cost.Bind(sample);
    }

    batch = batch / double(trainingset.size());  // To average the cost of all samples

    // 5. Instrument the training set for gradient descent

    std::vector<std::pair<Variable, Expression>> gradients;

    for (size_t i = 0; i < N; ++i)
    {
        gradient_w0[i] = weight_0[i] - rate * batch.Derive(weight_0[i]);
        gradients.emplace_back(weight_0[i], gradient_w0[i]);

        gradient_b0[i] = bias_0[i] - rate * batch.Derive(bias_0[i]);
        gradients.emplace_back(bias_0[i], gradient_b0[i]);

        gradient_w1[i] = weight_1[i] - rate * batch.Derive(weight_1[i]);
        gradients.emplace_back(weight_1[i], gradient_w1[i]);

        cout << ".";
    }
    cout << endl;

    gradient_b1 = bias_1 - rate * batch.Derive(bias_1);
    gradients.emplace_back(bias_1, gradient_b1);

    batch = batch.Bind(gradients);


    for (int i = 0; i < 40; ++i)
    {
        rate = i / 1000.0;
        cout << rate() << ", " << batch() << endl;
    }
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
