#include "monte_carlo.h"

int main() {
    int num_threads_sqrt = 2;

    printf("Enter the root of the number of threads on which the program will run: ");
    if (scanf("%d", &num_threads_sqrt) != 1) {
        fprintf(stderr, "failed to get num treads sqrt, pls enter number\n");
        return 1;
    }

    double result = CalculateIntegral(ExponentialFunc, num_threads_sqrt, 0.0, 1.0, 0.0, ExponentialFunc(1.0));

    printf("Result: %lg\n", result);

    return 0;
}
