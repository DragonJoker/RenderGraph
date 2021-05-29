/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include <vulkan/vulkan.h>

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <set>
#include <vector>

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
	template< typename DataT >
	struct Id;

	struct Attachment;
	struct AttachmentTransition;
	struct GraphContext;
	struct ImageData;
	struct ImageViewData;
	struct GraphNode;
	struct FramePass;
	struct RootNode;
	struct WriteDescriptorSet;

	class Exception;
	class FrameGraph;
	class FramePassTimer;
	class GraphVisitor;
	class RunnableGraph;
	class RunnablePass;

	class ImageCopy;
	class PipelinePass;
	class RenderPass;
	class RenderQuad;

	using ImageId = Id< ImageData >;
	using ImageViewId = Id< ImageViewData >;

	using FramePassPtr = std::unique_ptr< FramePass >;
	using GraphNodePtr = std::unique_ptr< GraphNode >;
	using RunnableGraphPtr = std::unique_ptr< RunnableGraph >;
	using RunnablePassPtr = std::unique_ptr< RunnablePass >;
	using GraphAdjacentNode = GraphNode *;
	using ConstGraphAdjacentNode = GraphNode const *;

	using AttachmentArray = std::vector< Attachment >;
	using AttachmentTransitionArray = std::vector< AttachmentTransition >;
	using FramePassPtrArray = std::vector< FramePassPtr >;
	using FramePassArray = std::vector< FramePass const * >;
	using GraphAdjacentNodeArray = std::vector< GraphAdjacentNode >;
	using ConstGraphAdjacentNodeArray = std::vector< ConstGraphAdjacentNode >;
	using GraphNodePtrArray = std::vector< GraphNodePtr >;
	using FramePassDependenciesMap = std::map< FramePass const *, AttachmentTransitionArray >;
	using WriteDescriptorSetArray = std::vector< WriteDescriptorSet >;
	using AttachmentsNodeMap = std::map< ConstGraphAdjacentNode, AttachmentTransitionArray >;
	using ImageMemoryMap = std::map< ImageId, std::pair< VkImage, VkDeviceMemory > >;
	using ImageViewMap = std::map< ImageViewId, VkImageView >;
	using ImageIdArray = std::vector< ImageId >;
	using ImageViewIdArray = std::vector< ImageViewId >;

	template< typename DataT >
	using IdAliasMap = std::map< Id< DataT >, Id< DataT > >;
	using ImageIdAliasMap = IdAliasMap< ImageData >;
	using ImageViewIdAliasMap = IdAliasMap< ImageViewData >;

	template< typename DataT >
	using IdDataOwnerCont = std::map< Id< DataT >, std::unique_ptr< DataT > >;
	using ImageIdDataOwnerCont = IdDataOwnerCont< ImageData >;
	using ImageViewIdDataOwnerCont = IdDataOwnerCont< ImageViewData >;

	using VkAttachmentDescriptionArray = std::vector< VkAttachmentDescription >;
	using VkAttachmentReferenceArray = std::vector< VkAttachmentReference >;
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

	struct VertexBuffer
	{
		VkBuffer buffer{ VK_NULL_HANDLE };
		VkDeviceMemory memory{ VK_NULL_HANDLE };
		VkVertexInputAttributeDescriptionArray vertexAttribs{};
		VkVertexInputBindingDescriptionArray vertexBindings{};
		VkPipelineVertexInputStateCreateInfo inputState{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, nullptr, {}, {}, {}, {}, {} };
	};

	using VertexBufferPtr = std::unique_ptr< VertexBuffer >;

	static constexpr VkPipelineColorBlendAttachmentState DefaultBlendState{ VK_FALSE
		, VK_BLEND_FACTOR_ONE
		, VK_BLEND_FACTOR_ZERO
		, VK_BLEND_OP_ADD
		, VK_BLEND_FACTOR_ONE
		, VK_BLEND_FACTOR_ZERO
		, VK_BLEND_OP_ADD
		, VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT };

	template< typename TypeT >
	struct DefaultValueGetterT;

	template< typename TypeT >
	static inline TypeT const & defaultV = DefaultValueGetterT< TypeT >::get();

	template< typename TypeT >
	struct RawTyperT
	{
		using Type = TypeT;
	};

	template< typename TypeT >
	using RawTypeT = typename RawTyperT< TypeT >::Type;

	struct SemaphoreWait
	{
		VkSemaphore semaphore;
		VkPipelineStageFlags dstStageMask;
	};

	using SemaphoreWaitArray = std::vector< SemaphoreWait >;
}
