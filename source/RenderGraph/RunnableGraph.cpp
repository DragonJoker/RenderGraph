/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/RunnableGraph.hpp"
#include "RenderGraph/GraphVisitor.hpp"

#include <array>
#include <fstream>
#include <string>
#include <type_traits>

namespace crg
{
	namespace
	{
		std::ostream & operator<<( std::ostream & stream
			, std::vector< ImageViewId > const & values )
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
			, FramePass const & value )
		{
			stream << value.name;
			return stream;
		}

		class DotOutVisitor
			: public GraphVisitor
		{
		public:
			static void submit( std::ostream & stream
				, GraphAdjacentNode node
				, std::set< GraphNode const * > & visited )
			{
				DotOutVisitor vis{ stream, visited };
				node->accept( &vis );
			}

			static void submit( std::ostream & stream
				, GraphAdjacentNode node )
			{
				std::set< GraphNode const * > visited;
				submit( stream, node, visited );
			}

			static void submit( std::ostream & stream
				, AttachmentTransitionArray const & transitions )
			{
				stream << "digraph \"Transitions\" {\n";
				stream << "  rankdir = \"LR\"";

				for ( auto & transition : transitions )
				{
					std::string name{ transition.view.data->name + "\\nTrans. to\\n" + transition.dstAttach.name };
					stream << "    \"" << name << "\" [ shape=square ];\n";

					if ( transition.srcAttach.pass )
					{
						stream << "    \"" << transition.srcAttach.pass->name << "\" -> \"" << name << "\" [ label=\"" << transition.view.data->name << "\" ];\n";
					}
					else
					{
						stream << "    \"ExternalSource\" -> \"" << name << "\" [ label=\"" << transition.view.data->name << "\" ];\n";
					}

					if ( transition.dstAttach.pass )
					{
						stream << "    \"" << name << "\" -> \"" << transition.dstAttach.pass->name << "\" [ label=\"" << transition.view.data->name << "\" ];\n";
					}
					else
					{
						stream << "    \"" << name << "\" -> \"ExternalDestination\" [ label=\"" << transition.view.data->name << "\" ];\n";
					}
				}

				stream << "}\n";
			}

		private:
			DotOutVisitor( std::ostream & stream
				, std::set< GraphNode const * > & visited )
				: m_stream{ stream }
				, m_visited{ visited }
			{
			}

			void printEdge( GraphNode * lhs
				, GraphNode * rhs )
			{
				std::string sep;
				auto transitions = rhs->getAttachsToPrev( lhs );
				std::sort( transitions.begin()
					, transitions.end()
					, []( AttachmentTransition const & lhs, AttachmentTransition const & rhs )
					{
						return lhs.srcAttach.name < rhs.srcAttach.name;
					} );
				uint32_t index{ 1u };

				for ( auto & transition : transitions )
				{
					std::string name{ transition.view.data->name + "\\nfrom\\n" + transition.srcAttach.name + "\\nto\\n" + transition.dstAttach.name };
					m_stream << "    \"" << name << "\" [ shape=square ];\n";
					m_stream << "    \"" << lhs->getName() << "\" -> \"" << name << "\" [ label=\"" << transition.view.data->name << "\" ];\n";
					m_stream << "    \"" << name << "\" -> \"" << rhs->getName() << "\" [ label=\"" << transition.view.data->name << "\" ];\n";
				}
			}

			void submit( GraphNode * node )
			{
				submit( m_stream
					, node
					, m_visited );
			}

			void visitRootNode( RootNode * node )override
			{
				m_stream << "digraph \"" << node->getName() << "\" {\n";

				for ( auto & next : node->getNext() )
				{
					submit( next );
				}

				m_stream << "}\n";
			}

			void visitFramePassNode( FramePassNode * node )override
			{
				m_visited.insert( node );
				auto nexts = node->getNext();

				for ( auto & next : nexts )
				{
					printEdge( node, next );

					if ( m_visited.end() == m_visited.find( next ) )
					{
						submit( next );
					}
				}
			}

		private:
			std::ostream & m_stream;
			std::set< GraphNode const * > & m_visited;
		};

		void displayPasses( FrameGraph & value )
		{
			std::ofstream file{ value.getGraph()->getName() + ".dot" };
			DotOutVisitor::submit( file, value.getGraph() );
		}

		void displayTransitions( FrameGraph & value )
		{
			std::ofstream file{ value.getGraph()->getName() + "_transitions.dot" };
			DotOutVisitor::submit( file, value.getTransitions() );
		}

		void display( FrameGraph & value )
		{
			displayTransitions( value );
			displayPasses( value );
		}

		struct Quad
		{
			using Data = std::array< float, 2u >;
			struct Vertex
			{
				Data position;
				Data texture;
			};

			Vertex vertex[6];
		};

		size_t makeHash( bool texCoords
			, bool invertU
			, bool invertV )
		{
			return ( ( texCoords ? 0x01 : 0x00 ) << 0 )
				| ( ( invertU ? 0x01 : 0x00 ) << 1 )
				| ( ( invertV ? 0x01 : 0x00 ) << 2 );
		}

		std::string getName( VkFilter filter )
		{
			switch ( filter )
			{
			case VK_FILTER_NEAREST:
				return "Nearest";
			case VK_FILTER_LINEAR:
				return "Linear";
			case VK_FILTER_CUBIC_IMG:
				return "Cubic";
			default:
				return "Unknown";
			}
		}

		VkImageCreateInfo convert( ImageData const & data )
		{
			return data.info;
		}

		VkImageViewCreateInfo convert( ImageViewData const & data
			, ImageMemoryMap const & images )
		{
			auto it = images.find( data.image );
			assert( it != images.end() );
			auto result = data.info;
			result.image = it->second.first;
			return result;
		}

		class DfsVisitor
			: public GraphVisitor
		{
		public:
			static GraphAdjacentNodeArray submit( GraphAdjacentNode node )
			{
				GraphAdjacentNodeArray result;
				std::set< GraphNode const * > visited;
				DfsVisitor vis{ result, visited };
				node->accept( &vis );
				return result;
			}

		private:
			DfsVisitor( GraphAdjacentNodeArray & result
				, std::set< GraphNode const * > & visited )
				: m_result{ result }
				, m_visited{ visited }
			{
			}

			void visitRootNode( RootNode * node )override
			{
				m_result.push_back( node );

				for ( auto & next : node->getNext() )
				{
					next->accept( this );
				}
			}

			void visitFramePassNode( FramePassNode * node )override
			{
				m_result.push_back( node );
				auto nexts = node->getNext();

				for ( auto & next : nexts )
				{
					if ( m_visited.end() == m_visited.find( next ) )
					{
						m_visited.insert( next );
						next->accept( this );
					}
				}
			}

		private:
			GraphAdjacentNodeArray & m_result;
			std::set< GraphNode const * > & m_visited;
		};
	}

	RunnableGraph::RunnableGraph( FrameGraph graph
		, GraphContext context )
		: m_graph{ std::move( graph ) }
		, m_context{ std::move( context ) }
	{
		m_graph.compile();
		display( m_graph );
		doCreateImages();
		doCreateImageViews();
		auto dfsNodes = DfsVisitor::submit( m_graph.getGraph() );

		for ( auto & node : dfsNodes )
		{
			if ( node->getKind() == GraphNode::Kind::FramePass )
			{
				auto & renderPassNode = nodeCast< FramePassNode >( *node );
				m_passes.push_back( renderPassNode.getFramePass().createRunnable( m_context , *this ) );
			}
		}
	}

	RunnableGraph::~RunnableGraph()
	{
		for ( auto & vertexBuffer : m_vertexBuffers )
		{
			if ( vertexBuffer.second->memory )
			{
				crgUnregisterObject( m_context, vertexBuffer.second->memory );
				m_context.vkFreeMemory( m_context.device
					, vertexBuffer.second->memory
					, m_context.allocator );
			}

			if ( vertexBuffer.second->buffer )
			{
				crgUnregisterObject( m_context, vertexBuffer.second->buffer );
				m_context.vkDestroyBuffer( m_context.device
					, vertexBuffer.second->buffer
					, m_context.allocator );
			}
		}

		for ( auto & view : m_imageViews )
		{
			crgUnregisterObject( m_context, view.second );
			m_context.vkDestroyImageView( m_context.device, view.second, m_context.allocator );
		}

		for ( auto & img : m_images )
		{
			crgUnregisterObject( m_context, img.second.second );
			m_context.vkFreeMemory( m_context.device, img.second.second, m_context.allocator );
			crgUnregisterObject( m_context, img.second.first );
			m_context.vkDestroyImage( m_context.device, img.second.first, m_context.allocator );
		}

		for ( auto & sampler : m_samplers )
		{
			crgUnregisterObject( m_context, sampler.second );
			m_context.vkDestroySampler( m_context.device
				, sampler.second
				, m_context.allocator );
		}
	}

	void RunnableGraph::record()
	{
		for ( auto & pass : m_passes )
		{
			pass->record();
		}
	}

	void RunnableGraph::recordInto( VkCommandBuffer commandBuffer )
	{
		for ( auto & pass : m_passes )
		{
			pass->recordInto( commandBuffer );
		}
	}

	SemaphoreWait RunnableGraph::run( SemaphoreWait toWait
		, VkQueue queue )
	{
		auto result = toWait;

		for ( auto & pass : m_passes )
		{
			result = pass->run( result, queue );
		}

		return result;
	}

	VkImage RunnableGraph::getImage( ImageId const & image )const
	{
		auto it = m_images.find( image );

		if ( it == m_images.end() )
		{
			return VK_NULL_HANDLE;
		}

		return it->second.first;
	}

	VkImage RunnableGraph::getImage( Attachment const & attach )const
	{
		auto it = m_images.find( attach.view.data->image );

		if ( it == m_images.end() )
		{
			return VK_NULL_HANDLE;
		}

		return it->second.first;
	}

	VkImageView RunnableGraph::getImageView( ImageViewId const & image )const
	{
		auto it = m_imageViews.find( image );

		if ( it == m_imageViews.end() )
		{
			return VK_NULL_HANDLE;
		}

		return it->second;
	}

	VkImageView RunnableGraph::getImageView( Attachment const & attach )const
	{
		auto it = m_graph.m_attachViews.find( attach.view.data->name );

		if ( it == m_graph.m_attachViews.end()
			|| *it->second.data != *attach.view.data )
		{
			return VK_NULL_HANDLE;
		}

		auto viewIt = m_imageViews.find( it->second );
		assert( viewIt != m_imageViews.end() );
		return viewIt->second;
	}

	VertexBuffer const & RunnableGraph::createQuadVertexBuffer( bool texCoords
		, bool invertU
		, bool invertV )
	{
		auto hash = makeHash( texCoords, invertU, invertV );
		auto ires = m_vertexBuffers.emplace( hash, std::make_unique< VertexBuffer >() );

		if ( ires.second )
		{
			auto & vertexBuffer = ires.first->second;
			VkBufferCreateInfo createInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO
				, nullptr
				, 0u
				, 4u * sizeof( Quad::Vertex )
				, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
				, VK_SHARING_MODE_EXCLUSIVE
				, 0u
				, nullptr };
			auto res = m_context.vkCreateBuffer( m_context.device
				, &createInfo
				, m_context.allocator
				, &vertexBuffer->buffer );
			checkVkResult( res, "Vertex buffer creation" );
			crgRegisterObject( m_context, "QuadVertexBuffer", vertexBuffer->buffer );

			VkMemoryRequirements requirements{};
			m_context.vkGetBufferMemoryRequirements( m_context.device
				, vertexBuffer->buffer
				, &requirements );
			uint32_t deduced = m_context.deduceMemoryType( requirements.memoryTypeBits
				, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT );
			VkMemoryAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO
				, nullptr
				, requirements.size
				, deduced };
			res = m_context.vkAllocateMemory( m_context.device
				, &allocateInfo
				, m_context.allocator
				, &vertexBuffer->memory );
			checkVkResult( res, "Buffer memory allocation" );
			crgRegisterObject( m_context, "QuadVertexMemory", vertexBuffer->memory );

			res = m_context.vkBindBufferMemory( m_context.device
				, vertexBuffer->buffer
				, vertexBuffer->memory
				, 0u );
			checkVkResult( res, "Buffer memory binding" );

			Quad::Vertex * buffer{};
			res = m_context.vkMapMemory( m_context.device
				, vertexBuffer->memory
				, 0u
				, VK_WHOLE_SIZE
				, 0u
				, reinterpret_cast< void ** >( &buffer ) );
			checkVkResult( res, "Buffer memory mapping" );

			if ( buffer )
			{
				std::array< Quad::Vertex, 4u > vertexData{ Quad::Vertex{ { -1.0, -1.0 }
					, ( texCoords
						? Quad::Data{ ( invertU ? 1.0f : 0.0f ), ( invertV ? 1.0f : 0.0f ) }
						: Quad::Data{ 0.0f, 0.0f } ) }
					, Quad::Vertex{ { -1.0, +1.0 }
						, ( texCoords
							? Quad::Data{ ( invertU ? 1.0f : 0.0f ), ( invertV ? 0.0f : 1.0f ) }
							: Quad::Data{ 0.0f, 0.0f } ) }
					, Quad::Vertex{ { +1.0f, -1.0f }
						, ( texCoords
							? Quad::Data{ ( invertU ? 0.0f : 1.0f ), ( invertV ? 1.0f : 0.0f ) }
							: Quad::Data{ 0.0f, 0.0f } ) }
					, Quad::Vertex{ { +1.0f, +1.0f }
						, ( texCoords
							? Quad::Data{ ( invertU ? 0.0f : 1.0f ), ( invertV ? 0.0f : 1.0f ) }
							: Quad::Data{ 0.0f, 0.0f } ) } };
				std::copy( vertexData.begin(), vertexData.end(), buffer );

				VkMappedMemoryRange memoryRange{ VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE
					, 0u
					, vertexBuffer->memory
					, 0u
					, VK_WHOLE_SIZE };
				m_context.vkFlushMappedMemoryRanges( m_context.device, 1u, &memoryRange );
				m_context.vkUnmapMemory( m_context.device, vertexBuffer->memory );
			}

			vertexBuffer->vertexAttribs.push_back( { 0u, 0u, VK_FORMAT_R32G32_SFLOAT, offsetof( Quad::Vertex, position ) } );

			if ( texCoords )
			{
				vertexBuffer->vertexAttribs.push_back( { 1u, 0u, VK_FORMAT_R32G32_SFLOAT, offsetof( Quad::Vertex, texture ) } );
			}

			vertexBuffer->vertexBindings.push_back( { 0u, sizeof( Quad::Vertex ), VK_VERTEX_INPUT_RATE_VERTEX } );
			vertexBuffer->inputState = VkPipelineVertexInputStateCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
				, nullptr
				, 0u
				, uint32_t( vertexBuffer->vertexBindings.size() )
				, vertexBuffer->vertexBindings.data()
				, uint32_t( vertexBuffer->vertexAttribs.size() )
				, vertexBuffer->vertexAttribs.data() };
		}

		return *ires.first->second;
	}

	VkSampler RunnableGraph::createSampler( VkFilter filter )
	{
		auto ires = m_samplers.emplace( filter, VkSampler{} );

		if ( ires.second )
		{
			VkSamplerCreateInfo createInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO
				, nullptr
				, 0u
				, filter
				, filter
				, VK_SAMPLER_MIPMAP_MODE_LINEAR
				, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
				, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
				, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
				, 0.0f // mipLodBias
				, VK_FALSE // anisotropyEnable
				, 0.0f // maxAnisotropy
				, VK_FALSE // compareEnable
				, VK_COMPARE_OP_ALWAYS // compareOp
				, -1000.0f
				, 1000.0f
				, VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK
				, VK_FALSE };
			auto res = m_context.vkCreateSampler( m_context.device
				, &createInfo
				, m_context.allocator
				, &ires.first->second );
			checkVkResult( res, "Sampler creation" );
			crgRegisterObject( m_context, getName( filter ) + "Sampler", ires.first->second );
		}

		return ires.first->second;
	}

	void RunnableGraph::memoryBarrier( VkCommandBuffer commandBuffer
		, VkPipelineStageFlagBits srcStageMask
		, VkPipelineStageFlagBits dstStageMask
		, Attachment const & attach
		, VkImageLayout wantedLayout )
	{
	}

	void RunnableGraph::doCreateImages()
	{
		for ( auto & img : m_graph.m_images )
		{
			// Create image
			VkImage image;
			auto createInfo = convert( *img.first.data );
			auto res = m_context.vkCreateImage( m_context.device
				, &createInfo
				, m_context.allocator
				, &image );
			checkVkResult( res, "Image creation" );
			crgRegisterObject( m_context, img.first.data->name, image );

			// Create Image memory
			VkMemoryRequirements requirements{};
			m_context.vkGetImageMemoryRequirements( m_context.device
				, image
				, &requirements );
			uint32_t deduced = m_context.deduceMemoryType( requirements.memoryTypeBits
				, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
			VkMemoryAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO
				, nullptr
				, requirements.size
				, deduced };
			VkDeviceMemory memory;
			res = m_context.vkAllocateMemory( m_context.device
				, &allocateInfo
				, m_context.allocator
				, &memory );
			checkVkResult( res, "Image memory allocation" );
			crgRegisterObject( m_context, img.first.data->name, memory );

			// Bind image and memory
			res = m_context.vkBindImageMemory( m_context.device
				, image
				, memory
				, 0u );
			checkVkResult( res, "Image memory binding" );
			m_images[img.first] = { image, memory };
		}
	}

	void RunnableGraph::doCreateImageViews()
	{
		for ( auto & view : m_graph.m_imageViews )
		{
			VkImageView imageView;
			auto createInfo = convert( *view.first.data, m_images );
			auto res = m_context.vkCreateImageView( m_context.device
				, &createInfo
				, m_context.allocator
				, &imageView );
			checkVkResult( res, "ImageView creation" );
			crgRegisterObject( m_context, view.first.data->name, imageView );
			m_imageViews[view.first] = imageView;
		}
	}
}
