
PoolAllocator
=========================================================================================================

This class contains the extracted G3D memory allocator which can be used with `SIMDString` class. The code doesn't 
have any additional dependencies, differently than the `g3d_allocator`, which pulls in practically the **entire** 
*G3D-base* library's code (sic!!!).

The extracted allocator seems to be implemeneted as a pooled block allocator.

## Usage:

    #define NO_G3D_ALLOCATOR 1 
    #include <SIMDString.h>
    
    #include <PoolAllocator.h>

    using SIMDString = SIMDString<64, G3D::g3d_pool_allocator<char>>;

    SIMDString strg("0123456789abcdefghijklmnopqrstuvwxyz");

This code can be seen in action in the *SimdStringTest.cpp* source file

## TODO:
 - experiment with replacing the allocator by C++17's `std::pool_memory_resource`
 - package the allocator as a *memory_resource*
 - compare performance of `SIMDString` with the G3D allocator vs. `std::pool_memory_resource`
 - switch to using SIMDString as an external github module 

## OPEN:
  - `SIMDString` (which is also extracted from G3D codebase) has a [MIT-license](https://opensource.org/licenses/MIT), but G3D allocator's files have 
    [BSD license](https://opensource.org/licenses/BSD). Anybody knows what licence the `SIMDString` + `PoolAllocator` combo would have???
