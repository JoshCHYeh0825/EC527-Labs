/* 
	nvcc GPT_SOR.cu -o GPT_SOR
*/

#include <cstdio>
#include <cstdlib>
#include <math.h>

// Assertion to check for errors
#define CUDA_SAFE_CALL(ans) { gpuAssert((ans), (char *)__FILE__, __LINE__); }
inline void gpuAssert(cudaError_t code, char *file, int line, bool abort=true)
{
  if (code != cudaSuccess)
  {
    fprintf(stderr, "CUDA_SAFE_CALL: %s %s %d\n",
                                       cudaGetErrorString(code), file, line);
    if (abort) exit(code);
  }
}

#define N 2048
#define ITERATIONS 2000
#define BLOCK_SIZE 16

void initializeArray1D(float *arr, int len, int seed);

__global__ void sor_kernel(float* A, float* B, int n) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x > 0 && x < n - 1 && y > 0 && y < n - 1) {
        for (int iter = 0; iter < ITERATIONS; iter++) {
            B[y * n + x] = 0.25f * (A[(y - 1) * n + x] + A[(y + 1) * n + x] + A[y * n + (x - 1)] + A[y * n + (x + 1)]);
            __syncthreads();
            A[y * n + x] = B[y * n + x];
            __syncthreads();
        }
    }
}

void sor_host(float* A, float* B, int n) {
    for (int iter = 0; iter < ITERATIONS; iter++) {
        for (int y = 1; y < n - 1; y++) {
            for (int x = 1; x < n - 1; x++) {
                B[y * n + x] = 0.25f * (A[(y - 1) * n + x] + A[(y + 1) * n + x] + A[y * n + (x - 1)] + A[y * n + (x + 1)]);
            }
        }
        memcpy(A, B, n * n * sizeof(float));
    }
}

int main() {

    cudaEvent_t start, stop;
    float elapsed_gpu, elapsed_cpu;
    
    float *h_A, *h_B, *h_B_gold;
    float *d_A, *d_B;
    size_t size = N * N * sizeof(float);

    h_A = (float*)malloc(size);
    h_B = (float*)malloc(size);
    h_B_gold = (float*)malloc(size);
    initializeArray1D(h_A, N * N, 2453);
    memcpy(h_B, h_A, size);
    memcpy(h_B_gold, h_A, size);

    CUDA_SAFE_CALL(cudaMalloc(&d_A, size));
    CUDA_SAFE_CALL(cudaMalloc(&d_B, size));
    CUDA_SAFE_CALL(cudaMemcpy(d_A, h_A, size, cudaMemcpyHostToDevice));
    
    dim3 threadsPerBlock(BLOCK_SIZE, BLOCK_SIZE);
    dim3 numBlocks(N / BLOCK_SIZE, N / BLOCK_SIZE);
    
    cudaEventCreate(&start);
    cudaEventCreate(&stop);
    cudaEventRecord(start, 0);
    sor_kernel<<<numBlocks, threadsPerBlock>>>(d_A, d_B, N);
    cudaEventRecord(stop, 0);
    cudaEventSynchronize(stop);
    cudaEventElapsedTime(&elapsed_gpu, start, stop);
    CUDA_SAFE_CALL(cudaMemcpy(h_B, d_B, size, cudaMemcpyDeviceToHost));
    
    cudaEventDestroy(start);
    cudaEventDestroy(stop);

    printf("GPU time: %f ms\n", elapsed_gpu);

    cudaEventCreate(&start);
    cudaEventCreate(&stop);
    cudaEventRecord(start, 0);
    sor_host(h_A, h_B_gold, N);
    cudaEventRecord(stop, 0);
    cudaEventSynchronize(stop);
    cudaEventElapsedTime(&elapsed_cpu, start, stop);
    cudaEventDestroy(start);
    cudaEventDestroy(stop);
    printf("CPU time: %f ms\n", elapsed_cpu);

    int errors = 0;
    for (int i = 0; i < N * N; i++) {
        if (fabs(h_B[i] - h_B_gold[i]) > 1e-6) errors++;
    }
    printf("Mismatches: %d\n", errors);

    CUDA_SAFE_CALL(cudaFree(d_A));
    CUDA_SAFE_CALL(cudaFree(d_B));
    free(h_A);
    free(h_B);
    free(h_B_gold);
    return 0;
}

void initializeArray1D(float *arr, int len, int seed) {
    int i;
    float randNum;
    srand(seed);
  
    for (i = 0; i < len; i++) {
      randNum = (float) rand();
      arr[i] = randNum;
    }
  }