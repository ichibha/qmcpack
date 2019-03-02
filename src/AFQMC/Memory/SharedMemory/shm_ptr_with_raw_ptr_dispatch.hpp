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

#ifndef AFQMC_SHM_PTR_WITH_RAW_PTR_DISPATCH_HPP
#define AFQMC_SHM_PTR_WITH_RAW_PTR_DISPATCH_HPP

#include <functional>
#include "Configuration.h"
#include<cassert>

#include "multi/array_ref.hpp"
#include "mpi3/communicator.hpp"
#include "mpi3/shared_communicator.hpp"
#include "mpi3/shared_window.hpp"
#include "AFQMC/Memory/raw_pointers.hpp"

namespace shm {

using qmcplusplus::afqmc::to_address;

namespace mpi3 = boost::mpi3;
namespace multi = boost::multi;

template<class T> struct allocator_shm_ptr_with_raw_ptr_dispatch;
template<class T> struct shm_ptr_with_raw_ptr_dispatch;

template<>
struct shm_ptr_with_raw_ptr_dispatch<const void>{
        using T = const void;
        std::shared_ptr<mpi3::shared_window<>> wSP_;
        std::ptrdiff_t offset = 0;
        shm_ptr_with_raw_ptr_dispatch(std::nullptr_t = nullptr){}
        shm_ptr_with_raw_ptr_dispatch(shm_ptr_with_raw_ptr_dispatch const& other) = default;
        shm_ptr_with_raw_ptr_dispatch& operator=(shm_ptr_with_raw_ptr_dispatch const& other) = default;
        shm_ptr_with_raw_ptr_dispatch& operator=(std::nullptr_t){wSP_.reset(); return *this;}
        bool operator==(std::nullptr_t) const{return (bool)wSP_;}
        bool operator!=(std::nullptr_t) const{return not operator==(nullptr);}
};

template<>
struct shm_ptr_with_raw_ptr_dispatch<void>{
        using T = void;
        using element_type = T;
        std::shared_ptr<mpi3::shared_window<>> wSP_;
        std::ptrdiff_t offset = 0;
        shm_ptr_with_raw_ptr_dispatch(std::nullptr_t = nullptr){}
        shm_ptr_with_raw_ptr_dispatch(shm_ptr_with_raw_ptr_dispatch const& other) = default;
        shm_ptr_with_raw_ptr_dispatch& operator=(shm_ptr_with_raw_ptr_dispatch const& other) = default;
        shm_ptr_with_raw_ptr_dispatch& operator=(std::nullptr_t){wSP_.reset(); return *this;}
        bool operator==(std::nullptr_t) const{return (bool)wSP_;}
        bool operator!=(std::nullptr_t) const{return not operator==(nullptr);}
};

template<class T>
struct shm_ptr_with_raw_ptr_dispatch{
        using element_type = T;
        using difference_type = std::ptrdiff_t;
        using value_type = std::decay_t<T>; // std::remove_cv_t<T>; // T until C++20?
        using pointer = T*; // TODO self?
        using reference = T&; //TODO fancy_reference?
        using iterator_category = std::random_access_iterator_tag;
        std::shared_ptr<mpi3::shared_window<value_type>> wSP_;
        std::ptrdiff_t offset = 0;
        shm_ptr_with_raw_ptr_dispatch(){}
        shm_ptr_with_raw_ptr_dispatch(std::nullptr_t){}
        shm_ptr_with_raw_ptr_dispatch(shm_ptr_with_raw_ptr_dispatch const& other) = default;
        shm_ptr_with_raw_ptr_dispatch& operator=(shm_ptr_with_raw_ptr_dispatch const& other) = default;
        shm_ptr_with_raw_ptr_dispatch& operator=(std::nullptr_t){return *this;}
        ~shm_ptr_with_raw_ptr_dispatch() = default;
        T& operator*() const{return *((T*)(wSP_->base(0)) + offset);}
        T& operator[](int idx) const{return ((T*)(wSP_->base(0)) + offset)[idx];}
        T* operator->() const{return (T*)(wSP_->base(0)) + offset;}
        T* get() const{return wSP_->base(0) + offset;}
        explicit operator T*() const{return get();}
        explicit operator bool() const{return (bool)wSP_;}//.get();}
        bool operator==(std::nullptr_t) const{return not (bool)wSP_;}
        bool operator!=(std::nullptr_t) const{return not operator==(nullptr);}
        operator shm_ptr_with_raw_ptr_dispatch<T const>() const{
                shm_ptr_with_raw_ptr_dispatch<T const> ret;
                ret.wSP_ = wSP_;
                ret.offset = offset;
                return ret;
        }
        operator shm_ptr_with_raw_ptr_dispatch<void const>() const{
                shm_ptr_with_raw_ptr_dispatch<void const> ret;
                ret.wSP_ = wSP_;
                ret.offset = offset;
                return ret;
        }
        shm_ptr_with_raw_ptr_dispatch operator+(std::ptrdiff_t d) const{
                shm_ptr_with_raw_ptr_dispatch ret(*this);
                ret += d;
                return ret;
        }
        shm_ptr_with_raw_ptr_dispatch operator-(std::ptrdiff_t d) const{
                shm_ptr_with_raw_ptr_dispatch ret(*this);
                ret -= d;
                return ret;
        }
        std::ptrdiff_t operator-(shm_ptr_with_raw_ptr_dispatch other) const{return offset-other.offset;}
        shm_ptr_with_raw_ptr_dispatch& operator--(){--offset; return *this;}
        shm_ptr_with_raw_ptr_dispatch& operator++(){++offset; return *this;}
        shm_ptr_with_raw_ptr_dispatch& operator-=(std::ptrdiff_t d){offset -= d; return *this;}
        shm_ptr_with_raw_ptr_dispatch& operator+=(std::ptrdiff_t d){offset += d; return *this;}
        bool operator==(shm_ptr_with_raw_ptr_dispatch<T> const& other) const{
                return wSP_->base(0) == other.wSP_->base(0) and offset == other.offset;
        }
        bool operator!=(shm_ptr_with_raw_ptr_dispatch<T> const& other) const{return not((*this)==other);}
        bool operator<(shm_ptr_with_raw_ptr_dispatch<T> const& other) const{
                return wSP_->base(0) + offset < other.wSP_->base(0) + other.offset;
        }
        static element_type* to_address(shm_ptr_with_raw_ptr_dispatch p) noexcept{
                return p.wSP_->base(0) + p.offset;
        }
        friend pointer to_address(shm_ptr_with_raw_ptr_dispatch const& p){return shm_ptr_with_raw_ptr_dispatch::to_address(p);}
};

template<class T = void> struct allocator_shm_ptr_with_raw_ptr_dispatch{
        template<class U> struct rebind{typedef allocator_shm_ptr_with_raw_ptr_dispatch<U> other;};
        using value_type = T;
        using pointer = shm_ptr_with_raw_ptr_dispatch<T>;
        using const_pointer = shm_ptr_with_raw_ptr_dispatch<T const>;
        using size_type = mpi3::size_t; // std::size_t; 
        using difference_type = std::make_signed_t<size_type>;//std::ptrdiff_t;

        mpi3::shared_communicator& comm_;
        allocator_shm_ptr_with_raw_ptr_dispatch() = delete;
        allocator_shm_ptr_with_raw_ptr_dispatch(mpi3::shared_communicator& comm) : comm_(comm){}
        allocator_shm_ptr_with_raw_ptr_dispatch(allocator_shm_ptr_with_raw_ptr_dispatch const& other) : comm_(other.comm_){}
        ~allocator_shm_ptr_with_raw_ptr_dispatch() = default;
        template<class U>  allocator_shm_ptr_with_raw_ptr_dispatch(allocator_shm_ptr_with_raw_ptr_dispatch<U> const& o) : comm_(o.comm_){}

        shm_ptr_with_raw_ptr_dispatch<T> allocate(size_type n, const void* /*hint*/ = 0){
                shm_ptr_with_raw_ptr_dispatch<T> ret = 0;
                if(n == 0){
                        ret.wSP_ = std::make_shared<mpi3::shared_window<T>>(
                                comm_.make_shared_window<T>(0)
                        );
                        return ret;
                }
                ret.wSP_ = std::make_shared<mpi3::shared_window<T>>(
                        comm_.make_shared_window<T>(comm_.root()?n:0)
                );
                return ret;
        }
        void deallocate(shm_ptr_with_raw_ptr_dispatch<T> ptr, size_type){ptr.wSP_.reset();}
        allocator_shm_ptr_with_raw_ptr_dispatch& operator=(allocator_shm_ptr_with_raw_ptr_dispatch const& other){
                assert( (*this)==other ); // TODO make comm a shared_ptr
                return *this;
        }
        bool operator==(allocator_shm_ptr_with_raw_ptr_dispatch const& other) const{return comm_ == other.comm_;}
        bool operator!=(allocator_shm_ptr_with_raw_ptr_dispatch const& other) const{return not(other == *this);}
        template<class U, class... As>
        void construct(U* p, As&&... as){::new((void*)p) U(std::forward<As>(as)...);}
        template< class U >     void destroy(U* p){p->~U();}
};


template<typename T, typename Size, typename TT>
shm_ptr_with_raw_ptr_dispatch<T> uninitialized_fill_n(shm_ptr_with_raw_ptr_dispatch<T> first, Size n, TT const& val){
        if(n == 0) return first;
        if(mpi3::group(*first.wSP_).root()) std::uninitialized_fill_n(to_address(first), n, val); // change to to_pointer
        first.wSP_->fence();
        first.wSP_->fence();
        return first + n;
}

template<typename T, typename Size>
shm_ptr_with_raw_ptr_dispatch<T> destroy_n(shm_ptr_with_raw_ptr_dispatch<T> first, Size n){
        if(n == 0) return first;
//        using std::destroy_n;
        if(mpi3::group(*first.wSP_).root()) { //destroy_n(to_address(first), n);
              auto first_ptr = to_address(first);
              for(; n > 0; (void) ++first_ptr, --n) first->~T();
        }
        first.wSP_->fence();
        first.wSP_->fence();
        return first + n;
}

template<class It1, typename T, typename Size>
shm_ptr_with_raw_ptr_dispatch<T> copy_n(It1 first, Size n, shm_ptr_with_raw_ptr_dispatch<T> d_first){
        d_first.wSP_->fence();
        using std::copy_n;
        if(mpi3::group(*d_first.wSP_).root()) copy_n(first, n, to_address(d_first));
        d_first.wSP_->fence();
        return d_first + n;
}

template<class It1, typename T>
shm_ptr_with_raw_ptr_dispatch<T> copy(It1 first, It1 last, shm_ptr_with_raw_ptr_dispatch<T> d_first){
        if(first == last) return d_first;
        first.wSP_->fence();
        using std::copy;
        if(mpi3::group(*d_first.wSP_).root()) copy(first, last, to_address(d_first));
        first.wSP_->fence();
        using std::distance;
        return d_first + distance(first, last);
}

template<class It1, class Size, typename T>
shm_ptr_with_raw_ptr_dispatch<T> uninitialized_copy_n(It1 f, Size n, shm_ptr_with_raw_ptr_dispatch<T> d){
        if(n == 0) return d;
        f.wSP_->fence();
        using std::uninitialized_copy_n;
        if(mpi3::group(*d.wSP_).root()) uninitialized_copy_n(f, n, to_address(d));
        f.wSP_->fence();
        return d + n;
}

template<class It1, typename T>
shm_ptr_with_raw_ptr_dispatch<T> uninitialized_copy(It1 f, It1 l, shm_ptr_with_raw_ptr_dispatch<T> d){
        if(f == l) return d;
        f.wSP_->fence();
        using std::uninitialized_copy;
        if(mpi3::group(*d.wSP_).root()) uninitialized_copy(f, l, to_address(d));
        f.wSP_->fence();
        using std::distance;
        return d + distance(f, l);
}

template<class T, class Size>
shm_ptr_with_raw_ptr_dispatch<T> uninitialized_default_construct_n(shm_ptr_with_raw_ptr_dispatch<T> f, Size n){
        if(n == 0) return f;
        f.wSP_->fence();
        if(group(*f.wSP_).root()) {
//            uninitialized_value_construct_n(to_address(f), n);
            T* current(to_address(f));
            try{
                for(; n > 0; ++current, --n) (::new((void*)current) T()); 
                return current;
            }catch(...) {throw;} // leak!
        }
        f.wSP_->fence();
        return f + n;
}

template<class T, class Size>
shm_ptr_with_raw_ptr_dispatch<T> uninitialized_value_construct_n(shm_ptr_with_raw_ptr_dispatch<T> f, Size n){
        if(n == 0) return f;
        f.wSP_->fence();
        if(group(*f.wSP_).root()) {
//            uninitialized_value_construct_n(to_address(f), n);
            T* current(to_address(f));
            try{
                for(; n > 0; ++current, --n) (::new((void*)current) T());
                return current;
            }catch(...){throw;} // leak !!
        }
        f.wSP_->fence();
        return f + n;
}

}

#if 0
namespace boost{
namespace multi{

using qmcplusplus::afqmc::to_address;

/**************** copy *****************/
// Can always call cudaMemcopy2D like you do in the blas backend

template<class Alloc, typename T, typename Size>
multi::array_iterator<T, 1, shm::shm_ptr_with_raw_ptr_dispatch<T>> uninitialized_fill_n(
                    Alloc &a,
                    multi::array_iterator<T, 1, shm::shm_ptr_with_raw_ptr_dispatch<T>> first, 
                    Size n, T const& val){
  if(n == 0) return first;
  kernels::fill_n(to_address(base(first)), n, stride(first), val);
  return first + n;
}

template<class Alloc, typename T, typename Size>
multi::array_iterator<T, 1, shm::shm_ptr_with_raw_ptr_dispatch<T>> uninitialized_fill(      
                    Alloc &a,
                    multi::array_iterator<T, 1, shm::shm_ptr_with_raw_ptr_dispatch<T>> first, 
                    multi::array_iterator<T, 1, shm::shm_ptr_with_raw_ptr_dispatch<T>> last, T const& val){
  assert( stride(first) == stride(last) );
  if(std::distance(first,last) == 0 ) return first;
  kernels::fill_n(to_address(base(first)), std::distance(first,last), stride(first), val);
  return first + std::distance(first,last);
}

template<class T, class Q1, class Q2>
multi::array_iterator<T, 1, shm::shm_ptr_with_raw_ptr_dispatch<T>> copy( 
           multi::array_iterator<Q1, 1, shm::shm_ptr_with_raw_ptr_dispatch<Q2>> first,
           multi::array_iterator<Q1, 1, shm::shm_ptr_with_raw_ptr_dispatch<Q2>> last,
           multi::array_iterator<T, 1, shm::shm_ptr_with_raw_ptr_dispatch<T>> dest ){
  static_assert(std::is_same<typename std::decay<Q1>::type,T>::value,"Wrong dispatch.\n");
  static_assert(std::is_same<typename std::decay<Q2>::type,T>::value,"Wrong dispatch.\n");
  assert( stride(first) == stride(last) );
  if(std::distance(first,last) == 0 ) return dest;
  if(cudaSuccess != cudaMemcpy2D(to_address(base(dest)),sizeof(T)*stride(dest),
                                 to_address(base(first)),sizeof(T)*stride(first),
                                 sizeof(T),std::distance(first,last),cudaMemcpyDeviceToDevice))
      throw std::runtime_error("Error: cudaMemcpy2D returned error code.");
  return dest+std::distance(first,last);
}

template<class T, class ForwardIt>
multi::array_iterator<T, 1, shm::shm_ptr_with_raw_ptr_dispatch<T>> copy(
                         ForwardIt first,
                         ForwardIt last,
                         multi::array_iterator<T, 1, shm::shm_ptr_with_raw_ptr_dispatch<T>> dest ){
  assert( stride(first) == stride(last) );
  if(std::distance(first,last) == 0 ) return dest;
  if(cudaSuccess != cudaMemcpy2D(to_address(base(dest)),sizeof(T)*stride(dest),
                                 to_address(base(first)),sizeof(T)*stride(first),
                                 sizeof(T),std::distance(first,last),cudaMemcpyDefault))
      throw std::runtime_error("Error: cudaMemcpy2D returned error code.");
  return dest+std::distance(first,last);
}

template<class T, class Q1, class Q2>
multi::array_iterator<T, 1, T*> copy(
                         multi::array_iterator<Q1, 1, shm::shm_ptr_with_raw_ptr_dispatch<Q2>> first,
                         multi::array_iterator<Q1, 1, shm::shm_ptr_with_raw_ptr_dispatch<Q2>> last,
                         multi::array_iterator<T, 1, T*> dest ){
  static_assert(std::is_same<typename std::decay<Q1>::type,T>::value,"Wrong dispatch.\n");
  static_assert(std::is_same<typename std::decay<Q2>::type,T>::value,"Wrong dispatch.\n");
  assert( stride(first) == stride(last) );
  if(std::distance(first,last) == 0 ) return dest;
  if(cudaSuccess != cudaMemcpy2D(to_address(base(dest)),sizeof(T)*stride(dest),
                                 to_address(base(first)),sizeof(T)*stride(first),
                                 sizeof(T),std::distance(first,last),cudaMemcpyDeviceToHost))
      throw std::runtime_error("Error: cudaMemcpy2D returned error code.");
  return dest+std::distance(first,last);
}

template<class T, class Q1, class Q2, typename Size>
multi::array_iterator<T, 1, shm::shm_ptr_with_raw_ptr_dispatch<T>> copy_n( 
             multi::array_iterator<Q1, 1, shm::shm_ptr_with_raw_ptr_dispatch<Q2>> first,
             Size N,
             multi::array_iterator<T, 1, shm::shm_ptr_with_raw_ptr_dispatch<T>> dest ){
  static_assert(std::is_same<typename std::decay<Q1>::type,T>::value,"Wrong dispatch.\n");
  static_assert(std::is_same<typename std::decay<Q2>::type,T>::value,"Wrong dispatch.\n");
  if(N==0) return dest;  
  if(cudaSuccess != cudaMemcpy2D(to_address(base(dest)),sizeof(T)*stride(dest),
                                 to_address(base(first)),sizeof(T)*stride(first),
                                 sizeof(T),N,cudaMemcpyDeviceToDevice))
      throw std::runtime_error("Error: cudaMemcpy2D returned error code.");
  return dest+N;
}

template<class T, class ForwardIt, typename Size>
multi::array_iterator<T, 1, shm::shm_ptr_with_raw_ptr_dispatch<T>> copy_n(
                         ForwardIt first,
                         Size n,
                         multi::array_iterator<T, 1, shm::shm_ptr_with_raw_ptr_dispatch<T>> dest ){
  if(n == 0) return dest;
  if(cudaSuccess != cudaMemcpy2D(to_address(base(dest)),sizeof(T)*stride(dest),
                                 to_address(base(first)),sizeof(T)*stride(first),
                                 sizeof(T),n,cudaMemcpyDefault))
      throw std::runtime_error("Error: cudaMemcpy2D returned error code.");
  return dest+n;
}

template<class T, class Q1, class Q2, typename Size>
multi::array_iterator<T, 1, T*> copy_n(
                         multi::array_iterator<Q1, 1, shm::shm_ptr_with_raw_ptr_dispatch<Q2>> first,
                         Size N,
                         multi::array_iterator<T, 1, T*> dest ){
  static_assert(std::is_same<typename std::decay<Q1>::type,T>::value,"Wrong dispatch.\n");
  static_assert(std::is_same<typename std::decay<Q2>::type,T>::value,"Wrong dispatch.\n");
  if( N == 0 ) return dest;
  if(cudaSuccess != cudaMemcpy2D(to_address(base(dest)),sizeof(T)*stride(dest),
                                 to_address(base(first)),sizeof(T)*stride(first),
                                 sizeof(T),N,cudaMemcpyDeviceToHost))
      throw std::runtime_error("Error: cudaMemcpy2D returned error code.");
  return dest+N;
}

template<class Alloc, class T, class ForwardIt>
multi::array_iterator<T, 1, shm::shm_ptr_with_raw_ptr_dispatch<T>> uninitialized_copy( 
                         Alloc &a,
                         ForwardIt first,
                         ForwardIt last,
                         multi::array_iterator<T, 1, shm::shm_ptr_with_raw_ptr_dispatch<T>> dest ){
  assert( stride(first) == stride(last) );
  if(std::distance(first,last) == 0 ) return dest;
  if(cudaSuccess != cudaMemcpy2D(to_address(base(dest)),sizeof(T)*stride(dest),
                                 to_address(base(first)),sizeof(T)*stride(first),
                                 sizeof(T),std::distance(first,last),cudaMemcpyDefault))
      throw std::runtime_error("Error: cudaMemcpy2D returned error code.");
  return dest+std::distance(first,last); 
}

template<class Alloc, class T, class Q1, class Q2>
multi::array_iterator<T, 1, T*> uninitialized_copy(
                         Alloc &a,
                         multi::array_iterator<Q1, 1, shm::shm_ptr_with_raw_ptr_dispatch<Q2>> first,
                         multi::array_iterator<Q1, 1, shm::shm_ptr_with_raw_ptr_dispatch<Q2>> last,
                         multi::array_iterator<T, 1, T*> dest ){
  static_assert(std::is_same<typename std::decay<Q1>::type,T>::value,"Wrong dispatch.\n");
  static_assert(std::is_same<typename std::decay<Q2>::type,T>::value,"Wrong dispatch.\n");
  assert( stride(first) == stride(last) );
  if(std::distance(first,last) == 0 ) return dest;
  if(cudaSuccess != cudaMemcpy2D(to_address(base(dest)),sizeof(T)*stride(dest),
                                 to_address(base(first)),sizeof(T)*stride(first),
                                 sizeof(T),std::distance(first,last),cudaMemcpyDeviceToHost))
      throw std::runtime_error("Error: cudaMemcpy2D returned error code.");
  return dest+std::distance(first,last);
}

template<class Alloc, class T, class Q, typename Size>
multi::array_iterator<T, 1, shm::shm_ptr_with_raw_ptr_dispatch<T>> uninitialized_copy_n( 
                           Alloc &a,
                           multi::array_iterator<Q, 1, shm::shm_ptr_with_raw_ptr_dispatch<Q>> first,
                           Size N,
                           multi::array_iterator<T, 1, shm::shm_ptr_with_raw_ptr_dispatch<T>> dest ){
  static_assert(std::is_same<typename std::decay<Q>::type,T>::value,"Wrong dispatch.\n");
  if(N==0) return dest;
  if(cudaSuccess != cudaMemcpy2D(to_address(base(dest)),sizeof(T)*stride(dest),
                                 to_address(base(first)),sizeof(T)*stride(first),
                                 sizeof(T),N,cudaMemcpyDeviceToDevice))
      throw std::runtime_error("Error: cudaMemcpy2D returned error code.");
  return dest+N;
}

template<class Alloc, class T, class Size>
shm_ptr_with_raw_ptr_dispatch<T> uninitialized_default_construct_n(Alloc& a, shm_ptr_with_raw_ptr_dispatch<T> f, Size n){
        using AT = typename std::allocator_traits<Alloc>;
        if(n == 0) return f;
        f.wSP_->fence();
        if(group(*f.wSP_).root()) {
//            uninitialized_value_construct_n(to_address(f), n);
            T* current(to_address(f));
            try{
                for(; n > 0; ++current, --n) AT::construct(a, current, T());
                return current;
            }catch(...){throw;}
        }
        f.wSP_->fence();
        return f + n;
}

template<class Alloc, class T, class Size>
shm_ptr_with_raw_ptr_dispatch<T> uninitialized_value_construct_n(Alloc& a, shm_ptr_with_raw_ptr_dispatch<T> f, Size n){
        using AT = typename std::allocator_traits<Alloc>;
        if(n == 0) return f;
        f.wSP_->fence();
        if(group(*f.wSP_).root()) {
//            uninitialized_value_construct_n(to_address(f), n);
            T* current(to_address(f));
            try{
                for(; n > 0; ++current, --n) AT::construct(a, current, T());
                return current;
            }catch(...){throw;}
        }
        f.wSP_->fence();
        return f + n;
}

} // multi
} // boost

#endif 
 
#endif