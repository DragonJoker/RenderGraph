/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "FrameGraphPrerequisites.hpp"

#include <array>
#include <string>
#include <unordered_map>

namespace crg
{
	template< typename ObjectT >
	struct DebugTypeTraits;

	template<>
	struct DebugTypeTraits< VkBuffer >
	{
#if VK_EXT_debug_utils
		static VkObjectType constexpr UtilsValue = VK_OBJECT_TYPE_BUFFER;
#endif
#if VK_EXT_debug_report || VK_EXT_debug_marker
		static VkDebugReportObjectTypeEXT constexpr ReportValue = VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT;
#endif
		static std::string const & getName()
		{
			static std::string result{ "VkBuffer" };
			return result;
		}
	};

	template<>
	struct DebugTypeTraits< VkBufferView >
	{
#if VK_EXT_debug_utils
		static VkObjectType constexpr UtilsValue = VK_OBJECT_TYPE_BUFFER_VIEW;
#endif
#if VK_EXT_debug_report || VK_EXT_debug_marker
		static VkDebugReportObjectTypeEXT constexpr ReportValue = VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT;
#endif
		static std::string const & getName()
		{
			static std::string result{ "VkBufferView" };
			return result;
		}
	};

	template<>
	struct DebugTypeTraits< VkCommandBuffer >
	{
#if VK_EXT_debug_utils
		static VkObjectType constexpr UtilsValue = VK_OBJECT_TYPE_COMMAND_BUFFER;
#endif
#if VK_EXT_debug_report || VK_EXT_debug_marker
		static VkDebugReportObjectTypeEXT constexpr ReportValue = VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT;
#endif
		static std::string const & getName()
		{
			static std::string result{ "VkCommandBuffer" };
			return result;
		}
	};

	template<>
	struct DebugTypeTraits< VkCommandPool >
	{
#if VK_EXT_debug_utils
		static VkObjectType constexpr UtilsValue = VK_OBJECT_TYPE_COMMAND_POOL;
#endif
#if VK_EXT_debug_report || VK_EXT_debug_marker
		static VkDebugReportObjectTypeEXT constexpr ReportValue = VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT;
#endif
		static std::string const & getName()
		{
			static std::string result{ "VkCommandPool" };
			return result;
		}
	};

	template<>
	struct DebugTypeTraits< VkDescriptorPool >
	{
#if VK_EXT_debug_utils
		static VkObjectType constexpr UtilsValue = VK_OBJECT_TYPE_DESCRIPTOR_POOL;
#endif
#if VK_EXT_debug_report || VK_EXT_debug_marker
		static VkDebugReportObjectTypeEXT constexpr ReportValue = VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT;
#endif
		static std::string const & getName()
		{
			static std::string result{ "VkDescriptorPool" };
			return result;
		}
	};

	template<>
	struct DebugTypeTraits< VkDescriptorSet >
	{
#if VK_EXT_debug_utils
		static VkObjectType constexpr UtilsValue = VK_OBJECT_TYPE_DESCRIPTOR_SET;
#endif
#if VK_EXT_debug_report || VK_EXT_debug_marker
		static VkDebugReportObjectTypeEXT constexpr ReportValue = VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT;
#endif
		static std::string const & getName()
		{
			static std::string result{ "VkDescriptorSet" };
			return result;
		}
	};

	template<>
	struct DebugTypeTraits< VkDescriptorSetLayout >
	{
#if VK_EXT_debug_utils
		static VkObjectType constexpr UtilsValue = VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT;
#endif
#if VK_EXT_debug_report || VK_EXT_debug_marker
		static VkDebugReportObjectTypeEXT constexpr ReportValue = VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT;
#endif
		static std::string const & getName()
		{
			static std::string result{ "VkDescriptorSetLayout" };
			return result;
		}
	};

	template<>
	struct DebugTypeTraits< VkDevice >
	{
#if VK_EXT_debug_utils
		static VkObjectType constexpr UtilsValue = VK_OBJECT_TYPE_PHYSICAL_DEVICE;
#endif
#if VK_EXT_debug_report || VK_EXT_debug_marker
		static VkDebugReportObjectTypeEXT constexpr ReportValue = VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT;
#endif
		static std::string const & getName()
		{
			static std::string result{ "VkDevice" };
			return result;
		}
	};

	template<>
	struct DebugTypeTraits< VkDeviceMemory >
	{
#if VK_EXT_debug_utils
		static VkObjectType constexpr UtilsValue = VK_OBJECT_TYPE_DEVICE_MEMORY;
#endif
#if VK_EXT_debug_report || VK_EXT_debug_marker
		static VkDebugReportObjectTypeEXT constexpr ReportValue = VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT;
#endif
		static std::string const & getName()
		{
			static std::string result{ "VkDeviceMemory" };
			return result;
		}
	};

	template<>
	struct DebugTypeTraits< VkEvent >
	{
#if VK_EXT_debug_utils
		static VkObjectType constexpr UtilsValue = VK_OBJECT_TYPE_EVENT;
#endif
#if VK_EXT_debug_report || VK_EXT_debug_marker
		static VkDebugReportObjectTypeEXT constexpr ReportValue = VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT;
#endif
		static std::string const & getName()
		{
			static std::string result{ "VkEvent" };
			return result;
		}
	};

	template<>
	struct DebugTypeTraits< VkFence >
	{
#if VK_EXT_debug_utils
		static VkObjectType constexpr UtilsValue = VK_OBJECT_TYPE_FENCE;
#endif
#if VK_EXT_debug_report || VK_EXT_debug_marker
		static VkDebugReportObjectTypeEXT constexpr ReportValue = VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT;
#endif
		static std::string const & getName()
		{
			static std::string result{ "VkFence" };
			return result;
		}
	};

	template<>
	struct DebugTypeTraits< VkFramebuffer >
	{
#if VK_EXT_debug_utils
		static VkObjectType constexpr UtilsValue = VK_OBJECT_TYPE_FRAMEBUFFER;
#endif
#if VK_EXT_debug_report || VK_EXT_debug_marker
		static VkDebugReportObjectTypeEXT constexpr ReportValue = VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT;
#endif
		static std::string const & getName()
		{
			static std::string result{ "VkFramebuffer" };
			return result;
		}
	};

	template<>
	struct DebugTypeTraits< VkImage >
	{
#if VK_EXT_debug_utils
		static VkObjectType constexpr UtilsValue = VK_OBJECT_TYPE_IMAGE;
#endif
#if VK_EXT_debug_report || VK_EXT_debug_marker
		static VkDebugReportObjectTypeEXT constexpr ReportValue = VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT;
#endif
		static std::string const & getName()
		{
			static std::string result{ "VkImage" };
			return result;
		}
	};

	template<>
	struct DebugTypeTraits< VkImageView >
	{
#if VK_EXT_debug_utils
		static VkObjectType constexpr UtilsValue = VK_OBJECT_TYPE_IMAGE_VIEW;
#endif
#if VK_EXT_debug_report || VK_EXT_debug_marker
		static VkDebugReportObjectTypeEXT constexpr ReportValue = VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT;
#endif
		static std::string const & getName()
		{
			static std::string result{ "VkImageView" };
			return result;
		}
	};

	template<>
	struct DebugTypeTraits< VkInstance >
	{
#if VK_EXT_debug_utils
		static VkObjectType constexpr UtilsValue = VK_OBJECT_TYPE_INSTANCE;
#endif
#if VK_EXT_debug_report || VK_EXT_debug_marker
		static VkDebugReportObjectTypeEXT constexpr ReportValue = VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT;
#endif
		static std::string const & getName()
		{
			static std::string result{ "VkInstance" };
			return result;
		}
	};

	template<>
	struct DebugTypeTraits< VkPhysicalDevice >
	{
#if VK_EXT_debug_utils
		static VkObjectType constexpr UtilsValue = VK_OBJECT_TYPE_PHYSICAL_DEVICE;
#endif
#if VK_EXT_debug_report || VK_EXT_debug_marker
		static VkDebugReportObjectTypeEXT constexpr ReportValue = VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT;
#endif
		static std::string const & getName()
		{
			static std::string result{ "VkPhysicalDevice" };
			return result;
		}
	};

	template<>
	struct DebugTypeTraits< VkPipeline >
	{
#if VK_EXT_debug_utils
		static VkObjectType constexpr UtilsValue = VK_OBJECT_TYPE_PIPELINE;
#endif
#if VK_EXT_debug_report || VK_EXT_debug_marker
		static VkDebugReportObjectTypeEXT constexpr ReportValue = VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT;
#endif
		static std::string const & getName()
		{
			static std::string result{ "VkPipeline" };
			return result;
		}
	};

	template<>
	struct DebugTypeTraits< VkPipelineLayout >
	{
#if VK_EXT_debug_utils
		static VkObjectType constexpr UtilsValue = VK_OBJECT_TYPE_PIPELINE_LAYOUT;
#endif
#if VK_EXT_debug_report || VK_EXT_debug_marker
		static VkDebugReportObjectTypeEXT constexpr ReportValue = VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT;
#endif
		static std::string const & getName()
		{
			static std::string result{ "VkPipelineLayout" };
			return result;
		}
	};

	template<>
	struct DebugTypeTraits< VkQueryPool >
	{
#if VK_EXT_debug_utils
		static VkObjectType constexpr UtilsValue = VK_OBJECT_TYPE_QUERY_POOL;
#endif
#if VK_EXT_debug_report || VK_EXT_debug_marker
		static VkDebugReportObjectTypeEXT constexpr ReportValue = VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT;
#endif
		static std::string const & getName()
		{
			static std::string result{ "VkQueryPool" };
			return result;
		}
	};

	template<>
	struct DebugTypeTraits< VkQueue >
	{
#if VK_EXT_debug_utils
		static VkObjectType constexpr UtilsValue = VK_OBJECT_TYPE_QUEUE;
#endif
#if VK_EXT_debug_report || VK_EXT_debug_marker
		static VkDebugReportObjectTypeEXT constexpr ReportValue = VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT;
#endif
		static std::string const & getName()
		{
			static std::string result{ "VkQueue" };
			return result;
		}
	};

	template<>
	struct DebugTypeTraits< VkRenderPass >
	{
#if VK_EXT_debug_utils
		static VkObjectType constexpr UtilsValue = VK_OBJECT_TYPE_RENDER_PASS;
#endif
#if VK_EXT_debug_report || VK_EXT_debug_marker
		static VkDebugReportObjectTypeEXT constexpr ReportValue = VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT;
#endif
		static std::string const & getName()
		{
			static std::string result{ "VkRenderPass" };
			return result;
		}
	};

	template<>
	struct DebugTypeTraits< VkSampler >
	{
#if VK_EXT_debug_utils
		static VkObjectType constexpr UtilsValue = VK_OBJECT_TYPE_SAMPLER;
#endif
#if VK_EXT_debug_report || VK_EXT_debug_marker
		static VkDebugReportObjectTypeEXT constexpr ReportValue = VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT;
#endif
		static std::string const & getName()
		{
			static std::string result{ "VkSampler" };
			return result;
		}
	};

	template<>
	struct DebugTypeTraits< VkSemaphore >
	{
#if VK_EXT_debug_utils
		static VkObjectType constexpr UtilsValue = VK_OBJECT_TYPE_SEMAPHORE;
#endif
#if VK_EXT_debug_report || VK_EXT_debug_marker
		static VkDebugReportObjectTypeEXT constexpr ReportValue = VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT;
#endif
		static std::string const & getName()
		{
			static std::string result{ "VkSemaphore" };
			return result;
		}
	};

	template<>
	struct DebugTypeTraits< VkShaderModule >
	{
#if VK_EXT_debug_utils
		static VkObjectType constexpr UtilsValue = VK_OBJECT_TYPE_SHADER_MODULE;
#endif
#if VK_EXT_debug_report || VK_EXT_debug_marker
		static VkDebugReportObjectTypeEXT constexpr ReportValue = VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT;
#endif
		static std::string const & getName()
		{
			static std::string result{ "VkShaderModule" };
			return result;
		}
	};

	template<>
	struct DebugTypeTraits< VkSurfaceKHR >
	{
#if VK_EXT_debug_utils
		static VkObjectType constexpr UtilsValue = VK_OBJECT_TYPE_SURFACE_KHR;
#endif
#if VK_EXT_debug_report || VK_EXT_debug_marker
		static VkDebugReportObjectTypeEXT constexpr ReportValue = VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT;
#endif
		static std::string const & getName()
		{
			static std::string result{ "VkSurfaceKHR" };
			return result;
		}
	};

	template<>
	struct DebugTypeTraits< VkSwapchainKHR >
	{
#if VK_EXT_debug_utils
		static VkObjectType constexpr UtilsValue = VK_OBJECT_TYPE_SWAPCHAIN_KHR;
#endif
#if VK_EXT_debug_report || VK_EXT_debug_marker
		static VkDebugReportObjectTypeEXT constexpr ReportValue = VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT;
#endif
		static std::string const & getName()
		{
			static std::string result{ "VkSwapchainKHR" };
			return result;
		}
	};

	struct DebugBlockInfo
	{
		std::string markerName;
		std::array< float, 4 > colour;
	};

	struct GraphContext
	{
		CRG_API GraphContext( VkDevice device
			, VkPipelineCache cache
			, VkAllocationCallbacks const * allocator
			, VkPhysicalDeviceMemoryProperties memoryProperties
			, VkPhysicalDeviceProperties properties
			, bool separateDepthStencilLayouts
			, PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr );

		VkDevice device{ VK_NULL_HANDLE };
		VkPipelineCache cache{ VK_NULL_HANDLE };
		VkAllocationCallbacks const * allocator{ nullptr };
		VkPhysicalDeviceMemoryProperties memoryProperties{};
		VkPhysicalDeviceProperties properties{};
		VkPhysicalDeviceFeatures features{};
		bool separateDepthStencilLayouts;

#define DECL_vkFunction( name )\
		PFN_vk##name vk##name{ nullptr }

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
		DECL_vkFunction( ResetCommandBuffer );
		DECL_vkFunction( CreateEvent );
		DECL_vkFunction( DestroyEvent );
		DECL_vkFunction( ResetEvent );
		DECL_vkFunction( SetEvent );
		DECL_vkFunction( GetEventStatus );
		DECL_vkFunction( CreateFence );
		DECL_vkFunction( DestroyFence );
		DECL_vkFunction( GetFenceStatus );
		DECL_vkFunction( WaitForFences );
		DECL_vkFunction( ResetFences );

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
		DECL_vkFunction( CmdPipelineBarrier );
		DECL_vkFunction( CmdCopyImage );
		DECL_vkFunction( CmdExecuteCommands );
		DECL_vkFunction( CmdResetEvent );
		DECL_vkFunction( CmdSetEvent );
		DECL_vkFunction( CmdWaitEvents );

#if VK_EXT_debug_utils || VK_EXT_debug_marker
#	if VK_EXT_debug_utils
		DECL_vkFunction( SetDebugUtilsObjectNameEXT );
		DECL_vkFunction( CmdBeginDebugUtilsLabelEXT );
		DECL_vkFunction( CmdEndDebugUtilsLabelEXT );
#	endif
#	if VK_EXT_debug_marker
		DECL_vkFunction( DebugMarkerSetObjectNameEXT );
		DECL_vkFunction( CmdDebugMarkerBeginEXT );
		DECL_vkFunction( CmdDebugMarkerEndEXT );
		DECL_vkFunction( CmdDebugMarkerInsertEXT );
#	endif
#undef DECL_vkFunction

#if VK_EXT_debug_utils || VK_EXT_debug_marker
		/**
		*\brief
		*	Begins a command buffer label.
		*\param[in] labelInfo
		*	The parameters of the label to begin.
		*/
		CRG_API void vkCmdBeginDebugBlock( VkCommandBuffer commandBuffer
			, DebugBlockInfo const & labelInfo )const;
		/**
		*\brief
		*	Ends the command label.
		*/
		CRG_API void vkCmdEndDebugBlock( VkCommandBuffer commandBuffer )const;
		/**
		*\brief
		*	Inserts a command label.
		*\param[in] labelInfo
		*	The parameters of the label to begin.
		*/
		CRG_API void vkCmdInsertDebugBlock( VkCommandBuffer commandBuffer
			, DebugBlockInfo const & labelInfo )const;
#endif
		CRG_API std::array< float, 4u > getNextRainbowColour()const;
		CRG_API uint32_t deduceMemoryType( uint32_t typeBits
			, VkMemoryPropertyFlags requirements )const;

	private:
		struct ObjectAllocation
		{
			std::string type;
			std::string name;
			std::string callstack;
		};

		mutable std::unordered_map< size_t, ObjectAllocation > m_allocated;

	public:
		template< typename ObjectT >
		static void stRegisterObject( GraphContext const & context
			, std::string const & name
			, ObjectT object )
		{
			using MyTraits = DebugTypeTraits< ObjectT >;
			context.doRegisterObject( uint64_t( object )
#if VK_EXT_debug_utils
				, uint32_t( MyTraits::UtilsValue )
#elif VK_EXT_debug_marker
				, uint32_t( MyTraits::ReportValue )
#endif
				, name
				, MyTraits::getName() );
		}

		template< typename ObjectT >
		static void stUnregisterObject( GraphContext const & context, ObjectT object )
		{
			context.doUnregisterObject( uint64_t( object ) );
		}

	private:
#if VK_EXT_debug_utils
		/**
		*\brief
		*	Begins a command buffer label.
		*\param[in] labelInfo
		*	The parameters of the label to begin.
		*/
		void doBeginDebugUtilsLabel( VkCommandBuffer commandBuffer
			, VkDebugUtilsLabelEXT const & labelInfo )const;
		/**
		*\brief
		*	Ends the command label.
		*/
		void doEndDebugUtilsLabel( VkCommandBuffer commandBuffer )const;
		/**
		*\brief
		*	Inserts a command label.
		*\param[in] labelInfo
		*	The parameters of the label to begin.
		*/
		void doInsertDebugUtilsLabel( VkCommandBuffer commandBuffer
			, VkDebugUtilsLabelEXT const & labelInfo )const;
#endif
#if VK_EXT_debug_marker
		/**
		*\brief
		*	Begins a command buffer label.
		*\param[in] labelInfo
		*	The parameters of the label to begin.
		*/
		void doDebugMarkerBegin( VkCommandBuffer commandBuffer
			, VkDebugMarkerMarkerInfoEXT const & labelInfo )const;
		/**
		*\brief
		*	Ends the command label.
		*/
		void doDebugMarkerEnd( VkCommandBuffer commandBuffer )const;
		/**
		*\brief
		*	Inserts a command label.
		*\param[in] labelInfo
		*	The parameters of the label to begin.
		*/
		void doDebugMarkerInsert( VkCommandBuffer commandBuffer
			, VkDebugMarkerMarkerInfoEXT const & labelInfo )const;
#endif

		CRG_API void doRegisterObject( uint64_t object
			, uint32_t objectType
			, std::string const & name
			, std::string const & typeName )const;
		CRG_API void doUnregisterObject( uint64_t object )const;
		CRG_API void doReportRegisteredObjects()const;

#	define crgRegisterObject( Cont, TypeName, Object )\
		crg::GraphContext::stRegisterObject( Cont, TypeName, Object )
#	define crgUnregisterObject( Cont, Object )\
		crg::GraphContext::stUnregisterObject( Cont, Object )

#	ifndef NDEBUG
#		define crgReportRegisteredObjects()\
		doReportRegisteredObjects()
#	else
#		define reportRegisteredObjects()
#	endif
#else
#	define registerObject( Dev, TypeName, Object )
#	define unregisterObject( Dev, Object )
#	define reportRegisteredObjects()
#endif
	};

	CRG_API void checkVkResult( VkResult result, char const * const stepName );
	CRG_API void checkVkResult( VkResult result, std::string const & stepName );
}
