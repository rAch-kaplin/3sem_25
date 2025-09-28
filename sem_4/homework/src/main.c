#include "monte_carlo.h"

int main() {
    int num_threads_sqrt = 2;

    double result = CalculateIntegral(ExponentialFunc, num_threads_sqrt, 0.0, 1.0, 0.0, ExponentialFunc(1.0));

    printf("Result: %lg\n", result);

    return 0;
}
