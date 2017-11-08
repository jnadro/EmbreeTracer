#pragma once
#include <random>

class RandomSample
{
	std::default_random_engine generator;
	std::uniform_real_distribution<float> distribution;

public:
	RandomSample(uint32_t seed) : generator(seed), distribution(0.0f, 1.0f) {}
	float next() { return distribution(generator); }
};
