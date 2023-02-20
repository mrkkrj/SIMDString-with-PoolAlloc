/**
  \file PoolAllocator.h
  
  --> from:
  \file G3D-base.lib/include/G3D-base/System.h

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
  <--

  mrkkrj: extracted impl. of the G3D pool allocator from the original G3D::System class,
          renamed G3D::System to G3D::SystemAlloc
*/

#ifndef G3D_PoolAllocator_h
#define G3D_PoolAllocator_h


// OPEN TODO::: ???
#include <string>
#define String std::string

// OPEN TODO::: ???
#define uint8 std::uint8_t
#define uint32 std::uint32_t


namespace G3D {

class SystemAlloc {
public:
    /**
       @param size Size of memory that the SystemAlloc was trying to allocate

       @param recoverable If true, the SystemAlloc will attempt to allocate again
       if the callback returns true.  If false, malloc is going to return 
       nullptr and this invocation is just to notify the application.

       @return Return true to force malloc to attempt allocation again if the
       error was recoverable.
     */
    typedef bool (*OutOfMemoryCallback)(size_t size, bool recoverable);

private:

    OutOfMemoryCallback m_outOfMemoryCallback;

    /** @brief Used for the singleton instance only. */
    SystemAlloc();

    /** @brief The singleton instance. 

        Used instead of a global variable to ensure that the order of
        intialization is correct, which is critical because other
        globals may allocate memory using SystemAlloc::malloc.
     */
    static SystemAlloc& instance();

public:  

    /**
       Uses pooled storage to optimize small allocations (1 byte to 5
       kilobytes).  Can be 10x to 100x faster than calling \c malloc or
       \c new.
       
       The result must be freed with free.
       
       Threadsafe on Win32.
       
       @sa calloc realloc OutOfMemoryCallback free
    */
    static void* malloc(size_t bytes);
    
    static void* calloc(size_t n, size_t x);

    /**
     Version of realloc that works with SystemAlloc::malloc.
     */
    static void* realloc(void* block, size_t bytes);

    /**
     Free data allocated with SystemAlloc::malloc.

     Threadsafe on Win32.
     */
    static void free(void* p);

    /**
       Guarantees that the start of the array is aligned to the 
       specified number of bytes.
    */
    static void* alignedMalloc(size_t bytes, size_t alignment);
    
    /**
     Frees memory allocated with alignedMalloc.
     */
    static void alignedFree(void* ptr);
   
    /** An implementation of memcpy that may be up to 2x as fast as the C library
        one on some processors.  Guaranteed to have the same behavior as memcpy
        in all cases. */
    static void memcpy(void* dst, const void* src, size_t numBytes);

    /** An implementation of memset that may be up to 2x as fast as the C library
        one on some processors.  Guaranteed to have the same behavior as memset
        in all cases. */
    static void memset(void* dst, uint8 value, size_t numBytes);

    /**
     When SystemAlloc::malloc fails to allocate memory because the SystemAlloc is
     out of memory, it invokes this handler (if it is not nullptr).
     The argument to the callback is the amount of memory that malloc
     was trying to allocate when it ran out.  If the callback returns
     true, SystemAlloc::malloc will attempt to allocate the memory again.
     If the callback returns false, then SystemAlloc::malloc will return nullptr.

     You can use outOfMemoryCallback to free data structures or to 
     register the failure.
     */
    inline static OutOfMemoryCallback outOfMemoryCallback() {
        return instance().m_outOfMemoryCallback;
    }

    inline static void setOutOfMemoryCallback(OutOfMemoryCallback c) {
        instance().m_outOfMemoryCallback = c;
    }

    /**
       Returns a string describing the current usage of the buffer pools used for
       optimizing SystemAlloc::malloc, and describing how well SystemAlloc::malloc is using
        its internal pooled storage.  "heap" memory was slow to
        allocate; the other data sizes are comparatively fast.
     */
    static String mallocStatus();

    static void resetMallocPerformanceCounters();
};


/** 
 \brief Implementation of a C++ Allocator (for example std::allocator) that uses G3D::SystemAlloc::malloc and G3D::SystemAlloc::free. 

 All instances of g3d_pool_allocator are stateless.

 mrkkrj: renamed g3d_allocator to g3d_pool_allocator as to enable parallel usage

 \sa G3D::MemoryManager, G3D::SystemAlloc::malloc, G3D::SystemAlloc::free
*/
template<class T>
class g3d_pool_allocator {
public:
    /** Allocates n * sizeof(T) bytes of uninitialized storage by calling G3D::SystemAlloc::malloc() */
    [[nodiscard]] constexpr T* allocate(std::size_t n) {
        return static_cast<T*>(SystemAlloc::malloc(sizeof(T) * n));
    }

    /** Deallocates the storage referenced by the pointer p, which must be a pointer obtained by an earlier call to allocate() */
    constexpr void deallocate(T* p, std::size_t n) {
        SystemAlloc::free(p);
    }
};

} // namespace G3D

// https://en.cppreference.com/w/cpp/memory/allocator/operator_cmp
template< class T1, class T2 >
constexpr bool operator==( const G3D::g3d_pool_allocator<T1>& lhs, const G3D::g3d_pool_allocator<T2>& rhs ) noexcept {
    return true;
}

#endif
