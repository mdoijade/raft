/*
 * Copyright (c) 2019-2020, NVIDIA CORPORATION.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "raft/error.hpp"

#include <cuda_runtime.h>

#include <execinfo.h>
#include <cstdio>

///@todo: enable once logging has been enabled in raft
//#include "logger.hpp"

namespace raft {

/** check for cuda runtime API errors and assert accordingly */
#define CUDA_CHECK(call)                                               \
  do {                                                                 \
    cudaError_t status = call;                                         \
    ASSERT(status == cudaSuccess, "FAIL: call='%s'. Reason:%s", #call, \
           cudaGetErrorString(status));                                \
  } while (0)

///@todo: enable this only after we have added logging support in raft
// /**
//  * @brief check for cuda runtime API errors but log error instead of raising
//  *        exception.
//  */
#define CUDA_CHECK_NO_THROW(call)                                         \
  do {                                                                    \
    cudaError_t status = call;                                            \
    if (status != cudaSuccess) {                                          \
      printf("CUDA call='%s' at file=%s line=%d failed with %s\n", #call, \
             __FILE__, __LINE__, cudaGetErrorString(status));             \
    }                                                                     \
  } while (0)

/** helper method to get max usable shared mem per block parameter */
inline int get_shared_memory_per_block() {
  int dev_id;
  CUDA_CHECK(cudaGetDevice(&dev_id));
  int smem_per_blk;
  CUDA_CHECK(cudaDeviceGetAttribute(
    &smem_per_blk, cudaDevAttrMaxSharedMemoryPerBlock, dev_id));
  return smem_per_blk;
}
/** helper method to get multi-processor count parameter */
inline int get_multi_processor_count() {
  int dev_id;
  CUDA_CHECK(cudaGetDevice(&dev_id));
  int mp_count;
  CUDA_CHECK(
    cudaDeviceGetAttribute(&mp_count, cudaDevAttrMultiProcessorCount, dev_id));
  return mp_count;
}

/** Helper method to get to know warp size in device code */
constexpr inline int warp_size() { return 32; }

/**
 * @brief Generic copy method for all kinds of transfers
 * @tparam Type data type
 * @param dst destination pointer
 * @param src source pointer
 * @param len lenth of the src/dst buffers in terms of number of elements
 * @param stream cuda stream
 */
template <typename Type>
void copy(Type* dst, const Type* src, size_t len, cudaStream_t stream) {
  CUDA_CHECK(
    cudaMemcpyAsync(dst, src, len * sizeof(Type), cudaMemcpyDefault, stream));
}

/**
 * @defgroup Copy Copy methods
 * These are here along with the generic 'copy' method in order to improve
 * code readability using explicitly specified function names
 * @{
 */
/** performs a host to device copy */
template <typename Type>
void update_device(Type* d_ptr, const Type* h_ptr, size_t len,
                   cudaStream_t stream) {
  copy(d_ptr, h_ptr, len, stream);
}

/** performs a device to host copy */
template <typename Type>
void update_host(Type* h_ptr, const Type* d_ptr, size_t len,
                 cudaStream_t stream) {
  copy(h_ptr, d_ptr, len, stream);
}

template <typename Type>
void copy_async(Type* d_ptr1, const Type* d_ptr2, size_t len,
                cudaStream_t stream) {
  CUDA_CHECK(cudaMemcpyAsync(d_ptr1, d_ptr2, len * sizeof(Type),
                             cudaMemcpyDeviceToDevice, stream));
}
/** @} */

/**
 * @defgroup Debug Utils for debugging host/device buffers
 * @{
 */
template <class T, class OutStream>
void print_host_vector(const char* variable_name, const T* host_mem,
                       size_t componentsCount, OutStream& out) {
  out << variable_name << "=[";
  for (size_t i = 0; i < componentsCount; ++i) {
    if (i != 0) out << ",";
    out << host_mem[i];
  }
  out << "];\n";
}

template <class T, class OutStream>
void print_device_vector(const char* variable_name, const T* devMem,
                         size_t componentsCount, OutStream& out) {
  T* host_mem = new T[componentsCount];
  CUDA_CHECK(cudaMemcpy(host_mem, devMem, componentsCount * sizeof(T),
                        cudaMemcpyDeviceToHost));
  print_host_vector(variable_name, host_mem, componentsCount, out);
  delete[] host_mem;
}
/** @} */

};  // namespace raft
