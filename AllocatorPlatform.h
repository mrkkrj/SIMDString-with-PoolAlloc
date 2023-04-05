/**
  \file AllocatorPlatform.h

  --> from:
  \file G3D-base.lib/include/G3D-base/platform.h

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
  <--

  mrkkrj: extracted from original G3D code for use in PoolAllocator's impl.
*/

#pragma once
#define G3D_platform_h

/** \def G3D_WINDOWS Defined on Windows platforms*/
/** \def G3D_FREEBSD Defined on FreeBSD and OpenBSD */
/** \def G3D_LINUX Defined on Linux, FreeBSD and OpenBSD */
/** \def G3D_OSX Defined on MacOS */

#ifdef _MSC_VER
#   define G3D_WINDOWS
#elif defined(__APPLE__)
#   define G3D_OSX

    // Prevent OS X fp.h header from being included; it defines
    // pi as a constant, which creates a conflict with G3D
#   define __FP__
#elif  defined(__FreeBSD__) || defined(__OpenBSD__)
#define G3D_FREEBSD
#define G3D_LINUX
#elif defined(__linux__)
#define G3D_LINUX
#else
#error Unknown platform
#endif

/** \def G3D_DEBUG
    Defined if G3D is built in debug mode. */
#if !defined(G3D_DEBUG) && (defined(_DEBUG) || defined(G3D_DEBUGRELEASE))
#   define G3D_DEBUG
#endif

// OPEN TODO::: 
//  --> other platforms also need something here???

#ifdef G3D_WINDOWS
#    ifndef WIN32_LEAN_AND_MEAN
#       define WIN32_LEAN_AND_MEAN 1
#    endif

#   ifndef NOMINMAX
#       define NOMINMAX 1
#   endif
#   ifndef _WIN32_WINNT
#       define _WIN32_WINNT 0x0602
#   endif
#   include <windows.h>
#   undef WIN32_LEAN_AND_MEAN
#   undef NOMINMAX
#endif


//
// From: G3D-base.lib/include/G3D-base/Thread.h
//

#include <atomic>
#include <functional>
#include <thread>

#ifndef G3D_WINDOWS
#   include <unistd.h> // For usleep
#endif

namespace G3D {

    /**
       \brief A mutual exclusion lock that busy-waits when locking.

       On a machine with one (significant) thread per processor core,
       a Spinlock may be substantially faster than a mutex.
     */
    class Spinlock {
    private:
        std::atomic_flag m_flag;

    public:
        Spinlock() {
            m_flag.clear();
        }

        /** Busy waits until the lock is unlocked, then locks it
            exclusively.

            A single thread cannot re-enter
            Spinlock::lock() if already locked.
         */
        void lock() {
            while (m_flag.test_and_set(std::memory_order_acquire)) {
#           ifdef G3D_WINDOWS
                Sleep(0);
#           else
                usleep(0);
#           endif
            }
        }

        void unlock() {
            m_flag.clear(std::memory_order_release);
        }

    };

} // namespace


//
// From: G3D-base.lib/include/G3D-base/debug.h
//

#ifdef _MSC_VER
#include <crtdbg.h>
#endif

namespace G3D {

#ifdef _MSC_VER
    // Turn off 64-bit warnings
#   pragma warning(push)
#   pragma warning( disable : 4312)
#   pragma warning( disable : 4267)
#   pragma warning( disable : 4311)
#endif


/**
 Useful for debugging purposes.
 */
    inline bool isValidHeapPointer(const void* x) {
#ifdef _MSC_VER
        return
            (x != nullptr) &&
            (x != (void*)0xcccccccc) && (x != (void*)0xdeadbeef) && (x != (void*)0xfeeefeee) &&
            (x != (void*)0xcdcdcdcd) && (x != (void*)0xabababab) && (x != (void*)0xfdfdfdfd);
#else
        return x != nullptr;
#endif
    }

    /**
     Returns true if the pointer is likely to be
     a valid pointer (instead of an arbitrary number).
     Useful for debugging purposes.
     */
    inline bool isValidPointer(const void* x) {
#ifdef _MSC_VER
        return (x != nullptr) &&
            (x != (void*)0xcdcdcdcdcdcdcdcd) && (x != (void*)0xcccccccccccccccc) &&
            (x != (void*)0xcccccccc) && (x != (void*)0xdeadbeef) && (x != (void*)0xfeeefeee) &&
            (x != (void*)0xcdcdcdcd) && (x != (void*)0xabababab) && (x != (void*)0xfdfdfdfd);
#else
        return x != nullptr;
#endif
    }

#ifdef _MSC_VER
#   pragma warning(pop)
#endif

} // namespace
