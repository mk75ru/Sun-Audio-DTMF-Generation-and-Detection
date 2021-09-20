#include <stdint.h>
#include <math.h>

#include "debug.h"
#include "goertzel.h"

void goertzel_init(GOERTZEL_STATE *gp, uint32_t N, double k) {
	// TO BE IMPLEMENTED
	if (gp == NULL) return;
	gp->N = N;
	gp->k = k;
	double A = 2 * M_PI * k / N;
	gp->A = A;
	gp->B = 2 * cos(A);
	gp->s0 = 0;
	gp->s1 = 0;
	gp->s2 = 0;
}

void goertzel_step(GOERTZEL_STATE *gp, double x) {
	// TO BE IMPLEMENTED
	if (gp == NULL) return;
	/*debug("%u\n", gp->N);
	debug("%f\n", gp->k);
	debug("%f\n", gp->A);
	debug("%f\n", gp->B);
	debug("%f\n", gp->s0);
	debug("%f\n", gp->s1);
	debug("%f\n", gp->s2);
	*/
	double s1 = gp->s1;
	double s0 = x + (gp->B) * s1 - (gp->s2);
	gp->s0 = s0;
	gp->s2 = s1;
	gp->s1 = s0;
}

double goertzel_strength(GOERTZEL_STATE *gp, double x) {
	// TO BE IMPLEMENTED
	if (gp == NULL) return -1;
	double A = gp->A;
	double s1 = gp->s1;
	double s0 = x + (gp->B) * s1 - (gp->s2);
	uint32_t N = gp->N;
	double DA = 2 * M_PI * (gp->k) / N * (N - 1);
	//double C = cos(A) - I * sin(A);
	//double y = s0 - s1 * C;
	//double D = cos(DA) - I * sin(DA);
	//y = y * D = (s0 - s1 * (cos(A) - I * sin(A))) * (cos(DA) - I * sin(DA))
	/*==========================================
	 The formula below is calculated by combining y calculation procedures shown above,
	 	and simplified using WolframAlpha calculator to get complex results for the
	 	calculation of its squared magnitude.
	*/
	//double y = pow(s0 * cos(DA) - s1 * cos(A + DA), 2) + pow(-s0 * sin(DA) + s1 * sin(A + DA), 2);
	double y1 = s0 * cos(DA) - s1 * cos(A + DA);
	double y2 = -s0 * sin(DA) + s1 * sin(A + DA);
	double y = y1 * y1 + y2 * y2;
	return (2 * y / (N * N));
}
