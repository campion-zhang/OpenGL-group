/*
 * Copyright 2020 Red Hat, Inc.
 * Copyright © 2021 Valve Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */


#include "zink_context.h"
#include "zink_screen.h"
#include "zink_resource.h"
#include "zink_kopper.h"
#include "vk_enum_to_str.h"

#define kopper_displaytarget(dt) ((struct kopper_displaytarget*)dt)

static void
init_dt_type(struct kopper_displaytarget *cdt)
{
    VkStructureType type = cdt->info.bos.sType;
    switch (type) {
#ifdef VK_USE_PLATFORM_XCB_KHR
    case VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR:
       cdt->type = KOPPER_X11;
       break;
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    case VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR:
       cdt->type = KOPPER_WAYLAND;
       break;
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
    case VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR:
       cdt->type = KOPPER_WIN32;
       break;
#endif
    default:
       unreachable("unsupported!");
    }
#ifdef WIN32
    // not hooked up yet so let's not sabotage benchmarks
    cdt->present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
#else
    // Matches the EGL and GLX_SGI_swap_interval default
    cdt->present_mode = VK_PRESENT_MODE_FIFO_KHR;
#endif
}

static VkSurfaceKHR
kopper_CreateSurface(struct zink_screen *screen, struct kopper_displaytarget *cdt)
{
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkResult error = VK_SUCCESS;

    init_dt_type(cdt);
    VkStructureType type = cdt->info.bos.sType;
    switch (type) {
#ifdef VK_USE_PLATFORM_XCB_KHR
    case VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR:
       error = VKSCR(CreateXcbSurfaceKHR)(screen->instance, &cdt->info.xcb, NULL, &surface);
       break;
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    case VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR:
       error = VKSCR(CreateWaylandSurfaceKHR)(screen->instance, &cdt->info.wl, NULL, &surface);
       break;
#endif
 #ifdef VK_USE_PLATFORM_WIN32_KHR
    case VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR:
       error = VKSCR(CreateWin32SurfaceKHR)(screen->instance, &cdt->info.win32, NULL, &surface);
       break;
#endif
    default:
       unreachable("unsupported!");
    }
    if (error != VK_SUCCESS) {
       return VK_NULL_HANDLE;
    }

    VkBool32 supported;
    error = VKSCR(GetPhysicalDeviceSurfaceSupportKHR)(screen->pdev, screen->gfx_queue, surface, &supported);
    if (!zink_screen_handle_vkresult(screen, error) || !supported)
       goto fail;

    unsigned count = 10;
    VkPresentModeKHR modes[10];
    error = VKSCR(GetPhysicalDeviceSurfacePresentModesKHR)(screen->pdev, surface, &count, modes);
    if (!zink_screen_handle_vkresult(screen, error))
       goto fail;

    for (unsigned i = 0; i < count; i++) {
       /* VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR and VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR
        * are not handled
        */
       assert(modes[i] <= VK_PRESENT_MODE_FIFO_RELAXED_KHR);
       if (modes[i] <= VK_PRESENT_MODE_FIFO_RELAXED_KHR)
          cdt->present_modes |= BITFIELD_BIT(modes[i]);
    }

    return surface;
fail:
   VKSCR(DestroySurfaceKHR)(screen->instance, surface, NULL);
   return VK_NULL_HANDLE;
}

static void
destroy_swapchain(struct zink_screen *screen, struct kopper_swapchain *cswap)
{
   if (!cswap)
      return;
   free(cswap->images);
   free(cswap->inits);
   for (unsigned i = 0; i < cswap->num_images; i++) {
      VKSCR(DestroySemaphore)(screen->dev, cswap->acquires[i], NULL);
   }
   free(cswap->acquires);
   hash_table_foreach(cswap->presents, he) {
      struct util_dynarray *arr = he->data;
      while (util_dynarray_contains(arr, VkSemaphore))
         VKSCR(DestroySemaphore)(screen->dev, util_dynarray_pop(arr, VkSemaphore), NULL);
      util_dynarray_fini(arr);
      free(arr);
   }
   _mesa_hash_table_destroy(cswap->presents, NULL);
   VKSCR(DestroySwapchainKHR)(screen->dev, cswap->swapchain, NULL);
   free(cswap);
}

static void
prune_old_swapchains(struct zink_screen *screen, struct kopper_displaytarget *cdt, bool wait)
{
   while (cdt->old_swapchain) {
      struct kopper_swapchain *cswap = cdt->old_swapchain;
      if (cswap->async_presents) {
         if (wait)
            continue;
         return;
      }
      cdt->old_swapchain = cswap->next;
      destroy_swapchain(screen, cswap);
   }
}

static struct hash_entry *
find_dt_entry(struct zink_screen *screen, const struct kopper_displaytarget *cdt)
{
   struct hash_entry *he = NULL;
   switch (cdt->type) {
#ifdef VK_USE_PLATFORM_XCB_KHR
   case KOPPER_X11:
      he = _mesa_hash_table_search_pre_hashed(&screen->dts, cdt->info.xcb.window, (void*)(uintptr_t)cdt->info.xcb.window);
      break;
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
   case KOPPER_WAYLAND:
      he = _mesa_hash_table_search(&screen->dts, cdt->info.wl.surface);
      break;
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
   case KOPPER_WIN32:
      he = _mesa_hash_table_search(&screen->dts, cdt->info.win32.hwnd);
      break;
#endif
   default:
      unreachable("unsupported!");
   }
   return he;
}

void
zink_kopper_deinit_displaytarget(struct zink_screen *screen, struct kopper_displaytarget *cdt)
{
   if (!cdt->surface)
      return;
   simple_mtx_lock(&screen->dt_lock);
   struct hash_entry *he = find_dt_entry(screen, cdt);
   assert(he);
   /* this deinits the registered entry, which should always be the "right" entry */
   cdt = he->data;
   _mesa_hash_table_remove(&screen->dts, he);
   simple_mtx_unlock(&screen->dt_lock);
   destroy_swapchain(screen, cdt->swapchain);
   prune_old_swapchains(screen, cdt, true);
   VKSCR(DestroySurfaceKHR)(screen->instance, cdt->surface, NULL);
   cdt->swapchain = cdt->old_swapchain = NULL;
   cdt->surface = VK_NULL_HANDLE;
   util_queue_fence_destroy(&cdt->present_fence);
}

static struct kopper_swapchain *
kopper_CreateSwapchain(struct zink_screen *screen, struct kopper_displaytarget *cdt, unsigned w, unsigned h, VkResult *result)
{
   VkResult error = VK_SUCCESS;
   struct kopper_swapchain *cswap = CALLOC_STRUCT(kopper_swapchain);
   if (!cswap)
      return NULL;
   cswap->last_present_prune = 1;

   bool has_alpha = cdt->info.has_alpha && (cdt->caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR);
   if (cdt->swapchain) {
      cswap->scci = cdt->swapchain->scci;
      cswap->scci.oldSwapchain = cdt->swapchain->swapchain;
   } else {
      cswap->scci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
      cswap->scci.pNext = NULL;
      cswap->scci.surface = cdt->surface;
      cswap->scci.flags = zink_kopper_has_srgb(cdt) ? VK_SWAPCHAIN_CREATE_MUTABLE_FORMAT_BIT_KHR : 0;
      cswap->scci.imageFormat = cdt->formats[0];
      cswap->scci.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
      // TODO: This is where you'd hook up stereo
      cswap->scci.imageArrayLayers = 1;
      cswap->scci.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                               VK_IMAGE_USAGE_SAMPLED_BIT |
                               VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                               VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
      cswap->scci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
      cswap->scci.queueFamilyIndexCount = 0;
      cswap->scci.pQueueFamilyIndices = NULL;
      cswap->scci.compositeAlpha = has_alpha ? VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR : VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
      cswap->scci.clipped = VK_TRUE;
   }
   cswap->scci.presentMode = cdt->present_mode;
   cswap->scci.minImageCount = cdt->caps.minImageCount;
   cswap->scci.preTransform = cdt->caps.currentTransform;
   if (cdt->formats[1])
      cswap->scci.pNext = &cdt->format_list;

   /* different display platforms have, by vulkan spec, different sizing methodologies */
   switch (cdt->type) {
   case KOPPER_X11:
   case KOPPER_WIN32:
      /* With Xcb, minImageExtent, maxImageExtent, and currentExtent must always equal the window size.
       * ...
       * Due to above restrictions, it is only possible to create a new swapchain on this
       * platform with imageExtent being equal to the current size of the window.
       */
      cswap->scci.imageExtent.width = cdt->caps.currentExtent.width;
      cswap->scci.imageExtent.height = cdt->caps.currentExtent.height;
      break;
   case KOPPER_WAYLAND:
      /* On Wayland, currentExtent is the special value (0xFFFFFFFF, 0xFFFFFFFF), indicating that the
       * surface size will be determined by the extent of a swapchain targeting the surface. Whatever the
       * application sets a swapchain’s imageExtent to will be the size of the window, after the first image is
       * presented.
       */
      cswap->scci.imageExtent.width = w;
      cswap->scci.imageExtent.height = h;
      break;
   default:
      unreachable("unknown display platform");
   }

   error = VKSCR(CreateSwapchainKHR)(screen->dev, &cswap->scci, NULL,
                                &cswap->swapchain);
   if (error == VK_ERROR_NATIVE_WINDOW_IN_USE_KHR) {
      if (util_queue_is_initialized(&screen->flush_queue))
         util_queue_finish(&screen->flush_queue);
      if (VKSCR(QueueWaitIdle)(screen->queue) != VK_SUCCESS)
         debug_printf("vkQueueWaitIdle failed\n");
      zink_kopper_deinit_displaytarget(screen, cdt);
      error = VKSCR(CreateSwapchainKHR)(screen->dev, &cswap->scci, NULL,
                                   &cswap->swapchain);
   }
   if (error != VK_SUCCESS) {
       mesa_loge("CreateSwapchainKHR failed with %s\n", vk_Result_to_str(error));
       free(cswap);
       *result = error;
       return NULL;
   }
   cswap->max_acquires = cswap->scci.minImageCount - cdt->caps.minImageCount;
   cswap->last_present = UINT32_MAX;

   *result = VK_SUCCESS;
   return cswap;
}

static VkResult
kopper_GetSwapchainImages(struct zink_screen *screen, struct kopper_swapchain *cswap)
{
   VkResult error = VKSCR(GetSwapchainImagesKHR)(screen->dev, cswap->swapchain, &cswap->num_images, NULL);
   zink_screen_handle_vkresult(screen, error);
   if (error != VK_SUCCESS)
      return error;
   cswap->images = malloc(sizeof(VkImage) * cswap->num_images);
   cswap->acquires = calloc(cswap->num_images, sizeof(VkSemaphore));
   cswap->inits = calloc(cswap->num_images, sizeof(bool));
   cswap->presents = _mesa_hash_table_create_u32_keys(NULL);
   error = VKSCR(GetSwapchainImagesKHR)(screen->dev, cswap->swapchain, &cswap->num_images, cswap->images);
   zink_screen_handle_vkresult(screen, error);
   return error;
}

static VkResult
update_caps(struct zink_screen *screen, struct kopper_displaytarget *cdt)
{
   VkResult error = VKSCR(GetPhysicalDeviceSurfaceCapabilitiesKHR)(screen->pdev, cdt->surface, &cdt->caps);
   zink_screen_handle_vkresult(screen, error);
   return error;
}

static VkResult
update_swapchain(struct zink_screen *screen, struct kopper_displaytarget *cdt, unsigned w, unsigned h)
{
   VkResult error = update_caps(screen, cdt);
   if (error != VK_SUCCESS)
      return error;
   struct kopper_swapchain *cswap = kopper_CreateSwapchain(screen, cdt, w, h, &error);
   if (!cswap)
      return error;
   prune_old_swapchains(screen, cdt, false);
   struct kopper_swapchain **pswap = &cdt->old_swapchain;
   while (*pswap)
      *pswap = (*pswap)->next;
   *pswap = cdt->swapchain;
   cdt->swapchain = cswap;

   return kopper_GetSwapchainImages(screen, cdt->swapchain);
}

struct kopper_displaytarget *
zink_kopper_displaytarget_create(struct zink_screen *screen, unsigned tex_usage,
                                 enum pipe_format format, unsigned width,
                                 unsigned height, unsigned alignment,
                                 const void *loader_private, unsigned *stride)
{
   struct kopper_displaytarget *cdt;
   const struct kopper_loader_info *info = loader_private;

   {
      struct kopper_displaytarget k;
      struct hash_entry *he = NULL;
      k.info = *info;
      init_dt_type(&k);
      simple_mtx_lock(&screen->dt_lock);
      if (unlikely(!screen->dts.table)) {
         switch (k.type) {
         case KOPPER_X11:
            _mesa_hash_table_init(&screen->dts, screen, NULL, _mesa_key_pointer_equal);
            break;
         case KOPPER_WAYLAND:
         case KOPPER_WIN32:
            _mesa_hash_table_init(&screen->dts, screen, _mesa_hash_pointer, _mesa_key_pointer_equal);
            break;
         default:
            unreachable("unknown kopper type");
         }
      } else {
         he = find_dt_entry(screen, &k);
      }
      simple_mtx_unlock(&screen->dt_lock);
      if (he) {
         cdt = he->data;
         p_atomic_inc(&cdt->refcount);
         *stride = cdt->stride;
         return cdt;
      }
   }

   cdt = CALLOC_STRUCT(kopper_displaytarget);
   if (!cdt)
      return NULL;

   cdt->refcount = 1;
   cdt->loader_private = (void*)loader_private;
   cdt->info = *info;
   util_queue_fence_init(&cdt->present_fence);

   enum pipe_format srgb = PIPE_FORMAT_NONE;
   if (screen->info.have_KHR_swapchain_mutable_format) {
      srgb = util_format_is_srgb(format) ? util_format_linear(format) : util_format_srgb(format);
      /* why do these helpers have different default return values? */
      if (srgb == format)
         srgb = PIPE_FORMAT_NONE;
   }
   cdt->formats[0] = zink_get_format(screen, format);
   if (srgb) {
      cdt->format_list.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO;
      cdt->format_list.pNext = NULL;
      cdt->format_list.viewFormatCount = 2;
      cdt->format_list.pViewFormats = cdt->formats;

      cdt->formats[1] = zink_get_format(screen, srgb);
   }

   cdt->surface = kopper_CreateSurface(screen, cdt);
   if (!cdt->surface)
      goto out;

   if (update_swapchain(screen, cdt, width, height) != VK_SUCCESS)
      goto out;

   simple_mtx_lock(&screen->dt_lock);
   switch (cdt->type) {
#ifdef VK_USE_PLATFORM_XCB_KHR
   case KOPPER_X11:
      _mesa_hash_table_insert_pre_hashed(&screen->dts, cdt->info.xcb.window, (void*)(uintptr_t)cdt->info.xcb.window, cdt);
      break;
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
   case KOPPER_WAYLAND:
      _mesa_hash_table_insert(&screen->dts, cdt->info.wl.surface, cdt);
      break;
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
   case KOPPER_WIN32:
      _mesa_hash_table_insert(&screen->dts, cdt->info.win32.hwnd, cdt);
      break;
#endif
   default:
      unreachable("unsupported!");
   }
   simple_mtx_unlock(&screen->dt_lock);

   *stride = cdt->stride;
   return cdt;

//moar cleanup
out:
   return NULL;
}

void
zink_kopper_displaytarget_destroy(struct zink_screen *screen, struct kopper_displaytarget *cdt)
{
   if (!p_atomic_dec_zero(&cdt->refcount))
      return;
   zink_kopper_deinit_displaytarget(screen, cdt);
   FREE(cdt);
}

static VkResult
kopper_acquire(struct zink_screen *screen, struct zink_resource *res, uint64_t timeout)
{
   struct kopper_displaytarget *cdt = kopper_displaytarget(res->obj->dt);
   if (res->obj->acquire)
      return true;
   res->obj->acquire = VK_NULL_HANDLE;
   VkSemaphore acquire = VK_NULL_HANDLE;

   while (true) {
      if (res->obj->new_dt) {
         VkResult error = update_swapchain(screen, cdt, res->base.b.width0, res->base.b.height0);
         zink_screen_handle_vkresult(screen, error);
         if (error != VK_SUCCESS)
            return error;
         res->obj->new_dt = false;
         res->layout = VK_IMAGE_LAYOUT_UNDEFINED;
         res->obj->access = 0;
         res->obj->access_stage = 0;
      }
      if (timeout == UINT64_MAX && util_queue_is_initialized(&screen->flush_queue) &&
          p_atomic_read_relaxed(&cdt->swapchain->num_acquires) > cdt->swapchain->max_acquires) {
         util_queue_fence_wait(&cdt->present_fence);
      }
      VkSemaphoreCreateInfo sci = {
         VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
         NULL,
         0
      };
      VkResult ret;
      if (!acquire) {
         ret = VKSCR(CreateSemaphore)(screen->dev, &sci, NULL, &acquire);
         assert(acquire);
         if (ret != VK_SUCCESS)
            return ret;
      }
      ASSERTED unsigned prev = res->obj->dt_idx;
      ret = VKSCR(AcquireNextImageKHR)(screen->dev, cdt->swapchain->swapchain, timeout, acquire, VK_NULL_HANDLE, &res->obj->dt_idx);
      if (ret != VK_SUCCESS && ret != VK_SUBOPTIMAL_KHR) {
         if (ret == VK_ERROR_OUT_OF_DATE_KHR) {
            res->obj->new_dt = true;
            continue;
         }
         VKSCR(DestroySemaphore)(screen->dev, acquire, NULL);
         return ret;
      }
      assert(prev != res->obj->dt_idx);
      break;
   }

   cdt->swapchain->acquires[res->obj->dt_idx] = res->obj->acquire = acquire;
   res->obj->image = cdt->swapchain->images[res->obj->dt_idx];
   res->obj->acquired = false;
   if (!cdt->swapchain->inits[res->obj->dt_idx]) {
      /* swapchain images are initially in the UNDEFINED layout */
      res->layout = VK_IMAGE_LAYOUT_UNDEFINED;
      cdt->swapchain->inits[res->obj->dt_idx] = true;
   }
   if (timeout == UINT64_MAX) {
      res->obj->indefinite_acquire = true;
      p_atomic_inc(&cdt->swapchain->num_acquires);
   }
   res->obj->dt_has_data = false;
   return VK_SUCCESS;
}

static void
kill_swapchain(struct zink_context *ctx, struct zink_resource *res)
{
   struct zink_screen *screen = zink_screen(ctx->base.screen);
   /* dead swapchain */
   mesa_loge("zink: swapchain killed %p\n", res);
   zink_batch_reference_resource(&ctx->batch, res);
   struct pipe_resource *pres = screen->base.resource_create(&screen->base, &res->base.b);
   zink_resource_object_reference(screen, &res->obj, zink_resource(pres)->obj);
   res->layout = VK_IMAGE_LAYOUT_UNDEFINED;
   res->swapchain = false;
   pipe_resource_reference(&pres, NULL);
}

static bool
is_swapchain_kill(VkResult ret)
{
   return ret != VK_SUCCESS &&
          ret != VK_TIMEOUT &&
          ret != VK_NOT_READY &&
          ret != VK_SUBOPTIMAL_KHR;
}

bool
zink_kopper_acquire(struct zink_context *ctx, struct zink_resource *res, uint64_t timeout)
{
   assert(zink_is_swapchain(res));
   struct kopper_displaytarget *cdt = kopper_displaytarget(res->obj->dt);
   if (!cdt)
      /* dead swapchain */
      return false;
   if (cdt->is_kill) {
      kill_swapchain(ctx, res);
      return false;
   }
   const struct kopper_swapchain *cswap = cdt->swapchain;
   res->obj->new_dt |= res->base.b.width0 != cswap->scci.imageExtent.width ||
                       res->base.b.height0 != cswap->scci.imageExtent.height;
   VkResult ret = kopper_acquire(zink_screen(ctx->base.screen), res, timeout);
   if (ret == VK_SUCCESS || ret == VK_SUBOPTIMAL_KHR) {
      if (cswap != cdt->swapchain) {
         ctx->swapchain_size = cdt->swapchain->scci.imageExtent;
         res->base.b.width0 = ctx->swapchain_size.width;
         res->base.b.height0 = ctx->swapchain_size.height;
      }
   } else if (is_swapchain_kill(ret)) {
      kill_swapchain(ctx, res);
   }
   return !is_swapchain_kill(ret);
}

VkSemaphore
zink_kopper_acquire_submit(struct zink_screen *screen, struct zink_resource *res)
{
   assert(res->obj->dt);
   struct kopper_displaytarget *cdt = kopper_displaytarget(res->obj->dt);
   if (res->obj->acquired)
      return VK_NULL_HANDLE;
   assert(res->obj->acquire);
   res->obj->acquired = true;
   /* this is now owned by the batch */
   cdt->swapchain->acquires[res->obj->dt_idx] = VK_NULL_HANDLE;
   return res->obj->acquire;
}

VkSemaphore
zink_kopper_present(struct zink_screen *screen, struct zink_resource *res)
{
   assert(res->obj->dt);
   assert(!res->obj->present);
   VkSemaphoreCreateInfo sci = {
      VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      NULL,
      0
   };
   assert(res->obj->acquired);
   VkResult ret = VKSCR(CreateSemaphore)(screen->dev, &sci, NULL, &res->obj->present);
   return zink_screen_handle_vkresult(screen, ret) ? res->obj->present : VK_NULL_HANDLE;
}

struct kopper_present_info {
   VkPresentInfoKHR info;
   uint32_t image;
   struct kopper_swapchain *swapchain;
   struct zink_resource *res;
   VkSemaphore sem;
   bool indefinite_acquire;
};

static void
kopper_present(void *data, void *gdata, int thread_idx)
{
   struct kopper_present_info *cpi = data;
   struct kopper_displaytarget *cdt = cpi->res->obj->dt;
   struct kopper_swapchain *swapchain = cpi->swapchain;
   struct zink_screen *screen = gdata;
   VkResult error = VK_SUCCESS;
   cpi->info.pResults = &error;

   simple_mtx_lock(&screen->queue_lock);
   if (screen->driver_workarounds.implicit_sync && cdt->type != KOPPER_WIN32) {
      if (!screen->fence) {
         VkFenceCreateInfo fci = {0};
         fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
         VKSCR(CreateFence)(screen->dev, &fci, NULL, &screen->fence);
      }
      VKSCR(ResetFences)(screen->dev, 1, &screen->fence);
      VkSubmitInfo si = {0};
      si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      si.waitSemaphoreCount = 1;
      si.pWaitSemaphores = cpi->info.pWaitSemaphores;
      VkPipelineStageFlags stages = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
      si.pWaitDstStageMask = &stages;

      error = VKSCR(QueueSubmit)(screen->queue, 1, &si, screen->fence);
      if (!zink_screen_handle_vkresult(screen, error)) {
         simple_mtx_unlock(&screen->queue_lock);
         VKSCR(DestroySemaphore)(screen->dev, cpi->sem, NULL);
         goto out;
      }
      error = VKSCR(WaitForFences)(screen->dev, 1, &screen->fence, VK_TRUE, UINT64_MAX);
      if (!zink_screen_handle_vkresult(screen, error)) {
         simple_mtx_unlock(&screen->queue_lock);
         VKSCR(DestroySemaphore)(screen->dev, cpi->sem, NULL);
         goto out;
      }
      cpi->info.pWaitSemaphores = NULL;
      cpi->info.waitSemaphoreCount = 0;
   }
   VkResult error2 = VKSCR(QueuePresentKHR)(screen->thread_queue, &cpi->info);
   simple_mtx_unlock(&screen->queue_lock);
   swapchain->last_present = cpi->image;
   if (cpi->indefinite_acquire)
      p_atomic_dec(&swapchain->num_acquires);
   if (error2 == VK_SUBOPTIMAL_KHR && cdt->swapchain == swapchain)
      cpi->res->obj->new_dt = true;

   /* it's illegal to destroy semaphores if they're in use by a cmdbuf.
    * but what does "in use" actually mean?
    * in truth, when using timelines, nobody knows. especially not VVL.
    *
    * thus, to avoid infinite error spam and thread-related races,
    * present semaphores need their own free queue based on the
    * last-known completed timeline id so that the semaphore persists through
    * normal cmdbuf submit/signal and then also exists here when it's needed for the present operation
    */
   struct util_dynarray *arr;
   for (; screen->last_finished && swapchain->last_present_prune != screen->last_finished; swapchain->last_present_prune++) {
      struct hash_entry *he = _mesa_hash_table_search(swapchain->presents,
                                                      (void*)(uintptr_t)swapchain->last_present_prune);
      if (he) {
         arr = he->data;
         while (util_dynarray_contains(arr, VkSemaphore))
            VKSCR(DestroySemaphore)(screen->dev, util_dynarray_pop(arr, VkSemaphore), NULL);
         util_dynarray_fini(arr);
         free(arr);
         _mesa_hash_table_remove(swapchain->presents, he);
      }
   }
   /* queue this wait semaphore for deletion on completion of the next batch */
   assert(screen->curr_batch > 0);
   uint32_t next = screen->curr_batch + 1;
   struct hash_entry *he = _mesa_hash_table_search(swapchain->presents, (void*)(uintptr_t)next);
   if (he)
      arr = he->data;
   else {
      arr = malloc(sizeof(struct util_dynarray));
      util_dynarray_init(arr, NULL);
      _mesa_hash_table_insert(swapchain->presents, (void*)(uintptr_t)next, arr);
   }
   util_dynarray_append(arr, VkSemaphore, cpi->sem);
out:
   if (thread_idx != -1)
      p_atomic_dec(&swapchain->async_presents);
   free(cpi);
}

void
zink_kopper_present_queue(struct zink_screen *screen, struct zink_resource *res)
{
   assert(res->obj->dt);
   struct kopper_displaytarget *cdt = kopper_displaytarget(res->obj->dt);
   assert(res->obj->acquired);
   assert(res->obj->present);
   struct kopper_present_info *cpi = malloc(sizeof(struct kopper_present_info));
   cpi->sem = res->obj->present;
   cpi->res = res;
   cpi->swapchain = cdt->swapchain;
   cpi->indefinite_acquire = res->obj->indefinite_acquire;
   res->obj->last_dt_idx = cpi->image = res->obj->dt_idx;
   cpi->info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
   cpi->info.pNext = NULL;
   cpi->info.waitSemaphoreCount = 1;
   cpi->info.pWaitSemaphores = &cpi->sem;
   cpi->info.swapchainCount = 1;
   cpi->info.pSwapchains = &cdt->swapchain->swapchain;
   cpi->info.pImageIndices = &cpi->image;
   cpi->info.pResults = NULL;
   res->obj->present = VK_NULL_HANDLE;
   if (util_queue_is_initialized(&screen->flush_queue)) {
      p_atomic_inc(&cpi->swapchain->async_presents);
      util_queue_add_job(&screen->flush_queue, cpi, &cdt->present_fence,
                         kopper_present, NULL, 0);
   } else {
      kopper_present(cpi, screen, -1);
   }
   res->obj->acquire = VK_NULL_HANDLE;
   res->obj->indefinite_acquire = res->obj->acquired = false;
   res->obj->dt_idx = UINT32_MAX;
}

bool
zink_kopper_acquire_readback(struct zink_context *ctx, struct zink_resource *res)
{
   struct zink_screen *screen = zink_screen(ctx->base.screen);
   assert(res->obj->dt);
   struct kopper_displaytarget *cdt = kopper_displaytarget(res->obj->dt);
   const struct kopper_swapchain *cswap = cdt->swapchain;
   uint32_t last_dt_idx = res->obj->last_dt_idx;
   VkResult ret = VK_SUCCESS;
   if (!res->obj->acquire) {
      ret = kopper_acquire(screen, res, UINT64_MAX);
      if (is_swapchain_kill(ret)) {
         kill_swapchain(ctx, res);
         return false;
      }
   }
   /* if this hasn't been presented or if it has data, use this as the readback target */
   if (res->obj->last_dt_idx == UINT32_MAX || res->obj->dt_has_data)
      return false;
   while (res->obj->dt_idx != last_dt_idx) {
      if (!zink_kopper_present_readback(ctx, res))
         break;
      do {
         ret = kopper_acquire(screen, res, 0);
      } while (!is_swapchain_kill(ret) && (ret == VK_NOT_READY || ret == VK_TIMEOUT));
      if (is_swapchain_kill(ret)) {
         kill_swapchain(ctx, res);
         return false;
      }
   }
   if (cswap != cdt->swapchain) {
      ctx->swapchain_size = cdt->swapchain->scci.imageExtent;
      res->base.b.width0 = ctx->swapchain_size.width;
      res->base.b.height0 = ctx->swapchain_size.height;
   }
   return true;
}

bool
zink_kopper_present_readback(struct zink_context *ctx, struct zink_resource *res)
{
   struct zink_screen *screen = zink_screen(ctx->base.screen);
   VkSubmitInfo si = {0};
   if (res->obj->last_dt_idx == UINT32_MAX)
      return true;
   if (res->layout != VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
      zink_resource_image_barrier(ctx, res, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 0, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
      ctx->base.flush(&ctx->base, NULL, 0);
   }
   si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
   si.signalSemaphoreCount = 1;
   VkPipelineStageFlags mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
   si.pWaitDstStageMask = &mask;
   VkSemaphore acquire = zink_kopper_acquire_submit(screen, res);
   VkSemaphore present = res->obj->present ? res->obj->present : zink_kopper_present(screen, res);
   if (screen->threaded)
      util_queue_finish(&screen->flush_queue);
   si.waitSemaphoreCount = !!acquire;
   si.pWaitSemaphores = &acquire;
   si.pSignalSemaphores = &present;
   VkResult error = VKSCR(QueueSubmit)(screen->thread_queue, 1, &si, VK_NULL_HANDLE);
   if (!zink_screen_handle_vkresult(screen, error))
      return false;

   zink_kopper_present_queue(screen, res);
   error = VKSCR(QueueWaitIdle)(screen->queue);
   return zink_screen_handle_vkresult(screen, error);
}

bool
zink_kopper_update(struct pipe_screen *pscreen, struct pipe_resource *pres, int *w, int *h)
{
   struct zink_resource *res = zink_resource(pres);
   struct zink_screen *screen = zink_screen(pscreen);
   assert(pres->bind & PIPE_BIND_DISPLAY_TARGET);
   if (!res->obj->dt)
      return false;
   struct kopper_displaytarget *cdt = kopper_displaytarget(res->obj->dt);
   if (cdt->type != KOPPER_X11) {
      *w = res->base.b.width0;
      *h = res->base.b.height0;
      return true;
   }
   VkResult ret = update_caps(screen, cdt);
   if (ret != VK_SUCCESS) {
      mesa_loge("zink: failed to update swapchain capabilities: %s", vk_Result_to_str(ret));
      cdt->is_kill = true;
      return false;
   }
   *w = cdt->caps.currentExtent.width;
   *h = cdt->caps.currentExtent.height;
   return true;
}

bool
zink_kopper_is_cpu(const struct pipe_screen *pscreen)
{
   const struct zink_screen *screen = (const struct zink_screen*)pscreen;
   return screen->is_cpu;
}

void
zink_kopper_fixup_depth_buffer(struct zink_context *ctx)
{
   struct zink_screen *screen = zink_screen(ctx->base.screen);
   if (!ctx->fb_state.zsbuf)
      return;

   assert(ctx->fb_state.zsbuf->texture->bind & PIPE_BIND_DISPLAY_TARGET);

   struct zink_resource *res = zink_resource(ctx->fb_state.zsbuf->texture);
   struct zink_surface *surf = zink_csurface(ctx->fb_state.zsbuf);
   struct zink_ctx_surface *csurf = (struct zink_ctx_surface*)ctx->fb_state.zsbuf;
   if (surf->info.width == ctx->fb_state.width &&
       surf->info.height == ctx->fb_state.height)
      return;

   struct pipe_resource templ = *ctx->fb_state.zsbuf->texture;
   templ.width0 = ctx->fb_state.width;
   templ.height0 = ctx->fb_state.height;
   struct pipe_resource *pz = screen->base.resource_create(&screen->base, &templ);
   struct zink_resource *z = zink_resource(pz);
   zink_resource_object_reference(screen, &res->obj, z->obj);
   res->base.b.width0 = ctx->fb_state.width;
   res->base.b.height0 = ctx->fb_state.height;
   pipe_resource_reference(&pz, NULL);

   ctx->fb_state.zsbuf->width = ctx->fb_state.width;
   ctx->fb_state.zsbuf->height = ctx->fb_state.height;
   struct pipe_surface *psurf = ctx->base.create_surface(&ctx->base, &res->base.b, ctx->fb_state.zsbuf);
   struct zink_ctx_surface *cz = (struct zink_ctx_surface*)psurf;

   /* oh god why */
   zink_surface_reference(screen, &csurf->surf, cz->surf);
   pipe_surface_release(&ctx->base, &psurf);
}

bool
zink_kopper_check(struct pipe_resource *pres)
{
   struct zink_resource *res = zink_resource(pres);
   assert(pres->bind & PIPE_BIND_DISPLAY_TARGET);
   if (!res->obj->dt)
      return false;
   struct kopper_displaytarget *cdt = kopper_displaytarget(res->obj->dt);
   return !cdt->is_kill;
}

void
zink_kopper_set_swap_interval(struct pipe_screen *pscreen, struct pipe_resource *pres, int interval)
{
   struct zink_resource *res = zink_resource(pres);
   struct zink_screen *screen = zink_screen(pscreen);
   assert(res->obj->dt);
   struct kopper_displaytarget *cdt = kopper_displaytarget(res->obj->dt);
   VkPresentModeKHR old_present_mode = cdt->present_mode;

   assert(interval >= 0); /* TODO: VK_PRESENT_MODE_FIFO_RELAXED_KHR */
   if (interval == 0) {
      if (cdt->present_modes & BITFIELD_BIT(VK_PRESENT_MODE_IMMEDIATE_KHR))
         cdt->present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
      else
         cdt->present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
   } else if (interval > 0) {
      cdt->present_mode = VK_PRESENT_MODE_FIFO_KHR;
   }
   assert(cdt->present_modes & BITFIELD_BIT(cdt->present_mode));

   if (old_present_mode != cdt->present_mode)
      update_swapchain(screen, cdt, cdt->caps.currentExtent.width, cdt->caps.currentExtent.height);
}