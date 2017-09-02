#ifndef RANDOM_H
#define RANDOM_H

#include <chrono>
#include <random>

unsigned seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
thread_local std::ranlux48_base rl48(seed);

double drand48() {
    return rl48() / (rl48.max() + 1.0);
}

#endif // RANDOM_H
