#include "Common.hpp"

#include <RenderGraph/FrameGraph.hpp>
#include <RenderGraph/FramePassTimer.hpp>
#include <RenderGraph/ImageData.hpp>
#include <RenderGraph/Log.hpp>
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
}

TEST( Bases, BaseFuncs )
{
	testBegin( "testBaseFuncs" )
	crg::ResourceHandler handler;
	auto image = handler.createImageId( test::createImage( "image", crg::PixelFormat::eR16G16B16A16_SFLOAT, 8u, 6u ) );
	auto view = handler.createViewId( test::createView( "view", image, crg::PixelFormat::eR16G16B16A16_SFLOAT, 4u, 2u, 2u, 3u ) );
	getMipExtent( view );
	auto type = getImageType( image );
	check( getImageType( view ) == type )
	check( getImageViewType( view ) == view.data->info.viewType )
	check( getImageCreateFlags( view ) == getImageCreateFlags( image ) )
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

	auto vb1 = crg::VertexBuffer{ handler.createViewId( test::createView( "vtx1", handler.createBufferId( test::createBuffer( "vtx1" ) ) ) ) };
	auto vb2 = crg::VertexBuffer{ handler.createViewId( test::createView( "vtx2", handler.createBufferId( test::createBuffer( "vtx2" ) ) ) ) };
	vb2 = std::move( vb1 );

	testEnd()
}

TEST( Bases, ClearValues )
{
	testBegin( "testClearValues" )
	crg::ClearColorValue clearColorInt32{ std::array< int32_t, 4u >{ 1, 2, 3, 4 } };
	crg::ClearColorValue clearColorUInt32{ std::array< uint32_t, 4u >{ 1u, 2u, 3u, 4u } };
	crg::ClearColorValue clearColorFloat32{ std::array< float, 4u >{ 1.0f, 2.0f, 3.0f, 4.0f } };
	crg::ClearDepthStencilValue clearDepthStencil{ 1.0f, 255u };

	checkNoThrow( getClearDepthStencilValue( crg::ClearValue{ clearColorFloat32 } ) );
	checkNoThrow( getClearColorValue( crg::ClearValue{ clearDepthStencil } ) );
	checkNoThrow( getClearColorValue( crg::ClearValue{ clearColorFloat32 } ) );
	checkNoThrow( getClearDepthStencilValue( crg::ClearValue{ clearDepthStencil } ) );
	checkNoThrow( convert( crg::ClearValue{ clearColorFloat32 } ) );
	checkNoThrow( convert( crg::ClearValue{ clearDepthStencil } ) );
	check( clearColorFloat32.isFloat32() );
	check( !clearColorFloat32.isInt32() );
	check( !clearColorFloat32.isUInt32() );
	check( !clearColorUInt32.isFloat32() );
	check( !clearColorUInt32.isInt32() );
	check( clearColorUInt32.isUInt32() );
	check( !clearColorInt32.isFloat32() );
	check( clearColorInt32.isInt32() );
	check( !clearColorInt32.isUInt32() );
	check( crg::ClearValue{ clearColorFloat32 }.isColor() );
	check( !crg::ClearValue{ clearColorFloat32 }.isDepthStencil() );
	check( !crg::ClearValue{ clearDepthStencil }.isColor() );
	check( crg::ClearValue{ clearDepthStencil }.isDepthStencil() );
	{
		auto vkClearColorFloat32 = crg::convert( clearColorFloat32 );
		for ( uint32_t i = 0; i < 4u; ++i )
			check( vkClearColorFloat32.float32[i] == clearColorFloat32.float32()[i] );
	}
	{
		auto vkClearColorFloat32 = crg::convert( crg::ClearValue{ clearColorFloat32 } );
		for ( uint32_t i = 0; i < 4u; ++i )
			check( vkClearColorFloat32.color.float32[i] == clearColorFloat32.float32()[i] );
	}
	{
		auto vkClearColorInt32 = crg::convert( clearColorInt32 );
		for ( uint32_t i = 0; i < 4u; ++i )
			check( vkClearColorInt32.int32[i] == clearColorInt32.int32()[i] );
	}
	{
		auto vkClearColorInt32 = crg::convert( crg::ClearValue{ clearColorInt32 } );
		for ( uint32_t i = 0; i < 4u; ++i )
			check( vkClearColorInt32.color.int32[i] == clearColorInt32.int32()[i] );
	}
	{
		auto vkClearColorUInt32 = crg::convert( clearColorUInt32 );
		for ( uint32_t i = 0; i < 4u; ++i )
			check( vkClearColorUInt32.uint32[i] == clearColorUInt32.uint32()[i] );
	}
	{
		auto vkClearColorUInt32 = crg::convert( crg::ClearValue{ clearColorUInt32 } );
		for ( uint32_t i = 0; i < 4u; ++i )
			check( vkClearColorUInt32.color.uint32[i] == clearColorUInt32.uint32()[i] );
	}
	{
		auto vkClearDepthStencil = crg::convert( clearDepthStencil );
		check( vkClearDepthStencil.depth == clearDepthStencil.depth );
		check( vkClearDepthStencil.stencil == clearDepthStencil.stencil );
	}
	{
		auto vkClearDepthStencil = crg::convert( crg::ClearValue{ clearDepthStencil } );
		check( vkClearDepthStencil.depthStencil.depth == clearDepthStencil.depth );
		check( vkClearDepthStencil.depthStencil.stencil == clearDepthStencil.stencil );
	}

	testEnd()
}

TEST( Bases, Signal )
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

TEST( Bases, Exception )
{
	testBegin( "testException" )
	auto & context = getContext();
	try
	{
		CRG_Exception( "Coin !!" );
	}
	catch ( crg::Exception & exc )
	{
		crg::Logger::logInfo( exc.what() );
	}
	testEnd()
}

TEST( Bases, Fence )
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

TEST( Bases, FramePassTimer )
{
	testBegin( "testFramePassTimer" )
	crg::ResourceHandler handler;
	crg::FrameGraph graph{ handler, testCounts.testName };
	crg::RunnablePass * runPass{};
	auto buffer = graph.createBuffer( test::createBuffer( "buffer" ) );
	auto bufferv = graph.createView( test::createView( "bufferv", buffer ) );
	auto & testPass = graph.createPass( "Mesh"
		, [&testCounts, &runPass]( crg::FramePass const & framePass
			, crg::GraphContext & context
			, crg::RunnableGraph & runGraph )
		{
			auto res = createDummy( testCounts
				, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			runPass = res.get();
			return res;
		} );
	testPass.addClearableOutputStorageBuffer( bufferv, 1u );
	checkThrow( crg::checkVkResult( VK_ERROR_VALIDATION_FAILED_EXT, std::string{ "Test" } ), crg::Exception )
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

TEST( Bases, ImplicitActions )
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
	auto buffer1 = graph.createBuffer( test::createBuffer( "buffer1" ) );
	auto buffer1v = graph.createView( test::createView( "buffer1v", buffer1 ) );
	auto buffer2 = graph.createBuffer( test::createBuffer( "buffer2" ) );
	auto buffer2v = graph.createView( test::createView( "buffer2v", buffer2 ) );
	auto buffer3 = graph.createBuffer( test::createBuffer( "buffer3" ) );
	auto buffer3v = graph.createView( test::createView( "buffer3v", buffer3 ) );
	auto buffer4 = graph.createBuffer( test::createBuffer( "buffer4" ) );
	auto buffer4v = graph.createView( test::createView( "buffer4v", buffer4 ) );
	auto & testPass1 = graph.createPass( "Mesh"
		, [&testCounts, depth2v, colour1v, colour2v, colour3v, colour4v, buffer1v, buffer2v, buffer3v]( crg::FramePass const & framePass
			, crg::GraphContext & context
			, crg::RunnableGraph & runGraph )
		{
			auto depthIt = framePass.targets.begin();
			auto colourIt = std::next( depthIt );
			auto extent3D = getExtent( colour2v );
			auto extent2D = crg::Extent2D{ extent3D.width, extent3D.height };
			return createDummy( testCounts
				, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader
				, test::checkDummy
				, 0u
				, false
				, crg::ru::Config{}
					.implicitAction( ( *depthIt )->view(), crg::RecordContext::clearAttachment( **depthIt, crg::ImageLayout::eDepthStencilAttachment ) )
					.implicitAction( depth2v, crg::RecordContext::clearAttachment( depth2v, crg::ClearDepthStencilValue{}, crg::ImageLayout::eDepthStencilAttachment ) )
					.implicitAction( ( *colourIt )->view(), crg::RecordContext::clearAttachment( **colourIt ) )
					.implicitAction( colour4v, crg::RecordContext::clearAttachment( colour4v, crg::ClearColorValue{}, crg::ImageLayout::eShaderReadOnly ) )
					.implicitAction( colour2v, crg::RecordContext::blitImage( colour1v, colour2v, { {}, extent2D }, { {}, extent2D }, crg::FilterMode::eLinear, crg::ImageLayout::eShaderReadOnly ) )
					.implicitAction( colour3v, crg::RecordContext::copyImage( colour2v, colour3v, extent2D, crg::ImageLayout::eShaderReadOnly ) )
					.implicitAction( buffer1v, crg::RecordContext::clearBuffer( buffer1v, { crg::AccessFlags::eShaderWrite, crg::PipelineStageFlags::eFragmentShader } ) )
					.implicitAction( buffer2v, crg::RecordContext::clearBuffer( buffer2v, 18u, { crg::AccessFlags::eShaderWrite, crg::PipelineStageFlags::eFragmentShader } ) )
					.implicitAction( buffer3v, crg::RecordContext::copyBuffer( buffer1v, buffer3v, 0u, 0u, 48u, { crg::AccessFlags::eShaderWrite, crg::PipelineStageFlags::eFragmentShader } ) ) );
		} );
	auto depth1a = testPass1.addOutputDepthStencilTarget( depth1v );
	auto colour1a = testPass1.addOutputColourTarget( colour1v );
	auto colour2a = testPass1.addOutputColourTarget( colour2v );
	auto colour3a = testPass1.addOutputColourTarget( colour3v );
	auto colour4a = testPass1.addOutputColourTarget( colour4v );
	auto depth2a = testPass1.addOutputStorageImage( depth2v, 0u );
	auto buffer2a = testPass1.addOutputStorageBuffer( buffer2v, 1 );
	auto buffer3a = testPass1.addOutputStorageBuffer( buffer3v, 2 );
	auto buffer4a = testPass1.addOutputStorageBuffer( buffer4v, 3 );
	auto buffer1a = testPass1.addOutputStorageBuffer( buffer1v, 4 );

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
	testPass2.addInOutDepthStencilTarget( *depth1a );
	testPass2.addInOutColourTarget( *colour1a );
	testPass2.addInOutColourTarget( *colour2a );
	testPass2.addInOutColourTarget( *colour3a );
	testPass2.addInOutColourTarget( *colour4a );
	testPass2.addInOutStorage( *depth2a, 0u );
	testPass2.addInOutStorage( *buffer2a, 1 );
	testPass2.addInOutStorage( *buffer3a, 2 );
	testPass2.addInOutStorage( *buffer4a, 3 );
	testPass2.addInOutStorage( *buffer1a, 4 );

	auto runnable = graph.compile( getContext() );
	test::checkRunnable( testCounts, runnable );
	testEnd()
}

TEST( Bases, PrePassActions )
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
			auto depthIt = framePass.targets.begin();
			auto colourIt = std::next( depthIt );
			auto extent3D = getExtent( colour2v );
			auto extent2D = crg::Extent2D{ extent3D.width, extent3D.height };
			return createDummy( testCounts
				, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader
				, test::checkDummy
				, crg::ru::Config{}
					.prePassAction( crg::RecordContext::clearAttachment( **depthIt, crg::ImageLayout::eDepthStencilAttachment ) )
					.prePassAction( crg::RecordContext::clearAttachment( depth2v, crg::ClearDepthStencilValue{}, crg::ImageLayout::eDepthStencilAttachment ) )
					.prePassAction( crg::RecordContext::clearAttachment( **colourIt ) )
					.prePassAction( crg::RecordContext::clearAttachment( colour4v, crg::ClearColorValue{} ) )
					.prePassAction( crg::RecordContext::blitImage( colour1v, colour2v, { {}, extent2D }, { {}, extent2D }, crg::FilterMode::eLinear, crg::ImageLayout::eShaderReadOnly ) )
					.prePassAction( crg::RecordContext::copyImage( colour2v, colour3v, extent2D, crg::ImageLayout::eShaderReadOnly ) ) );
		} );
	auto depth1a = testPass1.addOutputDepthStencilTarget( depth1v );
	auto colour1a = testPass1.addOutputColourTarget( colour1v );
	auto colour2a = testPass1.addOutputColourTarget( colour2v );
	auto colour3a = testPass1.addOutputColourTarget( colour3v );
	auto colour4a = testPass1.addOutputColourTarget( colour4v );
	auto depth2a = testPass1.addOutputStorageImage( depth2v, 0u );

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
	testPass2.addInOutDepthStencilTarget( *depth1a );
	testPass2.addInOutColourTarget( *colour1a );
	testPass2.addInOutColourTarget( *colour2a );
	testPass2.addInOutColourTarget( *colour3a );
	testPass2.addInOutColourTarget( *colour4a );
	testPass2.addInOutStorage( *depth2a, 0u );

	auto runnable = graph.compile( getContext() );
	test::checkRunnable( testCounts, runnable );
	testEnd()
}

TEST( Bases, PostPassActions )
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
	crg::RunnablePass const * runPass{};
	auto & testPass1 = graph.createPass( "Mesh"
		, [&runPass , &testCounts, depth2v, colour1v, colour2v, colour3v, colour4v]( crg::FramePass const & framePass
			, crg::GraphContext & context
			, crg::RunnableGraph & runGraph )
		{
			auto depthIt = framePass.targets.begin();
			auto colourIt = std::next( depthIt );
			auto extent3D = getExtent( colour2v );
			auto extent2D = crg::Extent2D{ extent3D.width, extent3D.height };
			auto res = createDummy( testCounts
				, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader
				, test::checkDummy
				, crg::ru::Config{ 1u, true }
					.postPassAction( crg::RecordContext::clearAttachment( **depthIt, crg::ImageLayout::eDepthStencilAttachment ) )
					.postPassAction( crg::RecordContext::clearAttachment( depth2v, crg::ClearDepthStencilValue{}, crg::ImageLayout::eDepthStencilAttachment ) )
					.postPassAction( crg::RecordContext::clearAttachment( **colourIt ) )
					.postPassAction( crg::RecordContext::clearAttachment( colour4v, crg::ClearColorValue{} ) )
					.postPassAction( crg::RecordContext::blitImage( colour1v, colour2v, { {}, extent2D }, { {}, extent2D }, crg::FilterMode::eLinear, crg::ImageLayout::eShaderReadOnly ) )
					.postPassAction( crg::RecordContext::copyImage( colour2v, colour3v, extent2D, crg::ImageLayout::eShaderReadOnly ) ) );
			runPass = res.get();
			return res;
		} );
	auto depth1a = testPass1.addOutputDepthStencilTarget( depth1v );
	auto colour1a = testPass1.addOutputColourTarget( colour1v );
	auto colour2a = testPass1.addOutputColourTarget( colour2v );
	auto colour3a = testPass1.addOutputColourTarget( colour3v );
	auto colour4a = testPass1.addOutputColourTarget( colour4v );
	auto depth2a = testPass1.addOutputStorageImage( depth2v, 0u );

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
	testPass2.addInOutDepthStencilTarget( *depth1a );
	testPass2.addInOutColourTarget( *colour1a );
	testPass2.addInOutColourTarget( *colour2a );
	testPass2.addInOutColourTarget( *colour3a );
	testPass2.addInOutColourTarget( *colour4a );
	testPass2.addInOutStorage( *depth2a, 0u );

	auto runnable = graph.compile( getContext() );
	test::checkRunnable( testCounts, runnable );
	testEnd()
}

TEST( Bases, GraphDeps )
{
	testBegin( "testGraphDeps" )
	crg::ResourceHandler handler;
	crg::FrameGraph graph1{ handler, testCounts.testName + "1" };
	auto colour = graph1.createImage( test::createImage( "colour", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
	auto colourv = graph1.createView( test::createView( "colourv", colour, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
	auto iocolour = graph1.createImage( test::createImage( "iocolour", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
	auto iocolourv = graph1.createView( test::createView( "iocolourv", iocolour, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
	auto buffer = graph1.createBuffer( test::createBuffer( "buffer" ) );
	auto bufferv = graph1.createView( test::createView( "bufferv", buffer ) );
	auto & testPass1 = graph1.createPass( "Mesh"
		, [&testCounts]( crg::FramePass const & framePass
			, crg::GraphContext & context
			, crg::RunnableGraph & runGraph )
		{
			return createDummy( testCounts
				, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
		} );
	auto coloura = testPass1.addOutputColourTarget( colourv );
	auto iocoloura = testPass1.addOutputColourTarget( iocolourv );
	auto bufferAttach = testPass1.addOutputStorageBuffer( bufferv, 0u );
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
	testPass2.addInputSampled( *coloura, 0u );
	testPass2.addInOutColourTarget( *iocoloura );

	auto runnable1 = graph1.compile( getContext() );
	test::checkRunnable( testCounts, runnable1 );

	auto runnable2 = graph2.compile( getContext() );
	test::checkRunnable( testCounts, runnable2 );
	testEnd()
}

TEST( Bases, 2PassGraphDeps )
{
	testBegin( "test2PassGraphDeps" )
	crg::ResourceHandler handler;
	crg::FrameGraph graph1{ handler, testCounts.testName + "1" };
	auto buffer = graph1.createBuffer( test::createBuffer( "buffer" ) );
	auto bufferv = graph1.createView( test::createView( "bufferv", buffer ) );
	auto colour = graph1.createImage( test::createImage( "colour", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
	auto colourv = graph1.createView( test::createView( "colourv", colour, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );

	auto & testPass11 = graph1.createPass( "Pass1"
		, [&testCounts]( crg::FramePass const & framePass
			, crg::GraphContext & context
			, crg::RunnableGraph & runGraph )
		{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
		} );
	auto coloura = testPass11.addOutputColourTarget( colourv );
	auto bufferAttach = testPass11.addOutputStorageBuffer( bufferv, 0u );

	auto & testPass12 = graph1.createPass( "Pass2"
		, [&testCounts]( crg::FramePass const & framePass
			, crg::GraphContext & context
			, crg::RunnableGraph & runGraph )
		{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
		} );
	bufferAttach = testPass12.addInOutStorage( *bufferAttach, 0u );
	coloura = testPass12.addInOutStorage( *coloura, 1u );

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
		testPass21.addInputSampled( *coloura, 0u );

		auto & testPass22 = graph2.createPass( "Pass2"
			, [&testCounts]( crg::FramePass const & framePass
				, crg::GraphContext & context
				, crg::RunnableGraph & runGraph )
			{
					return createDummy( testCounts
						, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
			} );
		testPass22.addInputStorage( *bufferAttach, 0u );
	}

	auto runnable1 = graph1.compile( getContext() );
	test::checkRunnable( testCounts, runnable1 );

	auto runnable2 = graph2.compile( getContext() );
	test::checkRunnable( testCounts, runnable2 );
	testEnd()
}

TEST( Bases, PassGroupDeps )
{
	testBegin( "testPassGroupDeps" )
	crg::ResourceHandler handler;
	crg::FrameGraph graph1{ handler, testCounts.testName + "1" };
	auto & group1 = graph1.getDefaultGroup();
	auto colour = group1.createImage( test::createImage( "colour", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
	auto colourv = group1.createView( test::createView( "colourv", colour, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
	auto buffer = graph1.createBuffer( test::createBuffer( "buffer" ) );
	auto bufferv = graph1.createView( test::createView( "bufferv", buffer ) );
	auto & testPass1 = group1.createPass( "Mesh"
		, [&testCounts]( crg::FramePass const & framePass
			, crg::GraphContext & context
			, crg::RunnableGraph & runGraph )
		{
			return createDummy( testCounts
				, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
		} );
	auto coloura = testPass1.addOutputColourTarget( colourv );
	testPass1.addOutputStorageBuffer( bufferv, 0u );
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
	testPass2.addInputSampled( *coloura, 0u );

	auto runnable1 = graph1.compile( getContext() );
	test::checkRunnable( testCounts, runnable1 );

	auto runnable2 = graph2.compile( getContext() );
	test::checkRunnable( testCounts, runnable2 );
	testEnd()
}

TEST( Bases, PassGroups )
{
	testBegin( "testPassGroups" )
	crg::ResourceHandler handler;
	crg::FrameGraph graph{ handler, testCounts.testName };
	auto & group1 = graph.createPassGroup( "First" );
	auto colour = group1.createImage( test::createImage( "colour", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
	auto colourv = group1.createView( test::createView( "colourv", colour, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
	auto buffer = group1.createBuffer( test::createBuffer( "buffer" ) );
	auto bufferv = group1.createView( test::createView( "bufferv", buffer ) );
	auto & testPass1 = group1.createPass( "Mesh"
		, [&testCounts]( crg::FramePass const & framePass
			, crg::GraphContext & context
			, crg::RunnableGraph & runGraph )
		{
			return createDummy( testCounts
				, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
		} );
	auto coloura = testPass1.addOutputColourTarget( colourv );
	auto bufferAttach = testPass1.addOutputStorageBuffer( bufferv, 0u );
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
	testPass2.addInputSampled( *coloura, 0u );
	testPass2.addInputStorage( *bufferAttach, 0u );

	auto runnable2 = graph.compile( getContext() );
	test::checkRunnable( testCounts, runnable2 );
	testEnd()
}

TEST( Bases, ResourcesCache )
{
	testBegin( "testResourcesCache" )
	{
		crg::ResourceHandler handler;
		crg::RecordContext context( handler );
		checkThrow( context.getContext(), crg::Exception )
	}
	auto & context = getContext();
	crgUnregisterObject( context, VkBuffer( 1 ) );
	checkThrow( context.deduceMemoryType( 0u, 0u ), crg::Exception )
	{
		crg::ResourceHandler handler;
		auto sampled = handler.createImageId( test::createImage( "sampled", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto sampledv = handler.createViewId( test::createView( "sampledv", sampled, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
		auto buffer = handler.createBufferId( test::createBuffer( "buffer" ) );
		auto bufferv = handler.createViewId( test::createView( "bufferv", buffer, crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		handler.createImage( context, sampled );
		handler.createImageView( context, sampledv );
		handler.createBuffer( context, buffer );
		handler.createBufferView( context, bufferv );
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
		auto buffer = handler.createBufferId( test::createBuffer( "buffer" ) );
		auto bufferv = handler.createViewId( test::createView( "bufferv", buffer, crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		resources.createBuffer( context, buffer );
		resources.createBufferView( context, bufferv );
		resources.destroyBufferView( context, bufferv );
		resources.destroyBuffer( context, buffer );
	}
	{
		auto result = handler.createImageId( test::createImage( "result", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto resultv = handler.createViewId( test::createView( "resultv", result, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
		resources.createImage( context, result );
		resources.createImageView( context, resultv );
		resources.destroyImageView( resultv );
		resources.destroyImage( result );
		auto buffer = handler.createBufferId( test::createBuffer( "resbuffer" ) );
		auto bufferv = handler.createViewId( test::createView( "resbufferv", buffer, crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		resources.createBuffer( context, buffer );
		resources.createBufferView( context, bufferv );
		resources.destroyBufferView( bufferv );
		resources.destroyBuffer( buffer );
	}
	{
		auto result = handler.createImageId( test::createImage( "result", crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto resultv = handler.createViewId( test::createView( "resultv", result, crg::PixelFormat::eR16G16B16A16_SFLOAT, 0u, 1u, 0u, 1u ) );
		resources.createImage( context, result );
		resources.createImageView( context, resultv );
		resources.createSampler( context, crg::SamplerDesc{} );
		resources.createQuadTriVertexBuffer( context, false, {} );
		resources.createQuadTriVertexBuffer( context, true, {} );
		auto buffer = handler.createBufferId( test::createBuffer( "resbuffer" ) );
		auto bufferv = handler.createViewId( test::createView( "resbufferv", buffer, crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		resources.createBuffer( context, buffer );
		resources.createBufferView( context, bufferv );
		resources.destroyBufferView( context, bufferv );
		resources.destroyBuffer( context, buffer );
	}
	testEnd()
}

TEST( Bases, GraphNodes )
{
	testBegin( "testGraphNodes" )
	crg::ResourceHandler handler;
	crg::FrameGraph graph{ handler, testCounts.testName };
	crg::RootNode root{ graph };
	check( getFramePass( root ) == nullptr )

	auto & testPass = graph.createPass( "testPass"
		, [&testCounts]( crg::FramePass const & framePass
			, crg::GraphContext & context
			, crg::RunnableGraph & runGraph )
		{
				return createDummy( testCounts
					, framePass, context, runGraph, crg::PipelineStageFlags::eFragmentShader );
		} );
	crg::FramePassNode node{ testPass };
	check( getFramePass( node ) == &testPass )
	testEnd()
}

testSuiteMain()
