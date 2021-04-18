
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

Expression activation_0(Expression const& x)
{
    return sinh(x);
    // return tanh(x);  // <---- pretty good!
}

Expression activation_1(Expression const& x)
{
    return x;
}

std::vector<std::pair<double, double>> CreateTrainingSet(int const numSamples = 100)
{
    std::vector<std::pair<double, double>> result;
    double const PI = acos(-1);

    for (int sample = 0; sample <= numSamples; ++sample)  // intentionally off-by-one (for 'n' intervals 'n+1' samples are needed)
    {
        double angle = sample * (PI / numSamples);
        result.emplace_back(cos(angle), sin(angle));
    }

    return result;
}

Variable x;          // Source value

Variable gain_0[N];  // Middle layer weights
Variable bias_0[N];  // Middle layer biases
Variable gain_1[N];  // Output layer weights
Variable bias_1;     // Output layer bias

Expression func;     // The resultant integral after training
Expression computation;     // Derivative to be trained
Expression expectation = (x * x - 1) * (x * x - 1) / (x * x + 1);
Expression loss;     // Squared difference of the approximation and the expectation (used in training)

Variable rate;       // Descent rate

std::vector<std::pair<Variable, Expression>> gradients;  // Variables & their augmentation by gradient times 'rate'

//**********************************************************************************************************************

int main() try
{
    cout << std::setprecision(14);

    for (size_t i = 0; i < N; ++i)
    {
        gain_0[i] = sin(i);
        gain_1[i] = cos(i);
    }

    // 1. Construct Universal Function Approximator (i.e. a neural network to learn the approximation of a function)

    Expression neuron[N];

    for (size_t i = 0; i < N; ++i) neuron[i] = activation_0(bias_0[i] + gain_0[i] * x);

    Expression output = x * bias_1;  // <---- Note that 'x*bias_1' degenerates to plain 'bias_1' when derived

    for (size_t i = 0; i < N; ++i) output = output + gain_1[i] * neuron[i];

    func = activation_1(output);

    // 2. Derive the UFA so that you can train its differential instead of the function itself

    computation = func.Derive(x);

    // 3. Construct the cost function:  Given 'x' compute the distance from value of 'differential' to 'y' >squared<

    loss = (computation - expectation) * (computation - expectation);

    // 4. Create the training batch

    Expression batch = 0;

    for (int i = 0; i <= 180; ++i)
    {
        double d = (i - 90.0) / 90.0;
        batch = batch + loss.Bind(x, d);
    }

    batch = batch / 181;

    // 5. Instrument the training set for gradient descent

    for (size_t i = 0; i < N; ++i)
    {
        cout << ".";
        gradients.emplace_back(gain_0[i], gain_0[i] - rate * batch.Derive(gain_0[i]));
        gradients.emplace_back(bias_0[i], bias_0[i] - rate * batch.Derive(bias_0[i]));
        gradients.emplace_back(gain_1[i], gain_1[i] - rate * batch.Derive(gain_1[i]));
    }
    cout << endl;
    gradients.emplace_back(bias_1, bias_1 - rate * batch.Derive(bias_1));

    batch = batch.AtomicBind(gradients);

    //

    auto slope = batch.Derive(rate);
    auto converge = rate - slope / slope.Derive(rate);

    for (int i = 0; i < 850; ++i)
    {
        if (i % 10 == 0) cout << ".";
        // cout << batch() << ", " << rate() << endl;

        rate = 0; rate = converge();
        // rate = converge();

        AtomicAssign(gradients);
    }
    cout << endl;

    for (int i = 0; i <= 180; ++i)
    {
        double d = (i - 90.0) / 90.0;
        x = d;
        cout << x() << ", " << expectation() << ", " << computation() << endl;
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
