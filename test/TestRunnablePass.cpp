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

#include <sstream>

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
		{
			crg::FrameGraph graph{ handler, testCounts.testName };
			auto & testPass = graph.createPass( "Pass"
				, []( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
					return std::make_unique< crg::BufferCopy >( pass, context, runGraph
						, 0u, 1024u );
				} );
			testPass.addTransferInputBuffer( crg::Buffer{ VkBuffer( 1 ), "inBuffer" }, 0u, 1024u );
			testPass.addTransferOutputBuffer( crg::Buffer{ VkBuffer( 1 ), "outBuffer" }, 0u, 1024u );

			auto runnable = graph.compile( getContext() );
			test::checkRunnable( testCounts, runnable );
		}
		{
			crg::FrameGraph graph{ handler, testCounts.testName };
			auto & testPass = graph.createPass( "Pass"
				, []( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
					return std::make_unique< crg::BufferCopy >( pass, context, runGraph
						, 0u, 512u );
				} );
			testPass.addTransferInOutBuffer( crg::Buffer{ VkBuffer( 1 ), "ioBuffer" }, 0u, 512u );
			testPass.addTransferInOutBuffer( crg::Buffer{ VkBuffer( 2 ), "ioBuffer" }, 512u, 1024u );

			auto runnable = graph.compile( getContext() );
			test::checkRunnable( testCounts, runnable );
		}
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
		testPass.addTransferInputBuffer( crg::Buffer{ VkBuffer( 1 ), "inBuffer" }, 0u, 1024u );
		testPass.addTransferOutputView( resultv );

		auto runnable = graph.compile( getContext() );
		test::checkRunnable( testCounts, runnable );
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
		test::checkRunnable( testCounts, runnable );
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
		test::checkRunnable( testCounts, runnable );
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
			test::checkRunnable( testCounts, runnable );
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
			test::checkRunnable( testCounts, runnable );
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
			test::checkRunnable( testCounts, runnable );
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
		testPass.addTransferOutputBuffer( crg::Buffer{ VkBuffer( 1 ), "outBuffer" }, 0u, 1024u );

		auto runnable = graph.compile( getContext() );
		test::checkRunnable( testCounts, runnable );
		testEnd()
	}

	void testComputePass( test::TestCounts & testCounts )
	{
		testBegin( "testComputePass" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, "/" + testCounts.testName };
		auto & testPass1 = graph.createPass( "Pass1"
			, []( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				crg::cp::Config cfg;
				cfg.indirectBuffer( crg::IndirectBuffer{ crg::Buffer{ VkBuffer( 1 ), "ind" }, sizeof( VkDrawIndirectCommand ) } );
				cfg.baseConfig( crg::pp::Config{}
					.programCreator( crg::ProgramCreator{ 1u
						, []( uint32_t ){ return crg::VkPipelineShaderStageCreateInfoArray{ VkPipelineShaderStageCreateInfo{} }; } } ) );
				return std::make_unique< crg::ComputePass >( pass, context, runGraph
					, crg::ru::Config{}, std::move( cfg ) );
			} );
		testPass1.addClearableOutputStorageBuffer( crg::Buffer{ VkBuffer( 1 ), "buffer1" }, 1u, 0u, 1024u );

		auto result = graph.createImage( test::createImage( "result", VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto resultv = graph.createView( test::createView( "resultv", result, VK_FORMAT_R16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
		auto depth = graph.createImage( test::createImage( "depth", VK_FORMAT_D32_SFLOAT ) );
		auto depthv = graph.createView( test::createView( "depthv", depth, VK_FORMAT_D32_SFLOAT, 0u, 1u, 0u, 1u ) );
		auto depthStencil = graph.createImage( test::createImage( "depthStencil", VK_FORMAT_D32_SFLOAT_S8_UINT ) );
		auto depthStencilv = graph.createView( test::createView( "depthStencilv", depthStencil, VK_FORMAT_D32_SFLOAT_S8_UINT, 0u, 1u, 0u, 1u ) );
		crg::ComputePass * computePass{};
		auto & testPass2 = graph.createPass( "Pass2"
			, [&computePass]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				crg::cp::Config cfg;
				cfg.baseConfig( crg::pp::Config{}
					.programCreator( crg::ProgramCreator{ 1u
						, []( uint32_t ){ return crg::VkPipelineShaderStageCreateInfoArray{ VkPipelineShaderStageCreateInfo{} }; } } ) );
				auto res = std::make_unique< crg::ComputePass >( pass, context, runGraph
					, crg::ru::Config{ 1u, true }, std::move( cfg ) );
				computePass = res.get();
				return res;
			} );
		testPass2.addDependency( testPass1 );
		testPass2.addUniformBuffer( crg::Buffer{ VkBuffer( 1 ), "buffer1" }, 0u, 0u, 1024u );
		testPass2.addClearableOutputStorageView( resultv, 2u );
		testPass2.addClearableOutputStorageView( depthv, 3u );

		auto & testPass3 = graph.createPass( "Pass3"
			, []( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				crg::cp::Config cfg;
				cfg.baseConfig( crg::pp::Config{}
					.programCreator( crg::ProgramCreator{ 1u
						, []( uint32_t ){ return crg::VkPipelineShaderStageCreateInfoArray{ VkPipelineShaderStageCreateInfo{} }; } } ) );
				return std::make_unique< crg::ComputePass >( pass, context, runGraph
					, crg::ru::Config{}, std::move( cfg ) );
			} );
		testPass3.addDependency( testPass2 );
		testPass3.addInputStorageBuffer( crg::Buffer{ VkBuffer( 2 ), "buffer1" }, 0u, 0u, 1024u );
		testPass3.addInOutStorageBuffer( crg::Buffer{ VkBuffer( 3 ), "buffer2" }, 0u, 0u, 1024u );
		testPass3.addUniformBufferView( crg::Buffer{ VkBuffer( 4 ), "buffer3" }, VkBufferView{}, 0u, 0u, 1024u );
		testPass3.addInputStorageBufferView( crg::Buffer{ VkBuffer( 5 ), "buffer4" }, VkBufferView{}, 0u, 0u, 1024u );
		testPass3.addOutputStorageBufferView( crg::Buffer{ VkBuffer( 6 ), "buffer5" }, VkBufferView{}, 0u, 0u, 1024u );
		testPass3.addInOutStorageBufferView( crg::Buffer{ VkBuffer( 7 ), "buffer6" }, VkBufferView{}, 0u, 0u, 1024u );
		testPass3.addImplicitColourView( resultv, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
		testPass3.addImplicitDepthView( depthv, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
		testPass3.addImplicitDepthStencilView( depthStencilv, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );

		auto runnable = graph.compile( getContext() );
		test::checkRunnable( testCounts, runnable );
		require( computePass )
		checkNoThrow( computePass->resetPipeline( crg::VkPipelineShaderStageCreateInfoArray{ VkPipelineShaderStageCreateInfo{} }, 0u ) )
		checkNoThrow( computePass->resetCommandBuffer( 0u ) )
		checkNoThrow( runnable->record() )
		checkNoThrow( runnable->run( VkQueue{} ) )
		checkNoThrow( graph.getFinalAccessState( crg::Buffer{ VkBuffer( 1 ), "buffer1" }, 0u ) )
		testEnd()
	}

	void testComputePassTransitions( test::TestCounts & testCounts )
	{
		testBegin( "testComputePassTransitions" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, "/" + testCounts.testName };
		auto result = graph.createImage( test::createImage( "result", VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto resultv = graph.createView( test::createView( "resultv", result, VK_FORMAT_R16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
		auto depth = graph.createImage( test::createImage( "depth", VK_FORMAT_D32_SFLOAT ) );
		auto depthv = graph.createView( test::createView( "depthv", depth, VK_FORMAT_D32_SFLOAT, 0u, 1u, 0u, 1u ) );
		crg::Buffer buffer1{ VkBuffer( 1 ), "buffer1" };
		crg::Buffer buffer2{ VkBuffer( 2 ), "buffer2" };
		auto & testPass1 = graph.createPass( "Pass1"
			, []( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				crg::cp::Config cfg;
				cfg.baseConfig( crg::pp::Config{}
					.programCreator( crg::ProgramCreator{ 1u
						, []( uint32_t ){ return crg::VkPipelineShaderStageCreateInfoArray{ VkPipelineShaderStageCreateInfo{} }; } } ) );
				return std::make_unique< crg::ComputePass >( pass, context, runGraph
					, crg::ru::Config{ 1u, true }, std::move( cfg ) );
			} );
		testPass1.addClearableOutputStorageView( resultv, 0u );
		testPass1.addClearableOutputStorageView( depthv, 1u );
		testPass1.addClearableOutputStorageBuffer( buffer1, 2u, 0u, 1024u );
		testPass1.addClearableOutputStorageBuffer( buffer2, 3u, 0u, 1024u );

		auto & testPass2 = graph.createPass( "Pass2"
			, []( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				crg::cp::Config cfg;
				cfg.baseConfig( crg::pp::Config{}
					.programCreator( crg::ProgramCreator{ 1u
						, []( uint32_t ){ return crg::VkPipelineShaderStageCreateInfoArray{ VkPipelineShaderStageCreateInfo{} }; } } ) );
				return std::make_unique< crg::ComputePass >( pass, context, runGraph
					, crg::ru::Config{}, std::move( cfg ) );
			} );
		testPass2.addDependency( testPass1 );
		testPass2.addImplicitColourView( resultv, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
		testPass2.addImplicitDepthView( depthv, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
		testPass2.addImplicitBuffer( buffer1, 0u, 1024u, {} );
		testPass2.addImplicitBuffer( buffer2, 0u, 1024u, {} );

		auto runnable = graph.compile( getContext() );
		test::checkRunnable( testCounts, runnable );
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
			test::checkRunnable( testCounts, runnable );
		}
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
								, crg::defaultV< crg::RunnablePass::RecordCallback >
								, crg::defaultV< crg::RenderPass::GetSubpassContentsCallback  > } );
				} );
			testPass.addOutputColourView( resultv );

			auto runnable = graph.compile( getContext() );
			test::checkRunnable( testCounts, runnable );
		}
		{
			crg::ResourceHandler handler;
			crg::FrameGraph graph{ handler, testCounts.testName };
			auto result = graph.createImage( test::createImage( "result", VK_FORMAT_R16G16B16A16_SFLOAT, 1u, 2u ) );
			auto result1v = graph.createView( test::createView( "result1v", result, VK_FORMAT_R16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
			auto result2v = graph.createView( test::createView( "result2v", result, VK_FORMAT_R16G16B16A16_SFLOAT, 0u, 1u, 1u, 1u ) );
			auto & testPass = graph.createPass( "Pass"
				, []( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
						return std::make_unique< crg::RenderPass >( pass, context, runGraph
							, crg::RenderPass::Callbacks{ crg::defaultV< crg::RunnablePass::InitialiseCallback >
								, crg::defaultV< crg::RunnablePass::RecordCallback >
								, crg::defaultV< crg::RenderPass::GetSubpassContentsCallback  > } );
				} );
			testPass.addOutputColourView( testPass.mergeViews( { result1v, result2v } ) );

			auto runnable = graph.compile( getContext() );
			test::checkRunnable( testCounts, runnable );
			check( graph.getFinalLayoutState( testPass.mergeViews( { result1v, result2v } ), 0u ).layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL )
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
			test::checkRunnable( testCounts, runnable );
		}
		{
			crg::ResourceHandler handler;
			crg::FrameGraph graph{ handler, testCounts.testName };
			uint32_t passIndex{};
			auto result = graph.createImage( test::createImage( "result", VK_FORMAT_R16G16B16A16_SFLOAT ) );
			auto resultv = graph.createView( test::createView( "resultv", result, VK_FORMAT_R16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
			auto depth = graph.createImage( test::createImage( "depth", VK_FORMAT_D32_SFLOAT ) );
			auto depthv = graph.createView( test::createView( "depthv", depth, VK_FORMAT_D32_SFLOAT, 0u, 1u, 0u, 1u ) );
			VkExtent2D extent{ getExtent( resultv ).width, getExtent( resultv ).height };
			auto & testPass = graph.createPass( "Pass"
				, [&passIndex, extent]( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
					return std::make_unique< crg::RenderPass >( pass, context, runGraph
						, crg::RenderPass::Callbacks{ crg::defaultV< crg::RunnablePass::InitialiseCallback >
							, crg::defaultV< crg::RunnablePass::RecordCallback >
							, crg::defaultV< crg::RenderPass::GetSubpassContentsCallback >
							, crg::RenderPass::GetPassIndexCallback( [passIndex](){ return passIndex; } ) }
						, extent
						, crg::ru::Config{ 2u } );
				} );
			testPass.addOutputColourView( resultv );
			testPass.addOutputDepthView( depthv );

			auto runnable = graph.compile( getContext() );
			test::checkRunnable( testCounts, runnable );
			passIndex = 1u - passIndex;
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
			testPass.addOutputColourView( testPass.mergeViews( { result1v, result2v }, false, true ) );
			testPass.addOutputColourView( testPass.mergeViews( { result1v, result2v }, true, false ) );
			testPass.addOutputColourView( testPass.mergeViews( { result1v, result2v }, false, false ) );

			auto runnable = graph.compile( getContext() );
			test::checkRunnable( testCounts, runnable );
		}
		testEnd()
	}

	void testRenderQuad( test::TestCounts & testCounts )
	{
		testBegin( "testRenderQuad" )
		crg::ResourceHandler handler;
		auto result = handler.createImageId( test::createImage( "result", VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto resultv = handler.createViewId( test::createView( "resultv", result, VK_FORMAT_R16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
		{
			crg::FrameGraph graph{ handler, testCounts.testName };
			crg::RenderQuad * renderQuad{};
			uint32_t passIndex{};
			auto & testPass = graph.createPass( "Pass"
				, [&renderQuad, &passIndex]( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
					crg::rq::Config cfg;
					cfg.passIndex( &passIndex );
					cfg.baseConfig( crg::pp::Config{}
						.programCreator( crg::ProgramCreator{ 2u
							, []( uint32_t ){ return crg::VkPipelineShaderStageCreateInfoArray{ VkPipelineShaderStageCreateInfo{} }; } } ) );
					auto res = std::make_unique< crg::RenderQuad >( pass, context, runGraph
						, crg::ru::Config{ 2u, true }, std::move( cfg ) );
					renderQuad = res.get();
					return res;
				} );
			testPass.addOutputColourView( resultv );

			auto runnable = graph.compile( getContext() );
			test::checkRunnable( testCounts, runnable );
			require( renderQuad )
			passIndex = 1u - passIndex;
			checkNoThrow( renderQuad->resetPipeline( crg::VkPipelineShaderStageCreateInfoArray{ VkPipelineShaderStageCreateInfo{} }, 0u ) )
			checkNoThrow( runnable->record() )
			checkNoThrow( runnable->run( VkQueue{} ) )
		}
		{
			crg::FrameGraph graph{ handler, testCounts.testName };
			crg::RenderQuad * renderQuad{};
			auto & testPass = graph.createPass( "Pass"
				, [&renderQuad]( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
					crg::rq::Config cfg;
					cfg.indirectBuffer( crg::IndirectBuffer{ crg::Buffer{ VkBuffer( 1 ), "ind" }, sizeof( VkDrawIndirectCommand ) } );
					cfg.baseConfig( crg::pp::Config{}
						.programCreator( crg::ProgramCreator{ 1u
							, []( uint32_t ){ return crg::VkPipelineShaderStageCreateInfoArray{ VkPipelineShaderStageCreateInfo{} }; } } ) );
					auto res = std::make_unique< crg::RenderQuad >( pass, context, runGraph
						, crg::ru::Config{ 2u, true }, std::move( cfg ) );
					renderQuad = res.get();
					return res;
				} );
			testPass.addOutputColourView( resultv );

			auto runnable = graph.compile( getContext() );
			test::checkRunnable( testCounts, runnable );
			require( renderQuad )
			checkNoThrow( renderQuad->resetPipeline( crg::VkPipelineShaderStageCreateInfoArray{ VkPipelineShaderStageCreateInfo{} }, 0u ) )
			checkNoThrow( runnable->record() )
			checkNoThrow( runnable->run( VkQueue{} ) )
		}
		testEnd()
	}

	void testRenderMesh( test::TestCounts & testCounts )
	{
		testBegin( "testRenderMesh" )
		crg::ResourceHandler handler;
		auto result = handler.createImageId( test::createImage( "result", VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto resultv = handler.createViewId( test::createView( "resultv", result, VK_FORMAT_R16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
		{
			crg::FrameGraph graph{ handler, testCounts.testName };
			crg::RenderMesh * renderMesh{};
			uint32_t passIndex{};
			auto & testPass = graph.createPass( "Pass"
				, [&renderMesh, &passIndex]( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
					crg::rm::Config cfg;
					cfg.getPassIndex( crg::RunnablePass::GetPassIndexCallback( [&passIndex](){ return passIndex; } ) );
					cfg.baseConfig( crg::pp::Config{}
						.programCreator( crg::ProgramCreator{ 2u
							, []( uint32_t ){ return crg::VkPipelineShaderStageCreateInfoArray{ VkPipelineShaderStageCreateInfo{} }; } } ) );
					auto res = std::make_unique< crg::RenderMesh >( pass, context, runGraph
						, crg::ru::Config{ 2u, true }, std::move( cfg ) );
					renderMesh = res.get();
					return res;
				} );
			testPass.addOutputColourView( resultv );

			auto runnable = graph.compile( getContext() );
			test::checkRunnable( testCounts, runnable );
			require( renderMesh )
			passIndex = 1u - passIndex;
			checkNoThrow( renderMesh->resetPipeline( crg::VkPipelineShaderStageCreateInfoArray{ VkPipelineShaderStageCreateInfo{} }, 0u ) )
			checkNoThrow( runnable->record() )
			checkNoThrow( runnable->run( VkQueue{} ) )
		}
		{
			crg::FrameGraph graph{ handler, testCounts.testName };
			crg::RenderMesh * renderMesh{};
			auto & testPass = graph.createPass( "Pass"
				, [&renderMesh]( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
					crg::rm::Config cfg;
					cfg.vertexBuffer( crg::VertexBuffer{ crg::Buffer{ VkBuffer( 1 ), "vtx" } } );
					cfg.baseConfig( crg::pp::Config{}
						.programs( { crg::VkPipelineShaderStageCreateInfoArray{ VkPipelineShaderStageCreateInfo{} } } ) );
					auto res = std::make_unique< crg::RenderMesh >( pass, context, runGraph
						, crg::ru::Config{ 1u, true }, std::move( cfg ) );
					renderMesh = res.get();
					return res;
				} );
			testPass.addOutputColourView( resultv );

			auto runnable = graph.compile( getContext() );
			test::checkRunnable( testCounts, runnable );
			require( renderMesh )
			checkNoThrow( renderMesh->resetPipeline( crg::VkPipelineShaderStageCreateInfoArray{ VkPipelineShaderStageCreateInfo{} }, 0u ) )
			checkNoThrow( runnable->record() )
			checkNoThrow( runnable->run( VkQueue{} ) )
		}
		{
			crg::FrameGraph graph{ handler, testCounts.testName };
			crg::RenderMesh * renderMesh{};
			auto & testPass = graph.createPass( "Pass"
				, [&renderMesh]( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
					crg::rm::Config cfg;
					auto vb = crg::VertexBuffer{ crg::Buffer{ VkBuffer( 1 ), "vtx" } };
					cfg.vertexBuffer( std::move( vb ) );
					cfg.indexBuffer( crg::IndexBuffer{ crg::Buffer{ VkBuffer( 2 ), "idx" } } );
					cfg.baseConfig( crg::pp::Config{}
						.programCreator( crg::ProgramCreator{ 1u
							, []( uint32_t ){ return crg::VkPipelineShaderStageCreateInfoArray{ VkPipelineShaderStageCreateInfo{} }; } } ) );
					auto res = std::make_unique< crg::RenderMesh >( pass, context, runGraph
						, crg::ru::Config{ 1u, true }, std::move( cfg ) );
					renderMesh = res.get();
					return res;
				} );
			testPass.addOutputColourView( resultv );

			auto runnable = graph.compile( getContext() );
			test::checkRunnable( testCounts, runnable );
			require( renderMesh )
			checkNoThrow( renderMesh->resetPipeline( crg::VkPipelineShaderStageCreateInfoArray{ VkPipelineShaderStageCreateInfo{} }, 0u ) )
			checkNoThrow( runnable->record() )
			checkNoThrow( runnable->run( VkQueue{} ) )
		}
		{
			crg::FrameGraph graph{ handler, testCounts.testName };
			crg::RenderMesh * renderMesh{};
			auto & testPass = graph.createPass( "Pass"
				, [&renderMesh]( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
					crg::rm::Config cfg;
					cfg.indirectBuffer( crg::IndirectBuffer{ crg::Buffer{ VkBuffer( 1 ), "ind" }, sizeof( VkDrawIndirectCommand ) } );
					cfg.baseConfig( crg::pp::Config{}
						.programCreator( crg::ProgramCreator{ 1u
							, []( uint32_t ){ return crg::VkPipelineShaderStageCreateInfoArray{ VkPipelineShaderStageCreateInfo{} }; } } ) );
					auto res = std::make_unique< crg::RenderMesh >( pass, context, runGraph
						, crg::ru::Config{ 1u, true }, std::move( cfg ) );
					renderMesh = res.get();
					return res;
				} );
			testPass.addOutputColourView( resultv );

			auto runnable = graph.compile( getContext() );
			test::checkRunnable( testCounts, runnable );
			require( renderMesh )
			checkNoThrow( renderMesh->resetPipeline( crg::VkPipelineShaderStageCreateInfoArray{ VkPipelineShaderStageCreateInfo{} }, 0u ) )
			checkNoThrow( runnable->record() )
			checkNoThrow( runnable->run( VkQueue{} ) )
		}
		{
			crg::FrameGraph graph{ handler, testCounts.testName };
			crg::RenderMesh * renderMesh{};
			auto & testPass = graph.createPass( "Pass"
				, [&renderMesh]( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
					crg::rm::Config cfg;
					cfg.indirectBuffer( crg::IndirectBuffer{ crg::Buffer{ VkBuffer( 1 ), "ind" }, sizeof( VkDrawIndexedIndirectCommand ) } );
					cfg.indexBuffer( crg::IndexBuffer{ crg::Buffer{ VkBuffer( 2 ), "idx" } } );
					cfg.baseConfig( crg::pp::Config{}
						.programCreator( crg::ProgramCreator{ 1u
							, []( uint32_t ){ return crg::VkPipelineShaderStageCreateInfoArray{ VkPipelineShaderStageCreateInfo{} }; } } ) );
					auto res = std::make_unique< crg::RenderMesh >( pass, context, runGraph
						, crg::ru::Config{ 1u, true }, std::move( cfg ) );
					renderMesh = res.get();
					return res;
				} );
			testPass.addOutputColourView( resultv );

			auto runnable = graph.compile( getContext() );
			test::checkRunnable( testCounts, runnable );
			require( renderMesh )
			checkNoThrow( renderMesh->resetPipeline( crg::VkPipelineShaderStageCreateInfoArray{ VkPipelineShaderStageCreateInfo{} }, 0u ) )
			checkNoThrow( runnable->record() )
			checkNoThrow( runnable->run( VkQueue{} ) )
		}
		testEnd()
	}

	void testRenderTexturedMesh( test::TestCounts & testCounts )
	{
		testBegin( "testRenderTexturedMesh" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };

		auto sampled = graph.createImage( test::createImage( "sampled", VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto sampledv = graph.createView( test::createView( "sampledv", sampled, VK_FORMAT_R16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
		crg::RenderQuad * renderQuad{};
		auto & testQuad = graph.createPass( "Quad"
			, [&renderQuad]( crg::FramePass const & pass
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
				auto res = std::make_unique< crg::RenderQuad >( pass, context, runGraph
					, crg::ru::Config{ 2u, true }, std::move( cfg ) );
				renderQuad = res.get();
				return res;
			} );
		testQuad.addOutputColourView( sampledv );

		auto result = graph.createImage( test::createImage( "result", VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto resultv = graph.createView( test::createView( "resultv", result, VK_FORMAT_R16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
		crg::RenderMesh * renderMesh{};
		auto & testMesh = graph.createPass( "Mesh"
			, [&renderMesh]( crg::FramePass const & pass
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
				auto res = std::make_unique< crg::RenderMesh >( pass, context, runGraph
					, crg::ru::Config{ 1u, true }, std::move( cfg ) );
				renderMesh = res.get();
				return res;
			} );
		testMesh.addDependency( testQuad );
		testMesh.addOutputColourView( resultv );
		testMesh.addSampledView( sampledv, 0u );

		auto runnable = graph.compile( getContext() );
		test::checkRunnable( testCounts, runnable );
		require( renderQuad )
		require( renderMesh )
		checkNoThrow( renderQuad->resetPipeline( crg::VkPipelineShaderStageCreateInfoArray{ VkPipelineShaderStageCreateInfo{} }, 0u ) )
		checkNoThrow( renderMesh->resetPipeline( crg::VkPipelineShaderStageCreateInfoArray{ VkPipelineShaderStageCreateInfo{} }, 0u ) )
		checkNoThrow( runnable->record() )
		checkNoThrow( runnable->run( crg::SemaphoreWait{ VkSemaphore( 1 ), VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT }, VkQueue{} ) )
		check( graph.getFinalLayoutState( sampledv, 0u ).layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL )
		check( graph.getDefaultGroup().getFinalLayoutState( sampledv, 0u ).layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL )
		testEnd()
	}
}

int main( int argc, char ** argv )
{
	testSuiteBegin( "TestRunnablePass" )
	testBufferCopy( testCounts );
	testBufferToImageCopy( testCounts );
	testGenerateMipmaps( testCounts );
	testImageBlit( testCounts );
	testImageCopy( testCounts );
	testImageToBufferCopy( testCounts );
	testComputePass( testCounts );
	testComputePassTransitions( testCounts );
	testRenderPass( testCounts );
	testRenderQuad( testCounts );
	testRenderMesh( testCounts );
	testRenderTexturedMesh( testCounts );
	testSuiteEnd()
}
