/**
   \brief Some tests (by mrkkrj) for SIMDString using the local PoolAlloc
   
   SIMDString:: 
   ------>  
   Very fast string class that follows the std::string/std::basic_string interface.

   - Recognizes constant segment strings and avoids copying them
   - Stores small strings internally to avoid heap allocation
   - Uses SSE instructions to copy internal strings
   - Uses the G3D free-list/block allocator when heap allocation is required

   INTERNAL_SIZE is in bytes. It should be chosen to be a multiple of 16.
   <------
*/

#define NO_G3D_ALLOCATOR 1 // do not pull the whole G3D in!!!
#include <SIMDString.h>

#include <PoolAllocator.h> // use the extracted allocator instead

#include <string>
#include <iostream>

#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L) // C++17
#ifndef ExcludeG3dBufferPoolResource
    #include <memory_resource>
    #include <g3d_buffer_pool_resource.h>
#endif
#endif

int main()
{
    // inject the extracted pool allocator
    using SIMDString = SIMDString<64, G3D::g3d_pool_allocator<char>>;

    // 1. basic usage: small string case
    char sampleString[44] = "the quick brown fox jumps over the lazy dog";

    SIMDString simdstring0;
    SIMDString simdstring1('a');
    SIMDString simdstring2("0123456789abcdefghijklmnopqrstuvwxyz");
    SIMDString simdstring3(simdstring2, 10);
    SIMDString simdstring4(simdstring2, 10, 10);
    SIMDString simdstring5(simdstring2.begin(), simdstring2.begin() + 10);
    SIMDString simdstring6{ SIMDString(sampleString) };
    SIMDString simdstring7({ 'a', 'b', 'c' });

    std::string string1(sampleString);
    SIMDString simdstring8(string1, 40);
    SIMDString simdstring9(string1, 4, 5);
    SIMDString simdstring10(10, 'b');
    SIMDString simdstring11(simdstring10);

    // 2. long string case - invokes pool_alloc:
    //  --> INTERNAL_SIZE = 64
    SIMDString simdstringXXL("0123456789abcdefghijklmnopqrstuvwxyz hjhjkhkhjkhjkhjkhjkhjkhjkhjkhjkkhjkjhkhjhjkhjhjkjkhhjkhjkhjkhjkkhjhjkhjk");
    simdstringXXL.append("xxxx");

    // 3. access allocator's statistics:
    auto status = G3D::SystemAlloc::mallocStatus();
    std::cout << "SystemAlloc's status:\n" << status << "\n";

#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L) // C++17
#ifndef ExcludeG3dBufferPoolResource
    // 4. use std::pool_memory_resource
    G3D::SystemAlloc::resetMallocPerformanceCounters();

    using SIMDStringPmr = ::SIMDString<64, std::pmr::unsynchronized_pool_resource>;
    {
       SIMDStringPmr simdstring0;
       SIMDStringPmr simdstring1('a');
       SIMDStringPmr simdstring2("0123456789abcdefghijklmnopqrstuvwxyz");
       SIMDStringPmr simdstring3(simdstring2, 10);
       SIMDStringPmr simdstring4(simdstring2, 10, 10);
       SIMDStringPmr simdstring5(simdstring2.begin(), simdstring2.begin() + 10);
       SIMDStringPmr simdstring6{ SIMDStringPmr(sampleString) };
       SIMDStringPmr simdstring7({ 'a', 'b', 'c' });

       SIMDStringPmr simdstringXXL("0123456789abcdefghijklmnopqrstuvwxyz hjhjkhkhjkhjkhjkhjkhjkhjkhjkhjkkhjkjhkhjhjkhjhjkjkhhjkhjkhjkhjkkhjhjkhjk");
       simdstringXXL.append("xxxx");

       std::cout << "\n" << "SystemAlloc's status (PMR):\n" << G3D::SystemAlloc::mallocStatus() << "\n";
    }

    // 5. use g3d_buffer_pool_resource
    G3D::SystemAlloc::resetMallocPerformanceCounters();

    using SIMDString_G3dPmr = ::SIMDString<64, g3d_buffer_pool_resource>;
    {
       SIMDString_G3dPmr simdstring0;
       SIMDString_G3dPmr simdstring1('a');
       SIMDString_G3dPmr simdstring2("0123456789abcdefghijklmnopqrstuvwxyz");
       SIMDString_G3dPmr simdstring3(simdstring2, 10);
       SIMDString_G3dPmr simdstring4(simdstring2, 10, 10);
       SIMDString_G3dPmr simdstring5(simdstring2.begin(), simdstring2.begin() + 10);
       SIMDString_G3dPmr simdstring6{ SIMDString_G3dPmr(sampleString) };
       SIMDString_G3dPmr simdstring7({ 'a', 'b', 'c' });

       SIMDString_G3dPmr simdstringXXL("0123456789abcdefghijklmnopqrstuvwxyz hjhjkhkhjkhjkhjkhjkhjkhjkhjkhjkkhjkjhkhjhjkhjhjkjkhhjkhjkhjkhjkkhjhjkhjk");
       simdstringXXL.append("xxxx");

       std::cout << "\n" << "SystemAlloc's status (G3D_PMR):\n" << G3D::SystemAlloc::mallocStatus() << "\n";
    }

    // 6. use g3d_buffer_pool_resource with std::pmr classes
    G3D::SystemAlloc::resetMallocPerformanceCounters();
    g3d_buffer_pool_resource memRes;

    std::pmr::string pmrstring0(&memRes);
    std::pmr::string pmrstring1("a", &memRes);
    std::pmr::string pmrstring2("0123456789abcdefghijklmnopqrstuvwxyz", &memRes);

    std::pmr::string pmrstring3(pmrstring2, 10, &memRes);
    std::pmr::string pmrstring4(pmrstring2, 10, 10, &memRes);
    std::pmr::string pmrstring5(pmrstring2.begin(), pmrstring2.begin() + 10, &memRes);

    //std::pmr::string pmrstring6{ std::pmr::string(sampleString), &memRes }; --> CRASH!!!!
    std::pmr::string pmrstring6{ std::pmr::string(sampleString, &memRes), &memRes }; // --> OK
    std::pmr::string pmrstring6a{ std::pmr::string(sampleString) }; // --> OK too
    
    std::pmr::string pmrstring7({ 'a', 'b', 'c' }, &memRes);

    std::pmr::string pmrstringXXL("0123456789abcdefghijklmnopqrstuvwxyz hjhjkhkhjkhjkhjkhjkhjkhjkhjkhjkkhjkjhkhjhjkhjhjkjkhhjkhjkhjkhjkkhjhjkhjk", &memRes);
    pmrstringXXL.append("xxxx");

    std::cout << "\n" << "SystemAlloc's status (std::pmr):\n" << G3D::SystemAlloc::mallocStatus() << "\n";
#endif
#endif

    // done
    std::cout << "\n --> done!\n";
}
