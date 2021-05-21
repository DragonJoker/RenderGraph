/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/GraphContext.hpp"
#include "RenderGraph/CallStack.hpp"

#include <cassert>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <sstream>

namespace crg
{
	GraphContext::GraphContext( VkDevice device
		, VkPipelineCache cache
		, VkAllocationCallbacks const * allocator
		, VkPhysicalDeviceMemoryProperties memoryProperties
		, VkPhysicalDeviceProperties properties
		, bool separateDepthStencilLayouts
		, PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr )
		: device{ device }
		, cache{ cache }
		, allocator{ allocator }
		, memoryProperties{ std::move( memoryProperties ) }
		, properties{ std::move( properties ) }
		, separateDepthStencilLayouts{ separateDepthStencilLayouts }
	{
#define DECL_vkFunction( name )\
		vk##name = reinterpret_cast< PFN_vk##name >( vkGetDeviceProcAddr( device, "vk"#name ) )

		DECL_vkFunction( CreateGraphicsPipelines );
		DECL_vkFunction( CreateComputePipelines );
		DECL_vkFunction( DestroyPipeline );
		DECL_vkFunction( CreatePipelineLayout );
		DECL_vkFunction( DestroyPipelineLayout );
		DECL_vkFunction( CreateDescriptorSetLayout );
		DECL_vkFunction( DestroyDescriptorSetLayout );
		DECL_vkFunction( CreateDescriptorPool );
		DECL_vkFunction( DestroyDescriptorPool );
		DECL_vkFunction( AllocateDescriptorSets );
		DECL_vkFunction( FreeDescriptorSets );
		DECL_vkFunction( CreateBuffer );
		DECL_vkFunction( DestroyBuffer );
		DECL_vkFunction( GetBufferMemoryRequirements );
		DECL_vkFunction( GetImageMemoryRequirements );
		DECL_vkFunction( AllocateMemory );
		DECL_vkFunction( FreeMemory );
		DECL_vkFunction( BindBufferMemory );
		DECL_vkFunction( BindImageMemory );
		DECL_vkFunction( MapMemory );
		DECL_vkFunction( UnmapMemory );
		DECL_vkFunction( FlushMappedMemoryRanges );
		DECL_vkFunction( InvalidateMappedMemoryRanges );
		DECL_vkFunction( CreateRenderPass );
		DECL_vkFunction( DestroyRenderPass );
		DECL_vkFunction( CreateFramebuffer );
		DECL_vkFunction( DestroyFramebuffer );
		DECL_vkFunction( CreateImage );
		DECL_vkFunction( DestroyImage );
		DECL_vkFunction( CreateImageView );
		DECL_vkFunction( DestroyImageView );
		DECL_vkFunction( CreateSampler );
		DECL_vkFunction( DestroySampler );
		DECL_vkFunction( CreateCommandPool );
		DECL_vkFunction( DestroyCommandPool );
		DECL_vkFunction( AllocateCommandBuffers );
		DECL_vkFunction( FreeCommandBuffers );
		DECL_vkFunction( CreateSemaphore );
		DECL_vkFunction( DestroySemaphore );
		DECL_vkFunction( UpdateDescriptorSets );
		DECL_vkFunction( BeginCommandBuffer );
		DECL_vkFunction( EndCommandBuffer );
		DECL_vkFunction( QueueSubmit );
		DECL_vkFunction( CreateQueryPool );
		DECL_vkFunction( DestroyQueryPool );
		DECL_vkFunction( GetQueryPoolResults );

		DECL_vkFunction( CmdBindPipeline );
		DECL_vkFunction( CmdBindDescriptorSets );
		DECL_vkFunction( CmdBindVertexBuffers );
		DECL_vkFunction( CmdBindIndexBuffer );
		DECL_vkFunction( CmdDraw );
		DECL_vkFunction( CmdDrawIndexed );
		DECL_vkFunction( CmdBeginRenderPass );
		DECL_vkFunction( CmdEndRenderPass );
		DECL_vkFunction( CmdResetQueryPool );
		DECL_vkFunction( CmdWriteTimestamp );
		DECL_vkFunction( CmdExecuteCommands );

#if VK_EXT_debug_utils
		DECL_vkFunction( SetDebugUtilsObjectNameEXT );
		DECL_vkFunction( CmdBeginDebugUtilsLabelEXT );
		DECL_vkFunction( CmdEndDebugUtilsLabelEXT );
#endif
#if VK_EXT_debug_marker
		DECL_vkFunction( DebugMarkerSetObjectNameEXT );
		DECL_vkFunction( CmdDebugMarkerBeginEXT );
		DECL_vkFunction( CmdDebugMarkerEndEXT );
		DECL_vkFunction( CmdDebugMarkerInsertEXT );
#endif

#undef DECL_vkFunction
	}

#if VK_EXT_debug_utils || VK_EXT_debug_marker

	void GraphContext::vkCmdBeginDebugBlock( VkCommandBuffer commandBuffer
		, DebugBlockInfo const & labelInfo )const
	{
#if VK_EXT_debug_utils
		doBeginDebugUtilsLabel( commandBuffer
			, { VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT
				, nullptr
				, labelInfo.markerName.c_str()
				, { labelInfo.colour[0]
					, labelInfo.colour[1]
					, labelInfo.colour[2]
					, labelInfo.colour[3] } } );
#endif
#if VK_EXT_debug_marker
		doDebugMarkerBegin( commandBuffer
			, { VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT
				, nullptr
				, labelInfo.markerName.c_str()
				, { labelInfo.colour[0]
					, labelInfo.colour[1]
					, labelInfo.colour[2]
					, labelInfo.colour[3] } } );
#endif
	}

	void GraphContext::vkCmdEndDebugBlock( VkCommandBuffer commandBuffer )const
	{
#if VK_EXT_debug_utils
		doEndDebugUtilsLabel( commandBuffer );
#endif
#if VK_EXT_debug_marker
		doDebugMarkerEnd( commandBuffer );
#endif
	}

	void GraphContext::vkCmdInsertDebugBlock( VkCommandBuffer commandBuffer
		, DebugBlockInfo const & labelInfo )const
	{
#if VK_EXT_debug_utils
		doInsertDebugUtilsLabel( commandBuffer
			, { VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT
				, nullptr
				, labelInfo.markerName.c_str()
				, { labelInfo.colour[0]
					, labelInfo.colour[1]
					, labelInfo.colour[2]
					, labelInfo.colour[3] } } );
#endif
#if VK_EXT_debug_marker
		doDebugMarkerInsert( commandBuffer
			, { VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT
				, nullptr
				, labelInfo.markerName.c_str()
				, { labelInfo.colour[0]
					, labelInfo.colour[1]
					, labelInfo.colour[2]
					, labelInfo.colour[3] } } );
#endif
	}

#endif

	std::array< float, 4u > GraphContext::getNextRainbowColour()const
	{
		static float currentColourHue{ 0.0f };
		currentColourHue += 0.05f;

		if ( currentColourHue > 1.0f )
		{
			currentColourHue = 0.0f;
		}

		float brightness = 1.0f;
		float saturation = 1.0f;
		float h = currentColourHue == 1.0f ? 0 : currentColourHue * 6.0f;
		float f = h - ( int )h;
		float p = brightness * ( 1.0f - saturation );
		float q = brightness * ( 1.0f - saturation * f );
		float t = brightness * ( 1.0f - ( saturation * ( 1.0f - f ) ) );

		if ( h < 1 )
		{
			return { brightness, t, p, 1.0f };
		}

		if ( h < 2 )
		{
			return { q, brightness, p, 1.0f };
		}

		if ( h < 3 )
		{
			return { p, brightness, t, 1.0f };
		}

		if ( h < 4 )
		{
			return { p, q, brightness, 1.0f };
		}

		if ( h < 5 )
		{
			return { t, p, brightness, 1.0f };
		}

		return { brightness, p, q, 1.0f };
	}

	uint32_t GraphContext::deduceMemoryType( uint32_t typeBits
		, VkMemoryPropertyFlags requirements )const
	{
		for ( uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i )
		{
			if ( ( typeBits & 1 ) == 1 )
			{
				if ( ( memoryProperties.memoryTypes[i].propertyFlags & requirements ) == requirements )
				{
					return i;
				}
			}

			typeBits >>= 1;
		}

		throw std::runtime_error{ "Could not deduce memory type" };
	}

#if VK_EXT_debug_utils

	void GraphContext::doBeginDebugUtilsLabel( VkCommandBuffer commandBuffer
		, VkDebugUtilsLabelEXT const & labelInfo )const
	{
		if ( vkCmdBeginDebugUtilsLabelEXT )
		{
			vkCmdBeginDebugUtilsLabelEXT( commandBuffer, &labelInfo );
		}
	}

	void GraphContext::doEndDebugUtilsLabel( VkCommandBuffer commandBuffer )const
	{
		if ( vkCmdEndDebugUtilsLabelEXT )
		{
			vkCmdEndDebugUtilsLabelEXT( commandBuffer );
		}
	}

	void GraphContext::doInsertDebugUtilsLabel( VkCommandBuffer commandBuffer
		, VkDebugUtilsLabelEXT const & labelInfo )const
	{
		if ( vkCmdBeginDebugUtilsLabelEXT )
		{
			vkCmdBeginDebugUtilsLabelEXT( commandBuffer, &labelInfo );
		}
	}

#endif
#if VK_EXT_debug_marker

	void GraphContext::doDebugMarkerBegin( VkCommandBuffer commandBuffer
		, VkDebugMarkerMarkerInfoEXT const & labelInfo )const
	{
		if ( vkCmdDebugMarkerBeginEXT )
		{
			vkCmdDebugMarkerBeginEXT( commandBuffer, &labelInfo );
		}
	}

	void GraphContext::doDebugMarkerEnd( VkCommandBuffer commandBuffer )const
	{
		if ( vkCmdDebugMarkerEndEXT )
		{
			vkCmdDebugMarkerEndEXT( commandBuffer );
		}
	}

	void GraphContext::doDebugMarkerInsert( VkCommandBuffer commandBuffer
		, VkDebugMarkerMarkerInfoEXT const & labelInfo )const
	{
		if ( vkCmdDebugMarkerInsertEXT )
		{
			vkCmdDebugMarkerInsertEXT( commandBuffer, &labelInfo );
		}
	}

#endif
#if VK_EXT_debug_utils || VK_EXT_debug_marker

	void GraphContext::doRegisterObject( uint64_t object
		, uint32_t objectType
		, std::string const & objectName
		, std::string const & typeName )const
	{
#	if VK_EXT_debug_utils
		if ( vkSetDebugUtilsObjectNameEXT )
		{
			VkDebugUtilsObjectNameInfoEXT nameInfo{ VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT
				, nullptr
				, VkObjectType( objectType )
				, object
				, objectName.c_str() };
			vkSetDebugUtilsObjectNameEXT( device, &nameInfo );
		}
#	endif
#	if VK_EXT_debug_marker
		if ( vkDebugMarkerSetObjectNameEXT )
		{
			VkDebugMarkerObjectNameInfoEXT nameInfo{ VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT
				, nullptr
				, VkDebugReportObjectTypeEXT( objectType )
				, object
				, objectName.c_str() };
			vkDebugMarkerSetObjectNameEXT( device, &nameInfo );
		}
#	endif
		std::stringstream stream;
		stream.imbue( std::locale{ "C" } );
		stream << "Created " << typeName
			<< " [0x" << std::hex << std::setw( 8u ) << std::setfill( '0' ) << object << "]"
			<< " - " << objectName;
		std::clog << stream.str() << std::endl;
		std::stringstream callStack;
		callStack << callstack::Backtrace{ 20, 4 };
		m_allocated.emplace( object
			, ObjectAllocation{
				typeName,
				objectName,
				callStack.str()
			} );
	}

	void GraphContext::doUnregisterObject( uint64_t object )const
	{
		auto it = m_allocated.find( object );
		assert( it != m_allocated.end() );
		m_allocated.erase( it );
	}

	void GraphContext::doReportRegisteredObjects()const
	{
		for ( auto & alloc : m_allocated )
		{
			std::stringstream stream;
			stream << "Leaked [" << alloc.second.type << "](" << alloc.second.name << "), allocation stack:\n";
			stream << alloc.second.callstack;
			std::cerr << stream.str() << "\n";
		}
	}

#endif

	void checkVkResult( VkResult result, char const * const stepName )
	{
		if ( result != VK_SUCCESS )
		{
			throw std::runtime_error{ stepName };
		}
	}
}
