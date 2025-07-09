#include "Common.hpp"

#include <RenderGraph/FrameGraph.hpp>
#include <RenderGraph/FramePassTimer.hpp>
#include <RenderGraph/ImageData.hpp>
#include <RenderGraph/ResourceHandler.hpp>
#include <RenderGraph/RunnableGraph.hpp>
#include <RenderGraph/RunnablePass.hpp>
#include <RenderGraph/RunnablePasses/GenerateMipmaps.hpp>

#include <sstream>
#include <thread>

namespace
{
	crg::GraphContext & getContext()
	{
		return test::getDummyContext();
	}

	void testBaseFuncs( test::TestCounts & testCounts )
	{
		testBegin( "testBaseFuncs" )
		crg::ResourceHandler handler;
		auto image = handler.createImageId( test::createImage( "image", crg::PixelFormat::eR16G16B16A16_SFLOAT, 8u, 6u ) );
		auto view = handler.createViewId( test::createView( "view", image, crg::PixelFormat::eR16G16B16A16_SFLOAT, 4u, 2u, 2u, 3u ) );
		getMipExtent( view );
		auto type = getImageType( image );
		check( getImageType( view ) == type )
		check( getMipLevels( image ) == 8u )
		check( getMipLevels( view ) == 2u )
		check( getArrayLayers( image ) == 6u )
		check( getArrayLayers( view ) == 3u )
		check( crg::getAccessMask( crg::ImageLayout::ePresentSrc ) == crg::AccessFlags::eMemoryRead )
		check( crg::getAccessMask( crg::ImageLayout::eSharedPresent ) == crg::AccessFlags::eMemoryRead )
		check( crg::getAccessMask( crg::ImageLayout::eColorAttachment ) == crg::AccessFlags::eColorAttachmentWrite )
		check( crg::getAccessMask( crg::ImageLayout::eDepthStencilAttachment ) == crg::AccessFlags::eDepthStencilAttachmentWrite )
		check( crg::getAccessMask( crg::ImageLayout::eDepthStencilReadOnly ) == crg::AccessFlags::eDepthStencilAttachmentRead )
		check( crg::getAccessMask( crg::ImageLayout::eShaderReadOnly ) == crg::AccessFlags::eShaderRead )
		check( crg::getAccessMask( crg::ImageLayout::eTransferSrc ) == crg::AccessFlags::eTransferRead )
		check( crg::getAccessMask( crg::ImageLayout::eTransferDst ) == crg::AccessFlags::eTransferWrite )
		check( crg::getAccessMask( crg::ImageLayout::eDepthReadOnlyStencilAttachment ) == ( crg::AccessFlags::eDepthStencilAttachmentRead | crg::AccessFlags::eDepthStencilAttachmentWrite ) )
		check( crg::getAccessMask( crg::ImageLayout::eDepthAttachmentStencilReadOnly ) == ( crg::AccessFlags::eDepthStencilAttachmentRead | crg::AccessFlags::eDepthStencilAttachmentWrite ) )
#ifdef VK_NV_shading_rate_image
		check( crg::getAccessMask( crg::ImageLayout::eFragmentShadingRateAttachment ) == crg::AccessFlags::eFragmentShadingRateAttachmentRead )
		check( crg::getPipelineState( crg::PipelineStageFlags::eFragmentShadingRateAttachment ).access == crg::AccessFlags::eFragmentShadingRateAttachmentRead )
		check( crg::getStageMask( crg::ImageLayout::eFragmentShadingRateAttachment ) == crg::PipelineStageFlags::eFragmentShadingRateAttachment )
#endif
#ifdef VK_EXT_fragment_density_map
		check( crg::getAccessMask( crg::ImageLayout::eFragmentDensityMap ) == crg::AccessFlags::eFragmentDensityMapRead )
		check( crg::getStageMask( crg::ImageLayout::eFragmentDensityMap ) == crg::PipelineStageFlags::eFragmentShader )
#endif

		check( crg::getPipelineState( crg::PipelineStageFlags::eBottomOfPipe ).access == crg::AccessFlags::eMemoryRead )
		check( crg::getPipelineState( crg::PipelineStageFlags::eColorAttachmentOutput ).access == ( crg::AccessFlags::eColorAttachmentWrite | crg::AccessFlags::eColorAttachmentRead ) )
		check( crg::getPipelineState( crg::PipelineStageFlags::eLateFragmentTests ).access == ( crg::AccessFlags::eDepthStencilAttachmentWrite | crg::AccessFlags::eDepthStencilAttachmentRead ) )
		check( crg::getPipelineState( crg::PipelineStageFlags::eFragmentShader ).access == crg::AccessFlags::eShaderRead )
		check( crg::getPipelineState( crg::PipelineStageFlags::eTransfer ).access == ( crg::AccessFlags::eTransferRead | crg::AccessFlags::eTransferWrite ) )
		check( crg::getPipelineState( crg::PipelineStageFlags::eComputeShader ).access == crg::AccessFlags::eShaderRead )

		check( crg::getStageMask( crg::ImageLayout::eUndefined ) == crg::PipelineStageFlags::eHost )
		check( crg::getStageMask( crg::ImageLayout::eGeneral ) == crg::PipelineStageFlags::eBottomOfPipe )
		check( crg::getStageMask( crg::ImageLayout::ePresentSrc ) == crg::PipelineStageFlags::eBottomOfPipe )
		check( crg::getStageMask( crg::ImageLayout::eSharedPresent ) == crg::PipelineStageFlags::eBottomOfPipe )
		check( crg::getStageMask( crg::ImageLayout::eDepthStencilReadOnly ) == crg::PipelineStageFlags::eLateFragmentTests )
		check( crg::getStageMask( crg::ImageLayout::eDepthReadOnlyStencilAttachment ) == crg::PipelineStageFlags::eLateFragmentTests )
		check( crg::getStageMask( crg::ImageLayout::eDepthAttachmentStencilReadOnly ) == crg::PipelineStageFlags::eLateFragmentTests )
		check( crg::getStageMask( crg::ImageLayout::eDepthStencilAttachment ) == crg::PipelineStageFlags::eLateFragmentTests )
		check( crg::getStageMask( crg::ImageLayout::eColorAttachment ) == crg::PipelineStageFlags::eColorAttachmentOutput )
		check( crg::getStageMask( crg::ImageLayout::eShaderReadOnly ) == crg::PipelineStageFlags::eFragmentShader )
		check( crg::getStageMask( crg::ImageLayout::eTransferSrc ) == crg::PipelineStageFlags::eTransfer )
		check( crg::getStageMask( crg::ImageLayout::eTransferDst ) == crg::PipelineStageFlags::eTransfer )

		for ( uint32_t i = 0; i <= uint32_t( crg::PixelFormat::eASTC_12x12_SRGB_BLOCK ); ++i )
			checkNoThrow( getName( crg::PixelFormat( i ) ) );

		for ( uint32_t i = 0; i <= uint32_t( crg::FilterMode::eLinear ); ++i )
			checkNoThrow( getName( crg::FilterMode( i ) ) );

		for ( uint32_t i = 0; i <= uint32_t( crg::MipmapMode::eLinear ); ++i )
			checkNoThrow( getName( crg::MipmapMode( i ) ) );

		for ( uint32_t i = 0; i <= uint32_t( crg::WrapMode::eMirrorClampToEdge ); ++i )
			checkNoThrow( getName( crg::WrapMode( i ) ) );

		auto vb1 = crg::VertexBuffer{ crg::Buffer{ VkBuffer( 1 ), "vtx1" } };
		auto vb2 = crg::VertexBuffer{ crg::Buffer{ VkBuffer( 2 ), "vtx2" } };
		vb2 = std::move( vb1 );

		testEnd()
	}

	void testSignal( test::TestCounts & testCounts )
	{
		testBegin( "testSignal" )
		{
			using DummyFunc = std::function< void() >;
			using OnDummy = crg::Signal< DummyFunc >;
			OnDummy onDummy;

			auto connection = onDummy.connect( []()
				{
					// Nothing to do here...
				} );
			onDummy();
			connection.disconnect();
			onDummy();
			connection = onDummy.connect( []()
				{
					// Nothing to do here...
				} );
			onDummy();
			connection = onDummy.connect( []()
				{
					// Nothing to do here...
				} );
			onDummy();
			auto connection2 = onDummy.connect( []()
				{
					// Nothing to do here...
				} );
			onDummy();
		}
		{
			using DummyFunc = std::function< void() >;
			using OnDummy = crg::Signal< DummyFunc >;
			auto onDummy = std::make_unique< OnDummy >();

			auto connection = onDummy->connect( []()
				{
					// Nothing to do here...
				} );
			( *onDummy )();
			auto connection2 = onDummy->connect( []()
				{
					// Nothing to do here...
				} );
			( *onDummy )();

			onDummy.reset();
		}
		{
			using DummyFunc = std::function< void( bool ) >;
			using OnDummy = crg::Signal< DummyFunc >;
			using OnDummyConnection = crg::SignalConnection< OnDummy >;
			auto onDummy = std::make_unique< OnDummy >();
			OnDummyConnection tmpConn;

			auto connection = onDummy->connect( []( bool )
				{
					// Nothing to do here...
				} );
			( *onDummy )( false );
			auto connection2 = onDummy->connect( [&tmpConn, &onDummy]( bool )
				{
					tmpConn = onDummy->connect( []( bool )
						{
							// Nothing to do here...
						} );
				} );
			( *onDummy )( true );

			onDummy.reset();
		}
		testEnd()
	}

	void testFence( test::TestCounts & testCounts )
	{
		testBegin( "testFence" )
		auto & context = getContext();
		{
			crg::Fence fence{ context, "test", {} };
			fence.reset();
			fence.wait( 0xFFFFFFFFFFFFFFFFULL );
			fence.reset();
		}
		testEnd()
	}

	void testFramePassTimer( test::TestCounts & testCounts )
	{
		testBegin( "testFramePassTimer" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		crg::RunnablePass * runPass{};
		graph.createPass( "Mesh"
			, [&testCounts, &runPass]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				auto res = createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
				runPass = res.get();
				return res;
			} );
		checkThrow( crg::checkVkResult( VK_ERROR_VALIDATION_FAILED_EXT, std::string{ "Test" } ) )
		auto runnable = graph.compile( getContext() );
		test::checkRunnable( testCounts, runnable );
		{
			auto & timer = runPass->getTimer();
			auto save = timer.getCpuTime();
			{
				auto block = timer.start();
				std::this_thread::sleep_for( std::chrono::milliseconds{ 10u } );
			}
			timer.retrieveGpuTime();
			auto end = timer.getCpuTime();
			auto total = ( end - save ) + timer.getGpuTime();
			check( total >= std::chrono::milliseconds{ 10u } );
			timer.reset();
			check( timer.getCpuTime() >= std::chrono::milliseconds{ 0u } );
			check( timer.getGpuTime() >= std::chrono::milliseconds{ 0u } );
		}
		{
			crg::FramePassTimer timer{ getContext(), "test", crg::TimerScope::eUpdate };
			auto save = timer.getCpuTime();
			{
				auto block = std::make_unique< crg::FramePassTimerBlock >( timer.start() );
				std::this_thread::sleep_for( std::chrono::milliseconds{ 10u } );
				block.reset();
			}
			timer.retrieveGpuTime();
			auto end = timer.getCpuTime();
			auto total = ( end - save ) + timer.getGpuTime();
			check( total >= std::chrono::milliseconds{ 10u } );
			timer.reset();
			check( timer.getCpuTime() >= std::chrono::milliseconds{ 0u } );
			check( timer.getGpuTime() >= std::chrono::milliseconds{ 0u } );
		}
		{
			auto timer = std::make_unique< crg::FramePassTimer >( getContext(), "test", crg::TimerScope::eUpdate );
			auto connection = timer->onDestroy.connect( []( crg::FramePassTimer const & thisTimer )
				{
					if ( thisTimer.getScope() == crg::TimerScope::eUpdate )
					{
						CRG_Exception( "WTF???" );
					}
				} );
			timer.reset();
		}
		testEnd()
	}

	void testImplicitActions( test::TestCounts & testCounts )
	{
		testBegin( "testImplicitActions" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto depth1 = graph.createImage( test::createImage( "depth1", crg::PixelFormat::eD32_SFLOAT_S8_UINT ) );
		auto depth1v = graph.createView( test::createView( "depth1v", depth1, crg::PixelFormat::eD32_SFLOAT_S8_UINT, 0u, 1u, 0u, 1u ) );
		auto depth2 = graph.createImage( test::createImage( "depth2", crg::PixelFormat::eD32_SFLOAT_S8_UINT ) );
		auto depth2v = graph.createView( test::createView( "depth2v", depth2, crg::PixelFormat::eD32_SFLOAT_S8_UINT, 0u, 1u, 0u, 1u ) );
		auto colour1 = graph.createImage( test::createImage( "colour1", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto colour1v = graph.createView( test::createView( "colour1v", colour1, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
		auto colour2 = graph.createImage( test::createImage( "colour2", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto colour2v = graph.createView( test::createView( "colour2v", colour2, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
		auto colour3 = graph.createImage( test::createImage( "colour3", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto colour3v = graph.createView( test::createView( "colour3v", colour3, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
		auto colour4 = graph.createImage( test::createImage( "colour4", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto colour4v = graph.createView( test::createView( "colour4v", colour4, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
		auto & testPass1 = graph.createPass( "Mesh"
			, [&testCounts, depth2v, colour1v, colour2v, colour3v, colour4v]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				auto depthIt = framePass.images.begin();
				auto colourIt = std::next( depthIt );
				auto extent3D = getExtent( colour2v );
				auto extent2D = VkExtent2D{ extent3D.width, extent3D.height };
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader
					, test::checkDummy
					, 0u
					, false
					, crg::ru::Config{}
						.implicitAction( depthIt->view(), crg::RecordContext::clearAttachment( *depthIt, crg::ImageLayout::eDepthStencilAttachment ) )
						.implicitAction( depth2v, crg::RecordContext::clearAttachment( depth2v, {}, crg::ImageLayout::eDepthStencilAttachment ) )
						.implicitAction( colourIt->view(), crg::RecordContext::clearAttachment( *colourIt ) )
						.implicitAction( colour4v, crg::RecordContext::clearAttachment( colour4v, {} ) )
						.implicitAction( colour2v, crg::RecordContext::blitImage( colour1v, colour2v, {}, extent2D, {}, extent2D, crg::FilterMode::eLinear, crg::ImageLayout::eShaderReadOnly ) )
						.implicitAction( colour3v, crg::RecordContext::copyImage( colour2v, colour3v, extent2D, crg::ImageLayout::eShaderReadOnly ) ) );
			} );
		testPass1.addOutputDepthStencilView( depth1v );
		testPass1.addOutputColourView( colour1v );
		testPass1.addOutputColourView( colour2v );
		testPass1.addOutputColourView( colour3v );
		testPass1.addOutputColourView( colour4v );
		testPass1.addOutputStorageView( depth2v, 0u );

		auto & testPass2 = graph.createPass( "Pass"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader
					, test::checkDummy
					, 0u );
			} );
		testPass2.addDependency( testPass1 );
		testPass2.addInOutDepthStencilView( depth1v );
		testPass2.addInOutColourView( colour1v );
		testPass2.addInOutColourView( colour2v );
		testPass2.addInOutColourView( colour3v );
		testPass2.addInOutColourView( colour4v );
		testPass2.addInOutStorageView( depth2v, 0u );

		auto runnable = graph.compile( getContext() );
		test::checkRunnable( testCounts, runnable );
		testEnd()
	}

	void testPrePassActions( test::TestCounts & testCounts )
	{
		testBegin( "testPrePassActions" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto depth1 = graph.createImage( test::createImage( "depth1", crg::PixelFormat::eD32_SFLOAT_S8_UINT ) );
		auto depth1v = graph.createView( test::createView( "depth1v", depth1, crg::PixelFormat::eD32_SFLOAT_S8_UINT, 0u, 1u, 0u, 1u ) );
		auto depth2 = graph.createImage( test::createImage( "depth2", crg::PixelFormat::eD32_SFLOAT_S8_UINT ) );
		auto depth2v = graph.createView( test::createView( "depth2v", depth2, crg::PixelFormat::eD32_SFLOAT_S8_UINT, 0u, 1u, 0u, 1u ) );
		auto colour1 = graph.createImage( test::createImage( "colour1", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto colour1v = graph.createView( test::createView( "colour1v", colour1, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
		auto colour2 = graph.createImage( test::createImage( "colour2", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto colour2v = graph.createView( test::createView( "colour2v", colour2, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
		auto colour3 = graph.createImage( test::createImage( "colour3", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto colour3v = graph.createView( test::createView( "colour3v", colour3, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
		auto colour4 = graph.createImage( test::createImage( "colour4", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto colour4v = graph.createView( test::createView( "colour4v", colour4, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
		auto & testPass1 = graph.createPass( "Mesh"
			, [&testCounts, depth2v, colour1v, colour2v, colour3v, colour4v]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				auto depthIt = framePass.images.begin();
				auto colourIt = std::next( depthIt );
				auto extent3D = getExtent( colour2v );
				auto extent2D = VkExtent2D{ extent3D.width, extent3D.height };
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader
					, test::checkDummy
					, crg::ru::Config{}
						.prePassAction( crg::RecordContext::clearAttachment( *depthIt, crg::ImageLayout::eDepthStencilAttachment ) )
						.prePassAction( crg::RecordContext::clearAttachment( depth2v, {}, crg::ImageLayout::eDepthStencilAttachment ) )
						.prePassAction( crg::RecordContext::clearAttachment( *colourIt ) )
						.prePassAction( crg::RecordContext::clearAttachment( colour4v, {} ) )
						.prePassAction( crg::RecordContext::blitImage( colour1v, colour2v, {}, extent2D, {}, extent2D, crg::FilterMode::eLinear, crg::ImageLayout::eShaderReadOnly ) )
						.prePassAction( crg::RecordContext::copyImage( colour2v, colour3v, extent2D, crg::ImageLayout::eShaderReadOnly ) ) );
			} );
		testPass1.addOutputDepthStencilView( depth1v );
		testPass1.addOutputColourView( colour1v );
		testPass1.addOutputColourView( colour2v );
		testPass1.addOutputColourView( colour3v );
		testPass1.addOutputColourView( colour4v );
		testPass1.addOutputStorageView( depth2v, 0u );

		auto & testPass2 = graph.createPass( "Pass"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader
					, test::checkDummy
					, 0u );
			} );
		testPass2.addDependency( testPass1 );
		testPass2.addInOutDepthStencilView( depth1v );
		testPass2.addInOutColourView( colour1v );
		testPass2.addInOutColourView( colour2v );
		testPass2.addInOutColourView( colour3v );
		testPass2.addInOutColourView( colour4v );
		testPass2.addInOutStorageView( depth2v, 0u );

		auto runnable = graph.compile( getContext() );
		test::checkRunnable( testCounts, runnable );
		testEnd()
	}

	void testPostPassActions( test::TestCounts & testCounts )
	{
		testBegin( "testPrePassActions" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto depth1 = graph.createImage( test::createImage( "depth1", crg::PixelFormat::eD32_SFLOAT_S8_UINT ) );
		auto depth1v = graph.createView( test::createView( "depth1v", depth1, crg::PixelFormat::eD32_SFLOAT_S8_UINT, 0u, 1u, 0u, 1u ) );
		auto depth2 = graph.createImage( test::createImage( "depth2", crg::PixelFormat::eD32_SFLOAT_S8_UINT ) );
		auto depth2v = graph.createView( test::createView( "depth2v", depth2, crg::PixelFormat::eD32_SFLOAT_S8_UINT, 0u, 1u, 0u, 1u ) );
		auto colour1 = graph.createImage( test::createImage( "colour1", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto colour1v = graph.createView( test::createView( "colour1v", colour1, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
		auto colour2 = graph.createImage( test::createImage( "colour2", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto colour2v = graph.createView( test::createView( "colour2v", colour2, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
		auto colour3 = graph.createImage( test::createImage( "colour3", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto colour3v = graph.createView( test::createView( "colour3v", colour3, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
		auto colour4 = graph.createImage( test::createImage( "colour4", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto colour4v = graph.createView( test::createView( "colour4v", colour4, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
		crg::RunnablePass * runPass{};
		auto & testPass1 = graph.createPass( "Mesh"
			, [&runPass , &testCounts, depth2v, colour1v, colour2v, colour3v, colour4v]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				auto depthIt = framePass.images.begin();
				auto colourIt = std::next( depthIt );
				auto extent3D = getExtent( colour2v );
				auto extent2D = VkExtent2D{ extent3D.width, extent3D.height };
				auto res = createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader
					, test::checkDummy
					, crg::ru::Config{ 1u, true }
						.postPassAction( crg::RecordContext::clearAttachment( *depthIt, crg::ImageLayout::eDepthStencilAttachment ) )
						.postPassAction( crg::RecordContext::clearAttachment( depth2v, {}, crg::ImageLayout::eDepthStencilAttachment ) )
						.postPassAction( crg::RecordContext::clearAttachment( *colourIt ) )
						.postPassAction( crg::RecordContext::clearAttachment( colour4v, {} ) )
						.postPassAction( crg::RecordContext::blitImage( colour1v, colour2v, {}, extent2D, {}, extent2D, crg::FilterMode::eLinear, crg::ImageLayout::eShaderReadOnly ) )
						.postPassAction( crg::RecordContext::copyImage( colour2v, colour3v, extent2D, crg::ImageLayout::eShaderReadOnly ) ) );
				runPass = res.get();
				return res;
			} );
		testPass1.addOutputDepthStencilView( depth1v );
		testPass1.addOutputColourView( colour1v );
		testPass1.addOutputColourView( colour2v );
		testPass1.addOutputColourView( colour3v );
		testPass1.addOutputColourView( colour4v );
		testPass1.addOutputStorageView( depth2v, 0u );

		auto & testPass2 = graph.createPass( "Pass"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader
					, test::checkDummy
					, 0u );
			} );
		testPass2.addDependency( testPass1 );
		testPass2.addInOutDepthStencilView( depth1v );
		testPass2.addInOutColourView( colour1v );
		testPass2.addInOutColourView( colour2v );
		testPass2.addInOutColourView( colour3v );
		testPass2.addInOutColourView( colour4v );
		testPass2.addInOutStorageView( depth2v, 0u );

		auto runnable = graph.compile( getContext() );
		test::checkRunnable( testCounts, runnable );
		testEnd()
	}

	void testGraphDeps( test::TestCounts & testCounts )
	{
		testBegin( "testGraphDeps" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph1{ handler, testCounts.testName + "1" };
		auto colour = graph1.createImage( test::createImage( "colour", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto colourv = graph1.createView( test::createView( "colourv", colour, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
		auto iocolour = graph1.createImage( test::createImage( "iocolour", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto iocolourv = graph1.createView( test::createView( "iocolourv", iocolour, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
		auto & testPass1 = graph1.createPass( "Mesh"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		testPass1.addOutputColourView( colourv );
		testPass1.addOutputColourView( iocolourv );
		testPass1.addOutputStorageBuffer( crg::Buffer{ ( VkBuffer )1u, "test" }, 0u, 0u, VK_WHOLE_SIZE );
		graph1.addOutput( colourv, crg::makeLayoutState( crg::ImageLayout::eShaderReadOnly ) );
		graph1.addOutput( iocolourv, crg::makeLayoutState( crg::ImageLayout::eColorAttachment ) );
		check( graph1.getOutputLayoutState( colourv ).layout == crg::ImageLayout::eShaderReadOnly )

		crg::FrameGraph graph2{ handler, testCounts.testName + "2" };
		graph2.addInput( colourv, crg::makeLayoutState( crg::ImageLayout::eShaderReadOnly ) );
		graph2.addInput( iocolourv, crg::makeLayoutState( crg::ImageLayout::eColorAttachment ) );
		check( graph2.getInputLayoutState( colourv ).layout == crg::ImageLayout::eShaderReadOnly )
		graph2.addDependency( graph1 );
		auto & testPass2 = graph2.createPass( "Pass"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		testPass2.addSampledView( colourv, 0u );
		testPass2.addInOutColourView( iocolourv );

		auto runnable1 = graph1.compile( getContext() );
		test::checkRunnable( testCounts, runnable1 );

		auto runnable2 = graph2.compile( getContext() );
		test::checkRunnable( testCounts, runnable2 );
		testEnd()
	}

	void test2PassGraphDeps( test::TestCounts & testCounts )
	{
		testBegin( "test2PassGraphDeps" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph1{ handler, testCounts.testName + "1" };
		crg::Buffer buffer{ ( VkBuffer )1u, "test" };
		auto colour = graph1.createImage( test::createImage( "colour", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto colourv = graph1.createView( test::createView( "colourv", colour, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
		{
			auto & testPass11 = graph1.createPass( "Pass1"
				, [&testCounts]( crg::FramePass const & framePass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
						return createDummy( testCounts
							, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
				} );
			testPass11.addOutputColourView( colourv );
			testPass11.addOutputStorageBuffer( buffer, 0u, 0u, VK_WHOLE_SIZE );
			auto & testPass12 = graph1.createPass( "Pass2"
				, [&testCounts]( crg::FramePass const & framePass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
						return createDummy( testCounts
							, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
				} );
			testPass12.addDependency( testPass11 );
			testPass12.addInOutStorageBuffer( buffer, 0u, 0u, VK_WHOLE_SIZE );
			testPass12.addInOutStorageView( colourv, 1u );
		}
		graph1.addOutput( colourv, crg::makeLayoutState( crg::ImageLayout::eShaderReadOnly ) );
		check( graph1.getOutputLayoutState( colourv ).layout == crg::ImageLayout::eShaderReadOnly )

		crg::FrameGraph graph2{ handler, testCounts.testName + "2" };
		graph2.addInput( colourv, crg::makeLayoutState( crg::ImageLayout::eShaderReadOnly ) );
		check( graph2.getInputLayoutState( colourv ).layout == crg::ImageLayout::eShaderReadOnly )
		graph2.addDependency( graph1 );
		{
			auto & testPass21 = graph2.createPass( "Pass1"
				, [&testCounts]( crg::FramePass const & framePass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
						return createDummy( testCounts
							, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
				} );
			testPass21.addSampledView( colourv, 0u );

			auto & testPass22 = graph2.createPass( "Pass2"
				, [&testCounts]( crg::FramePass const & framePass
					, crg::GraphContext & context
					, crg::RunnableGraph & runGraph )
				{
						return createDummy( testCounts
							, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
				} );
			testPass22.addDependency( testPass21 );
			testPass22.addInputStorageBuffer( buffer, 0u, 0u, VK_WHOLE_SIZE );
		}

		auto runnable1 = graph1.compile( getContext() );
		test::checkRunnable( testCounts, runnable1 );

		auto runnable2 = graph2.compile( getContext() );
		test::checkRunnable( testCounts, runnable2 );
		testEnd()
	}

	void testPassGroupDeps( test::TestCounts & testCounts )
	{
		testBegin( "testPassGroupDeps" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph1{ handler, testCounts.testName + "1" };
		auto & group1 = graph1.getDefaultGroup();
		auto colour = group1.createImage( test::createImage( "colour", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto colourv = group1.createView( test::createView( "colourv", colour, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
		auto & testPass1 = group1.createPass( "Mesh"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		testPass1.addOutputColourView( colourv );
		testPass1.addOutputStorageBuffer( crg::Buffer{ ( VkBuffer )1u, "test" }, 0u, 0u, VK_WHOLE_SIZE );
		group1.addOutput( colourv, crg::makeLayoutState( crg::ImageLayout::eShaderReadOnly ) );
		group1.addGroupOutput( colourv );

		crg::FrameGraph graph2{ handler, testCounts.testName + "2" };
		auto & group2 = graph2.getDefaultGroup();
		group2.addInput( colourv, crg::makeLayoutState( crg::ImageLayout::eShaderReadOnly ) );
		group2.addGroupInput( colourv );
		graph2.addDependency( graph1 );
		auto & testPass2 = group2.createPass( "Pass"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		testPass2.addSampledView( colourv, 0u );

		auto runnable1 = graph1.compile( getContext() );
		test::checkRunnable( testCounts, runnable1 );

		auto runnable2 = graph2.compile( getContext() );
		test::checkRunnable( testCounts, runnable2 );
		testEnd()
	}

	void testPassGroups( test::TestCounts & testCounts )
	{
		testBegin( "testPassGroups" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto & group1 = graph.createPassGroup( "First" );
		auto colour = group1.createImage( test::createImage( "colour", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto colourv = group1.createView( test::createView( "colourv", colour, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
		crg::Buffer buffer{ ( VkBuffer )1u, "test" };
		auto & testPass1 = group1.createPass( "Mesh"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		testPass1.addOutputColourView( colourv );
		testPass1.addOutputStorageBuffer( buffer, 0u, 0u, VK_WHOLE_SIZE );
		group1.addGroupOutput( colourv );

		auto & group2 = graph.createPassGroup( "Second" ).createPassGroup( "Third" );
		group2.addGroupInput( colourv );
		auto & testPass2 = group2.createPass( "Pass"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		testPass2.addDependency( testPass1 );
		testPass2.addSampledView( colourv, 0u );
		testPass2.addInputStorageBuffer( buffer, 0u, 0u, VK_WHOLE_SIZE );

		auto runnable2 = graph.compile( getContext() );
		test::checkRunnable( testCounts, runnable2 );
		testEnd()
	}

	void testResourcesCache( test::TestCounts & testCounts )
	{
		testBegin( "testResourcesCache" )
		{
			crg::ResourceHandler handler;
			crg::RecordContext context( handler );
			checkThrow( context.getContext() )
		}
		auto & context = getContext();
		crgUnregisterObject( context, VkBuffer( 1 ) );
		checkThrow( context.deduceMemoryType( 0u, 0u ) )
		{
			crg::ResourceHandler handler;
			auto sampled = handler.createImageId( test::createImage( "sampled", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
			auto sampledv = handler.createViewId( test::createView( "sampledv", sampled, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
			handler.createImage( context, sampled );
			handler.createImageView( context, sampledv );
			handler.createQuadTriVertexBuffer( context, "test", false, {} );
			handler.createQuadTriVertexBuffer( context, "test", true, {} );
			handler.createSampler( context, "test", crg::SamplerDesc{} );
		}
		crg::ResourceHandler handler;
		crg::ResourcesCache resources{ handler };
		{
			auto sampled = handler.createImageId( test::createImage( "sampled", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
			auto sampledv = handler.createViewId( test::createView( "sampledv", sampled, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
			resources.createImage( context, sampled );
			resources.createImageView( context, sampledv );
			resources.destroyImageView( context, sampledv );
			resources.destroyImage( context, sampled );
		}
		{
			auto result = handler.createImageId( test::createImage( "result", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
			auto resultv = handler.createViewId( test::createView( "resultv", result, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
			resources.createImage( context, result );
			resources.createImageView( context, resultv );
			resources.destroyImageView( resultv );
			resources.destroyImage( result );
		}
		{
			auto result = handler.createImageId( test::createImage( "result", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
			auto resultv = handler.createViewId( test::createView( "resultv", result, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
			resources.createImage( context, result );
			resources.createImageView( context, resultv );
			resources.createSampler( context, crg::SamplerDesc{} );
			resources.createQuadTriVertexBuffer( context, false, {} );
			resources.createQuadTriVertexBuffer( context, true, {} );
		}
		testEnd()
	}

	void testGraphNodes( test::TestCounts & testCounts )
	{
		testBegin( "testGraphNodes" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		crg::RootNode root{ graph };
		check( getFramePass( root ) == nullptr )
		testEnd()
	}
}

int main( int argc, char ** argv )
{
	testSuiteBegin( "TestBases" )
	testBaseFuncs( testCounts );
	testSignal( testCounts );
	testFence( testCounts );
	testFramePassTimer( testCounts );
	testImplicitActions( testCounts );
	testPrePassActions( testCounts );
	testPostPassActions( testCounts );
	testGraphDeps( testCounts );
	test2PassGraphDeps( testCounts );
	testPassGroupDeps( testCounts );
	testPassGroups( testCounts );
	testResourcesCache( testCounts );
	testGraphNodes( testCounts );
	testSuiteEnd()
}
