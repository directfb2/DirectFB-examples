/*
   This file is part of DirectFB-examples.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
*/

#include <direct/util.h>
#ifdef VK_PHYSICAL_DEVICE_TYPE
#define VK_USE_PLATFORM_DIRECTFB_EXT
#include <vulkan/vulkan.h>

#include "util.h"

/* DirectFB interfaces */
static IDirectFB            *dfb          = NULL;
static IDirectFBInputDevice *mouse        = NULL;
static IDirectFBEventBuffer *event_buffer = NULL;
static IDirectFBSurface     *primary      = NULL;

/* screen width and height */
static int screen_width, screen_height;

/**********************************************************************************************************************/

static VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

static VkInstance       instance;
static VkSurfaceKHR     surface;
static VkDevice         logicalDevice;
static VkQueue          queue;
static VkSwapchainKHR   swapchain;
static VkImage          image;
static VkCommandPool    commandPool;
static VkCommandBuffer  commandBuffer;

void vulkan_init( void )
{
     int                             i;
     uint32_t                        count;
     const char                     *extensionName;
     VkPhysicalDevice               *physicalDevices;
     VkPhysicalDeviceType            deviceType;
     VkDeviceQueueCreateInfo         queueCreateInfo           = { 0 };
     VkDeviceCreateInfo              deviceCreateInfo          = { 0 };
     VkDirectFBSurfaceCreateInfoEXT  surfaceCreateInfo         = { 0 };
     VkInstanceCreateInfo            instanceCreateInfo        = { 0 };
     VkSwapchainCreateInfoKHR        swapchainCreateInfo       = { 0 };
     VkCommandPoolCreateInfo         commandPoolCreateInfo     = { 0 };
     VkCommandBufferAllocateInfo     commandBufferAllocateInfo = { 0 };

     /* create instance */

     extensionName = VK_EXT_DIRECTFB_SURFACE_EXTENSION_NAME;

     instanceCreateInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
     instanceCreateInfo.enabledExtensionCount   = 1;
     instanceCreateInfo.ppEnabledExtensionNames = &extensionName;

     vkCreateInstance( &instanceCreateInfo, NULL, &instance );

     /* create surface */

     surfaceCreateInfo.sType   = VK_STRUCTURE_TYPE_DIRECTFB_SURFACE_CREATE_INFO_EXT;
     surfaceCreateInfo.dfb     = dfb;
     surfaceCreateInfo.surface = primary;

     vkCreateDirectFBSurfaceEXT( instance, &surfaceCreateInfo, NULL, &surface );

     /* select physical device */

     vkEnumeratePhysicalDevices( instance, &count, NULL );
     if (!count) {
          printf( "No physical devices\n" );
          vkDestroyInstance( instance, NULL );
          exit( 42 );
     }

     physicalDevices = alloca( count * sizeof(VkPhysicalDevice) );

     vkEnumeratePhysicalDevices( instance, &count, physicalDevices );

     for (i = 0; i < count; i++) {
          VkPhysicalDeviceProperties properties;

          vkGetPhysicalDeviceProperties( physicalDevices[i], &properties );

          deviceType = properties.deviceType;

          if (deviceType == VK_PHYSICAL_DEVICE_TYPE) {
               physicalDevice = physicalDevices[i];
               break;
          }
     }

     if (deviceType != VK_PHYSICAL_DEVICE_TYPE) {
          physicalDevice = physicalDevices[0];
          printf( "Using a fallback physical device (type: %d)\n", deviceType );
     }

     /* create logical device */

     queueCreateInfo.sType      = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
     queueCreateInfo.queueCount = 1;

     extensionName = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

     deviceCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
     deviceCreateInfo.queueCreateInfoCount    = 1;
     deviceCreateInfo.pQueueCreateInfos       = &queueCreateInfo;
     deviceCreateInfo.enabledExtensionCount   = 1;
     deviceCreateInfo.ppEnabledExtensionNames = &extensionName;

     vkCreateDevice( physicalDevice, &deviceCreateInfo, NULL, &logicalDevice );

     /* get queue */

     vkGetDeviceQueue( logicalDevice, 0, 0, &queue );

     /* create swapchain */

     swapchainCreateInfo.sType              = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
     swapchainCreateInfo.surface            = surface;
     swapchainCreateInfo.minImageCount      = 1;
     swapchainCreateInfo.imageFormat        = VK_FORMAT_B8G8R8A8_UNORM;
     swapchainCreateInfo.imageExtent.width  = screen_width;
     swapchainCreateInfo.imageExtent.height = screen_height;
     swapchainCreateInfo.imageArrayLayers   = 1;

     vkCreateSwapchainKHR( logicalDevice, &swapchainCreateInfo, NULL, &swapchain );

     /* get a presentable image */

     count = 1;

     vkGetSwapchainImagesKHR( logicalDevice, swapchain, &count, &image );

     /* create command pool */

     commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

     vkCreateCommandPool( logicalDevice, &commandPoolCreateInfo, NULL, &commandPool );

     /* allocate command buffer */

     commandBufferAllocateInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
     commandBufferAllocateInfo.commandPool        = commandPool;
     commandBufferAllocateInfo.commandBufferCount = 1;

     vkAllocateCommandBuffers( logicalDevice, &commandBufferAllocateInfo, &commandBuffer );
}

static void vulkan_term( void )
{
     vkFreeCommandBuffers( logicalDevice, commandPool, 1, &commandBuffer );
     vkDestroyCommandPool( logicalDevice, commandPool, NULL );
     vkDestroySwapchainKHR( logicalDevice, swapchain, NULL );
     vkDestroyDevice( logicalDevice, NULL );
     vkDestroySurfaceKHR( instance, surface, NULL );
     vkDestroyInstance( instance, NULL );
}

void color_gradient(VkClearColorValue startColor, VkClearColorValue endColor)
{
     int           i;
     DFBInputEvent evt;

     if (!physicalDevice)
          vulkan_init();

     /* draw the color transition between two vertices */

     for (i = 0; i < 256; i++) {
          float                    t;
          VkClearColorValue        color;
          uint32_t                 imageIndex             = 0;
          VkImageSubresourceRange  range                  = { 0 };
          VkSubmitInfo             submitInfo             = { 0 };
          VkPresentInfoKHR         presentInfo            = { 0 };
          VkCommandBufferBeginInfo commandBufferBeginInfo = { 0 };

          /* fill image with the interpolated color */

          commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

          vkBeginCommandBuffer( commandBuffer, &commandBufferBeginInfo );

          t = i / 255.0f;

          color.float32[0] = (1 - t) * startColor.float32[0] + t * endColor.float32[0];
          color.float32[1] = (1 - t) * startColor.float32[1] + t * endColor.float32[1];
          color.float32[2] = (1 - t) * startColor.float32[2] + t * endColor.float32[2];
          color.float32[3] = 1;

          usleep( 10000 );

          range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
          range.levelCount = 1;
          range.layerCount = 1;

          vkCmdClearColorImage( commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &color, 1, &range );

          vkEndCommandBuffer( commandBuffer );

          /* submit the command buffer */

          submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
          submitInfo.commandBufferCount = 1;
          submitInfo.pCommandBuffers    = &commandBuffer;

          vkQueueSubmit( queue, 1, &submitInfo, NULL );

          /* queue image for presentation */

          presentInfo.sType          = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
          presentInfo.swapchainCount = 1;
          presentInfo.pSwapchains    = &swapchain;
          presentInfo.pImageIndices  = &imageIndex;

          vkQueuePresentKHR( queue, &presentInfo );
     }

     /* process event buffer */

     while (event_buffer->GetEvent( event_buffer, DFB_EVENT(&evt) ) == DFB_OK) {
          if (evt.buttons & DIBM_LEFT) {
               if (event_buffer->WaitForEventWithTimeout( event_buffer, 2, 0 ) == DFB_TIMEOUT) {
                    vulkan_term();
                    exit( 42 );
               }
          }
     }
}

/**********************************************************************************************************************/

static void dfb_shutdown( void )
{
     if (primary)      primary->Release( primary );
     if (event_buffer) event_buffer->Release( event_buffer );
     if (mouse)        mouse->Release( mouse );
     if (dfb)          dfb->Release( dfb );
}

int main( int argc, char *argv[] )
{
     DFBSurfaceDescription   desc;
     const VkClearColorValue black   = { { 0, 0, 0, 1 } };
     const VkClearColorValue red     = { { 1, 0, 0, 1 } };
     const VkClearColorValue green   = { { 0, 1, 0, 1 } };
     const VkClearColorValue blue    = { { 0, 0, 1, 1 } };
     const VkClearColorValue cyan    = { { 0, 1, 1, 1 } };
     const VkClearColorValue magenta = { { 1, 0, 1, 1 } };
     const VkClearColorValue yellow  = { { 1, 1, 0, 1 } };
     const VkClearColorValue white   = { { 1, 1, 1, 1 } };

     /* initialize DirectFB including command line parsing */
     DFBCHECK(DirectFBInit( &argc, &argv ));

     /* create the main interface */
     DFBCHECK(DirectFBCreate( &dfb ));

     /* register termination function */
     atexit( dfb_shutdown );

     /* set the cooperative level to DFSCL_FULLSCREEN for exclusive access to the primary layer */
     dfb->SetCooperativeLevel( dfb, DFSCL_FULLSCREEN );

     /* create an event buffer for mouse events */
     DFBCHECK(dfb->GetInputDevice( dfb, DIDID_MOUSE, &mouse ));
     DFBCHECK(mouse->CreateEventBuffer( mouse, &event_buffer ));

     /* fill the surface description */
     desc.flags = DSDESC_CAPS;
     desc.caps  = DSCAPS_PRIMARY;

     /* get the primary surface, i.e. the surface of the primary layer */
     DFBCHECK(dfb->CreateSurface( dfb, &desc, &primary ));

     /* query the size of the primary surface */
     DFBCHECK(primary->GetSize( primary, &screen_width, &screen_height ));

     /* edge and face diagonal gradients */
     color_gradient( black,   red );
     color_gradient( red,     yellow );
     color_gradient( yellow,  magenta );
     color_gradient( magenta, red );
     color_gradient( red,     blue );
     color_gradient( blue,    green );
     color_gradient( green,   white );
     color_gradient( white,   yellow );
     color_gradient( yellow,  black );
     color_gradient( black,   cyan );
     color_gradient( cyan,    blue );
     color_gradient( blue,    magenta );
     color_gradient( magenta, white );
     color_gradient( white,   cyan );
     color_gradient( cyan,    magenta );
     color_gradient( magenta, black );
     color_gradient( black,   blue );
     color_gradient( blue,    white );
     color_gradient( white,   red );
     color_gradient( red,     green );
     color_gradient( green,   cyan );
     color_gradient( cyan,    yellow );
     color_gradient( yellow,  green );
     color_gradient( green,   black );

     vulkan_term();

     return 42;
}
#else
int main( int argc, char *argv[] )
{
     printf( "No Vulkan support\n" );
     exit( 1 );
}
#endif
