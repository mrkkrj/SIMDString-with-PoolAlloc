/**
  \file PoolAllocator.cpp
  
  --> from:
  \file G3D-base.lib/source/System.cpp

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
  <--

  mrkkrj: extracted impl. of the G3D pool allocator from the original G3D::System class,
          renamed G3D::System to G3D::SystemAlloc
*/


#include "AllocatorPlatform.h"
#include "PoolAllocator.h"
#include "DebugHelpers.h"


// OPEN TODO:::
//#define TRUE true ???? Linux? --> in AllocatorPlatform.h (mrkkrj)


//
// From: G3D-base.lib/include/G3D-base/g3dmath.h
//

namespace G3D {

    /**
      * True if num is a power of two.
      */

    inline bool isPow2(int num) {
        return ((num & -num) == num);
    }

    //inline bool isPow2(uint64 x) {
    //    // See http://www.exploringbinary.com/ten-ways-to-check-if-an-integer-is-a-power-of-two-in-c/, method #9
    //    return ((x != 0) && !(x & (x - 1)));
    //}

    inline bool isPow2(uint32 x) {
        // See http://www.exploringbinary.com/ten-ways-to-check-if-an-integer-is-a-power-of-two-in-c/, method #9
        return ((x != 0) && !(x & (x - 1)));
    }

    // also there
#   define max std::max    // OPEN TODO::: in  AllocatorPlatform.h ???

} // namespace


//
// From: G3D-base.lib/source/System.cpp
//

// Uncomment the following line to turn off G3D::SystemAlloc memory
// allocation and use the operating SystemAlloc's malloc.
//#define NO_BUFFERPOOL

#include <cstdlib>

#ifdef G3D_WINDOWS

#   include <conio.h>
#   include <sys/timeb.h>

#elif defined(G3D_LINUX) 

#   include <stdlib.h>
#   include <stdio.h>
#   include <errno.h>
#   include <sys/types.h>
#   include <sys/select.h>
#   include <termios.h>
#   include <unistd.h>
#   include <sys/ioctl.h>
#   include <sys/time.h>
#   include <pthread.h>

// fix on Linux
#   include <string.h>
#   include <stdarg.h>

#elif defined(G3D_OSX)

    #include <stdlib.h>
    #include <stdio.h>
    #include <errno.h>
    #include <sys/types.h>
    #include <sys/sysctl.h>
    #include <sys/select.h>
    #include <sys/time.h>
    #include <termios.h>
    #include <unistd.h>
    #include <pthread.h>
    #include <mach-o/arch.h>

    #include <sstream>
    #include <CoreServices/CoreServices.h>
#endif

#ifdef G3D_X86
// SIMM include
#include <xmmintrin.h>
#endif


namespace G3D {
 
SystemAlloc& SystemAlloc::instance() {
    static SystemAlloc theSystemAlloc;
    return theSystemAlloc;
}

SystemAlloc::SystemAlloc() 
    : m_outOfMemoryCallback(nullptr) 
{
}


////////////////////////////////////////////////////////////////
#define ALIGNMENT_SIZE 16 // must be at least sizeof(size_t)

#define REALPTR_TO_USERPTR(x)   ((uint8*)(x) + ALIGNMENT_SIZE)
#define USERPTR_TO_REALPTR(x)   ((uint8*)(x) - ALIGNMENT_SIZE)
#define USERSIZE_TO_REALSIZE(x)       ((x) + ALIGNMENT_SIZE)
#define REALSIZE_FROM_USERPTR(u) (*(size_t*)USERPTR_TO_REALPTR(ptr) + ALIGNMENT_SIZE)
#define USERSIZE_FROM_USERPTR(u) (*(size_t*)USERPTR_TO_REALPTR(ptr))

class BufferPool {
public:

    /** Only store buffers up to these sizes (in bytes) in each pool->
        Different pools have different management strategies.

        A large block is preallocated for tiny buffers; they are used with
        tremendous frequency.  Other buffers are allocated as demanded.
        Tiny buffers are 128 bytes long because that seems to align well with
        cache sizes on many machines.
      */
    enum {tinyBufferSize = 256, smallBufferSize = 2048, medBufferSize = 8192};

    /** 
       Most buffers we're allowed to store.
       250000 * { 128 |  256} = {32 | 64} MB (preallocated)
        40000 * {1024 | 2048} = {40 | 80} MB (allocated on demand)
         5000 * {4096 | 8192} = {20 | 40} MB (allocated on demand)
     */
    enum {maxTinyBuffers = 250000, maxSmallBuffers = 40000, maxMedBuffers = 5000};

private:

    /** Pointer given to the program.  Unless in the tiny heap, the user size of the block is stored right in front of the pointer as a uint32.*/
    typedef void* UserPtr;

    /** Actual block allocated on the heap */
    typedef void* RealPtr;

    class MemBlock {
    public:
        UserPtr     ptr;
        size_t      bytes;

        inline MemBlock() : ptr(nullptr), bytes(0) {}
        inline MemBlock(UserPtr p, size_t b) : ptr(p), bytes(b) {}
    };

    MemBlock smallPool[maxSmallBuffers];
    int smallPoolSize;

    MemBlock medPool[maxMedBuffers];
    int medPoolSize;

    /** The tiny pool is a single block of storage into which all tiny
        objects are allocated.  This provides better locality for
        small objects and avoids the search time, since all tiny
        blocks are exactly the same size. */
    void* tinyPool[maxTinyBuffers];
    int tinyPoolSize;

    /** Pointer to the data in the tiny pool */
    void* tinyHeap;

    Spinlock            m_lock;

    inline void lock() {
        m_lock.lock();
    }

    inline void unlock() {
        m_lock.unlock();
    }

    /** 
     Malloc out of the tiny heap. Returns nullptr if allocation failed.
     */
    inline UserPtr tinyMalloc(size_t bytes) {
        // Note that we ignore the actual byte size
        // and create a constant size block.
        (void)bytes;
        assert(tinyBufferSize >= bytes);

        UserPtr ptr = nullptr;

        if (tinyPoolSize > 0) {
            --tinyPoolSize;

            // Return the old last pointer from the freelist
            ptr = tinyPool[tinyPoolSize];

#           ifdef G3D_DEBUG
                if (tinyPoolSize > 0) {
                    assert(tinyPool[tinyPoolSize - 1] != ptr); 
                     //   "SystemAlloc::malloc heap corruption detected: "
                     //   "the last two pointers on the freelist are identical (during tinyMalloc).");
                }
#           endif

            // nullptr out the entry to help detect corruption
            tinyPool[tinyPoolSize] = nullptr;
        }

        return ptr;
    }

    /** Returns true if this is a pointer into the tiny heap. */
    bool inTinyHeap(UserPtr ptr) {
        return 
            (ptr >= tinyHeap) && 
            (ptr < (uint8*)tinyHeap + maxTinyBuffers * tinyBufferSize);
    }

    void tinyFree(UserPtr ptr) {
        assert(ptr);
        assert(tinyPoolSize < maxTinyBuffers);
 //           "Tried to free a tiny pool buffer when the tiny pool freelist is full.");

#       ifdef G3D_DEBUG
            if (tinyPoolSize > 0) {
                UserPtr prevOnHeap = tinyPool[tinyPoolSize - 1];
                assert(prevOnHeap != ptr); 
//                    "SystemAlloc::malloc heap corruption detected: "
//                    "the last two pointers on the freelist are identical (during tinyFree).");
            }
#       endif

        assert(tinyPool[tinyPoolSize] == nullptr);

        // Put the pointer back into the free list
        tinyPool[tinyPoolSize] = ptr;
        ++tinyPoolSize;

    }

    void flushPool(MemBlock* pool, int& poolSize) {
        for (int i = 0; i < poolSize; ++i) {
            bytesAllocated -= USERSIZE_TO_REALSIZE(pool[i].bytes);
            ::free(USERPTR_TO_REALPTR(pool[i].ptr));
            pool[i].ptr = nullptr;
            pool[i].bytes = 0;
        }
        poolSize = 0;
    }


    /** Allocate out of a specific pool.  Return nullptr if no suitable 
        memory was found. */
    UserPtr poolMalloc(MemBlock* pool, int& poolSize, const int maxPoolSize, size_t bytes) {

        // OPT: find the smallest block that satisfies the request.

        // See if there's something we can use in the buffer pool.
        // Search backwards since usually we'll re-use the last one.
        for (int i = (int)poolSize - 1; i >= 0; --i) {
            if (pool[i].bytes >= bytes) {
                // We found a suitable entry in the pool.

                // No need to offset the pointer; it is already offset
                UserPtr ptr = pool[i].ptr;

                // Remove this element from the pool, replacing it with
                // the one from the end (same as Array::fastRemove)
                --poolSize;
                pool[i] = pool[poolSize];

                return ptr;
            }
        }

        if (poolSize == maxPoolSize) {
            // Free even-indexed pools, and compact array in the same loop
            for (int i = 0; i < poolSize; i += 2) {
                bytesAllocated -= USERSIZE_TO_REALSIZE(pool[i].bytes);
                ::free(USERPTR_TO_REALPTR(pool[i].ptr));
                pool[i].ptr = nullptr;
                pool[i].bytes = 0;
                // Compact: (i/2) is the next open slot
                pool[i/2].ptr   = pool[i+1].ptr;
                pool[i/2].bytes = pool[i+1].bytes;
                pool[i+1].ptr = nullptr;
                pool[i+1].bytes = 0;
            }
            poolSize = poolSize/2;
            if (maxPoolSize == maxMedBuffers) {
                ++medPoolPurgeCount;
            } else if (maxPoolSize == maxSmallBuffers) {
                ++smallPoolPurgeCount;
            }

        }

        return nullptr;
    }

public:

    /** Count of memory allocations that have occurred. */
    int totalMallocs;
    int mallocsFromTinyPool;
    int mallocsFromSmallPool;
    int mallocsFromMedPool;

    int smallPoolPurgeCount;
    int medPoolPurgeCount;

    /** Amount of memory currently allocated (according to the application). 
        This does not count the memory still remaining in the buffer pool,
        but does count extra memory required for rounding off to the size
        of a buffer.
        Primarily useful for detecting leaks.*/
    std::atomic_size_t bytesAllocated;

    BufferPool() {
        totalMallocs         = 0;

        mallocsFromTinyPool  = 0;
        mallocsFromSmallPool = 0;
        mallocsFromMedPool   = 0;

        bytesAllocated       = 0;

        tinyPoolSize         = 0;
        tinyHeap             = nullptr;

        smallPoolSize        = 0;

        medPoolSize          = 0;

        smallPoolPurgeCount = 0;
        medPoolPurgeCount   = 0;


        // Initialize the tiny heap as a bunch of pointers into one
        // pre-allocated buffer.
        tinyHeap = ::malloc(maxTinyBuffers * tinyBufferSize);
        for (int i = 0; i < maxTinyBuffers; ++i) {
            tinyPool[i] = (uint8*)tinyHeap + (tinyBufferSize * i);
        }
        tinyPoolSize = maxTinyBuffers;
    }


    ~BufferPool() {
        ::free(tinyHeap);
        flushPool(smallPool, smallPoolSize);
        flushPool(medPool, medPoolSize);
    }

    
    UserPtr realloc(UserPtr ptr, size_t bytes) {
        if (ptr == nullptr) {
            return malloc(bytes);
        }

        if (inTinyHeap(ptr)) {
            if (bytes <= tinyBufferSize) {
                // The old pointer actually had enough space.
                return ptr;
            } else {
                // Free the old pointer and malloc
                
                UserPtr newPtr = malloc(bytes);
                SystemAlloc::memcpy(newPtr, ptr, tinyBufferSize);
                lock();
                tinyFree(ptr);
                unlock();
                return newPtr;

            }
        } else {
            // In one of our heaps.

            // See how big the block really was
            size_t userSize = USERSIZE_FROM_USERPTR(ptr);
            if (bytes <= userSize) {
                // The old block was big enough.
                return ptr;
            }

            // Need to reallocate and move
            UserPtr newPtr = malloc(bytes);
            SystemAlloc::memcpy(newPtr, ptr, userSize);
            free(ptr);
            return newPtr;
        }
    }


    UserPtr malloc(size_t bytes) {
        lock();
        ++totalMallocs;

        if (bytes <= tinyBufferSize) {

            UserPtr ptr = tinyMalloc(bytes);

            if (ptr) {
                debugAssertM((intptr_t)ptr % 16 == 0, "BufferPool::tinyMalloc returned non-16 byte aligned memory");
                ++mallocsFromTinyPool;
                unlock();
                return ptr;
            }

        } 
        
        // Failure to allocate a tiny buffer is allowed to flow
        // through to a small buffer
        if (bytes <= smallBufferSize) {
            
            UserPtr ptr = poolMalloc(smallPool, smallPoolSize, maxSmallBuffers, bytes);

            if (ptr) {
                debugAssertM((intptr_t)ptr % 16 == 0, "BufferPool::poolMalloc(small) returned non-16 byte aligned memory");
                ++mallocsFromSmallPool;
                unlock();
                return ptr;
            }

        } else if (bytes <= medBufferSize) {
            // Note that a small allocation failure does *not* fall
            // through into a medium allocation because that would
            // waste the medium buffer's resources.

            UserPtr ptr = poolMalloc(medPool, medPoolSize, maxMedBuffers, bytes);

            if (ptr) {
                debugAssertM((intptr_t)ptr % 16 == 0, "BufferPool::poolMalloc(med) returned non-16 byte aligned memory");
                ++mallocsFromMedPool;
                unlock();
                debugAssertM(ptr != nullptr, "BufferPool::malloc returned nullptr");
                return ptr;
            }
        }

        bytesAllocated.fetch_add(USERSIZE_TO_REALSIZE(bytes));
        unlock();

        // Heap allocate

        // Allocate 4 extra bytes for our size header (unfortunate,
        // since malloc already added its own header).
        RealPtr ptr = ::malloc(USERSIZE_TO_REALSIZE(bytes));
        if (ptr == nullptr) {
#           ifdef G3D_WINDOWS
                // Check for memory corruption
                alwaysAssertM(_CrtCheckMemory() == TRUE, "Heap corruption detected.");
#           endif

            // Flush memory pools to try and recover space
            flushPool(smallPool, smallPoolSize);
            flushPool(medPool, medPoolSize);
            ptr = ::malloc(USERSIZE_TO_REALSIZE(bytes));
        }

        if (ptr == nullptr) {
            if ((SystemAlloc::outOfMemoryCallback() != nullptr) &&
                (SystemAlloc::outOfMemoryCallback()(USERSIZE_TO_REALSIZE(bytes), true) == true)) {
                // Re-attempt the malloc
                ptr = ::malloc(USERSIZE_TO_REALSIZE(bytes));
                
            }
        }

        if (ptr == nullptr) {
            if (SystemAlloc::outOfMemoryCallback() != nullptr) {
                // Notify the application
                SystemAlloc::outOfMemoryCallback()(USERSIZE_TO_REALSIZE(bytes), false);
            }
#           ifdef G3D_DEBUG
            debugPrintf("::malloc(%d) returned nullptr\n", (int)USERSIZE_TO_REALSIZE(bytes));
#           endif
            debugAssertM(ptr != nullptr, 
                         "::malloc returned nullptr. Either the "
                         "operating SystemAlloc is out of memory or the "
                         "heap is corrupt.");
            return nullptr;
        }

        ((size_t*)ptr)[0] = bytes;
        debugAssertM((intptr_t)REALPTR_TO_USERPTR(ptr) % 16 == 0, "::malloc returned non-16 byte aligned memory");
        return REALPTR_TO_USERPTR(ptr);
    }


    void free(UserPtr ptr) {
        if (ptr == nullptr) {
            // Free does nothing on null pointers
            return;
        }

        assert(isValidPointer(ptr));

        if (inTinyHeap(ptr)) {
            lock();
            tinyFree(ptr);
            unlock();
            return;
        }

        size_t bytes = USERSIZE_FROM_USERPTR(ptr);

        lock();
        if (bytes <= smallBufferSize) {
            if (smallPoolSize < maxSmallBuffers) {
                smallPool[smallPoolSize] = MemBlock(ptr, bytes);
                ++smallPoolSize;
                unlock();
                return;
            }
        } else if (bytes <= medBufferSize) {
            if (medPoolSize < maxMedBuffers) {
                medPool[medPoolSize] = MemBlock(ptr, bytes);
                ++medPoolSize;
                unlock();
                return;
            }
        }
        bytesAllocated.fetch_sub(USERSIZE_TO_REALSIZE(bytes));
        unlock();

        // Free; the buffer pools are full or this is too big to store.
        ::free(USERPTR_TO_REALPTR(ptr));
    }

    String mallocRatioString() const {
        if (totalMallocs > 0) {
            int pooled = mallocsFromTinyPool +
                         mallocsFromSmallPool + 
                         mallocsFromMedPool;

            int total = totalMallocs;

            return format("Percent of Mallocs: %5.1f%% <= %db, %5.1f%% <= %db, "
                          "%5.1f%% <= %db, %5.1f%% > %db",
                          100.0 * mallocsFromTinyPool  / total,
                          BufferPool::tinyBufferSize,
                          100.0 * mallocsFromSmallPool / total,
                          BufferPool::smallBufferSize,
                          100.0 * mallocsFromMedPool   / total,
                          BufferPool::medBufferSize,
                          100.0 * (1.0 - (double)pooled / total),
                          BufferPool::medBufferSize);
        } else {
            return "No SystemAlloc::malloc calls made yet.";
        }
    }

    String status() const {
        String tinyPoolString = format("Tiny Pool: %5.1f%% of %d x %db Free", 100.0 * tinyPoolSize / maxTinyBuffers, 
                                       maxTinyBuffers, tinyBufferSize);
        String poolSizeString = format("Pool Sizes: %5d/%d x %db, %5d/%d x %db, %5d/%d x %db",
                                       tinyPoolSize,     maxTinyBuffers,     tinyBufferSize, 
                                       smallPoolSize,    maxSmallBuffers,    smallBufferSize,
                                       medPoolSize,      maxMedBuffers,      medBufferSize);

        int pooled = mallocsFromTinyPool +
            mallocsFromSmallPool + 
            mallocsFromMedPool;
        int outOfPoolsMallocs = totalMallocs - pooled;
        String outOfBufferMemoryString = format("Total out of pools mallocs: %d; Bytes allocated: %d", outOfPoolsMallocs, int(bytesAllocated));
        String purgeString = format("Small Pool Purges: %d; Med Pool Purges: %d", smallPoolPurgeCount, medPoolPurgeCount);
        return mallocRatioString() + "\n" + poolSizeString + "\n" + outOfBufferMemoryString + "\n" + purgeString;

    }
};

// Dynamically allocated because we need to ensure that
// the buffer pool is still around when the last global variable 
// is deallocated.
static BufferPool* bufferpool = nullptr;

String SystemAlloc::mallocStatus() {    
#ifndef NO_BUFFERPOOL
    return bufferpool->status();
#else
    return "NO_BUFFERPOOL";
#endif
}


void SystemAlloc::resetMallocPerformanceCounters() {
#ifndef NO_BUFFERPOOL
    bufferpool->totalMallocs         = 0;
    bufferpool->mallocsFromMedPool   = 0;
    bufferpool->mallocsFromSmallPool = 0;
    bufferpool->mallocsFromTinyPool  = 0;
#endif
}


#ifndef NO_BUFFERPOOL
inline void initMem() {
    // Putting the test here ensures that the SystemAlloc is always
    // initialized, even when globals are being allocated.
    static bool initialized = false;
    if (! initialized) {
        bufferpool = new BufferPool();
        initialized = true;
    }
}
#endif


void* SystemAlloc::malloc(size_t bytes) {
#ifndef NO_BUFFERPOOL
    initMem();
    return bufferpool->malloc(bytes);
#else
    return ::malloc(bytes);
#endif
}

void* SystemAlloc::calloc(size_t n, size_t x) {
#ifndef NO_BUFFERPOOL
    void* b = SystemAlloc::malloc(n * x);
    debugAssertM(b != nullptr, "SystemAlloc::malloc returned nullptr");
    debugAssertM(isValidHeapPointer(b), "SystemAlloc::malloc returned an invalid pointer");
    SystemAlloc::memset(b, 0, n * x);
    return b;
#else
    return ::calloc(n, x);
#endif
}


void* SystemAlloc::realloc(void* block, size_t bytes) {
#ifndef NO_BUFFERPOOL
    initMem();
    return bufferpool->realloc(block, bytes);
#else
    return ::realloc(block, bytes);
#endif
}


void SystemAlloc::free(void* p) {
#ifndef NO_BUFFERPOOL
    bufferpool->free(p);
#else
    return ::free(p);
#endif
}


void* SystemAlloc::alignedMalloc(size_t bytes, size_t alignment) {

    alwaysAssertM(isPow2((uint32)alignment), "alignment must be a power of 2");

    // We must align to at least a word boundary.
    alignment = max(alignment, sizeof(void *));

    // Pad the allocation size with the alignment size and the size of
    // the redirect pointer.  This is the worst-case size we'll need.
    // Since the alignment size is at least teh word size, we don't
    // need to allocate space for the redirect pointer.  We repeat the max here
    // for clarity.
    size_t totalBytes = bytes + max(alignment, sizeof(void*));

    size_t truePtr = (size_t)SystemAlloc::malloc(totalBytes);

    if (truePtr == 0) {
        // malloc returned nullptr
        return nullptr;
    }

    debugAssert(isValidHeapPointer((void*)truePtr));
    #ifdef G3D_WINDOWS
    // The blocks we return will not be valid Win32 debug heap
    // pointers because they are offset 
    //  debugAssert(_CrtIsValidPointer((void*)truePtr, totalBytes, TRUE) );
    #endif


    // We want alignedPtr % alignment == 0, which we'll compute with a
    // binary AND because 2^n - 1 has the form 1111... in binary.
    const size_t bitMask = (alignment - 1);

    // The return pointer will be the next aligned location that is at
    // least sizeof(void*) after the true pointer. We need the padding
    // to have a place to write the redirect pointer.
    size_t alignedPtr = truePtr + sizeof(void*);

    const size_t remainder = alignedPtr & bitMask;
    
    // Add what we need to make it to the next alignment boundary, but
    // if the remainder was zero, let it wrap to zero and don't add
    // anything.
    alignedPtr += ((alignment - remainder) & bitMask);

    debugAssert((alignedPtr & bitMask) == 0);
    debugAssert((alignedPtr - truePtr + bytes) <= totalBytes);

    // Immediately before the aligned location, write the true array location
    // so that we can free it correctly.
    size_t* redirectPtr = (size_t *)(alignedPtr - sizeof(void *));
    redirectPtr[0] = truePtr;

    debugAssert(isValidHeapPointer((void*)truePtr));

    #if defined(G3D_WINDOWS) && defined(G3D_DEBUG)
        if (bytes < 0xFFFFFFFF) { 
            debugAssert( _CrtIsValidPointer((void*)alignedPtr, (int)bytes, TRUE) );
        }
    #endif
    return (void *)alignedPtr;
}


void SystemAlloc::alignedFree(void* _ptr) {
    if (_ptr == nullptr) {
        return;
    }

    size_t alignedPtr = (size_t)_ptr;

    // Back up one word from the pointer the user passed in.
    // We now have a pointer to a pointer to the true start
    // of the memory block.
    size_t* redirectPtr = (size_t*)(alignedPtr - sizeof(void *));

    // Dereference that pointer so that ptr = true start
    void* truePtr = (void*)redirectPtr[0];

    debugAssert(isValidHeapPointer((void*)truePtr));
    SystemAlloc::free(truePtr);
}


void SystemAlloc::memcpy(void* dst, const void* src, size_t numBytes) {
    ::memcpy(dst, src, numBytes);
}

void SystemAlloc::memset(void* dst, uint8 value, size_t numBytes) {
    ::memset(dst, value, numBytes);
}


//
// impl. of DebugHelpers.h functions
//

// debug support
//  - From: G3D-base.lib/source/format.cpp

String format(const char* fmt, ...) {
   va_list argList;
   va_start(argList, fmt);
   String result = vformat(fmt, argList);
   va_end(argList);

   return result;
}

#if defined(_MSC_VER) && (_MSC_VER >= 1300) && (_MSC_VER < 1900)
String vformat(const char* fmt, va_list argPtr) {
   // mrkkrj: we don't support older MVSC versions right now! 
   //  --> OPEN TODO::: needed?????
   return "Old MSVC compiler, not yet implemented!!!";
}
#elif defined(_MSC_VER) && (_MSC_VER < 1300)
String vformat(const char* fmt, va_list argPtr) {
   // mrkkrj: we don't support old MVSC versions right now! 
   //  --> OPEN TODO::: needed?????
   return "Old MSVC compiler, not yet implemented!!!";
}
#else
String vformat(const char* fmt, va_list argPtr) {
   // If the string is less than 161 characters,
   // allocate it on the stack because this saves
   // the malloc/free time.  The number 161 is chosen
   // to support two lines of text on an 80 character
   // console (plus the null terminator)
   const int bufSize = 161;
   char stackBuffer[bufSize];

   va_list argPtrCopy;
   va_copy(argPtrCopy, argPtr); 
   int numChars = vsnprintf(stackBuffer, bufSize, fmt, argPtrCopy);
   va_end(argPtrCopy);

   if (numChars >= bufSize) {
      // We didn't allocate a big enough string.
      char* heapBuffer = (char*)::malloc((numChars + 1) * sizeof(char));

      debugAssert(heapBuffer);
      int numChars2 = vsnprintf(heapBuffer, numChars + 1, fmt, argPtr);
      debugAssert(numChars2 == numChars);
      (void)numChars2;

      String result(heapBuffer);

      ::free(heapBuffer);

      return result;
   }
   else {
      return String(stackBuffer);
   }
}
#endif


// debug support
//  - From: G3D-base.lib/source/debugAssert.cpp

static String debugPrint(const String& s) {
#   ifdef G3D_WINDOWS
   const int MAX_STRING_LEN = 1024;

   // Windows can't handle really long strings sent to
   // the console, so we break the string.
   if (s.size() < MAX_STRING_LEN) {
      OutputDebugStringA(s.c_str());
   }
   else {
      for (unsigned int i = 0; i < s.size(); i += MAX_STRING_LEN) {
         const String& sub = s.substr(i, MAX_STRING_LEN);
         OutputDebugStringA(sub.c_str());
      }
   }
#    else
   fprintf(stderr, "%s", s.c_str());
   fflush(stderr);
#    endif

   return s;
}

String debugPrintf(const char* fmt ...) {
   va_list argList;
   va_start(argList, fmt);
   String s = G3D::vformat(fmt, argList);
   va_end(argList);

   return debugPrint(s);
}

} // namespace
