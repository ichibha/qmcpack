//////////////////////////////////////////////////////////////////////
// This file is distributed under the University of Illinois/NCSA Open Source
// License.  See LICENSE file in top directory for details.
//
// Copyright (c) 2016 Jeongnim Kim and QMCPACK developers.
//
// File developed by:
//    Lawrence Livermore National Laboratory 
//
// File created by:
// Miguel A. Morales, moralessilva2@llnl.gov 
//    Lawrence Livermore National Laboratory 
////////////////////////////////////////////////////////////////////////////////

#ifndef AFQMC_BLAS_CUDA_GPU_PTR_HPP
#define AFQMC_BLAS_CUDA_GPU_PTR_HPP

#include<cassert>
#include<vector>
#include "AFQMC/Memory/CUDA/cuda_gpu_pointer.hpp"
#include "AFQMC/Numerics/detail/CUDA/cublas_wrapper.hpp"
#include "AFQMC/Numerics/detail/CUDA/cublasXt_wrapper.hpp"
// hand coded kernels for blas extensions
#include "AFQMC/Kernels/adotpby.cuh"
#include "AFQMC/Kernels/axty.cuh"
#include "AFQMC/Kernels/sum.cuh"
#include "AFQMC/Kernels/adiagApy.cuh"
#include "AFQMC/Kernels/acAxpbB.cuh"
#include "AFQMC/Kernels/zero_complex_part.cuh"

// Currently available:
// Lvl-1: dot, axpy, scal
// Lvl-2: gemv
// Lvl-3: gemm

namespace qmc_cuda 
{
  // copy Specializations
  template<typename T, typename Q>
  inline static void copy(int n, cuda_gpu_ptr<Q> x, int incx, cuda_gpu_ptr<T> && y, int incy)
  {
    if(CUBLAS_STATUS_SUCCESS != cublas::cublas_copy(*x.handles.cublas_handle,n,to_address(x),incx,to_address(y),incy))
      throw std::runtime_error("Error: cublas_copy returned error code.");
  }

  template<typename T>
  inline static void copy(int n, T const* x, int incx, cuda_gpu_ptr<T> && y, int incy)
  {
    if(cudaSuccess != cudaMemcpy2D(to_address(y),sizeof(T)*incy,
                                   x,sizeof(T)*incx,
                                   sizeof(T),n,cudaMemcpyHostToDevice))
      throw std::runtime_error("Error: cudaMemcpy2D returned error code.");
  }

  template<typename T, typename Q>
  inline static void copy(int n, cuda_gpu_ptr<Q> x, int incx, T* y, int incy)
  {
    static_assert(std::is_same<typename std::decay<Q>::type,T>::value,"Wrong dispatch.\n");
    assert(sizeof(Q)==sizeof(T));
    if(cudaSuccess != cudaMemcpy2D(y,sizeof(T)*incy,
                                   to_address(x),sizeof(Q)*incx,
                                   sizeof(T),n,cudaMemcpyDeviceToHost))
      throw std::runtime_error("Error: cudaMemcpy2D returned error code.");
  }

  // scal Specializations
  template<typename T, typename Q>
  inline static void scal(int n, Q alpha, cuda_gpu_ptr<T> && x, int incx=1)
  {
    if(CUBLAS_STATUS_SUCCESS != cublas::cublas_scal(*x.handles.cublas_handle,n,T(alpha),to_address(x),incx))
      throw std::runtime_error("Error: cublas_scal returned error code.");
  }

  // dot Specializations
  template<typename T>
  inline static auto dot(int const n, cuda_gpu_ptr<T const> x, int const incx, 
                                      cuda_gpu_ptr<T const> y, int const incy)
  {
    return cublas::cublas_dot(*x.handles.cublas_handle,n,to_address(x),incx,to_address(y),incy);
  }

  // axpy Specializations
  template<typename T>
  inline static void axpy(int n, T const a,
                          cuda_gpu_ptr<T const> x, int incx, 
                          cuda_gpu_ptr<T> && y, int incy)
  {
    if(CUBLAS_STATUS_SUCCESS != cublas::cublas_axpy(*x.handles.cublas_handle,n,a,
                                                    to_address(x),incx,to_address(y),incy))
      throw std::runtime_error("Error: cublas_axpy returned error code.");
  }

  // GEMV Specializations
  template<typename T>
  inline static void gemv(char Atrans, int M, int N,
                          T alpha,
                          cuda_gpu_ptr<T const> A, int lda,
                          cuda_gpu_ptr<T const> x, int incx,
                          T beta,
                          cuda_gpu_ptr<T> && y, int incy)
  {
    if(CUBLAS_STATUS_SUCCESS != cublas::cublas_gemv(*A.handles.cublas_handle,Atrans,
                                            M,N,alpha,to_address(A),lda,to_address(x),incx,
                                            beta,to_address(y),incy)) 
      throw std::runtime_error("Error: cublas_gemv returned error code.");
  }

  // GEMM Specializations
// why is this not working with T const????
  template<typename T, typename Q1, typename Q2> 
  inline static void gemm(char Atrans, char Btrans, int M, int N, int K,
                          T alpha,
                          cuda_gpu_ptr<Q1> const& A, int lda,
                          cuda_gpu_ptr<Q2> const& B, int ldb,
                          T beta,
                          cuda_gpu_ptr<T> && C, int ldc)
  {
    if(CUBLAS_STATUS_SUCCESS != cublas::cublas_gemm(*A.handles.cublas_handle,Atrans,Btrans,
                                            M,N,K,alpha,to_address(A),lda,to_address(B),ldb,beta,to_address(C),ldc)) 
      throw std::runtime_error("Error: cublas_gemm returned error code.");
  }

  // Blas Extensions
  // geam  
  template<typename T, typename Q1, typename Q2>
  inline static void geam(char Atrans, char Btrans, int M, int N,
                         T const alpha,
                         cuda_gpu_ptr<Q1> A, int lda,
                         T const beta,
                         cuda_gpu_ptr<Q2> B, int ldb,
                         cuda_gpu_ptr<T> && C, int ldc)
  {
    if(CUBLAS_STATUS_SUCCESS != cublas::cublas_geam(*A.handles.cublas_handle,Atrans,Btrans,M,N,alpha,to_address(A),lda,
                                                    beta,to_address(B),ldb,to_address(C),ldc))
      throw std::runtime_error("Error: cublas_geam returned error code.");
  }

  template<typename T>
  //inline static void set1D(int n, T const alpha, ptr x, int incx)
  inline static void set1D(int n, T const alpha, cuda_gpu_ptr<T> x, int incx)
  {
    // No set funcion in cuda!!! Avoiding kernels for now
    //std::vector<T> buff(n,alpha); 
    //if(CUBLAS_STATUS_SUCCESS != cublasSetVector(n,sizeof(T),buff.data(),1,to_address(x),incx)) 
    T alpha_(alpha);
    if(CUBLAS_STATUS_SUCCESS != cublasSetVector(n,sizeof(T),std::addressof(alpha),1,to_address(x),incx)) 
      throw std::runtime_error("Error: cublasSetVector returned error code.");
  }

  // dot extension 
  template<typename T, typename Q>
  inline static void adotpby(int const n, T const alpha, cuda_gpu_ptr<T const> x, int const incx, 
                                cuda_gpu_ptr<T const> y, int const incy, 
                                Q const beta, cuda_gpu_ptr<Q> && result)
  {
    kernels::adotpby(n,alpha,to_address(x),incx,to_address(y),incy,beta,to_address(result));
  }

  // axty
  template<typename T>
  inline static void axty(int n,
                         T const alpha,
                         cuda_gpu_ptr<T const> x, int incx,
                         cuda_gpu_ptr<T> && y, int incy)
  {
    if(incx != 1 || incy != 1)
      throw std::runtime_error("Error: axty with inc != 1 not implemented.");
    kernels::axty(n,alpha,to_address(x),to_address(y));
  }

  // acAxpbB
  template<typename T>
  inline static void acAxpbB(int m, int n,
                             T const alpha,
                             cuda_gpu_ptr<T const> A, int lda,
                             cuda_gpu_ptr<T const> x, int incx,
                             T const beta,
                             cuda_gpu_ptr<T> && B, int ldb)
  {
    kernels::acAxpbB(m,n,alpha,to_address(A),lda,to_address(x),incx,beta,to_address(B),ldb);
  }

  // adiagApy
  template<typename T>
  inline static void adiagApy(int n,
                         T const alpha,
                         cuda_gpu_ptr<T const> A, int lda,
                         cuda_gpu_ptr<T> && y, int incy)
  {
    kernels::adiagApy(n,alpha,to_address(A),lda,to_address(y),incy);
  }

  template<typename T>
  inline static void zero_complex_part(int n, cuda_gpu_ptr<T> x)
  {
    kernels::zero_complex_part(n,to_address(x));
  }

  template<typename T>
  inline static auto sum(int n, cuda_gpu_ptr<T const> x, int incx) 
  {
    return kernels::sum(n,to_address(x),incx);
  }

  template<typename T>
  inline static auto sum(int m, int n, cuda_gpu_ptr<T const> A, int lda)
  {
    return kernels::sum(m,n,to_address(A),lda);
  }

  template<typename T>
  inline static void gemmStridedBatched(char Atrans, char Btrans, int M, int N, int K,
                          T const alpha, cuda_gpu_ptr<T const> A, int lda, int strideA,
                          cuda_gpu_ptr<T const> B, int ldb, int strideB, T beta,
                          cuda_gpu_ptr<T> && C, int ldc, int strideC, int batchSize)
  {
    cublas::cublas_gemmStridedBatched(*A.handles.cublas_handle,Atrans,Btrans,M,N,K,
               alpha,to_address(A),lda,strideA,to_address(B),ldb,strideB,
               beta,to_address(C),ldc,strideC,batchSize);
  }

  template<typename T>
  inline static void gemmBatched(char Atrans, char Btrans, int M, int N, int K,
                          T const alpha, cuda_gpu_ptr<T const> const* A, int lda, 
                          cuda_gpu_ptr<T const> const* B, int ldb, T beta,
                          cuda_gpu_ptr<T> * C, int ldc, int batchSize)
  {
    T **A_d, **B_d, **C_d;
    T **A_h, **B_h, **C_h;
    A_h = new T*[batchSize];
    B_h = new T*[batchSize];
    C_h = new T*[batchSize];
    for(int i=0; i<batchSize; i++) {
      A_h[i] = to_address(A[i]);
      B_h[i] = to_address(B[i]);
      C_h[i] = to_address(C[i]);
    }
    cudaMalloc((void **)&A_d,  batchSize*sizeof(*A_h));
    cudaMalloc((void **)&B_d,  batchSize*sizeof(*B_h));
    cudaMalloc((void **)&C_d,  batchSize*sizeof(*C_h));
    cudaMemcpy(A_d, A_h, batchSize*sizeof(*A_h), cudaMemcpyHostToDevice);
    cudaMemcpy(B_d, B_h, batchSize*sizeof(*B_h), cudaMemcpyHostToDevice);
    cudaMemcpy(C_d, C_h, batchSize*sizeof(*C_h), cudaMemcpyHostToDevice);
    cublas::cublas_gemmBatched(*(A[0]).handles.cublas_handle,Atrans,Btrans,M,N,K,
               alpha,A_d,lda,B_d,ldb,beta,C_d,ldc,batchSize);
    cudaFree(A_d);
    cudaFree(B_d);
    cudaFree(C_d);
    delete [] A_h;
    delete [] B_h;
    delete [] C_h;
  }

}

#endif
