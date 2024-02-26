#include "Common.hpp"

#include <RenderGraph/DotExport.hpp>
#include <RenderGraph/GraphContext.hpp>
#include <RenderGraph/GraphVisitor.hpp>
#include <RenderGraph/ImageData.hpp>
#include <RenderGraph/ImageViewData.hpp>
#include <RenderGraph/FrameGraph.hpp>

#include <functional>
#include <map>
#include <sstream>

namespace test
{
	namespace
	{
		std::ostream & operator<<( std::ostream & stream
			, std::vector< crg::ImageViewId > const & values )
		{
			std::string sep;

			for ( auto & value : values )
			{
				stream << sep << value.id;
				sep = ", ";
			}

			return stream;
		}

		std::ostream & operator<<( std::ostream & stream
			, crg::FramePass const & value )
		{
			stream << value.getName();
			return stream;
		}

		void displayPasses( TestCounts & testCounts
			, std::ostream & stream
			, crg::RunnableGraph & value
			, crg::dot::Config const & cfg )
		{
			crg::dot::displayPasses( stream, value, cfg );
			std::ofstream file{ testCounts.testName + ".dot" };
			crg::dot::displayPasses( file, value, { true, true, true, false } );
		}

		void displayTransitions( TestCounts & testCounts
			, std::ostream & stream
			, crg::RunnableGraph & value
			, crg::dot::Config const & cfg )
		{
			crg::dot::displayTransitions( stream, value, cfg );
			std::ofstream file{ testCounts.testName + "_transitions.dot" };
			crg::dot::displayTransitions( file, value, { true, true, true, false } );
		}

		bool isDepthFormat( VkFormat fmt )
		{
			return fmt == VK_FORMAT_D16_UNORM
				|| fmt == VK_FORMAT_X8_D24_UNORM_PACK32
				|| fmt == VK_FORMAT_D32_SFLOAT
				|| fmt == VK_FORMAT_D16_UNORM_S8_UINT
				|| fmt == VK_FORMAT_D24_UNORM_S8_UINT
				|| fmt == VK_FORMAT_D32_SFLOAT_S8_UINT;
		}

		bool isStencilFormat( VkFormat fmt )
		{
			return fmt == VK_FORMAT_S8_UINT
				|| fmt == VK_FORMAT_D16_UNORM_S8_UINT
				|| fmt == VK_FORMAT_D24_UNORM_S8_UINT
				|| fmt == VK_FORMAT_D32_SFLOAT_S8_UINT;
		}

		bool isColourFormat( VkFormat fmt )
		{
			return !isDepthFormat( fmt ) && !isStencilFormat( fmt );
		}

		bool isDepthStencilFormat( VkFormat fmt )
		{
			return isDepthFormat( fmt ) && isStencilFormat( fmt );
		}
	}

	crg::ImageData createImage( std::string name
		, VkFormat format
		, uint32_t mipLevels
		, uint32_t arrayLayers )
	{
		return crg::ImageData{ std::move( name )
			, 0u
			, VK_IMAGE_TYPE_2D
			, format
			, { 1024, 1024 }
			, ( VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
				| VK_IMAGE_USAGE_SAMPLED_BIT )
			, mipLevels
			, arrayLayers };
	}

	crg::ImageViewData createView( std::string name
		, crg::ImageId image
		, uint32_t baseMipLevel
		, uint32_t levelCount
		, uint32_t baseArrayLayer
		, uint32_t layerCount )
	{
		return createView( std::move( name )
			, image
			, image.data->info.format
			, baseMipLevel
			, levelCount
			, baseArrayLayer
			, layerCount );
	}

	crg::ImageViewData createView( std::string name
		, crg::ImageId image
		, VkFormat format
		, uint32_t baseMipLevel
		, uint32_t levelCount
		, uint32_t baseArrayLayer
		, uint32_t layerCount )
	{
		VkImageAspectFlags aspect = ( isDepthStencilFormat( format )
			? ( VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_DEPTH_BIT )
			: ( isDepthFormat( format )
				? VK_IMAGE_ASPECT_DEPTH_BIT
				: ( isStencilFormat( format )
					? VK_IMAGE_ASPECT_STENCIL_BIT
					: VK_IMAGE_ASPECT_COLOR_BIT ) ) );
		return crg::ImageViewData{ std::move( name )
			, image
			, 0u
			, VK_IMAGE_VIEW_TYPE_2D
			, format
			, { aspect, baseMipLevel, levelCount, baseArrayLayer, layerCount } };
	}

	crg::GraphContext & getDummyContext()
	{
		static crg::GraphContext context{ nullptr
			, nullptr
			, nullptr
			, VkPhysicalDeviceMemoryProperties{}
			, VkPhysicalDeviceProperties{}
			, false
			, nullptr };
		context.vkCreateGraphicsPipelines = PFN_vkCreateGraphicsPipelines( []( VkDevice, VkPipelineCache, uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo * pCreateInfos, const VkAllocationCallbacks *, VkPipeline * pPipelines )
			{
				for ( uint32_t i = 0u; i < createInfoCount; ++i )
				{
					pPipelines[i] = ( VkPipeline )uintptr_t( i + 1u );
				}
				return VK_SUCCESS;
			} );
		context.vkCreateComputePipelines = PFN_vkCreateComputePipelines( []( VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkComputePipelineCreateInfo * pCreateInfos, const VkAllocationCallbacks * pAllocator, VkPipeline * pPipelines )
			{
				for ( uint32_t i = 0u; i < createInfoCount; ++i )
				{
					pPipelines[i] = ( VkPipeline )uintptr_t( i + 1u );
				}
				return VK_SUCCESS;
			} );
		context.vkCreatePipelineLayout = PFN_vkCreatePipelineLayout( []( VkDevice device, const VkPipelineLayoutCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkPipelineLayout * pPipelineLayout )
			{
				*pPipelineLayout = ( VkPipelineLayout )1u;
				return VK_SUCCESS;
			} );
		context.vkCreateDescriptorSetLayout = PFN_vkCreateDescriptorSetLayout( []( VkDevice device, const VkDescriptorSetLayoutCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkDescriptorSetLayout * pSetLayout )
			{
				*pSetLayout = ( VkDescriptorSetLayout )1u;
				return VK_SUCCESS;
			} );
		context.vkCreateDescriptorPool = PFN_vkCreateDescriptorPool( []( VkDevice device, const VkDescriptorPoolCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkDescriptorPool * pDescriptorPool )
			{
				*pDescriptorPool = ( VkDescriptorPool )1u;
				return VK_SUCCESS;
			} );
		context.vkAllocateDescriptorSets = PFN_vkAllocateDescriptorSets( []( VkDevice device, const VkDescriptorSetAllocateInfo * pAllocateInfo, VkDescriptorSet * pDescriptorSets )
			{
				if ( !pAllocateInfo )
					return VK_ERROR_UNKNOWN;

				for ( uint32_t i = 0u; i < pAllocateInfo->descriptorSetCount; ++i )
				{
					pDescriptorSets[i] = ( VkDescriptorSet )uintptr_t( i + 1u );
				}
				return VK_SUCCESS;
			} );
		context.vkCreateBuffer = PFN_vkCreateBuffer( []( VkDevice device, const VkBufferCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkBuffer * pBuffer )
			{
				*pBuffer = ( VkBuffer )1u;
				return VK_SUCCESS;
			} );
		context.vkAllocateMemory = PFN_vkAllocateMemory( []( VkDevice device, const VkMemoryAllocateInfo * pAllocateInfo, const VkAllocationCallbacks * pAllocator, VkDeviceMemory * pMemory )
			{
				*pMemory = ( VkDeviceMemory )1u;
				return VK_SUCCESS;
			} );
		context.vkCreateRenderPass = PFN_vkCreateRenderPass( []( VkDevice device, const VkRenderPassCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkRenderPass * pRenderPass )
			{
				*pRenderPass = ( VkRenderPass )1u;
				return VK_SUCCESS;
			} );
		context.vkCreateFramebuffer = PFN_vkCreateFramebuffer( []( VkDevice device, const VkFramebufferCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkFramebuffer * pFramebuffer )
			{
				*pFramebuffer = ( VkFramebuffer )1u;
				return VK_SUCCESS;
			} );
		context.vkCreateImage = PFN_vkCreateImage( []( VkDevice device, const VkImageCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkImage * pImage )
			{
				*pImage = ( VkImage )1u;
				return VK_SUCCESS;
			} );
		context.vkCreateImageView = PFN_vkCreateImageView( []( VkDevice device, const VkImageViewCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkImageView * pView )
			{
				*pView = ( VkImageView )1u;
				return VK_SUCCESS;
			} );
		context.vkCreateSampler = PFN_vkCreateSampler( []( VkDevice device, const VkSamplerCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkSampler * pSampler )
			{
				*pSampler = ( VkSampler )1u;
				return VK_SUCCESS;
			} );
		context.vkCreateCommandPool = PFN_vkCreateCommandPool( []( VkDevice device, const VkCommandPoolCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkCommandPool * pCommandPool )
			{
				*pCommandPool = ( VkCommandPool )1u;
				return VK_SUCCESS;
			} );
		context.vkAllocateCommandBuffers = PFN_vkAllocateCommandBuffers( []( VkDevice device, const VkCommandBufferAllocateInfo * pAllocateInfo, VkCommandBuffer * pCommandBuffers )
			{
				if ( !pAllocateInfo )
					return VK_ERROR_UNKNOWN;

				for ( uint32_t i = 0u; i < pAllocateInfo->commandBufferCount; ++i )
				{
					pCommandBuffers[i] = ( VkCommandBuffer )uintptr_t( i + 1u );
				}
				return VK_SUCCESS;
			} );
		context.vkCreateSemaphore = PFN_vkCreateSemaphore( []( VkDevice device, const VkSemaphoreCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkSemaphore * pSemaphore )
			{
				*pSemaphore = ( VkSemaphore )1u;
				return VK_SUCCESS;
			} );
		context.vkCreateQueryPool = PFN_vkCreateQueryPool( []( VkDevice device, const VkQueryPoolCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkQueryPool * pQueryPool )
			{
				*pQueryPool = ( VkQueryPool )1u;
				return VK_SUCCESS;
			} );
		context.vkCreateEvent = PFN_vkCreateEvent( []( VkDevice device, const VkEventCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkEvent * pEvent )
			{
				*pEvent = ( VkEvent )1u;
				return VK_SUCCESS;
			} );
		context.vkCreateFence = PFN_vkCreateFence( []( VkDevice device, const VkFenceCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkFence * pFence )
			{
				*pFence = ( VkFence )1u;
				return VK_SUCCESS;
			} );

		context.vkDestroyPipeline = PFN_vkDestroyPipeline( []( VkDevice device, VkPipeline pipeline, const VkAllocationCallbacks * pAllocator ){} );
		context.vkDestroyPipelineLayout = PFN_vkDestroyPipelineLayout( []( VkDevice device, VkPipelineLayout pipelineLayout, const VkAllocationCallbacks * pAllocator ){} );
		context.vkDestroyDescriptorSetLayout = PFN_vkDestroyDescriptorSetLayout( []( VkDevice device, VkDescriptorSetLayout descriptorSetLayout, const VkAllocationCallbacks* pAllocator ){} );
		context.vkDestroyDescriptorPool = PFN_vkDestroyDescriptorPool( []( VkDevice device, VkDescriptorPool descriptorPool, const VkAllocationCallbacks * pAllocator ){} );
		context.vkFreeDescriptorSets = PFN_vkFreeDescriptorSets( []( VkDevice device, VkDescriptorPool descriptorPool, uint32_t descriptorSetCount, const VkDescriptorSet * pDescriptorSets ){ return VK_SUCCESS; } );
		context.vkDestroyBuffer = PFN_vkDestroyBuffer( []( VkDevice device, VkBuffer buffer, const VkAllocationCallbacks * pAllocator ){} );
		context.vkFreeMemory = PFN_vkFreeMemory( []( VkDevice device, VkDeviceMemory memory, const VkAllocationCallbacks * pAllocator ){} );
		context.vkDestroyRenderPass = PFN_vkDestroyRenderPass( []( VkDevice device, VkRenderPass renderPass, const VkAllocationCallbacks * pAllocator ){} );
		context.vkDestroyFramebuffer = PFN_vkDestroyFramebuffer( []( VkDevice device, VkFramebuffer framebuffer, const VkAllocationCallbacks * pAllocator ){} );
		context.vkDestroyImage = PFN_vkDestroyImage( []( VkDevice device, VkImage image, const VkAllocationCallbacks * pAllocator ){} );
		context.vkDestroyImageView = PFN_vkDestroyImageView( []( VkDevice device, VkImageView imageView, const VkAllocationCallbacks * pAllocator ){} );
		context.vkDestroySampler = PFN_vkDestroySampler( []( VkDevice device, VkSampler sampler, const VkAllocationCallbacks * pAllocator ){} );
		context.vkDestroyCommandPool = PFN_vkDestroyCommandPool( []( VkDevice device, VkCommandPool commandPool, const VkAllocationCallbacks * pAllocator ){} );
		context.vkFreeCommandBuffers = PFN_vkFreeCommandBuffers( []( VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount, const VkCommandBuffer * pCommandBuffers ){} );
		context.vkDestroySemaphore = PFN_vkDestroySemaphore( []( VkDevice device, VkSemaphore semaphore, const VkAllocationCallbacks * pAllocator ){} );
		context.vkDestroyQueryPool = PFN_vkDestroyQueryPool( []( VkDevice device, VkQueryPool queryPool, const VkAllocationCallbacks * pAllocator ){} );
		context.vkDestroyEvent = PFN_vkDestroyEvent( []( VkDevice device, VkEvent event, const VkAllocationCallbacks * pAllocator ){} );
		context.vkDestroyFence = PFN_vkDestroyFence( []( VkDevice device, VkFence fence, const VkAllocationCallbacks * pAllocator ){} );

		context.vkGetBufferMemoryRequirements = PFN_vkGetBufferMemoryRequirements( []( VkDevice device, VkBuffer buffer, VkMemoryRequirements * pMemoryRequirements ){} );
		context.vkGetImageMemoryRequirements = PFN_vkGetImageMemoryRequirements( []( VkDevice device, VkImage image, VkMemoryRequirements * pMemoryRequirements ){} );
		context.vkBindBufferMemory = PFN_vkBindBufferMemory( []( VkDevice device, VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize memoryOffset ){ return VK_SUCCESS; } );
		context.vkBindImageMemory = PFN_vkBindImageMemory( []( VkDevice device, VkImage image, VkDeviceMemory memory, VkDeviceSize memoryOffset ){ return VK_SUCCESS; } );
		context.vkMapMemory = PFN_vkMapMemory( []( VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void ** ppData ){ return VK_SUCCESS; } );
		context.vkUnmapMemory = PFN_vkUnmapMemory( []( VkDevice device, VkDeviceMemory memory ){} );
		context.vkFlushMappedMemoryRanges = PFN_vkFlushMappedMemoryRanges( []( VkDevice device, uint32_t memoryRangeCount, const VkMappedMemoryRange * pMemoryRanges ){ return VK_SUCCESS; } );
		context.vkInvalidateMappedMemoryRanges = PFN_vkInvalidateMappedMemoryRanges( []( VkDevice device, uint32_t memoryRangeCount, const VkMappedMemoryRange * pMemoryRanges ){ return VK_SUCCESS; } );
		context.vkUpdateDescriptorSets = PFN_vkUpdateDescriptorSets( []( VkDevice device, uint32_t descriptorWriteCount, const VkWriteDescriptorSet * pDescriptorWrites, uint32_t descriptorCopyCount, const VkCopyDescriptorSet * pDescriptorCopies ){} );
		context.vkBeginCommandBuffer = PFN_vkBeginCommandBuffer( []( VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo * pBeginInfo ){ return VK_SUCCESS; } );
		context.vkEndCommandBuffer = PFN_vkEndCommandBuffer( []( VkCommandBuffer commandBuffer ){ return VK_SUCCESS; } );
		context.vkQueueSubmit = PFN_vkQueueSubmit( []( VkQueue queue, uint32_t submitCount, const VkSubmitInfo * pSubmits, VkFence fence ){ return VK_SUCCESS; } );
		context.vkGetQueryPoolResults = PFN_vkGetQueryPoolResults( []( VkDevice device, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, size_t dataSize, void * pData, VkDeviceSize stride, VkQueryResultFlags flags ){ return VK_SUCCESS; } );
		context.vkResetCommandBuffer = PFN_vkResetCommandBuffer( []( VkCommandBuffer commandBuffer, VkCommandBufferResetFlags flags ){ return VK_SUCCESS; } );
		context.vkResetEvent = PFN_vkResetEvent( []( VkDevice device, VkEvent event ){ return VK_SUCCESS; } );
		context.vkSetEvent = PFN_vkSetEvent( []( VkDevice device, VkEvent event ){ return VK_SUCCESS; } );
		context.vkGetEventStatus = PFN_vkGetEventStatus( []( VkDevice device, VkEvent event ){ return VK_SUCCESS; } );
		context.vkGetFenceStatus = PFN_vkGetFenceStatus( []( VkDevice device, VkFence fence ){ return VK_SUCCESS; } );
		context.vkWaitForFences = PFN_vkWaitForFences( []( VkDevice device, uint32_t fenceCount, const VkFence * pFences, VkBool32 waitAll, uint64_t timeout ){ return VK_SUCCESS; } );
		context.vkResetFences = PFN_vkResetFences( []( VkDevice device, uint32_t fenceCount, const VkFence * pFences ){ return VK_SUCCESS; } );

		context.vkCmdBindPipeline = PFN_vkCmdBindPipeline( []( VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline ){} );
		context.vkCmdBindDescriptorSets = PFN_vkCmdBindDescriptorSets( []( VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount, const VkDescriptorSet * pDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t * pDynamicOffsets ){} );
		context.vkCmdBindVertexBuffers = PFN_vkCmdBindVertexBuffers( []( VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount, const VkBuffer * pBuffers, const VkDeviceSize * pOffsets ){} );
		context.vkCmdBindIndexBuffer = PFN_vkCmdBindIndexBuffer( []( VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType ){} );
		context.vkCmdClearColorImage = PFN_vkCmdClearColorImage( []( VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, const VkClearColorValue * pColor, uint32_t rangeCount, const VkImageSubresourceRange * pRanges ){} );
		context.vkCmdClearDepthStencilImage = PFN_vkCmdClearDepthStencilImage( []( VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, const VkClearDepthStencilValue * pDepthStencil, uint32_t rangeCount, const VkImageSubresourceRange * pRanges ){} );
		context.vkCmdDispatch = PFN_vkCmdDispatch( []( VkCommandBuffer commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ ){} );
		context.vkCmdDispatchIndirect = PFN_vkCmdDispatchIndirect( []( VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset ){} );
		context.vkCmdDraw = PFN_vkCmdDraw( []( VkCommandBuffer commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance ){} );
		context.vkCmdDrawIndexed = PFN_vkCmdDrawIndexed( []( VkCommandBuffer commandBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance ){} );
		context.vkCmdDrawIndexedIndirect = PFN_vkCmdDrawIndexedIndirect( []( VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride ){} );
		context.vkCmdDrawIndirect = PFN_vkCmdDrawIndirect( []( VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride ){} );
		context.vkCmdBeginRenderPass = PFN_vkCmdBeginRenderPass( []( VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo * pRenderPassBegin, VkSubpassContents contents ){} );
		context.vkCmdEndRenderPass = PFN_vkCmdEndRenderPass( []( VkCommandBuffer commandBuffer ){} );
		context.vkCmdPushConstants = PFN_vkCmdPushConstants( []( VkCommandBuffer commandBuffer, VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void * pValues ){} );
		context.vkCmdResetQueryPool = PFN_vkCmdResetQueryPool( []( VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount ){} );
		context.vkCmdWriteTimestamp = PFN_vkCmdWriteTimestamp( []( VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool, uint32_t query ){} );
		context.vkCmdPipelineBarrier = PFN_vkCmdPipelineBarrier( []( VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const VkMemoryBarrier * pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier * pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier * pImageMemoryBarriers ){} );
		context.vkCmdBlitImage = PFN_vkCmdBlitImage( []( VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageBlit * pRegions, VkFilter filter ){} );
		context.vkCmdCopyBuffer = PFN_vkCmdCopyBuffer( []( VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy * pRegions ){} );
		context.vkCmdCopyBufferToImage = PFN_vkCmdCopyBufferToImage( []( VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkBufferImageCopy * pRegions ){} );
		context.vkCmdCopyImage = PFN_vkCmdCopyImage( []( VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageCopy * pRegions ){} );
		context.vkCmdCopyImageToBuffer = PFN_vkCmdCopyImageToBuffer( []( VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferImageCopy * pRegions ){} );
		context.vkCmdExecuteCommands = PFN_vkCmdExecuteCommands( []( VkCommandBuffer commandBuffer, uint32_t commandBufferCount, const VkCommandBuffer * pCommandBuffers ){} );
		context.vkCmdResetEvent = PFN_vkCmdResetEvent( []( VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask ){} );
		context.vkCmdSetEvent = PFN_vkCmdSetEvent( []( VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask ){} );
		context.vkCmdWaitEvents = PFN_vkCmdWaitEvents( []( VkCommandBuffer commandBuffer, uint32_t eventCount, const VkEvent * pEvents, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t memoryBarrierCount, const VkMemoryBarrier * pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier * pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier * pImageMemoryBarriers ){} );
		context.vkCmdFillBuffer = PFN_vkCmdFillBuffer( []( VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data ){} );

#if VK_EXT_debug_utils || VK_EXT_debug_marker
#	if VK_EXT_debug_utils
		context.vkSetDebugUtilsObjectNameEXT = PFN_vkSetDebugUtilsObjectNameEXT();
		context.vkCmdBeginDebugUtilsLabelEXT = PFN_vkCmdBeginDebugUtilsLabelEXT();
		context.vkCmdEndDebugUtilsLabelEXT = PFN_vkCmdEndDebugUtilsLabelEXT();
#	endif
#	if VK_EXT_debug_marker
		context.vkDebugMarkerSetObjectNameEXT = PFN_vkDebugMarkerSetObjectNameEXT();
		context.vkCmdDebugMarkerBeginEXT = PFN_vkCmdDebugMarkerBeginEXT();
		context.vkCmdDebugMarkerEndEXT = PFN_vkCmdDebugMarkerEndEXT();
		context.vkCmdDebugMarkerInsertEXT = PFN_vkCmdDebugMarkerInsertEXT();
#	endif
#endif
		return context;
	}

	void display( TestCounts & testCounts
		, std::ostream & stream
		, crg::RunnableGraph & value
		, bool withColours
		, bool withIds
		, bool withGroups )
	{
		std::stringstream trans;
		displayTransitions( testCounts, trans, value, { withColours, withIds, withGroups } );
		displayPasses( testCounts, stream, value, { withColours, withIds, withGroups } );
	}

	void display( TestCounts & testCounts
		, crg::RunnableGraph & value
		, bool withColours
		, bool withIds
		, bool withGroups )
	{
		display( testCounts, std::cout, value, withColours, withIds, withGroups );
	}
}
