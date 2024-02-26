#include "Common.hpp"

#include <RenderGraph/Attachment.hpp>
#include <RenderGraph/FramePass.hpp>
#include <RenderGraph/FrameGraph.hpp>
#include <RenderGraph/ImageData.hpp>
#include <RenderGraph/ResourceHandler.hpp>
#include <RenderGraph/RunnablePasses/BufferCopy.hpp>
#include <RenderGraph/RunnablePasses/BufferToImageCopy.hpp>
#include <RenderGraph/RunnablePasses/ComputePass.hpp>
#include <RenderGraph/RunnablePasses/GenerateMipmaps.hpp>
#include <RenderGraph/RunnablePasses/ImageBlit.hpp>
#include <RenderGraph/RunnablePasses/ImageCopy.hpp>
#include <RenderGraph/RunnablePasses/ImageToBufferCopy.hpp>
#include <RenderGraph/RunnablePasses/RenderMesh.hpp>
#include <RenderGraph/RunnablePasses/RenderPass.hpp>
#include <RenderGraph/RunnablePasses/RenderQuad.hpp>

namespace
{
	crg::GraphContext & getContext()
	{
		return test::getDummyContext();
	}

	void testBufferCopy( test::TestCounts & testCounts )
	{
		testBegin( "testBufferCopy" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto & testPass = graph.createPass( "Pass"
			, []( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return std::make_unique< crg::BufferCopy >( pass, context, runGraph
					, 0u, 1024u );
			} );
		testPass.addInputStorageBuffer( crg::Buffer{ nullptr, "inBuffer" }, 0u, 0u, 1024u );
		testPass.addOutputStorageBuffer( crg::Buffer{ nullptr, "outBuffer" }, 1u, 0u, 1024u );

		auto runnable = graph.compile( getContext() );
		require( runnable );
		checkNoThrow( runnable->record() )
		checkNoThrow( runnable->run( VkQueue{} ) )
		testEnd()
	}

	void testBufferToImageCopy( test::TestCounts & testCounts )
	{
		testBegin( "testBufferToImageCopy" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto result = graph.createImage( test::createImage( "result", VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto resultv = graph.createView( test::createView( "resultv", result, VK_FORMAT_R16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
		auto & testPass = graph.createPass( "Pass"
			, []( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return std::make_unique< crg::BufferToImageCopy >( pass, context, runGraph
					, VkOffset3D{}, VkExtent3D{ 1024, 1024, 1u } );
			} );
		testPass.addInputStorageBuffer( crg::Buffer{ nullptr, "inBuffer" }, 0u, 0u, 1024u );
		testPass.addTransferOutputView( resultv );

		auto runnable = graph.compile( getContext() );
		require( runnable );
		checkNoThrow( runnable->record() )
		checkNoThrow( runnable->run( VkQueue{} ) )
		testEnd()
	}

	void testGenerateMipmaps( test::TestCounts & testCounts )
	{
		testBegin( "testGenerateMipmaps" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto result = graph.createImage( test::createImage( "result", VK_FORMAT_R16G16B16A16_SFLOAT, 10u ) );
		auto resultv = graph.createView( test::createView( "resultv", result, VK_FORMAT_R16G16B16A16_SFLOAT, 0u, 10u, 0u, 1u ) );
		auto & testPass = graph.createPass( "Pass"
			, []( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return std::make_unique< crg::GenerateMipmaps >( pass, context, runGraph );
			} );
		testPass.addTransferInOutView( resultv );

		auto runnable = graph.compile( getContext() );
		require( runnable );
		checkNoThrow( runnable->record() )
		checkNoThrow( runnable->run( VkQueue{} ) )
		testEnd()
	}

	void testImageBlit( test::TestCounts & testCounts )
	{
		testBegin( "testImageBlit" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto input = graph.createImage( test::createImage( "input", VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto result = graph.createImage( test::createImage( "result", VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto inputv = graph.createView( test::createView( "inputv", input, VK_FORMAT_R16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
		auto resultv = graph.createView( test::createView( "resultv", result, VK_FORMAT_R16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
		auto & testPass = graph.createPass( "Pass"
			, [inputv, resultv]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return std::make_unique< crg::ImageBlit >( pass, context, runGraph
					, VkOffset3D{}, getExtent( inputv )
					, VkOffset3D{}, getExtent( resultv )
					, VK_FILTER_LINEAR );
			} );
		testPass.addTransferInputView( inputv );
		testPass.addTransferOutputView( resultv );

		auto runnable = graph.compile( getContext() );
		require( runnable );
		checkNoThrow( runnable->record() )
		checkNoThrow( runnable->run( VkQueue{} ) )
		testEnd()
	}

	void testImageCopy( test::TestCounts & testCounts )
	{
		testBegin( "testImageCopy" )
		{
			crg::ResourceHandler handler;
			crg::FrameGraph graph{ handler, testCounts.testName };
			auto input = graph.createImage( test::createImage( "input", VK_FORMAT_R16G16B16A16_SFLOAT ) );
			auto result = graph.createImage( test::createImage( "result", VK_FORMAT_R16G16B16A16_SFLOAT ) );
			auto inputv = graph.createView( test::createView( "inputv", input, VK_FORMAT_R16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
			auto resultv = graph.createView( test::createView( "resultv", result, VK_FORMAT_R16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
			auto & testPass = graph.createPass( "Pass"
				, [inputv]( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
						return std::make_unique< crg::ImageCopy >( pass, context, runGraph
							, getExtent( inputv ) );
				} );
			testPass.addTransferInputView( inputv );
			testPass.addTransferOutputView( resultv );

			auto runnable = graph.compile( getContext() );
			require( runnable );
			checkNoThrow( runnable->record() )
			checkNoThrow( runnable->run( VkQueue{} ) )
		}
		{
			crg::ResourceHandler handler;
			crg::FrameGraph graph{ handler, testCounts.testName };
			auto input = graph.createImage( test::createImage( "input", VK_FORMAT_R16G16B16A16_SFLOAT ) );
			auto result = graph.createImage( test::createImage( "result", VK_FORMAT_R16G16B16A16_SFLOAT ) );
			auto inputv = graph.createView( test::createView( "inputv", input, VK_FORMAT_R16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
			auto resultv = graph.createView( test::createView( "resultv", result, VK_FORMAT_R16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
			auto & testPass = graph.createPass( "Pass"
				, [inputv]( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
						return std::make_unique< crg::ImageCopy >( pass, context, runGraph
							, getExtent( inputv )
							, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
				} );
			testPass.addTransferInputView( inputv );
			testPass.addTransferOutputView( resultv );

			auto runnable = graph.compile( getContext() );
			require( runnable );
			checkNoThrow( runnable->record() )
			checkNoThrow( runnable->run( VkQueue{} ) )
		}
		{
			crg::ResourceHandler handler;
			crg::FrameGraph graph{ handler, testCounts.testName };
			auto input = graph.createImage( test::createImage( "input", VK_FORMAT_R16G16B16A16_SFLOAT ) );
			auto result = graph.createImage( test::createImage( "result", VK_FORMAT_R16G16B16A16_SFLOAT ) );
			auto inputv = graph.createView( test::createView( "inputv", input, VK_FORMAT_R16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
			auto resultv = graph.createView( test::createView( "resultv", result, VK_FORMAT_R16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
			auto & testPass = graph.createPass( "Pass"
				, [inputv]( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
						return std::make_unique< crg::ImageCopy >( pass, context, runGraph
							, getExtent( inputv )
							, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
				} );
			testPass.addTransferInputView( inputv );
			testPass.addTransferOutputView( resultv );
			testPass.addTransferInputView( resultv );
			testPass.addTransferOutputView( inputv );

			auto runnable = graph.compile( getContext() );
			require( runnable );
			checkNoThrow( runnable->record() )
			checkNoThrow( runnable->run( VkQueue{} ) )
		}
		testEnd()
	}

	void testImageToBufferCopy( test::TestCounts & testCounts )
	{
		testBegin( "testImageToBufferCopy" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto input = graph.createImage( test::createImage( "input", VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto inputv = graph.createView( test::createView( "inputv", input, VK_FORMAT_R16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
		auto & testPass = graph.createPass( "Pass"
			, [inputv]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return std::make_unique< crg::ImageToBufferCopy >( pass, context, runGraph
					, VkOffset3D{}, getExtent( inputv ) );
			} );
		testPass.addTransferInputView( inputv );
		testPass.addOutputStorageBuffer( crg::Buffer{ nullptr, "outBuffer" }, 1u, 0u, 1024u );

		auto runnable = graph.compile( getContext() );
		require( runnable );
		checkNoThrow( runnable->record() )
		checkNoThrow( runnable->run( VkQueue{} ) )
		testEnd()
	}

	void testComputePass( test::TestCounts & testCounts )
	{
		testBegin( "testComputePass" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto result = graph.createImage( test::createImage( "result", VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto resultv = graph.createView( test::createView( "resultv", result, VK_FORMAT_R16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
		auto depth = graph.createImage( test::createImage( "result", VK_FORMAT_D32_SFLOAT ) );
		auto depthv = graph.createView( test::createView( "resultv", depth, VK_FORMAT_D32_SFLOAT, 0u, 1u, 0u, 1u ) );
		auto & testPass = graph.createPass( "Pass"
			, []( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				crg::cp::Config cfg;
				cfg.baseConfig( crg::pp::Config{}
					.programCreator( crg::ProgramCreator{ 1u
						, []( uint32_t )
						{
							return crg::VkPipelineShaderStageCreateInfoArray{ VkPipelineShaderStageCreateInfo{} };
						} } ) );
				return std::make_unique< crg::ComputePass >( pass, context, runGraph
					, crg::ru::Config{}, cfg );
			} );
		testPass.addInOutStorageBuffer( crg::Buffer{ nullptr, "buffer1" }, 0u, 0u, 1024u );
		testPass.addClearableOutputStorageBuffer( crg::Buffer{ nullptr, "buffer2" }, 1u, 0u, 1024u );
		testPass.addClearableOutputStorageView( resultv, 2u );
		testPass.addClearableOutputStorageView( depthv, 3u );

		auto runnable = graph.compile( getContext() );
		require( runnable );
		checkNoThrow( runnable->record() )
		checkNoThrow( runnable->run( VkQueue{} ) )
		testEnd()
	}

	void testRenderPass( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass" )
		{
			crg::ResourceHandler handler;
			crg::FrameGraph graph{ handler, testCounts.testName };
			auto result = graph.createImage( test::createImage( "result", VK_FORMAT_R16G16B16A16_SFLOAT ) );
			auto resultv = graph.createView( test::createView( "resultv", result, VK_FORMAT_R16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
			auto & testPass = graph.createPass( "Pass"
				, []( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
						return std::make_unique< crg::RenderPass >( pass, context, runGraph
							, crg::RenderPass::Callbacks{ crg::defaultV< crg::RunnablePass::InitialiseCallback >
								, crg::defaultV< crg::RunnablePass::RecordCallback > } );
				} );
			testPass.addOutputColourView( resultv );

			auto runnable = graph.compile( getContext() );
			require( runnable );
			checkNoThrow( runnable->record() )
			checkNoThrow( runnable->run( VkQueue{} ) )
		}
		{
			crg::ResourceHandler handler;
			crg::FrameGraph graph{ handler, testCounts.testName };
			auto depth = graph.createImage( test::createImage( "depth", VK_FORMAT_D32_SFLOAT ) );
			auto depthv = graph.createView( test::createView( "depthv", depth, VK_FORMAT_D32_SFLOAT, 0u, 1u, 0u, 1u ) );
			auto & testPass = graph.createPass( "Pass"
				, []( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
					return std::make_unique< crg::RenderPass >( pass, context, runGraph
						, crg::RenderPass::Callbacks{ crg::defaultV< crg::RunnablePass::InitialiseCallback >
							, crg::defaultV< crg::RunnablePass::RecordCallback > } );
				} );
			testPass.addOutputDepthView( depthv );

			auto runnable = graph.compile( getContext() );
			require( runnable );
			checkNoThrow( runnable->record() )
			checkNoThrow( runnable->run( VkQueue{} ) )
		}
		{
			crg::ResourceHandler handler;
			crg::FrameGraph graph{ handler, testCounts.testName };
			auto result = graph.createImage( test::createImage( "result", VK_FORMAT_R16G16B16A16_SFLOAT ) );
			auto resultv = graph.createView( test::createView( "resultv", result, VK_FORMAT_R16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
			auto depth = graph.createImage( test::createImage( "depth", VK_FORMAT_D32_SFLOAT ) );
			auto depthv = graph.createView( test::createView( "depthv", depth, VK_FORMAT_D32_SFLOAT, 0u, 1u, 0u, 1u ) );
			auto & testPass = graph.createPass( "Pass"
				, []( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
					return std::make_unique< crg::RenderPass >( pass, context, runGraph
						, crg::RenderPass::Callbacks{ crg::defaultV< crg::RunnablePass::InitialiseCallback >
							, crg::defaultV< crg::RunnablePass::RecordCallback > } );
				} );
			testPass.addOutputColourView( resultv );
			testPass.addOutputDepthView( depthv );

			auto runnable = graph.compile( getContext() );
			require( runnable );
			checkNoThrow( runnable->record() )
			checkNoThrow( runnable->run( VkQueue{} ) )
		}
		{
			crg::ResourceHandler handler;
			crg::FrameGraph graph{ handler, testCounts.testName };
			auto result1 = graph.createImage( test::createImage( "result1", VK_FORMAT_R16G16B16A16_SFLOAT, 1u, 2u ) );
			auto result1v = graph.createView( test::createView( "result1v", result1, VK_FORMAT_R16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
			auto result2v = graph.createView( test::createView( "result2v", result1, VK_FORMAT_R16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
			auto & testPass = graph.createPass( "Pass"
				, []( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
					return std::make_unique< crg::RenderPass >( pass, context, runGraph
						, crg::RenderPass::Callbacks{ crg::defaultV< crg::RunnablePass::InitialiseCallback >
							, crg::defaultV< crg::RunnablePass::RecordCallback > } );
				} );
			testPass.addOutputColourView( testPass.mergeViews( { result1v, result2v } ) );

			auto runnable = graph.compile( getContext() );
			require( runnable );
			checkNoThrow( runnable->record() )
			checkNoThrow( runnable->run( VkQueue{} ) )
		}
		testEnd()
	}

	void testRenderQuad( test::TestCounts & testCounts )
	{
		testBegin( "testRenderQuad" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto result = graph.createImage( test::createImage( "result", VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto resultv = graph.createView( test::createView( "resultv", result, VK_FORMAT_R16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
		auto & testPass = graph.createPass( "Pass"
			, []( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				crg::rq::Config cfg;
				cfg.baseConfig( crg::pp::Config{}
					.programCreator( crg::ProgramCreator{ 1u
						, []( uint32_t )
						{
							return crg::VkPipelineShaderStageCreateInfoArray{ VkPipelineShaderStageCreateInfo{} };
						} } ) );
				return std::make_unique< crg::RenderQuad >( pass, context, runGraph
					, crg::ru::Config{}, cfg );
			} );
		testPass.addOutputColourView( resultv );

		auto runnable = graph.compile( getContext() );
		require( runnable );
		checkNoThrow( runnable->record() )
		checkNoThrow( runnable->run( VkQueue{} ) )
		testEnd()
	}

	void testRenderMesh( test::TestCounts & testCounts )
	{
		testBegin( "testRenderMesh" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto result = graph.createImage( test::createImage( "result", VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto resultv = graph.createView( test::createView( "resultv", result, VK_FORMAT_R16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
		auto & testPass = graph.createPass( "Pass"
			, []( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				crg::rm::Config cfg;
				cfg.baseConfig( crg::pp::Config{}
					.programCreator( crg::ProgramCreator{ 1u
						, []( uint32_t )
						{
							return crg::VkPipelineShaderStageCreateInfoArray{ VkPipelineShaderStageCreateInfo{} };
						} } ) );
				return std::make_unique< crg::RenderMesh >( pass, context, runGraph
					, crg::ru::Config{}, cfg );
			} );
		testPass.addOutputColourView( resultv );

		auto runnable = graph.compile( getContext() );
		require( runnable );
		checkNoThrow( runnable->record() )
		checkNoThrow( runnable->run( VkQueue{} ) )
		testEnd()
	}
}

int main( int argc, char ** argv )
{
	testSuiteBegin( "TestRunnablePass" );
	testBufferCopy( testCounts );
	testBufferToImageCopy( testCounts );
	testGenerateMipmaps( testCounts );
	testImageBlit( testCounts );
	testImageCopy( testCounts );
	testImageToBufferCopy( testCounts );
	testComputePass( testCounts );
	testRenderPass( testCounts );
	testRenderQuad( testCounts );
	testRenderMesh( testCounts );
	testSuiteEnd();
}
