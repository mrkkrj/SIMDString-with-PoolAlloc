/**
  \file DebugHelpers.h
  
  --> from:
  \file G3D-base.lib/include/G3D-base/format.h

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
  <--

  mrkkrj: extracted from original G3D code for use in PoolAllocator's impl.
*/

#include <cassert>

#ifndef debugAssertM
#ifdef G3D_DEBUG
#define debugAssertM(a, b) assert((a) && (b))
#else
#define debugAssertM(a, b) 
#endif
#endif

#ifndef alwaysAssertM
#define alwaysAssertM(a, b) assert((a) && (b))
#endif

#ifndef debugAssert
#ifdef G3D_DEBUG
#define debugAssert(a) assert((a))
#else
#define debugAssert(a) 
#endif
#endif


//
// From: G3D-base.lib/include/G3D-base/format.h
//

namespace G3D {

    /**
      Produces a string from arguments of the style of printf.  This avoids
      problems with buffer overflows when using sprintf and makes it easy
      to use the result functionally.  This function is fast when the resulting
      string is under 160 characters (not including terminator) and slower
      when the string is longer.
     */
   String format(const char* fmt ...); /*G3D_CHECK_PRINTF_ARGS*/  

    /**
      Like format, but can be called with the argument list from a ... function.
     */
   String vformat(const char* fmt,va_list argPtr); /*G3D_CHECK_VPRINTF_ARGS*/ 

} // namespace


//
// From: G3D-base.lib/include/G3D-base/debugPrintf.h
//

namespace G3D  {

   /**
      Under Visual Studio, appears in the Debug pane of the Output window
      On Unix-based operating systems the output is sent to stderr.

      OPEN TODO::: (mrkkrj) -> Also sends output to the console (G3D::consolePrintf) if there is a consolePrintHook, and flushes before returning.

      \return The string that was printed
   */
   String debugPrintf(const char* fmt ...);

} // namespace
