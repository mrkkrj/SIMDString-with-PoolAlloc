
PoolAllocator
=========================================================================================================

This class contains the free-list/block memory allocator extracted from G3D project, directly useable with the `SIMDString` class. 

The resulted code doesn't have any additional dependencies, differently than the original `g3d_allocator`, which pulls in practically the **entire** *G3D-base* library's code (sic!!!). In this way we can use the highly optimized `SIMDString` with custom local allocator, which wasn't possible before!

**Note:** the extracted allocator is placed in the */src* subdirectory.<br>
**Note:** the extracted allocator is implemeneted as a *pooled block allocator* (i.e. a small block free list allocator).

## Usage:

    #define NO_G3D_ALLOCATOR 1 
    #include <SIMDString.h>
    
    #include <PoolAllocator.h>

    using SIMDString = SIMDString<64, G3D::g3d_pool_allocator<char>>;

    SIMDString strg("0123456789abcdefghijklmnopqrstuvwxyz");

This code can be seen in action in the *SimdStringTest.cpp* source file. 

Alternatively, we can also use a "shortcut" header *SIMDStringWithPoolAlloc.h* as shown below:
    
    #include <SIMDStringWithPoolAlloc.h>
    
    SIMDStringWithPoolAlloc strg("0123456789abcdefghijklmnopqrstuvwxyz");

The extracted *PoolAllocator* has been also wrapped in a *pmr::memory_resource*, so it can be also used independently from the *SIMDString* class as shown in the example code below:

    #include <memory_resource>
    #include <g3d_buffer_pool_resource.h>

    g3d_buffer_pool_resource memRes;

    std::pmr::string pmrstring2("0123456789abcdefghijklmnopqrstuvwxyz", &memRes);


## TODO:
 - support for older VisualStudio compilers dropped in 'mallocStatus()' as for now -> add it?
 - add CMake support, test on Linux
 - package the allocator as a *std::memory_resource* (??)
 - compare performance of `SIMDString` with the G3D allocator vs. `std::pool_memory_resource`
 - switch to using SIMDString as an external github module (???)

## OPEN:
  - `SIMDString` (which is also extracted from G3D codebase) has a [MIT-license](https://opensource.org/licenses/MIT), but G3D allocator's files have 
    [BSD license](https://opensource.org/licenses/BSD). Does anybody know what licence the `SIMDString` + `PoolAllocator` combo would have ???


## Note: what are SIMDString optimizations: 

- from a 2022 CppCon presentation:

"From what we’ve seen so far, here are the properties of string usage in graphics. These tell us the requirements our string class should have. Of these, the most important to note are that many strings are small or empty, and many reside in the const segment of the compiled executable.

So, from these **we need optimized:** copy construction, concatenation, conversion from *'const char*'*

And, of course, we can’t have a class with just these operations --- we need the behavior of the full *std::string* and we need it to be as performant as possible. It needs to fit the standard string ecosystem. Many game engines use standard string. We want a drop-in replacement for standard string that implements the full *'std::string'* API and is performant for large strings and optimized for shorter strings."

