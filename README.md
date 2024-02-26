<p align="center">
  <a href="https://github.com/DragonJoker/RenderGraph/actions?query=workflow%3ABuild+event%3Apush"><img alt="Build status" src="https://github.com/DragonJoker/RenderGraph/workflows/Build/badge.svg?event=push"></a>
  <a href="https://codecov.io/gh/DragonJoker/RenderGraph" ><img src="https://codecov.io/gh/DragonJoker/RenderGraph/graph/badge.svg?token=E0IGAPHLJO"/></a>
</p>


# RenderGraph

This library owes to be used to handle Vulkan render passes and image transitions smoothly.

It allows the user to register its render passes, along with their attachments (input, sampled, colour, depth stencil...), and will generate a runnable graph from that data.

## Current status

- The user can register its passes and their attachments.  
- The runnable graph is generated, and image layout transitions are handled.  
- The runnable graph commands can be recorded and submitted to a queue.  
- Handling of "variants" (optional passes, or paths of a single pass that are triggered by specific conditions).  

## Building

RenderGraph uses CMake.

The only dependency is Vulkan-Headers, and the CMake variable holding its folder is VULKAN_HEADERS_INCLUDE_DIRS, which you need to set on command line, or by editing the CMakeCache.txt.  
