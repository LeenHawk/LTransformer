//#include "convolution2d.h"#define
//typedef int DATA_TYPE;

/* Problem size */
#define NI 32
#define NJ 32
#define SIZE NI*NJ
#define c11 2
#define c12 -3
#define c21 5
#define c22 6

/*For 3x3 filter*/
#define c13 4
#define c23 7
#define c31 -8
#define c32 -9
#define c33 10

int convolution2d_compute(int a1,int a2,int a3,int a4,int a5,int a6,int a7,int a8,int a9){
	#pragma HLS INLINE OFF
	return c11 * a1 + c12 * a2 + c13 * a3 + c21 * a4 + c22 * a5 + c23 * a6 + c31 * a7 + c32 * a8 + c33 * a9;
}

void convolution2d(int A[SIZE],int B[SIZE]) {
	for (int i = 1; i < NI -1; i++) {
		for (int j = 1; j < NJ-1; j++) {
	/*For 3x3 filter*/
		B[i*NJ+j]=convolution2d_compute(A[(i - 1)*NJ + (j - 1)],A[(i + 0)*NJ + (j - 1)],A[(i + 1)*NJ + (j - 1)],
		A[(i - 1)*NJ + (j + 0)],A[(i + 0)*NJ + (j + 0)],A[(i + 1)*NJ + (j + 0)],
		A[(i - 1)*NJ + (j + 1)],A[(i + 0)*NJ + (j + 1)],A[(i + 1)*NJ + (j + 1)]);
		}
	}
}