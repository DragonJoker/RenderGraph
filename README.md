<p align="center">
  <a href="https://github.com/DragonJoker/RenderGraph/actions?query=workflow%3ABuild+event%3Apush"><img alt="Build status" src="https://github.com/DragonJoker/RenderGraph/workflows/Build/badge.svg?event=push"></a>
</p>


RenderGraph
============

This library owes to be used to handle Vulkan render passes and image transitions smoothly.

It allows the user to register its render passes, along with their attachments (input, sampled, colour, depth stencil...), and will generate a graph from that data.

It is a work in progress.

Current status
--------------

The user can register its passes.  
The graph is generated.  
The image view transitions are explicited in the graph, and can be used to determine render pass sequence.  

Todo
----

Support blend loops (first pass clears/loads, other passes blend to the result of the first pass).  
Handling of "variants" (paths of a single pass that are triggered by specific conditions) if needed.  
Generate the VkRenderPassCreateInfo, VkFramebufferCreateInfo, VkImageCreateInfo and VkImageViewCreateInfo from the graph status (for each variant if needed).  
Sort of run the graph, given variants, and a number of queues.