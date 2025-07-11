#include "Common.hpp"

#include <RenderGraph/FrameGraph.hpp>
#include <RenderGraph/ImageData.hpp>
#include <RenderGraph/ResourceHandler.hpp>
#include <RenderGraph/RunnableGraph.hpp>
#include <RenderGraph/RunnablePass.hpp>
#include <RenderGraph/RunnablePasses/GenerateMipmaps.hpp>

#include <sstream>

namespace
{
	crg::GraphContext & getContext()
	{
		return test::getDummyContext();
	}

	void checkOutputColourIsShaderReadOnly( test::TestCounts & testCounts
		, crg::FramePass const & framePass
		, crg::RunnableGraph const &
		, crg::RecordContext const & context
		, uint32_t index )
	{
		for ( auto & attach : framePass.images )
		{
			auto view = attach.view( index );

			if ( attach.isColourOutputAttach() )
			{
				auto resolved = crg::resolveView( view, index );
				check( context.getNextLayoutState( resolved ).layout == crg::ImageLayout::eShaderReadOnly )
				check( context.getNextLayoutState( resolved.data->image
					, resolved.data->info.viewType
					, resolved.data->info.subresourceRange ).layout == crg::ImageLayout::eShaderReadOnly )
			}
		}
	}

	void checkSampledIsShaderReadOnly( test::TestCounts & testCounts
		, crg::FramePass const & framePass
		, crg::RunnableGraph const &
		, crg::RecordContext const & context
		, uint32_t index )
	{
		for ( auto & attach : framePass.images )
		{
			auto view = attach.view( index );

			if ( attach.isSampledView() )
			{
				check( context.getLayoutState( crg::resolveView( view, index ) ).layout == crg::ImageLayout::eShaderReadOnly )
			}
		}
	}

	crg::FrameGraph buildNoPassGraph( test::TestCounts & testCounts
		, crg::ResourceHandler & handler )
	{
		crg::FrameGraph graph{ handler, testCounts.testName };
		checkThrow( graph.compile( getContext() ) )
		return graph;
	}

	void testNoPass( test::TestCounts & testCounts )
	{
		testBegin( "testNoPass" )
		crg::ResourceHandler handler;
		auto graph1 = buildNoPassGraph( testCounts, handler );
		crg::FrameGraph graph{ std::move( graph1 ) };

		auto rt = graph.createImage( test::createImage( "rt", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto rtv = graph.createView( test::createView( "rtv", rt ) );
		auto & pass = graph.createPass( "pass1C"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		pass.addOutputColourView( rtv );
		testEnd()
	}

	void testOnePass( test::TestCounts & testCounts )
	{
		testBegin( "testOnePass" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto rt = graph.createImage( test::createImage( "rt", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto rtv = graph.createView( test::createView( "rtv", rt ) );
		auto & pass = graph.createPass( "pass1C"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		pass.addOutputColourView(  rtv );
		auto runnable = graph.compile( getContext() );
		auto stream = test::checkRunnable( testCounts, runnable );
		std::string ref = R"(digraph {
}
)";
		checkEqualSortedLines( stream.str(), ref )
		testEnd()
	}

	void testDuplicateName( test::TestCounts & testCounts )
	{
		testBegin( "testDuplicateName" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		checkNoThrow( graph.createPass( "pass1C"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} ) )
		checkThrow( graph.createPass( "pass1C"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} ) )
		testEnd()
	}

	void testOneDependency( test::TestCounts & testCounts )
	{
		testBegin( "testOneDependency" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto rt = graph.createImage( test::createImage( "rt", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto rtv = graph.createView( test::createView( "rtv", rt ) );
		auto & pass1 = graph.createPass( "pass1C"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		pass1.addOutputColourView( rtv );

		auto out = graph.createImage( test::createImage( "out", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto outv = graph.createView( test::createView( "outv", out ) );
		auto & pass2 = graph.createPass( "pass2C"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		pass2.addDependency( pass1 );
		pass2.addSampledView( rtv, 0u );
		pass2.addOutputColourView( outv );

		auto runnable = graph.compile( getContext() );
		auto stream = test::checkRunnable( testCounts, runnable );
		std::string ref = R"(digraph {
  "Transition to\npass2C/rtv/Spl" -> "pass2C" [ label="rtv" ];
  "Transition to\npass2C/rtv/Spl" [ shape=box ];
  "pass1C" -> "Transition to\npass2C/rtv/Spl" [ label="rtv" ];
  "pass1C" [ shape=ellipse ];
  "pass2C" [ shape=ellipse ];
}
)";
		checkEqualSortedLines( stream.str(), ref )
		testEnd()
	}

	void testCycleDependency( test::TestCounts & testCounts )
	{
		testBegin( "testCycleDependency" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto rt = graph.createImage( test::createImage( "rt", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto rtv = graph.createView( test::createView( "rtv", rt ) );
		auto & pass1 = graph.createPass( "pass1C"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		pass1.addOutputColourView( rtv );

		auto out = graph.createImage( test::createImage( "out", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto outv = graph.createView( test::createView( "outv", out ) );
		auto & pass2 = graph.createPass( "pass2C"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		pass2.addDependency( pass1 );
		pass2.addSampledView( rtv, 0u );
		pass2.addOutputColourView( outv );

		auto & pass3 = graph.createPass( "pass3C"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		pass3.addDependency( pass2 );
		pass3.addSampledView( rtv, 0u );
		pass3.addInOutColourView( outv );

		pass1.addDependency( pass3 );
		auto runnable = graph.compile( getContext() );
		auto stream = test::checkRunnable( testCounts, runnable );
		std::string ref = R"(digraph {
  "pass1C" [ shape=ellipse ];
  "pass2C" [ shape=ellipse ];
  "Transition to\npass2C/rtv/Spl" [ shape=box ];
  "pass1C" -> "Transition to\npass2C/rtv/Spl" [ label="rtv" ];
  "Transition to\npass2C/rtv/Spl" -> "pass2C" [ label="rtv" ];
  "pass3C" [ shape=ellipse ];
  "Transition to\npass3C/outv/IOc" [ shape=box ];
  "pass2C" -> "Transition to\npass3C/outv/IOc" [ label="outv" ];
  "Transition to\npass3C/outv/IOc" -> "pass3C" [ label="outv" ];
}
)";
		checkEqualSortedLines( stream.str(), ref )
		testEnd()
	}

	void testUnsortableCycleDependency( test::TestCounts & testCounts )
	{
		testBegin( "testUnsortableCycleDependency" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto rt = graph.createImage( test::createImage( "rt", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto rtv = graph.createView( test::createView( "rtv", rt ) );
		auto & pass1 = graph.createPass( "pass1C"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		pass1.addOutputColourView( rtv );

		auto out = graph.createImage( test::createImage( "out", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto outv = graph.createView( test::createView( "outv", out ) );
		auto & pass2 = graph.createPass( "pass2C"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		pass2.addDependency( pass1 );
		pass2.addSampledView( rtv, 0u );
		pass2.addOutputColourView( outv );

		auto & pass3 = graph.createPass( "pass3C"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		pass3.addDependency( pass2 );
		pass3.addSampledView( rtv, 0u );
		pass3.addInOutColourView( outv );

		pass1.addDependency( pass3 );
		pass1.addDependency( pass2 );
		pass2.addDependency( pass3 );
		checkThrow( graph.compile( getContext() ) );
		testEnd()
	}

	void testChainedDependencies( test::TestCounts & testCounts )
	{
		testBegin( "testChainedDependencies" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto d0 = graph.createImage( test::createImage( "d0", crg::PixelFormat::eR32G32B32_SFLOAT ) );
		auto d0v = graph.createView( test::createView( "d0v", d0 ) );
		auto & pass0 = graph.createPass( "pass0"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		pass0.addOutputColourView( d0v );

		auto d1 = graph.createImage( test::createImage( "d1", crg::PixelFormat::eR32G32B32_SFLOAT ) );
		auto d1v = graph.createView( test::createView( "d1v", d1 ) );
		auto & pass1 = graph.createPass( "pass1"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		pass1.addDependency( pass0 );
		pass1.addSampledView( d0v, 0u );
		pass1.addOutputColourView( d1v );

		auto d2 = graph.createImage( test::createImage( "d2", crg::PixelFormat::eR32G32B32_SFLOAT ) );
		auto d2v = graph.createView( test::createView( "d2v", d2 ) );
		auto & pass2 = graph.createPass( "pass2"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		pass2.addDependency( pass1 );
		pass2.addSampledView( d1v, 0u );
		pass2.addOutputColourView( d2v );

		auto runnable = graph.compile( getContext() );
		auto stream = test::checkRunnable( testCounts, runnable );
		std::string ref = R"(digraph {
  "Transition to\npass1/d0v/Spl" [ shape=box ];
  "pass0" [ shape=ellipse ];
  "pass1" [ shape=ellipse ];
  "pass0" -> "Transition to\npass1/d0v/Spl" [ label="d0v" ];
  "Transition to\npass1/d0v/Spl" -> "pass1" [ label="d0v" ];
  "Transition to\npass2/d1v/Spl" [ shape=box ];
  "pass2" [ shape=ellipse ];
  "pass1" -> "Transition to\npass2/d1v/Spl" [ label="d1v" ];
  "Transition to\npass2/d1v/Spl" -> "pass2" [ label="d1v" ];
}
)";
		checkEqualSortedLines( stream.str(), ref )
		testEnd()
	}

	void testSharedDependencies( test::TestCounts & testCounts )
	{
		testBegin( "testSharedDependencies" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto d = graph.createImage( test::createImage( "d", crg::PixelFormat::eD32_SFLOAT_S8_UINT ) );
		auto dstv1 = graph.createView( test::createView( "dstv1", d ) );
		auto d0 = graph.createImage( test::createImage( "d0", crg::PixelFormat::eR32G32B32_SFLOAT ) );
		auto d0v = graph.createView( test::createView( "d0v", d0 ) );
		auto & pass0 = graph.createPass( "pass0"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		pass0.addOutputDepthView( dstv1 );
		pass0.addOutputColourView( d0v );

		auto dstv2 = graph.createView( test::createView( "dstv2", d ) );
		auto d1 = graph.createImage( test::createImage( "d1", crg::PixelFormat::eR32G32B32_SFLOAT ) );
		auto d1v = graph.createView( test::createView( "d1v", d1 ) );
		auto & pass1 = graph.createPass( "pass1"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		pass1.addDependency( pass0 );
		pass1.addSampledView( d0v, 0 );
		pass1.addOutputDepthView( dstv2 );
		pass1.addOutputColourView( d1v );

		auto d2 = graph.createImage( test::createImage( "d2", crg::PixelFormat::eR32G32B32_SFLOAT ) );
		auto d2v = graph.createView( test::createView( "d2v", d2 ) );
		auto & pass2 = graph.createPass( "pass2"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		pass2.addDependency( pass1 );
		pass2.addSampledView( d1v, 0 );
		pass2.addOutputColourView( d2v );

		auto runnable = graph.compile( getContext() );
		auto stream = test::checkRunnable( testCounts, runnable );
		std::string ref = R"(digraph {
  "Transition to\npass1/d0v/Spl" [ shape=box ];
  "pass0" [ shape=ellipse ];
  "pass1" [ shape=ellipse ];
  "pass0" -> "Transition to\npass1/d0v/Spl" [ label="d0v" ];
  "Transition to\npass1/d0v/Spl" -> "pass1" [ label="d0v" ];
  "Transition to\npass2/d1v/Spl" [ shape=box ];
  "pass2" [ shape=ellipse ];
  "pass1" -> "Transition to\npass2/d1v/Spl" [ label="d1v" ];
  "Transition to\npass2/d1v/Spl" -> "pass2" [ label="d1v" ];
}
)";
		checkEqualSortedLines( stream.str(), ref )
		testEnd()
	}

	void test2MipDependencies( test::TestCounts & testCounts )
	{
		testBegin( "test2MipDependencies" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto lp = graph.createImage( test::createImage( "lp", crg::PixelFormat::eR32G32B32_SFLOAT, 3u ) );
		auto m0v = graph.createView( test::createView( "m0v", lp, 0u ) );
		auto m1v = graph.createView( test::createView( "m1v", lp, 1u ) );
		auto & ssaoMinifyPass1 = graph.createPass( "ssaoMinifyPass1"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		ssaoMinifyPass1.addSampledView( m0v, 0 );
		ssaoMinifyPass1.addOutputColourView( m1v );

		auto m2v = graph.createView( test::createView( "m2v", lp, 2u ) );
		auto & ssaoMinifyPass2 = graph.createPass( "ssaoMinifyPass2"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		ssaoMinifyPass2.addDependency( ssaoMinifyPass1 );
		ssaoMinifyPass2.addSampledView( m1v, 0 );
		ssaoMinifyPass2.addOutputColourView( m2v );

		auto runnable = graph.compile( getContext() );
		auto stream = test::checkRunnable( testCounts, runnable );
		std::string ref = R"(digraph {
  "Transition to\nssaoMinifyPass2/m1v/Spl" [ shape=box ];
  "ssaoMinifyPass1" [ shape=ellipse ];
  "ssaoMinifyPass2" [ shape=ellipse ];
  "ssaoMinifyPass1" -> "Transition to\nssaoMinifyPass2/m1v/Spl" [ label="m1v" ];
  "Transition to\nssaoMinifyPass2/m1v/Spl" -> "ssaoMinifyPass2" [ label="m1v" ];
}
)";
		checkEqualSortedLines( stream.str(), ref )
		testEnd()
	}

	void test3MipDependencies( test::TestCounts & testCounts )
	{
		testBegin( "test3MipDependencies" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto lp = graph.createImage( test::createImage( "lp", crg::PixelFormat::eR32G32B32_SFLOAT, 4u ) );
		auto m0v = graph.createView( test::createView( "m0v", lp, crg::PixelFormat::eR32G32B32_SFLOAT, 0u ) );
		auto m1v = graph.createView( test::createView( "m1v", lp, crg::PixelFormat::eR32G32B32_SFLOAT, 1u ) );
		auto & ssaoMinifyPass1 = graph.createPass( "ssaoMinifyPass1"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		ssaoMinifyPass1.addSampledView( m0v, 0 );
		ssaoMinifyPass1.addOutputColourView( m1v );

		auto m2v = graph.createView( test::createView( "m2v", lp, crg::PixelFormat::eR32G32B32_SFLOAT, 2u ) );
		auto & ssaoMinifyPass2 = graph.createPass( "ssaoMinifyPass2"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		ssaoMinifyPass2.addDependency( ssaoMinifyPass1 );
		ssaoMinifyPass2.addSampledView( m1v, 0 );
		ssaoMinifyPass2.addOutputColourView( m2v );

		auto m3v = graph.createView( test::createView( "m3v", lp, crg::PixelFormat::eR32G32B32_SFLOAT, 3u ) );
		auto & ssaoMinifyPass3 = graph.createPass( "ssaoMinifyPass3"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		ssaoMinifyPass3.addDependency( ssaoMinifyPass2 );
		ssaoMinifyPass3.addSampledView( m2v, 0 );
		ssaoMinifyPass3.addOutputColourView( m3v );

		auto runnable = graph.compile( getContext() );
		auto stream = test::checkRunnable( testCounts, runnable );
		std::string ref = R"(digraph {
  "Transition to\nssaoMinifyPass2/m1v/Spl" [ shape=box ];
  "ssaoMinifyPass1" [ shape=ellipse ];
  "ssaoMinifyPass2" [ shape=ellipse ];
  "ssaoMinifyPass1" -> "Transition to\nssaoMinifyPass2/m1v/Spl" [ label="m1v" ];
  "Transition to\nssaoMinifyPass2/m1v/Spl" -> "ssaoMinifyPass2" [ label="m1v" ];
  "Transition to\nssaoMinifyPass3/m2v/Spl" [ shape=box ];
  "ssaoMinifyPass3" [ shape=ellipse ];
  "ssaoMinifyPass2" -> "Transition to\nssaoMinifyPass3/m2v/Spl" [ label="m2v" ];
  "Transition to\nssaoMinifyPass3/m2v/Spl" -> "ssaoMinifyPass3" [ label="m2v" ];
}
)";
		checkEqualSortedLines( stream.str(), ref )
		testEnd()
	}

	void testLoopDependencies( test::TestCounts & testCounts )
	{
		testBegin( "testLoopDependencies" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto a = graph.createImage( test::createImage( "a", crg::PixelFormat::eR32G32B32_SFLOAT ) );
		auto av = graph.createView( test::createView( "av", a, crg::PixelFormat::eR32G32B32_SFLOAT ) );
		auto b = graph.createImage( test::createImage( "b", crg::PixelFormat::eR32G32B32_SFLOAT ) );
		auto bv = graph.createView( test::createView( "bv", b, crg::PixelFormat::eR32G32B32_SFLOAT ) );
		auto & pass1 = graph.createPass( "pass1"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		pass1.addSampledView( bv, 0 );
		pass1.addOutputColourView( av );

		auto & pass2 = graph.createPass( "pass2"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		pass2.addDependency( pass1 );
		pass2.addSampledView( av, 0 );
		pass2.addOutputColourView( bv );

		auto runnable = graph.compile( getContext() );
		test::checkRunnable( testCounts, runnable );
		testEnd()
	}

	void testLoopDependenciesWithRoot( test::TestCounts & testCounts )
	{
		testBegin( "testLoopDependenciesWithRoot" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto b = graph.createImage( test::createImage( "b", crg::PixelFormat::eR32G32B32_SFLOAT ) );
		auto bv = graph.createView( test::createView( "bv", b, crg::PixelFormat::eR32G32B32_SFLOAT ) );
		auto & pass0 = graph.createPass( "pass0"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		pass0.addOutputColourView( bv );

		auto a = graph.createImage( test::createImage( "a", crg::PixelFormat::eR32G32B32_SFLOAT ) );
		auto av = graph.createView( test::createView( "av", a, crg::PixelFormat::eR32G32B32_SFLOAT ) );
		auto & pass1 = graph.createPass( "pass1"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		pass1.addDependency( pass0 );
		pass1.addSampledView( bv, 0 );
		pass1.addOutputColourView( av );

		auto & pass2 = graph.createPass( "pass2"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		pass2.addDependency( pass1 );
		pass2.addSampledView( av, 0 );
		pass2.addOutputColourView( bv );

		auto runnable = graph.compile( getContext() );
		test::checkRunnable( testCounts, runnable );
		testEnd()
	}

	void testLoopDependenciesWithRootAndLeaf( test::TestCounts & testCounts )
	{
		testBegin( "testLoopDependenciesWithRootAndLeaf" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto c = graph.createImage( test::createImage( "c", crg::PixelFormat::eR32G32B32_SFLOAT ) );
		auto cv = graph.createView( test::createView( "cv", c, crg::PixelFormat::eR32G32B32_SFLOAT ) );
		auto & pass0 = graph.createPass( "pass0"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		pass0.addOutputColourView( cv );

		auto a = graph.createImage( test::createImage( "a", crg::PixelFormat::eR32G32B32_SFLOAT ) );
		auto av = graph.createView( test::createView( "av", a, crg::PixelFormat::eR32G32B32_SFLOAT ) );
		auto b = graph.createImage( test::createImage( "b", crg::PixelFormat::eR32G32B32_SFLOAT ) );
		auto bv = graph.createView( test::createView( "bv", b, crg::PixelFormat::eR32G32B32_SFLOAT ) );
		auto & pass1 = graph.createPass( "pass1"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		pass1.addDependency( pass0 );
		pass1.addSampledView( bv, 0 );
		pass1.addSampledView( cv, 1 );
		pass1.addOutputColourView( av );

		auto & pass2 = graph.createPass( "pass2"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		pass2.addDependency( pass1 );
		pass2.addSampledView( av, 0 );
		pass2.addSampledView( cv, 1 );
		pass2.addOutputColourView( bv );

		auto & pass3 = graph.createPass( "pass3"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		pass3.addDependency( pass2 );
		pass3.addSampledView( cv, 0 );

		auto runnable = graph.compile( getContext() );
		auto stream = test::checkRunnable( testCounts, runnable );
		std::string ref = R"(digraph {
  "Transition to\npass1/cv/Spl" [ shape=box ];
  "pass0" [ shape=ellipse ];
  "pass1" [ shape=ellipse ];
  "pass0" -> "Transition to\npass1/cv/Spl" [ label="cv" ];
  "Transition to\npass1/cv/Spl" -> "pass1" [ label="cv" ];
  "Transition to\npass2/av/Spl" [ shape=box ];
  "pass2" [ shape=ellipse ];
  "pass1" -> "Transition to\npass2/av/Spl" [ label="av" ];
  "Transition to\npass2/av/Spl" -> "pass2" [ label="av" ];
}
)";
		checkEqualSortedLines( stream.str(), ref )
		testEnd()
	}

	std::pair< crg::ImageViewId, crg::FramePass * > buildSsaoPass( test::TestCounts & testCounts
		, crg::FramePass const & previous
		, crg::ImageViewId const & dsv
		, crg::FrameGraph & graph )
	{
		auto lp = graph.createImage( test::createImage( "lp", crg::PixelFormat::eR32_SFLOAT, 4u ) );
		auto m0v = graph.createView( test::createView( "m0v", lp, crg::PixelFormat::eR32_SFLOAT, 0u ) );
		auto & ssaoLinearisePass = graph.createPass( "ssaoLinearisePass"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		ssaoLinearisePass.addDependency( previous );
		ssaoLinearisePass.addSampledView( dsv, 0 );
		ssaoLinearisePass.addOutputColourView( m0v );

		auto m1v = graph.createView( test::createView( "m1v", lp, crg::PixelFormat::eR32_SFLOAT, 1u ) );
		auto & ssaoMinifyPass1 = graph.createPass( "ssaoMinifyPass1"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		ssaoMinifyPass1.addDependency( ssaoLinearisePass );
		ssaoMinifyPass1.addSampledView( m0v, 0 );
		ssaoMinifyPass1.addOutputColourView( m1v );

		auto m2v = graph.createView( test::createView( "m2v", lp, crg::PixelFormat::eR32G32B32_SFLOAT, 2u ) );
		auto & ssaoMinifyPass2 = graph.createPass( "ssaoMinifyPass2"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		ssaoMinifyPass2.addDependency( ssaoMinifyPass1 );
		ssaoMinifyPass2.addSampledView( m1v, 0 );
		ssaoMinifyPass2.addOutputColourView( m2v );

		auto m3v = graph.createView( test::createView( "m3v", lp, crg::PixelFormat::eR32G32B32_SFLOAT, 3u ) );
		auto & ssaoMinifyPass3 = graph.createPass( "ssaoMinifyPass3"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		ssaoMinifyPass3.addDependency( ssaoMinifyPass2 );
		ssaoMinifyPass3.addSampledView( m2v, 0 );
		ssaoMinifyPass3.addOutputColourView( m3v );

		auto mv = graph.createView( test::createView("mv",  lp, crg::PixelFormat::eR32_SFLOAT, 0u, 4u ) );
		auto rs = graph.createImage( test::createImage( "rs", crg::PixelFormat::eR32_SFLOAT ) );
		auto rsv = graph.createView( test::createView( "rsv", rs, crg::PixelFormat::eR32_SFLOAT ) );
		auto & ssaoRawPass = graph.createPass( "ssaoRawPass"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		ssaoRawPass.addDependency( ssaoMinifyPass3 );
		ssaoRawPass.addSampledView( mv, 0 );
		ssaoRawPass.addSampledView( m3v, 1 );
		ssaoRawPass.addOutputColourView( rsv );

		auto bl = graph.createImage( test::createImage( "b1", crg::PixelFormat::eR32_SFLOAT ) );
		auto blv = graph.createView( test::createView( "b1v", bl, crg::PixelFormat::eR32_SFLOAT ) );
		auto & ssaoBlurPass = graph.createPass( "ssaoBlurPass"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		ssaoBlurPass.addDependency( ssaoRawPass );
		ssaoBlurPass.addSampledView( rsv, 0 );
		ssaoBlurPass.addSampledView( m3v, 1 );
		ssaoBlurPass.addOutputColourView( blv );

		return { blv, &ssaoBlurPass };
	}

	void testSsaoPass( test::TestCounts & testCounts )
	{
		testBegin( "testSsaoPass" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto d = graph.createImage( test::createImage( "d", crg::PixelFormat::eD32_SFLOAT_S8_UINT ) );
		auto dtv = graph.createView( test::createView( "dtv", d, crg::PixelFormat::eD32_SFLOAT_S8_UINT ) );
		auto v = graph.createImage( test::createImage( "v", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto vv = graph.createView( test::createView( "vv", v, crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto d1 = graph.createImage( test::createImage( "d1", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto d1v = graph.createView( test::createView( "d1v", d1, crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto d2 = graph.createImage( test::createImage( "d2", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto d2v = graph.createView( test::createView( "d2v", d2, crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto d3 = graph.createImage( test::createImage( "d3", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto d3v = graph.createView( test::createView( "d3v", d3, crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto d4 = graph.createImage( test::createImage( "d4", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto d4v = graph.createView( test::createView( "d4v", d4, crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto & geometryPass = graph.createPass( "geometryPass"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		geometryPass.addOutputColourView( d1v );
		geometryPass.addOutputColourView( d2v );
		geometryPass.addOutputColourView( d3v );
		geometryPass.addOutputColourView( d4v );
		geometryPass.addOutputColourView( vv );
		geometryPass.addOutputDepthStencilView( dtv );

		auto dsv = graph.createView( test::createView( "dsv", d, crg::PixelFormat::eR32_SFLOAT ) );
		auto [ssaoResult, ssaoPass] = buildSsaoPass( testCounts
			, geometryPass
			, dsv
			, graph );

		auto of = graph.createImage( test::createImage( "of", crg::PixelFormat::eR32G32B32_SFLOAT ) );
		auto ofv = graph.createView( test::createView( "ofv", of, crg::PixelFormat::eR32G32B32_SFLOAT ) );
		auto & ambientPass = graph.createPass( "ambientPass"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		ambientPass.addDependency( *ssaoPass );
		ambientPass.addSampledView( dsv, 0 );
		ambientPass.addSampledView( d1v, 1 );
		ambientPass.addSampledView( d2v, 2 );
		ambientPass.addSampledView( d3v, 3 );
		ambientPass.addSampledView( d4v, 4 );
		ambientPass.addSampledView( ssaoResult, 5 );
		ambientPass.addOutputColourView( ofv );

		auto runnable = graph.compile( getContext() );
		auto stream = test::checkRunnable( testCounts, runnable );
		std::string ref = R"(digraph {
  "Transition to\nssaoLinearisePass/dsv/Spl" [ shape=box ];
  "geometryPass" [ shape=ellipse ];
  "ssaoLinearisePass" [ shape=ellipse ];
  "geometryPass" -> "Transition to\nssaoLinearisePass/dsv/Spl" [ label="dtv" ];
  "Transition to\nssaoLinearisePass/dsv/Spl" -> "ssaoLinearisePass" [ label="dtv" ];
  "Transition to\nssaoMinifyPass1/m0v/Spl" [ shape=box ];
  "ssaoMinifyPass1" [ shape=ellipse ];
  "ssaoLinearisePass" -> "Transition to\nssaoMinifyPass1/m0v/Spl" [ label="m0v" ];
  "Transition to\nssaoMinifyPass1/m0v/Spl" -> "ssaoMinifyPass1" [ label="m0v" ];
  "Transition to\nssaoMinifyPass2/m1v/Spl" [ shape=box ];
  "ssaoMinifyPass2" [ shape=ellipse ];
  "ssaoMinifyPass1" -> "Transition to\nssaoMinifyPass2/m1v/Spl" [ label="m1v" ];
  "Transition to\nssaoMinifyPass2/m1v/Spl" -> "ssaoMinifyPass2" [ label="m1v" ];
  "Transition to\nssaoMinifyPass3/m2v/Spl" [ shape=box ];
  "ssaoMinifyPass3" [ shape=ellipse ];
  "ssaoMinifyPass2" -> "Transition to\nssaoMinifyPass3/m2v/Spl" [ label="m2v" ];
  "Transition to\nssaoMinifyPass3/m2v/Spl" -> "ssaoMinifyPass3" [ label="m2v" ];
  "Transition to\nssaoRawPass/m3v/Spl" [ shape=box ];
  "ssaoRawPass" [ shape=ellipse ];
  "ssaoMinifyPass3" -> "Transition to\nssaoRawPass/m3v/Spl" [ label="m3v" ];
  "Transition to\nssaoRawPass/m3v/Spl" -> "ssaoRawPass" [ label="m3v" ];
  "Transition to\nssaoBlurPass/rsv/Spl" [ shape=box ];
  "ssaoBlurPass" [ shape=ellipse ];
  "ssaoRawPass" -> "Transition to\nssaoBlurPass/rsv/Spl" [ label="rsv" ];
  "Transition to\nssaoBlurPass/rsv/Spl" -> "ssaoBlurPass" [ label="rsv" ];
  "Transition to\nambientPass/b1v/Spl" [ shape=box ];
  "ambientPass" [ shape=ellipse ];
  "ssaoBlurPass" -> "Transition to\nambientPass/b1v/Spl" [ label="b1v" ];
  "Transition to\nambientPass/b1v/Spl" -> "ambientPass" [ label="b1v" ];
  "Transition to\nambientPass/d1v/Spl" [ shape=box ];
  "geometryPass" [ shape=ellipse ];
  "ambientPass" [ shape=ellipse ];
  "geometryPass" -> "Transition to\nambientPass/d1v/Spl" [ label="d1v" ];
  "Transition to\nambientPass/d1v/Spl" -> "ambientPass" [ label="d1v" ];
  "Transition to\nambientPass/d2v/Spl" [ shape=box ];
  "ambientPass" [ shape=ellipse ];
  "geometryPass" -> "Transition to\nambientPass/d2v/Spl" [ label="d2v" ];
  "Transition to\nambientPass/d2v/Spl" -> "ambientPass" [ label="d2v" ];
  "Transition to\nambientPass/d3v/Spl" [ shape=box ];
  "geometryPass" [ shape=ellipse ];
  "ambientPass" [ shape=ellipse ];
  "geometryPass" -> "Transition to\nambientPass/d3v/Spl" [ label="d3v" ];
  "Transition to\nambientPass/d3v/Spl" -> "ambientPass" [ label="d3v" ];
  "Transition to\nambientPass/d4v/Spl" [ shape=box ];
  "ambientPass" [ shape=ellipse ];
  "geometryPass" -> "Transition to\nambientPass/d4v/Spl" [ label="d4v" ];
  "Transition to\nambientPass/d4v/Spl" -> "ambientPass" [ label="d4v" ];
  "Transition to\nambientPass/dsv/Spl" [ shape=box ];
  "ambientPass" [ shape=ellipse ];
  "geometryPass" -> "Transition to\nambientPass/dsv/Spl" [ label="dtv" ];
  "Transition to\nambientPass/dsv/Spl" -> "ambientPass" [ label="dtv" ];
}
)";
		checkEqualSortedLines( stream.str(), ref )
		testEnd()
	}

	void testBloomPostEffect( test::TestCounts & testCounts )
	{
		testBegin( "testBloomPostEffect" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto scene = graph.createImage( test::createImage( "scene", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto scenev = graph.createView( test::createView( "scenev", scene, crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto output = graph.createImage( test::createImage( "output", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto outputv = graph.createView( test::createView( "outputv", output, crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto hi = graph.createImage( test::createImage( "hi", crg::PixelFormat::eR32G32B32A32_SFLOAT, 4u ) );
		auto hi0v = graph.createView( test::createView( "hi0v", hi, crg::PixelFormat::eR32G32B32A32_SFLOAT, 0u ) );
		auto hi1v = graph.createView( test::createView( "hi1v", hi, crg::PixelFormat::eR32G32B32A32_SFLOAT, 1u ) );
		auto hi2v = graph.createView( test::createView( "hi2v", hi, crg::PixelFormat::eR32G32B32A32_SFLOAT, 2u ) );
		auto hi3v = graph.createView( test::createView( "hi3v", hi, crg::PixelFormat::eR32G32B32A32_SFLOAT, 3u ) );
		auto bl = graph.createImage( test::createImage( "bl", crg::PixelFormat::eR32G32B32A32_SFLOAT, 4u ) );
		auto bl0v = graph.createView( test::createView( "bl0v", bl, crg::PixelFormat::eR32G32B32A32_SFLOAT, 0u ) );
		auto bl1v = graph.createView( test::createView( "bl1v", bl, crg::PixelFormat::eR32G32B32A32_SFLOAT, 1u ) );
		auto bl2v = graph.createView( test::createView( "bl2v", bl, crg::PixelFormat::eR32G32B32A32_SFLOAT, 2u ) );
		auto bl3v = graph.createView( test::createView( "bl3v", bl, crg::PixelFormat::eR32G32B32A32_SFLOAT, 3u ) );

		auto & hiPass = graph.createPass( "hiPass"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		hiPass.addSampledView( scenev, 0u );
		hiPass.addOutputColourView( hiPass.mergeViews( { hi0v, hi1v, hi2v, hi3v } ) );

		auto & blurPass0X = graph.createPass( "blurPass0X"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		blurPass0X.addDependency( hiPass );
		blurPass0X.addSampledView( hi0v, 0u );
		blurPass0X.addOutputColourView( bl0v );
		auto & blurPass0Y = graph.createPass( "blurPass0Y"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		blurPass0Y.addDependency( blurPass0X );
		blurPass0Y.addSampledView( bl0v, 0u );
		blurPass0Y.addOutputColourView( hi0v );

		auto & blurPass1X = graph.createPass( "blurPass1X"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		blurPass1X.addDependency( hiPass );
		blurPass1X.addSampledView( hi1v, 0u );
		blurPass1X.addOutputColourView( bl1v );
		auto & blurPass1Y = graph.createPass( "blurPass1Y"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		blurPass1Y.addDependency( blurPass1X );
		blurPass1Y.addSampledView( bl1v, 0u );
		blurPass1Y.addOutputColourView( hi1v );

		auto & blurPass2X = graph.createPass( "blurPass2X"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		blurPass2X.addDependency( hiPass );
		blurPass2X.addSampledView( hi2v, 0u );
		blurPass2X.addOutputColourView( bl2v );
		auto & blurPass2Y = graph.createPass( "blurPass2Y"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		blurPass2Y.addDependency( blurPass2X );
		blurPass2Y.addSampledView( bl2v, 0u );
		blurPass2Y.addOutputColourView( hi2v );

		auto & blurPass3X = graph.createPass( "blurPass3X"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		blurPass3X.addDependency( hiPass );
		blurPass3X.addSampledView( hi3v, 0u );
		blurPass3X.addOutputColourView( bl3v );
		auto & blurPass3Y = graph.createPass( "blurPass3Y"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		blurPass3Y.addDependency( blurPass3X );
		blurPass3Y.addSampledView( bl3v, 0u );
		blurPass3Y.addOutputColourView( hi3v );

		auto & combinePass = graph.createPass( "combinePass"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		combinePass.addDependency( blurPass0Y );
		combinePass.addDependency( blurPass1Y );
		combinePass.addDependency( blurPass2Y );
		combinePass.addDependency( blurPass3Y );
		combinePass.addSampledView( scenev, 0u );
		combinePass.addSampledView( combinePass.mergeViews( { hi0v, hi1v, hi2v, hi3v } ), 1u );
		combinePass.addOutputColourView( outputv );

		auto runnable = graph.compile( getContext() );
		test::checkRunnable( testCounts, runnable );
		testEnd()
	}

	template< bool EnableSsao >
	std::pair< crg::ImageViewId, crg::FramePass * > buildDeferred( test::TestCounts & testCounts
		, crg::FramePass const * previous
		, crg::ImageViewId const & dsv
		, crg::ImageViewId const & dtv
		, crg::ImageViewId const & vtv
		, crg::FrameGraph & graph )
	{
		auto d1 = graph.createImage( test::createImage( "d1", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto d1v = graph.createView( test::createView( "d1v", d1, crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto d2 = graph.createImage( test::createImage( "d2", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto d2v = graph.createView( test::createView( "d2v", d2, crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto d3 = graph.createImage( test::createImage( "d3", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto d3v = graph.createView( test::createView( "d3v", d3, crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto d4 = graph.createImage( test::createImage( "d4", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto d4v = graph.createView( test::createView( "d4v", d4, crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto & geometryPass = graph.createPass( "geometryPass"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );

		if ( previous )
		{
			geometryPass.addDependency( *previous );
		}

		geometryPass.addOutputColourView( d1v );
		geometryPass.addOutputColourView( d2v );
		geometryPass.addOutputColourView( d3v );
		geometryPass.addOutputColourView( d4v );
		geometryPass.addOutputColourView( vtv );
		geometryPass.addOutputDepthStencilView( dtv );

		auto df = graph.createImage( test::createImage( "df", crg::PixelFormat::eR32G32B32_SFLOAT ) );
		auto dfv = graph.createView( test::createView( "dfv", df, crg::PixelFormat::eR32G32B32_SFLOAT ) );
		auto sp = graph.createImage( test::createImage( "sp", crg::PixelFormat::eR32G32B32_SFLOAT ) );
		auto spv = graph.createView( test::createView( "spv", sp, crg::PixelFormat::eR32G32B32_SFLOAT ) );
		auto & lightingPass = graph.createPass( "lightingPass"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		lightingPass.addDependency( geometryPass );
		lightingPass.addSampledView( dtv, 0 );
		lightingPass.addSampledView( d1v, 1 );
		lightingPass.addSampledView( d2v, 2 );
		lightingPass.addSampledView( d3v, 3 );
		lightingPass.addSampledView( d4v, 4 );
		lightingPass.addOutputColourView( dfv );
		lightingPass.addOutputColourView( spv );

		auto of = graph.createImage( test::createImage( "of", crg::PixelFormat::eR32G32B32_SFLOAT ) );
		auto ofv = graph.createView( test::createView( "ofv", of, crg::PixelFormat::eR32G32B32_SFLOAT ) );

		if constexpr ( EnableSsao )
		{
			auto [ssaoResult, ssaoPass] = buildSsaoPass( testCounts
				, geometryPass
				, dsv
				, graph );
			auto & ambientPass = graph.createPass( "ambientPass"
				, [&testCounts]( crg::FramePass const & framePass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
					return createDummy( testCounts
						, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
				} );
			ambientPass.addDependency( *ssaoPass );
			ambientPass.addDependency( lightingPass );
			ambientPass.addSampledView( dsv, 0 );
			ambientPass.addSampledView( d1v, 1 );
			ambientPass.addSampledView( d2v, 2 );
			ambientPass.addSampledView( d3v, 3 );
			ambientPass.addSampledView( d4v, 4 );
			ambientPass.addSampledView( dfv, 5 );
			ambientPass.addSampledView( spv, 6 );
			ambientPass.addSampledView( ssaoResult, 7 );
			ambientPass.addOutputColourView( ofv );
			return { ofv, &ambientPass };
		}
		else
		{
			auto & ambientPass = graph.createPass( "ambientPass"
				, [&testCounts]( crg::FramePass const & framePass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
					return createDummy( testCounts
						, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
				} );
			ambientPass.addDependency( lightingPass );
			ambientPass.addSampledView( dsv, 0 );
			ambientPass.addSampledView( d1v, 1 );
			ambientPass.addSampledView( d2v, 2 );
			ambientPass.addSampledView( d3v, 3 );
			ambientPass.addSampledView( d4v, 4 );
			ambientPass.addSampledView( dfv, 5 );
			ambientPass.addSampledView( spv, 6 );
			ambientPass.addOutputColourView( ofv );
			return { ofv, &ambientPass };
		}
	}
    
	std::pair< crg::ImageViewId, crg::FramePass * > buildWeightedBlended( test::TestCounts & testCounts
		, crg::FramePass const * previous
		, crg::ImageViewId const & dsv
		, crg::ImageViewId const & dtv
		, crg::ImageViewId const & vtv
		, crg::FrameGraph & graph )
	{
		auto a = graph.createImage( test::createImage( "a", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto av = graph.createView( test::createView( "av", a, crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto r = graph.createImage( test::createImage( "r", crg::PixelFormat::eR16_SFLOAT ) );
		auto rv = graph.createView( test::createView( "rv", r, crg::PixelFormat::eR16_SFLOAT ) );
		auto & accumulationPass = graph.createPass( "accumulationPass"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );

		if ( previous )
		{
			accumulationPass.addDependency( *previous );
		}

		accumulationPass.addInOutColourView( av );
		accumulationPass.addInOutColourView( rv );
		accumulationPass.addInOutColourView( vtv );
		accumulationPass.addOutputDepthStencilView( dtv );

		auto c = graph.createImage( test::createImage( "c", crg::PixelFormat::eR32G32B32_SFLOAT ) );
		auto cv = graph.createView( test::createView( "cv", c, crg::PixelFormat::eR32G32B32_SFLOAT ) );
		auto & combinePass = graph.createPass( "combinePass"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		combinePass.addDependency( accumulationPass );
		combinePass.addSampledView( dsv, 0u );
		combinePass.addSampledView( av, 1u );
		combinePass.addSampledView( rv, 2u );
		combinePass.addOutputColourView( cv );

		return { cv, &combinePass };
	}

	template< bool EnableDepthPrepass
		, bool EnableOpaque
		, bool EnableSsao
		, bool EnableTransparent >
	void testRender( test::TestCounts & testCounts )
	{
		testBegin( "testRender"
			+ ( EnableDepthPrepass ? std::string{ "Prepass" } : std::string{} )
			+ ( EnableOpaque ? std::string{ "Opaque" } : std::string{} )
			+ ( EnableSsao ? std::string{ "Ssao" } : std::string{} )
			+ ( EnableTransparent ? std::string{ "Transparent" } : std::string{} ) )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto d = graph.createImage( test::createImage( "d", crg::PixelFormat::eD32_SFLOAT_S8_UINT ) );
		auto dtv = graph.createView( test::createView( "dtv", d, crg::PixelFormat::eD32_SFLOAT_S8_UINT ) );
		crg::FramePass * previous{};

		if constexpr ( EnableDepthPrepass )
		{
			auto & depthPrepass = graph.createPass( "depthPrepass"
				, [&testCounts]( crg::FramePass const & framePass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
					return createDummy( testCounts
						, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
				} );
			depthPrepass.addOutputDepthView( dtv );
			previous = &depthPrepass;
		}

		auto dsv = graph.createView( test::createView( "dsv", d, crg::PixelFormat::eR32_SFLOAT ) );
		auto o = graph.createImage( test::createImage( "o", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto otv = graph.createView( test::createView( "otv", o, crg::PixelFormat::eR16G16B16A16_SFLOAT ) );

		if constexpr ( EnableOpaque )
		{
			auto v = graph.createImage( test::createImage( "v", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
			auto vv = graph.createView( test::createView( "vv", v, crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
			auto dcs = buildDeferred< EnableSsao >( testCounts
				, previous
				, dsv
				, dtv
				, vv
				, graph );

			if constexpr ( EnableTransparent )
			{
				auto wbcsv = buildWeightedBlended( testCounts
					, dcs.second
					, dsv
					, dtv
					, vv
					, graph );
				auto & finalCombinePass = graph.createPass( "finalCombinePass"
					, [&testCounts]( crg::FramePass const & framePass
						, crg::GraphContext & context
						, crg::RunnableGraph & runGraph )
					{
						return createDummy( testCounts
							, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
					} );
				finalCombinePass.addDependency( *wbcsv.second );
				finalCombinePass.addSampledView( dcs.first, 0 );
				finalCombinePass.addSampledView( wbcsv.first, 1 );
				finalCombinePass.addSampledView( vv, 2 );
				finalCombinePass.addOutputColourView( otv );
			}
			else
			{
				auto & finalCombinePass = graph.createPass( "finalCombinePass"
					, [&testCounts]( crg::FramePass const & framePass
						, crg::GraphContext & context
						, crg::RunnableGraph & runGraph )
					{
						return createDummy( testCounts
							, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
					} );
				finalCombinePass.addDependency( *dcs.second );
				finalCombinePass.addSampledView( dcs.first, 0 );
				finalCombinePass.addSampledView( vv, 1 );
				finalCombinePass.addOutputColourView( otv );
			}
			
		}
		else if constexpr ( EnableTransparent )
		{
			auto v = graph.createImage( test::createImage( "v", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
			auto vv = graph.createView( test::createView( "vv", v, crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
			auto [wbResult, wbPass] = buildWeightedBlended( testCounts
				, previous
				, dsv
				, dtv
				, vv
				, graph );
			auto & finalCombinePass = graph.createPass( "finalCombinePass"
				, [&testCounts]( crg::FramePass const & framePass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
					return createDummy( testCounts
						, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
				} );
			finalCombinePass.addDependency( *wbPass );
			finalCombinePass.addSampledView( wbResult, 0 );
			finalCombinePass.addSampledView( vv, 1 );
			finalCombinePass.addOutputColourView( otv );
		}
		else
		{
			auto & finalCombinePass = graph.createPass( "finalCombinePass"
				, [&testCounts]( crg::FramePass const & framePass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
					return createDummy( testCounts
						, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
				} );

			if ( previous )
			{
				finalCombinePass.addDependency( *previous );
			}

			finalCombinePass.addSampledView( dsv, 0 );
			finalCombinePass.addOutputColourView( otv );
		}

		auto runnable = graph.compile( getContext() );
		auto stream = test::checkRunnable( testCounts, runnable );
		std::string ref;

		if constexpr ( EnableDepthPrepass )
		{
			if constexpr ( EnableOpaque )
			{
				if constexpr ( EnableSsao )
				{
					if constexpr ( EnableTransparent )
					{
						ref = R"(digraph {
  "Transition to\nlightingPass/d1v/Spl" [ shape=box ];
  "geometryPass" [ shape=ellipse ];
  "lightingPass" [ shape=ellipse ];
  "geometryPass" -> "Transition to\nlightingPass/d1v/Spl" [ label="d1v" ];
  "Transition to\nlightingPass/d1v/Spl" -> "lightingPass" [ label="d1v" ];
  "Transition to\nlightingPass/d2v/Spl" [ shape=box ];
  "geometryPass" -> "Transition to\nlightingPass/d2v/Spl" [ label="d2v" ];
  "Transition to\nlightingPass/d2v/Spl" -> "lightingPass" [ label="d2v" ];
  "Transition to\nlightingPass/d3v/Spl" [ shape=box ];
  "geometryPass" -> "Transition to\nlightingPass/d3v/Spl" [ label="d3v" ];
  "Transition to\nlightingPass/d3v/Spl" -> "lightingPass" [ label="d3v" ];
  "Transition to\nlightingPass/d4v/Spl" [ shape=box ];
  "geometryPass" -> "Transition to\nlightingPass/d4v/Spl" [ label="d4v" ];
  "Transition to\nlightingPass/d4v/Spl" -> "lightingPass" [ label="d4v" ];
  "Transition to\nlightingPass/dtv/Spl" [ shape=box ];
  "geometryPass" -> "Transition to\nlightingPass/dtv/Spl" [ label="dtv" ];
  "Transition to\nlightingPass/dtv/Spl" -> "lightingPass" [ label="dtv" ];
  "Transition to\nambientPass/dfv/Spl" [ shape=box ];
  "ambientPass" [ shape=ellipse ];
  "lightingPass" -> "Transition to\nambientPass/dfv/Spl" [ label="dfv" ];
  "Transition to\nambientPass/dfv/Spl" -> "ambientPass" [ label="dfv" ];
  "Transition to\nambientPass/spv/Spl" [ shape=box ];
  "lightingPass" -> "Transition to\nambientPass/spv/Spl" [ label="spv" ];
  "Transition to\nambientPass/spv/Spl" -> "ambientPass" [ label="spv" ];
  "Transition to\nfinalCombinePass/ofv/Spl" [ shape=box ];
  "finalCombinePass" [ shape=ellipse ];
  "ambientPass" -> "Transition to\nfinalCombinePass/ofv/Spl" [ label="ofv" ];
  "Transition to\nfinalCombinePass/ofv/Spl" -> "finalCombinePass" [ label="ofv" ];
  "Transition to\nssaoLinearisePass/dsv/Spl" [ shape=box ];
  "ssaoLinearisePass" [ shape=ellipse ];
  "geometryPass" -> "Transition to\nssaoLinearisePass/dsv/Spl" [ label="dtv" ];
  "Transition to\nssaoLinearisePass/dsv/Spl" -> "ssaoLinearisePass" [ label="dtv" ];
  "Transition to\nssaoMinifyPass1/m0v/Spl" [ shape=box ];
  "ssaoMinifyPass1" [ shape=ellipse ];
  "ssaoLinearisePass" -> "Transition to\nssaoMinifyPass1/m0v/Spl" [ label="m0v" ];
  "Transition to\nssaoMinifyPass1/m0v/Spl" -> "ssaoMinifyPass1" [ label="m0v" ];
  "Transition to\nssaoMinifyPass2/m1v/Spl" [ shape=box ];
  "ssaoMinifyPass2" [ shape=ellipse ];
  "ssaoMinifyPass1" -> "Transition to\nssaoMinifyPass2/m1v/Spl" [ label="m1v" ];
  "Transition to\nssaoMinifyPass2/m1v/Spl" -> "ssaoMinifyPass2" [ label="m1v" ];
  "Transition to\nssaoMinifyPass3/m2v/Spl" [ shape=box ];
  "ssaoMinifyPass3" [ shape=ellipse ];
  "ssaoMinifyPass2" -> "Transition to\nssaoMinifyPass3/m2v/Spl" [ label="m2v" ];
  "Transition to\nssaoMinifyPass3/m2v/Spl" -> "ssaoMinifyPass3" [ label="m2v" ];
  "Transition to\nssaoRawPass/m3v/Spl" [ shape=box ];
  "ssaoRawPass" [ shape=ellipse ];
  "ssaoMinifyPass3" -> "Transition to\nssaoRawPass/m3v/Spl" [ label="m3v" ];
  "Transition to\nssaoRawPass/m3v/Spl" -> "ssaoRawPass" [ label="m3v" ];
  "Transition to\nssaoBlurPass/rsv/Spl" [ shape=box ];
  "ssaoBlurPass" [ shape=ellipse ];
  "ssaoRawPass" -> "Transition to\nssaoBlurPass/rsv/Spl" [ label="rsv" ];
  "Transition to\nssaoBlurPass/rsv/Spl" -> "ssaoBlurPass" [ label="rsv" ];
  "Transition to\nambientPass/b1v/Spl" [ shape=box ];
  "ssaoBlurPass" -> "Transition to\nambientPass/b1v/Spl" [ label="b1v" ];
  "Transition to\nambientPass/b1v/Spl" -> "ambientPass" [ label="b1v" ];
  "Transition to\nambientPass/d1v/Spl" [ shape=box ];
  "geometryPass" -> "Transition to\nambientPass/d1v/Spl" [ label="d1v" ];
  "Transition to\nambientPass/d1v/Spl" -> "ambientPass" [ label="d1v" ];
  "Transition to\nambientPass/d2v/Spl" [ shape=box ];
  "geometryPass" -> "Transition to\nambientPass/d2v/Spl" [ label="d2v" ];
  "Transition to\nambientPass/d2v/Spl" -> "ambientPass" [ label="d2v" ];
  "Transition to\nambientPass/d3v/Spl" [ shape=box ];
  "geometryPass" -> "Transition to\nambientPass/d3v/Spl" [ label="d3v" ];
  "Transition to\nambientPass/d3v/Spl" -> "ambientPass" [ label="d3v" ];
  "Transition to\nambientPass/d4v/Spl" [ shape=box ];
  "geometryPass" -> "Transition to\nambientPass/d4v/Spl" [ label="d4v" ];
  "Transition to\nambientPass/d4v/Spl" -> "ambientPass" [ label="d4v" ];
  "Transition to\nambientPass/dsv/Spl" [ shape=box ];
  "geometryPass" -> "Transition to\nambientPass/dsv/Spl" [ label="dtv" ];
  "Transition to\nambientPass/dsv/Spl" -> "ambientPass" [ label="dtv" ];
  "Transition to\naccumulationPass/vv/IOc" [ shape=box ];
  "accumulationPass" [ shape=ellipse ];
  "geometryPass" -> "Transition to\naccumulationPass/vv/IOc" [ label="vv" ];
  "Transition to\naccumulationPass/vv/IOc" -> "accumulationPass" [ label="vv" ];
  "Transition to\ncombinePass/av/Spl" [ shape=box ];
  "combinePass" [ shape=ellipse ];
  "accumulationPass" -> "Transition to\ncombinePass/av/Spl" [ label="av" ];
  "Transition to\ncombinePass/av/Spl" -> "combinePass" [ label="av" ];
  "Transition to\ncombinePass/dsv/Spl" [ shape=box ];
  "accumulationPass" -> "Transition to\ncombinePass/dsv/Spl" [ label="dtv" ];
  "Transition to\ncombinePass/dsv/Spl" -> "combinePass" [ label="dtv" ];
  "Transition to\ncombinePass/rv/Spl" [ shape=box ];
  "accumulationPass" -> "Transition to\ncombinePass/rv/Spl" [ label="rv" ];
  "Transition to\ncombinePass/rv/Spl" -> "combinePass" [ label="rv" ];
  "Transition to\nfinalCombinePass/cv/Spl" [ shape=box ];
  "combinePass" -> "Transition to\nfinalCombinePass/cv/Spl" [ label="cv" ];
  "Transition to\nfinalCombinePass/cv/Spl" -> "finalCombinePass" [ label="cv" ];
  "Transition to\nfinalCombinePass/vv/Spl" [ shape=box ];
  "accumulationPass" -> "Transition to\nfinalCombinePass/vv/Spl" [ label="vv" ];
  "Transition to\nfinalCombinePass/vv/Spl" -> "finalCombinePass" [ label="vv" ];
}
)";
					}
					else
					{
						ref = R"(digraph {
  "Transition to\nlightingPass/d1v/Spl" [ shape=box ];
  "geometryPass" [ shape=ellipse ];
  "lightingPass" [ shape=ellipse ];
  "geometryPass" -> "Transition to\nlightingPass/d1v/Spl" [ label="d1v" ];
  "Transition to\nlightingPass/d1v/Spl" -> "lightingPass" [ label="d1v" ];
  "Transition to\nlightingPass/d2v/Spl" [ shape=box ];
  "geometryPass" -> "Transition to\nlightingPass/d2v/Spl" [ label="d2v" ];
  "Transition to\nlightingPass/d2v/Spl" -> "lightingPass" [ label="d2v" ];
  "Transition to\nlightingPass/d3v/Spl" [ shape=box ];
  "geometryPass" -> "Transition to\nlightingPass/d3v/Spl" [ label="d3v" ];
  "Transition to\nlightingPass/d3v/Spl" -> "lightingPass" [ label="d3v" ];
  "Transition to\nlightingPass/d4v/Spl" [ shape=box ];
  "geometryPass" -> "Transition to\nlightingPass/d4v/Spl" [ label="d4v" ];
  "Transition to\nlightingPass/d4v/Spl" -> "lightingPass" [ label="d4v" ];
  "Transition to\nlightingPass/dtv/Spl" [ shape=box ];
  "geometryPass" -> "Transition to\nlightingPass/dtv/Spl" [ label="dtv" ];
  "Transition to\nlightingPass/dtv/Spl" -> "lightingPass" [ label="dtv" ];
  "Transition to\nambientPass/dfv/Spl" [ shape=box ];
  "ambientPass" [ shape=ellipse ];
  "lightingPass" -> "Transition to\nambientPass/dfv/Spl" [ label="dfv" ];
  "Transition to\nambientPass/dfv/Spl" -> "ambientPass" [ label="dfv" ];
  "Transition to\nambientPass/spv/Spl" [ shape=box ];
  "lightingPass" -> "Transition to\nambientPass/spv/Spl" [ label="spv" ];
  "Transition to\nambientPass/spv/Spl" -> "ambientPass" [ label="spv" ];
  "Transition to\nfinalCombinePass/ofv/Spl" [ shape=box ];
  "finalCombinePass" [ shape=ellipse ];
  "ambientPass" -> "Transition to\nfinalCombinePass/ofv/Spl" [ label="ofv" ];
  "Transition to\nfinalCombinePass/ofv/Spl" -> "finalCombinePass" [ label="ofv" ];
  "Transition to\nssaoLinearisePass/dsv/Spl" [ shape=box ];
  "ssaoLinearisePass" [ shape=ellipse ];
  "geometryPass" -> "Transition to\nssaoLinearisePass/dsv/Spl" [ label="dtv" ];
  "Transition to\nssaoLinearisePass/dsv/Spl" -> "ssaoLinearisePass" [ label="dtv" ];
  "Transition to\nssaoMinifyPass1/m0v/Spl" [ shape=box ];
  "ssaoMinifyPass1" [ shape=ellipse ];
  "ssaoLinearisePass" -> "Transition to\nssaoMinifyPass1/m0v/Spl" [ label="m0v" ];
  "Transition to\nssaoMinifyPass1/m0v/Spl" -> "ssaoMinifyPass1" [ label="m0v" ];
  "Transition to\nssaoMinifyPass2/m1v/Spl" [ shape=box ];
  "ssaoMinifyPass2" [ shape=ellipse ];
  "ssaoMinifyPass1" -> "Transition to\nssaoMinifyPass2/m1v/Spl" [ label="m1v" ];
  "Transition to\nssaoMinifyPass2/m1v/Spl" -> "ssaoMinifyPass2" [ label="m1v" ];
  "Transition to\nssaoMinifyPass3/m2v/Spl" [ shape=box ];
  "ssaoMinifyPass3" [ shape=ellipse ];
  "ssaoMinifyPass2" -> "Transition to\nssaoMinifyPass3/m2v/Spl" [ label="m2v" ];
  "Transition to\nssaoMinifyPass3/m2v/Spl" -> "ssaoMinifyPass3" [ label="m2v" ];
  "Transition to\nssaoRawPass/m3v/Spl" [ shape=box ];
  "ssaoRawPass" [ shape=ellipse ];
  "ssaoMinifyPass3" -> "Transition to\nssaoRawPass/m3v/Spl" [ label="m3v" ];
  "Transition to\nssaoRawPass/m3v/Spl" -> "ssaoRawPass" [ label="m3v" ];
  "Transition to\nssaoBlurPass/rsv/Spl" [ shape=box ];
  "ssaoBlurPass" [ shape=ellipse ];
  "ssaoRawPass" -> "Transition to\nssaoBlurPass/rsv/Spl" [ label="rsv" ];
  "Transition to\nssaoBlurPass/rsv/Spl" -> "ssaoBlurPass" [ label="rsv" ];
  "Transition to\nambientPass/b1v/Spl" [ shape=box ];
  "ssaoBlurPass" -> "Transition to\nambientPass/b1v/Spl" [ label="b1v" ];
  "Transition to\nambientPass/b1v/Spl" -> "ambientPass" [ label="b1v" ];
  "Transition to\nambientPass/d1v/Spl" [ shape=box ];
  "geometryPass" -> "Transition to\nambientPass/d1v/Spl" [ label="d1v" ];
  "Transition to\nambientPass/d1v/Spl" -> "ambientPass" [ label="d1v" ];
  "Transition to\nambientPass/d2v/Spl" [ shape=box ];
  "geometryPass" -> "Transition to\nambientPass/d2v/Spl" [ label="d2v" ];
  "Transition to\nambientPass/d2v/Spl" -> "ambientPass" [ label="d2v" ];
  "Transition to\nambientPass/d3v/Spl" [ shape=box ];
  "geometryPass" -> "Transition to\nambientPass/d3v/Spl" [ label="d3v" ];
  "Transition to\nambientPass/d3v/Spl" -> "ambientPass" [ label="d3v" ];
  "Transition to\nambientPass/d4v/Spl" [ shape=box ];
  "geometryPass" -> "Transition to\nambientPass/d4v/Spl" [ label="d4v" ];
  "Transition to\nambientPass/d4v/Spl" -> "ambientPass" [ label="d4v" ];
  "Transition to\nambientPass/dsv/Spl" [ shape=box ];
  "geometryPass" -> "Transition to\nambientPass/dsv/Spl" [ label="dtv" ];
  "Transition to\nambientPass/dsv/Spl" -> "ambientPass" [ label="dtv" ];
  "Transition to\nfinalCombinePass/vv/Spl" [ shape=box ];
  "geometryPass" -> "Transition to\nfinalCombinePass/vv/Spl" [ label="vv" ];
  "Transition to\nfinalCombinePass/vv/Spl" -> "finalCombinePass" [ label="vv" ];
}
)";
					}
				}
				else if constexpr ( EnableTransparent )
				{
					ref = R"(digraph {
  "Transition to\nlightingPass/d1v/Spl" [ shape=box ];
  "geometryPass" [ shape=ellipse ];
  "lightingPass" [ shape=ellipse ];
  "geometryPass" -> "Transition to\nlightingPass/d1v/Spl" [ label="d1v" ];
  "Transition to\nlightingPass/d1v/Spl" -> "lightingPass" [ label="d1v" ];
  "Transition to\nlightingPass/d2v/Spl" [ shape=box ];
  "geometryPass" -> "Transition to\nlightingPass/d2v/Spl" [ label="d2v" ];
  "Transition to\nlightingPass/d2v/Spl" -> "lightingPass" [ label="d2v" ];
  "Transition to\nlightingPass/d3v/Spl" [ shape=box ];
  "geometryPass" -> "Transition to\nlightingPass/d3v/Spl" [ label="d3v" ];
  "Transition to\nlightingPass/d3v/Spl" -> "lightingPass" [ label="d3v" ];
  "Transition to\nlightingPass/d4v/Spl" [ shape=box ];
  "geometryPass" -> "Transition to\nlightingPass/d4v/Spl" [ label="d4v" ];
  "Transition to\nlightingPass/d4v/Spl" -> "lightingPass" [ label="d4v" ];
  "Transition to\nlightingPass/dtv/Spl" [ shape=box ];
  "geometryPass" -> "Transition to\nlightingPass/dtv/Spl" [ label="dtv" ];
  "Transition to\nlightingPass/dtv/Spl" -> "lightingPass" [ label="dtv" ];
  "Transition to\nambientPass/dfv/Spl" [ shape=box ];
  "ambientPass" [ shape=ellipse ];
  "lightingPass" -> "Transition to\nambientPass/dfv/Spl" [ label="dfv" ];
  "Transition to\nambientPass/dfv/Spl" -> "ambientPass" [ label="dfv" ];
  "Transition to\nambientPass/spv/Spl" [ shape=box ];
  "lightingPass" -> "Transition to\nambientPass/spv/Spl" [ label="spv" ];
  "Transition to\nambientPass/spv/Spl" -> "ambientPass" [ label="spv" ];
  "Transition to\nfinalCombinePass/ofv/Spl" [ shape=box ];
  "finalCombinePass" [ shape=ellipse ];
  "ambientPass" -> "Transition to\nfinalCombinePass/ofv/Spl" [ label="ofv" ];
  "Transition to\nfinalCombinePass/ofv/Spl" -> "finalCombinePass" [ label="ofv" ];
  "Transition to\naccumulationPass/vv/IOc" [ shape=box ];
  "accumulationPass" [ shape=ellipse ];
  "geometryPass" -> "Transition to\naccumulationPass/vv/IOc" [ label="vv" ];
  "Transition to\naccumulationPass/vv/IOc" -> "accumulationPass" [ label="vv" ];
  "Transition to\ncombinePass/av/Spl" [ shape=box ];
  "combinePass" [ shape=ellipse ];
  "accumulationPass" -> "Transition to\ncombinePass/av/Spl" [ label="av" ];
  "Transition to\ncombinePass/av/Spl" -> "combinePass" [ label="av" ];
  "Transition to\ncombinePass/dsv/Spl" [ shape=box ];
  "accumulationPass" -> "Transition to\ncombinePass/dsv/Spl" [ label="dtv" ];
  "Transition to\ncombinePass/dsv/Spl" -> "combinePass" [ label="dtv" ];
  "Transition to\ncombinePass/rv/Spl" [ shape=box ];
  "accumulationPass" -> "Transition to\ncombinePass/rv/Spl" [ label="rv" ];
  "Transition to\ncombinePass/rv/Spl" -> "combinePass" [ label="rv" ];
  "Transition to\nfinalCombinePass/cv/Spl" [ shape=box ];
  "combinePass" -> "Transition to\nfinalCombinePass/cv/Spl" [ label="cv" ];
  "Transition to\nfinalCombinePass/cv/Spl" -> "finalCombinePass" [ label="cv" ];
  "Transition to\nfinalCombinePass/vv/Spl" [ shape=box ];
  "accumulationPass" -> "Transition to\nfinalCombinePass/vv/Spl" [ label="vv" ];
  "Transition to\nfinalCombinePass/vv/Spl" -> "finalCombinePass" [ label="vv" ];
}
)";
				}
				else
				{
					ref = R"(digraph {
  "Transition to\nlightingPass/d1v/Spl" [ shape=box ];
  "geometryPass" [ shape=ellipse ];
  "lightingPass" [ shape=ellipse ];
  "geometryPass" -> "Transition to\nlightingPass/d1v/Spl" [ label="d1v" ];
  "Transition to\nlightingPass/d1v/Spl" -> "lightingPass" [ label="d1v" ];
  "Transition to\nlightingPass/d2v/Spl" [ shape=box ];
  "geometryPass" -> "Transition to\nlightingPass/d2v/Spl" [ label="d2v" ];
  "Transition to\nlightingPass/d2v/Spl" -> "lightingPass" [ label="d2v" ];
  "Transition to\nlightingPass/d3v/Spl" [ shape=box ];
  "geometryPass" -> "Transition to\nlightingPass/d3v/Spl" [ label="d3v" ];
  "Transition to\nlightingPass/d3v/Spl" -> "lightingPass" [ label="d3v" ];
  "Transition to\nlightingPass/d4v/Spl" [ shape=box ];
  "geometryPass" -> "Transition to\nlightingPass/d4v/Spl" [ label="d4v" ];
  "Transition to\nlightingPass/d4v/Spl" -> "lightingPass" [ label="d4v" ];
  "Transition to\nlightingPass/dtv/Spl" [ shape=box ];
  "geometryPass" -> "Transition to\nlightingPass/dtv/Spl" [ label="dtv" ];
  "Transition to\nlightingPass/dtv/Spl" -> "lightingPass" [ label="dtv" ];
  "Transition to\nambientPass/dfv/Spl" [ shape=box ];
  "ambientPass" [ shape=ellipse ];
  "lightingPass" -> "Transition to\nambientPass/dfv/Spl" [ label="dfv" ];
  "Transition to\nambientPass/dfv/Spl" -> "ambientPass" [ label="dfv" ];
  "Transition to\nambientPass/spv/Spl" [ shape=box ];
  "lightingPass" -> "Transition to\nambientPass/spv/Spl" [ label="spv" ];
  "Transition to\nambientPass/spv/Spl" -> "ambientPass" [ label="spv" ];
  "Transition to\nfinalCombinePass/ofv/Spl" [ shape=box ];
  "finalCombinePass" [ shape=ellipse ];
  "ambientPass" -> "Transition to\nfinalCombinePass/ofv/Spl" [ label="ofv" ];
  "Transition to\nfinalCombinePass/ofv/Spl" -> "finalCombinePass" [ label="ofv" ];
  "Transition to\nfinalCombinePass/vv/Spl" [ shape=box ];
  "geometryPass" -> "Transition to\nfinalCombinePass/vv/Spl" [ label="vv" ];
  "Transition to\nfinalCombinePass/vv/Spl" -> "finalCombinePass" [ label="vv" ];
}
)";
				}
			}
			else if constexpr ( EnableTransparent )
			{
				ref = R"(digraph {
  "Transition to\ncombinePass/av/Spl" [ shape=box ];
  "accumulationPass" [ shape=ellipse ];
  "combinePass" [ shape=ellipse ];
  "accumulationPass" -> "Transition to\ncombinePass/av/Spl" [ label="av" ];
  "Transition to\ncombinePass/av/Spl" -> "combinePass" [ label="av" ];
  "Transition to\ncombinePass/dsv/Spl" [ shape=box ];
  "accumulationPass" [ shape=ellipse ];
  "accumulationPass" -> "Transition to\ncombinePass/dsv/Spl" [ label="dtv" ];
  "Transition to\ncombinePass/dsv/Spl" -> "combinePass" [ label="dtv" ];
  "Transition to\ncombinePass/rv/Spl" [ shape=box ];
  "accumulationPass" [ shape=ellipse ];
  "accumulationPass" -> "Transition to\ncombinePass/rv/Spl" [ label="rv" ];
  "Transition to\ncombinePass/rv/Spl" -> "combinePass" [ label="rv" ];
  "Transition to\nfinalCombinePass/cv/Spl" [ shape=box ];
  "finalCombinePass" [ shape=ellipse ];
  "combinePass" -> "Transition to\nfinalCombinePass/cv/Spl" [ label="cv" ];
  "Transition to\nfinalCombinePass/cv/Spl" -> "finalCombinePass" [ label="cv" ];
  "Transition to\nfinalCombinePass/vv/Spl" [ shape=box ];
  "accumulationPass" [ shape=ellipse ];
  "accumulationPass" -> "Transition to\nfinalCombinePass/vv/Spl" [ label="vv" ];
  "Transition to\nfinalCombinePass/vv/Spl" -> "finalCombinePass" [ label="vv" ];
}
)";
			}
			else
			{
				ref = R"(digraph {
  "Transition to\nfinalCombinePass/dsv/Spl" [ shape=box ];
  "depthPrepass" [ shape=ellipse ];
  "finalCombinePass" [ shape=ellipse ];
  "depthPrepass" -> "Transition to\nfinalCombinePass/dsv/Spl" [ label="dtv" ];
  "Transition to\nfinalCombinePass/dsv/Spl" -> "finalCombinePass" [ label="dtv" ];
}
)";
			}
		}
		else
		{
			if constexpr ( EnableOpaque )
			{
				if constexpr ( EnableSsao )
				{
					if constexpr ( EnableTransparent )
					{
						ref = R"(digraph {
  "Transition to\nlightingPass/d1v/Spl" [ shape=box ];
  "geometryPass" [ shape=ellipse ];
  "lightingPass" [ shape=ellipse ];
  "geometryPass" -> "Transition to\nlightingPass/d1v/Spl" [ label="d1v" ];
  "Transition to\nlightingPass/d1v/Spl" -> "lightingPass" [ label="d1v" ];
  "Transition to\nlightingPass/d2v/Spl" [ shape=box ];
  "geometryPass" [ shape=ellipse ];
  "geometryPass" -> "Transition to\nlightingPass/d2v/Spl" [ label="d2v" ];
  "Transition to\nlightingPass/d2v/Spl" -> "lightingPass" [ label="d2v" ];
  "Transition to\nlightingPass/d3v/Spl" [ shape=box ];
  "geometryPass" [ shape=ellipse ];
  "geometryPass" -> "Transition to\nlightingPass/d3v/Spl" [ label="d3v" ];
  "Transition to\nlightingPass/d3v/Spl" -> "lightingPass" [ label="d3v" ];
  "Transition to\nlightingPass/d4v/Spl" [ shape=box ];
  "geometryPass" [ shape=ellipse ];
  "geometryPass" -> "Transition to\nlightingPass/d4v/Spl" [ label="d4v" ];
  "Transition to\nlightingPass/d4v/Spl" -> "lightingPass" [ label="d4v" ];
  "Transition to\nlightingPass/dtv/Spl" [ shape=box ];
  "geometryPass" [ shape=ellipse ];
  "geometryPass" -> "Transition to\nlightingPass/dtv/Spl" [ label="dtv" ];
  "Transition to\nlightingPass/dtv/Spl" -> "lightingPass" [ label="dtv" ];
  "Transition to\nambientPass/dfv/Spl" [ shape=box ];
  "ambientPass" [ shape=ellipse ];
  "lightingPass" -> "Transition to\nambientPass/dfv/Spl" [ label="dfv" ];
  "Transition to\nambientPass/dfv/Spl" -> "ambientPass" [ label="dfv" ];
  "Transition to\nambientPass/spv/Spl" [ shape=box ];
  "ambientPass" [ shape=ellipse ];
  "lightingPass" -> "Transition to\nambientPass/spv/Spl" [ label="spv" ];
  "Transition to\nambientPass/spv/Spl" -> "ambientPass" [ label="spv" ];
  "Transition to\nfinalCombinePass/ofv/Spl" [ shape=box ];
  "finalCombinePass" [ shape=ellipse ];
  "ambientPass" -> "Transition to\nfinalCombinePass/ofv/Spl" [ label="ofv" ];
  "Transition to\nfinalCombinePass/ofv/Spl" -> "finalCombinePass" [ label="ofv" ];
  "Transition to\nssaoLinearisePass/dsv/Spl" [ shape=box ];
  "geometryPass" [ shape=ellipse ];
  "ssaoLinearisePass" [ shape=ellipse ];
  "geometryPass" -> "Transition to\nssaoLinearisePass/dsv/Spl" [ label="dtv" ];
  "Transition to\nssaoLinearisePass/dsv/Spl" -> "ssaoLinearisePass" [ label="dtv" ];
  "Transition to\nssaoMinifyPass1/m0v/Spl" [ shape=box ];
  "ssaoMinifyPass1" [ shape=ellipse ];
  "ssaoLinearisePass" -> "Transition to\nssaoMinifyPass1/m0v/Spl" [ label="m0v" ];
  "Transition to\nssaoMinifyPass1/m0v/Spl" -> "ssaoMinifyPass1" [ label="m0v" ];
  "Transition to\nssaoMinifyPass2/m1v/Spl" [ shape=box ];
  "ssaoMinifyPass2" [ shape=ellipse ];
  "ssaoMinifyPass1" -> "Transition to\nssaoMinifyPass2/m1v/Spl" [ label="m1v" ];
  "Transition to\nssaoMinifyPass2/m1v/Spl" -> "ssaoMinifyPass2" [ label="m1v" ];
  "Transition to\nssaoMinifyPass3/m2v/Spl" [ shape=box ];
  "ssaoMinifyPass3" [ shape=ellipse ];
  "ssaoMinifyPass2" -> "Transition to\nssaoMinifyPass3/m2v/Spl" [ label="m2v" ];
  "Transition to\nssaoMinifyPass3/m2v/Spl" -> "ssaoMinifyPass3" [ label="m2v" ];
  "Transition to\nssaoRawPass/m3v/Spl" [ shape=box ];
  "ssaoRawPass" [ shape=ellipse ];
  "ssaoMinifyPass3" -> "Transition to\nssaoRawPass/m3v/Spl" [ label="m3v" ];
  "Transition to\nssaoRawPass/m3v/Spl" -> "ssaoRawPass" [ label="m3v" ];
  "Transition to\nssaoBlurPass/rsv/Spl" [ shape=box ];
  "ssaoBlurPass" [ shape=ellipse ];
  "ssaoRawPass" -> "Transition to\nssaoBlurPass/rsv/Spl" [ label="rsv" ];
  "Transition to\nssaoBlurPass/rsv/Spl" -> "ssaoBlurPass" [ label="rsv" ];
  "Transition to\nambientPass/b1v/Spl" [ shape=box ];
  "ambientPass" [ shape=ellipse ];
  "ssaoBlurPass" -> "Transition to\nambientPass/b1v/Spl" [ label="b1v" ];
  "Transition to\nambientPass/b1v/Spl" -> "ambientPass" [ label="b1v" ];
  "Transition to\nambientPass/d1v/Spl" [ shape=box ];
  "ambientPass" [ shape=ellipse ];
  "geometryPass" -> "Transition to\nambientPass/d1v/Spl" [ label="d1v" ];
  "Transition to\nambientPass/d1v/Spl" -> "ambientPass" [ label="d1v" ];
  "Transition to\nambientPass/d2v/Spl" [ shape=box ];
  "ambientPass" [ shape=ellipse ];
  "geometryPass" -> "Transition to\nambientPass/d2v/Spl" [ label="d2v" ];
  "Transition to\nambientPass/d2v/Spl" -> "ambientPass" [ label="d2v" ];
  "Transition to\nambientPass/d3v/Spl" [ shape=box ];
  "geometryPass" [ shape=ellipse ];
  "ambientPass" [ shape=ellipse ];
  "geometryPass" -> "Transition to\nambientPass/d3v/Spl" [ label="d3v" ];
  "Transition to\nambientPass/d3v/Spl" -> "ambientPass" [ label="d3v" ];
  "Transition to\nambientPass/d4v/Spl" [ shape=box ];
  "geometryPass" [ shape=ellipse ];
  "ambientPass" [ shape=ellipse ];
  "geometryPass" -> "Transition to\nambientPass/d4v/Spl" [ label="d4v" ];
  "Transition to\nambientPass/d4v/Spl" -> "ambientPass" [ label="d4v" ];
  "Transition to\nambientPass/dsv/Spl" [ shape=box ];
  "geometryPass" [ shape=ellipse ];
  "geometryPass" -> "Transition to\nambientPass/dsv/Spl" [ label="dtv" ];
  "Transition to\nambientPass/dsv/Spl" -> "ambientPass" [ label="dtv" ];
  "Transition to\naccumulationPass/vv/IOc" [ shape=box ];
  "accumulationPass" [ shape=ellipse ];
  "geometryPass" -> "Transition to\naccumulationPass/vv/IOc" [ label="vv" ];
  "Transition to\naccumulationPass/vv/IOc" -> "accumulationPass" [ label="vv" ];
  "Transition to\ncombinePass/av/Spl" [ shape=box ];
  "combinePass" [ shape=ellipse ];
  "accumulationPass" -> "Transition to\ncombinePass/av/Spl" [ label="av" ];
  "Transition to\ncombinePass/av/Spl" -> "combinePass" [ label="av" ];
  "Transition to\ncombinePass/dsv/Spl" [ shape=box ];
  "accumulationPass" -> "Transition to\ncombinePass/dsv/Spl" [ label="dtv" ];
  "Transition to\ncombinePass/dsv/Spl" -> "combinePass" [ label="dtv" ];
  "Transition to\ncombinePass/rv/Spl" [ shape=box ];
  "accumulationPass" [ shape=ellipse ];
  "combinePass" [ shape=ellipse ];
  "accumulationPass" -> "Transition to\ncombinePass/rv/Spl" [ label="rv" ];
  "Transition to\ncombinePass/rv/Spl" -> "combinePass" [ label="rv" ];
  "Transition to\nfinalCombinePass/cv/Spl" [ shape=box ];
  "finalCombinePass" [ shape=ellipse ];
  "combinePass" -> "Transition to\nfinalCombinePass/cv/Spl" [ label="cv" ];
  "Transition to\nfinalCombinePass/cv/Spl" -> "finalCombinePass" [ label="cv" ];
  "Transition to\nfinalCombinePass/vv/Spl" [ shape=box ];
  "accumulationPass" -> "Transition to\nfinalCombinePass/vv/Spl" [ label="vv" ];
  "Transition to\nfinalCombinePass/vv/Spl" -> "finalCombinePass" [ label="vv" ];
}
)";
					}
					else
					{
						ref = R"(digraph {
  "Transition to\nlightingPass/d1v/Spl" [ shape=box ];
  "geometryPass" [ shape=ellipse ];
  "lightingPass" [ shape=ellipse ];
  "geometryPass" -> "Transition to\nlightingPass/d1v/Spl" [ label="d1v" ];
  "Transition to\nlightingPass/d1v/Spl" -> "lightingPass" [ label="d1v" ];
  "Transition to\nlightingPass/d2v/Spl" [ shape=box ];
  "geometryPass" [ shape=ellipse ];
  "geometryPass" -> "Transition to\nlightingPass/d2v/Spl" [ label="d2v" ];
  "Transition to\nlightingPass/d2v/Spl" -> "lightingPass" [ label="d2v" ];
  "Transition to\nlightingPass/d3v/Spl" [ shape=box ];
  "geometryPass" [ shape=ellipse ];
  "geometryPass" -> "Transition to\nlightingPass/d3v/Spl" [ label="d3v" ];
  "Transition to\nlightingPass/d3v/Spl" -> "lightingPass" [ label="d3v" ];
  "Transition to\nlightingPass/d4v/Spl" [ shape=box ];
  "geometryPass" [ shape=ellipse ];
  "geometryPass" -> "Transition to\nlightingPass/d4v/Spl" [ label="d4v" ];
  "Transition to\nlightingPass/d4v/Spl" -> "lightingPass" [ label="d4v" ];
  "Transition to\nlightingPass/dtv/Spl" [ shape=box ];
  "geometryPass" [ shape=ellipse ];
  "geometryPass" -> "Transition to\nlightingPass/dtv/Spl" [ label="dtv" ];
  "Transition to\nlightingPass/dtv/Spl" -> "lightingPass" [ label="dtv" ];
  "Transition to\nambientPass/dfv/Spl" [ shape=box ];
  "ambientPass" [ shape=ellipse ];
  "lightingPass" -> "Transition to\nambientPass/dfv/Spl" [ label="dfv" ];
  "Transition to\nambientPass/dfv/Spl" -> "ambientPass" [ label="dfv" ];
  "Transition to\nambientPass/spv/Spl" [ shape=box ];
  "ambientPass" [ shape=ellipse ];
  "lightingPass" -> "Transition to\nambientPass/spv/Spl" [ label="spv" ];
  "Transition to\nambientPass/spv/Spl" -> "ambientPass" [ label="spv" ];
  "Transition to\nfinalCombinePass/ofv/Spl" [ shape=box ];
  "ambientPass" [ shape=ellipse ];
  "finalCombinePass" [ shape=ellipse ];
  "ambientPass" -> "Transition to\nfinalCombinePass/ofv/Spl" [ label="ofv" ];
  "Transition to\nfinalCombinePass/ofv/Spl" -> "finalCombinePass" [ label="ofv" ];
  "Transition to\nssaoLinearisePass/dsv/Spl" [ shape=box ];
  "ssaoLinearisePass" [ shape=ellipse ];
  "geometryPass" -> "Transition to\nssaoLinearisePass/dsv/Spl" [ label="dtv" ];
  "Transition to\nssaoLinearisePass/dsv/Spl" -> "ssaoLinearisePass" [ label="dtv" ];
  "Transition to\nssaoMinifyPass1/m0v/Spl" [ shape=box ];
  "ssaoMinifyPass1" [ shape=ellipse ];
  "ssaoLinearisePass" -> "Transition to\nssaoMinifyPass1/m0v/Spl" [ label="m0v" ];
  "Transition to\nssaoMinifyPass1/m0v/Spl" -> "ssaoMinifyPass1" [ label="m0v" ];
  "Transition to\nssaoMinifyPass2/m1v/Spl" [ shape=box ];
  "ssaoMinifyPass2" [ shape=ellipse ];
  "ssaoMinifyPass1" -> "Transition to\nssaoMinifyPass2/m1v/Spl" [ label="m1v" ];
  "Transition to\nssaoMinifyPass2/m1v/Spl" -> "ssaoMinifyPass2" [ label="m1v" ];
  "Transition to\nssaoMinifyPass3/m2v/Spl" [ shape=box ];
  "ssaoMinifyPass3" [ shape=ellipse ];
  "ssaoMinifyPass2" -> "Transition to\nssaoMinifyPass3/m2v/Spl" [ label="m2v" ];
  "Transition to\nssaoMinifyPass3/m2v/Spl" -> "ssaoMinifyPass3" [ label="m2v" ];
  "Transition to\nssaoRawPass/m3v/Spl" [ shape=box ];
  "ssaoRawPass" [ shape=ellipse ];
  "ssaoMinifyPass3" -> "Transition to\nssaoRawPass/m3v/Spl" [ label="m3v" ];
  "Transition to\nssaoRawPass/m3v/Spl" -> "ssaoRawPass" [ label="m3v" ];
  "Transition to\nssaoBlurPass/rsv/Spl" [ shape=box ];
  "ssaoBlurPass" [ shape=ellipse ];
  "ssaoRawPass" -> "Transition to\nssaoBlurPass/rsv/Spl" [ label="rsv" ];
  "Transition to\nssaoBlurPass/rsv/Spl" -> "ssaoBlurPass" [ label="rsv" ];
  "Transition to\nambientPass/b1v/Spl" [ shape=box ];
  "ambientPass" [ shape=ellipse ];
  "ssaoBlurPass" -> "Transition to\nambientPass/b1v/Spl" [ label="b1v" ];
  "Transition to\nambientPass/b1v/Spl" -> "ambientPass" [ label="b1v" ];
  "Transition to\nambientPass/d1v/Spl" [ shape=box ];
  "ambientPass" [ shape=ellipse ];
  "geometryPass" -> "Transition to\nambientPass/d1v/Spl" [ label="d1v" ];
  "Transition to\nambientPass/d1v/Spl" -> "ambientPass" [ label="d1v" ];
  "Transition to\nambientPass/d2v/Spl" [ shape=box ];
  "ambientPass" [ shape=ellipse ];
  "geometryPass" -> "Transition to\nambientPass/d2v/Spl" [ label="d2v" ];
  "Transition to\nambientPass/d2v/Spl" -> "ambientPass" [ label="d2v" ];
  "Transition to\nambientPass/d3v/Spl" [ shape=box ];
  "ambientPass" [ shape=ellipse ];
  "geometryPass" -> "Transition to\nambientPass/d3v/Spl" [ label="d3v" ];
  "Transition to\nambientPass/d3v/Spl" -> "ambientPass" [ label="d3v" ];
  "Transition to\nambientPass/d4v/Spl" [ shape=box ];
  "ambientPass" [ shape=ellipse ];
  "geometryPass" -> "Transition to\nambientPass/d4v/Spl" [ label="d4v" ];
  "Transition to\nambientPass/d4v/Spl" -> "ambientPass" [ label="d4v" ];
  "Transition to\nambientPass/dsv/Spl" [ shape=box ];
  "ambientPass" [ shape=ellipse ];
  "geometryPass" -> "Transition to\nambientPass/dsv/Spl" [ label="dtv" ];
  "Transition to\nambientPass/dsv/Spl" -> "ambientPass" [ label="dtv" ];
  "Transition to\nfinalCombinePass/vv/Spl" [ shape=box ];
  "finalCombinePass" [ shape=ellipse ];
  "geometryPass" -> "Transition to\nfinalCombinePass/vv/Spl" [ label="vv" ];
  "Transition to\nfinalCombinePass/vv/Spl" -> "finalCombinePass" [ label="vv" ];
}
)";
					}
				}
				else if constexpr ( EnableTransparent )
				{
					ref = R"(digraph {
  "Transition to\nlightingPass/d1v/Spl" [ shape=box ];
  "geometryPass" [ shape=ellipse ];
  "lightingPass" [ shape=ellipse ];
  "geometryPass" -> "Transition to\nlightingPass/d1v/Spl" [ label="d1v" ];
  "Transition to\nlightingPass/d1v/Spl" -> "lightingPass" [ label="d1v" ];
  "Transition to\nlightingPass/d2v/Spl" [ shape=box ];
  "geometryPass" -> "Transition to\nlightingPass/d2v/Spl" [ label="d2v" ];
  "Transition to\nlightingPass/d2v/Spl" -> "lightingPass" [ label="d2v" ];
  "Transition to\nlightingPass/d3v/Spl" [ shape=box ];
  "geometryPass" -> "Transition to\nlightingPass/d3v/Spl" [ label="d3v" ];
  "Transition to\nlightingPass/d3v/Spl" -> "lightingPass" [ label="d3v" ];
  "Transition to\nlightingPass/d4v/Spl" [ shape=box ];
  "geometryPass" -> "Transition to\nlightingPass/d4v/Spl" [ label="d4v" ];
  "Transition to\nlightingPass/d4v/Spl" -> "lightingPass" [ label="d4v" ];
  "Transition to\nlightingPass/dtv/Spl" [ shape=box ];
  "geometryPass" -> "Transition to\nlightingPass/dtv/Spl" [ label="dtv" ];
  "Transition to\nlightingPass/dtv/Spl" -> "lightingPass" [ label="dtv" ];
  "Transition to\nambientPass/dfv/Spl" [ shape=box ];
  "ambientPass" [ shape=ellipse ];
  "lightingPass" -> "Transition to\nambientPass/dfv/Spl" [ label="dfv" ];
  "Transition to\nambientPass/dfv/Spl" -> "ambientPass" [ label="dfv" ];
  "Transition to\nambientPass/spv/Spl" [ shape=box ];
  "lightingPass" -> "Transition to\nambientPass/spv/Spl" [ label="spv" ];
  "Transition to\nambientPass/spv/Spl" -> "ambientPass" [ label="spv" ];
  "Transition to\nfinalCombinePass/ofv/Spl" [ shape=box ];
  "finalCombinePass" [ shape=ellipse ];
  "ambientPass" -> "Transition to\nfinalCombinePass/ofv/Spl" [ label="ofv" ];
  "Transition to\nfinalCombinePass/ofv/Spl" -> "finalCombinePass" [ label="ofv" ];
  "Transition to\naccumulationPass/vv/IOc" [ shape=box ];
  "accumulationPass" [ shape=ellipse ];
  "geometryPass" -> "Transition to\naccumulationPass/vv/IOc" [ label="vv" ];
  "Transition to\naccumulationPass/vv/IOc" -> "accumulationPass" [ label="vv" ];
  "Transition to\ncombinePass/av/Spl" [ shape=box ];
  "combinePass" [ shape=ellipse ];
  "accumulationPass" -> "Transition to\ncombinePass/av/Spl" [ label="av" ];
  "Transition to\ncombinePass/av/Spl" -> "combinePass" [ label="av" ];
  "Transition to\ncombinePass/dsv/Spl" [ shape=box ];
  "accumulationPass" -> "Transition to\ncombinePass/dsv/Spl" [ label="dtv" ];
  "Transition to\ncombinePass/dsv/Spl" -> "combinePass" [ label="dtv" ];
  "Transition to\ncombinePass/rv/Spl" [ shape=box ];
  "accumulationPass" -> "Transition to\ncombinePass/rv/Spl" [ label="rv" ];
  "Transition to\ncombinePass/rv/Spl" -> "combinePass" [ label="rv" ];
  "Transition to\nfinalCombinePass/cv/Spl" [ shape=box ];
  "combinePass" -> "Transition to\nfinalCombinePass/cv/Spl" [ label="cv" ];
  "Transition to\nfinalCombinePass/cv/Spl" -> "finalCombinePass" [ label="cv" ];
  "Transition to\nfinalCombinePass/vv/Spl" [ shape=box ];
  "accumulationPass" -> "Transition to\nfinalCombinePass/vv/Spl" [ label="vv" ];
  "Transition to\nfinalCombinePass/vv/Spl" -> "finalCombinePass" [ label="vv" ];
}
)";
				}
				else
				{
					ref = R"(digraph {
  "Transition to\nlightingPass/d1v/Spl" [ shape=box ];
  "geometryPass" [ shape=ellipse ];
  "lightingPass" [ shape=ellipse ];
  "geometryPass" -> "Transition to\nlightingPass/d1v/Spl" [ label="d1v" ];
  "Transition to\nlightingPass/d1v/Spl" -> "lightingPass" [ label="d1v" ];
  "Transition to\nlightingPass/d2v/Spl" [ shape=box ];
  "geometryPass" -> "Transition to\nlightingPass/d2v/Spl" [ label="d2v" ];
  "Transition to\nlightingPass/d2v/Spl" -> "lightingPass" [ label="d2v" ];
  "Transition to\nlightingPass/d3v/Spl" [ shape=box ];
  "geometryPass" -> "Transition to\nlightingPass/d3v/Spl" [ label="d3v" ];
  "Transition to\nlightingPass/d3v/Spl" -> "lightingPass" [ label="d3v" ];
  "Transition to\nlightingPass/d4v/Spl" [ shape=box ];
  "geometryPass" [ shape=ellipse ];
  "geometryPass" -> "Transition to\nlightingPass/d4v/Spl" [ label="d4v" ];
  "Transition to\nlightingPass/d4v/Spl" -> "lightingPass" [ label="d4v" ];
  "Transition to\nlightingPass/dtv/Spl" [ shape=box ];
  "geometryPass" [ shape=ellipse ];
  "geometryPass" -> "Transition to\nlightingPass/dtv/Spl" [ label="dtv" ];
  "Transition to\nlightingPass/dtv/Spl" -> "lightingPass" [ label="dtv" ];
  "Transition to\nambientPass/dfv/Spl" [ shape=box ];
  "ambientPass" [ shape=ellipse ];
  "lightingPass" -> "Transition to\nambientPass/dfv/Spl" [ label="dfv" ];
  "Transition to\nambientPass/dfv/Spl" -> "ambientPass" [ label="dfv" ];
  "Transition to\nambientPass/spv/Spl" [ shape=box ];
  "lightingPass" -> "Transition to\nambientPass/spv/Spl" [ label="spv" ];
  "Transition to\nambientPass/spv/Spl" -> "ambientPass" [ label="spv" ];
  "Transition to\nfinalCombinePass/ofv/Spl" [ shape=box ];
  "finalCombinePass" [ shape=ellipse ];
  "ambientPass" -> "Transition to\nfinalCombinePass/ofv/Spl" [ label="ofv" ];
  "Transition to\nfinalCombinePass/ofv/Spl" -> "finalCombinePass" [ label="ofv" ];
  "Transition to\nfinalCombinePass/vv/Spl" [ shape=box ];
  "geometryPass" [ shape=ellipse ];
  "geometryPass" -> "Transition to\nfinalCombinePass/vv/Spl" [ label="vv" ];
  "Transition to\nfinalCombinePass/vv/Spl" -> "finalCombinePass" [ label="vv" ];
}
)";
				}
			}
			else if constexpr ( EnableTransparent )
			{
				ref = R"(digraph {
  "Transition to\ncombinePass/av/Spl" [ shape=box ];
  "accumulationPass" [ shape=ellipse ];
  "combinePass" [ shape=ellipse ];
  "accumulationPass" -> "Transition to\ncombinePass/av/Spl" [ label="av" ];
  "Transition to\ncombinePass/av/Spl" -> "combinePass" [ label="av" ];
  "Transition to\ncombinePass/dsv/Spl" [ shape=box ];
  "accumulationPass" -> "Transition to\ncombinePass/dsv/Spl" [ label="dtv" ];
  "Transition to\ncombinePass/dsv/Spl" -> "combinePass" [ label="dtv" ];
  "Transition to\ncombinePass/rv/Spl" [ shape=box ];
  "accumulationPass" -> "Transition to\ncombinePass/rv/Spl" [ label="rv" ];
  "Transition to\ncombinePass/rv/Spl" -> "combinePass" [ label="rv" ];
  "Transition to\nfinalCombinePass/cv/Spl" [ shape=box ];
  "finalCombinePass" [ shape=ellipse ];
  "combinePass" -> "Transition to\nfinalCombinePass/cv/Spl" [ label="cv" ];
  "Transition to\nfinalCombinePass/cv/Spl" -> "finalCombinePass" [ label="cv" ];
  "Transition to\nfinalCombinePass/vv/Spl" [ shape=box ];
  "accumulationPass" -> "Transition to\nfinalCombinePass/vv/Spl" [ label="vv" ];
  "Transition to\nfinalCombinePass/vv/Spl" -> "finalCombinePass" [ label="vv" ];
}
)";
			}
			else
			{
				ref = R"(digraph {
}
)";
			}
		}

		checkEqualSortedLines( stream.str(), ref )
		testEnd()
	}

	void testVarianceShadowMap( test::TestCounts & testCounts )
	{
		testBegin( "testVarianceShadowMap" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };

		auto & dirGroup = graph.createPassGroup( "Directional" );
		auto dirShadowMap = dirGroup.createImage( test::createImage( "dirShadowMap", crg::PixelFormat::eX8_D24_UNORM, 1u, 4u ) );
		auto dirVarianceMap = dirGroup.createImage( test::createImage( "dirVarianceMap", crg::PixelFormat::eR32G32_SFLOAT, 1u, 4u ) );
		auto dirShadowMapv = dirGroup.createView( test::createView( "dirShadowMapv", dirShadowMap, crg::PixelFormat::eX8_D24_UNORM, 0u, 1u, 0u, 4u ) );
		auto dirVarianceMapv = dirGroup.createView( test::createView( "dirVarianceMapv", dirVarianceMap, crg::PixelFormat::eR32G32_SFLOAT, 0u, 1u, 0u, 4u ) );
		crg::Buffer buffer{ VkBuffer( uintptr_t( 1 ) ), "buffer" };
		crg::ImageViewIdArray dirShadows;
		crg::ImageViewIdArray dirVariances;
		{
			auto dirIntermediate = dirGroup.createImage( test::createImage( "dirIntermediate", crg::PixelFormat::eR32G32_SFLOAT ) );
			auto dirIntermediatev = dirGroup.createView( test::createView( "dirIntermediatev", dirIntermediate, crg::PixelFormat::eR32G32_SFLOAT ) );

			for ( uint32_t index = 0u; index < 4u; ++index )
			{
				auto shadowMapv = dirGroup.createView( test::createView( "dirShadowMapv" + std::to_string( index ), dirShadowMap, crg::PixelFormat::eX8_D24_UNORM, 0u, 1u, index ) );
				dirShadows.push_back( shadowMapv );
				auto varianceMapv = dirGroup.createView( test::createView( "dirVarianceMapv" + std::to_string( index ), dirVarianceMap, crg::PixelFormat::eR32G32_SFLOAT, 0u, 1u, index ) );
				dirVariances.push_back( varianceMapv );
				auto & shadowPass = dirGroup.createPass( "dirShadowPass" + std::to_string( index )
					, [&testCounts]( crg::FramePass const & framePass
						, crg::GraphContext & context
						, crg::RunnableGraph & runGraph )
					{
						return createDummy( testCounts
							, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
					} );
				auto previous = &shadowPass;
				shadowPass.addClearableOutputStorageBuffer( buffer, 0u, 0u, VK_WHOLE_SIZE );
				shadowPass.addOutputDepthView( shadowMapv );
				shadowPass.addOutputColourView( varianceMapv );

				auto & blurPassX = dirGroup.createPass( "dirBlurPassX" + std::to_string( index )
					, [&testCounts]( crg::FramePass const & framePass
						, crg::GraphContext & context
						, crg::RunnableGraph & runGraph )
					{
						return createDummy( testCounts
							, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader
							, checkSampledIsShaderReadOnly );
					} );
				blurPassX.addDependency( *previous );
				previous = &blurPassX;
				blurPassX.addInputStorageBuffer( buffer, 0u, 0u, VK_WHOLE_SIZE );
				blurPassX.addSampledView( varianceMapv, 1u );
				blurPassX.addOutputColourView( dirIntermediatev );

				auto & blurPassY = dirGroup.createPass( "dirBlurPassY" + std::to_string( index )
					, [&testCounts]( crg::FramePass const & framePass
						, crg::GraphContext & context
						, crg::RunnableGraph & runGraph )
					{
						return createDummy( testCounts
							, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader
							, checkSampledIsShaderReadOnly );
					} );
				blurPassY.addDependency( *previous );
				previous = &blurPassY;
				blurPassY.addInputStorageBuffer( buffer, 0u, 0u, VK_WHOLE_SIZE );
				blurPassY.addSampledView( dirIntermediatev, 1u );
				blurPassY.addOutputColourView( varianceMapv );
				dirGroup.addGroupOutput( varianceMapv );
			}
		}
		auto & pntGroup = graph.createPassGroup( "Point" );
		auto pntShadowMap = pntGroup.createImage( test::createImage( "pntShadowMap", crg::PixelFormat::eX8_D24_UNORM, 1u, 36u ) );
		auto pntVarianceMap = pntGroup.createImage( test::createImage( "pntVarianceMap", crg::PixelFormat::eR32G32_SFLOAT, 1u, 36u ) );
		auto pntShadowMapv = pntGroup.createView( test::createView( "pntShadowMapv", pntShadowMap, crg::PixelFormat::eX8_D24_UNORM, 0u, 1u, 0u, 36u ) );
		auto pntVarianceMapv = pntGroup.createView( test::createView( "pntVarianceMapv", pntVarianceMap, crg::PixelFormat::eR32G32_SFLOAT, 0u, 1u, 0u, 36u ) );
		crg::ImageViewIdArray pntShadows;
		crg::ImageViewIdArray pntVariances;
		{
			auto pntIntermediate = pntGroup.createImage( test::createImage( "pntIntermediate", crg::PixelFormat::eR32G32_SFLOAT ) );
			auto pntIntermediatev = pntGroup.createView( test::createView( "pntIntermediatev", pntIntermediate, crg::PixelFormat::eR32G32_SFLOAT ) );

			for ( uint32_t index = 0u; index < 36u; ++index )
			{
				auto shadowMapv = pntGroup.createView( test::createView( "pntShadowMapv" + std::to_string( index ), pntShadowMap, crg::PixelFormat::eX8_D24_UNORM, 0u, 1u, index ) );
				pntShadows.push_back( shadowMapv );
				auto varianceMapv = pntGroup.createView( test::createView( "pntVarianceMapv" + std::to_string( index ), pntVarianceMap, crg::PixelFormat::eR32G32_SFLOAT, 0u, 1u, index ) );
				pntVariances.push_back( varianceMapv );
				auto & shadowPass = pntGroup.createPass( "pntShadowPass" + std::to_string( index )
					, [&testCounts]( crg::FramePass const & framePass
						, crg::GraphContext & context
						, crg::RunnableGraph & runGraph )
					{
						return createDummy( testCounts
							, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
					} );
				auto previous = &shadowPass;
				shadowPass.addClearableOutputStorageBuffer( buffer, 0u, 0u, VK_WHOLE_SIZE );
				shadowPass.addOutputDepthView( shadowMapv );
				shadowPass.addOutputColourView( varianceMapv );

				auto & blurPassX = pntGroup.createPass( "pntBlurPassX" + std::to_string( index )
					, [&testCounts]( crg::FramePass const & framePass
						, crg::GraphContext & context
						, crg::RunnableGraph & runGraph )
					{
						return createDummy( testCounts
							, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader
							, checkSampledIsShaderReadOnly );
					} );
				blurPassX.addDependency( *previous );
				previous = &blurPassX;
				blurPassX.addInputStorageBuffer( buffer, 0u, 0u, VK_WHOLE_SIZE );
				blurPassX.addSampledView( varianceMapv, 1u );
				blurPassX.addOutputColourView( pntIntermediatev );

				auto & blurPassY = pntGroup.createPass( "pntBlurPassY" + std::to_string( index )
					, [&testCounts]( crg::FramePass const & framePass
						, crg::GraphContext & context
						, crg::RunnableGraph & runGraph )
					{
						return createDummy( testCounts
							, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader
							, checkSampledIsShaderReadOnly );
					} );
				blurPassY.addDependency( *previous );
				previous = &blurPassY;
				blurPassY.addInputStorageBuffer( buffer, 0u, 0u, VK_WHOLE_SIZE );
				blurPassY.addSampledView( pntIntermediatev, 1u );
				blurPassY.addOutputColourView( varianceMapv );
				pntGroup.addGroupOutput( varianceMapv );
			}
		}
		auto & sptGroup = graph.createPassGroup( "Spot" );
		auto sptShadowMap = sptGroup.createImage( test::createImage( "sptShadowMap", crg::PixelFormat::eX8_D24_UNORM, 1u, 10u ) );
		auto sptVarianceMap = sptGroup.createImage( test::createImage( "pntVarianceMap", crg::PixelFormat::eR32G32_SFLOAT, 1u, 10u ) );
		auto sptShadowMapv = sptGroup.createView( test::createView( "sptShadowMapv", sptShadowMap, crg::PixelFormat::eX8_D24_UNORM, 0u, 1u, 0u, 10u ) );
		auto sptVarianceMapv = sptGroup.createView( test::createView( "sptVarianceMapv", sptVarianceMap, crg::PixelFormat::eR32G32_SFLOAT, 0u, 1u, 0u, 10u ) );
		crg::ImageViewIdArray sptShadows;
		crg::ImageViewIdArray sptVariances;
		{
			auto sptIntermediate = sptGroup.createImage( test::createImage( "sptIntermediate", crg::PixelFormat::eR32G32_SFLOAT ) );
			auto sptIntermediatev = sptGroup.createView( test::createView( "sptIntermediatev", sptIntermediate, crg::PixelFormat::eR32G32_SFLOAT ) );

			for ( uint32_t index = 0u; index < 10u; ++index )
			{
				auto shadowMapv = sptGroup.createView( test::createView( "sptShadowMapv" + std::to_string( index ), sptShadowMap, crg::PixelFormat::eX8_D24_UNORM, 0u, 1u, index ) );
				sptShadows.push_back( shadowMapv );
				auto varianceMapv = sptGroup.createView( test::createView( "sptVarianceMapv" + std::to_string( index ), sptVarianceMap, crg::PixelFormat::eR32G32_SFLOAT, 0u, 1u, index ) );
				sptVariances.push_back( varianceMapv );
				auto & shadowPass = sptGroup.createPass( "sptShadowPass" + std::to_string( index )
					, [&testCounts]( crg::FramePass const & framePass
						, crg::GraphContext & context
						, crg::RunnableGraph & runGraph )
					{
						return createDummy( testCounts
							, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader
							, checkOutputColourIsShaderReadOnly );
					} );
				auto previous = &shadowPass;
				shadowPass.addClearableOutputStorageBuffer( buffer, 0u, 0u, VK_WHOLE_SIZE );
				shadowPass.addOutputDepthView( shadowMapv );
				shadowPass.addOutputColourView( varianceMapv );

				auto & blurPassX = sptGroup.createPass( "sptBlurPassX" + std::to_string( index )
					, [&testCounts]( crg::FramePass const & framePass
						, crg::GraphContext & context
						, crg::RunnableGraph & runGraph )
					{
						return createDummy( testCounts
							, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader
							, checkSampledIsShaderReadOnly );
					} );
				blurPassX.addDependency( *previous );
				previous = &blurPassX;
				blurPassX.addInputStorageBuffer( buffer, 0u, 0u, VK_WHOLE_SIZE );
				blurPassX.addSampledView( varianceMapv, 1u );
				blurPassX.addOutputColourView( sptIntermediatev );

				auto & blurPassY = sptGroup.createPass( "sptBlurPassY" + std::to_string( index )
					, [&testCounts]( crg::FramePass const & framePass
						, crg::GraphContext & context
						, crg::RunnableGraph & runGraph )
					{
						return createDummy( testCounts
							, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader
							, checkSampledIsShaderReadOnly );
					} );
				blurPassY.addDependency( *previous );
				previous = &blurPassY;
				blurPassY.addInputStorageBuffer( buffer, 0u, 0u, VK_WHOLE_SIZE );
				blurPassY.addSampledView( sptIntermediatev, 1u );
				blurPassY.addOutputColourView( varianceMapv );
				sptGroup.addGroupOutput( varianceMapv );
			}
		}

		auto & objGroup = graph.createPassGroup( "Objects" );
		auto depth = graph.createImage( test::createImage( "depth", crg::PixelFormat::eD32_SFLOAT_S8_UINT ) );
		auto depthv = graph.createView( test::createView( "depthv", depth, crg::PixelFormat::eD32_SFLOAT_S8_UINT ) );
		auto & depthPrepass = graph.createPass( "depthPrepass"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		depthPrepass.addOutputDepthView( depthv );
		auto previous = &depthPrepass;

		auto colour = graph.createImage( test::createImage( "colour", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto colourv = graph.createView( test::createView( "colourv", colour, crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto & backgroundPass = graph.createPass( "backgroundPass"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		backgroundPass.addDependency( *previous );
		previous = &backgroundPass;
		backgroundPass.addInOutDepthView( depthv );
		backgroundPass.addOutputColourView( colourv );
		objGroup.addOutput( colourv
			, crg::makeLayoutState( crg::ImageLayout::eShaderReadOnly ) );

		auto & opaquePass = objGroup.createPass( "opaquePass"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader
					, checkSampledIsShaderReadOnly );
			} );
		opaquePass.addDependency( *previous );
		previous = &opaquePass;
		opaquePass.addSampledView( dirShadowMapv, 0u );
		opaquePass.addSampledView( dirVarianceMapv, 1u );
		opaquePass.addSampledView( opaquePass.mergeViews( dirShadows ), 0u );
		opaquePass.addSampledView( opaquePass.mergeViews( dirVariances ), 1u );
		opaquePass.addSampledView( pntShadowMapv, 2u );
		opaquePass.addSampledView( pntVarianceMapv, 3u );
		opaquePass.addSampledView( opaquePass.mergeViews( pntShadows ), 2u );
		opaquePass.addSampledView( opaquePass.mergeViews( pntVariances ), 3u );
		opaquePass.addSampledView( sptShadowMapv, 4u );
		opaquePass.addSampledView( sptVarianceMapv, 5u );
		opaquePass.addSampledView( opaquePass.mergeViews( sptShadows ), 4u );
		opaquePass.addSampledView( opaquePass.mergeViews( sptVariances ), 5u );
		opaquePass.addInOutDepthView( depthv );
		opaquePass.addInOutColourView( colourv );

		auto & transparentPass = objGroup.createPass( "transparentPass"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		transparentPass.addDependency( *previous );
		previous = &transparentPass;
		transparentPass.addInOutDepthView( depthv );
		transparentPass.addInOutColourView( colourv );

		auto runnable = graph.compile( getContext() );
		test::checkRunnable( testCounts, runnable );
		testEnd()
	}

	void testEnvironmentMap( test::TestCounts & testCounts )
	{
		testBegin( "testEnvironmentMap" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto depth = graph.createImage( test::createImage( "depth", crg::PixelFormat::eD32_SFLOAT_S8_UINT, 1u, 6u ) );
		auto cube = graph.createImage( test::createImageCube( "cube", crg::PixelFormat::eR16G16B16A16_SFLOAT, 8u, 1u ) );
		auto cubev = graph.createView( test::createView( "cubev", cube, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0, 8u, 0u, 6u ) );
		auto cubes = graph.createImage( test::createImageCube( "cubes", crg::PixelFormat::eR16G16B16A16_SFLOAT, 8u, 6u ) );
		auto cubesv = graph.createView( test::createView( "cubesv", cube, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0, 8u, 0u, 36u ) );
		auto colour = graph.createImage( test::createImage1D( "colour", crg::PixelFormat::eR16G16B16A16_SFLOAT, 8u, 6u ) );
		auto colourv = graph.createView( test::createView( "colourv", colour, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 8u, 0u, 6u ) );
		crg::ImageViewIdArray colourViews;
		crg::ImageViewIdArray cubeViews;
		crg::ImageViewIdArray cubesViews;
		crg::ImageViewIdArray cubesLayersViews;
		std::vector< crg::FramePass const * > previouses{};
		crg::FramePass const * previous{};

		for ( auto index = 0u; index < 6u; ++index )
		{
			auto strIndex = std::to_string( index );
			auto colourvn = graph.createView( test::createView( "colourv" + strIndex, colour, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, index, 1u ) );
			auto cubevn = graph.createView( test::createView( "cubev" + strIndex, cube, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, index, 1u ) );
			auto cubesvn = graph.createView( test::createView( "cubesv" + strIndex, cubes, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, index * 6u, 6u ) );
			auto depthvn = graph.createView( test::createView( "depthv" + strIndex, depth, crg::PixelFormat::eD32_SFLOAT_S8_UINT, 0u, 1u, index, 1u ) );
			auto & opaquePass = graph.createPass( "EnvOpaquePass" + strIndex
				, [&testCounts]( crg::FramePass const & framePass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
					return createDummy( testCounts
						, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
				} );
			colourViews.push_back( colourvn );
			cubeViews.push_back( cubevn );
			cubesViews.push_back( cubesvn );
			cubesLayersViews.push_back( graph.createView( test::createView( "cubesv" + strIndex + std::to_string( 0u ), cubes, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, index * 6u + 0u, 1u ) ) );
			cubesLayersViews.push_back( graph.createView( test::createView( "cubesv" + strIndex + std::to_string( 1u ), cubes, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, index * 6u + 1u, 1u ) ) );
			cubesLayersViews.push_back( graph.createView( test::createView( "cubesv" + strIndex + std::to_string( 2u ), cubes, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, index * 6u + 2u, 1u ) ) );
			cubesLayersViews.push_back( graph.createView( test::createView( "cubesv" + strIndex + std::to_string( 3u ), cubes, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, index * 6u + 3u, 1u ) ) );
			cubesLayersViews.push_back( graph.createView( test::createView( "cubesv" + strIndex + std::to_string( 4u ), cubes, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, index * 6u + 4u, 1u ) ) );
			cubesLayersViews.push_back( graph.createView( test::createView( "cubesv" + strIndex + std::to_string( 5u ), cubes, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, index * 6u + 5u, 1u ) ) );
			previous = &opaquePass;
			opaquePass.addOutputDepthView( depthvn );
			opaquePass.addOutputColourView( colourvn );
			opaquePass.addOutputColourView( cubevn );
			opaquePass.addOutputColourView( cubesvn );

			auto & backgroundPass = graph.createPass( "EnvBackgroundPass" + strIndex
				, [&testCounts]( crg::FramePass const & framePass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
					return createDummy( testCounts
						, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
				} );
			backgroundPass.addDependency( *previous );
			previous = &backgroundPass;
			backgroundPass.addInOutDepthView( depthvn );
			backgroundPass.addInOutColourView( colourvn );
			backgroundPass.addInOutColourView( cubevn );
			backgroundPass.addInOutColourView( cubesvn );

			auto & transparentPass = graph.createPass( "EnvTransparentPass" + strIndex
				, [&testCounts]( crg::FramePass const & framePass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
					return createDummy( testCounts
						, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
				} );
			transparentPass.addDependency( *previous );
			previouses.push_back( &transparentPass );
			transparentPass.addInputDepthView( depthvn );
			transparentPass.addInOutColourView( colourvn );
			transparentPass.addInOutColourView( cubevn );
			transparentPass.addInOutColourView( cubesvn );
		}

		auto & mipsGen = graph.createPass( "EnvMips"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eTransfer );
			} );
		mipsGen.addDependencies( previouses );
		mipsGen.addTransferInputView( mipsGen.mergeViews( colourViews ) );
		mipsGen.addTransferInputView( mipsGen.mergeViews( cubeViews ) );
		mipsGen.addTransferInputView( mipsGen.mergeViews( cubesViews ) );
		mipsGen.addTransferInputView( mipsGen.mergeViews( cubesLayersViews ) );
		mipsGen.addTransferOutputView( colourv );
		mipsGen.addTransferOutputView( cubev );
		mipsGen.addTransferOutputView( cubesv );

		auto runnable = graph.compile( getContext() );
		test::checkRunnable( testCounts, runnable );
		testEnd()
	}

	void testDisabledPasses( test::TestCounts & testCounts )
	{
		testBegin( "testDisabledPasses" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto depth = graph.createImage( test::createImage( "depth", crg::PixelFormat::eD32_SFLOAT_S8_UINT, 1u, 6u ) );
		auto cube = graph.createImage( test::createImageCube( "cube", crg::PixelFormat::eR16G16B16A16_SFLOAT, 8u, 1u ) );
		auto cubev = graph.createView( test::createView( "cubev", cube, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0, 8u, 0u, 6u ) );
		auto cubes = graph.createImage( test::createImageCube( "cubes", crg::PixelFormat::eR16G16B16A16_SFLOAT, 8u, 6u ) );
		auto cubesv = graph.createView( test::createView( "cubesv", cube, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0, 8u, 0u, 36u ) );
		auto colour = graph.createImage( test::createImage1D( "colour", crg::PixelFormat::eR16G16B16A16_SFLOAT, 8u, 6u ) );
		auto colourv = graph.createView( test::createView( "colourv", colour, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 8u, 0u, 6u ) );
		crg::ImageViewIdArray colourViews;
		crg::ImageViewIdArray cubeViews;
		crg::ImageViewIdArray cubesViews;
		crg::ImageViewIdArray cubesLayersViews;
		std::vector< crg::FramePass const * > previouses{};
		crg::FramePass const * previous{};

		for ( auto index = 0u; index < 6u; ++index )
		{
			auto strIndex = std::to_string( index );
			auto colourvn = graph.createView( test::createView( "colourv" + strIndex, colour, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, index, 1u ) );
			auto cubevn = graph.createView( test::createView( "cubev" + strIndex, cube, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, index, 1u ) );
			auto cubesvn = graph.createView( test::createView( "cubesv" + strIndex, cubes, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, index * 6u, 6u ) );
			auto depthvn = graph.createView( test::createView( "depthv" + strIndex, depth, crg::PixelFormat::eD32_SFLOAT_S8_UINT, 0u, 1u, index, 1u ) );
			auto & opaquePass = graph.createPass( "EnvOpaquePass" + strIndex
				, [&testCounts]( crg::FramePass const & framePass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
					return createDummy( testCounts
						, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
				} );
			colourViews.push_back( colourvn );
			cubeViews.push_back( cubevn );
			cubesViews.push_back( cubesvn );
			cubesLayersViews.push_back( graph.createView( test::createView( "cubesv" + strIndex + std::to_string( 0u ), cubes, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, index * 6u + 0u, 1u ) ) );
			cubesLayersViews.push_back( graph.createView( test::createView( "cubesv" + strIndex + std::to_string( 1u ), cubes, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, index * 6u + 1u, 1u ) ) );
			cubesLayersViews.push_back( graph.createView( test::createView( "cubesv" + strIndex + std::to_string( 2u ), cubes, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, index * 6u + 2u, 1u ) ) );
			cubesLayersViews.push_back( graph.createView( test::createView( "cubesv" + strIndex + std::to_string( 3u ), cubes, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, index * 6u + 3u, 1u ) ) );
			cubesLayersViews.push_back( graph.createView( test::createView( "cubesv" + strIndex + std::to_string( 4u ), cubes, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, index * 6u + 4u, 1u ) ) );
			cubesLayersViews.push_back( graph.createView( test::createView( "cubesv" + strIndex + std::to_string( 5u ), cubes, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, index * 6u + 5u, 1u ) ) );
			previous = &opaquePass;
			opaquePass.addOutputDepthView( depthvn );
			opaquePass.addOutputColourView( colourvn );
			opaquePass.addOutputColourView( cubevn );
			opaquePass.addOutputColourView( cubesvn );

			auto & backgroundPass = graph.createPass( "EnvBackgroundPass" + strIndex
				, [&testCounts]( crg::FramePass const & framePass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
					return createDummy( testCounts
						, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader
						, test::checkDummy, 0u, false );
				} );
			backgroundPass.addDependency( *previous );
			previous = &backgroundPass;
			backgroundPass.addInOutDepthView( depthvn );
			backgroundPass.addInOutColourView( colourvn );
			backgroundPass.addInOutColourView( cubevn );
			backgroundPass.addInOutColourView( cubesvn );

			auto & transparentPass = graph.createPass( "EnvTransparentPass" + strIndex
				, [&testCounts]( crg::FramePass const & framePass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
					return createDummy( testCounts
						, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader
						, test::checkDummy, 0u, false );
				} );
			transparentPass.addDependency( *previous );
			previouses.push_back( &transparentPass );
			transparentPass.addInputDepthView( depthvn );
			transparentPass.addInOutColourView( colourvn );
			transparentPass.addInOutColourView( cubevn );
			transparentPass.addInOutColourView( cubesvn );
		}

		auto & mipsGen = graph.createPass( "EnvMips"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eTransfer );
			} );
		mipsGen.addDependencies( previouses );
		mipsGen.addTransferInputView( mipsGen.mergeViews( colourViews ) );
		mipsGen.addTransferInputView( mipsGen.mergeViews( cubeViews ) );
		mipsGen.addTransferInputView( mipsGen.mergeViews( cubesViews ) );
		mipsGen.addTransferInputView( mipsGen.mergeViews( cubesLayersViews ) );
		mipsGen.addTransferOutputView( colourv );
		mipsGen.addTransferOutputView( cubev );
		mipsGen.addTransferOutputView( cubesv );

		auto runnable = graph.compile( getContext() );
		test::checkRunnable( testCounts, runnable );
		testEnd()
	}
}

int main( int argc, char ** argv )
{
	testSuiteBegin( "TestRenderGraph" )
	testNoPass( testCounts );
	testOnePass( testCounts );
	testDuplicateName( testCounts );
	testOneDependency( testCounts );
	testCycleDependency( testCounts );
	testUnsortableCycleDependency( testCounts );
	testChainedDependencies( testCounts );
	testSharedDependencies( testCounts );
	test2MipDependencies( testCounts );
	test3MipDependencies( testCounts );
	testLoopDependencies( testCounts );
	testLoopDependenciesWithRoot( testCounts );
	testLoopDependenciesWithRootAndLeaf( testCounts );
	testSsaoPass( testCounts );
	testBloomPostEffect( testCounts );
	testRender< false, false, false, false >( testCounts );
	testRender< false, true, false, false >( testCounts );
	testRender< false, true, true, false >( testCounts );
	testRender< false, false, false, true >( testCounts );
	testRender< false, true, false, true >( testCounts );
	testRender< false, true, true, true >( testCounts );
	testRender< true, false, false, false >( testCounts );
	testRender< true, true, false, false >( testCounts );
	testRender< true, true, true, false >( testCounts );
	testRender< true, false, false, true >( testCounts );
	testRender< true, true, false, true >( testCounts );
	testRender< true, true, true, true >( testCounts );
	testVarianceShadowMap( testCounts );
	testEnvironmentMap( testCounts );
	testDisabledPasses( testCounts );
	testSuiteEnd()
}
