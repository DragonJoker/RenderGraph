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
	struct AttachmentTransitions;
	struct Buffer;
	struct FramePassTransitions;
	struct GraphContext;
	struct ImageData;
	struct ImageViewData;
	struct GraphNode;
	struct FramePass;
	struct RootNode;
	struct SamplerDesc;
	struct Texcoord;
	struct WriteDescriptorSet;

	class Exception;
	class FrameGraph;
	class FramePassTimer;
	class GraphVisitor;
	class ResourceHandler;
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

	template< typename DataT >
	struct DataTransitionT;
	template< typename DataT >
	using DataTransitionArrayT = std::vector< DataTransitionT< DataT > >;
	/**
	*\brief
	*	The transition between two states of an image view.
	*/
	using ViewTransition = DataTransitionT< ImageViewId >;
	using ViewTransitionArray = DataTransitionArrayT< ImageViewId >;
	/**
	*\brief
	*	The transition between two states of a storage buffer.
	*/
	using BufferTransition = DataTransitionT< Buffer >;
	using BufferTransitionArray = DataTransitionArrayT< Buffer >;

	using AttachmentArray = std::vector< Attachment >;
	using FramePassPtrArray = std::vector< FramePassPtr >;
	using FramePassArray = std::vector< FramePass const * >;
	using GraphAdjacentNodeArray = std::vector< GraphAdjacentNode >;
	using ConstGraphAdjacentNodeArray = std::vector< ConstGraphAdjacentNode >;
	using GraphNodePtrArray = std::vector< GraphNodePtr >;
	using FramePassDependencies = std::vector< FramePassTransitions >;
	using WriteDescriptorSetArray = std::vector< WriteDescriptorSet >;
	using AttachmentsNodeMap = std::map< ConstGraphAdjacentNode, AttachmentTransitions >;
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

	struct LayoutState
	{
		VkImageLayout layout;
		VkAccessFlags access;
		VkPipelineStageFlags pipelineStage;
	};

	struct AccessState
	{
		VkAccessFlags access;
		VkPipelineStageFlags pipelineStage;
	};

	struct Buffer
	{
		VkBuffer buffer;
		std::string name;
	};
	CRG_API bool operator==( Buffer const & lhs, Buffer const & rhs );

	struct VertexBuffer
	{
		Buffer buffer{ nullptr, std::string{} };
		VkDeviceMemory memory{ nullptr };
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
	static inline TypeT defaultV = DefaultValueGetterT< TypeT >::get();

	template< typename TypeT >
	static inline TypeT getDefaultV()
	{
		return DefaultValueGetterT< TypeT >::get();
	}

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

	CRG_API VkExtent3D getExtent( ImageId const & image );
	CRG_API VkExtent3D getExtent( ImageViewId const & image );
	CRG_API VkFormat getFormat( ImageId const & image );
	CRG_API VkFormat getFormat( ImageViewId const & image );
	CRG_API VkImageType getImageType( ImageId const & image );
	CRG_API VkImageType getImageType( ImageViewId const & image );
	CRG_API uint32_t getMipLevels( ImageId const & image );
	CRG_API uint32_t getMipLevels( ImageViewId const & image );
	CRG_API uint32_t getArrayLayers( ImageId const & image );
	CRG_API uint32_t getArrayLayers( ImageViewId const & image );

	template< typename T >
	static size_t hashCombine( size_t hash
		, T const & rhs )
	{
		const uint64_t kMul = 0x9ddfea08eb382d69ULL;
		auto seed = hash;

		std::hash< T > hasher;
		uint64_t a = ( hasher( rhs ) ^ seed ) * kMul;
		a ^= ( a >> 47 );

		uint64_t b = ( seed ^ a ) * kMul;
		b ^= ( b >> 47 );

#pragma warning( push )
#pragma warning( disable: 4068 )
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuseless-cast"
		hash = static_cast< std::size_t >( b * kMul );
#pragma GCC diagnostic pop
#pragma clang diagnostic pop
#pragma warning( pop )
		return hash;
	}

	CRG_API void convert( SemaphoreWaitArray const & toWait
		, std::vector< VkSemaphore > & semaphores
		, std::vector< VkPipelineStageFlags > & dstStageMasks );
	CRG_API void convert( SemaphoreWaitArray const & toWait
		, std::vector< VkSemaphore > & semaphores );
}
