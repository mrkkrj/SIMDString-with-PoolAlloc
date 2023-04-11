/**
  \file g3d_buffer_pool_resource.h

  \brief Implementation of g3d_buffer_pool_resource class
  
  mrkkrj: a PMR wrapping G3D::SystemAlloc memory allocator impl.
*/

#pragma once

#ifndef PMR_BUFFER_POOL_RESOURCE_H
#define PMR_BUFFER_POOL_RESOURCE_H

#include <memory_resource>
#include "PoolAllocator.h"


struct g3d_buffer_pool_resource 
   : public std::pmr::memory_resource
{
   virtual void* do_allocate(size_t bytes, size_t align)
   {
      return G3D::SystemAlloc::alignedMalloc(bytes, align);
   }

   virtual void do_deallocate(void* ptr, size_t bytes, size_t align)
   {
      (void)bytes;
      (void)align;

      G3D::SystemAlloc::alignedFree(ptr);
   }

   virtual bool do_is_equal(const memory_resource& that) const noexcept
   {
      (void)that;

      // As all instances of g3d_pool_allocator are stateless.
      return true;
   }
};

#endif
