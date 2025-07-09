/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include <vulkan/vulkan.h>

#pragma warning( push )
#pragma warning( disable: 4365 )
#pragma warning( disable: 5262 )
#include <algorithm>
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

#define CRG_MakeFlags( FlagBits )\
	constexpr FlagBits operator|( FlagBits lhs, FlagBits rhs ) { return FlagBits( std::underlying_type_t< FlagBits >( lhs ) | std::underlying_type_t< FlagBits >( rhs ) ); }\
	constexpr FlagBits operator&( FlagBits lhs, FlagBits rhs ) { return FlagBits( std::underlying_type_t< FlagBits >( lhs ) & std::underlying_type_t< FlagBits >( rhs ) ); }\
	constexpr FlagBits operator^( FlagBits lhs, FlagBits rhs ) { return FlagBits( std::underlying_type_t< FlagBits >( lhs ) ^ std::underlying_type_t< FlagBits >( rhs ) ); }\
	constexpr FlagBits & operator|=( FlagBits & lhs, FlagBits rhs ) { return lhs = lhs | rhs; }\
	constexpr FlagBits & operator&=( FlagBits & lhs, FlagBits rhs ) { return lhs = lhs & rhs; }\
	constexpr FlagBits & operator^=( FlagBits & lhs, FlagBits rhs ) { return lhs = lhs ^ rhs; }\
	constexpr bool checkFlag( FlagBits lhs, FlagBits rhs ) { return ( lhs & rhs ) == rhs; }

namespace crg
{
	enum class PixelFormat : uint32_t
	{
#define RGPF_ENUM_VALUE( name, value, components, alpha, colour, depth, stencil, compressed ) e##name = value,
#include "PixelFormat.enum"
	};

	enum class ImageType : uint32_t
	{
		e1D = 0,
		e2D = 1,
		e3D = 2,
	};

	enum class ImageViewType : uint32_t
	{
		e1D = 0,
		e2D = 1,
		e3D = 2,
		eCube = 3,
		e1DArray = 4,
		e2DArray = 5,
		eCubeArray = 6,
	};

	enum class ImageLayout : uint32_t
	{
		eUndefined = 0,
		eGeneral = 1,
		eColorAttachment = 2,
		eDepthStencilAttachment = 3,
		eDepthStencilReadOnly = 4,
		eShaderReadOnly = 5,
		eTransferSrc = 6,
		eTransferDst = 7,
		ePreinitialized = 8,
		eDepthReadOnlyStencilAttachment = 1000117000,
		eDepthAttachmentStencilReadOnly = 1000117001,
		eDepthAttachment = 1000241000,
		eDepthReadOnly = 1000241001,
		eStencilAttachment = 1000241002,
		eStencilReadOnly = 1000241003,
		eReadOnly = 1000314000,
		eAttachment = 1000314001,
		eRenderingLocalRead = 1000232000,
		ePresentSrc = 1000001002,
		eVideoDecodeDst = 1000024000,
		eVideoDecodeSrc = 1000024001,
		eVideoDecodeDpb = 1000024002,
		eSharedPresent = 1000111000,
		eFragmentDensityMap = 1000218000,
		eFragmentShadingRateAttachment = 1000164003,
		eVideoEncodeDst = 1000299000,
		eVideoEncodeSrc = 1000299001,
		eVideoEncodeDpb = 1000299002,
		eAttachmentFeedbackLoop = 1000339000,
		eVideoEncodeQuantizationMap = 1000553000,
	};

	enum class FilterMode : uint32_t
	{
		eNearest,
		eLinear,
	};

	enum class MipmapMode : uint32_t
	{
		eNearest,
		eLinear,
	};

	enum class WrapMode : uint32_t
	{
		eRepeat,
		eMirroredRepeat,
		eClampToBorder,
		eClampToEdge,
		eMirrorClampToEdge,
	};

	enum class PipelineStageFlags : uint32_t
	{
		eNone = 0,
		eTopOfPipe = 0x00000001,
		eDrawIndirect = 0x00000002,
		eVertexInput = 0x00000004,
		eVertexShader = 0x00000008,
		eTessellationControlShader = 0x00000010,
		eTessellationEvaluationShader = 0x00000020,
		eGeometryShader = 0x00000040,
		eFragmentShader = 0x00000080,
		eEarlyFragmentTests = 0x00000100,
		eLateFragmentTests = 0x00000200,
		eColorAttachmentOutput = 0x00000400,
		eComputeShader = 0x00000800,
		eTransfer = 0x00001000,
		eBottomOfPipe = 0x00002000,
		eHost = 0x00004000,
		eAllGraphics = 0x00008000,
		eAllCommands = 0x00010000,
		eTransformFeedback = 0x01000000,
		eConditionalRendering = 0x00040000,
		eAccelerationStructureBuild = 0x02000000,
		eRayTracingShader = 0x00200000,
		eFragmentDensityProcess = 0x00800000,
		eFragmentShadingRateAttachment = 0x00400000,
		eTaskShader = 0x00080000,
		eMeshShader = 0x00100000,
		eCommandPreprocess = 0x00020000,
	};
	CRG_MakeFlags( PipelineStageFlags )

	enum class AccessFlags : uint32_t
	{
		eNone = 0,
		eIndirectCommandRead = 0x00000001,
		eIndexRead = 0x00000002,
		eVertexAttributeRead = 0x00000004,
		eUniformRead = 0x00000008,
		eInputAttachmentRead = 0x00000010,
		eShaderRead = 0x00000020,
		eShaderWrite = 0x00000040,
		eColorAttachmentRead = 0x00000080,
		eColorAttachmentWrite = 0x00000100,
		eDepthStencilAttachmentRead = 0x00000200,
		eDepthStencilAttachmentWrite = 0x00000400,
		eTransferRead = 0x00000800,
		eTransferWrite = 0x00001000,
		eHostRead = 0x00002000,
		eHostWrite = 0x00004000,
		eMemoryRead = 0x00008000,
		eMemoryWrite = 0x00010000,
		eTransformFeedbackWrite = 0x02000000,
		eTransformFeedback_counterRead = 0x04000000,
		eTransformFeedback_counterWrite = 0x08000000,
		eConditionalRenderingRead = 0x00100000,
		eColorAttachmentReadNonCoherent = 0x00080000,
		eAccelerationStructureRead = 0x00200000,
		eAccelerationStructureWrite = 0x00400000,
		eFragmentDensityMapRead = 0x01000000,
		eFragmentShadingRateAttachmentRead = 0x00800000,
		eCommandPreprocessRead = 0x00020000,
		eCommandPreprocessWrite = 0x00040000,
	};
	CRG_MakeFlags( AccessFlags )

	CRG_API std::string_view getName( PixelFormat v );
	CRG_API std::string_view getName( FilterMode v );
	CRG_API std::string_view getName( MipmapMode v );
	CRG_API std::string_view getName( WrapMode v );

	constexpr VkFormat convert( PixelFormat v )noexcept
	{
		return VkFormat( v );
	}

	constexpr PixelFormat convert( VkFormat v )noexcept
	{
		return PixelFormat( v );
	}

	constexpr bool isDepthFormat( PixelFormat fmt )noexcept
	{
		return fmt == PixelFormat::eD16_UNORM
			|| fmt == PixelFormat::eX8_D24_UNORM
			|| fmt == PixelFormat::eD32_SFLOAT
			|| fmt == PixelFormat::eD16_UNORM_S8_UINT
			|| fmt == PixelFormat::eD24_UNORM_S8_UINT
			|| fmt == PixelFormat::eD32_SFLOAT_S8_UINT;
	}

	constexpr bool isStencilFormat( PixelFormat fmt )noexcept
	{
		return fmt == PixelFormat::eS8_UINT
			|| fmt == PixelFormat::eD16_UNORM_S8_UINT
			|| fmt == PixelFormat::eD24_UNORM_S8_UINT
			|| fmt == PixelFormat::eD32_SFLOAT_S8_UINT;
	}

	constexpr bool isColourFormat( PixelFormat fmt )noexcept
	{
		return !isDepthFormat( fmt ) && !isStencilFormat( fmt );
	}

	constexpr bool isDepthStencilFormat( PixelFormat fmt )noexcept
	{
		return isDepthFormat( fmt ) && isStencilFormat( fmt );
	}

	constexpr VkImageType convert( ImageType v )noexcept
	{
		return VkImageType( v );
	}

	constexpr ImageType convert( VkImageType  v )noexcept
	{
		return ImageType( v );
	}

	constexpr VkImageViewType convert( ImageViewType v )noexcept
	{
		return VkImageViewType( v );
	}

	constexpr ImageViewType convert( VkImageViewType  v )noexcept
	{
		return ImageViewType( v );
	}

	constexpr VkImageLayout convert( ImageLayout v )noexcept
	{
		return VkImageLayout( v );
	}

	constexpr ImageLayout convert( VkImageLayout  v )noexcept
	{
		return ImageLayout( v );
	}

	constexpr VkFilter convert( FilterMode v )noexcept
	{
		return VkFilter( v );
	}

	constexpr FilterMode convert( VkFilter  v )noexcept
	{
		return FilterMode( v );
	}

	constexpr VkSamplerMipmapMode convert( MipmapMode v )noexcept
	{
		return VkSamplerMipmapMode( v );
	}

	constexpr MipmapMode convert( VkSamplerMipmapMode v )noexcept
	{
		return MipmapMode( v );
	}

	constexpr VkSamplerAddressMode convert( WrapMode v )noexcept
	{
		return VkSamplerAddressMode( v );
	}

	constexpr WrapMode convert( VkSamplerAddressMode v )noexcept
	{
		return WrapMode( v );
	}

	constexpr VkPipelineStageFlags convert( PipelineStageFlags v )noexcept
	{
		return VkPipelineStageFlags( v );
	}

	constexpr PipelineStageFlags getPipelineStageFlags( VkPipelineStageFlags v )noexcept
	{
		return PipelineStageFlags( v );
	}

	constexpr VkAccessFlags convert( AccessFlags v )noexcept
	{
		return VkAccessFlags( v );
	}

	constexpr AccessFlags getAccessFlags( VkAccessFlags v )noexcept
	{
		return AccessFlags( v );
	}

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
	struct FramePassGroup;
	struct RootNode;
	struct SamplerDesc;
	struct Texcoord;
	struct WriteDescriptorSet;

	class ContextResourcesCache;
	class Exception;
	class Fence;
	class FrameGraph;
	class FramePassTimer;
	class GraphVisitor;
	class RecordContext;
	class ResourceHandler;
	class ResourcesCache;
	class RunnableGraph;
	class RunnablePass;

	class ImageCopy;
	class PipelinePass;
	class RenderPass;
	class RenderQuad;

	using ImageId = Id< ImageData >;
	using ImageViewId = Id< ImageViewData >;
	using DependencyCache = std::unordered_map< size_t, bool >;
	using PassDependencyCache = std::unordered_map< FramePass const *, DependencyCache >;

	using FramePassPtr = std::unique_ptr< FramePass >;
	using FramePassGroupPtr = std::unique_ptr< FramePassGroup >;
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
	using FramePassGroupPtrArray = std::vector< FramePassGroupPtr >;
	using FrameGraphArray = std::vector< FrameGraph const * >;
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

	struct PipelineState
	{
		AccessFlags access;
		PipelineStageFlags pipelineStage;
	};

	struct LayoutState
	{
		ImageLayout layout;
		PipelineState state;
	};

	using AccessState = PipelineState;

	using MipLayoutStates = std::map< uint32_t, LayoutState >;
	using LayerLayoutStates = std::map< uint32_t, MipLayoutStates >;
	using LayoutStateMap = std::unordered_map< uint32_t, LayerLayoutStates >;
	using LayerLayoutStatesMap = std::map< uint32_t, LayerLayoutStates >;
	using AccessStateMap = std::unordered_map< VkBuffer, AccessState >;
	using ViewsLayout = LayoutStateMap;
	using BuffersLayout = AccessStateMap;
	using ViewsLayoutPtr = std::unique_ptr< ViewsLayout >;
	using BuffersLayoutPtr = std::unique_ptr< BuffersLayout >;
	using ViewsLayouts = std::vector< ViewsLayoutPtr >;
	using BuffersLayouts = std::vector< BuffersLayoutPtr >;

	using ViewLayoutIterators = std::map< uint32_t, ViewsLayouts::iterator >;
	using BufferLayoutIterators = std::map< uint32_t, BuffersLayouts::iterator >;

	template< typename VkTypeT >
	struct ContextObjectT
	{
		ContextObjectT( ContextObjectT const & rhs ) = delete;
		ContextObjectT( ContextObjectT && rhs )noexcept = delete;
		ContextObjectT & operator=( ContextObjectT const & rhs ) = delete;
		ContextObjectT & operator=( ContextObjectT && rhs )noexcept = delete;

		explicit ContextObjectT( GraphContext & ctx
			, VkTypeT obj = {}
			, void( *dtor )( GraphContext &, VkTypeT & )noexcept = nullptr )
			: context{ ctx }
			, object{ obj }
			, destroy{ dtor }
		{
		}

		~ContextObjectT()noexcept
		{
			if ( destroy && object )
			{
				destroy( context, object );
			}
		}

		GraphContext & context;
		VkTypeT object;
		void ( *destroy )( GraphContext &, VkTypeT & )noexcept;
	};

	struct Buffer
	{
		std::string name;

		Buffer( VkBufferArray pbuffers
			, std::string pname )noexcept
			: name{ std::move( pname ) }
			, m_buffers{ std::move( pbuffers ) }
		{
		}

		Buffer( VkBuffer buffer
			, std::string name )noexcept
			: Buffer{ VkBufferArray{ buffer }, std::move( name ) }
		{
		}

		VkBuffer const & buffer( uint32_t index = 0 )const noexcept
		{
			return m_buffers.size() == 1u
				? m_buffers.front()
				: m_buffers[index];
		}

		VkBuffer & buffer( uint32_t index = 0 )noexcept
		{
			return m_buffers.size() == 1u
				? m_buffers.front()
				: m_buffers[index];
		}

		size_t getCount()const noexcept
		{
			return m_buffers.size();
		}

	private:
		VkBufferArray m_buffers;

		friend CRG_API bool operator==( Buffer const & lhs, Buffer const & rhs );
	};

	CRG_API bool operator==( Buffer const & lhs, Buffer const & rhs );

	struct VertexBuffer
	{
		VertexBuffer( VertexBuffer const & rhs ) = delete;
		VertexBuffer & operator=( VertexBuffer const & rhs ) = delete;
		~VertexBuffer()noexcept = default;

		explicit VertexBuffer( Buffer pbuffer = { VkBuffer{}, std::string{} }
			, VkDeviceMemory pmemory = VkDeviceMemory{}
			, VkVertexInputAttributeDescriptionArray pvertexAttribs = {}
			, VkVertexInputBindingDescriptionArray pvertexBindings = {} )
			: buffer{ std::move( pbuffer ) }
			, memory{ pmemory }
			, vertexAttribs{ std::move( pvertexAttribs ) }
			, vertexBindings{ std::move( pvertexBindings ) }
		{
			inputState.vertexAttributeDescriptionCount = uint32_t( vertexAttribs.size() );
			inputState.pVertexAttributeDescriptions = vertexAttribs.data();
			inputState.vertexBindingDescriptionCount = uint32_t( vertexBindings.size() );
			inputState.pVertexBindingDescriptions = vertexBindings.data();
		}

		VertexBuffer( VertexBuffer && rhs )noexcept
			: buffer{ rhs.buffer }
			, memory{ rhs.memory }
			, vertexAttribs{ std::move( rhs.vertexAttribs ) }
			, vertexBindings{ std::move( rhs.vertexBindings ) }
			, inputState{ std::move( rhs.inputState ) }
		{
			inputState.vertexAttributeDescriptionCount = uint32_t( vertexAttribs.size() );
			inputState.pVertexAttributeDescriptions = vertexAttribs.data();
			inputState.vertexBindingDescriptionCount = uint32_t( vertexBindings.size() );
			inputState.pVertexBindingDescriptions = vertexBindings.data();
		}

		VertexBuffer & operator=( VertexBuffer && rhs )noexcept
		{
			buffer = rhs.buffer;
			memory = rhs.memory;
			vertexAttribs = std::move( rhs.vertexAttribs );
			vertexBindings = std::move( rhs.vertexBindings );
			inputState = std::move( rhs.inputState );

			inputState.vertexAttributeDescriptionCount = uint32_t( vertexAttribs.size() );
			inputState.pVertexAttributeDescriptions = vertexAttribs.data();
			inputState.vertexBindingDescriptionCount = uint32_t( vertexBindings.size() );
			inputState.pVertexBindingDescriptions = vertexBindings.data();

			return *this;
		}

		Buffer buffer{ VkBuffer{}, std::string{} };
		VkDeviceMemory memory{};
		VkVertexInputAttributeDescriptionArray vertexAttribs{};
		VkVertexInputBindingDescriptionArray vertexBindings{};
		VkPipelineVertexInputStateCreateInfo inputState{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, nullptr, {}, {}, {}, {}, {} };
	};

	using VertexBufferPtr = std::unique_ptr< VertexBuffer >;

	struct IndexBuffer
	{
		explicit IndexBuffer( Buffer pbuffer = { VkBuffer{}, std::string{} }
			, VkDeviceMemory pmemory = VkDeviceMemory{} )
			: buffer{ std::move( pbuffer ) }
			, memory{ pmemory }
		{
		}

		Buffer buffer;
		VkDeviceMemory memory;
	};

	struct IndirectBuffer
	{
		explicit IndirectBuffer( Buffer pbuffer
			, uint32_t pstride
			, VkDeviceSize poffset = {} )
			: buffer{ std::move( pbuffer ) }
			, offset{ poffset }
			, stride{ pstride }
		{
		}

		Buffer buffer;
		VkDeviceSize offset;
		uint32_t stride;
	};

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
	static inline const TypeT defaultV = DefaultValueGetterT< TypeT >::get();

	template< typename TypeT >
	static inline TypeT getDefaultV()
	{
		return DefaultValueGetterT< TypeT >::get();
	}

	template<>
	struct DefaultValueGetterT< VkPipelineVertexInputStateCreateInfo >
	{
		static VkPipelineVertexInputStateCreateInfo get()
		{
			VkPipelineVertexInputStateCreateInfo const result{ []()
				{
					return VkPipelineVertexInputStateCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
						, nullptr
						, {}
						, {}
						, {}
						, {}
						, {} };
				}() };
			return result;
		}
	};

	template<>
	struct DefaultValueGetterT< IndirectBuffer >
	{
		static IndirectBuffer get()
		{
			IndirectBuffer const result{ Buffer{ VkBuffer{}, std::string{} }, 0u };
			return result;
		}
	};

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
		PipelineStageFlags dstStageMask;
	};

	using SemaphoreWaitArray = std::vector< SemaphoreWait >;

	CRG_API VkExtent3D getExtent( ImageId const & image )noexcept;
	CRG_API VkExtent3D getExtent( ImageViewId const & image )noexcept;
	CRG_API VkExtent3D getMipExtent( ImageViewId const & image )noexcept;
	CRG_API PixelFormat getFormat( ImageId const & image )noexcept;
	CRG_API PixelFormat getFormat( ImageViewId const & image )noexcept;
	CRG_API ImageType getImageType( ImageId const & image )noexcept;
	CRG_API ImageType getImageType( ImageViewId const & image )noexcept;
	CRG_API uint32_t getMipLevels( ImageId const & image )noexcept;
	CRG_API uint32_t getMipLevels( ImageViewId const & image )noexcept;
	CRG_API uint32_t getArrayLayers( ImageId const & image )noexcept;
	CRG_API uint32_t getArrayLayers( ImageViewId const & image )noexcept;
	CRG_API AccessFlags getAccessMask( ImageLayout layout )noexcept;
	CRG_API PipelineStageFlags getStageMask( ImageLayout layout )noexcept;
	CRG_API PipelineState getPipelineState( PipelineStageFlags flags )noexcept;
	CRG_API LayoutState makeLayoutState( ImageLayout layout )noexcept;
	CRG_API VkImageAspectFlags getAspectMask( PixelFormat format )noexcept;
	CRG_API LayoutState const & addSubresourceRangeLayout( LayerLayoutStates & ranges
		, VkImageSubresourceRange const & range
		, LayoutState const & newLayout );
	CRG_API LayoutState getSubresourceRangeLayout( LayerLayoutStates const & ranges
		, VkImageSubresourceRange const & range );
	CRG_API VkImageSubresourceRange getVirtualRange( ImageId const & image
		, ImageViewType viewType
		, VkImageSubresourceRange const & range )noexcept;
	CRG_API bool match( ImageViewData const & lhs, ImageViewData const & rhs )noexcept;
	CRG_API ImageViewId const & resolveView( ImageViewId const & view
		, uint32_t passIndex );

	CRG_API void convert( SemaphoreWaitArray const & toWait
		, std::vector< VkSemaphore > & semaphores
		, std::vector< VkPipelineStageFlags > & dstStageMasks );
	CRG_API VkQueryPool createQueryPool( GraphContext & context
		, std::string const & name
		, uint32_t passesCount );
}
