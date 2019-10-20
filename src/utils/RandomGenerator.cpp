#include "RandomGenerator.h"
#include <math.h>
#include <stdexcept>

namespace Utils
{
	RandomGenerator::RandomGenerator(int seed)
	{
		rand = new CMRRandomGenerator(seed / 200 + 1, seed % 200);
	}

	uint32_t RandomGenerator::Get_uint(uint32_t maxValue)
	{
		uint32_t v = (uint32_t)(FloatRandom() * (maxValue + 1));
		return v;
	}

	int32_t RandomGenerator::Get_int(int32_t maxValue)
	{
		int32_t v = (int32_t)(FloatRandom() * (maxValue + 1));
		return v;
	}

	double RandomGenerator::FloatRandom()
	{
		return rand->NextDouble();
	}

	double RandomGenerator::Uniform(double a, double b)
	{
		return a + (b - a) * FloatRandom();
	}

	/*
	*   Return a random integer uniformly distributed
	*   in the range m to n, inclusive
	*/
	uint32_t RandomGenerator::Uniform_uint(uint32_t m, uint32_t n)
	{
		return m + (uint32_t)((n - m + 1.0) * FloatRandom());
	}

	int64_t RandomGenerator::Uniform_long(int64_t m, int64_t n)
	{
		return m + (int64_t)((double)(n - m + 1) * FloatRandom());
	}

	uint64_t RandomGenerator::Uniform_ulong(uint64_t m, uint64_t n)
	{
		return m + (uint64_t)((double)(n - m + 1) * FloatRandom());
	}

	/*
	*   Return a random double number from a negative exponential
	*   distribution with mean 'mean'
	*/
	double RandomGenerator::Exponential(double mean)
	{
		return 0-mean * log(FloatRandom());
	}

	/*
	*   Return a random double number from an erlang distribution
	*   with mean x and standard deviation s
	*/
	double RandomGenerator::Erlang(double x, double s)
	{
		int64_t i, k;
		double z;
		z = x / s;
		k = (uint64_t)(z * z);
		z = 1.0;
		for (i = 0; i < k; i++) {
			z *= FloatRandom();
		}
		return -(x / (double) k) * log(z);
	}

	/*
	*   Return a random double number from a hyperexponential
	*   distribution with mean x and standard deviation s > x
	*/
	double RandomGenerator::HyperExponential(double x, double s)
	{
		if (s < x) {
			throw std::logic_error("RandomGenerator::HyperExponential: Mean must be greater than standard deviation.");
		}

		double cv, z, p;
		cv = s / x;
		z = cv * cv;
		p = 0.5 * (1.0 - sqrt((z - 1.0) / (z + 1.0)));
		z = (FloatRandom() > p) ? (x / (1.0 - p)) : (x / p);
		return -0.5 * z * log(FloatRandom());
	}

	/*
	*    Return a random double number from a normal
	*    distribution with mean x and standard deviation s
	*/
	double RandomGenerator::Normal(double x, double s)
	{
		double v1, v2, w, z1;
		if (Normal_z2 != 0.0) {
			/* use value from previous call */
			z1 = Normal_z2;
			Normal_z2 = 0.0;
		} else {
			do {
				v1 = 2.0 * FloatRandom() - 1.0;
				v2 = 2.0 * FloatRandom() - 1.0;
				w = v1 * v1 + v2 * v2;
			} while (w >= 1.0);

			w = sqrt((-2.0*log(w)) / w);
			z1 = v1 * w;
			Normal_z2 = v2 * w;
		}

		return x + z1 * s;
	}

	/*
	*    Return a random double number from a log-normal
	*    distribution with mean x and standard deviation s.
	*/
	double RandomGenerator::LogNormal(double x, double s)
	{
		return exp(Normal(x, s));
	}


	/*
	*   Return a random number from a geometric distribution
	*   with mean x (x > 0).
	*/
	int64_t RandomGenerator::Geometric0(double x)
	{
		double p = 1 / x;
		int64_t i = 0;
		while (FloatRandom() < p) {
			++i;
		}

		return i;
	}

	int64_t RandomGenerator::Geometric1(double x)
	{
		return Geometric0(x) + 1;
	}

	/*
	*   Return a random number from a hypergeometric distribution
	*   with mean x and standard deviation s
	*/
	double RandomGenerator::Hyper_geometric(double x, double s)
	{
		double sqrtz = s / x;
		double z = sqrtz * sqrtz;
		double p = 0.5 * (1 - sqrt((z - 1) / (z + 1)));
		double d = (FloatRandom() > p) ? (1 - p) : p;

		return -x * log(FloatRandom()) / (2 * d);
	}

	/*
	*   Return a random number from a binomial distribution
	*   of n items where each item has a probability u of
	*   being drawn.
	*/
	int64_t RandomGenerator::Binomial(int64_t n, double u)
	{
		int64_t k = 0;
		for (long i = 0; i < n; i++) {
			if (FloatRandom() < u) {
				++k;
			}
		}

		return k;
	}

	/*
	*   Return a random number from a Poisson distribution
	*   with mean x.
	*/
	int64_t RandomGenerator::Poisson(double x)
	{
		double b = exp(-x);
		int64_t k = 0;
		double p = 1;
		while (p >= b) {
			++k;
			p *= FloatRandom();
		}

		return k - 1;
	}

	/*
	*   Return a random number from a Weibull distribution
	*   with parameters alpha and beta.
	*/
	double RandomGenerator::Weibull(double alpha, double beta)
	{
		return pow(-beta * log(1 - FloatRandom()), 1 / alpha);
	}

	/*
	*   Return a random number from a Pareto distribution
	*   with parameters alpha(shape) and beta(position).
	*/
	double RandomGenerator::Pareto(double alpha, double beta)
	{
		return beta * pow(FloatRandom(), -1.0 / alpha);
	}

	/*
	*   Return a random number from a 1/x * 1/ln(max/min) distribution
	*   with min and max parameters.
	* mean = (max-min)/(ln(max)-ln(min))
	*/
	double RandomGenerator::Inverse(double min, double max)
	{
		return min*exp(FloatRandom()*log(max / min));
	}

	/*
	*   Return a random number from a triangular distribution
	*   with min and max and middle parameters.
	*
	*/
	double RandomGenerator::Triangular(double min, double middle, double max)
	{
		double y = FloatRandom();
		if (y <= (middle - min) / (max - min)) {
			return min + sqrt(y*max*middle - y*max*min - y*min*middle + y*min*min);
		} else {
			return max - sqrt(max*max + y*max*middle - y*max*max - y*min*middle + y*max*min - max*min - max*middle + min*middle);
		}
	}
}