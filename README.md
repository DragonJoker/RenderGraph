|         Server     | Result |
|:------------------:|--------|
| AppVeyor           | [![Build Status](https://ci.appveyor.com/api/projects/status/github/DragonJoker/rendergraph?branch=master&svg=true)](https://ci.appveyor.com/project/DragonJoker/rendergraph) |


RenderGraph
============

This library owes to be used to handle Vulkan render passes and image transitions smoothly.

It allows the user to register its render passes, along with their attachments (input, sampled, colour, depth stencil...), and will generate a graph from that data.

It is a work in progress.

Current status
--------------

The user can register its passes.
The graph is generated.

Todo
----

Handling of "variants" (paths of a single pass that are triggered by specific conditions) if needed.
Generate the VkRenderPassCreateInfo, VkFramebufferCreateInfo, VkImageCreateInfo and VkImageViewCreateInfo from the graph status (for each variant if needed).
