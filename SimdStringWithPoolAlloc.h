
/**
   \brief Shortcut deifinition for SIMDString using the locally defined PoolAllocator
   
   SIMDString:: 
   ------>  
   Very fast string class that follows the std::string/std::basic_string interface.

   - Recognizes constant segment strings and avoids copying them
   - Stores small strings internally to avoid heap allocation
   - Uses SSE instructions to copy internal strings
   - Uses the G3D free-list/block (or other !!!) allocator when heap allocation is required

   INTERNAL_SIZE is in bytes. It should be chosen to be a multiple of 16.
   <------
*/

#define NO_G3D_ALLOCATOR 1 // do not pull the whole G3D in!!!
#include <SIMDString.h>    // the SIMD optimized string class
#include <PoolAllocator.h> // the allocator to be used

using SIMDStringWithPoolAlloc = SIMDString<64, G3D::g3d_pool_allocator<char>>;
