#include "CMRRandomGenerator.h"

namespace Utils
{
	double CMRRandomGenerator::norm = 2.328306549295728e-10;
	double CMRRandomGenerator::m1 = 4294967087.0;
	double CMRRandomGenerator::m2 = 4294944443.0;
	double CMRRandomGenerator::a12 = 1403580.0;
	double CMRRandomGenerator::a13 = -810728.0;
	double CMRRandomGenerator::a21 = 527612.0;
	double CMRRandomGenerator::a23 = -1370589.0;

	double CMRRandomGenerator::a[2][3][3] = { {{0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, {a13, a12, 0.0}},
	{{0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, {a23, 0.0, a21}} };

	double CMRRandomGenerator::m[2] = { m1, m2 };
	double CMRRandomGenerator::init_s[2][3] = { {1.0, 1.0, 1.0}, {1.0, 1.0, 1.0} };

	CMRRandomGenerator::CMRRandomGenerator(int64_t n, int32_t e)
	{
		for (int i = 0; i <= 1; i++) {
			for (int j = 0; j <= 2; j++) {
				s[i][j] = init_s[i][j];
			}
		}
		Advance(n, e);
	}

	void CMRRandomGenerator::Advance(int64_t n, int32_t e)
	{
		int64_t B[2][3][3];

		int64_t S[2][3];
		int64_t M[2];

		for (int i = 0; i <= 1; i++) {
			m_ftoi(a[i], B[i], m[i]);
			v_ftoi(s[i], S[i], m[i]);
			M[i] = (int64_t)(m[i]);
		}

		while (e-- != 0) {
			for (int i = 0; i <= 1; i++) {
				mm_mul(B[i], B[i], B[i], M[i]);
			}
		}

		while (n != 0) {
			if ((n & 1) != 0) {
				for (int i = 0; i <= 1; i++) {
					mv_mul(B[i], S[i], S[i], M[i]);
				}
			}
			n >>= 1;
			if (n != 0) {
				for (int i = 0; i <= 1; i++) {
					mm_mul(B[i], B[i], B[i], M[i]);
				}
			}
		}

		for (int i = 0; i <= 1; i++) {
			v_itof(S[i], s[i], M[i]);
		}
	}

	double CMRRandomGenerator::NextDouble()
	{
		double p1 = mod(a12 * s[0][1] + a13 * s[0][0], m1);
		s[0][0] = s[0][1];
		s[0][1] = s[0][2];
		s[0][2] = p1;
		double p2 = mod(a21 * s[1][2] + a23 * s[1][0], m2);
		s[1][0] = s[1][1];
		s[1][1] = s[1][2];
		s[1][2] = p2;
		double p = p1 - p2;
		if (p < 0.0) {
			p += m1;
		}

		return (p + 1) * norm;
	}
}
