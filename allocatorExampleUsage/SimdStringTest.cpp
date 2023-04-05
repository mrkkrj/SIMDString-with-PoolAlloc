
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

    // done
    std::cout << "\n --> done!\n";
}
