#include "Helper_Functions.h"
#include <cmath>

namespace Utils
{
	double Combination_count(double n, double k)
	{
		if (k > n) {
			return 0;
		}
		
		if (k * 2 > n) {
			k = n - k;
		}

		if (k == 0) {
			return 1;
		}

		double result = n;
		for (int i = 2; i <= k; ++i) {
			result *= (n - i + 1);
			result /= i;
		}

		return result;
	}

	double Combination_count(unsigned int n, unsigned int k)
	{
		return Combination_count(double(n), double(k));
	}


	void Euler_estimation(std::vector<double>& mu, unsigned int b, double rho, int d, double h, double max_diff, int itr_max)
	{
		std::vector<double> w_0, w;
		for (int i = 0; i <= mu.size(); i++) {
			if (i == 0) {
				w_0.push_back(1);
				w.push_back(1);
			} else {
				w_0.push_back(0);
				w.push_back(0);
				for (int j = i; j < mu.size(); j++)
					w_0[i] += mu[j];
			}
		}

		double t = h;
		int itr = 0;
		double diff = 100000000000000;
		while (itr < itr_max && diff > max_diff) {
			double sigma = 0;
			for (unsigned int j = 1; j <= b; j++) {
				sigma += std::pow(w_0[j], d);
			}

			for (unsigned int i = 1; i < b; i++) {
				w[i] = w_0[i] + h * (1 - std::pow(w_0[i], d) - (b - sigma) * ((i * (w_0[i] - w_0[i + 1])) / (b * rho)));
			}

			diff = std::abs(w[0] - w_0[0]);
			for (unsigned int i = 1; i <= b; i++) {
				if (std::abs(w[i] - w_0[i]) > diff) {
					diff = std::abs(w[i] - w_0[i]);
				}
			}

			for (int i = 1; i < w_0.size(); i++) {
				w_0[i] = w[i];
			}

			t += h;
			itr++;
		}

		for (unsigned int i = 0; i <= b; i++) {
			mu[i] = w_0[i] - w_0[i + 1];
		}
	}
}
