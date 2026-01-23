/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#pragma warning( push )
#pragma warning( disable: 4365 )
#pragma warning( disable: 4865 )
#pragma warning( disable: 5262 )
#include <vulkan/vulkan.h>

#include <algorithm>
#include <bit>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <set>
#include <unordered_map>
#include <vector>
#pragma warning( pop )

#if !defined( _WIN32 ) || defined( CRG_BUILD_STATIC )
#	define CRG_API
#else
#	if defined( RenderGraph_EXPORTS )
#		define CRG_API __declspec( dllexport )
#	else
#		define CRG_API __declspec( dllimport )
#	endif
#endif

namespace crg
{
	struct Attachment;
	struct AttachmentTransitions;
	struct BufferData;
	struct BufferViewData;
	struct FramePass;
	struct FramePassGroup;
	struct GraphContext;
	struct GraphNode;
	struct ImageData;
	struct ImageViewData;
	struct IndexBuffer;
	struct IndirectBuffer;
	struct LayoutState;
	struct PipelineState;
	struct RootNode;
	struct SamplerDesc;
	struct SemaphoreWait;
	struct Texcoord;
	struct VertexBuffer;
	struct WriteDescriptorSet;

	class ContextResourcesCache;
	class Exception;
	class Fence;
	class FrameGraph;
	class FramePassTimer;
	class GraphVisitor;
	class ImageCopy;
	class PipelinePass;
	class RecordContext;
	class RenderPass;
	class RenderQuad;
	class ResourceHandler;
	class ResourcesCache;
	class RunnableGraph;
	class RunnablePass;

	template< typename DataT >
	struct Id;
	template< typename DataT >
	struct DataTransitionT;
	template< typename DataT >
	using DataTransitionArrayT = std::vector< DataTransitionT< DataT > >;
	template< typename VkTypeT >
	struct ContextObjectT;
	template< typename TypeT >
	struct DefaultValueGetterT;
	template< typename TypeT >
	struct RawTyperT;

	using BufferId = Id< BufferData >;
	using BufferViewId = Id< BufferViewData >;
	using ImageId = Id< ImageData >;
	using ImageViewId = Id< ImageViewData >;
	using AccessState = PipelineState;
	using DependencyCache = std::unordered_map< size_t, bool >;
	using PassDependencyCache = std::unordered_map< FramePass const *, DependencyCache >;
	using DeviceSize = VkDeviceSize;

	using AttachmentPtr = std::unique_ptr< Attachment >;
	using FramePassPtr = std::unique_ptr< FramePass >;
	using FramePassGroupPtr = std::unique_ptr< FramePassGroup >;
	using GraphNodePtr = std::unique_ptr< GraphNode >;
	using RunnableGraphPtr = std::unique_ptr< RunnableGraph >;
	using RunnablePassPtr = std::unique_ptr< RunnablePass >;
	using VertexBufferPtr = std::unique_ptr< VertexBuffer >;

	using GraphAdjacentNode = GraphNode *;
	using ConstGraphAdjacentNode = GraphNode const *;

	/**
	*\brief
	*	The transition between two states of an image view.
	*/
	using ImageTransition = DataTransitionT< ImageViewId >;
	using ImageTransitionArray = DataTransitionArrayT< ImageViewId >;
	/**
	*\brief
	*	The transition between two states of a buffer.
	*/
	using BufferTransition = DataTransitionT< BufferViewId >;
	using BufferTransitionArray = DataTransitionArrayT< BufferViewId >;

	using AttachmentArray = std::vector< Attachment const * >;
	using BufferViewIdArray = std::vector< BufferViewId >;
	using FramePassPtrArray = std::vector< FramePassPtr >;
	using FramePassGroupPtrArray = std::vector< FramePassGroupPtr >;
	using FrameGraphArray = std::vector< FrameGraph const * >;
	using FramePassArray = std::vector< FramePass const * >;
	using GraphAdjacentNodeArray = std::vector< GraphAdjacentNode >;
	using ConstGraphAdjacentNodeArray = std::vector< ConstGraphAdjacentNode >;
	using GraphNodePtrArray = std::vector< GraphNodePtr >;
	using WriteDescriptorSetArray = std::vector< WriteDescriptorSet >;
	using AttachmentsNodeMap = std::map< ConstGraphAdjacentNode, AttachmentTransitions >;
	using BufferMemoryMap = std::map< BufferId, std::pair< VkBuffer, VkDeviceMemory > >;
	using BufferViewMap = std::map< BufferViewId, VkBufferView >;
	using ImageMemoryMap = std::map< ImageId, std::pair< VkImage, VkDeviceMemory > >;
	using ImageViewMap = std::map< ImageViewId, VkImageView >;
	using ImageViewIdArray = std::vector< ImageViewId >;
	using SemaphoreWaitArray = std::vector< SemaphoreWait >;

	template< typename DataT >
	using IdDataOwnerCont = std::map< Id< DataT >, std::unique_ptr< DataT > >;
	using BufferIdDataOwnerCont = IdDataOwnerCont< BufferData >;
	using BufferViewIdDataOwnerCont = IdDataOwnerCont< BufferViewData >;
	using ImageIdDataOwnerCont = IdDataOwnerCont< ImageData >;
	using ImageViewIdDataOwnerCont = IdDataOwnerCont< ImageViewData >;

	using VkAttachmentDescriptionArray = std::vector< VkAttachmentDescription >;
	using VkAttachmentReferenceArray = std::vector< VkAttachmentReference >;
	using VkBufferArray = std::vector< VkBuffer >;
	using VkBufferViewArray = std::vector< VkBufferView >;
	using VkDescriptorBufferInfoArray = std::vector< VkDescriptorBufferInfo >;
	using VkDescriptorImageInfoArray = std::vector< VkDescriptorImageInfo >;
	using VkDescriptorSetLayoutBindingArray = std::vector< VkDescriptorSetLayoutBinding >;
	using VkDescriptorPoolSizeArray = std::vector< VkDescriptorPoolSize >;
	using VkImageViewArray = std::vector< VkImageView >;
	using VkPipelineColorBlendAttachmentStateArray = std::vector< VkPipelineColorBlendAttachmentState >;
	using VkPipelineShaderStageCreateInfoArray = std::vector< VkPipelineShaderStageCreateInfo >;
	using VkPushConstantRangeArray = std::vector< VkPushConstantRange >;
	using VkScissorArray = std::vector< VkRect2D >;
	using VkSubpassDependencyArray = std::vector< VkSubpassDependency >;
	using VkVertexInputAttributeDescriptionArray = std::vector< VkVertexInputAttributeDescription >;
	using VkVertexInputBindingDescriptionArray = std::vector< VkVertexInputBindingDescription >;
	using VkViewportArray = std::vector< VkViewport >;
	using VkWriteDescriptorSetArray = std::vector< VkWriteDescriptorSet >;

	using MipLayoutStates = std::map< uint32_t, LayoutState >;
	using LayerLayoutStates = std::map< uint32_t, MipLayoutStates >;
	using LayoutStateMap = std::unordered_map< uint32_t, LayerLayoutStates >;
	using LayerLayoutStatesMap = std::map< uint32_t, LayerLayoutStates >;
	using AccessStateMap = std::unordered_map< uint32_t, AccessState >;
	using ViewsLayout = LayoutStateMap;
	using BuffersLayout = AccessStateMap;
	using ViewsLayoutPtr = std::unique_ptr< ViewsLayout >;
	using BuffersLayoutPtr = std::unique_ptr< BuffersLayout >;
	using ViewsLayouts = std::vector< ViewsLayoutPtr >;
	using BuffersLayouts = std::vector< BuffersLayoutPtr >;

	using ViewLayoutIterators = std::map< uint32_t, ViewsLayouts::iterator >;
	using BufferLayoutIterators = std::map< uint32_t, BuffersLayouts::iterator >;
}
