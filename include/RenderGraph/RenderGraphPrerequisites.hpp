/*
This file belongs to RenderGraph.
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
	struct RenderPass;
	struct RenderPassDependencies;
	struct RootNode;
	struct WriteDescriptorSet;

	class GraphVisitor;
	class RenderGraph;
	class RunnableGraph;
	class RunnablePass;

	using ImageId = Id< ImageData >;
	using ImageViewId = Id< ImageViewData >;

	using RenderPassPtr = std::unique_ptr< RenderPass >;
	using GraphNodePtr = std::unique_ptr< GraphNode >;
	using RunnableGraphPtr = std::unique_ptr< RunnableGraph >;
	using RunnablePassPtr = std::unique_ptr< RunnablePass >;
	using GraphAdjacentNode = GraphNode *;
	using ConstGraphAdjacentNode = GraphNode const *;

	using AttachmentArray = std::vector< Attachment >;
	using AttachmentTransitionArray = std::vector< AttachmentTransition >;
	using RenderPassPtrArray = std::vector< RenderPassPtr >;
	using GraphAdjacentNodeArray = std::vector< GraphAdjacentNode >;
	using GraphNodePtrArray = std::vector< GraphNodePtr >;
	using RenderPassDependenciesArray = std::vector< RenderPassDependencies >;
	using WriteDescriptorSetArray = std::vector< WriteDescriptorSet >;
	using AttachmentsNodeMap = std::map< ConstGraphAdjacentNode, AttachmentTransitionArray >;
	using ImageMemoryMap = std::map< ImageId, std::pair< VkImage, VkDeviceMemory > >;
	using ImageViewMap = std::map< ImageViewId, VkImageView >;

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
}
