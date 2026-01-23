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

	TEST( RunnablePass, BufferCopy_I_O )
	{
		testBegin( "testBufferCopy_I_O" )
		{
			crg::ResourceHandler handler;
			crg::FrameGraph graph{ handler, testCounts.testName };
			auto buffer1 = graph.createBuffer( test::createBuffer( "buffer1" ) );
			auto buffer2 = graph.createBuffer( test::createBuffer( "buffer2" ) );
			auto buffer1v = graph.createView( test::createView( "buffer1v", buffer1 ) );
			auto buffer2v = graph.createView( test::createView( "buffer2v", buffer2 ) );
			auto buffer1a = crg::Attachment::createDefault( buffer1v );
			auto & testPass = graph.createPass( "Pass"
				, []( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
					return std::make_unique< crg::BufferCopy >( pass, context, runGraph
						, 0u, 1024u );
				} );
			testPass.addInputTransfer( buffer1a );
			testPass.addOutputTransferBuffer( buffer2v );

			auto runnable = graph.compile( getContext() );
			test::checkRunnable( testCounts, runnable );
		}
		testEnd()
	}

	TEST( RunnablePass, BufferCopy_IO_IO )
	{
		testBegin( "testBufferCopy_IO_IO" )
		{
			crg::ResourceHandler handler;
			crg::FrameGraph graph{ handler, testCounts.testName };
			auto buffer1 = graph.createBuffer( test::createBuffer( "buffer1" ) );
			auto buffer2 = graph.createBuffer( test::createBuffer( "buffer2" ) );
			auto buffer1v = graph.createView( test::createView( "buffer1v", buffer1 ) );
			auto buffer2v = graph.createView( test::createView( "buffer2v", buffer2 ) );
			auto buffer1a = crg::Attachment::createDefault( buffer1v );
			auto buffer2a = crg::Attachment::createDefault( buffer2v );
			auto & testPass = graph.createPass( "Pass"
				, []( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
					return std::make_unique< crg::BufferCopy >( pass, context, runGraph
						, 0u, 512u );
				} );
			testPass.addInOutTransfer( buffer1a );
			testPass.addInOutTransfer( buffer2a );

			auto runnable = graph.compile( getContext() );
			test::checkRunnable( testCounts, runnable );
		}
		testEnd()
	}

	TEST( RunnablePass, BufferToImageCopy )
	{
		testBegin( "testBufferToImageCopy" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto result = graph.createImage( test::createImage( "result", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto buffer = graph.createBuffer( test::createBuffer( "buffer" ) );
		auto resultv = graph.createView( test::createView( "resultv", result, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
		auto bufferv = graph.createView( test::createView( "bufferv", buffer ) );
		auto buffera = crg::Attachment::createDefault( bufferv );
		auto & testPass = graph.createPass( "Pass"
			, []( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return std::make_unique< crg::BufferToImageCopy >( pass, context, runGraph
					, crg::Offset3D{}, crg::Extent3D{ 1024, 1024, 1u } );
			} );
		testPass.addInputTransfer( buffera );
		testPass.addOutputTransferImage( resultv );

		auto runnable = graph.compile( getContext() );
		test::checkRunnable( testCounts, runnable );
		testEnd()
	}

	TEST( RunnablePass, GenerateMipmaps )
	{
		testBegin( "testGenerateMipmaps" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto result = graph.createImage( test::createImage( "result", crg::PixelFormat::eR16G16B16A16_SFLOAT, 10u ) );
		auto resultv = graph.createView( test::createView( "resultv", result, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 10u, 0u, 1u ) );
		auto resulta = crg::Attachment::createDefault( resultv );
		auto & testPass = graph.createPass( "Pass"
			, []( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return std::make_unique< crg::GenerateMipmaps >( pass, context, runGraph );
			} );
		testPass.addInOutTransfer( resulta );

		auto runnable = graph.compile( getContext() );
		test::checkRunnable( testCounts, runnable );
		testEnd()
	}

	TEST( RunnablePass, ImageBlit )
	{
		testBegin( "testImageBlit" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto input = graph.createImage( test::createImage( "input", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto result = graph.createImage( test::createImage( "result", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto inputv = graph.createView( test::createView( "inputv", input, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
		auto resultv = graph.createView( test::createView( "resultv", result, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
		auto inputa = crg::Attachment::createDefault( inputv );
		auto & testPass = graph.createPass( "Pass"
			, [inputv, resultv]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return std::make_unique< crg::ImageBlit >( pass, context, runGraph
					, crg::Rect3D{ crg::Offset3D{}, getExtent( inputv ) }
					, crg::Rect3D{ crg::Offset3D{}, getExtent( resultv ) }
					, crg::FilterMode::eLinear );
			} );
		testPass.addInputTransfer( inputa );
		testPass.addOutputTransferImage( resultv );

		auto runnable = graph.compile( getContext() );
		test::checkRunnable( testCounts, runnable );
		testEnd()
	}

	TEST( RunnablePass, Image3DBlit )
	{
		testBegin( "testImage3DBlit" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto input = graph.createImage( test::createImage3D( "input", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto result = graph.createImage( test::createImage3D( "result", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto inputv = graph.createView( test::createView( "inputv", input, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
		auto resultv = graph.createView( test::createView( "resultv", result, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
		auto inputa = crg::Attachment::createDefault( inputv );
		auto & testPass = graph.createPass( "Pass"
			, [inputv, resultv]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return std::make_unique< crg::ImageBlit >( pass, context, runGraph
					, crg::Rect3D{ crg::Offset3D{}, getExtent( inputv ) }
					, crg::Rect3D{ crg::Offset3D{}, getExtent( resultv ) }
					, crg::FilterMode::eLinear );
			} );
		testPass.addInputTransfer( inputa );
		testPass.addOutputTransferImage( resultv );

		auto runnable = graph.compile( getContext() );
		test::checkRunnable( testCounts, runnable );
		testEnd()
	}

	TEST( RunnablePass, ImageCopy_Base )
	{
		testBegin( "testImageCopy_Base" )
		{
			crg::ResourceHandler handler;
			crg::FrameGraph graph{ handler, testCounts.testName };
			auto input = graph.createImage( test::createImage( "input", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
			auto result = graph.createImage( test::createImage( "result", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
			auto inputv = graph.createView( test::createView( "inputv", input, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
			auto resultv = graph.createView( test::createView( "resultv", result, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
			auto inputa = crg::Attachment::createDefault( inputv );
			auto & testPass = graph.createPass( "Pass"
				, [inputv]( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
						return std::make_unique< crg::ImageCopy >( pass, context, runGraph
							, getExtent( inputv ) );
				} );
			testPass.addInputTransfer( inputa );
			testPass.addOutputTransferImage( resultv );

			auto runnable = graph.compile( getContext() );
			test::checkRunnable( testCounts, runnable );
		}
		testEnd()
	}

	TEST( RunnablePass, ImageCopy_SingleInputMultipleOutputsSameImage )
	{
		testBegin( "ImageCopy_SingleInputMultipleOutputsSameImage" )
		{
			crg::ResourceHandler handler;
			crg::FrameGraph graph{ handler, testCounts.testName };
			auto input = graph.createImage( test::createImage( "input", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
			auto result = graph.createImage( test::createImage( "result", crg::PixelFormat::eR16G16B16A16_SFLOAT, 1u, 6u ) );
			auto inputv = graph.createView( test::createView( "inputv", input, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
			auto resultv0 = graph.createView( test::createView( "result0v", result, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
			auto resultv1 = graph.createView( test::createView( "result1v", result, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 1u, 1u ) );
			auto resultv2 = graph.createView( test::createView( "result2v", result, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 2u, 1u ) );
			auto resultv3 = graph.createView( test::createView( "result3v", result, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 3u, 1u ) );
			auto resultv4 = graph.createView( test::createView( "result4v", result, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 4u, 1u ) );
			auto resultv5 = graph.createView( test::createView( "result5v", result, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 5u, 1u ) );
			auto inputa = crg::Attachment::createDefault( inputv );
			auto & testPass = graph.createPass( "Pass"
				, [inputv]( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
						return std::make_unique< crg::ImageCopy >( pass, context, runGraph
							, getExtent( inputv ), crg::ImageLayout::eShaderReadOnly );
				} );
			testPass.addInputTransfer( inputa );
			testPass.addOutputTransferImage( resultv0 );
			testPass.addOutputTransferImage( resultv1 );
			testPass.addOutputTransferImage( resultv2 );
			testPass.addOutputTransferImage( resultv3 );
			testPass.addOutputTransferImage( resultv4 );
			testPass.addOutputTransferImage( resultv5 );

			auto runnable = graph.compile( getContext() );
			test::checkRunnable( testCounts, runnable );
		}
		testEnd()
	}

	TEST( RunnablePass, ImageCopy_SingleInputMultipleOutputsDifferentImage )
	{
		testBegin( "ImageCopy_SingleInputMultipleOutputsDifferentImage" )
		{
			crg::ResourceHandler handler;
			crg::FrameGraph graph{ handler, testCounts.testName };
			auto input = graph.createImage( test::createImage( "input", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
			auto result0 = graph.createImage( test::createImage( "result0", crg::PixelFormat::eR16G16B16A16_SFLOAT, 1u, 3u ) );
			auto result1 = graph.createImage( test::createImage( "result1", crg::PixelFormat::eR16G16B16A16_SFLOAT, 1u, 3u ) );
			auto inputv = graph.createView( test::createView( "inputv", input, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
			auto result0v0 = graph.createView( test::createView( "result0v", result0, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
			auto result0v1 = graph.createView( test::createView( "result1v", result0, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 1u, 1u ) );
			auto result0v2 = graph.createView( test::createView( "result2v", result0, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 2u, 1u ) );
			auto result1v0 = graph.createView( test::createView( "result3v", result1, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
			auto result1v1 = graph.createView( test::createView( "result4v", result1, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 1u, 1u ) );
			auto result1v2 = graph.createView( test::createView( "result5v", result1, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 2u, 1u ) );
			auto inputa = crg::Attachment::createDefault( inputv );
			auto & testPass = graph.createPass( "Pass"
				, [inputv]( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
						return std::make_unique< crg::ImageCopy >( pass, context, runGraph
							, getExtent( inputv ) );
				} );
			testPass.addInputTransfer( inputa );
			testPass.addOutputTransferImage( result0v0 );
			testPass.addOutputTransferImage( result0v1 );
			testPass.addOutputTransferImage( result0v2 );
			testPass.addOutputTransferImage( result1v0 );
			testPass.addOutputTransferImage( result1v1 );
			testPass.addOutputTransferImage( result1v2 );

			auto runnable = graph.compile( getContext() );
			test::checkRunnable( testCounts, runnable );
		}
		testEnd()
	}

	TEST( RunnablePass, ImageCopy_MultipleInputsSameImageSingleOutput )
	{
		testBegin( "ImageCopy_SingleInputMultipleOutputsSameImage" )
		{
			crg::ResourceHandler handler;
			crg::FrameGraph graph{ handler, testCounts.testName };
			auto input = graph.createImage( test::createImage( "input", crg::PixelFormat::eR16G16B16A16_SFLOAT, 1u, 6u ) );
			auto result = graph.createImage( test::createImage( "result", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
			auto inputv0 = graph.createView( test::createView( "inputv0", input, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
			auto inputv1 = graph.createView( test::createView( "inputv1", input, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 1u, 1u ) );
			auto inputv2 = graph.createView( test::createView( "inputv2", input, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 2u, 1u ) );
			auto inputv3 = graph.createView( test::createView( "inputv3", input, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 3u, 1u ) );
			auto inputv4 = graph.createView( test::createView( "inputv4", input, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 4u, 1u ) );
			auto inputv5 = graph.createView( test::createView( "inputv5", input, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 5u, 1u ) );
			auto resultv = graph.createView( test::createView( "resultv", result, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
			auto & testPass = graph.createPass( "Pass"
				, [inputv0]( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
						return std::make_unique< crg::ImageCopy >( pass, context, runGraph
							, getExtent( inputv0 ), crg::ImageLayout::eShaderReadOnly );
				} );
			testPass.addInputTransferImage( inputv0 );
			testPass.addInputTransferImage( inputv1 );
			testPass.addInputTransferImage( inputv2 );
			testPass.addInputTransferImage( inputv3 );
			testPass.addInputTransferImage( inputv4 );
			testPass.addInputTransferImage( inputv5 );
			testPass.addOutputTransferImage( resultv );

			auto runnable = graph.compile( getContext() );
			test::checkRunnable( testCounts, runnable );
		}
		testEnd()
	}

	TEST( RunnablePass, ImageCopy_MultipleInputsDifferentImageSingleOutput )
	{
		testBegin( "ImageCopy_MultipleInputsDifferentImageSingleOutput" )
		{
			crg::ResourceHandler handler;
			crg::FrameGraph graph{ handler, testCounts.testName };
			auto input0 = graph.createImage( test::createImage( "input0", crg::PixelFormat::eR16G16B16A16_SFLOAT, 1u, 3u ) );
			auto input1 = graph.createImage( test::createImage( "input1", crg::PixelFormat::eR16G16B16A16_SFLOAT, 1u, 3u ) );
			auto result = graph.createImage( test::createImage( "result", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
			auto input0v0 = graph.createView( test::createView( "input0v0", input0, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
			auto input0v1 = graph.createView( test::createView( "input0v1", input0, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 1u, 1u ) );
			auto input0v2 = graph.createView( test::createView( "input0v2", input0, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 2u, 1u ) );
			auto input1v0 = graph.createView( test::createView( "input1v0", input1, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
			auto input1v1 = graph.createView( test::createView( "input1v1", input1, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 1u, 1u ) );
			auto input1v2 = graph.createView( test::createView( "input1v2", input1, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 2u, 1u ) );
			auto resultv = graph.createView( test::createView( "resultv", result, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
			auto & testPass = graph.createPass( "Pass"
				, [result]( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
						return std::make_unique< crg::ImageCopy >( pass, context, runGraph
							, getExtent( result ) );
				} );
			testPass.addInputTransferImage( input0v0 );
			testPass.addInputTransferImage( input0v1 );
			testPass.addInputTransferImage( input0v2 );
			testPass.addInputTransferImage( input1v0 );
			testPass.addInputTransferImage( input1v1 );
			testPass.addInputTransferImage( input1v2 );
			testPass.addOutputTransferImage( resultv );

			auto runnable = graph.compile( getContext() );
			test::checkRunnable( testCounts, runnable );
		}
		testEnd()
	}

	TEST( RunnablePass, ImageCopy_OutputLayout )
	{
		testBegin( "testImageCopy_OutputLayout" )
		{
			crg::ResourceHandler handler;
			crg::FrameGraph graph{ handler, testCounts.testName };
			auto input = graph.createImage( test::createImage( "input", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
			auto result = graph.createImage( test::createImage( "result", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
			auto inputv = graph.createView( test::createView( "inputv", input, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
			auto resultv = graph.createView( test::createView( "resultv", result, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
			auto inputa = crg::Attachment::createDefault( inputv );
			auto & testPass = graph.createPass( "Pass"
				, [inputv]( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
						return std::make_unique< crg::ImageCopy >( pass, context, runGraph
							, getExtent( inputv )
							, crg::ImageLayout::eShaderReadOnly );
				} );
			testPass.addInputTransfer( inputa );
			testPass.addOutputTransferImage( resultv );

			auto runnable = graph.compile( getContext() );
			test::checkRunnable( testCounts, runnable );
		}
		testEnd()
	}

	TEST( RunnablePass, ImageCopy_BackAndForth )
	{
		testBegin( "testImageCopy_BackAndForth" )
		{
			crg::ResourceHandler handler;
			crg::FrameGraph graph{ handler, testCounts.testName };
			auto input = graph.createImage( test::createImage( "input", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
			auto result = graph.createImage( test::createImage( "result", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
			auto inputv = graph.createView( test::createView( "inputv", input, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
			auto resultv = graph.createView( test::createView( "resultv", result, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
			auto inputa = crg::Attachment::createDefault( inputv );
			auto resulta = crg::Attachment::createDefault( resultv );
			auto & testPass = graph.createPass( "Pass"
				, [inputv]( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
						return std::make_unique< crg::ImageCopy >( pass, context, runGraph
							, getExtent( inputv )
							, crg::ImageLayout::eShaderReadOnly );
				} );
			testPass.addInputTransfer( inputa );
			testPass.addOutputTransferImage( resultv );
			testPass.addInputTransfer( resulta );
			testPass.addOutputTransferImage( inputv );

			auto runnable = graph.compile( getContext() );
			test::checkRunnable( testCounts, runnable );
		}
		testEnd()
	}

	TEST( RunnablePass, ImageToBufferCopy )
	{
		testBegin( "testImageToBufferCopy" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto input = graph.createImage( test::createImage( "input", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto buffer = graph.createBuffer( test::createBuffer( "buffer" ) );
		auto inputv = graph.createView( test::createView( "inputv", input, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
		auto bufferv = graph.createView( test::createView( "bufferv", buffer ) );
		auto inputa = crg::Attachment::createDefault( inputv );
		auto & testPass = graph.createPass( "Pass"
			, [inputv]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return std::make_unique< crg::ImageToBufferCopy >( pass, context, runGraph
					, crg::Offset3D{}, getExtent( inputv ) );
			} );
		testPass.addInputTransfer( inputa );
		testPass.addOutputTransferBuffer( bufferv );

		auto runnable = graph.compile( getContext() );
		test::checkRunnable( testCounts, runnable );
		testEnd()
	}

	TEST( RunnablePass, ComputePass )
	{
		testBegin( "testComputePass" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, "/" + testCounts.testName };
		auto buffer = graph.createBuffer( test::createBuffer( "buffer" ) );
		auto bufferv = graph.createView( test::createView( "bufferv", buffer ) );
		auto indirect = graph.createBuffer( test::createBuffer( "indirect" ) );
		auto indirectv = graph.createView( test::createView( "indirectv", indirect ) );
		auto & testPass1 = graph.createPass( "Pass1"
			, [&indirectv]( crg::FramePass const & pass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				crg::cp::Config cfg;
				cfg.indirectBuffer( crg::IndirectBuffer{ indirectv, sizeof( VkDrawIndirectCommand ) } );
				cfg.baseConfig( crg::pp::Config{}
					.programCreator( crg::ProgramCreator{ 1u
						, []( uint32_t ){ return crg::VkPipelineShaderStageCreateInfoArray{ VkPipelineShaderStageCreateInfo{} }; } } ) );
				return std::make_unique< crg::ComputePass >( pass, context, runGraph
					, crg::ru::Config{}, std::move( cfg ) );
			} );
		auto buffera = testPass1.addClearableOutputStorageBuffer( bufferv, 1u );

		auto result = graph.createImage( test::createImage( "result", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto depth = graph.createImage( test::createImage( "depth", crg::PixelFormat::eD32_SFLOAT ) );
		auto depthStencil = graph.createImage( test::createImage( "depthStencil", crg::PixelFormat::eD32_SFLOAT_S8_UINT ) );
		auto buffer1 = graph.createBuffer( test::createBuffer( "buffer1" ) );
		auto resultv = graph.createView( test::createView( "resultv", result, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
		auto depthv = graph.createView( test::createView( "depthv", depth, crg::PixelFormat::eD32_SFLOAT, 0u, 1u, 0u, 1u ) );
		auto depthStencilv = graph.createView( test::createView( "depthStencilv", depthStencil, crg::PixelFormat::eD32_SFLOAT_S8_UINT, 0u, 1u, 0u, 1u ) );
		auto buffer1v = graph.createView( test::createView( "buffer1v", buffer1 ) );
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
		testPass2.addInputUniformBuffer( buffer1v, 0u );
		testPass2.addInputUniform( *buffera, 1u );
		auto resulta = testPass2.addClearableOutputStorageImage( resultv, 2u );
		auto deptha = testPass2.addClearableOutputStorageImage( depthv, 3u );

		auto buffer2 = graph.createBuffer( test::createBuffer( "buffer2" ) );
		auto buffer3 = graph.createBuffer( test::createBuffer( "buffer3" ) );
		auto buffer4 = graph.createBuffer( test::createBuffer( "buffer4" ) );
		auto buffer5 = graph.createBuffer( test::createBuffer( "buffer5" ) );
		auto buffer6 = graph.createBuffer( test::createBuffer( "buffer6" ) );
		auto buffer7 = graph.createBuffer( test::createBuffer( "buffer7" ) );
		auto buffer2v = graph.createView( test::createView( "buffer2v", buffer2 ) );
		auto buffer3v = graph.createView( test::createView( "buffer3v", buffer3 ) );
		auto buffer4v = graph.createView( test::createView( "buffer4v", buffer4, crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto buffer5v = graph.createView( test::createView( "buffer5v", buffer5, crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto buffer6v = graph.createView( test::createView( "buffer6v", buffer6, crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto buffer7v = graph.createView( test::createView( "buffer7v", buffer7, crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto buffer2a = crg::Attachment::createDefault( buffer2v );
		auto buffer3a = crg::Attachment::createDefault( buffer3v );
		auto buffer5a = crg::Attachment::createDefault( buffer5v );
		auto buffer7a = crg::Attachment::createDefault( buffer7v );
		auto depthStencila = crg::Attachment::createDefault( depthStencilv );
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
		testPass3.addInputStorage( buffer2a, 2u );
		testPass3.addInOutStorage( buffer3a, 3u );

		testPass3.addInputUniformBuffer( buffer4v, 4u );
		testPass3.addInputStorage( buffer5a, 5u );
		testPass3.addOutputStorageBuffer( buffer6v, 6u );
		testPass3.addInOutStorage( buffer7a, 7u );

		testPass3.addImplicit( *resulta, crg::ImageLayout::eShaderReadOnly );
		testPass3.addImplicit( *deptha, crg::ImageLayout::eShaderReadOnly );
		testPass3.addImplicit( depthStencila, crg::ImageLayout::eShaderReadOnly );

		auto runnable = graph.compile( getContext() );
		test::checkRunnable( testCounts, runnable );
		require( computePass )
		checkNoThrow( computePass->resetPipeline( crg::VkPipelineShaderStageCreateInfoArray{ VkPipelineShaderStageCreateInfo{} }, 0u ) )
		checkNoThrow( computePass->resetCommandBuffer( 0u ) )
		checkNoThrow( runnable->record() )
		checkNoThrow( runnable->run( VkQueue{} ) )
		checkNoThrow( graph.getFinalAccessState( buffer1v ) )
		testEnd()
	}

	TEST( RunnablePass, ComputePassTransitions )
	{
		testBegin( "testComputePassTransitions" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, "/" + testCounts.testName };
		auto result = graph.createImage( test::createImage( "result", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto depth = graph.createImage( test::createImage( "depth", crg::PixelFormat::eD32_SFLOAT ) );
		auto buffer1 = graph.createBuffer( test::createBuffer( "buffer1" ) );
		auto buffer2 = graph.createBuffer( test::createBuffer( "buffer2" ) );
		auto resultv = graph.createView( test::createView( "resultv", result, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
		auto depthv = graph.createView( test::createView( "depthv", depth, crg::PixelFormat::eD32_SFLOAT, 0u, 1u, 0u, 1u ) );
		auto buffer1v = graph.createView( test::createView( "buffer1v", buffer1 ) );
		auto buffer2v = graph.createView( test::createView( "buffer2v", buffer2 ) );
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
		auto resulta = testPass1.addClearableOutputStorageImage( resultv, 0u );
		auto deptha = testPass1.addClearableOutputStorageImage( depthv, 1u );
		auto buffer1a = testPass1.addClearableOutputStorageBuffer( buffer1v, 2u );
		auto buffer2a = testPass1.addClearableOutputStorageBuffer( buffer2v, 3u );

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
		testPass2.addImplicit( *resulta, crg::ImageLayout::eShaderReadOnly );
		testPass2.addImplicit( *deptha, crg::ImageLayout::eShaderReadOnly );
		testPass2.addInputStorage( *buffer1a, 1u );
		testPass2.addImplicit( *buffer2a, crg::AccessState{} );

		auto runnable = graph.compile( getContext() );
		test::checkRunnable( testCounts, runnable );
		testEnd()
	}

	TEST( RunnablePass, RenderPass_ORcl )
	{
		testBegin( "testRenderPass_ORcl" )
		{
			crg::ResourceHandler handler;
			crg::FrameGraph graph{ handler, testCounts.testName };
			auto result = graph.createImage( test::createImage( "result", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
			auto resultv = graph.createView( test::createView( "resultv", result, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
			auto sampled = graph.createImage( test::createImage( "sampled", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
			auto sampledv = graph.createView( test::createView( "sampledv", sampled, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
			auto & testPass = graph.createPass( "Pass"
				, []( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
						return std::make_unique< crg::RenderPass >( pass, context, runGraph
							, crg::RenderPass::Callbacks{ crg::defaultV< crg::RunnablePass::InitialiseCallback >
								, crg::defaultV< crg::RunnablePass::RecordCallback > } );
				} );
			testPass.addInputSampledImage( sampledv, 0u );
			testPass.addOutputColourTarget( resultv );

			auto runnable = graph.compile( getContext() );
			test::checkRunnable( testCounts, runnable );
		}
		testEnd()
	}

	TEST( RunnablePass, RenderPass_ORcl_DefCont )
	{
		testBegin( "testRenderPass_ORcl_DefCont" )
		{
			crg::ResourceHandler handler;
			crg::FrameGraph graph{ handler, testCounts.testName };
			auto result = graph.createImage( test::createImage( "result", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
			auto resultv = graph.createView( test::createView( "resultv", result, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
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
			testPass.addOutputColourTarget( resultv );

			auto runnable = graph.compile( getContext() );
			test::checkRunnable( testCounts, runnable );
		}
		testEnd()
	}

	TEST( RunnablePass, RenderPass_MergedImageViews_ORcl )
	{
		testBegin( "testRenderPass_MergedImageViews_ORcl" )
		{
			crg::ResourceHandler handler;
			crg::FrameGraph graph{ handler, testCounts.testName };
			auto & group = graph.createPassGroup( "testGroup" );
			auto result = group.createImage( test::createImage( "result", crg::PixelFormat::eR16G16B16A16_SFLOAT, 1u, 2u ) );
			auto result1v = group.createView( test::createView( "result1v", result, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
			auto result2v = group.createView( test::createView( "result2v", result, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 1u, 1u ) );
			auto resultv = group.mergeViews( { result1v, result2v } );
			auto & testPass = group.createPass( "Pass"
				, []( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
						return std::make_unique< crg::RenderPass >( pass, context, runGraph
							, crg::RenderPass::Callbacks{ crg::defaultV< crg::RunnablePass::InitialiseCallback >
								, crg::defaultV< crg::RunnablePass::RecordCallback >
								, crg::defaultV< crg::RenderPass::GetSubpassContentsCallback  > } );
				} );
			testPass.addOutputColourTarget( resultv );

			auto runnable = graph.compile( getContext() );
			test::checkRunnable( testCounts, runnable );
			checkEqual( graph.getFinalLayoutState( resultv ).layout, crg::ImageLayout::eColorAttachment )
		}
		testEnd()
	}

	TEST( RunnablePass, RenderPass_MergedImageViews_CubeARray_ORcl )
	{
		testBegin( "testRenderPass_MergedImageViews_CubeARray_ORcl" )
		{
			crg::ResourceHandler handler;
			crg::FrameGraph graph{ handler, testCounts.testName };
			auto & group = graph.createPassGroup( "testGroup" );
			auto result = group.createImage( test::createImageCube( "result", crg::PixelFormat::eR16G16B16A16_SFLOAT, 1u, 12u ) );
			auto result1v = group.createView( crg::ImageViewData{ "result1v"
				, result
				, crg::ImageViewCreateFlags::eNone
				, crg::ImageViewType::e2D
				, getFormat( result )
				, { getAspectMask( getFormat( result ) ), 0u, 1u, 0u, 6u } } );
			auto result2v = group.createView( crg::ImageViewData{ "result2v"
				, result
				, crg::ImageViewCreateFlags::eNone
				, crg::ImageViewType::e2D
				, getFormat( result )
				, { getAspectMask( getFormat( result ) ), 0u, 1u, 6u, 6u } } );
			auto resultv = group.mergeViews( { result1v, result2v } );
			auto & testPass = group.createPass( "Pass"
				, []( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
						return std::make_unique< crg::RenderPass >( pass, context, runGraph
							, crg::RenderPass::Callbacks{ crg::defaultV< crg::RunnablePass::InitialiseCallback >
								, crg::defaultV< crg::RunnablePass::RecordCallback >
								, crg::defaultV< crg::RenderPass::GetSubpassContentsCallback  > } );
				} );
			testPass.addOutputColourTarget( resultv );

			auto runnable = graph.compile( getContext() );
			test::checkRunnable( testCounts, runnable );
			checkEqual( graph.getFinalLayoutState( resultv ).layout, crg::ImageLayout::eColorAttachment )
		}
		testEnd()
	}

	TEST( RunnablePass, RenderPass_MergedBufferViews_ORcl )
	{
		testBegin( "testRenderPass_MergedBufferViews_ORcl" )
		{
			crg::ResourceHandler handler;
			crg::FrameGraph graph{ handler, testCounts.testName };
			auto & group = graph.createPassGroup( "testGroup" );
			auto result = group.createImage( test::createImage( "result", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
			auto buffer = group.createBuffer( test::createBuffer( "buffer" ) );
			auto resultv = group.createView( test::createView( "resultv", result, crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
			auto buffer1v = group.createView( test::createView( "buffer1v", buffer, 0, 512u ) );
			auto buffer2v = group.createView( test::createView( "buffer2v", buffer, 512u, 512u ) );
			auto bufferv = group.mergeViews( { buffer1v, buffer2v } );
			auto & testPass = group.createPass( "Pass"
				, []( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
						return std::make_unique< crg::RenderPass >( pass, context, runGraph
							, crg::RenderPass::Callbacks{ crg::defaultV< crg::RunnablePass::InitialiseCallback >
								, crg::defaultV< crg::RunnablePass::RecordCallback >
								, crg::defaultV< crg::RenderPass::GetSubpassContentsCallback  > } );
				} );
			testPass.addOutputStorageBuffer( bufferv, 0u );
			testPass.addOutputColourTarget( resultv );

			auto runnable = graph.compile( getContext() );
			test::checkRunnable( testCounts, runnable );
			checkEqual( graph.getFinalAccessState( bufferv ).access, crg::AccessFlags::eShaderWrite )
		}
		testEnd()
	}

	TEST( RunnablePass, RenderPass_MergedImageAttachs_ORcl )
	{
		testBegin( "testRenderPass_MergedImageAttachs_ORcl" )
		{
			crg::ResourceHandler handler;
			crg::FrameGraph graph{ handler, testCounts.testName };
			auto intermediate = graph.createImage( test::createImage( "intermediate", crg::PixelFormat::eR16G16B16A16_SFLOAT, 1u, 2u ) );
			auto intermediate1v = graph.createView( test::createView( "intermediate1v", intermediate, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
			auto intermediate2v = graph.createView( test::createView( "intermediate2v", intermediate, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 1u, 1u ) );
			auto & testPass1 = graph.createPass( "Pass1"
				, []( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
						return std::make_unique< crg::RenderPass >( pass, context, runGraph
							, crg::RenderPass::Callbacks{ crg::defaultV< crg::RunnablePass::InitialiseCallback >
								, crg::defaultV< crg::RunnablePass::RecordCallback >
								, crg::defaultV< crg::RenderPass::GetSubpassContentsCallback  > } );
				} );
			auto intermediate1a = testPass1.addOutputColourTarget( intermediate1v );
			auto intermediate2a = testPass1.addOutputColourTarget( intermediate2v );

			auto result = graph.createImage( test::createImage( "result", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
			auto resultv = graph.createView( test::createView( "resultv", result, crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
			auto & testPass2 = graph.createPass( "Pass2"
				, []( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
						return std::make_unique< crg::RenderPass >( pass, context, runGraph
							, crg::RenderPass::Callbacks{ crg::defaultV< crg::RunnablePass::InitialiseCallback >
								, crg::defaultV< crg::RunnablePass::RecordCallback >
								, crg::defaultV< crg::RenderPass::GetSubpassContentsCallback  > } );
				} );
			auto intermediatea = graph.mergeAttachments( { intermediate1a, intermediate2a } );
			testPass2.addInputSampled( *intermediatea, 0u );
			testPass2.addOutputColourTarget( resultv );

			auto runnable = graph.compile( getContext() );
			test::checkRunnable( testCounts, runnable );
			checkEqual( graph.getFinalLayoutState( intermediatea->view() ).layout, crg::ImageLayout::eShaderReadOnly )
		}
		testEnd()
	}

	TEST( RunnablePass, RenderPass_MergedBufferAttachs_ORcl )
	{
		testBegin( "testRenderPass_MergedBufferAttachs_ORcl" )
		{
			crg::ResourceHandler handler;
			crg::FrameGraph graph{ handler, testCounts.testName };
			auto buffer = graph.createBuffer( test::createBuffer( "buffer" ) );
			auto buffer1v = graph.createView( test::createView( "buffer1v", buffer, 0u, 512u ) );
			auto buffer2v = graph.createView( test::createView( "buffer2v", buffer, 512u, 512u ) );
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
			auto buffer1a = testPass1.addOutputStorageBuffer( buffer1v, 0u );
			auto buffer2a = testPass1.addOutputStorageBuffer( buffer2v, 1u );

			auto result = graph.createImage( test::createImage( "result", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
			auto resultv = graph.createView( test::createView( "resultv", result, crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
			auto & testPass2 = graph.createPass( "Pass2"
				, []( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
					return std::make_unique< crg::RenderPass >( pass, context, runGraph
						, crg::RenderPass::Callbacks{ crg::defaultV< crg::RunnablePass::InitialiseCallback >
							, crg::defaultV< crg::RunnablePass::RecordCallback >
							, crg::defaultV< crg::RenderPass::GetSubpassContentsCallback  > } );
				} );
			auto buffera = graph.mergeAttachments( { buffer1a, buffer2a } );
			testPass2.addInputStorage( *buffera, 0u );
			testPass2.addOutputColourTarget( resultv );

			auto runnable = graph.compile( getContext() );
			test::checkRunnable( testCounts, runnable );
			checkEqual( graph.getFinalAccessState( buffera->buffer() ).access, crg::AccessFlags::eShaderRead )
		}
		testEnd()
	}

	TEST( RunnablePass, RenderPass_ORdp )
	{
		testBegin( "testRenderPass_ORdp" )
		{
			crg::ResourceHandler handler;
			crg::FrameGraph graph{ handler, testCounts.testName };
			auto depth = graph.createImage( test::createImage( "depth", crg::PixelFormat::eD32_SFLOAT ) );
			auto depthv = graph.createView( test::createView( "depthv", depth, crg::PixelFormat::eD32_SFLOAT, 0u, 1u, 0u, 1u ) );
			auto & testPass = graph.createPass( "Pass"
				, []( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
					return std::make_unique< crg::RenderPass >( pass, context, runGraph
						, crg::RenderPass::Callbacks{ crg::defaultV< crg::RunnablePass::InitialiseCallback >
							, crg::defaultV< crg::RunnablePass::RecordCallback > } );
				} );
			testPass.addOutputDepthTarget ( depthv );

			auto runnable = graph.compile( getContext() );
			test::checkRunnable( testCounts, runnable );
		}
		testEnd()
	}

	TEST( RunnablePass, RenderPass_ORcl_ORdp )
	{
		testBegin( "testRenderPass_ORcl_ORdp" )
		{
			crg::ResourceHandler handler;
			crg::FrameGraph graph{ handler, testCounts.testName };
			uint32_t passIndex{};
			auto result = graph.createImage( test::createImage( "result", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
			auto resultv = graph.createView( test::createView( "resultv", result, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
			auto depth = graph.createImage( test::createImage( "depth", crg::PixelFormat::eD32_SFLOAT ) );
			auto depthv = graph.createView( test::createView( "depthv", depth, crg::PixelFormat::eD32_SFLOAT, 0u, 1u, 0u, 1u ) );
			crg::Extent2D extent{ getExtent( resultv ).width, getExtent( resultv ).height };
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
			testPass.addOutputColourTarget( resultv );
			testPass.addOutputDepthTarget ( depthv );

			auto runnable = graph.compile( getContext() );
			test::checkRunnable( testCounts, runnable );
			passIndex = 1u - passIndex;
			checkNoThrow( runnable->record() )
			checkNoThrow( runnable->run( VkQueue{} ) )
		}
		testEnd()
	}

	TEST( RunnablePass, RenderPass )
	{
		testBegin( "testRenderPass" )
		{
			crg::ResourceHandler handler;
			crg::FrameGraph graph{ handler, testCounts.testName };
			auto result1 = graph.createImage( test::createImage( "result1", crg::PixelFormat::eR16G16B16A16_SFLOAT, 1u, 2u ) );
			auto result1v = graph.createView( test::createView( "result1v", result1, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
			auto result2v = graph.createView( test::createView( "result2v", result1, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
			auto & testPass = graph.createPass( "Pass"
				, []( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
					return std::make_unique< crg::RenderPass >( pass, context, runGraph
						, crg::RenderPass::Callbacks{ crg::defaultV< crg::RunnablePass::InitialiseCallback >
							, crg::defaultV< crg::RunnablePass::RecordCallback > } );
				} );
			testPass.addOutputColourTarget( graph.mergeViews( { result1v, result2v } ) );
			testPass.addOutputColourTarget( graph.mergeViews( { result1v, result2v }, false, true ) );
			testPass.addOutputColourTarget( graph.mergeViews( { result1v, result2v }, true, false ) );
			testPass.addOutputColourTarget( graph.mergeViews( { result1v, result2v }, false, false ) );

			auto runnable = graph.compile( getContext() );
			test::checkRunnable( testCounts, runnable );
		}
		testEnd()
	}

	TEST( RunnablePass, RenderQuad )
	{
		testBegin( "testRenderQuad" )
		crg::ResourceHandler handler;
		auto result = handler.createImageId( test::createImage( "result", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto resultv = handler.createViewId( test::createView( "resultv", result, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
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
			testPass.addOutputColourTarget( resultv );

			auto runnable = graph.compile( getContext() );
			test::checkRunnable( testCounts, runnable );
			require( renderQuad )
			passIndex = 1u - passIndex;
			checkNoThrow( renderQuad->resetPipeline( crg::VkPipelineShaderStageCreateInfoArray{ VkPipelineShaderStageCreateInfo{} }, 0u ) )
			checkNoThrow( runnable->record() )
			checkNoThrow( runnable->run( VkQueue{} ) )
		}
		testEnd()
	}

	TEST( RunnablePass, RenderQuad_Indirect )
	{
		testBegin( "testRenderQuad_Indirect" )
		crg::ResourceHandler handler;
		auto result = handler.createImageId( test::createImage( "result", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto resultv = handler.createViewId( test::createView( "resultv", result, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
		{
			crg::FrameGraph graph{ handler, testCounts.testName };
			crg::RenderQuad * renderQuad{};
			auto indirect = graph.createBuffer( test::createBuffer( "indirect" ) );
			auto indirectv = graph.createView( test::createView( "indirectv", indirect ) );
			auto & testPass = graph.createPass( "Pass"
				, [&renderQuad, &indirectv]( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
					crg::rq::Config cfg;
					cfg.indirectBuffer( crg::IndirectBuffer{ indirectv, sizeof( VkDrawIndirectCommand ) } );
					cfg.baseConfig( crg::pp::Config{}
						.programCreator( crg::ProgramCreator{ 1u
							, []( uint32_t ){ return crg::VkPipelineShaderStageCreateInfoArray{ VkPipelineShaderStageCreateInfo{} }; } } ) );
					auto res = std::make_unique< crg::RenderQuad >( pass, context, runGraph
						, crg::ru::Config{ 2u, true }, std::move( cfg ) );
					renderQuad = res.get();
					return res;
				} );
			testPass.addOutputColourTarget( resultv );

			auto runnable = graph.compile( getContext() );
			test::checkRunnable( testCounts, runnable );
			require( renderQuad )
			checkNoThrow( renderQuad->resetPipeline( crg::VkPipelineShaderStageCreateInfoArray{ VkPipelineShaderStageCreateInfo{} }, 0u ) )
			checkNoThrow( runnable->record() )
			checkNoThrow( runnable->run( VkQueue{} ) )
		}
		testEnd()
	}

	TEST( RunnablePass, RenderMesh )
	{
		testBegin( "testRenderMesh" )
		crg::ResourceHandler handler;
		auto result = handler.createImageId( test::createImage( "result", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto resultv = handler.createViewId( test::createView( "resultv", result, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
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
			testPass.addOutputColourTarget( resultv );

			auto runnable = graph.compile( getContext() );
			test::checkRunnable( testCounts, runnable );
			require( renderMesh )
			passIndex = 1u - passIndex;
			checkNoThrow( renderMesh->resetPipeline( crg::VkPipelineShaderStageCreateInfoArray{ VkPipelineShaderStageCreateInfo{} }, 0u ) )
			checkNoThrow( runnable->record() )
			checkNoThrow( runnable->run( VkQueue{} ) )
		}
		testEnd()
	}

	TEST( RunnablePass, RenderMesh_Vertex )
	{
		testBegin( "testRenderMesh_Vertex" )
			crg::ResourceHandler handler;
		auto result = handler.createImageId( test::createImage( "result", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto resultv = handler.createViewId( test::createView( "resultv", result, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
		{
			crg::FrameGraph graph{ handler, testCounts.testName };
			crg::RenderMesh * renderMesh{};
			auto vertex = graph.createBuffer( test::createBuffer( "vertex" ) );
			auto vertexv = graph.createView( test::createView( "vertexv", vertex ) );
			auto & testPass = graph.createPass( "Pass"
				, [&renderMesh, &vertexv]( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
					crg::rm::Config cfg;
					cfg.vertexBuffer( crg::VertexBuffer{ vertexv } );
					cfg.baseConfig( crg::pp::Config{}
						.programs( { crg::VkPipelineShaderStageCreateInfoArray{ VkPipelineShaderStageCreateInfo{} } } ) );
					auto res = std::make_unique< crg::RenderMesh >( pass, context, runGraph
						, crg::ru::Config{ 1u, true }, std::move( cfg ) );
					renderMesh = res.get();
					return res;
				} );
			testPass.addOutputColourTarget( resultv );

			auto runnable = graph.compile( getContext() );
			test::checkRunnable( testCounts, runnable );
			require( renderMesh )
			checkNoThrow( renderMesh->resetPipeline( crg::VkPipelineShaderStageCreateInfoArray{ VkPipelineShaderStageCreateInfo{} }, 0u ) )
			checkNoThrow( runnable->record() )
			checkNoThrow( runnable->run( VkQueue{} ) )
		}
		testEnd()
	}

	TEST( RunnablePass, RenderMesh_Vertex_Index )
	{
		testBegin( "testRenderMesh_Vertex_Index" )
			crg::ResourceHandler handler;
		auto result = handler.createImageId( test::createImage( "result", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto resultv = handler.createViewId( test::createView( "resultv", result, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
		{
			crg::FrameGraph graph{ handler, testCounts.testName };
			auto vertex = graph.createBuffer( test::createBuffer( "vertex" ) );
			auto index = graph.createBuffer( test::createBuffer( "index" ) );
			auto vertexv = graph.createView( test::createView( "vertexv", vertex ) );
			auto indexv = graph.createView( test::createView( "indexv", index ) );
			crg::RenderMesh * renderMesh{};
			auto & testPass = graph.createPass( "Pass"
				, [&renderMesh, &vertexv, &indexv]( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
					crg::rm::Config cfg;
					auto vb = crg::VertexBuffer{ vertexv };
					cfg.vertexBuffer( std::move( vb ) );
					cfg.indexBuffer( crg::IndexBuffer{ indexv } );
					cfg.baseConfig( crg::pp::Config{}
						.programCreator( crg::ProgramCreator{ 1u
							, []( uint32_t ){ return crg::VkPipelineShaderStageCreateInfoArray{ VkPipelineShaderStageCreateInfo{} }; } } ) );
					auto res = std::make_unique< crg::RenderMesh >( pass, context, runGraph
						, crg::ru::Config{ 1u, true }, std::move( cfg ) );
					renderMesh = res.get();
					return res;
				} );
			testPass.addOutputColourTarget( resultv );

			auto runnable = graph.compile( getContext() );
			test::checkRunnable( testCounts, runnable );
			require( renderMesh )
			checkNoThrow( renderMesh->resetPipeline( crg::VkPipelineShaderStageCreateInfoArray{ VkPipelineShaderStageCreateInfo{} }, 0u ) )
			checkNoThrow( runnable->record() )
			checkNoThrow( runnable->run( VkQueue{} ) )
		}
		testEnd()
	}

	TEST( RunnablePass, RenderMesh_Indirect )
	{
		testBegin( "testRenderMesh_Indirect" )
			crg::ResourceHandler handler;
		auto result = handler.createImageId( test::createImage( "result", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto resultv = handler.createViewId( test::createView( "resultv", result, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
		{
			crg::FrameGraph graph{ handler, testCounts.testName };
			auto indirect = graph.createBuffer( test::createBuffer( "indirect" ) );
			auto indirectv = graph.createView( test::createView( "indirectv", indirect ) );
			crg::RenderMesh * renderMesh{};
			auto & testPass = graph.createPass( "Pass"
				, [&renderMesh, &indirectv]( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
					crg::rm::Config cfg;
					cfg.indirectBuffer( crg::IndirectBuffer{ indirectv, sizeof( VkDrawIndirectCommand ) } );
					cfg.baseConfig( crg::pp::Config{}
						.programCreator( crg::ProgramCreator{ 1u
							, []( uint32_t ){ return crg::VkPipelineShaderStageCreateInfoArray{ VkPipelineShaderStageCreateInfo{} }; } } ) );
					auto res = std::make_unique< crg::RenderMesh >( pass, context, runGraph
						, crg::ru::Config{ 1u, true }, std::move( cfg ) );
					renderMesh = res.get();
					return res;
				} );
			testPass.addOutputColourTarget( resultv );

			auto runnable = graph.compile( getContext() );
			test::checkRunnable( testCounts, runnable );
			require( renderMesh )
			checkNoThrow( renderMesh->resetPipeline( crg::VkPipelineShaderStageCreateInfoArray{ VkPipelineShaderStageCreateInfo{} }, 0u ) )
			checkNoThrow( runnable->record() )
			checkNoThrow( runnable->run( VkQueue{} ) )
		}
		testEnd()
	}

	TEST( RunnablePass, RenderMesh_Indirect_Index )
	{
		testBegin( "testRenderMesh_Indirect_Index" )
			crg::ResourceHandler handler;
		auto result = handler.createImageId( test::createImage( "result", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto resultv = handler.createViewId( test::createView( "resultv", result, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
		{
			crg::FrameGraph graph{ handler, testCounts.testName };
			auto indirect = graph.createBuffer( test::createBuffer( "indirect" ) );
			auto index = graph.createBuffer( test::createBuffer( "index" ) );
			auto indirectv = graph.createView( test::createView( "indirectv", indirect ) );
			auto indexv = graph.createView( test::createView( "indexv", index ) );
			crg::RenderMesh * renderMesh{};
			auto & testPass = graph.createPass( "Pass"
				, [&renderMesh, &indirectv, &indexv]( crg::FramePass const & pass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
					crg::rm::Config cfg;
					cfg.indirectBuffer( crg::IndirectBuffer{ indirectv, sizeof( VkDrawIndexedIndirectCommand ) } );
					cfg.indexBuffer( crg::IndexBuffer{ indexv } );
					cfg.baseConfig( crg::pp::Config{}
						.programCreator( crg::ProgramCreator{ 1u
							, []( uint32_t ){ return crg::VkPipelineShaderStageCreateInfoArray{ VkPipelineShaderStageCreateInfo{} }; } } ) );
					auto res = std::make_unique< crg::RenderMesh >( pass, context, runGraph
						, crg::ru::Config{ 1u, true }, std::move( cfg ) );
					renderMesh = res.get();
					return res;
				} );
			testPass.addOutputColourTarget( resultv );

			auto runnable = graph.compile( getContext() );
			test::checkRunnable( testCounts, runnable );
			require( renderMesh )
			checkNoThrow( renderMesh->resetPipeline( crg::VkPipelineShaderStageCreateInfoArray{ VkPipelineShaderStageCreateInfo{} }, 0u ) )
			checkNoThrow( runnable->record() )
			checkNoThrow( runnable->run( VkQueue{} ) )
		}
		testEnd()
	}

	TEST( RunnablePass, RenderTexturedMesh )
	{
		testBegin( "testRenderTexturedMesh" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };

		auto sampled = graph.createImage( test::createImage( "sampled", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto sampledv = graph.createView( test::createView( "sampledv", sampled, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
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
		auto sampledAttach = testQuad.addOutputColourTarget( sampledv );

		auto result = graph.createImage( test::createImage( "result", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto resultv = graph.createView( test::createView( "resultv", result, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
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
		testMesh.addOutputColourTarget( resultv );
		testMesh.addInputSampled( *sampledAttach, 0u );

		auto runnable = graph.compile( getContext() );
		test::checkRunnable( testCounts, runnable );
		require( renderQuad )
		require( renderMesh )
		checkNoThrow( renderQuad->resetPipeline( crg::VkPipelineShaderStageCreateInfoArray{ VkPipelineShaderStageCreateInfo{} }, 0u ) )
		checkNoThrow( renderQuad->resetPipelineLayout( {}, {}, crg::VkPipelineShaderStageCreateInfoArray{ VkPipelineShaderStageCreateInfo{} }, 0u ) )
		checkNoThrow( renderMesh->resetPipeline( crg::VkPipelineShaderStageCreateInfoArray{ VkPipelineShaderStageCreateInfo{} }, 0u ) )
		checkNoThrow( renderMesh->resetPipelineLayout( {}, {}, crg::VkPipelineShaderStageCreateInfoArray{ VkPipelineShaderStageCreateInfo{} }, 0u ) )
		checkNoThrow( runnable->record() )
		checkNoThrow( runnable->run( crg::SemaphoreWait{ VkSemaphore( 1 ), crg::PipelineStageFlags::eAllGraphics }, VkQueue{} ) )
		checkEqual( graph.getFinalLayoutState( sampledv, 0u ).layout,  crg::ImageLayout::eShaderReadOnly )
		checkEqual( graph.getDefaultGroup().getFinalLayoutState( sampledv, 0u ).layout,  crg::ImageLayout::eShaderReadOnly )
		testEnd()
	}
}

testSuiteMain()
