
#include "Laskenta.h"
#include "Tools.h"

#include <assert.h>
#include <iomanip>
#include <iostream>
#include <vector>

using std::cout;
using std::endl;

//**********************************************************************************************************************

static size_t const N = 341;  // Size of the Universal Function Approximator (i.e. neural network having topology 1:N:1 and range-unrestricted input and output neurons).

Expression activation_0(Expression const& x)
{
    // return log(1 + exp(x));
    // return asinh(x);  // <---- pretty good!
    // return tanh(x);  // <---- pretty good!
    return sin(x);  // <---- very good!
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
Variable y;          // Target value

Variable gain_0[N];  // Middle layer weights
Variable bias_0[N];  // Middle layer biases
Variable gain_1[N];  // Output layer weights
Variable bias_1;     // Output layer bias

Expression func;     // The resultant integral after training
Expression diff;     // Derivative to be trained
Expression loss;     // Squared difference of the approximation and the expectation (used in training)

Variable rate;       // Descent rate

std::vector<std::pair<Variable, Expression>> gradients;  // Variables & their augmentation by gradient times 'rate'

//**********************************************************************************************************************

int main() try
{
    cout << std::setprecision(14);

    auto trainingset = CreateTrainingSet(180);  // Vector of {x,y} pairs, where 'y = F(x)', where 'F()' is the function to approximate

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

    diff = func.Derive(x);

    // 3. Construct the cost function:  Given 'x' compute the distance from value of 'differential' to 'y' >squared<

    loss = (diff - y) * (diff - y);

    // 4. Create the training batch

    Expression batch = 0;

    for (auto const& item : trainingset) batch = batch + loss.Bind(x, item.first).Bind(y, item.second);
    batch = batch / double(trainingset.size());  // To average the cost of all samples

    // 5. Instrument the training set for gradient descent

    for (size_t i = 0; i < N; ++i)
    {
        gradients.emplace_back(gain_0[i], gain_0[i] - rate * batch.Derive(gain_0[i]));
        gradients.emplace_back(bias_0[i], bias_0[i] - rate * batch.Derive(bias_0[i]));
        gradients.emplace_back(gain_1[i], gain_1[i] - rate * batch.Derive(gain_1[i]));
        cout << ".";
    }

    gradients.emplace_back(bias_1, bias_1 - rate * batch.Derive(bias_1));
    cout << endl;

    batch = batch.AtomicBind(gradients);

    // 'batch' needs to be minimized wrt/'rate', then 

    auto slope = batch.Derive(rate);
    auto converge = rate + slope / -slope.Derive(rate);

    for (int i = 0; i < 256; ++i)
    {
        rate = converge();
        AtomicAssign(gradients);
    }

    for (int i = 0; i < 1000; ++i)
    {
        rate = i / 1000.0;
        cout << batch() << ", " << slope() << ", " << converge() << endl;
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
