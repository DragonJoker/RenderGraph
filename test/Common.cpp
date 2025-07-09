#include "Common.hpp"
#include "BaseTest.hpp"

#include <RenderGraph/DotExport.hpp>
#include <RenderGraph/GraphContext.hpp>
#include <RenderGraph/GraphVisitor.hpp>
#include <RenderGraph/ImageData.hpp>
#include <RenderGraph/ImageViewData.hpp>
#include <RenderGraph/FrameGraph.hpp>
#include <RenderGraph/RunnableGraph.hpp>

#include <atomic>
#include <functional>
#include <map>
#include <sstream>
#include <cstring>

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

		void displayPasses( TestCounts const & testCounts
			, std::ostream & stream
			, crg::RunnableGraph const & value
			, crg::dot::Config const & cfg )
		{
			crg::dot::displayPasses( stream, value, cfg );
			std::ofstream file{ testCounts.testName + ".dot" };
			crg::dot::displayPasses( file, value, { true, true, true, false } );
		}

		void displayTransitions( TestCounts const & testCounts
			, std::ostream & stream
			, crg::RunnableGraph const & value
			, crg::dot::Config const & cfg )
		{
			crg::dot::displayTransitions( stream, value, cfg );
			std::ofstream file{ testCounts.testName + "_transitions.dot" };
			crg::dot::displayTransitions( file, value, { true, true, true, false } );
		}

		crg::ImageViewType getViewType( crg::ImageType type
			, VkImageCreateFlags flags
			, uint32_t layerCount )
		{
			switch ( type )
			{
			case crg::ImageType::e1D:
				return layerCount > 1u
					? crg::ImageViewType::e1DArray
					: crg::ImageViewType::e1D;
			case crg::ImageType::e3D:
				return crg::ImageViewType::e3D;
			default:
				if ( layerCount > 1u )
				{
					if ( ( ( layerCount % 6u ) == 0u ) && ( flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT ) )
					{
						return ( layerCount > 6u )
							? crg::ImageViewType::eCubeArray
							: crg::ImageViewType::eCube;
					}
					else
					{
						return crg::ImageViewType::e2DArray;
					}
				}
				else
				{
					return crg::ImageViewType::e2D;
				}
			}
		}
	}

	crg::ImageData createImage( std::string name
		, crg::PixelFormat format
		, uint32_t mipLevels
		, uint32_t arrayLayers )
	{
		return crg::ImageData{ std::move( name )
			, 0u
			, crg::ImageType::e2D
			, format
			, { 1024, 1024 }
			, ( VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
				| VK_IMAGE_USAGE_SAMPLED_BIT )
			, mipLevels
			, arrayLayers };
	}

	crg::ImageData createImage1D( std::string name
		, crg::PixelFormat format
		, uint32_t mipLevels
		, uint32_t arrayLayers )
	{
		return crg::ImageData{ std::move( name )
			, 0u
			, crg::ImageType::e1D
			, format
			, { 1024 }
			, ( VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
				| VK_IMAGE_USAGE_SAMPLED_BIT )
			, mipLevels
			, arrayLayers };
	}

	crg::ImageData createImageCube( std::string name
		, crg::PixelFormat format
		, uint32_t mipLevels
		, uint32_t arrayLayers )
	{
		return crg::ImageData{ std::move( name )
			, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT
			, crg::ImageType::e2D
			, format
			, { 1024, 1024u }
			, ( VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
				| VK_IMAGE_USAGE_SAMPLED_BIT )
			, mipLevels
			, arrayLayers * 6u };
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
			, crg::convert( image.data->info.format )
			, baseMipLevel
			, levelCount
			, baseArrayLayer
			, layerCount );
	}

	crg::ImageViewData createView( std::string name
		, crg::ImageId image
		, crg::PixelFormat format
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
			, getViewType( crg::convert( image.data->info.imageType ), image.data->info.flags, layerCount )
			, format
			, { aspect, baseMipLevel, levelCount, baseArrayLayer, layerCount } };
	}

	crg::GraphContext & getDummyContext()
	{
		static VkPhysicalDeviceMemoryProperties const MemoryProperties = []()
			{
				VkPhysicalDeviceMemoryProperties result{};

				// Emulate one device local heap
				result.memoryHeaps[result.memoryHeapCount++] = { ~0ULL, VK_MEMORY_HEAP_DEVICE_LOCAL_BIT };
				// and one host visible heap
				result.memoryHeaps[result.memoryHeapCount++] = { ~0ULL, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT };

				// Emulate few combinations of device local memory types
				result.memoryTypes[result.memoryTypeCount++] = { VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0u };
				result.memoryTypes[result.memoryTypeCount++] = { VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 1u };
				result.memoryTypes[result.memoryTypeCount++] = { VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT, 1u };

				return result;
			}();
		static VkPhysicalDeviceProperties const Properties = []()
			{
				std::string_view name{ "Test" };

				VkPhysicalDeviceProperties result{};
#if defined( _MSC_VER )
				strncpy_s( result.deviceName
					, name.data()
					, std::min( name.size() + 1u, size_t( VK_MAX_PHYSICAL_DEVICE_NAME_SIZE - 1u ) ) );
#else
				strncpy( result.deviceName
					, name.data()
					, std::min( name.size() + 1u, size_t( VK_MAX_PHYSICAL_DEVICE_NAME_SIZE - 1u ) ) );
#endif
				result.deviceID = 0u;
				result.vendorID = 0u;
				result.driverVersion = VK_MAKE_VERSION( 1, 0, 0 );
				result.apiVersion = VK_MAKE_VERSION( 1, 0, 0 );
				result.deviceType = VK_PHYSICAL_DEVICE_TYPE_CPU;

				result.limits.maxImageDimension1D = 16384u;
				result.limits.maxImageDimension2D = 16384u;
				result.limits.maxImageDimension3D = 2048u;
				result.limits.maxImageDimensionCube = 16384u;
				result.limits.maxImageArrayLayers = 2048u;
				result.limits.maxTexelBufferElements = 134217728u;
				result.limits.maxUniformBufferRange = 65536u;
				result.limits.maxStorageBufferRange = 4294967295u;
				result.limits.maxPushConstantsSize = 256u;
				result.limits.maxMemoryAllocationCount = 4096u;
				result.limits.maxSamplerAllocationCount = 4000u;
				result.limits.bufferImageGranularity = 1024u;
				result.limits.sparseAddressSpaceSize = 18446744073709551615u;
				result.limits.maxBoundDescriptorSets = 8u;
				result.limits.maxPerStageDescriptorSamplers = 1048576u;
				result.limits.maxPerStageDescriptorUniformBuffers = 15u;
				result.limits.maxPerStageDescriptorStorageBuffers = 1048576u;
				result.limits.maxPerStageDescriptorSampledImages = 1048576u;
				result.limits.maxPerStageDescriptorStorageImages = 1048576u;
				result.limits.maxPerStageDescriptorInputAttachments = 1048576u;
				result.limits.maxPerStageResources = 4294967295u;
				result.limits.maxDescriptorSetSamplers = 1048576u;
				result.limits.maxDescriptorSetUniformBuffers = 90u;
				result.limits.maxDescriptorSetUniformBuffersDynamic = 15u;
				result.limits.maxDescriptorSetStorageBuffers = 1048576u;
				result.limits.maxDescriptorSetStorageBuffersDynamic = 16u;
				result.limits.maxDescriptorSetSampledImages = 1048576u;
				result.limits.maxDescriptorSetStorageImages = 1048576u;
				result.limits.maxDescriptorSetInputAttachments = 1048576u;
				result.limits.maxVertexInputAttributes = 32u;
				result.limits.maxVertexInputBindings = 32u;
				result.limits.maxVertexInputAttributeOffset = 2047u;
				result.limits.maxVertexInputBindingStride = 2048u;
				result.limits.maxVertexOutputComponents = 128u;
				result.limits.maxTessellationGenerationLevel = 64u;
				result.limits.maxTessellationPatchSize = 32u;
				result.limits.maxTessellationControlPerVertexInputComponents = 128u;
				result.limits.maxTessellationControlPerVertexOutputComponents = 128u;
				result.limits.maxTessellationControlPerPatchOutputComponents = 120u;
				result.limits.maxTessellationControlTotalOutputComponents = 4216u;
				result.limits.maxTessellationEvaluationInputComponents = 128u;
				result.limits.maxTessellationEvaluationOutputComponents = 128u;
				result.limits.maxGeometryShaderInvocations = 32u;
				result.limits.maxGeometryInputComponents = 128u;
				result.limits.maxGeometryOutputComponents = 128u;
				result.limits.maxGeometryOutputVertices = 1024u;
				result.limits.maxGeometryTotalOutputComponents = 1024u;
				result.limits.maxFragmentInputComponents = 128u;
				result.limits.maxFragmentOutputAttachments = 8u;
				result.limits.maxFragmentDualSrcAttachments = 1u;
				result.limits.maxFragmentCombinedOutputResources = 16u;
				result.limits.maxComputeSharedMemorySize = 49152u;
				result.limits.maxComputeWorkGroupCount[0] = 2147483647u;
				result.limits.maxComputeWorkGroupCount[1] = 65535u;
				result.limits.maxComputeWorkGroupCount[2] = 65535u;
				result.limits.maxComputeWorkGroupInvocations = 1536u;
				result.limits.maxComputeWorkGroupSize[0] = 1536u;
				result.limits.maxComputeWorkGroupSize[1] = 1024u;
				result.limits.maxComputeWorkGroupSize[2] = 64u;
				result.limits.subPixelPrecisionBits = 8u;
				result.limits.subTexelPrecisionBits = 8u;
				result.limits.mipmapPrecisionBits = 8u;
				result.limits.maxDrawIndexedIndexValue = 4294967295u;
				result.limits.maxDrawIndirectCount = 4294967295u;
				result.limits.maxSamplerLodBias = 15.0f;
				result.limits.maxSamplerAnisotropy = 16.0f;
				result.limits.maxViewports = 16u;
				result.limits.maxViewportDimensions[0] = 16384u;
				result.limits.maxViewportDimensions[1] = 16384u;
				result.limits.viewportBoundsRange[0] = -32768.0f;
				result.limits.viewportBoundsRange[1] = 32768.0f;
				result.limits.viewportSubPixelBits = 8u;
				result.limits.minMemoryMapAlignment = 64u;
				result.limits.minTexelBufferOffsetAlignment = 16u;
				result.limits.minUniformBufferOffsetAlignment = 256u;
				result.limits.minStorageBufferOffsetAlignment = 32u;
				result.limits.minTexelOffset = -8;
				result.limits.maxTexelOffset = 7u;
				result.limits.minTexelGatherOffset = -32;
				result.limits.maxTexelGatherOffset = 31u;
				result.limits.minInterpolationOffset = -0.5f;
				result.limits.maxInterpolationOffset = 0.4375f;
				result.limits.subPixelInterpolationOffsetBits = 4u;
				result.limits.maxFramebufferWidth = 16384u;
				result.limits.maxFramebufferHeight = 16384u;
				result.limits.maxFramebufferLayers = 2048u;
				result.limits.framebufferColorSampleCounts = 15u;
				result.limits.framebufferDepthSampleCounts = 15u;
				result.limits.framebufferStencilSampleCounts = 31u;
				result.limits.framebufferNoAttachmentsSampleCounts = 31u;
				result.limits.maxColorAttachments = 8u;
				result.limits.sampledImageColorSampleCounts = 15u;
				result.limits.sampledImageIntegerSampleCounts = 15u;
				result.limits.sampledImageDepthSampleCounts = 15u;
				result.limits.sampledImageStencilSampleCounts = 31u;
				result.limits.storageImageSampleCounts = 15u;
				result.limits.maxSampleMaskWords = 1u;
				result.limits.timestampComputeAndGraphics = true;
				result.limits.timestampPeriod = 1.0f;
				result.limits.maxClipDistances = 8u;
				result.limits.maxCullDistances = 8u;
				result.limits.maxCombinedClipAndCullDistances = 8u;
				result.limits.discreteQueuePriorities = 2u;
				result.limits.pointSizeRange[0] = 1.0f;
				result.limits.pointSizeRange[1] = 189.875f;
				result.limits.lineWidthRange[0] = 0.5f;
				result.limits.lineWidthRange[1] = 10.0f;
				result.limits.pointSizeGranularity = 0.125f;
				result.limits.lineWidthGranularity = 0.125f;
				result.limits.strictLines = true;
				result.limits.standardSampleLocations = true;
				result.limits.optimalBufferCopyOffsetAlignment = 1u;
				result.limits.optimalBufferCopyRowPitchAlignment = 1u;
				result.limits.nonCoherentAtomSize = 64ULL;

				result.sparseProperties.residencyAlignedMipSize = true;
				result.sparseProperties.residencyNonResidentStrict = true;
				result.sparseProperties.residencyStandard2DBlockShape = true;
				result.sparseProperties.residencyStandard2DMultisampleBlockShape = true;
				result.sparseProperties.residencyStandard3DBlockShape = true;

				return result;
			}();
		static crg::GraphContext context{ nullptr
			, nullptr
			, nullptr
			, MemoryProperties
			, Properties
			, false
			, nullptr };
		static std::atomic< uintptr_t > counter{};
		context.device = VkDevice( counter.load() );
		++counter;
		context.vkCreateGraphicsPipelines = PFN_vkCreateGraphicsPipelines( []( VkDevice, VkPipelineCache, uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo *, const VkAllocationCallbacks *, VkPipeline * pPipelines )
			{
				for ( uint32_t i = 0u; i < createInfoCount; ++i )
				{
					pPipelines[i] = VkPipeline( counter.load() );
					++counter;
				}
				return VK_SUCCESS;
			} );
		context.vkCreateComputePipelines = PFN_vkCreateComputePipelines( []( VkDevice, VkPipelineCache, uint32_t createInfoCount, const VkComputePipelineCreateInfo *, const VkAllocationCallbacks *, VkPipeline * pPipelines )
			{
				for ( uint32_t i = 0u; i < createInfoCount; ++i )
				{
					pPipelines[i] = VkPipeline( counter.load() );
					++counter;
				}
				return VK_SUCCESS;
			} );
		context.vkCreatePipelineLayout = PFN_vkCreatePipelineLayout( []( VkDevice, const VkPipelineLayoutCreateInfo *, const VkAllocationCallbacks *, VkPipelineLayout * pPipelineLayout )
			{
				*pPipelineLayout = VkPipelineLayout( counter.load() );
				++counter;
				return VK_SUCCESS;
			} );
		context.vkCreateDescriptorSetLayout = PFN_vkCreateDescriptorSetLayout( []( VkDevice, const VkDescriptorSetLayoutCreateInfo *, const VkAllocationCallbacks *, VkDescriptorSetLayout * pSetLayout )
			{
				*pSetLayout = VkDescriptorSetLayout( counter.load() );
				++counter;
				return VK_SUCCESS;
			} );
		context.vkCreateDescriptorPool = PFN_vkCreateDescriptorPool( []( VkDevice, const VkDescriptorPoolCreateInfo *, const VkAllocationCallbacks *, VkDescriptorPool * pDescriptorPool )
			{
				*pDescriptorPool = VkDescriptorPool( counter.load() );
				++counter;
				return VK_SUCCESS;
			} );
		context.vkAllocateDescriptorSets = PFN_vkAllocateDescriptorSets( []( VkDevice, const VkDescriptorSetAllocateInfo * pAllocateInfo, VkDescriptorSet * pDescriptorSets )
			{
				if ( !pAllocateInfo )
					return VK_ERROR_UNKNOWN;

				for ( uint32_t i = 0u; i < pAllocateInfo->descriptorSetCount; ++i )
				{
					pDescriptorSets[i] = VkDescriptorSet( counter.load() );
					++counter;
				}
				return VK_SUCCESS;
			} );
		context.vkCreateBuffer = PFN_vkCreateBuffer( []( VkDevice, const VkBufferCreateInfo *, const VkAllocationCallbacks *, VkBuffer * pBuffer )
			{
				*pBuffer = VkBuffer( counter.load() );
				++counter;
				return VK_SUCCESS;
			} );
		context.vkAllocateMemory = PFN_vkAllocateMemory( []( VkDevice, const VkMemoryAllocateInfo *, const VkAllocationCallbacks *, VkDeviceMemory * pMemory )
			{
				*pMemory = VkDeviceMemory( counter.load() );
				++counter;
				return VK_SUCCESS;
			} );
		context.vkCreateRenderPass = PFN_vkCreateRenderPass( []( VkDevice, const VkRenderPassCreateInfo *, const VkAllocationCallbacks *, VkRenderPass * pRenderPass )
			{
				*pRenderPass = VkRenderPass( counter.load() );
				++counter;
				return VK_SUCCESS;
			} );
		context.vkCreateFramebuffer = PFN_vkCreateFramebuffer( []( VkDevice, const VkFramebufferCreateInfo *, const VkAllocationCallbacks *, VkFramebuffer * pFramebuffer )
			{
				*pFramebuffer = VkFramebuffer( counter.load() );
				++counter;
				return VK_SUCCESS;
			} );
		context.vkCreateImage = PFN_vkCreateImage( []( VkDevice, const VkImageCreateInfo *, const VkAllocationCallbacks *, VkImage * pImage )
			{
				*pImage = VkImage( counter.load() );
				++counter;
				return VK_SUCCESS;
			} );
		context.vkCreateImageView = PFN_vkCreateImageView( []( VkDevice, const VkImageViewCreateInfo *, const VkAllocationCallbacks *, VkImageView * pView )
			{
				*pView = VkImageView( counter.load() );
				++counter;
				return VK_SUCCESS;
			} );
		context.vkCreateSampler = PFN_vkCreateSampler( []( VkDevice, const VkSamplerCreateInfo *, const VkAllocationCallbacks *, VkSampler * pSampler )
			{
				*pSampler = VkSampler( counter.load() );
				++counter;
				return VK_SUCCESS;
			} );
		context.vkCreateCommandPool = PFN_vkCreateCommandPool( []( VkDevice, const VkCommandPoolCreateInfo *, const VkAllocationCallbacks *, VkCommandPool * pCommandPool )
			{
				*pCommandPool = VkCommandPool( counter.load() );
				++counter;
				return VK_SUCCESS;
			} );
		context.vkAllocateCommandBuffers = PFN_vkAllocateCommandBuffers( []( VkDevice, const VkCommandBufferAllocateInfo * pAllocateInfo, VkCommandBuffer * pCommandBuffers )
			{
				if ( !pAllocateInfo )
					return VK_ERROR_UNKNOWN;

				for ( uint32_t i = 0u; i < pAllocateInfo->commandBufferCount; ++i )
				{
					pCommandBuffers[i] = VkCommandBuffer( counter.load() );
					++counter;
				}
				return VK_SUCCESS;
			} );
		context.vkCreateSemaphore = PFN_vkCreateSemaphore( []( VkDevice, const VkSemaphoreCreateInfo *, const VkAllocationCallbacks *, VkSemaphore * pSemaphore )
			{
				*pSemaphore = VkSemaphore( counter.load() );
				++counter;
				return VK_SUCCESS;
			} );
		context.vkCreateQueryPool = PFN_vkCreateQueryPool( []( VkDevice, const VkQueryPoolCreateInfo *, const VkAllocationCallbacks *, VkQueryPool * pQueryPool )
			{
				*pQueryPool = VkQueryPool( counter.load() );
				++counter;
				return VK_SUCCESS;
			} );
		context.vkCreateEvent = PFN_vkCreateEvent( []( VkDevice, const VkEventCreateInfo *, const VkAllocationCallbacks *, VkEvent * pEvent )
			{
				*pEvent = VkEvent( counter.load() );
				++counter;
				return VK_SUCCESS;
			} );
		context.vkCreateFence = PFN_vkCreateFence( []( VkDevice, const VkFenceCreateInfo *, const VkAllocationCallbacks *, VkFence * pFence )
			{
				*pFence = VkFence( counter.load() );
				++counter;
				return VK_SUCCESS;
			} );

		context.vkDestroyPipeline = PFN_vkDestroyPipeline( []( VkDevice, VkPipeline, const VkAllocationCallbacks * ){} );
		context.vkDestroyPipelineLayout = PFN_vkDestroyPipelineLayout( []( VkDevice, VkPipelineLayout, const VkAllocationCallbacks * ){} );
		context.vkDestroyDescriptorSetLayout = PFN_vkDestroyDescriptorSetLayout( []( VkDevice, VkDescriptorSetLayout, const VkAllocationCallbacks* ){} );
		context.vkDestroyDescriptorPool = PFN_vkDestroyDescriptorPool( []( VkDevice, VkDescriptorPool, const VkAllocationCallbacks * ){} );
		context.vkFreeDescriptorSets = PFN_vkFreeDescriptorSets( []( VkDevice, VkDescriptorPool, uint32_t, const VkDescriptorSet * ){ return VK_SUCCESS; } );
		context.vkDestroyBuffer = PFN_vkDestroyBuffer( []( VkDevice, VkBuffer, const VkAllocationCallbacks * ){} );
		context.vkFreeMemory = PFN_vkFreeMemory( []( VkDevice, VkDeviceMemory, const VkAllocationCallbacks * ){} );
		context.vkDestroyRenderPass = PFN_vkDestroyRenderPass( []( VkDevice, VkRenderPass, const VkAllocationCallbacks * ){} );
		context.vkDestroyFramebuffer = PFN_vkDestroyFramebuffer( []( VkDevice, VkFramebuffer, const VkAllocationCallbacks * ){} );
		context.vkDestroyImage = PFN_vkDestroyImage( []( VkDevice, VkImage, const VkAllocationCallbacks * ){} );
		context.vkDestroyImageView = PFN_vkDestroyImageView( []( VkDevice, VkImageView, const VkAllocationCallbacks * ){} );
		context.vkDestroySampler = PFN_vkDestroySampler( []( VkDevice, VkSampler, const VkAllocationCallbacks * ){} );
		context.vkDestroyCommandPool = PFN_vkDestroyCommandPool( []( VkDevice, VkCommandPool, const VkAllocationCallbacks * ){} );
		context.vkFreeCommandBuffers = PFN_vkFreeCommandBuffers( []( VkDevice, VkCommandPool, uint32_t, const VkCommandBuffer * ){} );
		context.vkDestroySemaphore = PFN_vkDestroySemaphore( []( VkDevice, VkSemaphore, const VkAllocationCallbacks * ){} );
		context.vkDestroyQueryPool = PFN_vkDestroyQueryPool( []( VkDevice, VkQueryPool, const VkAllocationCallbacks * ){} );
		context.vkDestroyEvent = PFN_vkDestroyEvent( []( VkDevice, VkEvent, const VkAllocationCallbacks * ){} );
		context.vkDestroyFence = PFN_vkDestroyFence( []( VkDevice, VkFence, const VkAllocationCallbacks * ){} );

		context.vkGetBufferMemoryRequirements = PFN_vkGetBufferMemoryRequirements( []( VkDevice, VkBuffer, VkMemoryRequirements * pMemoryRequirements )
			{
				pMemoryRequirements->memoryTypeBits = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
				pMemoryRequirements->size = 1024u;
				pMemoryRequirements->alignment = 1u;
			} );
		context.vkGetImageMemoryRequirements = PFN_vkGetImageMemoryRequirements( []( VkDevice, VkImage, VkMemoryRequirements * pMemoryRequirements )
			{
				pMemoryRequirements->memoryTypeBits = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
				pMemoryRequirements->size = 1024u * 1024 * 64;
				pMemoryRequirements->alignment = 4u;
			} );
		context.vkBindBufferMemory = PFN_vkBindBufferMemory( []( VkDevice, VkBuffer, VkDeviceMemory, VkDeviceSize ){ return VK_SUCCESS; } );
		context.vkBindImageMemory = PFN_vkBindImageMemory( []( VkDevice, VkImage, VkDeviceMemory, VkDeviceSize ){ return VK_SUCCESS; } );
		context.vkMapMemory = PFN_vkMapMemory( []( VkDevice, VkDeviceMemory, VkDeviceSize, VkDeviceSize, VkMemoryMapFlags, void ** ppData )
			{
				thread_local std::vector< uint8_t > data( 1024u * 1024u * 64u );
				*ppData = data.data();
				return VK_SUCCESS;
			} );
		context.vkUnmapMemory = PFN_vkUnmapMemory( []( VkDevice, VkDeviceMemory ){} );
		context.vkFlushMappedMemoryRanges = PFN_vkFlushMappedMemoryRanges( []( VkDevice, uint32_t , const VkMappedMemoryRange * ){ return VK_SUCCESS; } );
		context.vkInvalidateMappedMemoryRanges = PFN_vkInvalidateMappedMemoryRanges( []( VkDevice, uint32_t , const VkMappedMemoryRange * ){ return VK_SUCCESS; } );
		context.vkUpdateDescriptorSets = PFN_vkUpdateDescriptorSets( []( VkDevice, uint32_t, const VkWriteDescriptorSet *, uint32_t, const VkCopyDescriptorSet * ){} );
		context.vkBeginCommandBuffer = PFN_vkBeginCommandBuffer( []( VkCommandBuffer, const VkCommandBufferBeginInfo * ){ return VK_SUCCESS; } );
		context.vkEndCommandBuffer = PFN_vkEndCommandBuffer( []( VkCommandBuffer ){ return VK_SUCCESS; } );
		context.vkQueueSubmit = PFN_vkQueueSubmit( []( VkQueue, uint32_t, const VkSubmitInfo *, VkFence ){ return VK_SUCCESS; } );
		context.vkGetQueryPoolResults = PFN_vkGetQueryPoolResults( []( VkDevice, VkQueryPool, uint32_t, uint32_t, size_t, void *, VkDeviceSize, VkQueryResultFlags ){ return VK_SUCCESS; } );
		context.vkResetCommandBuffer = PFN_vkResetCommandBuffer( []( VkCommandBuffer, VkCommandBufferResetFlags ){ return VK_SUCCESS; } );
		context.vkResetEvent = PFN_vkResetEvent( []( VkDevice, VkEvent ){ return VK_SUCCESS; } );
		context.vkSetEvent = PFN_vkSetEvent( []( VkDevice, VkEvent ){ return VK_SUCCESS; } );
		context.vkGetEventStatus = PFN_vkGetEventStatus( []( VkDevice, VkEvent ){ return VK_SUCCESS; } );
		context.vkGetFenceStatus = PFN_vkGetFenceStatus( []( VkDevice, VkFence ){ return VK_SUCCESS; } );
		context.vkWaitForFences = PFN_vkWaitForFences( []( VkDevice, uint32_t, const VkFence *, VkBool32, uint64_t ){ return VK_SUCCESS; } );
		context.vkResetFences = PFN_vkResetFences( []( VkDevice, uint32_t, const VkFence * ){ return VK_SUCCESS; } );

		context.vkCmdBindPipeline = PFN_vkCmdBindPipeline( []( VkCommandBuffer, VkPipelineBindPoint, VkPipeline ){} );
		context.vkCmdBindDescriptorSets = PFN_vkCmdBindDescriptorSets( []( VkCommandBuffer, VkPipelineBindPoint, VkPipelineLayout, uint32_t, uint32_t, const VkDescriptorSet *, uint32_t, const uint32_t * ){} );
		context.vkCmdBindVertexBuffers = PFN_vkCmdBindVertexBuffers( []( VkCommandBuffer, uint32_t, uint32_t, const VkBuffer *, const VkDeviceSize * ){} );
		context.vkCmdBindIndexBuffer = PFN_vkCmdBindIndexBuffer( []( VkCommandBuffer, VkBuffer, VkDeviceSize, VkIndexType ){} );
		context.vkCmdClearColorImage = PFN_vkCmdClearColorImage( []( VkCommandBuffer, VkImage, VkImageLayout, const VkClearColorValue *, uint32_t, const VkImageSubresourceRange * ){} );
		context.vkCmdClearDepthStencilImage = PFN_vkCmdClearDepthStencilImage( []( VkCommandBuffer, VkImage, VkImageLayout, const VkClearDepthStencilValue *, uint32_t, const VkImageSubresourceRange * ){} );
		context.vkCmdDispatch = PFN_vkCmdDispatch( []( VkCommandBuffer, uint32_t, uint32_t, uint32_t ){} );
		context.vkCmdDispatchIndirect = PFN_vkCmdDispatchIndirect( []( VkCommandBuffer, VkBuffer, VkDeviceSize ){} );
		context.vkCmdDraw = PFN_vkCmdDraw( []( VkCommandBuffer, uint32_t, uint32_t, uint32_t, uint32_t ){} );
		context.vkCmdDrawIndexed = PFN_vkCmdDrawIndexed( []( VkCommandBuffer, uint32_t, uint32_t, uint32_t, int32_t, uint32_t ){} );
		context.vkCmdDrawIndexedIndirect = PFN_vkCmdDrawIndexedIndirect( []( VkCommandBuffer, VkBuffer, VkDeviceSize, uint32_t, uint32_t ){} );
		context.vkCmdDrawIndirect = PFN_vkCmdDrawIndirect( []( VkCommandBuffer, VkBuffer, VkDeviceSize, uint32_t, uint32_t ){} );
		context.vkCmdBeginRenderPass = PFN_vkCmdBeginRenderPass( []( VkCommandBuffer, const VkRenderPassBeginInfo *, VkSubpassContents ){} );
		context.vkCmdEndRenderPass = PFN_vkCmdEndRenderPass( []( VkCommandBuffer ){} );
		context.vkCmdPushConstants = PFN_vkCmdPushConstants( []( VkCommandBuffer, VkPipelineLayout, VkShaderStageFlags, uint32_t, uint32_t, const void * ){} );
		context.vkCmdResetQueryPool = PFN_vkCmdResetQueryPool( []( VkCommandBuffer, VkQueryPool, uint32_t, uint32_t ){} );
		context.vkCmdWriteTimestamp = PFN_vkCmdWriteTimestamp( []( VkCommandBuffer, VkPipelineStageFlagBits, VkQueryPool, uint32_t ){} );
		context.vkCmdPipelineBarrier = PFN_vkCmdPipelineBarrier( []( VkCommandBuffer, VkPipelineStageFlags, VkPipelineStageFlags, VkDependencyFlags, uint32_t, const VkMemoryBarrier *, uint32_t, const VkBufferMemoryBarrier *, uint32_t, const VkImageMemoryBarrier * ){} );
		context.vkCmdBlitImage = PFN_vkCmdBlitImage( []( VkCommandBuffer, VkImage, VkImageLayout, VkImage, VkImageLayout, uint32_t, const VkImageBlit *, VkFilter ){} );
		context.vkCmdCopyBuffer = PFN_vkCmdCopyBuffer( []( VkCommandBuffer, VkBuffer, VkBuffer, uint32_t, const VkBufferCopy * ){} );
		context.vkCmdCopyBufferToImage = PFN_vkCmdCopyBufferToImage( []( VkCommandBuffer, VkBuffer, VkImage, VkImageLayout, uint32_t, const VkBufferImageCopy * ){} );
		context.vkCmdCopyImage = PFN_vkCmdCopyImage( []( VkCommandBuffer, VkImage, VkImageLayout, VkImage, VkImageLayout, uint32_t, const VkImageCopy * ){} );
		context.vkCmdCopyImageToBuffer = PFN_vkCmdCopyImageToBuffer( []( VkCommandBuffer, VkImage, VkImageLayout, VkBuffer, uint32_t, const VkBufferImageCopy * ){} );
		context.vkCmdExecuteCommands = PFN_vkCmdExecuteCommands( []( VkCommandBuffer, uint32_t, const VkCommandBuffer * ){} );
		context.vkCmdResetEvent = PFN_vkCmdResetEvent( []( VkCommandBuffer, VkEvent, VkPipelineStageFlags ){} );
		context.vkCmdSetEvent = PFN_vkCmdSetEvent( []( VkCommandBuffer, VkEvent, VkPipelineStageFlags ){} );
		context.vkCmdWaitEvents = PFN_vkCmdWaitEvents( []( VkCommandBuffer, uint32_t, const VkEvent *, VkPipelineStageFlags, VkPipelineStageFlags, uint32_t, const VkMemoryBarrier *, uint32_t, const VkBufferMemoryBarrier *, uint32_t, const VkImageMemoryBarrier * ){} );
		context.vkCmdFillBuffer = PFN_vkCmdFillBuffer( []( VkCommandBuffer, VkBuffer, VkDeviceSize, VkDeviceSize, uint32_t ){} );

#if VK_EXT_debug_utils || VK_EXT_debug_marker
#	if VK_EXT_debug_utils
		context.vkSetDebugUtilsObjectNameEXT = PFN_vkSetDebugUtilsObjectNameEXT( []( VkDevice, const VkDebugUtilsObjectNameInfoEXT* ){ return VK_SUCCESS; } );
		context.vkCmdBeginDebugUtilsLabelEXT = PFN_vkCmdBeginDebugUtilsLabelEXT( []( VkCommandBuffer, const VkDebugUtilsLabelEXT * ){} );
		context.vkCmdEndDebugUtilsLabelEXT = PFN_vkCmdEndDebugUtilsLabelEXT( []( VkCommandBuffer ){} );
#	endif
#	if VK_EXT_debug_marker
		context.vkDebugMarkerSetObjectNameEXT = PFN_vkDebugMarkerSetObjectNameEXT( []( VkDevice, const VkDebugMarkerObjectNameInfoEXT * ){ return VK_SUCCESS; } );
		context.vkCmdDebugMarkerBeginEXT = PFN_vkCmdDebugMarkerBeginEXT( []( VkCommandBuffer, const VkDebugMarkerMarkerInfoEXT * ){} );
		context.vkCmdDebugMarkerEndEXT = PFN_vkCmdDebugMarkerEndEXT( []( VkCommandBuffer ){} );
#	endif
#endif
		context.setCallstackCallback( []()
			{
				return "coin !!";
			} );
		return context;
	}

	std::stringstream checkRunnable( TestCounts & testCounts
		, crg::RunnableGraph * runnable )
	{
		require( runnable )
		std::stringstream stream;
		test::display( testCounts, stream, *runnable );
		checkNoThrow( runnable->record() )
		checkNoThrow( runnable->run( VkQueue{} ) )
		return stream;
	}

	void display( TestCounts const & testCounts
		, std::ostream & stream
		, crg::RunnableGraph const & value
		, bool withColours
		, bool withIds
		, bool withGroups )
	{
		std::stringstream tmp;
		crg::dot::displayTransitions( tmp, value, { true, true, true, true } );
		crg::dot::displayTransitions( tmp, value, { true, true, true, false } );
		crg::dot::displayTransitions( tmp, value, { true, true, false, true } );
		crg::dot::displayTransitions( tmp, value, { true, true, false, false } );
		crg::dot::displayTransitions( tmp, value, { true, false, true, true } );
		crg::dot::displayTransitions( tmp, value, { true, false, true, false } );
		crg::dot::displayTransitions( tmp, value, { true, false, false, true } );
		crg::dot::displayTransitions( tmp, value, { true, false, false, false } );
		crg::dot::displayTransitions( tmp, value, { false, true, true, true } );
		crg::dot::displayTransitions( tmp, value, { false, true, true, false } );
		crg::dot::displayTransitions( tmp, value, { false, true, false, true } );
		crg::dot::displayTransitions( tmp, value, { false, true, false, false } );
		crg::dot::displayTransitions( tmp, value, { false, false, true, true } );
		crg::dot::displayTransitions( tmp, value, { false, false, true, false } );
		crg::dot::displayTransitions( tmp, value, { false, false, false, true } );
		crg::dot::displayTransitions( tmp, value, { false, false, false, false } );
		crg::dot::displayPasses( tmp, value, { true, true, true, true } );
		crg::dot::displayPasses( tmp, value, { true, true, true, false } );
		crg::dot::displayPasses( tmp, value, { true, true, false, true } );
		crg::dot::displayPasses( tmp, value, { true, true, false, false } );
		crg::dot::displayPasses( tmp, value, { true, false, true, true } );
		crg::dot::displayPasses( tmp, value, { true, false, true, false } );
		crg::dot::displayPasses( tmp, value, { true, false, false, true } );
		crg::dot::displayPasses( tmp, value, { true, false, false, false } );
		crg::dot::displayPasses( tmp, value, { false, true, true, true } );
		crg::dot::displayPasses( tmp, value, { false, true, true, false } );
		crg::dot::displayPasses( tmp, value, { false, true, false, true } );
		crg::dot::displayPasses( tmp, value, { false, true, false, false } );
		crg::dot::displayPasses( tmp, value, { false, false, true, true } );
		crg::dot::displayPasses( tmp, value, { false, false, true, false } );
		crg::dot::displayPasses( tmp, value, { false, false, false, true } );
		crg::dot::displayPasses( tmp, value, { false, false, false, false } );

		displayTransitions( testCounts, tmp, value, { withColours, withIds, withGroups } );
		displayPasses( testCounts, stream, value, { withColours, withIds, withGroups } );
	}

	void display( TestCounts const & testCounts
		, crg::RunnableGraph const & value
		, bool withColours
		, bool withIds
		, bool withGroups )
	{
		display( testCounts, std::cout, value, withColours, withIds, withGroups );
	}

	class DummyRunnable
		: public crg::RunnablePass
	{
	public:
		DummyRunnable( crg::FramePass const & framePass
			, crg::GraphContext & context
			, crg::RunnableGraph & runGraph
			, test::TestCounts & testCounts
			, crg::PipelineStageFlags pipelineStageFlags
			, CheckViews checkViews
			, uint32_t index
			, bool enabled
			, crg::ru::Config config )
			: crg::RunnablePass{ framePass
				, context
				, runGraph
				, { crg::defaultV< crg::RunnablePass::InitialiseCallback >
					, crg::RunnablePass::GetPipelineStateCallback( [this](){ return crg::getPipelineState( m_pipelineStageFlags ); } )
					, crg::RunnablePass::RecordCallback( [this]( crg::RecordContext & ctx, VkCommandBuffer, uint32_t i ){ doRecordInto( ctx, i ); } )
					, crg::RunnablePass::GetPassIndexCallback( [index](){ return index; } )
					, crg::RunnablePass::IsEnabledCallback( [enabled](){ return enabled; } ) }
				, std::move( config ) }
			, m_testCounts{ testCounts }
			, m_pipelineStageFlags{ pipelineStageFlags }
			, m_checkViews{ std::move( checkViews ) }
		{
		}

		DummyRunnable( crg::FramePass const & framePass
			, crg::GraphContext & context
			, crg::RunnableGraph & runGraph
			, test::TestCounts & testCounts
			, crg::PipelineStageFlags pipelineStageFlags
			, CheckViews checkViews
			, uint32_t index
			, crg::ru::Config config )
			: crg::RunnablePass{ framePass
				, context
				, runGraph
				, { crg::defaultV< crg::RunnablePass::InitialiseCallback >
					, crg::RunnablePass::GetPipelineStateCallback( [this](){ return crg::getPipelineState( m_pipelineStageFlags ); } )
					, crg::RunnablePass::RecordCallback( [this]( crg::RecordContext & ctx, VkCommandBuffer, uint32_t i ){ doRecordInto( ctx, i ); } )
					, crg::RunnablePass::GetPassIndexCallback( [index](){ return index; } ) }
				, std::move( config ) }
			, m_testCounts{ testCounts }
			, m_pipelineStageFlags{ pipelineStageFlags }
			, m_checkViews{ std::move( checkViews ) }
		{
		}

		DummyRunnable( crg::FramePass const & framePass
			, crg::GraphContext & context
			, crg::RunnableGraph & runGraph
			, test::TestCounts & testCounts
			, crg::PipelineStageFlags pipelineStageFlags
			, CheckViews checkViews
			, crg::ru::Config config )
			: crg::RunnablePass{ framePass
				, context
				, runGraph
				, { crg::defaultV< crg::RunnablePass::InitialiseCallback >
					, crg::RunnablePass::GetPipelineStateCallback( [this](){ return crg::getPipelineState( m_pipelineStageFlags ); } )
					, crg::RunnablePass::RecordCallback( [this]( crg::RecordContext & ctx, VkCommandBuffer, uint32_t i ){ doRecordInto( ctx, i ); } ) }
				, std::move( config ) }
			, m_testCounts{ testCounts }
			, m_pipelineStageFlags{ pipelineStageFlags }
			, m_checkViews{ std::move( checkViews ) }
		{
		}

		DummyRunnable( crg::FramePass const & framePass
			, crg::GraphContext & context
			, crg::RunnableGraph & runGraph
			, test::TestCounts & testCounts
			, crg::PipelineStageFlags pipelineStageFlags
			, crg::ru::Config config )
			: crg::RunnablePass{ framePass
				, context
				, runGraph
				, { crg::defaultV< crg::RunnablePass::InitialiseCallback >
					, crg::RunnablePass::GetPipelineStateCallback( [this](){ return crg::getPipelineState( m_pipelineStageFlags ); } ) }
				, std::move( config ) }
			, m_testCounts{ testCounts }
			, m_pipelineStageFlags{ pipelineStageFlags }
		{
		}

	private:
		void doRecordInto( crg::RecordContext & context
			, uint32_t index )
		{
			for ( auto & attach : m_pass.images )
			{
				auto view = attach.view( index );
				context.setLayoutState( crg::resolveView( view, index )
					, crg::makeLayoutState( attach.getImageLayout( m_context.separateDepthStencilLayouts ) ) );
			}

			m_checkViews( m_testCounts
				, m_pass
				, m_graph
				, context
				, index );
		}

		test::TestCounts & m_testCounts;
		crg::PipelineStageFlags m_pipelineStageFlags;
		CheckViews m_checkViews;
	};

	crg::RunnablePassPtr createDummy( test::TestCounts & testCounts
		, crg::FramePass const & framePass
		, crg::GraphContext & context
		, crg::RunnableGraph & runGraph
		, crg::PipelineStageFlags pipelineStageFlags
		, CheckViews checkViews
		, uint32_t index
		, bool enabled
		, crg::ru::Config config )
	{
		return std::make_unique< DummyRunnable >( framePass
			, context
			, runGraph
			, testCounts
			, pipelineStageFlags
			, checkViews
			, index
			, enabled
			, std::move( config ) );
	}

	crg::RunnablePassPtr createDummy( test::TestCounts & testCounts
		, crg::FramePass const & framePass
		, crg::GraphContext & context
		, crg::RunnableGraph & runGraph
		, crg::PipelineStageFlags pipelineStageFlags
		, CheckViews checkViews
		, uint32_t index
		, crg::ru::Config config )
	{
		return std::make_unique< DummyRunnable >( framePass
			, context
			, runGraph
			, testCounts
			, pipelineStageFlags
			, checkViews
			, index
			, std::move( config ) );
	}

	crg::RunnablePassPtr createDummy( test::TestCounts & testCounts
		, crg::FramePass const & framePass
		, crg::GraphContext & context
		, crg::RunnableGraph & runGraph
		, crg::PipelineStageFlags pipelineStageFlags
		, CheckViews checkViews
		, crg::ru::Config config )
	{
		return std::make_unique< DummyRunnable >( framePass
			, context
			, runGraph
			, testCounts
			, pipelineStageFlags
			, std::move( checkViews )
			, std::move( config ) );
	}

	crg::RunnablePassPtr createDummy( test::TestCounts & testCounts
		, crg::FramePass const & framePass
		, crg::GraphContext & context
		, crg::RunnableGraph & runGraph
		, crg::PipelineStageFlags pipelineStageFlags
		, crg::ru::Config config )
	{
		return std::make_unique< DummyRunnable >( framePass
			, context
			, runGraph
			, testCounts
			, pipelineStageFlags
			, std::move( config ) );
	}

	void checkDummy( [[maybe_unused]] test::TestCounts & testCounts
		, [[maybe_unused]] crg::FramePass const & framePass
		, [[maybe_unused]] crg::RunnableGraph const & graph
		, [[maybe_unused]] crg::RecordContext const & context
		, [[maybe_unused]] uint32_t index )
	{
		// Nothing checked yet...
	}
}
