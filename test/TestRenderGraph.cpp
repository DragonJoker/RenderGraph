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

	void checkTargetColourIsShaderReadOnly( [[maybe_unused]] test::TestCounts const & testCounts
		, crg::FramePass const & framePass
		, crg::RunnableGraph const &
		, crg::RecordContext const & context
		, uint32_t index )
	{
		for ( auto attach : framePass.getTargets() )
		{
			auto view = attach->view( index );

			if ( attach->isColourOutputImageTarget() )
			{
				auto resolved = crg::resolveView( view, index );
				checkEqual( context.getNextLayoutState( resolved ).layout, crg::ImageLayout::eShaderReadOnly )
				checkEqual( context.getNextLayoutState( resolved.data->image
					, resolved.data->info.viewType
					, getSubresourceRange( resolved ) ).layout, crg::ImageLayout::eShaderReadOnly )
			}
		}
	}

	void checkSampledIsShaderReadOnly( [[maybe_unused]] test::TestCounts const & testCounts
		, crg::FramePass const & framePass
		, crg::RunnableGraph const &
		, crg::RecordContext const & context
		, uint32_t index )
	{
		for ( auto & [binding, attach] : framePass.getSampled() )
		{
			auto view = attach.attach->view( index );
			checkEqual( context.getLayoutState( crg::resolveView( view, index ) ).layout, crg::ImageLayout::eShaderReadOnly )
		}
	}

	crg::FrameGraph buildNoPassGraph( test::TestCounts const & testCounts
		, crg::ResourceHandler & handler )
	{
		crg::FrameGraph graph{ handler, testCounts.testName };
		checkThrow( graph.compile( getContext() ), crg::Exception )
		return graph;
	}
}

TEST( RenderGraph, NoPass )
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
	pass.addOutputColourTarget( rtv );
	testEnd()
}

TEST( RenderGraph, OnePass )
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
	pass.addOutputColourTarget(  rtv );
	auto runnable = graph.compile( getContext() );
	auto stream = test::checkRunnable( testCounts, runnable );
	std::string ref = R"(digraph {
}
)";
	checkEqualSortedLines( stream, ref )
	testEnd()
}

TEST( RenderGraph, DuplicateName )
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
		} ), crg::Exception )
	testEnd()
}

TEST( RenderGraph, OneDependency )
{
	testBegin( "testOneDependency" )
	crg::ResourceHandler handler;
	crg::FrameGraph graph{ handler, testCounts.testName };
	auto rt = graph.createImage( test::createImage( "rt", crg::PixelFormat::eR32G32B32A32_SFLOAT, 1u, 4u ) );
	auto dep = graph.createImage( test::createImage( "dep", crg::PixelFormat::eD32_SFLOAT_S8_UINT ) );
	auto rtv0 = graph.createView( test::createView( "rtv0", rt, 0u, 1u, 0u, 1u ) );
	auto rtv1 = graph.createView( test::createView( "rtv1", rt, 0u, 1u, 1u, 1u ) );
	auto rtv2 = graph.createView( test::createView( "rtv2", rt, 0u, 1u, 2u, 1u ) );
	auto rtv3 = graph.createView( test::createView( "rtv3", rt, 0u, 1u, 3u, 1u ) );
	auto depv = graph.createView( test::createView( "depv", dep ) );
	auto buf = graph.createBuffer( test::createBuffer( "buf" ) );
	auto bufv = graph.createView( test::createView( "bufv", buf ) );
	auto & pass1 = graph.createPass( "pass1C"
		, [&testCounts]( crg::FramePass const & framePass
			, crg::GraphContext & context
			, crg::RunnableGraph & runGraph )
		{
			return createDummy( testCounts
				, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
		} );
	pass1.addInputUniformBuffer( bufv, 1u );
	auto rta = pass1.addOutputColourTarget( graph.mergeViews( { rtv0, rtv1, rtv2, rtv3 } ) );
	auto depa = pass1.addOutputDepthStencilTarget( depv );

	auto out = graph.createImage( test::createImage( "out", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
	auto outv = graph.createView( test::createView( "outv", out ) );
	auto & pass2 = graph.createPass( "pass2C"
		, []( crg::FramePass const & framePass
			, crg::GraphContext & context
			, crg::RunnableGraph & runGraph )
		{
			return test::createDummyNoRecord( framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
		} );
	pass2.addInputSampled( *rta, 0u );
	pass2.addInputUniformBuffer( bufv, 1u );
	pass2.addOutputColourTarget( outv );
	pass2.addInputDepthStencilTarget( *depa );

	auto runnable = graph.compile( getContext() );
	auto stream = test::checkRunnable( testCounts, runnable );
	std::string ref = R"(digraph {
  "pass1C" [ shape=ellipse ];
  "pass2C" [ shape=ellipse ];
  "Transition to\npass2C/rt/Spl0" [ shape=box ];
  "pass1C" -> "Transition to\npass2C/rt/Spl0" [ label="rtv0" ];
  "Transition to\npass2C/rt/Spl0" -> "pass2C" [ label="rtv0" ];
  "Transition to\npass2C/rt/Spl1" [ shape=box ];
  "pass1C" -> "Transition to\npass2C/rt/Spl1" [ label="rtv1" ];
  "Transition to\npass2C/rt/Spl1" -> "pass2C" [ label="rtv1" ];
  "Transition to\npass2C/rt/Spl2" [ shape=box ];
  "pass1C" -> "Transition to\npass2C/rt/Spl2" [ label="rtv2" ];
  "Transition to\npass2C/rt/Spl2" -> "pass2C" [ label="rtv2" ];
  "Transition to\npass2C/rt/Spl3" [ shape=box ];
  "pass1C" -> "Transition to\npass2C/rt/Spl3" [ label="rtv3" ];
  "Transition to\npass2C/rt/Spl3" -> "pass2C" [ label="rtv3" ];
  "Transition to\npass2C/depv/IRds" [ shape=box ];
  "pass1C" -> "Transition to\npass2C/depv/IRds" [ label="depv" ];
  "Transition to\npass2C/depv/IRds" -> "pass2C" [ label="depv" ];
  "ExternalSource" [ shape=ellipse ];
  "Transition to\npass1C/bufv/UB" [ shape=box ];
  "ExternalSource" -> "Transition to\npass1C/bufv/UB" [ label="bufv" ];
  "Transition to\npass1C/bufv/UB" -> "pass1C" [ label="bufv" ];
}
)";
	checkEqualSortedLines( stream, ref )
	testEnd()
}

TEST( RenderGraph, CycleDependency )
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
	auto rta = pass1.addOutputColourTarget( rtv );

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
	pass2.addInputSampled( *rta, 0u );
	auto outa = pass2.addOutputColourTarget( outv );

	auto & pass3 = graph.createPass( "pass3C"
		, [&testCounts]( crg::FramePass const & framePass
			, crg::GraphContext & context
			, crg::RunnableGraph & runGraph )
		{
			return createDummy( testCounts
				, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
		} );
	pass3.addInputSampled( *rta, 0u );
	pass3.addInOutColourTarget( *outa );

	auto runnable = graph.compile( getContext() );
	auto stream = test::checkRunnable( testCounts, runnable );
	std::string ref = R"(digraph {
  "pass1C" [ shape=ellipse ];
  "pass2C" [ shape=ellipse ];
  "Transition to\npass2C/rtv/Spl" [ shape=box ];
  "pass1C" -> "Transition to\npass2C/rtv/Spl" [ label="rtv" ];
  "Transition to\npass2C/rtv/Spl" -> "pass2C" [ label="rtv" ];
  "pass3C" [ shape=ellipse ];
  "Transition to\npass3C/outv/IORcl" [ shape=box ];
  "pass2C" -> "Transition to\npass3C/outv/IORcl" [ label="outv" ];
  "Transition to\npass3C/outv/IORcl" -> "pass3C" [ label="outv" ];
}
)";
	checkEqualSortedLines( stream, ref )
	testEnd()
}

TEST( RenderGraph, ChainedDependencies )
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
	auto d0a = pass0.addOutputColourTarget( d0v );

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
	pass1.addInputSampled( *d0a, 0u );
	auto d1a = pass1.addOutputColourTarget( d1v );

	auto buf = graph.createBuffer( test::createBuffer( "buf" ) );
	auto bufv = graph.createView( test::createView( "bufv", buf ) );
	auto & pass2 = graph.createPass( "pass2"
		, [&testCounts]( crg::FramePass const & framePass
			, crg::GraphContext & context
			, crg::RunnableGraph & runGraph )
		{
			return createDummy( testCounts
				, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
		} );
	pass2.addInputSampled( *d1a, 0u );
	pass2.addInputUniformBuffer( bufv, 1u );
	pass2.addInputColourTarget( *d1a );

	auto runnable = graph.compile( getContext() );
	auto stream = test::checkRunnable( testCounts, runnable );
	std::string ref = R"(digraph {
  "pass1" [ shape=ellipse ];
  "pass2" [ shape=ellipse ];
  "Transition to\npass2/d1v/Spl" [ shape=box ];
  "pass1" -> "Transition to\npass2/d1v/Spl" [ label="d1v" ];
  "Transition to\npass2/d1v/Spl" -> "pass2" [ label="d1v" ];
  "Transition to\npass2/d1v/IRcl" [ shape=box ];
  "pass1" -> "Transition to\npass2/d1v/IRcl" [ label="d1v" ];
  "Transition to\npass2/d1v/IRcl" -> "pass2" [ label="d1v" ];
  "pass0" [ shape=ellipse ];
  "Transition to\npass1/d0v/Spl" [ shape=box ];
  "pass0" -> "Transition to\npass1/d0v/Spl" [ label="d0v" ];
  "Transition to\npass1/d0v/Spl" -> "pass1" [ label="d0v" ];
  "ExternalSource" [ shape=ellipse ];
  "Transition to\npass2/bufv/UB" [ shape=box ];
  "ExternalSource" -> "Transition to\npass2/bufv/UB" [ label="bufv" ];
  "Transition to\npass2/bufv/UB" -> "pass2" [ label="bufv" ];
}
)";
	checkEqualSortedLines( stream, ref )
	testEnd()
}

TEST( RenderGraph, SharedDependencies )
{
	testBegin( "testSharedDependencies" )
	crg::ResourceHandler handler;
	crg::FrameGraph graph{ handler, testCounts.testName };
	auto d = graph.createImage( test::createImage( "d", crg::PixelFormat::eD32_SFLOAT_S8_UINT ) );
	auto dstv1 = graph.createView( test::createView( "dstv1", d ) );
	auto buf = graph.createBuffer( test::createBuffer( "buf" ) );
	auto bufv1 = graph.createView( test::createView( "bufv1", buf ) );
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
	pass0.addOutputStorageBuffer( bufv1, 0 );
	pass0.addOutputDepthTarget( dstv1 );
	auto d0a = pass0.addOutputColourTarget( d0v );

	auto dstv2 = graph.createView( test::createView( "dstv2", d ) );
	auto bufv2 = graph.createView( test::createView( "bufv2", buf ) );
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
	pass1.addInputSampled( *d0a, 0 );
	pass0.addOutputStorageBuffer( bufv2, 0 );
	auto dsta = pass1.addOutputDepthTarget ( dstv2 );
	auto d1a = pass1.addOutputColourTarget( d1v );

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
	pass2.addInputSampled( *d1a, 0 );
	pass2.addInputDepthTarget( *dsta );
	pass2.addOutputColourTarget( d2v );

	auto runnable = graph.compile( getContext() );
	auto stream = test::checkRunnable( testCounts, runnable );
	std::string ref = R"(digraph {
  "pass2" [ shape=ellipse ];
  "pass1" [ shape=ellipse ];
  "Transition to\npass2/d1v/Spl" [ shape=box ];
  "pass1" -> "Transition to\npass2/d1v/Spl" [ label="d1v" ];
  "Transition to\npass2/d1v/Spl" -> "pass2" [ label="d1v" ];
  "Transition to\npass2/dstv1/IRdp" [ shape=box ];
  "pass1" -> "Transition to\npass2/dstv1/IRdp" [ label="dstv1" ];
  "Transition to\npass2/dstv1/IRdp" -> "pass2" [ label="dstv1" ];
  "pass0" [ shape=ellipse ];
  "Transition to\npass1/d0v/Spl" [ shape=box ];
  "pass0" -> "Transition to\npass1/d0v/Spl" [ label="d0v" ];
  "Transition to\npass1/d0v/Spl" -> "pass1" [ label="d0v" ];
}
)";
	checkEqualSortedLines( stream, ref )
	testEnd()
}

TEST( RenderGraph, 2MipDependencies )
{
	testBegin( "test2MipDependencies" )
	crg::ResourceHandler handler;
	crg::FrameGraph graph{ handler, testCounts.testName };
	auto lp = graph.createImage( test::createImage( "lp", crg::PixelFormat::eR32G32B32_SFLOAT, 3u ) );
	auto m0v = graph.createView( test::createView( "m0v", lp, 0u ) );
	auto m1v = graph.createView( test::createView( "m1v", lp, 1u ) );
	auto m0a = crg::Attachment::createDefault( m0v );
	graph.addInput( m0v, makeLayoutState( crg::ImageLayout::eShaderReadOnly ) );
	auto & ssaoMinifyPass1 = graph.createPass( "ssaoMinifyPass1"
		, [&testCounts]( crg::FramePass const & framePass
			, crg::GraphContext & context
			, crg::RunnableGraph & runGraph )
		{
			return createDummy( testCounts
				, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
		} );
	ssaoMinifyPass1.addInputSampled( m0a, 0 );
	auto m1a = ssaoMinifyPass1.addOutputColourTarget( m1v );

	auto m2v = graph.createView( test::createView( "m2v", lp, 2u ) );
	auto & ssaoMinifyPass2 = graph.createPass( "ssaoMinifyPass2"
		, [&testCounts]( crg::FramePass const & framePass
			, crg::GraphContext & context
			, crg::RunnableGraph & runGraph )
		{
			return createDummy( testCounts
				, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
		} );
	ssaoMinifyPass2.addInputSampled( *m1a, 0 );
	ssaoMinifyPass2.addOutputColourTarget( m2v );

	auto runnable = graph.compile( getContext() );
	auto stream = test::checkRunnable( testCounts, runnable );
	std::string ref = R"(digraph {
  "ssaoMinifyPass1" [ shape=ellipse ];
  "ssaoMinifyPass2" [ shape=ellipse ];
  "Transition to\nssaoMinifyPass2/m1v/Spl" [ shape=box ];
  "ssaoMinifyPass1" -> "Transition to\nssaoMinifyPass2/m1v/Spl" [ label="m1v" ];
  "Transition to\nssaoMinifyPass2/m1v/Spl" -> "ssaoMinifyPass2" [ label="m1v" ];
  "ExternalSource" [ shape=ellipse ];
  "Transition to\nssaoMinifyPass1/m0v/Spl" [ shape=box ];
  "ExternalSource" -> "Transition to\nssaoMinifyPass1/m0v/Spl" [ label="m0v" ];
  "Transition to\nssaoMinifyPass1/m0v/Spl" -> "ssaoMinifyPass1" [ label="m0v" ];
}
)";
	checkEqualSortedLines( stream, ref )
	testEnd()
}

TEST( RenderGraph, 3MipDependencies )
{
	testBegin( "test3MipDependencies" )
	crg::ResourceHandler handler;
	crg::FrameGraph graph{ handler, testCounts.testName };
	auto lp = graph.createImage( test::createImage( "lp", crg::PixelFormat::eR32G32B32_SFLOAT, 4u ) );
	auto m0v = graph.createView( test::createView( "m0v", lp, crg::PixelFormat::eR32G32B32_SFLOAT, 0u ) );
	auto m1v = graph.createView( test::createView( "m1v", lp, crg::PixelFormat::eR32G32B32_SFLOAT, 1u ) );
	auto m0a = crg::Attachment::createDefault( m0v );
	auto & ssaoMinifyPass1 = graph.createPass( "ssaoMinifyPass1"
		, [&testCounts]( crg::FramePass const & framePass
			, crg::GraphContext & context
			, crg::RunnableGraph & runGraph )
		{
			return createDummy( testCounts
				, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
		} );
	ssaoMinifyPass1.addInputSampled( m0a, 0 );
	auto m1a = ssaoMinifyPass1.addOutputColourTarget( m1v );

	auto m2v = graph.createView( test::createView( "m2v", lp, crg::PixelFormat::eR32G32B32_SFLOAT, 2u ) );
	auto & ssaoMinifyPass2 = graph.createPass( "ssaoMinifyPass2"
		, [&testCounts]( crg::FramePass const & framePass
			, crg::GraphContext & context
			, crg::RunnableGraph & runGraph )
		{
			return createDummy( testCounts
				, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
		} );
	ssaoMinifyPass2.addInputSampled( *m1a, 0 );
	auto m2a = ssaoMinifyPass2.addOutputColourTarget( m2v );

	auto m3v = graph.createView( test::createView( "m3v", lp, crg::PixelFormat::eR32G32B32_SFLOAT, 3u ) );
	auto & ssaoMinifyPass3 = graph.createPass( "ssaoMinifyPass3"
		, [&testCounts]( crg::FramePass const & framePass
			, crg::GraphContext & context
			, crg::RunnableGraph & runGraph )
		{
			return createDummy( testCounts
				, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
		} );
	ssaoMinifyPass3.addInputSampled( *m2a, 0 );
	ssaoMinifyPass3.addOutputColourTarget( m3v );

	auto runnable = graph.compile( getContext() );
	auto stream = test::checkRunnable( testCounts, runnable );
	std::string ref = R"(digraph {
  "ssaoMinifyPass2" [ shape=ellipse ];
  "ssaoMinifyPass3" [ shape=ellipse ];
  "Transition to\nssaoMinifyPass3/m2v/Spl" [ shape=box ];
  "ssaoMinifyPass2" -> "Transition to\nssaoMinifyPass3/m2v/Spl" [ label="m2v" ];
  "Transition to\nssaoMinifyPass3/m2v/Spl" -> "ssaoMinifyPass3" [ label="m2v" ];
  "ssaoMinifyPass1" [ shape=ellipse ];
  "Transition to\nssaoMinifyPass2/m1v/Spl" [ shape=box ];
  "ssaoMinifyPass1" -> "Transition to\nssaoMinifyPass2/m1v/Spl" [ label="m1v" ];
  "Transition to\nssaoMinifyPass2/m1v/Spl" -> "ssaoMinifyPass2" [ label="m1v" ];
  "ExternalSource" [ shape=ellipse ];
  "Transition to\nssaoMinifyPass1/m0v/Spl" [ shape=box ];
  "ExternalSource" -> "Transition to\nssaoMinifyPass1/m0v/Spl" [ label="m0v" ];
  "Transition to\nssaoMinifyPass1/m0v/Spl" -> "ssaoMinifyPass1" [ label="m0v" ];
}
)";
	checkEqualSortedLines( stream, ref )
	testEnd()
}

TEST( RenderGraph, LoopDependencies )
{
	testBegin( "testLoopDependencies" )
	crg::ResourceHandler handler;
	crg::FrameGraph graph{ handler, testCounts.testName };
	auto a = graph.createImage( test::createImage( "a", crg::PixelFormat::eR32G32B32_SFLOAT ) );
	auto av = graph.createView( test::createView( "av", a, crg::PixelFormat::eR32G32B32_SFLOAT ) );
	auto b = graph.createImage( test::createImage( "b", crg::PixelFormat::eR32G32B32_SFLOAT ) );
	auto bv = graph.createView( test::createView( "bv", b, crg::PixelFormat::eR32G32B32_SFLOAT ) );
	auto ba = crg::Attachment::createDefault( bv );
	auto & pass1 = graph.createPass( "pass1"
		, [&testCounts]( crg::FramePass const & framePass
			, crg::GraphContext & context
			, crg::RunnableGraph & runGraph )
		{
			return createDummy( testCounts
				, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
		} );
	pass1.addInputSampled( ba, 0 );
	auto aa = pass1.addOutputColourTarget( av );

	auto & pass2 = graph.createPass( "pass2"
		, [&testCounts]( crg::FramePass const & framePass
			, crg::GraphContext & context
			, crg::RunnableGraph & runGraph )
		{
			return createDummy( testCounts
				, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
		} );
	pass2.addInputSampled( *aa, 0 );
	pass2.addOutputColourTarget( bv );

	auto runnable = graph.compile( getContext() );
	test::checkRunnable( testCounts, runnable );
	testEnd()
}

TEST( RenderGraph, LoopDependenciesWithRoot )
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
	auto ba = pass0.addOutputColourTarget( bv );

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
	pass1.addInputSampled( *ba, 0 );
	auto aa = pass1.addOutputColourTarget( av );

	auto & pass2 = graph.createPass( "pass2"
		, [&testCounts]( crg::FramePass const & framePass
			, crg::GraphContext & context
			, crg::RunnableGraph & runGraph )
		{
			return createDummy( testCounts
				, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
		} );
	pass2.addInputSampled( *aa, 0 );
	pass2.addOutputColourTarget( bv );

	auto runnable = graph.compile( getContext() );
	test::checkRunnable( testCounts, runnable );
	testEnd()
}

TEST( RenderGraph, LoopDependenciesWithRootAndLeaf )
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
	auto ca = pass0.addOutputColourTarget( cv );

	auto a = graph.createImage( test::createImage( "a", crg::PixelFormat::eR32G32B32_SFLOAT ) );
	auto av = graph.createView( test::createView( "av", a, crg::PixelFormat::eR32G32B32_SFLOAT ) );
	auto b = graph.createImage( test::createImage( "b", crg::PixelFormat::eR32G32B32_SFLOAT ) );
	auto bv = graph.createView( test::createView( "bv", b, crg::PixelFormat::eR32G32B32_SFLOAT ) );
	auto ba = crg::Attachment::createDefault( bv );
	auto & pass1 = graph.createPass( "pass1"
		, [&testCounts]( crg::FramePass const & framePass
			, crg::GraphContext & context
			, crg::RunnableGraph & runGraph )
		{
			return createDummy( testCounts
				, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
		} );
	pass1.addInputSampled( ba, 0 );
	pass1.addInputSampled( *ca, 1 );
	auto aa = pass1.addOutputColourTarget( av );

	auto & pass2 = graph.createPass( "pass2"
		, [&testCounts]( crg::FramePass const & framePass
			, crg::GraphContext & context
			, crg::RunnableGraph & runGraph )
		{
			return createDummy( testCounts
				, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
		} );
	pass2.addInputSampled( *aa, 0 );
	pass2.addInputSampled( *ca, 1 );
	pass2.addOutputColourTarget( bv );

	auto buf = graph.createBuffer( test::createBuffer( "buf" ) );
	auto bufv = graph.createView( test::createView( "bufv", buf ) );
	auto & pass3 = graph.createPass( "pass3"
		, [&testCounts]( crg::FramePass const & framePass
			, crg::GraphContext & context
			, crg::RunnableGraph & runGraph )
		{
			return createDummy( testCounts
				, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
		} );
	pass3.addInputSampled( *ca, 0 );
	pass3.addInputUniformBuffer( bufv, 1 );

	auto runnable = graph.compile( getContext() );
	auto stream = test::checkRunnable( testCounts, runnable );
	std::string ref = R"(digraph {
  "pass1" [ shape=ellipse ];
  "pass2" [ shape=ellipse ];
  "Transition to\npass2/av/Spl" [ shape=box ];
  "pass1" -> "Transition to\npass2/av/Spl" [ label="av" ];
  "Transition to\npass2/av/Spl" -> "pass2" [ label="av" ];
  "pass0" [ shape=ellipse ];
  "Transition to\npass1/cv/Spl" [ shape=box ];
  "pass0" -> "Transition to\npass1/cv/Spl" [ label="cv" ];
  "Transition to\npass1/cv/Spl" -> "pass1" [ label="cv" ];
  "pass3" [ shape=ellipse ];
  "ExternalSource" [ shape=ellipse ];
  "Transition to\npass1/bv/Spl" [ shape=box ];
  "ExternalSource" -> "Transition to\npass1/bv/Spl" [ label="bv" ];
  "Transition to\npass1/bv/Spl" -> "pass1" [ label="bv" ];
  "Transition to\npass3/bufv/UB" [ shape=box ];
  "ExternalSource" -> "Transition to\npass3/bufv/UB" [ label="bufv" ];
  "Transition to\npass3/bufv/UB" -> "pass3" [ label="bufv" ];
}
)";
	checkEqualSortedLines( stream, ref )
	testEnd()
}

crg::Attachment const * buildSsaoPass( test::TestCounts & testCounts
	, crg::Attachment const & lda
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
	ssaoLinearisePass.addInputSampled( lda, 0 );
	auto m0a = ssaoLinearisePass.addOutputColourTarget( m0v );

	auto m1v = graph.createView( test::createView( "m1v", lp, crg::PixelFormat::eR32_SFLOAT, 1u ) );
	auto & ssaoMinifyPass1 = graph.createPass( "ssaoMinifyPass1"
		, [&testCounts]( crg::FramePass const & framePass
			, crg::GraphContext & context
			, crg::RunnableGraph & runGraph )
		{
			return createDummy( testCounts
				, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
		} );
	ssaoMinifyPass1.addInputSampled( *m0a, 0 );
	auto m1a = ssaoMinifyPass1.addOutputColourTarget( m1v );

	auto m2v = graph.createView( test::createView( "m2v", lp, crg::PixelFormat::eR32G32B32_SFLOAT, 2u ) );
	auto & ssaoMinifyPass2 = graph.createPass( "ssaoMinifyPass2"
		, [&testCounts]( crg::FramePass const & framePass
			, crg::GraphContext & context
			, crg::RunnableGraph & runGraph )
		{
			return createDummy( testCounts
				, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
		} );
	ssaoMinifyPass2.addInputSampled( *m1a, 0 );
	auto m2a = ssaoMinifyPass2.addOutputColourTarget( m2v );

	auto m3v = graph.createView( test::createView( "m3v", lp, crg::PixelFormat::eR32G32B32_SFLOAT, 3u ) );
	auto & ssaoMinifyPass3 = graph.createPass( "ssaoMinifyPass3"
		, [&testCounts]( crg::FramePass const & framePass
			, crg::GraphContext & context
			, crg::RunnableGraph & runGraph )
		{
			return createDummy( testCounts
				, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
		} );
	ssaoMinifyPass3.addInputSampled( *m2a, 0 );
	auto m3a = ssaoMinifyPass3.addOutputColourTarget( m3v );

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
	ssaoRawPass.addInputSampled( *graph.mergeAttachments( { m0a, m1a, m2a, m3a } ), 0 );
	ssaoRawPass.addInputSampled( *m3a, 1 );
	auto rsa = ssaoRawPass.addOutputColourTarget( rsv );

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
	ssaoBlurPass.addInputSampled( *rsa, 0 );
	ssaoBlurPass.addInputSampled( *m3a, 1 );
	return ssaoBlurPass.addOutputColourTarget( blv );
}

TEST( RenderGraph, SsaoPass )
{
	testBegin( "testSsaoPass" )
	crg::ResourceHandler handler;
	crg::FrameGraph graph{ handler, testCounts.testName };
	auto d = graph.createImage( test::createImage( "d", crg::PixelFormat::eD32_SFLOAT_S8_UINT ) );
	auto dtv = graph.createView( test::createView( "dtv", d, crg::PixelFormat::eD32_SFLOAT_S8_UINT ) );
	auto ld = graph.createImage( test::createImage( "ld", crg::PixelFormat::eR32_SFLOAT ) );
	auto ldv = graph.createView( test::createView( "ldv", ld, crg::PixelFormat::eR32_SFLOAT ) );
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
	auto d1a = geometryPass.addOutputColourTarget( d1v );
	auto d2a = geometryPass.addOutputColourTarget( d2v );
	auto d3a = geometryPass.addOutputColourTarget( d3v );
	auto d4a = geometryPass.addOutputColourTarget( d4v );
	auto lda = geometryPass.addOutputColourTarget( ldv );
	geometryPass.addOutputColourTarget( vv );
	geometryPass.addOutputDepthStencilTarget( dtv );

	auto ssaoa = buildSsaoPass( testCounts
		, *lda
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
	ambientPass.addInputSampled( *lda, 0 );
	ambientPass.addInputSampled( *d1a, 1 );
	ambientPass.addInputSampled( *d2a, 2 );
	ambientPass.addInputSampled( *d3a, 3 );
	ambientPass.addInputSampled( *d4a, 4 );
	ambientPass.addInputSampled( *ssaoa, 5 );
	ambientPass.addOutputColourTarget( ofv );

	auto runnable = graph.compile( getContext() );
	auto stream = test::checkRunnable( testCounts, runnable );
	std::string ref = R"(digraph {
  "ambientPass" [ shape=ellipse ];
  "geometryPass" [ shape=ellipse ];
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
  "ssaoBlurPass" [ shape=ellipse ];
  "Transition to\nambientPass/b1v/Spl" [ shape=box ];
  "ssaoBlurPass" -> "Transition to\nambientPass/b1v/Spl" [ label="b1v" ];
  "Transition to\nambientPass/b1v/Spl" -> "ambientPass" [ label="b1v" ];
  "ssaoRawPass" [ shape=ellipse ];
  "Transition to\nssaoBlurPass/rsv/Spl" [ shape=box ];
  "ssaoRawPass" -> "Transition to\nssaoBlurPass/rsv/Spl" [ label="rsv" ];
  "Transition to\nssaoBlurPass/rsv/Spl" -> "ssaoBlurPass" [ label="rsv" ];
  "ssaoMinifyPass3" [ shape=ellipse ];
  "Transition to\nssaoRawPass/lp/Spl3" [ shape=box ];
  "ssaoMinifyPass3" -> "Transition to\nssaoRawPass/lp/Spl3" [ label="m3v" ];
  "Transition to\nssaoRawPass/lp/Spl3" -> "ssaoRawPass" [ label="m3v" ];
  "ssaoMinifyPass2" [ shape=ellipse ];
  "Transition to\nssaoMinifyPass3/m2v/Spl" [ shape=box ];
  "ssaoMinifyPass2" -> "Transition to\nssaoMinifyPass3/m2v/Spl" [ label="m2v" ];
  "Transition to\nssaoMinifyPass3/m2v/Spl" -> "ssaoMinifyPass3" [ label="m2v" ];
  "ssaoMinifyPass1" [ shape=ellipse ];
  "Transition to\nssaoMinifyPass2/m1v/Spl" [ shape=box ];
  "ssaoMinifyPass1" -> "Transition to\nssaoMinifyPass2/m1v/Spl" [ label="m1v" ];
  "Transition to\nssaoMinifyPass2/m1v/Spl" -> "ssaoMinifyPass2" [ label="m1v" ];
  "ssaoLinearisePass" [ shape=ellipse ];
  "Transition to\nssaoMinifyPass1/m0v/Spl" [ shape=box ];
  "ssaoLinearisePass" -> "Transition to\nssaoMinifyPass1/m0v/Spl" [ label="m0v" ];
  "Transition to\nssaoMinifyPass1/m0v/Spl" -> "ssaoMinifyPass1" [ label="m0v" ];
  "Transition to\nssaoLinearisePass/ldv/Spl" [ shape=box ];
  "geometryPass" -> "Transition to\nssaoLinearisePass/ldv/Spl" [ label="ldv" ];
  "Transition to\nssaoLinearisePass/ldv/Spl" -> "ssaoLinearisePass" [ label="ldv" ];
}
)";
	checkEqualSortedLines( stream, ref )
	testEnd()
}

TEST( RenderGraph, BloomPostEffect )
{
	testBegin( "testBloomPostEffect" )
	crg::ResourceHandler handler;
	crg::FrameGraph graph{ handler, testCounts.testName };
	auto scene = graph.createImage( test::createImage( "scene", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
	auto scenev = graph.createView( test::createView( "scenev", scene, crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
	auto scenea = crg::Attachment::createDefault( scenev );
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
	hiPass.addInputSampled( scenea, 0u );
	auto hia = hiPass.addOutputColourTarget( graph.mergeViews( { hi0v, hi1v, hi2v, hi3v } ) );
	require( hia->source.size() == 4u );

	auto & blurPass0X = graph.createPass( "blurPass0X"
		, [&testCounts]( crg::FramePass const & framePass
			, crg::GraphContext & context
			, crg::RunnableGraph & runGraph )
		{
			return createDummy( testCounts
				, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
		} );
	blurPass0X.addInputSampled( *hia->getSource( 0 ), 0u );
	auto bl0a = blurPass0X.addOutputColourTarget( bl0v );
	auto & blurPass0Y = graph.createPass( "blurPass0Y"
		, [&testCounts]( crg::FramePass const & framePass
			, crg::GraphContext & context
			, crg::RunnableGraph & runGraph )
		{
			return createDummy( testCounts
				, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
		} );
	blurPass0Y.addInputSampled( *bl0a, 0u );
	auto hi0a = blurPass0Y.addOutputColourTarget( hi0v );

	auto & blurPass1X = graph.createPass( "blurPass1X"
		, [&testCounts]( crg::FramePass const & framePass
			, crg::GraphContext & context
			, crg::RunnableGraph & runGraph )
		{
			return createDummy( testCounts
				, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
		} );
	blurPass1X.addInputSampled( *hia->getSource( 1 ), 0u );
	auto bl1a = blurPass1X.addOutputColourTarget( bl1v );
	auto & blurPass1Y = graph.createPass( "blurPass1Y"
		, [&testCounts]( crg::FramePass const & framePass
			, crg::GraphContext & context
			, crg::RunnableGraph & runGraph )
		{
			return createDummy( testCounts
				, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
		} );
	blurPass1Y.addInputSampled( *bl1a, 0u );
	auto hi1a = blurPass1Y.addOutputColourTarget( hi1v );

	auto & blurPass2X = graph.createPass( "blurPass2X"
		, [&testCounts]( crg::FramePass const & framePass
			, crg::GraphContext & context
			, crg::RunnableGraph & runGraph )
		{
			return createDummy( testCounts
				, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
		} );
	blurPass2X.addInputSampled( *hia->getSource( 2 ), 0u );
	auto bl2a = blurPass2X.addOutputColourTarget( bl2v );
	auto & blurPass2Y = graph.createPass( "blurPass2Y"
		, [&testCounts]( crg::FramePass const & framePass
			, crg::GraphContext & context
			, crg::RunnableGraph & runGraph )
		{
			return createDummy( testCounts
				, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
		} );
	blurPass2Y.addInputSampled( *bl2a, 0u );
	auto hi2a = blurPass2Y.addOutputColourTarget( hi2v );

	auto & blurPass3X = graph.createPass( "blurPass3X"
		, [&testCounts]( crg::FramePass const & framePass
			, crg::GraphContext & context
			, crg::RunnableGraph & runGraph )
		{
			return createDummy( testCounts
				, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
		} );
	blurPass3X.addInputSampled( *hia->getSource( 3 ), 0u );
	auto bl3a = blurPass3X.addOutputColourTarget( bl3v );
	auto & blurPass3Y = graph.createPass( "blurPass3Y"
		, [&testCounts]( crg::FramePass const & framePass
			, crg::GraphContext & context
			, crg::RunnableGraph & runGraph )
		{
			return createDummy( testCounts
				, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
		} );
	blurPass3Y.addInputSampled( *bl3a, 0u );
	auto hi3a = blurPass3Y.addOutputColourTarget( hi3v );

	auto & combinePass = graph.createPass( "combinePass"
		, [&testCounts]( crg::FramePass const & framePass
			, crg::GraphContext & context
			, crg::RunnableGraph & runGraph )
		{
			return createDummy( testCounts
				, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
		} );
	combinePass.addInputSampled( scenea, 0u );
	combinePass.addInputSampled( *graph.mergeAttachments( { hi0a, hi1a, hi2a, hi3a } ), 1u );
	combinePass.addOutputColourTarget( outputv );

	auto runnable = graph.compile( getContext() );
	test::checkRunnable( testCounts, runnable );
	testEnd()
}

template< bool EnableSsao >
crg::Attachment const * buildDeferred( test::TestCounts & testCounts
	, crg::Attachment const *& lda
	, crg::Attachment const *& dta
	, crg::ImageViewId const & ldv
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

	auto d1a = geometryPass.addOutputColourTarget( d1v );
	auto d2a = geometryPass.addOutputColourTarget( d2v );
	auto d3a = geometryPass.addOutputColourTarget( d3v );
	auto d4a = geometryPass.addOutputColourTarget( d4v );
	if ( !lda )
		lda = geometryPass.addOutputColourTarget( ldv );
	geometryPass.addOutputColourTarget( vtv );
	if ( dta )
		geometryPass.addInputDepthStencilTarget( *dta );
	else
		dta = geometryPass.addOutputDepthStencilTarget( dtv );

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
	lightingPass.addInputSampled( *lda, 0 );
	lightingPass.addInputSampled( *d1a, 1 );
	lightingPass.addInputSampled( *d2a, 2 );
	lightingPass.addInputSampled( *d3a, 3 );
	lightingPass.addInputSampled( *d4a, 4 );
	auto dfa = lightingPass.addOutputColourTarget( dfv );
	auto spa = lightingPass.addOutputColourTarget( spv );

	auto of = graph.createImage( test::createImage( "of", crg::PixelFormat::eR32G32B32_SFLOAT ) );
	auto ofv = graph.createView( test::createView( "ofv", of, crg::PixelFormat::eR32G32B32_SFLOAT ) );

	if constexpr ( EnableSsao )
	{
		auto ssaoa = buildSsaoPass( testCounts
			, *lda
			, graph );
		auto & ambientPass = graph.createPass( "ambientPass"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		ambientPass.addInputSampled( *lda, 0 );
		ambientPass.addInputSampled( *d1a, 1 );
		ambientPass.addInputSampled( *d2a, 2 );
		ambientPass.addInputSampled( *d3a, 3 );
		ambientPass.addInputSampled( *d4a, 4 );
		ambientPass.addInputSampled( *dfa, 5 );
		ambientPass.addInputSampled( *spa, 6 );
		ambientPass.addInputSampled( *ssaoa, 7 );
		return ambientPass.addOutputColourTarget( ofv );
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
		ambientPass.addInputSampled( *lda, 0 );
		ambientPass.addInputSampled( *d1a, 1 );
		ambientPass.addInputSampled( *d2a, 2 );
		ambientPass.addInputSampled( *d3a, 3 );
		ambientPass.addInputSampled( *d4a, 4 );
		ambientPass.addInputSampled( *dfa, 5 );
		ambientPass.addInputSampled( *spa, 6 );
		return ambientPass.addOutputColourTarget( ofv );
	}
}

crg::Attachment const * buildWeightedBlended( test::TestCounts & testCounts
	, crg::Attachment const *& dta
	, crg::Attachment const * lda
	, crg::ImageViewId const & dtv
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
	auto aa = accumulationPass.addOutputColourTarget( av );
	auto ra = accumulationPass.addOutputColourTarget( rv );
	if ( dta )
		accumulationPass.addInputDepthStencilTarget( *dta );
	else
		dta = accumulationPass.addOutputDepthStencilTarget( dtv );

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
	if ( lda )
		combinePass.addInputSampled( *lda, 0u );
	combinePass.addInputSampled( *aa, 1u );
	combinePass.addInputSampled( *ra, 2u );
	return combinePass.addOutputColourTarget( cv );
}

template< bool EnableDepthPrepass
	, bool EnableOpaque
	, bool EnableSsao
	, bool EnableTransparent >
struct ParamsT
{
	static constexpr bool EnableDepthPrepassT = EnableDepthPrepass;
	static constexpr bool EnableOpaqueT = EnableOpaque;
	static constexpr bool EnableSsaoT = EnableSsao;
	static constexpr bool EnableTransparentT = EnableTransparent;
};

using ParamTypes = testing::Types< ParamsT< false, false, false, false >
	, ParamsT< false, false, false, true >
	, ParamsT< false, true, false, false >
	, ParamsT< false, true, false, true >
	, ParamsT< false, true, true, false >
	, ParamsT< false, true, true, true >
	, ParamsT< true, false, false, false >
	, ParamsT< true, false, false, true >
	, ParamsT< true, true, false, false >
	, ParamsT< true, true, false, true >
	, ParamsT< true, true, true, false >
	, ParamsT< true, true, true, true > >;

class ParamTypeNames
{
public:
	template< typename T >
	static std::string GetName( int )
	{
		static constexpr bool EnableDepthPrepass = T::EnableDepthPrepassT;
		static constexpr bool EnableOpaque = T::EnableOpaqueT;
		static constexpr bool EnableSsao = T::EnableSsaoT;
		static constexpr bool EnableTransparent = T::EnableTransparentT;
		return ( EnableDepthPrepass ? std::string{ "Prepass" } : std::string{} )
			+ ( EnableOpaque ? std::string{ "Opaque" } : std::string{} )
			+ ( EnableSsao ? std::string{ "Ssao" } : std::string{} )
			+ ( EnableTransparent ? std::string{ "Transparent" } : std::string{} );
	}
};

template< typename ParamT >
struct RenderGraphT : public ::testing::Test
{
};

TYPED_TEST_SUITE( RenderGraphT, ParamTypes, ParamTypeNames );

TYPED_TEST( RenderGraphT, Render )
{
	static constexpr bool EnableDepthPrepass = TypeParam::EnableDepthPrepassT;
	static constexpr bool EnableOpaque = TypeParam::EnableOpaqueT;
	static constexpr bool EnableSsao = TypeParam::EnableSsaoT;
	static constexpr bool EnableTransparent = TypeParam::EnableTransparentT;
	testBegin( "testRender"
		+ ( EnableDepthPrepass ? std::string{ "Prepass" } : std::string{} )
		+ ( EnableOpaque ? std::string{ "Opaque" } : std::string{} )
		+ ( EnableSsao ? std::string{ "Ssao" } : std::string{} )
		+ ( EnableTransparent ? std::string{ "Transparent" } : std::string{} ) )
	crg::ResourceHandler handler;
	crg::FrameGraph graph{ handler, testCounts.testName };
	auto d = graph.createImage( test::createImage( "d", crg::PixelFormat::eD32_SFLOAT_S8_UINT ) );
	auto dtv = graph.createView( test::createView( "dtv", d, crg::PixelFormat::eD32_SFLOAT_S8_UINT ) );
	auto ld = graph.createImage( test::createImage( "ld", crg::PixelFormat::eR32_SFLOAT ) );
	auto ldv = graph.createView( test::createView( "ldv", ld, crg::PixelFormat::eR32_SFLOAT ) );
	crg::Attachment const * dta{};
	crg::Attachment const * lda{};

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
		lda = depthPrepass.addOutputColourTarget ( ldv );
		dta = depthPrepass.addOutputDepthTarget ( dtv );
	}

	auto o = graph.createImage( test::createImage( "o", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
	auto otv = graph.createView( test::createView( "otv", o, crg::PixelFormat::eR16G16B16A16_SFLOAT ) );

	if constexpr ( EnableOpaque )
	{
		auto v = graph.createImage( test::createImage( "v", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto vv = graph.createView( test::createView( "vv", v, crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto dca = buildDeferred< EnableSsao >( testCounts
			, lda
			, dta
			, ldv
			, dtv
			, vv
			, graph );

		if constexpr ( EnableTransparent )
		{
			auto wbcsa = buildWeightedBlended( testCounts
				, dta
				, lda
				, dtv
				, graph );
			auto & finalCombinePass = graph.createPass( "finalCombinePass"
				, [&testCounts]( crg::FramePass const & framePass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
					return createDummy( testCounts
						, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
				} );
			finalCombinePass.addInputSampled( *lda, 0 );
			finalCombinePass.addInputSampled( *dca, 1 );
			finalCombinePass.addInputSampled( *wbcsa, 2 );
			finalCombinePass.addOutputColourTarget( otv );
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
			finalCombinePass.addInputSampled( *lda, 0 );
			finalCombinePass.addInputSampled( *dca, 1 );
			finalCombinePass.addOutputColourTarget( otv );
		}
			
	}
	else if constexpr ( EnableTransparent )
	{
		auto wba = buildWeightedBlended( testCounts
			, dta
			, lda
			, dtv
			, graph );
		auto & finalCombinePass = graph.createPass( "finalCombinePass"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		if ( lda )
			finalCombinePass.addInputSampled( *lda, 0 );
		finalCombinePass.addInputSampled( *wba, 1 );
		finalCombinePass.addOutputColourTarget( otv );
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
		if ( lda )
			finalCombinePass.addInputSampled( *lda, 0 );
		finalCombinePass.addOutputColourTarget( otv );
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
  "depthPrepass" [ shape=ellipse ];
  "geometryPass" [ shape=ellipse ];
  "Transition to\ngeometryPass/dtv/IRds" [ shape=box ];
  "depthPrepass" -> "Transition to\ngeometryPass/dtv/IRds" [ label="dtv" ];
  "Transition to\ngeometryPass/dtv/IRds" -> "geometryPass" [ label="dtv" ];
  "ambientPass" [ shape=ellipse ];
  "finalCombinePass" [ shape=ellipse ];
  "combinePass" [ shape=ellipse ];
  "Transition to\nfinalCombinePass/ofv/Spl" [ shape=box ];
  "ambientPass" -> "Transition to\nfinalCombinePass/ofv/Spl" [ label="ofv" ];
  "Transition to\nfinalCombinePass/ofv/Spl" -> "finalCombinePass" [ label="ofv" ];
  "Transition to\nfinalCombinePass/cv/Spl" [ shape=box ];
  "combinePass" -> "Transition to\nfinalCombinePass/cv/Spl" [ label="cv" ];
  "Transition to\nfinalCombinePass/cv/Spl" -> "finalCombinePass" [ label="cv" ];
  "lightingPass" [ shape=ellipse ];
  "ssaoBlurPass" [ shape=ellipse ];
  "Transition to\nambientPass/dfv/Spl" [ shape=box ];
  "lightingPass" -> "Transition to\nambientPass/dfv/Spl" [ label="dfv" ];
  "Transition to\nambientPass/dfv/Spl" -> "ambientPass" [ label="dfv" ];
  "Transition to\nambientPass/spv/Spl" [ shape=box ];
  "lightingPass" -> "Transition to\nambientPass/spv/Spl" [ label="spv" ];
  "Transition to\nambientPass/spv/Spl" -> "ambientPass" [ label="spv" ];
  "Transition to\nambientPass/b1v/Spl" [ shape=box ];
  "ssaoBlurPass" -> "Transition to\nambientPass/b1v/Spl" [ label="b1v" ];
  "Transition to\nambientPass/b1v/Spl" -> "ambientPass" [ label="b1v" ];
  "Transition to\nlightingPass/ldv/Spl" [ shape=box ];
  "depthPrepass" -> "Transition to\nlightingPass/ldv/Spl" [ label="ldv" ];
  "Transition to\nlightingPass/ldv/Spl" -> "lightingPass" [ label="ldv" ];
  "Transition to\nlightingPass/d1v/Spl" [ shape=box ];
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
  "ssaoRawPass" [ shape=ellipse ];
  "Transition to\nssaoBlurPass/rsv/Spl" [ shape=box ];
  "ssaoRawPass" -> "Transition to\nssaoBlurPass/rsv/Spl" [ label="rsv" ];
  "Transition to\nssaoBlurPass/rsv/Spl" -> "ssaoBlurPass" [ label="rsv" ];
  "ssaoMinifyPass3" [ shape=ellipse ];
  "Transition to\nssaoRawPass/lp/Spl3" [ shape=box ];
  "ssaoMinifyPass3" -> "Transition to\nssaoRawPass/lp/Spl3" [ label="m3v" ];
  "Transition to\nssaoRawPass/lp/Spl3" -> "ssaoRawPass" [ label="m3v" ];
  "ssaoMinifyPass2" [ shape=ellipse ];
  "Transition to\nssaoMinifyPass3/m2v/Spl" [ shape=box ];
  "ssaoMinifyPass2" -> "Transition to\nssaoMinifyPass3/m2v/Spl" [ label="m2v" ];
  "Transition to\nssaoMinifyPass3/m2v/Spl" -> "ssaoMinifyPass3" [ label="m2v" ];
  "ssaoMinifyPass1" [ shape=ellipse ];
  "Transition to\nssaoMinifyPass2/m1v/Spl" [ shape=box ];
  "ssaoMinifyPass1" -> "Transition to\nssaoMinifyPass2/m1v/Spl" [ label="m1v" ];
  "Transition to\nssaoMinifyPass2/m1v/Spl" -> "ssaoMinifyPass2" [ label="m1v" ];
  "ssaoLinearisePass" [ shape=ellipse ];
  "Transition to\nssaoMinifyPass1/m0v/Spl" [ shape=box ];
  "ssaoLinearisePass" -> "Transition to\nssaoMinifyPass1/m0v/Spl" [ label="m0v" ];
  "Transition to\nssaoMinifyPass1/m0v/Spl" -> "ssaoMinifyPass1" [ label="m0v" ];
  "accumulationPass" [ shape=ellipse ];
  "Transition to\ncombinePass/av/Spl" [ shape=box ];
  "accumulationPass" -> "Transition to\ncombinePass/av/Spl" [ label="av" ];
  "Transition to\ncombinePass/av/Spl" -> "combinePass" [ label="av" ];
  "Transition to\ncombinePass/rv/Spl" [ shape=box ];
  "accumulationPass" -> "Transition to\ncombinePass/rv/Spl" [ label="rv" ];
  "Transition to\ncombinePass/rv/Spl" -> "combinePass" [ label="rv" ];
}
)";
				}
				else
				{
					ref = R"(digraph {
  "depthPrepass" [ shape=ellipse ];
  "geometryPass" [ shape=ellipse ];
  "Transition to\ngeometryPass/dtv/IRds" [ shape=box ];
  "depthPrepass" -> "Transition to\ngeometryPass/dtv/IRds" [ label="dtv" ];
  "Transition to\ngeometryPass/dtv/IRds" -> "geometryPass" [ label="dtv" ];
  "ambientPass" [ shape=ellipse ];
  "finalCombinePass" [ shape=ellipse ];
  "Transition to\nfinalCombinePass/ofv/Spl" [ shape=box ];
  "ambientPass" -> "Transition to\nfinalCombinePass/ofv/Spl" [ label="ofv" ];
  "Transition to\nfinalCombinePass/ofv/Spl" -> "finalCombinePass" [ label="ofv" ];
  "lightingPass" [ shape=ellipse ];
  "ssaoBlurPass" [ shape=ellipse ];
  "Transition to\nambientPass/dfv/Spl" [ shape=box ];
  "lightingPass" -> "Transition to\nambientPass/dfv/Spl" [ label="dfv" ];
  "Transition to\nambientPass/dfv/Spl" -> "ambientPass" [ label="dfv" ];
  "Transition to\nambientPass/spv/Spl" [ shape=box ];
  "lightingPass" -> "Transition to\nambientPass/spv/Spl" [ label="spv" ];
  "Transition to\nambientPass/spv/Spl" -> "ambientPass" [ label="spv" ];
  "Transition to\nambientPass/b1v/Spl" [ shape=box ];
  "ssaoBlurPass" -> "Transition to\nambientPass/b1v/Spl" [ label="b1v" ];
  "Transition to\nambientPass/b1v/Spl" -> "ambientPass" [ label="b1v" ];
  "Transition to\nlightingPass/ldv/Spl" [ shape=box ];
  "depthPrepass" -> "Transition to\nlightingPass/ldv/Spl" [ label="ldv" ];
  "Transition to\nlightingPass/ldv/Spl" -> "lightingPass" [ label="ldv" ];
  "Transition to\nlightingPass/d1v/Spl" [ shape=box ];
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
  "ssaoRawPass" [ shape=ellipse ];
  "Transition to\nssaoBlurPass/rsv/Spl" [ shape=box ];
  "ssaoRawPass" -> "Transition to\nssaoBlurPass/rsv/Spl" [ label="rsv" ];
  "Transition to\nssaoBlurPass/rsv/Spl" -> "ssaoBlurPass" [ label="rsv" ];
  "ssaoMinifyPass3" [ shape=ellipse ];
  "Transition to\nssaoRawPass/lp/Spl3" [ shape=box ];
  "ssaoMinifyPass3" -> "Transition to\nssaoRawPass/lp/Spl3" [ label="m3v" ];
  "Transition to\nssaoRawPass/lp/Spl3" -> "ssaoRawPass" [ label="m3v" ];
  "ssaoMinifyPass2" [ shape=ellipse ];
  "Transition to\nssaoMinifyPass3/m2v/Spl" [ shape=box ];
  "ssaoMinifyPass2" -> "Transition to\nssaoMinifyPass3/m2v/Spl" [ label="m2v" ];
  "Transition to\nssaoMinifyPass3/m2v/Spl" -> "ssaoMinifyPass3" [ label="m2v" ];
  "ssaoMinifyPass1" [ shape=ellipse ];
  "Transition to\nssaoMinifyPass2/m1v/Spl" [ shape=box ];
  "ssaoMinifyPass1" -> "Transition to\nssaoMinifyPass2/m1v/Spl" [ label="m1v" ];
  "Transition to\nssaoMinifyPass2/m1v/Spl" -> "ssaoMinifyPass2" [ label="m1v" ];
  "ssaoLinearisePass" [ shape=ellipse ];
  "Transition to\nssaoMinifyPass1/m0v/Spl" [ shape=box ];
  "ssaoLinearisePass" -> "Transition to\nssaoMinifyPass1/m0v/Spl" [ label="m0v" ];
  "Transition to\nssaoMinifyPass1/m0v/Spl" -> "ssaoMinifyPass1" [ label="m0v" ];
}
)";
				}
			}
			else if constexpr ( EnableTransparent )
			{
				ref = R"(digraph {
  "depthPrepass" [ shape=ellipse ];
  "geometryPass" [ shape=ellipse ];
  "Transition to\ngeometryPass/dtv/IRds" [ shape=box ];
  "depthPrepass" -> "Transition to\ngeometryPass/dtv/IRds" [ label="dtv" ];
  "Transition to\ngeometryPass/dtv/IRds" -> "geometryPass" [ label="dtv" ];
  "ambientPass" [ shape=ellipse ];
  "finalCombinePass" [ shape=ellipse ];
  "combinePass" [ shape=ellipse ];
  "Transition to\nfinalCombinePass/ofv/Spl" [ shape=box ];
  "ambientPass" -> "Transition to\nfinalCombinePass/ofv/Spl" [ label="ofv" ];
  "Transition to\nfinalCombinePass/ofv/Spl" -> "finalCombinePass" [ label="ofv" ];
  "Transition to\nfinalCombinePass/cv/Spl" [ shape=box ];
  "combinePass" -> "Transition to\nfinalCombinePass/cv/Spl" [ label="cv" ];
  "Transition to\nfinalCombinePass/cv/Spl" -> "finalCombinePass" [ label="cv" ];
  "lightingPass" [ shape=ellipse ];
  "Transition to\nambientPass/dfv/Spl" [ shape=box ];
  "lightingPass" -> "Transition to\nambientPass/dfv/Spl" [ label="dfv" ];
  "Transition to\nambientPass/dfv/Spl" -> "ambientPass" [ label="dfv" ];
  "Transition to\nambientPass/spv/Spl" [ shape=box ];
  "lightingPass" -> "Transition to\nambientPass/spv/Spl" [ label="spv" ];
  "Transition to\nambientPass/spv/Spl" -> "ambientPass" [ label="spv" ];
  "Transition to\nlightingPass/ldv/Spl" [ shape=box ];
  "depthPrepass" -> "Transition to\nlightingPass/ldv/Spl" [ label="ldv" ];
  "Transition to\nlightingPass/ldv/Spl" -> "lightingPass" [ label="ldv" ];
  "Transition to\nlightingPass/d1v/Spl" [ shape=box ];
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
  "accumulationPass" [ shape=ellipse ];
  "Transition to\ncombinePass/av/Spl" [ shape=box ];
  "accumulationPass" -> "Transition to\ncombinePass/av/Spl" [ label="av" ];
  "Transition to\ncombinePass/av/Spl" -> "combinePass" [ label="av" ];
  "Transition to\ncombinePass/rv/Spl" [ shape=box ];
  "accumulationPass" -> "Transition to\ncombinePass/rv/Spl" [ label="rv" ];
  "Transition to\ncombinePass/rv/Spl" -> "combinePass" [ label="rv" ];
}
)";
			}
			else
			{
				ref = R"(digraph {
  "depthPrepass" [ shape=ellipse ];
  "geometryPass" [ shape=ellipse ];
  "Transition to\ngeometryPass/dtv/IRds" [ shape=box ];
  "depthPrepass" -> "Transition to\ngeometryPass/dtv/IRds" [ label="dtv" ];
  "Transition to\ngeometryPass/dtv/IRds" -> "geometryPass" [ label="dtv" ];
  "ambientPass" [ shape=ellipse ];
  "finalCombinePass" [ shape=ellipse ];
  "Transition to\nfinalCombinePass/ofv/Spl" [ shape=box ];
  "ambientPass" -> "Transition to\nfinalCombinePass/ofv/Spl" [ label="ofv" ];
  "Transition to\nfinalCombinePass/ofv/Spl" -> "finalCombinePass" [ label="ofv" ];
  "lightingPass" [ shape=ellipse ];
  "Transition to\nambientPass/dfv/Spl" [ shape=box ];
  "lightingPass" -> "Transition to\nambientPass/dfv/Spl" [ label="dfv" ];
  "Transition to\nambientPass/dfv/Spl" -> "ambientPass" [ label="dfv" ];
  "Transition to\nambientPass/spv/Spl" [ shape=box ];
  "lightingPass" -> "Transition to\nambientPass/spv/Spl" [ label="spv" ];
  "Transition to\nambientPass/spv/Spl" -> "ambientPass" [ label="spv" ];
  "Transition to\nlightingPass/ldv/Spl" [ shape=box ];
  "depthPrepass" -> "Transition to\nlightingPass/ldv/Spl" [ label="ldv" ];
  "Transition to\nlightingPass/ldv/Spl" -> "lightingPass" [ label="ldv" ];
  "Transition to\nlightingPass/d1v/Spl" [ shape=box ];
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
}
)";
			}
		}
		else if constexpr ( EnableTransparent )
		{
			ref = R"(digraph {
  "combinePass" [ shape=ellipse ];
  "finalCombinePass" [ shape=ellipse ];
  "Transition to\nfinalCombinePass/cv/Spl" [ shape=box ];
  "combinePass" -> "Transition to\nfinalCombinePass/cv/Spl" [ label="cv" ];
  "Transition to\nfinalCombinePass/cv/Spl" -> "finalCombinePass" [ label="cv" ];
  "depthPrepass" [ shape=ellipse ];
  "accumulationPass" [ shape=ellipse ];
  "Transition to\ncombinePass/ldv/Spl" [ shape=box ];
  "depthPrepass" -> "Transition to\ncombinePass/ldv/Spl" [ label="ldv" ];
  "Transition to\ncombinePass/ldv/Spl" -> "combinePass" [ label="ldv" ];
  "Transition to\ncombinePass/av/Spl" [ shape=box ];
  "accumulationPass" -> "Transition to\ncombinePass/av/Spl" [ label="av" ];
  "Transition to\ncombinePass/av/Spl" -> "combinePass" [ label="av" ];
  "Transition to\ncombinePass/rv/Spl" [ shape=box ];
  "accumulationPass" -> "Transition to\ncombinePass/rv/Spl" [ label="rv" ];
  "Transition to\ncombinePass/rv/Spl" -> "combinePass" [ label="rv" ];
  "Transition to\naccumulationPass/dtv/IRds" [ shape=box ];
  "depthPrepass" -> "Transition to\naccumulationPass/dtv/IRds" [ label="dtv" ];
  "Transition to\naccumulationPass/dtv/IRds" -> "accumulationPass" [ label="dtv" ];
}
)";
		}
		else
		{
			ref = R"(digraph {
  "Transition to\nfinalCombinePass/ldv/Spl" [ shape=box ];
  "depthPrepass" [ shape=ellipse ];
  "finalCombinePass" [ shape=ellipse ];
  "depthPrepass" -> "Transition to\nfinalCombinePass/ldv/Spl" [ label="ldv" ];
  "Transition to\nfinalCombinePass/ldv/Spl" -> "finalCombinePass" [ label="ldv" ];
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
  "finalCombinePass" [ shape=ellipse ];
  "ambientPass" [ shape=ellipse ];
  "Transition to\nfinalCombinePass/ofv/Spl" [ shape=box ];
  "ambientPass" -> "Transition to\nfinalCombinePass/ofv/Spl" [ label="ofv" ];
  "Transition to\nfinalCombinePass/ofv/Spl" -> "finalCombinePass" [ label="ofv" ];
  "lightingPass" [ shape=ellipse ];
  "Transition to\nambientPass/dfv/Spl" [ shape=box ];
  "lightingPass" -> "Transition to\nambientPass/dfv/Spl" [ label="dfv" ];
  "Transition to\nambientPass/dfv/Spl" -> "ambientPass" [ label="dfv" ];
  "Transition to\nambientPass/spv/Spl" [ shape=box ];
  "lightingPass" -> "Transition to\nambientPass/spv/Spl" [ label="spv" ];
  "Transition to\nambientPass/spv/Spl" -> "ambientPass" [ label="spv" ];
  "geometryPass" [ shape=ellipse ];
  "Transition to\nlightingPass/d1v/Spl" [ shape=box ];
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
  "Transition to\nlightingPass/ldv/Spl" [ shape=box ];
  "geometryPass" -> "Transition to\nlightingPass/ldv/Spl" [ label="ldv" ];
  "Transition to\nlightingPass/ldv/Spl" -> "lightingPass" [ label="ldv" ];
  "ssaoBlurPass" [ shape=ellipse ];
  "Transition to\nambientPass/b1v/Spl" [ shape=box ];
  "ssaoBlurPass" -> "Transition to\nambientPass/b1v/Spl" [ label="b1v" ];
  "Transition to\nambientPass/b1v/Spl" -> "ambientPass" [ label="b1v" ];
  "ssaoRawPass" [ shape=ellipse ];
  "Transition to\nssaoBlurPass/rsv/Spl" [ shape=box ];
  "ssaoRawPass" -> "Transition to\nssaoBlurPass/rsv/Spl" [ label="rsv" ];
  "Transition to\nssaoBlurPass/rsv/Spl" -> "ssaoBlurPass" [ label="rsv" ];
  "ssaoMinifyPass3" [ shape=ellipse ];
  "Transition to\nssaoRawPass/lp/Spl3" [ shape=box ];
  "ssaoMinifyPass3" -> "Transition to\nssaoRawPass/lp/Spl3" [ label="m3v" ];
  "Transition to\nssaoRawPass/lp/Spl3" -> "ssaoRawPass" [ label="m3v" ];
  "ssaoMinifyPass2" [ shape=ellipse ];
  "Transition to\nssaoMinifyPass3/m2v/Spl" [ shape=box ];
  "ssaoMinifyPass2" -> "Transition to\nssaoMinifyPass3/m2v/Spl" [ label="m2v" ];
  "Transition to\nssaoMinifyPass3/m2v/Spl" -> "ssaoMinifyPass3" [ label="m2v" ];
  "ssaoMinifyPass1" [ shape=ellipse ];
  "Transition to\nssaoMinifyPass2/m1v/Spl" [ shape=box ];
  "ssaoMinifyPass1" -> "Transition to\nssaoMinifyPass2/m1v/Spl" [ label="m1v" ];
  "Transition to\nssaoMinifyPass2/m1v/Spl" -> "ssaoMinifyPass2" [ label="m1v" ];
  "ssaoLinearisePass" [ shape=ellipse ];
  "Transition to\nssaoMinifyPass1/m0v/Spl" [ shape=box ];
  "ssaoLinearisePass" -> "Transition to\nssaoMinifyPass1/m0v/Spl" [ label="m0v" ];
  "Transition to\nssaoMinifyPass1/m0v/Spl" -> "ssaoMinifyPass1" [ label="m0v" ];
  "combinePass" [ shape=ellipse ];
  "Transition to\nfinalCombinePass/cv/Spl" [ shape=box ];
  "combinePass" -> "Transition to\nfinalCombinePass/cv/Spl" [ label="cv" ];
  "Transition to\nfinalCombinePass/cv/Spl" -> "finalCombinePass" [ label="cv" ];
  "accumulationPass" [ shape=ellipse ];
  "Transition to\ncombinePass/av/Spl" [ shape=box ];
  "accumulationPass" -> "Transition to\ncombinePass/av/Spl" [ label="av" ];
  "Transition to\ncombinePass/av/Spl" -> "combinePass" [ label="av" ];
  "Transition to\ncombinePass/rv/Spl" [ shape=box ];
  "accumulationPass" -> "Transition to\ncombinePass/rv/Spl" [ label="rv" ];
  "Transition to\ncombinePass/rv/Spl" -> "combinePass" [ label="rv" ];
  "Transition to\naccumulationPass/dtv/IRds" [ shape=box ];
  "geometryPass" -> "Transition to\naccumulationPass/dtv/IRds" [ label="dtv" ];
  "Transition to\naccumulationPass/dtv/IRds" -> "accumulationPass" [ label="dtv" ];
}
)";
				}
				else
				{
					ref = R"(digraph {
  "finalCombinePass" [ shape=ellipse ];
  "ambientPass" [ shape=ellipse ];
  "Transition to\nfinalCombinePass/ofv/Spl" [ shape=box ];
  "ambientPass" -> "Transition to\nfinalCombinePass/ofv/Spl" [ label="ofv" ];
  "Transition to\nfinalCombinePass/ofv/Spl" -> "finalCombinePass" [ label="ofv" ];
  "lightingPass" [ shape=ellipse ];
  "Transition to\nambientPass/dfv/Spl" [ shape=box ];
  "lightingPass" -> "Transition to\nambientPass/dfv/Spl" [ label="dfv" ];
  "Transition to\nambientPass/dfv/Spl" -> "ambientPass" [ label="dfv" ];
  "Transition to\nambientPass/spv/Spl" [ shape=box ];
  "lightingPass" -> "Transition to\nambientPass/spv/Spl" [ label="spv" ];
  "Transition to\nambientPass/spv/Spl" -> "ambientPass" [ label="spv" ];
  "geometryPass" [ shape=ellipse ];
  "Transition to\nlightingPass/d1v/Spl" [ shape=box ];
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
  "Transition to\nlightingPass/ldv/Spl" [ shape=box ];
  "geometryPass" -> "Transition to\nlightingPass/ldv/Spl" [ label="ldv" ];
  "Transition to\nlightingPass/ldv/Spl" -> "lightingPass" [ label="ldv" ];
  "ssaoBlurPass" [ shape=ellipse ];
  "Transition to\nambientPass/b1v/Spl" [ shape=box ];
  "ssaoBlurPass" -> "Transition to\nambientPass/b1v/Spl" [ label="b1v" ];
  "Transition to\nambientPass/b1v/Spl" -> "ambientPass" [ label="b1v" ];
  "ssaoRawPass" [ shape=ellipse ];
  "Transition to\nssaoBlurPass/rsv/Spl" [ shape=box ];
  "ssaoRawPass" -> "Transition to\nssaoBlurPass/rsv/Spl" [ label="rsv" ];
  "Transition to\nssaoBlurPass/rsv/Spl" -> "ssaoBlurPass" [ label="rsv" ];
  "ssaoMinifyPass3" [ shape=ellipse ];
  "Transition to\nssaoRawPass/lp/Spl3" [ shape=box ];
  "ssaoMinifyPass3" -> "Transition to\nssaoRawPass/lp/Spl3" [ label="m3v" ];
  "Transition to\nssaoRawPass/lp/Spl3" -> "ssaoRawPass" [ label="m3v" ];
  "ssaoMinifyPass2" [ shape=ellipse ];
  "Transition to\nssaoMinifyPass3/m2v/Spl" [ shape=box ];
  "ssaoMinifyPass2" -> "Transition to\nssaoMinifyPass3/m2v/Spl" [ label="m2v" ];
  "Transition to\nssaoMinifyPass3/m2v/Spl" -> "ssaoMinifyPass3" [ label="m2v" ];
  "ssaoMinifyPass1" [ shape=ellipse ];
  "Transition to\nssaoMinifyPass2/m1v/Spl" [ shape=box ];
  "ssaoMinifyPass1" -> "Transition to\nssaoMinifyPass2/m1v/Spl" [ label="m1v" ];
  "Transition to\nssaoMinifyPass2/m1v/Spl" -> "ssaoMinifyPass2" [ label="m1v" ];
  "ssaoLinearisePass" [ shape=ellipse ];
  "Transition to\nssaoMinifyPass1/m0v/Spl" [ shape=box ];
  "ssaoLinearisePass" -> "Transition to\nssaoMinifyPass1/m0v/Spl" [ label="m0v" ];
  "Transition to\nssaoMinifyPass1/m0v/Spl" -> "ssaoMinifyPass1" [ label="m0v" ];
}
)";
				}
			}
			else if constexpr ( EnableTransparent )
			{
				ref = R"(digraph {
  "finalCombinePass" [ shape=ellipse ];
  "ambientPass" [ shape=ellipse ];
  "Transition to\nfinalCombinePass/ofv/Spl" [ shape=box ];
  "ambientPass" -> "Transition to\nfinalCombinePass/ofv/Spl" [ label="ofv" ];
  "Transition to\nfinalCombinePass/ofv/Spl" -> "finalCombinePass" [ label="ofv" ];
  "lightingPass" [ shape=ellipse ];
  "Transition to\nambientPass/dfv/Spl" [ shape=box ];
  "lightingPass" -> "Transition to\nambientPass/dfv/Spl" [ label="dfv" ];
  "Transition to\nambientPass/dfv/Spl" -> "ambientPass" [ label="dfv" ];
  "Transition to\nambientPass/spv/Spl" [ shape=box ];
  "lightingPass" -> "Transition to\nambientPass/spv/Spl" [ label="spv" ];
  "Transition to\nambientPass/spv/Spl" -> "ambientPass" [ label="spv" ];
  "geometryPass" [ shape=ellipse ];
  "Transition to\nlightingPass/d1v/Spl" [ shape=box ];
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
  "Transition to\nlightingPass/ldv/Spl" [ shape=box ];
  "geometryPass" -> "Transition to\nlightingPass/ldv/Spl" [ label="ldv" ];
  "Transition to\nlightingPass/ldv/Spl" -> "lightingPass" [ label="ldv" ];
  "combinePass" [ shape=ellipse ];
  "Transition to\nfinalCombinePass/cv/Spl" [ shape=box ];
  "combinePass" -> "Transition to\nfinalCombinePass/cv/Spl" [ label="cv" ];
  "Transition to\nfinalCombinePass/cv/Spl" -> "finalCombinePass" [ label="cv" ];
  "accumulationPass" [ shape=ellipse ];
  "Transition to\ncombinePass/av/Spl" [ shape=box ];
  "accumulationPass" -> "Transition to\ncombinePass/av/Spl" [ label="av" ];
  "Transition to\ncombinePass/av/Spl" -> "combinePass" [ label="av" ];
  "Transition to\ncombinePass/rv/Spl" [ shape=box ];
  "accumulationPass" -> "Transition to\ncombinePass/rv/Spl" [ label="rv" ];
  "Transition to\ncombinePass/rv/Spl" -> "combinePass" [ label="rv" ];
  "Transition to\naccumulationPass/dtv/IRds" [ shape=box ];
  "geometryPass" -> "Transition to\naccumulationPass/dtv/IRds" [ label="dtv" ];
  "Transition to\naccumulationPass/dtv/IRds" -> "accumulationPass" [ label="dtv" ];
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
  "Transition to\nlightingPass/ldv/Spl" [ shape=box ];
  "geometryPass" [ shape=ellipse ];
  "geometryPass" -> "Transition to\nlightingPass/ldv/Spl" [ label="ldv" ];
  "Transition to\nlightingPass/ldv/Spl" -> "lightingPass" [ label="ldv" ];
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
  "geometryPass" [ shape=ellipse ];
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
  "Transition to\ncombinePass/rv/Spl" [ shape=box ];
  "accumulationPass" -> "Transition to\ncombinePass/rv/Spl" [ label="rv" ];
  "Transition to\ncombinePass/rv/Spl" -> "combinePass" [ label="rv" ];
  "Transition to\nfinalCombinePass/cv/Spl" [ shape=box ];
  "finalCombinePass" [ shape=ellipse ];
  "combinePass" -> "Transition to\nfinalCombinePass/cv/Spl" [ label="cv" ];
  "Transition to\nfinalCombinePass/cv/Spl" -> "finalCombinePass" [ label="cv" ];
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

	checkEqualSortedLines( stream, ref )
	testEnd()
}

TEST( RenderGraph, VarianceShadowMap )
{
	testBegin( "testVarianceShadowMap" )
	crg::ResourceHandler handler;
	crg::FrameGraph graph{ handler, testCounts.testName };

	auto & dirGroup = graph.createPassGroup( "Directional" );
	auto dirShadowMap = dirGroup.createImage( test::createImage( "dirShadowMap", crg::PixelFormat::eX8_D24_UNORM, 1u, 4u ) );
	auto dirVarianceMap = dirGroup.createImage( test::createImage( "dirVarianceMap", crg::PixelFormat::eR32G32_SFLOAT, 1u, 4u ) );
	auto buffer = dirGroup.createBuffer( test::createBuffer( "buffer" ) );
	auto bufferv = dirGroup.createView( test::createView( "bufferv", buffer ) );
	crg::AttachmentArray dirShadows;
	crg::AttachmentArray dirVariances;
	{
		auto intermediate = dirGroup.createImage( test::createImage( "dirIntermediate", crg::PixelFormat::eR32G32_SFLOAT ) );
		auto intermediatev = dirGroup.createView( test::createView( "dirIntermediatev", intermediate, crg::PixelFormat::eR32G32_SFLOAT ) );

		for ( uint32_t index = 0u; index < 4u; ++index )
		{
			auto shadowMapv = dirGroup.createView( test::createView( "dirShadowMapv" + std::to_string( index ), dirShadowMap, crg::PixelFormat::eX8_D24_UNORM, 0u, 1u, index ) );
			auto varianceMapv = dirGroup.createView( test::createView( "dirVarianceMapv" + std::to_string( index ), dirVarianceMap, crg::PixelFormat::eR32G32_SFLOAT, 0u, 1u, index ) );
			auto & shadowPass = dirGroup.createPass( "dirShadowPass" + std::to_string( index )
				, [&testCounts]( crg::FramePass const & framePass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
					return createDummy( testCounts
						, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
				} );
			auto buffera = shadowPass.addClearableOutputStorageBuffer( bufferv, 0u );
			auto shadowMapa = shadowPass.addOutputDepthTarget( shadowMapv );
			auto varianceMapa = shadowPass.addOutputColourTarget( varianceMapv );
			dirShadows.push_back( shadowMapa );

			auto & blurPassX = dirGroup.createPass( "dirBlurPassX" + std::to_string( index )
				, [&testCounts]( crg::FramePass const & framePass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
					return createDummy( testCounts
						, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader
						, checkSampledIsShaderReadOnly );
				} );
			blurPassX.addInputStorage( *buffera, 0u );
			blurPassX.addInputSampled( *varianceMapa, 1u );
			auto intermediatea = blurPassX.addOutputColourTarget( intermediatev );

			auto & blurPassY = dirGroup.createPass( "dirBlurPassY" + std::to_string( index )
				, [&testCounts]( crg::FramePass const & framePass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
					return createDummy( testCounts
						, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader
						, checkSampledIsShaderReadOnly );
				} );
			blurPassY.addInputStorage( *buffera, 0u );
			blurPassY.addInputSampled( *intermediatea, 1u );
			varianceMapa = blurPassY.addOutputColourTarget( varianceMapv );
			dirGroup.addGroupOutput( varianceMapv );
			dirVariances.push_back( varianceMapa );
		}
	}
	auto & pntGroup = graph.createPassGroup( "Point" );
	auto pntShadowMap = pntGroup.createImage( test::createImage( "pntShadowMap", crg::PixelFormat::eX8_D24_UNORM, 1u, 36u ) );
	auto pntVarianceMap = pntGroup.createImage( test::createImage( "pntVarianceMap", crg::PixelFormat::eR32G32_SFLOAT, 1u, 36u ) );
	crg::AttachmentArray pntShadows;
	crg::AttachmentArray pntVariances;
	{
		auto intermediate = pntGroup.createImage( test::createImage( "pntIntermediate", crg::PixelFormat::eR32G32_SFLOAT ) );
		auto intermediatev = pntGroup.createView( test::createView( "pntIntermediatev", intermediate, crg::PixelFormat::eR32G32_SFLOAT ) );

		for ( uint32_t index = 0u; index < 36u; ++index )
		{
			auto shadowMapv = pntGroup.createView( test::createView( "pntShadowMapv" + std::to_string( index ), pntShadowMap, crg::PixelFormat::eX8_D24_UNORM, 0u, 1u, index ) );
			auto varianceMapv = pntGroup.createView( test::createView( "pntVarianceMapv" + std::to_string( index ), pntVarianceMap, crg::PixelFormat::eR32G32_SFLOAT, 0u, 1u, index ) );
			auto & shadowPass = pntGroup.createPass( "pntShadowPass" + std::to_string( index )
				, [&testCounts]( crg::FramePass const & framePass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
					return createDummy( testCounts
						, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
				} );
			auto buffera = shadowPass.addClearableOutputStorageBuffer( bufferv, 0u );
			auto shadowMapa = shadowPass.addOutputDepthTarget( shadowMapv );
			auto varianceMapa = shadowPass.addOutputColourTarget( varianceMapv );
			pntShadows.push_back( shadowMapa );

			auto & blurPassX = pntGroup.createPass( "pntBlurPassX" + std::to_string( index )
				, [&testCounts]( crg::FramePass const & framePass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
					return createDummy( testCounts
						, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader
						, checkSampledIsShaderReadOnly );
				} );
			blurPassX.addInputStorage( *buffera, 0u );
			blurPassX.addInputSampled( *varianceMapa, 1u );
			auto intermediatea = blurPassX.addOutputColourTarget( intermediatev );

			auto & blurPassY = pntGroup.createPass( "pntBlurPassY" + std::to_string( index )
				, [&testCounts]( crg::FramePass const & framePass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
					return createDummy( testCounts
						, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader
						, checkSampledIsShaderReadOnly );
				} );
			blurPassY.addInputStorage( *buffera, 0u );
			blurPassY.addInputSampled( *intermediatea, 1u );
			varianceMapa = blurPassY.addOutputColourTarget( varianceMapv );
			pntVariances.push_back( varianceMapa );
			pntGroup.addGroupOutput( varianceMapv );
		}
	}
	auto & sptGroup = graph.createPassGroup( "Spot" );
	auto sptShadowMap = sptGroup.createImage( test::createImage( "sptShadowMap", crg::PixelFormat::eX8_D24_UNORM, 1u, 10u ) );
	auto sptVarianceMap = sptGroup.createImage( test::createImage( "pntVarianceMap", crg::PixelFormat::eR32G32_SFLOAT, 1u, 10u ) );
	crg::AttachmentArray sptShadows;
	crg::AttachmentArray sptVariances;
	{
		auto intermediate = sptGroup.createImage( test::createImage( "sptIntermediate", crg::PixelFormat::eR32G32_SFLOAT ) );
		auto intermediatev = sptGroup.createView( test::createView( "sptIntermediatev", intermediate, crg::PixelFormat::eR32G32_SFLOAT ) );

		for ( uint32_t index = 0u; index < 10u; ++index )
		{
			auto shadowMapv = sptGroup.createView( test::createView( "sptShadowMapv" + std::to_string( index ), sptShadowMap, crg::PixelFormat::eX8_D24_UNORM, 0u, 1u, index ) );
			auto varianceMapv = sptGroup.createView( test::createView( "sptVarianceMapv" + std::to_string( index ), sptVarianceMap, crg::PixelFormat::eR32G32_SFLOAT, 0u, 1u, index ) );
			auto & shadowPass = sptGroup.createPass( "sptShadowPass" + std::to_string( index )
				, [&testCounts]( crg::FramePass const & framePass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
					return createDummy( testCounts
						, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader
						, checkTargetColourIsShaderReadOnly );
				} );
			auto buffera = shadowPass.addClearableOutputStorageBuffer( bufferv, 0u );
			auto shadowMapa = shadowPass.addOutputDepthTarget( shadowMapv );
			auto varianceMapa = shadowPass.addOutputColourTarget( varianceMapv );
			sptShadows.push_back( shadowMapa );

			auto & blurPassX = sptGroup.createPass( "sptBlurPassX" + std::to_string( index )
				, [&testCounts]( crg::FramePass const & framePass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
					return createDummy( testCounts
						, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader
						, checkSampledIsShaderReadOnly );
				} );
			blurPassX.addInputStorage( *buffera, 0u );
			blurPassX.addInputSampled( *varianceMapa, 1u );
			auto intermediatea = blurPassX.addOutputColourTarget( intermediatev );

			auto & blurPassY = sptGroup.createPass( "sptBlurPassY" + std::to_string( index )
				, [&testCounts]( crg::FramePass const & framePass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
					return createDummy( testCounts
						, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader
						, checkSampledIsShaderReadOnly );
				} );
			blurPassY.addInputStorage( *buffera, 0u );
			blurPassY.addInputSampled( *intermediatea, 1u );
			varianceMapa = blurPassY.addOutputColourTarget( varianceMapv );
			sptVariances.push_back( varianceMapa );
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
	auto deptha = depthPrepass.addOutputDepthTarget ( depthv );

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
	backgroundPass.addInputDepthTarget( *deptha );
	auto coloura = backgroundPass.addOutputColourTarget( colourv );
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
	opaquePass.addInputSampled( *objGroup.mergeAttachments( dirShadows ), 0u );
	opaquePass.addInputSampled( *objGroup.mergeAttachments( dirVariances ), 1u );
	opaquePass.addInputSampled( *objGroup.mergeAttachments( pntShadows ), 2u );
	opaquePass.addInputSampled( *objGroup.mergeAttachments( pntVariances ), 3u );
	opaquePass.addInputSampled( *objGroup.mergeAttachments( sptShadows ), 4u );
	opaquePass.addInputSampled( *objGroup.mergeAttachments( sptVariances ), 5u );
	opaquePass.addInputDepthTarget( *deptha );
	coloura = opaquePass.addInOutColourTarget( *coloura );

	auto & transparentPass = objGroup.createPass( "transparentPass"
		, [&testCounts]( crg::FramePass const & framePass
			, crg::GraphContext & context
			, crg::RunnableGraph & runGraph )
		{
			return createDummy( testCounts
				, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
		} );
	transparentPass.addInputDepthTarget( *deptha );
	transparentPass.addInOutColourTarget( *coloura );

	crg::ResourcesCache cache{ handler };
	auto & context = getContext();
	VkDeviceMemory bufferMemory;
	cache.createBuffer( context, buffer, bufferMemory );
	cache.createBufferView( context, bufferv );
	VkDeviceMemory imageMemory;
	cache.createImage( context, depth, imageMemory );
	cache.createImageView( context, depthv );

	handler.createBuffer( context, buffer );
	handler.createBufferView( context, bufferv );

	auto runnable = graph.compile( context );
	test::checkRunnable( testCounts, runnable );
	testEnd()
}

TEST( RenderGraph, EnvironmentMap )
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
	crg::AttachmentArray colourViews;
	crg::AttachmentArray cubeViews;
	crg::AttachmentArray cubesViews;

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
		auto depthan = opaquePass.addOutputDepthTarget( depthvn );
		auto colouran = opaquePass.addOutputColourTarget( colourvn );
		auto cubean = opaquePass.addOutputColourTarget( cubevn );
		auto cubesan = opaquePass.addOutputColourTarget( cubesvn );

		auto & backgroundPass = graph.createPass( "EnvBackgroundPass" + strIndex
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		backgroundPass.addInputDepthTarget( *depthan );
		colouran = backgroundPass.addInOutColourTarget( *colouran );
		cubean = backgroundPass.addInOutColourTarget( *cubean );
		cubesan = backgroundPass.addInOutColourTarget( *cubesan );

		auto & transparentPass = graph.createPass( "EnvTransparentPass" + strIndex
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		transparentPass.addInputDepthTarget( *depthan );
		colourViews.push_back( transparentPass.addInOutColourTarget( *colouran ) );
		cubeViews.push_back( transparentPass.addInOutColourTarget( *cubean ) );
		cubesViews.push_back( transparentPass.addInOutColourTarget( *cubesan ) );
	}

	auto & mipsGen = graph.createPass( "EnvMips"
		, [&testCounts]( crg::FramePass const & framePass
			, crg::GraphContext & context
			, crg::RunnableGraph & runGraph )
		{
			return createDummy( testCounts
				, framePass, context, runGraph, crg::PipelineStageFlags::eTransfer );
		} );
	mipsGen.addInputTransfer( *graph.mergeAttachments( colourViews ) );
	mipsGen.addInputTransfer( *graph.mergeAttachments( cubeViews ) );
	mipsGen.addInputTransfer( *graph.mergeAttachments( cubesViews ) );
	mipsGen.addOutputTransferImage( colourv );
	mipsGen.addOutputTransferImage( cubev );
	mipsGen.addOutputTransferImage( cubesv );

	auto runnable = graph.compile( getContext() );
	test::checkRunnable( testCounts, runnable );
	testEnd()
}

TEST( RenderGraph, DisabledPasses )
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
	crg::AttachmentArray colourViews;
	crg::AttachmentArray cubeViews;
	crg::AttachmentArray cubesViews;

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
		auto depthan = opaquePass.addOutputDepthTarget( depthvn );
		auto colouran = opaquePass.addOutputColourTarget( colourvn );
		auto cubean = opaquePass.addOutputColourTarget( cubevn );
		auto cubesan = opaquePass.addOutputColourTarget( cubesvn );

		auto & backgroundPass = graph.createPass( "EnvBackgroundPass" + strIndex
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader
					, test::checkDummy, 0u, false );
			} );
		backgroundPass.addInputDepthTarget( *depthan );
		colouran = backgroundPass.addInOutColourTarget( *colouran );
		cubean = backgroundPass.addInOutColourTarget( *cubean );
		cubesan = backgroundPass.addInOutColourTarget( *cubesan );

		auto & transparentPass = graph.createPass( "EnvTransparentPass" + strIndex
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader
					, test::checkDummy, 0u, false );
			} );
		transparentPass.addInputDepthTarget( *depthan );
		colourViews.push_back( transparentPass.addInOutColourTarget( *colouran ) );
		cubeViews.push_back( transparentPass.addInOutColourTarget( *cubean ) );
		cubesViews.push_back( transparentPass.addInOutColourTarget( *cubesan ) );
	}

	auto & mipsGen = graph.createPass( "EnvMips"
		, [&testCounts]( crg::FramePass const & framePass
			, crg::GraphContext & context
			, crg::RunnableGraph & runGraph )
		{
			return createDummy( testCounts
				, framePass, context, runGraph, crg::PipelineStageFlags::eTransfer );
		} );
	mipsGen.addInputTransfer( *graph.mergeAttachments( colourViews ) );
	mipsGen.addInputTransfer( *graph.mergeAttachments( cubeViews ) );
	mipsGen.addInputTransfer( *graph.mergeAttachments( cubesViews ) );
	mipsGen.addOutputTransferImage( colourv );
	mipsGen.addOutputTransferImage( cubev );
	mipsGen.addOutputTransferImage( cubesv );

	auto runnable = graph.compile( getContext() );
	test::checkRunnable( testCounts, runnable );
	testEnd()
}

testSuiteMain()
