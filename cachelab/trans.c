/*
 * Name: Zechen Zhang
 * AndrewID: zechenz
 *
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"
#include "contracts.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. The REQUIRES and ENSURES from 15-122 are included
 *     for your convenience. They can be removed if you like.
 */

char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
	int i, j, k, l, temp, subMatrixSize, tempRow, tempColumn;
// i,j control the loops for submatrix (8*8 in first two part, 18*18 in third part)
// k,l control the loops for lines in a submatrix
    REQUIRES(M > 0);
    REQUIRES(N > 0);
//-----------------------------------------------------------------
// 32 * 32 matrix
//-----------------------------------------------------------------
	if ((M == 32) && (N == 32)) {
		subMatrixSize = 8;
		for (i = subMatrixSize; i < 32; i += subMatrixSize)
			for (j = 0; j < i; j += subMatrixSize) {
				for (k = i; k < i + subMatrixSize; k++)
					for (l = j; l < j + subMatrixSize; l++)
						B[l][k] = A[k][l];
				for (k = i; k < i + subMatrixSize; k++)
					for (l = j; l < j + subMatrixSize; l++)
						B[k][l] = A[l][k];
			}
		for (i = 0; i < 32; i += subMatrixSize) {
			j = (i + 16) % 32;
			for (k = 0; k < subMatrixSize; k++)
				for (l = 0; l < subMatrixSize; l++)
				B[j+k][j+l] = A[i+l][i+k];
		}
		for (i = 0; i < 16; i += subMatrixSize) {
			j = i + 16;
			for (k = 0; k < subMatrixSize; k++)
				for (l = 0; l < subMatrixSize; l++) {
						temp = B[i+k][i+l];
						B[i+k][i+l] = B[j+k][j+l];
						B[j+k][j+l] = temp;
				}
		}
	} // if statement end
//--------------------------------------------------------------------
// 64 * 64 matrix
// I wrote this part quite long because I'm not sure if I call a function, 
// if the parameters would be counted as variables here. There are about 
// 6 numbers that should be passed into the "function". So I just copy-paste
// these codes instead of using a function.
// I devided the matrix into 4 parts to fulfill the transpose.
//--------------------------------------------------------------------
	if ((M == 64) && (N == 64)) {

		tempRow = 56;
		tempColumn = 56;
		for (i = 0; i < 56; i += 8) // This loop is the first part.
			for (j = 0; j < 56; j += 8)
				if (j != i)
					{
					for (k = 0; k < 4; k++) {
						for (l = 0; l < 4; l++)
							B[tempRow + l][tempColumn + k] = A[i + k][j + l];
						for (l = 4; l < 8; l++)
							B[tempRow + l - 4][tempColumn + k + 4] = A[i + k][j + l];
					}
					for (k = 4; k < 8; k++) {
						for (l = 4; l < 8; l++)
							B[j + l][i + k] = A[i + k][j + l];
						for (l = 0; l < 4; l++)
							B[j + k][i + l] = B[tempRow + k - 4][tempColumn + l + 4];
					}
					for (k = 4; k < 8; k++) 
						for (l = 0; l < 4; l++)
							B[tempRow + l][tempColumn + k] = A[i + k][j + l];
					
					for (k = 0; k < 4; k++)
						for (l = 0; l < 8; l++)
							B[j + k][i + l] = B[tempRow + k][tempColumn + l];
				}

		tempColumn = 0; tempRow = 0; // 'i = 56' and 'j = 56' consist the second part
			i = 56;
			for (j = 8; j < 56; j += 8) {
				for (k = 0; k < 4; k++) {
					for (l = 0; l < 4; l++)
						B[tempRow + l][tempColumn + k] = A[i + k][j + l];
					for (l = 4; l < 8; l++)
						B[tempRow + l - 4][tempColumn + k + 4] = A[i + k][j + l];
				}
				for (k = 4; k < 8; k++) {
					for (l = 4; l < 8; l++)
						B[j + l][i + k] = A[i + k][j + l];
					for (l = 0; l < 4; l++)
						B[j + k][i + l] = B[tempRow + k - 4][tempColumn + l + 4];
				}
				for (k = 4; k < 8; k++) 
					for (l = 0; l < 4; l++)
						B[tempRow + l][tempColumn + k] = A[i + k][j + l];
				for (k = 0; k < 4; k ++)
					for (l = 0; l < 8; l++)
						B[j + k][i + l] = B[tempRow + k][tempColumn + l];
			}
			j = 56;
			for (i = 8; i < 56; i += 8) {
				for (k = 0; k < 4; k++) {
					for (l = 0; l < 4; l++)
						B[tempRow + l][tempColumn + k] = A[i + k][j + l];
					for (l = 4; l < 8; l++)
						B[tempRow + l - 4][tempColumn + k + 4] = A[i + k][j + l];
				}
				for (k = 4; k < 8; k++) {
					for (l = 4; l < 8; l++)
						B[j + l][i + k] = A[i + k][j + l];
					for (l = 0; l < 4; l++)
						B[j + k][i + l] = B[tempRow + k - 4][tempColumn + l + 4];
				}
				for (k = 4; k < 8; k++) 
					for (l = 0; l < 4; l++)
						B[tempRow + l][tempColumn + k] = A[i + k][j + l];
				for (k = 0; k < 4; k ++)
					for (l = 0; l < 8; l++)
						B[j + k][i + l] = B[tempRow + k][tempColumn + l];
			}
	
		tempRow = 8; tempColumn = 8; // The bottom left and top right submatrix consist the third part
		i = 56; j = 0; 
				for (k = 0; k < 4; k++) {
					for (l = 0; l < 4; l++)
						B[tempRow + l][tempColumn + k] = A[i + k][j + l];
					for (l = 4; l < 8; l++)
						B[tempRow + l - 4][tempColumn + k + 4] = A[i + k][j + l];
				}
				for (k = 4; k < 8; k++) {
					for (l = 4; l < 8; l++)
						B[j + l][i + k] = A[i + k][j + l];
					for (l = 0; l < 4; l++)
						B[j + k][i + l] = B[tempRow + k - 4][tempColumn + l + 4];
				}
				for (k = 4; k < 8; k++) 
					for (l = 0; l < 4; l++)
						B[tempRow + l][tempColumn + k] = A[i + k][j + l];
				
				for (k = 0; k < 4; k ++)
					for (l = 0; l < 8; l++)
						B[j + k][i + l] = B[tempRow + k][tempColumn + l];
		i = 0; j = 56;
				for (k = 0; k < 4; k++) {
					for (l = 0; l < 4; l++)
						B[tempRow + l][tempColumn + k] = A[i + k][j + l];
					for (l = 4; l < 8; l++)
						B[tempRow + l - 4][tempColumn + k + 4] = A[i + k][j + l];
				}
				for (k = 4; k < 8; k++) {
					for (l = 4; l < 8; l++)
						B[j + l][i + k] = A[i + k][j + l];
					for (l = 0; l < 4; l++)
						B[j + k][i + l] = B[tempRow + k - 4][tempColumn + l + 4];
				}
				for (k = 4; k < 8; k++) 
					for (l = 0; l < 4; l++)
						B[tempRow + l][tempColumn + k] = A[i + k][j + l];
				for (k = 0; k < 4; k ++)
					for (l = 0; l < 8; l++)
						B[j + k][i + l] = B[tempRow + k][tempColumn + l];

		tempRow = 56; tempColumn = 56; // Submatrices on diagonal are in the fourth part
		for (i = 0; i < 48; i += 8) {
				j = 48 - i;
				for (k = 0; k < 4; k++) {
					for (l = 0; l < 4; l++)
						B[tempRow + l][tempColumn + k] = A[i + k][i + l];
					for (l = 4; l < 8; l++)
						B[tempRow + l - 4][tempColumn + k + 4] = A[i + k][i + l];
				}
				for (k = 4; k < 8; k++) {
					for (l = 4; l < 8; l++)
						B[j + l][j + k] = A[i + k][i + l];
					for (l = 0; l < 4; l++)
						B[j + k][j + l] = B[tempRow + k - 4][tempColumn + l + 4];
				}
				for (k = 4; k < 8; k++) 
					for (l = 0; l < 4; l++)
						B[tempRow + l][tempColumn + k] = A[i + k][i + l];
				for (k = 0; k < 4; k ++)
					for (l = 0; l < 8; l++)
						B[j + k][j + l] = B[tempRow + k][tempColumn + l];
		}
		for (i = 0; i < 24; i += 8) {
				j = 48 - i;
				for (k = 0; k < 8; k++)
					for (l = 0; l < 8; l++) {
						temp = B[i + k][i + l];
						B[i + k][i + l] = B[j + k][j + l];
						B[j + k][j + l] = temp;
					}
		}
		for (i = 48; i < 64; i += 8)
		{
			j = 104 - i; // this makes j is one value of 48 & 56 when i is the other one.
			for (k = 0; k < 4; k++) {
				for (l = 0; l < 4; l++)
					B[j + l][j + k] = A[i + k][i + l];
				for (l = 4; l < 8; l++)
					B[j + l][j + k] = A[i + k][i + l];
			}
			for (k = 4; k < 8; k++) {
				for (l = 4; l < 8; l++)
					B[j + l][j + k] = A[i + k][i + l];
				for (l = 0; l < 4; l++)
					B[j + l][j + k] = A[i + k][i + l];
			}
		}
		i = 48; j = 56;
		for (k = 0; k < 8; k++)
			for (l = 0; l < 8; l++)
			{
				temp = B[j + k][j + l];
				B[j + k][j + l] = B[i + k][i + l];
				B[i + k][i + l] = temp;
			}

	} // if statement end

//--------------------------------------------------------------------
// 67 * 61 matrix
//--------------------------------------------------------------------
	if ((M == 61) && (N == 67)) {
		subMatrixSize = 18; // 17 and 18 give similar results.
 		for (i = 0; i < M; i += subMatrixSize)
			for (j = 0; j < N; j += subMatrixSize) {
				for (k = i; k < i + subMatrixSize; k++)
					for (l = j; l < j + subMatrixSize; l++)
						if (k > 60 || l > 66) continue;
						else B[k][l] = A[l][k];
		}
	}


    ENSURES(is_transpose(M, N, A, B));
}
/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    REQUIRES(M > 0);
    REQUIRES(N > 0);

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

    ENSURES(is_transpose(M, N, A, B));
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 
    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

