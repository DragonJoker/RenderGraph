#include "Common.hpp"

#include <RenderGraph/RenderGraph.hpp>
#include <RenderGraph/ImageData.hpp>

#include <sstream>

namespace
{
	void testNoPass( test::TestCounts & testCounts )
	{
		testBegin( "testNoPass" );
		crg::RenderGraph graph;
		checkThrow( graph.compile() );

		auto rt = graph.createImage( test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtv = graph.createView( test::createView( rt, VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtAttach = crg::Attachment::createColour( "RT"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, rtv );
		crg::RenderPass pass
		{
			"pass1C",
			{},
			{ rtAttach },
		};

		checkNoThrow( graph.add( pass ) );
		checkNoThrow( graph.remove( pass ) );
		checkThrow( graph.compile() );
		testEnd();
	}

	void testOnePass( test::TestCounts & testCounts )
	{
		testBegin( "testOnePass" );
		crg::RenderGraph graph;
		auto rt = graph.createImage( test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtv = graph.createView( test::createView( rt, VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtAttach = crg::Attachment::createColour( "RT"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, rtv );
		crg::RenderPass pass
		{
			"pass1C",
			{},
			{ rtAttach },
		};

		checkNoThrow( graph.add( pass ) );
		check( graph.compile() );
		std::stringstream stream;
		test::display( testCounts, stream, graph );
		std::string ref = R"(digraph render {
}
)";
		checkEqualStr( stream.str(), ref );
		testEnd();
	}

	void testDuplicateName( test::TestCounts & testCounts )
	{
		testBegin( "testDuplicateName" );
		crg::RenderGraph graph;
		auto rt = graph.createImage( test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtv = graph.createView( test::createView( rt, VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtAttach = crg::Attachment::createColour( "RT"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, rtv );
		crg::RenderPass pass
		{
			"pass1C",
			{},
			{ rtAttach },
		};
		checkNoThrow( graph.add( pass ) );
		checkThrow( graph.add( pass ) );
		checkNoThrow( graph.remove( pass ) );
		testEnd();
	}

	void testWrongRemove( test::TestCounts & testCounts )
	{
		testBegin( "testWrongRemove" );
		crg::RenderGraph graph;
		auto rt = graph.createImage( test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtv = graph.createView( test::createView( rt, VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtAttach = crg::Attachment::createColour( "RT"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, rtv );
		crg::RenderPass pass1
		{
			"pass1C",
			{},
			{ rtAttach },
		};
		crg::RenderPass pass2
		{
			"pass2C",
			{},
			{ rtAttach },
		};
		checkNoThrow( graph.add( pass1 ) );
		checkThrow( graph.remove( pass2 ) );
		checkNoThrow( graph.remove( pass1 ) );
		testEnd();
	}

	void testOneDependency( test::TestCounts & testCounts )
	{
		testBegin( "testOneDependency" );
		crg::RenderGraph graph;
		auto rt = graph.createImage( test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtv = graph.createView( test::createView( rt, VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtAttach = crg::Attachment::createColour( "RT"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, rtv );
		crg::RenderPass pass1
		{
			"pass1C",
			{},
			{ rtAttach },
		};
		checkNoThrow( graph.add( pass1 ) );

		auto inAttach = crg::Attachment::createInput( "IN"
			, rtv );
		auto out = graph.createImage( test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto outv = graph.createView( test::createView( out, VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto outAttach = crg::Attachment::createColour( "OUT"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, outv );
		crg::RenderPass pass2
		{
			"pass2C",
			{ inAttach },
			{ outAttach },
		};
		checkNoThrow( graph.add( pass2 ) );

		check( graph.compile() );
		std::stringstream stream;
		test::display( testCounts, stream, graph );
		std::string ref = R"(digraph render {
    pass1C -> pass2C[ label="RT" ];
}
)";
		checkEqualStr( stream.str(), ref );
		testEnd();
	}

	void testChainedDependencies( test::TestCounts & testCounts )
	{
		testBegin( "testChainedDependencies" );
		crg::RenderGraph graph;
		auto d0 = graph.createImage( test::createImage( VK_FORMAT_R32G32B32_SFLOAT ) );
		auto d0v = graph.createView( test::createView( d0, VK_FORMAT_R32G32B32_SFLOAT, 1u ) );
		auto d0tAttach = crg::Attachment::createColour( "D0Target"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, d0v );
		crg::RenderPass pass0
		{
			"pass0",
			{},
			{ d0tAttach },
		};
		checkNoThrow( graph.add( pass0 ) );

		auto d0sAttach = crg::Attachment::createInput( "D0Sampled"
			, d0v );
		auto d1 = graph.createImage( test::createImage( VK_FORMAT_R32G32B32_SFLOAT ) );
		auto d1v = graph.createView( test::createView( d1, VK_FORMAT_R32G32B32_SFLOAT, 1u ) );
		auto d1tAttach = crg::Attachment::createColour( "D1Target"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, d1v );
		crg::RenderPass pass1
		{
			"pass1",
			{ d0sAttach },
			{ d1tAttach },
		};
		checkNoThrow( graph.add( pass1 ) );

		auto d1sAttach = crg::Attachment::createInput( "D1Sampled"
			, d1v );
		auto d2 = graph.createImage( test::createImage( VK_FORMAT_R32G32B32_SFLOAT ) );
		auto d2v = graph.createView( test::createView( d2, VK_FORMAT_R32G32B32_SFLOAT, 1u ) );
		auto d2tAttach = crg::Attachment::createColour( "D2Target"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, d2v );
		crg::RenderPass pass2
		{
			"pass2",
			{ d1sAttach },
			{ d2tAttach },
		};
		checkNoThrow( graph.add( pass2 ) );

		check( graph.compile() );
		std::stringstream stream;
		test::display( testCounts, stream, graph );
		std::string ref = R"(digraph render {
    pass0 -> pass1[ label="D0Target" ];
    pass1 -> pass2[ label="D1Target" ];
}
)";
		checkEqualStr( stream.str(), ref );
		testEnd();
	}

	void testSharedDependencies( test::TestCounts & testCounts )
	{
		testBegin( "testSharedDependencies" );
		crg::RenderGraph graph;
		auto d = graph.createImage( test::createImage( VK_FORMAT_D32_SFLOAT_S8_UINT ) );
		auto dstv = graph.createView( test::createView( d, VK_FORMAT_D32_SFLOAT_S8_UINT, 1u ) );
		auto dstAttach1 = crg::Attachment::createDepthStencil( "DSTarget1"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, dstv );
		auto dstAttach2 = crg::Attachment::createDepthStencil( "DSTarget2"
			, VK_ATTACHMENT_LOAD_OP_LOAD
			, VK_ATTACHMENT_STORE_OP_STORE
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, dstv );
		auto dssv = graph.createView( test::createView( d, VK_FORMAT_R32_SFLOAT, 1u ) );
		auto dssAttach = crg::Attachment::createDepthStencil( "DSSampled"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, dssv );

		auto d0 = graph.createImage( test::createImage( VK_FORMAT_R32G32B32_SFLOAT ) );
		auto d0v = graph.createView( test::createView( d0, VK_FORMAT_R32G32B32_SFLOAT, 1u ) );
		auto d0tAttach = crg::Attachment::createColour( "D0Target"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, d0v );
		crg::RenderPass pass0
		{
			"pass0",
			{},
			{ d0tAttach },
			{ dstAttach1 },
		};
		checkNoThrow( graph.add( pass0 ) );

		auto d0sAttach = crg::Attachment::createInput( "D0Sampled"
			, d0v );
		auto d1 = graph.createImage( test::createImage( VK_FORMAT_R32G32B32_SFLOAT ) );
		auto d1v = graph.createView( test::createView( d1, VK_FORMAT_R32G32B32_SFLOAT, 1u ) );
		auto d1tAttach = crg::Attachment::createColour( "D1Target"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, d1v );
		crg::RenderPass pass1
		{
			"pass1",
			{ d0sAttach },
			{ d1tAttach },
			{ dstAttach2 },
		};
		checkNoThrow( graph.add( pass1 ) );

		auto d1sAttach = crg::Attachment::createInput( "D1Sampled"
			, d1v );
		auto d2 = graph.createImage( test::createImage( VK_FORMAT_R32G32B32_SFLOAT ) );
		auto d2v = graph.createView( test::createView( d2, VK_FORMAT_R32G32B32_SFLOAT, 1u ) );
		auto d2tAttach = crg::Attachment::createColour( "D2Target"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, d2v );
		crg::RenderPass pass2
		{
			"pass2",
			{ d1sAttach },
			{ d2tAttach },
		};
		checkNoThrow( graph.add( pass2 ) );

		check( graph.compile() );
		std::stringstream stream;
		test::display( testCounts, stream, graph );
		std::string ref = R"(digraph render {
    pass0 -> pass1[ label="D0Target\nDSTarget1" ];
    pass1 -> pass2[ label="D1Target" ];
}
)";
		checkEqualStr( stream.str(), ref );
		testEnd();
	}

	void test2MipDependencies( test::TestCounts & testCounts )
	{
		testBegin( "testMip2Dependencies" );
		crg::RenderGraph graph;
		auto lp = graph.createImage( test::createImage( VK_FORMAT_R32G32B32_SFLOAT, 3u ) );
		auto m0v = graph.createView( test::createView( lp, VK_FORMAT_R32G32B32_SFLOAT, 0u ) );

		auto m0sAttach = crg::Attachment::createInput( "SSAOLineariseSampled"
			, m0v );
		auto m1v = graph.createView( test::createView( lp, VK_FORMAT_R32G32B32_SFLOAT, 1u ) );
		auto m1tAttach = crg::Attachment::createColour( "SSAOMinify1Target"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, m1v );
		crg::RenderPass ssaoMinifyPass1
		{
			"ssaoMinifyPass1",
			{ m0sAttach },
			{ m1tAttach },
		};
		checkNoThrow( graph.add( ssaoMinifyPass1 ) );

		auto m1sAttach = crg::Attachment::createInput( "SSAOMinify1Sampled"
			, m1v );
		auto m2v = graph.createView( test::createView( lp, VK_FORMAT_R32G32B32_SFLOAT, 2u ) );
		auto m2tAttach = crg::Attachment::createColour( "SSAOMinify2Target"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, m2v );
		crg::RenderPass ssaoMinifyPass2
		{
			"ssaoMinifyPass2",
			{ m1sAttach },
			{ m2tAttach },
		};
		checkNoThrow( graph.add( ssaoMinifyPass2 ) );

		check( graph.compile() );
		std::stringstream stream;
		test::display( testCounts, stream, graph );
		std::string ref = R"(digraph render {
    ssaoMinifyPass1 -> ssaoMinifyPass2[ label="SSAOMinify1Target" ];
}
)";
		checkEqualStr( stream.str(), ref );
		testEnd();
	}

	void test3MipDependencies( test::TestCounts & testCounts )
	{
		testBegin( "testMip3Dependencies" );
		crg::RenderGraph graph;
		auto lp = graph.createImage( test::createImage( VK_FORMAT_R32G32B32_SFLOAT, 4u ) );
		auto m0v = graph.createView( test::createView( lp, VK_FORMAT_R32G32B32_SFLOAT, 0u ) );

		auto m0sAttach = crg::Attachment::createInput( "SSAOLineariseSampled"
			, m0v );
		auto m1v = graph.createView( test::createView( lp, VK_FORMAT_R32G32B32_SFLOAT, 1u ) );
		auto m1tAttach = crg::Attachment::createColour( "SSAOMinify1Target"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, m1v );
		crg::RenderPass ssaoMinifyPass1
		{
			"ssaoMinifyPass1",
			{ m0sAttach },
			{ m1tAttach },
		};
		checkNoThrow( graph.add( ssaoMinifyPass1 ) );

		auto m1sAttach = crg::Attachment::createInput( "SSAOMinify1Sampled"
			, m1v );
		auto m2v = graph.createView( test::createView( lp, VK_FORMAT_R32G32B32_SFLOAT, 2u ) );
		auto m2tAttach = crg::Attachment::createColour( "SSAOMinify2Target"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, m2v );
		crg::RenderPass ssaoMinifyPass2
		{
			"ssaoMinifyPass2",
			{ m1sAttach },
			{ m2tAttach },
		};
		checkNoThrow( graph.add( ssaoMinifyPass2 ) );

		auto m2sAttach = crg::Attachment::createInput( "SSAOMinify2Sampled"
			, m2v );
		auto m3v = graph.createView( test::createView( lp, VK_FORMAT_R32G32B32_SFLOAT, 3u ) );
		auto m3tAttach = crg::Attachment::createColour( "SSAOMinify3Target"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, m3v );
		crg::RenderPass ssaoMinifyPass3
		{
			"ssaoMinifyPass3",
			{ m2sAttach },
			{ m3tAttach },
		};
		checkNoThrow( graph.add( ssaoMinifyPass3 ) );

		check( graph.compile() );
		std::stringstream stream;
		test::display( testCounts, stream, graph );
		std::string ref = R"(digraph render {
    ssaoMinifyPass1 -> ssaoMinifyPass2[ label="SSAOMinify1Target" ];
    ssaoMinifyPass2 -> ssaoMinifyPass3[ label="SSAOMinify2Target" ];
}
)";
		checkEqualStr( stream.str(), ref );
		testEnd();
	}

	crg::Attachment buildSsaoPass( test::TestCounts & testCounts
		, crg::RenderPass const & previous
		, crg::Attachment const & dsAttach
		, crg::Attachment const & d2sAttach
		, crg::RenderGraph & graph )
	{
		auto lp = graph.createImage( test::createImage( VK_FORMAT_R32_SFLOAT, 4u ) );
		auto m0v = graph.createView( test::createView( lp, VK_FORMAT_R32_SFLOAT, 0u ) );
		auto m0tAttach = crg::Attachment::createColour( "SSAOLineariseTarget"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, m0v );
		crg::RenderPass ssaoLinearisePass
		{
			"ssaoLinearisePass",
			{ dsAttach },
			{ m0tAttach },
		};
		checkNoThrow( graph.add( ssaoLinearisePass ) );

		auto m0sAttach = crg::Attachment::createInput( "SSAOMinify0Sampled"
			, m0v );
		auto m1v = graph.createView( test::createView( lp, VK_FORMAT_R32_SFLOAT, 1u ) );
		auto m1tAttach = crg::Attachment::createColour( "SSAOMinify1Target"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, m1v );
		crg::RenderPass ssaoMinifyPass1
		{
			"ssaoMinifyPass1",
			{ m0sAttach },
			{ m1tAttach },
		};
		checkNoThrow( graph.add( ssaoMinifyPass1 ) );

		auto m1sAttach = crg::Attachment::createInput( "SSAOMinify1Sampled"
			, m1v );
		auto m2v = graph.createView( test::createView( lp, VK_FORMAT_R32_SFLOAT, 2u ) );
		auto m2tAttach = crg::Attachment::createColour( "SSAOMinify2Target"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, m2v );
		crg::RenderPass ssaoMinifyPass2
		{
			"ssaoMinifyPass2",
			{ m1sAttach },
			{ m2tAttach },
		};
		checkNoThrow( graph.add( ssaoMinifyPass2 ) );

		auto m2sAttach = crg::Attachment::createInput( "SSAOMinify2Sampled"
			, m2v );
		auto m3v = graph.createView( test::createView( lp, VK_FORMAT_R32_SFLOAT, 3u ) );
		auto m3tAttach = crg::Attachment::createColour( "SSAOMinify3Target"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, m3v );
		auto m3sAttach = crg::Attachment::createInput( "SSAOMinify3Sampled"
			, m3v );
		crg::RenderPass ssaoMinifyPass3
		{
			"ssaoMinifyPass3",
			{ m2sAttach },
			{ m3tAttach },
		};
		checkNoThrow( graph.add( ssaoMinifyPass3 ) );

		auto mv = graph.createView( test::createView( lp, VK_FORMAT_R32_SFLOAT, 0u, 4u ) );
		auto msAttach = crg::Attachment::createInput( "SSAOMinifySampled"
			, mv );
		auto rs = graph.createImage( test::createImage( VK_FORMAT_R32_SFLOAT ) );
		auto rsv = graph.createView( test::createView( rs, VK_FORMAT_R32_SFLOAT ) );
		auto rstAttach = crg::Attachment::createColour( "SSAORawTarget"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, rsv );
		crg::RenderPass ssaoRawPass
		{
			"ssaoRawPass",
			{ msAttach, d2sAttach },
			{ rstAttach },
		};
		checkNoThrow( graph.add( ssaoRawPass ) );

		auto rssAttach = crg::Attachment::createInput( "SSAORawSampled"
			, rsv );
		auto bl = graph.createImage( test::createImage( VK_FORMAT_R32_SFLOAT ) );
		auto blv = graph.createView( test::createView( bl, VK_FORMAT_R32_SFLOAT ) );
		auto bltAttach = crg::Attachment::createColour( "SSAOBlurTarget"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, blv );
		crg::RenderPass ssaoBlurPass
		{
			"ssaoBlurPass",
			{ rssAttach, d2sAttach },
			{ bltAttach },
		};
		checkNoThrow( graph.add( ssaoBlurPass ) );

		return crg::Attachment::createInput( "SSAOBlurSampled"
			, blv );
	}

	void testSsaoPass( test::TestCounts & testCounts )
	{
		testBegin( "testSsaoPass" );
		crg::RenderGraph graph;
		auto d = graph.createImage( test::createImage( VK_FORMAT_D32_SFLOAT_S8_UINT ) );
		auto dtv = graph.createView( test::createView( d, VK_FORMAT_D32_SFLOAT_S8_UINT ) );
		auto dt1Attach = crg::Attachment::createDepthStencil( "DepthTarget1"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, dtv );
		auto dt2Attach = crg::Attachment::createDepthStencil( "DepthTarget2"
			, VK_ATTACHMENT_LOAD_OP_LOAD
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, dtv );
		crg::RenderPass depthPrepass
		{
			"depthPrepass",
			{},
			{},
			{ dt1Attach },
		};
		checkNoThrow( graph.add( depthPrepass ) );

		auto dsv = graph.createView( test::createView( d, VK_FORMAT_R32_SFLOAT ) );
		auto dsAttach = crg::Attachment::createInput( "DepthSampled"
			, dsv );
		auto v = graph.createImage( test::createImage( VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto vv = graph.createView( test::createView( v, VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto vtAttach = crg::Attachment::createDepthStencil( "VelocityTarget"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, vv );
		auto d1 = graph.createImage( test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto d1v = graph.createView( test::createView( d1, VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto d1tAttach = crg::Attachment::createColour( "Data1Target"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, d1v );
		auto d2 = graph.createImage( test::createImage( VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto d2v = graph.createView( test::createView( d2, VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto d2tAttach = crg::Attachment::createColour( "Data2Target"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, d2v );
		auto d3 = graph.createImage( test::createImage( VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto d3v = graph.createView( test::createView( d3, VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto d3tAttach = crg::Attachment::createColour( "Data3Target"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, d3v );
		auto d4 = graph.createImage( test::createImage( VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto d4v = graph.createView( test::createView( d4, VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto d4tAttach = crg::Attachment::createColour( "Data4Target"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, d4v );
		crg::RenderPass geometryPass
		{
			"geometryPass",
			{},
			{ d1tAttach, d2tAttach, d3tAttach, d4tAttach, vtAttach },
			{ dt2Attach },
		};
		checkNoThrow( graph.add( geometryPass ) );

		auto d2sAttach = crg::Attachment::createInput( "Data2Sampled"
			, d2v );
		auto ssaosAttach = buildSsaoPass( testCounts
			, geometryPass
			, dsAttach
			, d2sAttach
			, graph );

		auto d1sAttach = crg::Attachment::createInput( "Data1Sampled"
			, d1v );
		auto d3sAttach = crg::Attachment::createInput( "Data3Sampled"
			, d3v );
		auto d4sAttach = crg::Attachment::createInput( "Data4Sampled"
			, d4v );
		auto of = graph.createImage( test::createImage( VK_FORMAT_R32G32B32_SFLOAT ) );
		auto ofv = graph.createView( test::createView( of, VK_FORMAT_R32G32B32_SFLOAT ) );
		auto oftAttach = crg::Attachment::createColour( "OutputTarget"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, ofv );
		crg::RenderPass ambientPass
		{
			"ambientPass",
			{ dsAttach, d1sAttach, d2sAttach, d3sAttach, d4sAttach, ssaosAttach },
			{ oftAttach },
		};
		checkNoThrow( graph.add( ambientPass ) );

		check( graph.compile() );
		std::stringstream stream;
		test::display( testCounts, stream, graph );
		std::string ref = R"(digraph render {
    depthPrepass -> geometryPass[ label="DepthTarget1" ];
    geometryPass -> ssaoRawPass[ label="Data2Target" ];
    ssaoRawPass -> ssaoBlurPass[ label="SSAORawTarget" ];
    ssaoBlurPass -> ambientPass[ label="SSAOBlurTarget" ];
    geometryPass -> ssaoBlurPass[ label="Data2Target" ];
    geometryPass -> ambientPass[ label="Data2Target\nData1Target\nData3Target\nData4Target" ];
    depthPrepass -> ssaoLinearisePass[ label="DepthTarget1" ];
    ssaoLinearisePass -> ssaoMinifyPass1[ label="SSAOLineariseTarget" ];
    ssaoMinifyPass1 -> ssaoMinifyPass2[ label="SSAOMinify1Target" ];
    ssaoMinifyPass2 -> ssaoMinifyPass3[ label="SSAOMinify2Target" ];
    ssaoMinifyPass3 -> ssaoRawPass[ label="SSAOMinify3Target" ];
    ssaoMinifyPass2 -> ssaoRawPass[ label="SSAOMinify2Target" ];
    ssaoMinifyPass1 -> ssaoRawPass[ label="SSAOMinify1Target" ];
    ssaoLinearisePass -> ssaoRawPass[ label="SSAOLineariseTarget" ];
    depthPrepass -> ambientPass[ label="DepthTarget1" ];
}
)";
			checkEqualStr( stream.str(), ref );
		testEnd();
	}

	template< bool EnableSsao >
	crg::Attachment buildDeferred( test::TestCounts & testCounts
		, crg::RenderPass const & depthPrepass
		, crg::Attachment const & dsAttach
		, crg::Attachment const & dtAttach
		, crg::Attachment const & vtAttach
		, crg::RenderGraph & graph )
	{
		auto d1 = graph.createImage( test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto d1v = graph.createView( test::createView( d1, VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto d1tAttach = crg::Attachment::createColour( "Data1Target"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, d1v );
		auto d2 = graph.createImage( test::createImage( VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto d2v = graph.createView( test::createView( d2, VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto d2tAttach = crg::Attachment::createColour( "Data2Target"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, d2v );
		auto d3 = graph.createImage( test::createImage( VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto d3v = graph.createView( test::createView( d3, VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto d3tAttach = crg::Attachment::createColour( "Data3Target"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, d3v );
		auto d4 = graph.createImage( test::createImage( VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto d4v = graph.createView( test::createView( d4, VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto d4tAttach = crg::Attachment::createColour( "Data4Target"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, d4v );
		crg::RenderPass geometryPass
		{
			"geometryPass",
			{},
			{ d1tAttach, d2tAttach, d3tAttach, d4tAttach, vtAttach },
			{ dtAttach },
		};
		checkNoThrow( graph.add( geometryPass ) );

		auto d1sAttach = crg::Attachment::createInput( "Data1Sampled"
			, d1v );
		auto d2sAttach = crg::Attachment::createInput( "Data2Sampled"
			, d2v );
		auto d3sAttach = crg::Attachment::createInput( "Data3Sampled"
			, d3v );
		auto d4sAttach = crg::Attachment::createInput( "Data4Sampled"
			, d4v );
		auto df = graph.createImage( test::createImage( VK_FORMAT_R32G32B32_SFLOAT ) );
		auto dfv = graph.createView( test::createView( df, VK_FORMAT_R32G32B32_SFLOAT ) );
		auto dftAttach = crg::Attachment::createColour( "DiffuseTarget"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, dfv );
		auto sp = graph.createImage( test::createImage( VK_FORMAT_R32G32B32_SFLOAT ) );
		auto spv = graph.createView( test::createView( sp, VK_FORMAT_R32G32B32_SFLOAT ) );
		auto sptAttach = crg::Attachment::createColour( "SpecularTarget"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, spv );
		crg::RenderPass lightingPass
		{
			"lightingPass",
			{ dsAttach, d1sAttach, d2sAttach, d3sAttach, d4sAttach },
			{ dftAttach, sptAttach },
		};
		checkNoThrow( graph.add( lightingPass ) );

		auto dfsAttach = crg::Attachment::createInput( "DiffuseSampled"
			, dfv );
		auto spsAttach = crg::Attachment::createInput( "SpecularSampled"
			, spv );
		auto of = graph.createImage( test::createImage( VK_FORMAT_R32G32B32_SFLOAT ) );
		auto ofv = graph.createView( test::createView( of, VK_FORMAT_R32G32B32_SFLOAT ) );
		auto oftAttach = crg::Attachment::createColour( "OutputTarget"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, ofv );

		if constexpr ( EnableSsao )
		{
			auto ssaosAttach = buildSsaoPass( testCounts
				, geometryPass
				, dsAttach
				, d2sAttach
				, graph );
			crg::RenderPass ambientPass
			{
				"ambientPass",
				{ dsAttach, d1sAttach, d2sAttach, d3sAttach, d4sAttach, dfsAttach, spsAttach, ssaosAttach },
				{ oftAttach },
			};
			checkNoThrow( graph.add( ambientPass ) );
		}
		else
		{
			crg::RenderPass ambientPass
			{
				"ambientPass",
				{ dsAttach, d1sAttach, d2sAttach, d3sAttach, d4sAttach, dfsAttach, spsAttach },
				{ oftAttach },
			};
			checkNoThrow( graph.add( ambientPass ) );
		}

		return crg::Attachment::createInput( "OutputSampled"
			, ofv );
	}

	crg::Attachment buildWeightedBlended( test::TestCounts & testCounts
		, crg::RenderPass const & depthPrepass
		, crg::Attachment const & dsAttach
		, crg::Attachment const & dtAttach
		, crg::Attachment const & vtAttach
		, crg::RenderGraph & graph )
	{
		auto a = graph.createImage( test::createImage( VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto av = graph.createView( test::createView( a, VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto atAttach = crg::Attachment::createColour( "AccumulationTarget"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, av );
		auto r = graph.createImage( test::createImage( VK_FORMAT_R16_SFLOAT ) );
		auto rv = graph.createView( test::createView( r, VK_FORMAT_R16_SFLOAT ) );
		auto rtAttach = crg::Attachment::createColour( "RevealageTarget"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, rv );
		crg::RenderPass accumulationPass
		{
			"accumulationPass",
			{},
			{ atAttach, rtAttach, vtAttach },
			{ dtAttach },
		};
		checkNoThrow( graph.add( accumulationPass ) );

		auto asAttach = crg::Attachment::createInput( "AccumulationSampled"
			, av );
		auto rsAttach = crg::Attachment::createInput( "RevealageSampled"
			, rv );
		auto c = graph.createImage( test::createImage( VK_FORMAT_R32G32B32_SFLOAT ) );
		auto cv = graph.createView( test::createView( c, VK_FORMAT_R32G32B32_SFLOAT ) );
		auto ctAttach = crg::Attachment::createColour( "CombineTarget"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, cv );
		crg::RenderPass combinePass
		{
			"combinePass",
			{ dsAttach, asAttach, rsAttach },
			{ ctAttach },
		};
		checkNoThrow( graph.add( combinePass ) );

		return crg::Attachment::createInput( "CombineSampled"
			, cv );
	}

	template< bool EnableOpaque
		, bool EnableSsao
		, bool EnableTransparent >
	void testFullPass( test::TestCounts & testCounts )
	{
		testBegin( "testFullPass"
			+ ( EnableOpaque ? std::string{ "Opaque" } : std::string{} )
			+ ( EnableSsao ? std::string{ "Ssao" } : std::string{} )
			+ ( EnableTransparent ? std::string{ "Transparent" } : std::string{} ) );
		crg::RenderGraph graph;
		auto d = graph.createImage( test::createImage( VK_FORMAT_D32_SFLOAT_S8_UINT ) );
		auto dtv = graph.createView( test::createView( d, VK_FORMAT_D32_SFLOAT_S8_UINT ) );
		auto dt1Attach = crg::Attachment::createDepthStencil( "DepthTarget1"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, dtv );
		auto dt2Attach = crg::Attachment::createDepthStencil( "DepthTarget2"
			, VK_ATTACHMENT_LOAD_OP_LOAD
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, dtv );
		crg::RenderPass depthPrepass
		{
			"depthPrepass",
			{},
			{},
			{ dt1Attach },
		};
		checkNoThrow( graph.add( depthPrepass ) );

		auto dsv = graph.createView( test::createView( d, VK_FORMAT_R32_SFLOAT ) );
		auto dsAttach = crg::Attachment::createInput( "DepthSampled"
			, dsv );
		auto o = graph.createImage( test::createImage( VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto otv = graph.createView( test::createView( o, VK_FORMAT_R16G16B16A16_SFLOAT ) );
		auto otAttach = crg::Attachment::createDepthStencil( "FinalCombineTarget"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, otv );

		if constexpr ( EnableOpaque )
		{
			auto v = graph.createImage( test::createImage( VK_FORMAT_R16G16B16A16_SFLOAT ) );
			auto vv = graph.createView( test::createView( v, VK_FORMAT_R16G16B16A16_SFLOAT ) );
			auto vt1Attach = crg::Attachment::createDepthStencil( "VelocityTarget1"
				, VK_ATTACHMENT_LOAD_OP_CLEAR
				, VK_ATTACHMENT_STORE_OP_STORE
				, VK_ATTACHMENT_LOAD_OP_DONT_CARE
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, vv );
			auto dcsAttach = buildDeferred< EnableSsao >( testCounts
				, depthPrepass
				, dsAttach
				, dt2Attach
				, vt1Attach
				, graph );

			if constexpr ( EnableTransparent )
			{
				auto vt2Attach = crg::Attachment::createDepthStencil( "VelocityTarget2"
					, VK_ATTACHMENT_LOAD_OP_LOAD
					, VK_ATTACHMENT_STORE_OP_STORE
					, VK_ATTACHMENT_LOAD_OP_DONT_CARE
					, VK_ATTACHMENT_STORE_OP_DONT_CARE
					, vv );
				auto wbcsAttach = buildWeightedBlended( testCounts
					, depthPrepass
					, dsAttach
					, dt2Attach
					, vt2Attach
					, graph );

				auto vsAttach = crg::Attachment::createInput( "VelocitySampled"
					, vv );
				crg::RenderPass finalCombinePass
				{
					"finalCombinePass",
					{ dcsAttach, wbcsAttach, vsAttach },
					{ otAttach },
				};
				checkNoThrow( graph.add( finalCombinePass ) );
			}
			else
			{
				auto vsAttach = crg::Attachment::createInput( "VelocitySampled"
					, vv );
				crg::RenderPass finalCombinePass
				{
					"finalCombinePass",
					{ dcsAttach, vsAttach },
					{ otAttach },
				};
				checkNoThrow( graph.add( finalCombinePass ) );
			}
			
		}
		else if constexpr ( EnableTransparent )
		{
			auto v = graph.createImage( test::createImage( VK_FORMAT_R16G16B16A16_SFLOAT ) );
			auto vv = graph.createView( test::createView( v, VK_FORMAT_R16G16B16A16_SFLOAT ) );
			auto vt1Attach = crg::Attachment::createDepthStencil( "VelocityTarget1"
				, VK_ATTACHMENT_LOAD_OP_CLEAR
				, VK_ATTACHMENT_STORE_OP_STORE
				, VK_ATTACHMENT_LOAD_OP_DONT_CARE
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, vv );
			auto wbcsAttach = buildWeightedBlended( testCounts
				, depthPrepass
				, dsAttach
				, dt2Attach
				, vt1Attach
				, graph );

			auto vsAttach = crg::Attachment::createInput( "VelocitySampled"
				, vv );
			crg::RenderPass finalCombinePass
			{
				"finalCombinePass",
				{ wbcsAttach, vsAttach },
				{ otAttach },
			};
			checkNoThrow( graph.add( finalCombinePass ) );
		}
		else
		{
			crg::RenderPass finalCombinePass
			{
				"finalCombinePass",
				{ dsAttach },
				{ otAttach },
			};
			checkNoThrow( graph.add( finalCombinePass ) );
		}
		

		check( graph.compile() );
		std::stringstream stream;
		test::display( testCounts, stream, graph );

		if constexpr ( EnableOpaque )
		{
			if constexpr ( EnableSsao )
			{
				if constexpr ( EnableTransparent )
				{
					std::string ref = R"(digraph render {
    depthPrepass -> geometryPass[ label="DepthTarget1" ];
    geometryPass -> lightingPass[ label="Data1Target\nData2Target\nData3Target\nData4Target" ];
    lightingPass -> ambientPass[ label="DiffuseTarget\nSpecularTarget" ];
    ambientPass -> finalCombinePass[ label="OutputTarget" ];
    geometryPass -> ambientPass[ label="Data1Target\nData2Target\nData3Target\nData4Target" ];
    geometryPass -> ssaoRawPass[ label="Data2Target" ];
    ssaoRawPass -> ssaoBlurPass[ label="SSAORawTarget" ];
    ssaoBlurPass -> ambientPass[ label="SSAOBlurTarget" ];
    geometryPass -> ssaoBlurPass[ label="Data2Target" ];
    geometryPass -> accumulationPass[ label="VelocityTarget1" ];
    accumulationPass -> finalCombinePass[ label="VelocityTarget1\nVelocityTarget2" ];
    accumulationPass -> combinePass[ label="AccumulationTarget\nRevealageTarget" ];
    combinePass -> finalCombinePass[ label="CombineTarget" ];
    geometryPass -> finalCombinePass[ label="VelocityTarget1" ];
    depthPrepass -> lightingPass[ label="DepthTarget1" ];
    depthPrepass -> ssaoLinearisePass[ label="DepthTarget1" ];
    ssaoLinearisePass -> ssaoMinifyPass1[ label="SSAOLineariseTarget" ];
    ssaoMinifyPass1 -> ssaoMinifyPass2[ label="SSAOMinify1Target" ];
    ssaoMinifyPass2 -> ssaoMinifyPass3[ label="SSAOMinify2Target" ];
    ssaoMinifyPass3 -> ssaoRawPass[ label="SSAOMinify3Target" ];
    ssaoMinifyPass2 -> ssaoRawPass[ label="SSAOMinify2Target" ];
    ssaoMinifyPass1 -> ssaoRawPass[ label="SSAOMinify1Target" ];
    ssaoLinearisePass -> ssaoRawPass[ label="SSAOLineariseTarget" ];
    depthPrepass -> ambientPass[ label="DepthTarget1" ];
    depthPrepass -> accumulationPass[ label="DepthTarget1" ];
    depthPrepass -> combinePass[ label="DepthTarget1" ];
}
)";
					checkEqualStr( stream.str(), ref );
				}
				else
				{
					std::string ref = R"(digraph render {
    depthPrepass -> geometryPass[ label="DepthTarget1" ];
    geometryPass -> lightingPass[ label="Data1Target\nData2Target\nData3Target\nData4Target" ];
    lightingPass -> ambientPass[ label="DiffuseTarget\nSpecularTarget" ];
    ambientPass -> finalCombinePass[ label="OutputTarget" ];
    geometryPass -> ambientPass[ label="Data1Target\nData2Target\nData3Target\nData4Target" ];
    geometryPass -> ssaoRawPass[ label="Data2Target" ];
    ssaoRawPass -> ssaoBlurPass[ label="SSAORawTarget" ];
    ssaoBlurPass -> ambientPass[ label="SSAOBlurTarget" ];
    geometryPass -> ssaoBlurPass[ label="Data2Target" ];
    geometryPass -> finalCombinePass[ label="VelocityTarget1" ];
    depthPrepass -> lightingPass[ label="DepthTarget1" ];
    depthPrepass -> ssaoLinearisePass[ label="DepthTarget1" ];
    ssaoLinearisePass -> ssaoMinifyPass1[ label="SSAOLineariseTarget" ];
    ssaoMinifyPass1 -> ssaoMinifyPass2[ label="SSAOMinify1Target" ];
    ssaoMinifyPass2 -> ssaoMinifyPass3[ label="SSAOMinify2Target" ];
    ssaoMinifyPass3 -> ssaoRawPass[ label="SSAOMinify3Target" ];
    ssaoMinifyPass2 -> ssaoRawPass[ label="SSAOMinify2Target" ];
    ssaoMinifyPass1 -> ssaoRawPass[ label="SSAOMinify1Target" ];
    ssaoLinearisePass -> ssaoRawPass[ label="SSAOLineariseTarget" ];
    depthPrepass -> ambientPass[ label="DepthTarget1" ];
}
)";
					checkEqualStr( stream.str(), ref );
				}
			}
			else
			{
				if constexpr ( EnableTransparent )
				{
					std::string ref = R"(digraph render {
    depthPrepass -> geometryPass[ label="DepthTarget1" ];
    geometryPass -> lightingPass[ label="Data1Target\nData2Target\nData3Target\nData4Target" ];
    lightingPass -> ambientPass[ label="DiffuseTarget\nSpecularTarget" ];
    ambientPass -> finalCombinePass[ label="OutputTarget" ];
    geometryPass -> ambientPass[ label="Data1Target\nData2Target\nData3Target\nData4Target" ];
    geometryPass -> accumulationPass[ label="VelocityTarget1" ];
    accumulationPass -> finalCombinePass[ label="VelocityTarget1\nVelocityTarget2" ];
    accumulationPass -> combinePass[ label="AccumulationTarget\nRevealageTarget" ];
    combinePass -> finalCombinePass[ label="CombineTarget" ];
    geometryPass -> finalCombinePass[ label="VelocityTarget1" ];
    depthPrepass -> lightingPass[ label="DepthTarget1" ];
    depthPrepass -> ambientPass[ label="DepthTarget1" ];
    depthPrepass -> accumulationPass[ label="DepthTarget1" ];
    depthPrepass -> combinePass[ label="DepthTarget1" ];
}
)";
					checkEqualStr( stream.str(), ref );
				}
				else
				{
					std::string ref = R"(digraph render {
    depthPrepass -> geometryPass[ label="DepthTarget1" ];
    geometryPass -> lightingPass[ label="Data1Target\nData2Target\nData3Target\nData4Target" ];
    lightingPass -> ambientPass[ label="DiffuseTarget\nSpecularTarget" ];
    ambientPass -> finalCombinePass[ label="OutputTarget" ];
    geometryPass -> ambientPass[ label="Data1Target\nData2Target\nData3Target\nData4Target" ];
    geometryPass -> finalCombinePass[ label="VelocityTarget1" ];
    depthPrepass -> lightingPass[ label="DepthTarget1" ];
    depthPrepass -> ambientPass[ label="DepthTarget1" ];
}
)";
					checkEqualStr( stream.str(), ref );
				}
			}
		}
		else
		{
			if constexpr ( EnableTransparent )
			{
				std::string ref = R"(digraph render {
    depthPrepass -> accumulationPass[ label="DepthTarget1" ];
    accumulationPass -> combinePass[ label="AccumulationTarget\nRevealageTarget" ];
    combinePass -> finalCombinePass[ label="CombineTarget" ];
    accumulationPass -> finalCombinePass[ label="VelocityTarget1" ];
    depthPrepass -> combinePass[ label="DepthTarget1" ];
}
)";
				checkEqualStr( stream.str(), ref );
			}
			else
			{
				std::string ref = R"(digraph render {
    depthPrepass -> finalCombinePass[ label="DepthTarget1" ];
}
)";
				checkEqualStr( stream.str(), ref );
			}
		}

		testEnd();
	}
}

int main( int argc, char ** argv )
{
	testSuiteBegin( "TestRenderGraph" );
	testNoPass( testCounts );
	testOnePass( testCounts );
	testDuplicateName( testCounts );
	testWrongRemove( testCounts );
	testOneDependency( testCounts );
	testChainedDependencies( testCounts );
	testSharedDependencies( testCounts );
	test2MipDependencies( testCounts );
	test3MipDependencies( testCounts );
	testSsaoPass( testCounts );
	testFullPass< false, false, false >( testCounts );
	testFullPass< true, false, false >( testCounts );
	testFullPass< true, true, false >( testCounts );
	testFullPass< false, false, true >( testCounts );
	testFullPass< true, false, true >( testCounts );
	testFullPass< true, true, true >( testCounts );
	testSuiteEnd();
}
