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

#ifndef AFQMC_CUDA_UTILITIES_HPP 
#define AFQMC_CUDA_UTILITIES_HPP

#define GPU_MEMORY_POINTER_TYPE      1001
#define MANAGED_MEMORY_POINTER_TYPE  2001
#define CPU_OUTOFCARS_POINTER_TYPE   3001

#include<cassert>
#include<cstdlib>
#include<iostream>
#include<stdexcept>
#include <cuda_runtime.h>
#include "cublas_v2.h"
#include "cublasXt.h"
#include "cusolverDn.h"
#include "curand.h"
#include "mpi3/communicator.hpp"
#include "mpi3/shared_communicator.hpp"

namespace qmc_cuda {

/*
// Do I need these here???
  extern cublasHandle_t afqmc_cublas_handle;
  extern cublasXtHandle_t afqmc_cublasXt_handle;
  extern cusolverDnHandle_t afqmc_cusolverDn_handle;
  extern curandGenerator_t afqmc_curand_generator;
*/

  void cuda_check(cudaError_t sucess, std::string message="");
  void cublas_check(cublasStatus_t sucess, std::string message="");
  void curand_check(curandStatus_t sucess, std::string message="");
  void cusolver_check(cusolverStatus_t sucess, std::string message="");
  cublasOperation_t cublasOperation(char A); 

  void CUDA_INIT(boost::mpi3::shared_communicator& node);

  struct gpu_handles {
    cublasHandle_t* cublas_handle;
    cublasXtHandle_t* cublasXt_handle;
    cusolverDnHandle_t* cusolverDn_handle; 
    curandGenerator_t* curand_generator;
    bool operator==(gpu_handles const& other) const {
      return (cublas_handle==other.cublas_handle &&
              cublasXt_handle==other.cublasXt_handle &&
              cusolverDn_handle==other.cusolverDn_handle &&
              curand_generator==other.curand_generator);
    }
    bool operator!=(gpu_handles const& other) const { return not (*this == other); }
  };
}

#endif
