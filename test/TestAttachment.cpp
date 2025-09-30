#include "Common.hpp"

#include <RenderGraph/Attachment.hpp>
#include <RenderGraph/FrameGraph.hpp>
#include <RenderGraph/FramePass.hpp>
#include <RenderGraph/ResourceHandler.hpp>

namespace
{
	constexpr crg::SamplerDesc defaultSamplerDesc{};

	TEST( Attachment, SampledAttachment )
	{
		testBegin( "testSampledAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		auto tmpAttach1 = crg::Attachment::createDefault( view );
		auto tmpAttach2 = crg::Attachment::createDefault( view );
		tmpAttach2 = std::move( tmpAttach1 );
		tmpAttach1 = tmpAttach2;
		auto viewAttach = std::move( tmpAttach2 );
		pass.addInputSampled( viewAttach, 1u );
		require( pass.getSampled().size() == 1u )
		auto attachIt = pass.getSampled().begin();
		auto const & attachment = attachIt->second;
		check( attachment.attach->isImage() )
		check( attachment.attach->isInput() )
		check( !attachment.attach->isOutput() )
		check( !attachment.attach->isClearable() )
		check( !attachment.attach->isTransitionImageView() )
		check( !attachment.attach->isTransferImageView() )
		check( !attachment.attach->imageAttach.isDepthStencilTarget() )
		check( !attachment.attach->imageAttach.isDepthTarget() )
		check( !attachment.attach->imageAttach.isStencilTarget() )
		check( !attachment.attach->imageAttach.isStencilInputTarget() )
		check( !attachment.attach->imageAttach.isStencilOutputTarget() )
		check( attachment.attach->name == pass.getGroupName() + "/" + view.data->name + "/Spl" )
		checkEqual( attachment.attach->getLoadOp(), crg::AttachmentLoadOp::eDontCare )
		checkEqual( attachment.attach->getStoreOp(), crg::AttachmentStoreOp::eDontCare )
		checkEqual( attachment.attach->getStencilLoadOp(), crg::AttachmentLoadOp::eDontCare )
		checkEqual( attachment.attach->getStencilStoreOp(), crg::AttachmentStoreOp::eDontCare )
		check( attachment.attach->view() == view )
		check( attachIt->first == 1u )
		check( attachment.sampler == defaultSamplerDesc )
		checkEqual( attachment.attach->getImageLayout( false ), crg::ImageLayout::eShaderReadOnly )
		checkEqual( attachment.attach->getImageLayout( true ), crg::ImageLayout::eShaderReadOnly )
		testEnd()
	}

	TEST( Attachment, SampledImage )
	{
		testBegin( "testSampledImage" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addInputSampledImage( view, 1u );
		require( pass.getSampled().size() == 1u )
		auto attachIt = pass.getSampled().begin();
		auto const & attachment = attachIt->second;
		check( attachment.attach->isImage() )
		check( attachment.attach->isInput() )
		check( !attachment.attach->isOutput() )
		check( !attachment.attach->isClearable() )
		check( !attachment.attach->isTransitionImageView() )
		check( !attachment.attach->isTransferImageView() )
		check( !attachment.attach->imageAttach.isDepthStencilTarget() )
		check( !attachment.attach->imageAttach.isDepthTarget() )
		check( !attachment.attach->imageAttach.isStencilTarget() )
		check( !attachment.attach->imageAttach.isStencilInputTarget() )
		check( !attachment.attach->imageAttach.isStencilOutputTarget() )
		check( attachment.attach->name == pass.getGroupName() + "/" + view.data->name + "/Spl" )
		checkEqual( attachment.attach->getLoadOp(), crg::AttachmentLoadOp::eDontCare )
		checkEqual( attachment.attach->getStoreOp(), crg::AttachmentStoreOp::eDontCare )
		checkEqual( attachment.attach->getStencilLoadOp(), crg::AttachmentLoadOp::eDontCare )
		checkEqual( attachment.attach->getStencilStoreOp(), crg::AttachmentStoreOp::eDontCare )
		check( attachment.attach->view() == view )
		check( attachIt->first == 1u )
		check( attachment.sampler == defaultSamplerDesc )
		checkEqual( attachment.attach->getImageLayout( false ), crg::ImageLayout::eShaderReadOnly )
		checkEqual( attachment.attach->getImageLayout( true ), crg::ImageLayout::eShaderReadOnly )
		testEnd()
	}

	TEST( Attachment, ImplicitColourAttachment )
	{
		testBegin( "testImplicitColourAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( crg::ImageData{ "Test", crg::ImageCreateFlags::eNone, crg::ImageType::e3D, crg::PixelFormat::eR32G32B32A32_SFLOAT, { 1024, 1024, 1024 }, crg::ImageUsageFlags::eSampled, 1u, 1u } );
		auto range = crg::getVirtualRange( image, crg::ImageViewType::e3D, { crg::ImageAspectFlags::eColor, 0u, 1u, 0u, 1u } );
		check( range.baseArrayLayer == 0 )
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		auto viewAttach = crg::Attachment::createDefault( view );
		auto imageLayout = crg::ImageLayout::eShaderReadOnly;
		pass.addImplicit( viewAttach, imageLayout );
		require( pass.getInputs().size() == 1u )
		auto attachIt = pass.getInputs().begin();
		auto const & attachment = attachIt->second;
		check( attachment->isImage() )
		check( attachment->isInput() )
		check( !attachment->isOutput() )
		check( !attachment->isClearable() )
		check( attachment->isTransitionImageView() )
		check( !attachment->isTransferImageView() )
		check( !attachment->imageAttach.isDepthStencilTarget() )
		check( !attachment->imageAttach.isDepthTarget() )
		check( !attachment->imageAttach.isStencilTarget() )
		check( !attachment->imageAttach.isStencilInputTarget() )
		check( !attachment->imageAttach.isStencilOutputTarget() )
		checkEqual( attachment->name, pass.getGroupName() + "/" + view.data->name + "/Impl" )
		checkEqual( attachment->getLoadOp(), crg::AttachmentLoadOp::eDontCare )
		checkEqual( attachment->getStoreOp(), crg::AttachmentStoreOp::eDontCare )
		checkEqual( attachment->getStencilLoadOp(), crg::AttachmentLoadOp::eDontCare )
		checkEqual( attachment->getStencilStoreOp(), crg::AttachmentStoreOp::eDontCare )
		check( attachment->view() == view )
		checkEqual( attachment->getImageLayout( false ), imageLayout )
		checkEqual( attachment->getImageLayout( true ), imageLayout )
		testEnd()
	}

	TEST( Attachment, ImplicitDepthAttachment )
	{
		testBegin( "testImplicitDepthAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eD32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		auto viewAttach = crg::Attachment::createDefault( view );
		auto imageLayout = crg::ImageLayout::ePreinitialized;
		pass.addImplicit( viewAttach, imageLayout );
		require( pass.getInputs().size() == 1u )
		auto attachIt = pass.getInputs().begin();
		auto const & attachment = attachIt->second;
		check( attachment->isImage() )
		check( attachment->isInput() )
		check( !attachment->isOutput() )
		check( !attachment->isClearable() )
		check( attachment->isTransitionImageView() )
		check( !attachment->isTransferImageView() )
		check( !attachment->imageAttach.isDepthStencilTarget() )
		check( !attachment->imageAttach.isDepthTarget() )
		check( !attachment->imageAttach.isStencilTarget() )
		check( !attachment->imageAttach.isStencilInputTarget() )
		check( !attachment->imageAttach.isStencilOutputTarget() )
		checkEqual( attachment->name, pass.getGroupName() + "/" + view.data->name + "/Impl" )
		checkEqual( attachment->getLoadOp(), crg::AttachmentLoadOp::eDontCare )
		checkEqual( attachment->getStoreOp(), crg::AttachmentStoreOp::eDontCare )
		checkEqual( attachment->getStencilLoadOp(), crg::AttachmentLoadOp::eDontCare )
		checkEqual( attachment->getStencilStoreOp(), crg::AttachmentStoreOp::eDontCare )
		check( attachment->view() == view )
		checkEqual( attachment->getImageLayout( false ), imageLayout )
		checkEqual( attachment->getImageLayout( true ), imageLayout )
		testEnd()
	}

	TEST( Attachment, ImplicitDepthStencilAttachment )
	{
		testBegin( "testImplicitDepthStencilAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eD32_SFLOAT_S8_UINT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		auto viewAttach = crg::Attachment::createDefault( view );
		auto imageLayout = crg::ImageLayout::eShaderReadOnly;
		pass.addImplicit( viewAttach, imageLayout );
		require( pass.getInputs().size() == 1u )
		auto attachIt = pass.getInputs().begin();
		auto const & attachment = attachIt->second;
		check( attachment->isImage() )
		check( attachment->isInput() )
		check( !attachment->isOutput() )
		check( !attachment->isClearable() )
		check( attachment->isTransitionImageView() )
		check( !attachment->isTransferImageView() )
		check( !attachment->imageAttach.isDepthStencilTarget() )
		check( !attachment->imageAttach.isDepthTarget() )
		check( !attachment->imageAttach.isStencilTarget() )
		check( !attachment->imageAttach.isStencilInputTarget() )
		check( !attachment->imageAttach.isStencilOutputTarget() )
		checkEqual( attachment->name, pass.getGroupName() + "/" + view.data->name + "/Impl" )
		checkEqual( attachment->getLoadOp(), crg::AttachmentLoadOp::eDontCare )
		checkEqual( attachment->getStoreOp(), crg::AttachmentStoreOp::eDontCare )
		checkEqual( attachment->getStencilLoadOp(), crg::AttachmentLoadOp::eDontCare )
		checkEqual( attachment->getStencilStoreOp(), crg::AttachmentStoreOp::eDontCare )
		check( attachment->view() == view )
		checkEqual( attachment->getImageLayout( false ), imageLayout )
		checkEqual( attachment->getImageLayout( true ), imageLayout )
		testEnd()
	}

	TEST( Attachment, InStorageAttachment )
	{
		testBegin( "testInStorageAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		auto viewAttach = crg::Attachment::createDefault( view );
		pass.addInputStorage( viewAttach, 1u );
		require( pass.getInputs().size() == 1u )
		auto attachIt = pass.getInputs().begin();
		auto const & attachment = attachIt->second;
		check( attachment->isImage() )
		check( attachment->isInput() )
		check( !attachment->isOutput() )
		check( !attachment->isClearable() )
		check( !attachment->isTransitionImageView() )
		check( !attachment->isTransferImageView() )
		check( !attachment->imageAttach.isDepthStencilTarget() )
		check( !attachment->imageAttach.isDepthTarget() )
		check( !attachment->imageAttach.isStencilTarget() )
		check( !attachment->imageAttach.isStencilInputTarget() )
		check( !attachment->imageAttach.isStencilOutputTarget() )
		checkEqual( attachment->name, pass.getGroupName() + "/" + view.data->name + "/IStr" )
		checkEqual( attachment->getLoadOp(), crg::AttachmentLoadOp::eDontCare )
		checkEqual( attachment->getStoreOp(), crg::AttachmentStoreOp::eDontCare )
		checkEqual( attachment->getStencilLoadOp(), crg::AttachmentLoadOp::eDontCare )
		checkEqual( attachment->getStencilStoreOp(), crg::AttachmentStoreOp::eDontCare )
		check( attachment->view() == view )
		check( attachIt->first == 1u )
		checkEqual( attachment->getImageLayout( false ), crg::ImageLayout::eGeneral )
		checkEqual( attachment->getImageLayout( true ), crg::ImageLayout::eGeneral )
		testEnd()
	}

	TEST( Attachment, InStorageImage )
	{
		testBegin( "testInStorageImage" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addInputStorageImage( view, 1u );
		require( pass.getInputs().size() == 1u )
		auto attachIt = pass.getInputs().begin();
		auto const & attachment = attachIt->second;
		check( attachment->isImage() )
		check( attachment->isInput() )
		check( !attachment->isOutput() )
		check( !attachment->isClearable() )
		check( !attachment->isTransitionImageView() )
		check( !attachment->isTransferImageView() )
		check( !attachment->imageAttach.isDepthStencilTarget() )
		check( !attachment->imageAttach.isDepthTarget() )
		check( !attachment->imageAttach.isStencilTarget() )
		check( !attachment->imageAttach.isStencilInputTarget() )
		check( !attachment->imageAttach.isStencilOutputTarget() )
		checkEqual( attachment->name, pass.getGroupName() + "/" + view.data->name + "/IStr" )
		checkEqual( attachment->getLoadOp(), crg::AttachmentLoadOp::eDontCare )
		checkEqual( attachment->getStoreOp(), crg::AttachmentStoreOp::eDontCare )
		checkEqual( attachment->getStencilLoadOp(), crg::AttachmentLoadOp::eDontCare )
		checkEqual( attachment->getStencilStoreOp(), crg::AttachmentStoreOp::eDontCare )
		check( attachment->view() == view )
		check( attachIt->first == 1u )
		checkEqual( attachment->getImageLayout( false ), crg::ImageLayout::eGeneral )
		checkEqual( attachment->getImageLayout( true ), crg::ImageLayout::eGeneral )
		testEnd()
	}

	TEST( Attachment, InStorageBuffer )
	{
		testBegin( "testInStorageBuffer" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto buffer = graph.createBuffer( test::createBuffer( "Test" ) );
		auto view = graph.createView( test::createView( "Test", buffer ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addInputStorageBuffer( view, 1u );
		require( pass.getInputs().size() == 1u )
		auto attachIt = pass.getInputs().begin();
		auto const & attachment = attachIt->second;
		check( attachment->isBuffer() )
		check( attachment->isInput() )
		check( !attachment->isOutput() )
		check( !attachment->isClearable() )
		check( !attachment->isTransitionImageView() )
		check( !attachment->isTransferImageView() )
		check( attachment->isStorageBuffer() )
		check( attachment->bufferAttach.isStorage() )
		checkEqual( attachment->name, pass.getGroupName() + "/" + view.data->name + "/SB" )
		check( attachment->buffer() == view )
		check( attachIt->first == 1u )
		testEnd()
	}

	TEST( Attachment, OutStorageAttachment )
	{
		testBegin( "testOutStorageAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addOutputStorageImage( view, 1u );
		require( pass.getOutputs().size() == 1u )
		auto attachIt = pass.getOutputs().begin();
		auto const & attachment = attachIt->second;
		check( attachment->isImage() )
		check( !attachment->isInput() )
		check( attachment->isOutput() )
		check( !attachment->isClearable() )
		check( !attachment->isTransitionImageView() )
		check( !attachment->isTransferImageView() )
		check( !attachment->imageAttach.isDepthStencilTarget() )
		check( !attachment->imageAttach.isDepthTarget() )
		check( !attachment->imageAttach.isStencilTarget() )
		check( !attachment->imageAttach.isStencilInputTarget() )
		check( !attachment->imageAttach.isStencilOutputTarget() )
		checkEqual( attachment->name, pass.getGroupName() + "/" + view.data->name + "/OStr" )
		checkEqual( attachment->getLoadOp(), crg::AttachmentLoadOp::eDontCare )
		checkEqual( attachment->getStoreOp(), crg::AttachmentStoreOp::eDontCare )
		checkEqual( attachment->getStencilLoadOp(), crg::AttachmentLoadOp::eDontCare )
		checkEqual( attachment->getStencilStoreOp(), crg::AttachmentStoreOp::eDontCare )
		check( attachment->view() == view )
		check( attachIt->first == 1u )
		checkEqual( attachment->getImageLayout( false ), crg::ImageLayout::eGeneral )
		checkEqual( attachment->getImageLayout( true ), crg::ImageLayout::eGeneral )
		testEnd()
	}

	TEST( Attachment, ClearOutStorageAttachment )
	{
		testBegin( "testClearOutStorageAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addClearableOutputStorageImage( view, 1u );
		require( pass.getOutputs().size() == 1u )
		auto attachIt = pass.getOutputs().begin();
		auto const & attachment = attachIt->second;
		check( attachment->isImage() )
		check( !attachment->isInput() )
		check( attachment->isOutput() )
		check( attachment->isClearable() )
		check( !attachment->isTransitionImageView() )
		check( !attachment->isTransferImageView() )
		check( !attachment->imageAttach.isDepthStencilTarget() )
		check( !attachment->imageAttach.isDepthTarget() )
		check( !attachment->imageAttach.isStencilTarget() )
		check( !attachment->imageAttach.isStencilInputTarget() )
		check( !attachment->imageAttach.isStencilOutputTarget() )
		checkEqual( attachment->name, pass.getGroupName() + "/" + view.data->name + "/COStr" )
		checkEqual( attachment->getLoadOp(), crg::AttachmentLoadOp::eDontCare )
		checkEqual( attachment->getStoreOp(), crg::AttachmentStoreOp::eDontCare )
		checkEqual( attachment->getStencilLoadOp(), crg::AttachmentLoadOp::eDontCare )
		checkEqual( attachment->getStencilStoreOp(), crg::AttachmentStoreOp::eDontCare )
		check( attachment->view() == view )
		check( attachIt->first == 1u )
		checkEqual( attachment->getImageLayout( false ), crg::ImageLayout::eGeneral )
		checkEqual( attachment->getImageLayout( true ), crg::ImageLayout::eGeneral )
		testEnd()
	}

	TEST( Attachment, InOutStorageAttachment )
	{
		testBegin( "testInOutStorageAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		auto viewAttach = crg::Attachment::createDefault( view );
		pass.addInOutStorage( viewAttach, 1u );
		require( pass.getInouts().size() == 1u )
		auto attachIt = pass.getInouts().begin();
		auto const & attachment = attachIt->second;
		check( attachment->isImage() )
		check( attachment->isInput() )
		check( attachment->isOutput() )
		check( !attachment->isClearable() )
		check( !attachment->isTransitionImageView() )
		check( !attachment->isTransferImageView() )
		check( !attachment->imageAttach.isDepthStencilTarget() )
		check( !attachment->imageAttach.isDepthTarget() )
		check( !attachment->imageAttach.isStencilTarget() )
		check( !attachment->imageAttach.isStencilInputTarget() )
		check( !attachment->imageAttach.isStencilOutputTarget() )
		checkEqual( attachment->name, pass.getGroupName() + "/" + view.data->name + "/IOStr" )
		checkEqual( attachment->getLoadOp(), crg::AttachmentLoadOp::eDontCare )
		checkEqual( attachment->getStoreOp(), crg::AttachmentStoreOp::eDontCare )
		checkEqual( attachment->getStencilLoadOp(), crg::AttachmentLoadOp::eDontCare )
		checkEqual( attachment->getStencilStoreOp(), crg::AttachmentStoreOp::eDontCare )
		check( attachment->view() == view )
		check( attachIt->first == 1u )
		checkEqual( attachment->getImageLayout( false ), crg::ImageLayout::eGeneral )
		checkEqual( attachment->getImageLayout( true ), crg::ImageLayout::eGeneral )
		testEnd()
	}

	TEST( Attachment, InTransferAttachment )
	{
		testBegin( "testInTransferAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		auto viewAttach = crg::Attachment::createDefault( view );
		pass.addInputTransfer( viewAttach );
		require( pass.getInputs().size() == 1u )
		auto attachIt = pass.getInputs().begin();
		auto const & attachment = attachIt->second;
		check( attachment->isImage() )
		check( attachment->isInput() )
		check( !attachment->isOutput() )
		check( !attachment->isClearable() )
		check( !attachment->isTransitionImageView() )
		check( attachment->isTransferImageView() )
		check( !attachment->imageAttach.isDepthStencilTarget() )
		check( !attachment->imageAttach.isDepthTarget() )
		check( !attachment->imageAttach.isStencilTarget() )
		check( !attachment->imageAttach.isStencilInputTarget() )
		check( !attachment->imageAttach.isStencilOutputTarget() )
		checkEqual( attachment->name, pass.getGroupName() + "/" + view.data->name + "/ITrf" )
		checkEqual( attachment->getLoadOp(), crg::AttachmentLoadOp::eDontCare )
		checkEqual( attachment->getStoreOp(), crg::AttachmentStoreOp::eDontCare )
		checkEqual( attachment->getStencilLoadOp(), crg::AttachmentLoadOp::eDontCare )
		checkEqual( attachment->getStencilStoreOp(), crg::AttachmentStoreOp::eDontCare )
		check( attachment->view() == view )
		checkEqual( attachment->getImageLayout( false ), crg::ImageLayout::eTransferSrc )
		checkEqual( attachment->getImageLayout( true ), crg::ImageLayout::eTransferSrc )
		testEnd()
	}

	TEST( Attachment, InTransferImage )
	{
		testBegin( "testInTransferImage" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addInputTransferImage( view );
		require( pass.getInputs().size() == 1u )
		auto attachIt = pass.getInputs().begin();
		auto const & attachment = attachIt->second;
		check( attachment->isImage() )
		check( attachment->isInput() )
		check( !attachment->isOutput() )
		check( !attachment->isClearable() )
		check( !attachment->isTransitionImageView() )
		check( attachment->isTransferImageView() )
		check( !attachment->imageAttach.isDepthStencilTarget() )
		check( !attachment->imageAttach.isDepthTarget() )
		check( !attachment->imageAttach.isStencilTarget() )
		check( !attachment->imageAttach.isStencilInputTarget() )
		check( !attachment->imageAttach.isStencilOutputTarget() )
		checkEqual( attachment->name, pass.getGroupName() + "/" + view.data->name + "/ITrf" )
		checkEqual( attachment->getLoadOp(), crg::AttachmentLoadOp::eDontCare )
		checkEqual( attachment->getStoreOp(), crg::AttachmentStoreOp::eDontCare )
		checkEqual( attachment->getStencilLoadOp(), crg::AttachmentLoadOp::eDontCare )
		checkEqual( attachment->getStencilStoreOp(), crg::AttachmentStoreOp::eDontCare )
		check( attachment->view() == view )
		checkEqual( attachment->getImageLayout( false ), crg::ImageLayout::eTransferSrc )
		checkEqual( attachment->getImageLayout( true ), crg::ImageLayout::eTransferSrc )
		testEnd()
	}

	TEST( Attachment, InTransferBuffer )
	{
		testBegin( "testInTransferBuffer" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto buffer = graph.createBuffer( test::createBuffer( "Test" ) );
		auto view = graph.createView( test::createView( "Test", buffer ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addInputTransferBuffer( view );
		require( pass.getInputs().size() == 1u )
		auto attachIt = pass.getInputs().begin();
		auto const & attachment = attachIt->second;
		check( attachment->isBuffer() )
		check( attachment->isInput() )
		check( !attachment->isOutput() )
		check( !attachment->isClearable() )
		check( !attachment->isTransitionImageView() )
		check( !attachment->isTransferImageView() )
		check( attachment->isTransferBuffer() )
		checkEqual( attachment->name, pass.getGroupName() + "/" + view.data->name + "/ITrf" )
		check( attachment->buffer() == view )
		testEnd()
	}

	TEST( Attachment, OutTransferAttachment )
	{
		testBegin( "testOutTransferAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addOutputTransferImage( view );
		require( pass.getOutputs().size() == 1u )
		auto attachIt = pass.getOutputs().begin();
		auto const & attachment = attachIt->second;
		check( attachment->isImage() )
		check( !attachment->isInput() )
		check( attachment->isOutput() )
		check( !attachment->isClearable() )
		check( !attachment->isTransitionImageView() )
		check( attachment->isTransferImageView() )
		check( !attachment->imageAttach.isDepthStencilTarget() )
		check( !attachment->imageAttach.isDepthTarget() )
		check( !attachment->imageAttach.isStencilTarget() )
		check( !attachment->imageAttach.isStencilInputTarget() )
		check( !attachment->imageAttach.isStencilOutputTarget() )
		checkEqual( attachment->name, pass.getGroupName() + "/" + view.data->name + "/OT" )
		checkEqual( attachment->getLoadOp(), crg::AttachmentLoadOp::eDontCare )
		checkEqual( attachment->getStoreOp(), crg::AttachmentStoreOp::eDontCare )
		checkEqual( attachment->getStencilLoadOp(), crg::AttachmentLoadOp::eDontCare )
		checkEqual( attachment->getStencilStoreOp(), crg::AttachmentStoreOp::eDontCare )
		check( attachment->view() == view )
		checkEqual( attachment->getImageLayout( false ), crg::ImageLayout::eTransferDst )
		checkEqual( attachment->getImageLayout( true ), crg::ImageLayout::eTransferDst )
		testEnd()
	}

	TEST( Attachment, InOutTransferAttachment )
	{
		testBegin( "testInOutTransferAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		auto viewAttach = crg::Attachment::createDefault( view );
		pass.addInOutTransfer( viewAttach );
		require( pass.getInouts().size() == 1u )
		auto attachIt = pass.getInouts().begin();
		auto const & attachment = attachIt->second;
		check( attachment->isImage() )
		check( attachment->isInput() )
		check( attachment->isOutput() )
		check( !attachment->isClearable() )
		check( !attachment->isTransitionImageView() )
		check( attachment->isTransferImageView() )
		check( !attachment->imageAttach.isDepthStencilTarget() )
		check( !attachment->imageAttach.isDepthTarget() )
		check( !attachment->imageAttach.isStencilTarget() )
		check( !attachment->imageAttach.isStencilInputTarget() )
		check( !attachment->imageAttach.isStencilOutputTarget() )
		checkEqual( attachment->name, pass.getGroupName() + "/" + view.data->name + "/IOTrf" )
		checkEqual( attachment->getLoadOp(), crg::AttachmentLoadOp::eDontCare )
		checkEqual( attachment->getStoreOp(), crg::AttachmentStoreOp::eDontCare )
		checkEqual( attachment->getStencilLoadOp(), crg::AttachmentLoadOp::eDontCare )
		checkEqual( attachment->getStencilStoreOp(), crg::AttachmentStoreOp::eDontCare )
		check( attachment->view() == view )
		checkEqual( attachment->getImageLayout( false ), crg::ImageLayout::eTransferDst )
		checkEqual( attachment->getImageLayout( true ), crg::ImageLayout::eTransferDst )
		testEnd()
	}

	TEST( Attachment, InColourAttachment )
	{
		testBegin( "testInColourAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		auto viewAttach = crg::Attachment::createDefault( view );
		pass.addInputColourTarget( viewAttach );
		require( pass.getTargets().size() == 1u )
		auto const & attachment = pass.getTargets()[0];
		check( attachment->isImage() )
		check( attachment->isInput() )
		check( !attachment->isOutput() )
		check( !attachment->isClearable() )
		check( attachment->isColourImageTarget() )
		check( !attachment->isColourInOutImageTarget() )
		check( attachment->isColourInputImageTarget() )
		check( !attachment->isColourOutputImageTarget() )
		check( !attachment->isTransitionImageView() )
		check( !attachment->isTransferImageView() )
		check( attachment->imageAttach.isColourTarget() )
		check( !attachment->imageAttach.isDepthStencilTarget() )
		check( !attachment->imageAttach.isDepthTarget() )
		check( !attachment->imageAttach.isStencilTarget() )
		check( !attachment->imageAttach.isStencilInputTarget() )
		check( !attachment->imageAttach.isStencilOutputTarget() )
		checkEqual( attachment->name, pass.getGroupName() + "/" + view.data->name + "/IRcl" )
		checkEqual( attachment->getLoadOp(), crg::AttachmentLoadOp::eLoad )
		checkEqual( attachment->getStoreOp(), crg::AttachmentStoreOp::eDontCare )
		checkEqual( attachment->getStencilLoadOp(), crg::AttachmentLoadOp::eDontCare )
		checkEqual( attachment->getStencilStoreOp(), crg::AttachmentStoreOp::eDontCare )
		check( attachment->view() == view )
		checkEqual( attachment->getImageLayout( false ), crg::ImageLayout::eColorAttachment )
		checkEqual( attachment->getImageLayout( true ), crg::ImageLayout::eColorAttachment )
		testEnd()
	}

	TEST( Attachment, InColourImage )
	{
		testBegin( "testInColourImage" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addInputColourTargetImage( view );
		require( pass.getTargets().size() == 1u )
		auto const & attachment = pass.getTargets()[0];
		check( attachment->isImage() )
		check( attachment->isInput() )
		check( !attachment->isOutput() )
		check( !attachment->isClearable() )
		check( attachment->isColourImageTarget() )
		check( !attachment->isColourInOutImageTarget() )
		check( attachment->isColourInputImageTarget() )
		check( !attachment->isColourOutputImageTarget() )
		check( !attachment->isTransitionImageView() )
		check( !attachment->isTransferImageView() )
		check( attachment->imageAttach.isColourTarget() )
		check( !attachment->imageAttach.isDepthStencilTarget() )
		check( !attachment->imageAttach.isDepthTarget() )
		check( !attachment->imageAttach.isStencilTarget() )
		check( !attachment->imageAttach.isStencilInputTarget() )
		check( !attachment->imageAttach.isStencilOutputTarget() )
		checkEqual( attachment->name, pass.getGroupName() + "/" + view.data->name + "/IRcl" )
		checkEqual( attachment->getLoadOp(), crg::AttachmentLoadOp::eLoad )
		checkEqual( attachment->getStoreOp(), crg::AttachmentStoreOp::eDontCare )
		checkEqual( attachment->getStencilLoadOp(), crg::AttachmentLoadOp::eDontCare )
		checkEqual( attachment->getStencilStoreOp(), crg::AttachmentStoreOp::eDontCare )
		check( attachment->view() == view )
		checkEqual( attachment->getImageLayout( false ), crg::ImageLayout::eColorAttachment )
		checkEqual( attachment->getImageLayout( true ), crg::ImageLayout::eColorAttachment )
		testEnd()
	}

	TEST( Attachment, OutColourAttachment )
	{
		testBegin( "testOutColourAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addOutputColourTarget( view );
		require( pass.getTargets().size() == 1u )
		auto const & attachment = pass.getTargets()[0];
		check( attachment->isImage() )
		check( !attachment->isInput() )
		check( attachment->isOutput() )
		check( !attachment->isClearable() )
		check( attachment->isColourImageTarget() )
		check( !attachment->isColourInOutImageTarget() )
		check( !attachment->isColourInputImageTarget() )
		check( attachment->isColourOutputImageTarget() )
		check( !attachment->isTransitionImageView() )
		check( !attachment->isTransferImageView() )
		check( attachment->imageAttach.isColourTarget() )
		check( !attachment->imageAttach.isDepthStencilTarget() )
		check( !attachment->imageAttach.isDepthTarget() )
		check( !attachment->imageAttach.isStencilTarget() )
		check( !attachment->imageAttach.isStencilInputTarget() )
		check( !attachment->imageAttach.isStencilOutputTarget() )
		checkEqual( attachment->name, pass.getGroupName() + "/" + view.data->name + "/ORcl" )
		checkEqual( attachment->getLoadOp(), crg::AttachmentLoadOp::eClear )
		checkEqual( attachment->getStoreOp(), crg::AttachmentStoreOp::eStore )
		checkEqual( attachment->getStencilLoadOp(), crg::AttachmentLoadOp::eDontCare )
		checkEqual( attachment->getStencilStoreOp(), crg::AttachmentStoreOp::eDontCare )
		check( attachment->view() == view )
		checkEqual( attachment->getImageLayout( false ), crg::ImageLayout::eColorAttachment )
		checkEqual( attachment->getImageLayout( true ), crg::ImageLayout::eColorAttachment )
		testEnd()
	}

	TEST( Attachment, InOutColourAttachment )
	{
		testBegin( "testInOutColourAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		auto viewAttach = crg::Attachment::createDefault( view );
		pass.addInOutColourTarget( viewAttach );
		require( pass.getTargets().size() == 1u )
		auto const & attachment = pass.getTargets()[0];
		check( attachment->isImage() )
		check( attachment->isInput() )
		check( attachment->isOutput() )
		check( !attachment->isClearable() )
		check( attachment->isColourImageTarget() )
		check( attachment->isColourInOutImageTarget() )
		check( attachment->isColourInputImageTarget() )
		check( attachment->isColourOutputImageTarget() )
		check( !attachment->isTransitionImageView() )
		check( !attachment->isTransferImageView() )
		check( attachment->imageAttach.isColourTarget() )
		check( !attachment->imageAttach.isDepthStencilTarget() )
		check( !attachment->imageAttach.isDepthTarget() )
		check( !attachment->imageAttach.isStencilTarget() )
		check( !attachment->imageAttach.isStencilInputTarget() )
		check( !attachment->imageAttach.isStencilOutputTarget() )
		checkEqual( attachment->name, pass.getGroupName() + "/" + view.data->name + "/IORcl" )
		checkEqual( attachment->getLoadOp(), crg::AttachmentLoadOp::eLoad )
		checkEqual( attachment->getStoreOp(), crg::AttachmentStoreOp::eStore )
		checkEqual( attachment->getStencilLoadOp(), crg::AttachmentLoadOp::eDontCare )
		checkEqual( attachment->getStencilStoreOp(), crg::AttachmentStoreOp::eDontCare )
		check( attachment->view() == view )
		checkEqual( attachment->getImageLayout( false ), crg::ImageLayout::eColorAttachment )
		checkEqual( attachment->getImageLayout( true ), crg::ImageLayout::eColorAttachment )
		testEnd()
	}

	TEST( Attachment, InDepthAttachment )
	{
		testBegin( "testInDepthAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eD32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		auto viewAttach = crg::Attachment::createDefault( view );
		pass.addInputDepthTarget( viewAttach );
		require( !pass.getTargets().empty() )
		auto const & attachment = pass.getTargets()[0];
		check( attachment->isImage() )
		check( attachment->isInput() )
		check( !attachment->isOutput() )
		check( !attachment->isClearable() )
		check( !attachment->isColourImageTarget() )
		check( !attachment->isColourInOutImageTarget() )
		check( !attachment->isColourInputImageTarget() )
		check( !attachment->isColourOutputImageTarget() )
		check( !attachment->isTransitionImageView() )
		check( !attachment->isTransferImageView() )
		check( !attachment->imageAttach.isColourTarget() )
		check( !attachment->imageAttach.isDepthStencilTarget() )
		check( attachment->imageAttach.isDepthTarget() )
		check( !attachment->imageAttach.isStencilTarget() )
		check( !attachment->imageAttach.isStencilInputTarget() )
		check( !attachment->imageAttach.isStencilOutputTarget() )
		checkEqual( attachment->name, pass.getGroupName() + "/" + view.data->name + "/IRdp" )
		checkEqual( attachment->getLoadOp(), crg::AttachmentLoadOp::eLoad )
		checkEqual( attachment->getStoreOp(), crg::AttachmentStoreOp::eDontCare )
		checkEqual( attachment->getStencilLoadOp(), crg::AttachmentLoadOp::eDontCare )
		checkEqual( attachment->getStencilStoreOp(), crg::AttachmentStoreOp::eDontCare )
		check( attachment->view() == view )
		checkEqual( attachment->getImageLayout( false ), crg::ImageLayout::eDepthStencilReadOnly )
		checkEqual( attachment->getImageLayout( true ), crg::ImageLayout::eDepthReadOnly )
		testEnd()
	}

	TEST( Attachment, InDepthImage )
	{
		testBegin( "testInDepthImage" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eD32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addInputDepthTargetImage( view );
		require( !pass.getTargets().empty() )
		auto const & attachment = pass.getTargets()[0];
		check( attachment->isImage() )
		check( attachment->isInput() )
		check( !attachment->isOutput() )
		check( !attachment->isClearable() )
		check( !attachment->isColourImageTarget() )
		check( !attachment->isColourInOutImageTarget() )
		check( !attachment->isColourInputImageTarget() )
		check( !attachment->isColourOutputImageTarget() )
		check( !attachment->isTransitionImageView() )
		check( !attachment->isTransferImageView() )
		check( !attachment->imageAttach.isColourTarget() )
		check( !attachment->imageAttach.isDepthStencilTarget() )
		check( attachment->imageAttach.isDepthTarget() )
		check( !attachment->imageAttach.isStencilTarget() )
		check( !attachment->imageAttach.isStencilInputTarget() )
		check( !attachment->imageAttach.isStencilOutputTarget() )
		checkEqual( attachment->name, pass.getGroupName() + "/" + view.data->name + "/IRdp" )
		checkEqual( attachment->getLoadOp(), crg::AttachmentLoadOp::eLoad )
		checkEqual( attachment->getStoreOp(), crg::AttachmentStoreOp::eDontCare )
		checkEqual( attachment->getStencilLoadOp(), crg::AttachmentLoadOp::eDontCare )
		checkEqual( attachment->getStencilStoreOp(), crg::AttachmentStoreOp::eDontCare )
		check( attachment->view() == view )
		checkEqual( attachment->getImageLayout( false ), crg::ImageLayout::eDepthStencilReadOnly )
		checkEqual( attachment->getImageLayout( true ), crg::ImageLayout::eDepthReadOnly )
		testEnd()
	}

	TEST( Attachment, OutDepthAttachment )
	{
		testBegin( "testOutDepthAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eD32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addOutputDepthTarget( view );
		require( !pass.getTargets().empty() )
		auto const & attachment = pass.getTargets()[0];
		check( attachment->isImage() )
		check( !attachment->isInput() )
		check( attachment->isOutput() )
		check( !attachment->isClearable() )
		check( !attachment->isColourImageTarget() )
		check( !attachment->isColourInOutImageTarget() )
		check( !attachment->isColourInputImageTarget() )
		check( !attachment->isColourOutputImageTarget() )
		check( !attachment->isTransitionImageView() )
		check( !attachment->isTransferImageView() )
		check( !attachment->imageAttach.isColourTarget() )
		check( !attachment->imageAttach.isDepthStencilTarget() )
		check( attachment->imageAttach.isDepthTarget() )
		check( !attachment->imageAttach.isStencilTarget() )
		check( !attachment->imageAttach.isStencilInputTarget() )
		check( !attachment->imageAttach.isStencilOutputTarget() )
		checkEqual( attachment->name, pass.getGroupName() + "/" + view.data->name + "/ORdp" )
		checkEqual( attachment->getLoadOp(), crg::AttachmentLoadOp::eClear )
		checkEqual( attachment->getStoreOp(), crg::AttachmentStoreOp::eStore )
		checkEqual( attachment->getStencilLoadOp(), crg::AttachmentLoadOp::eDontCare )
		checkEqual( attachment->getStencilStoreOp(), crg::AttachmentStoreOp::eDontCare )
		check( attachment->view() == view )
		checkEqual( attachment->getImageLayout( false ), crg::ImageLayout::eDepthStencilAttachment )
		checkEqual( attachment->getImageLayout( true ), crg::ImageLayout::eDepthAttachment )
		testEnd()
	}

	TEST( Attachment, InOutDepthAttachment )
	{
		testBegin( "testInOutDepthAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eD32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		auto viewAttach = crg::Attachment::createDefault( view );
		pass.addInOutDepthTarget( viewAttach );
		require( !pass.getTargets().empty() )
		auto const & attachment = pass.getTargets()[0];
		check( attachment->isImage() )
		check( attachment->isInput() )
		check( attachment->isOutput() )
		check( !attachment->isClearable() )
		check( !attachment->isColourImageTarget() )
		check( !attachment->isColourInOutImageTarget() )
		check( !attachment->isColourInputImageTarget() )
		check( !attachment->isColourOutputImageTarget() )
		check( !attachment->isTransitionImageView() )
		check( !attachment->isTransferImageView() )
		check( !attachment->imageAttach.isColourTarget() )
		check( !attachment->imageAttach.isDepthStencilTarget() )
		check( attachment->imageAttach.isDepthTarget() )
		check( !attachment->imageAttach.isStencilTarget() )
		check( !attachment->imageAttach.isStencilInputTarget() )
		check( !attachment->imageAttach.isStencilOutputTarget() )
		checkEqual( attachment->name, pass.getGroupName() + "/" + view.data->name + "/IORdp" )
		checkEqual( attachment->getLoadOp(), crg::AttachmentLoadOp::eLoad )
		checkEqual( attachment->getStoreOp(), crg::AttachmentStoreOp::eStore )
		checkEqual( attachment->getStencilLoadOp(), crg::AttachmentLoadOp::eDontCare )
		checkEqual( attachment->getStencilStoreOp(), crg::AttachmentStoreOp::eDontCare )
		checkEqual( attachment->getImageLayout( false ), crg::ImageLayout::eDepthStencilAttachment )
		checkEqual( attachment->getImageLayout( true ), crg::ImageLayout::eDepthAttachment )
		check( attachment->view() == view )
		testEnd()
	}

	TEST( Attachment, InDepthStencilAttachment )
	{
		testBegin( "testInDepthStencilAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eD32_SFLOAT_S8_UINT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		auto viewAttach = crg::Attachment::createDefault( view );
		pass.addInputDepthStencilTarget( viewAttach );
		require( !pass.getTargets().empty() )
		auto const & attachment = pass.getTargets()[0];
		check( attachment->isImage() )
		check( attachment->isInput() )
		check( !attachment->isOutput() )
		check( !attachment->isClearable() )
		check( !attachment->isColourImageTarget() )
		check( !attachment->isColourInOutImageTarget() )
		check( !attachment->isColourInputImageTarget() )
		check( !attachment->isColourOutputImageTarget() )
		check( !attachment->isTransitionImageView() )
		check( !attachment->isTransferImageView() )
		check( !attachment->imageAttach.isColourTarget() )
		check( attachment->imageAttach.isDepthStencilTarget() )
		check( attachment->imageAttach.isDepthTarget() )
		check( attachment->imageAttach.isStencilTarget() )
		check( attachment->imageAttach.isStencilInputTarget() )
		check( !attachment->imageAttach.isStencilOutputTarget() )
		checkEqual( attachment->name, pass.getGroupName() + "/" + view.data->name + "/IRds" )
		checkEqual( attachment->getLoadOp(), crg::AttachmentLoadOp::eLoad )
		checkEqual( attachment->getStoreOp(), crg::AttachmentStoreOp::eDontCare )
		checkEqual( attachment->getStencilLoadOp(), crg::AttachmentLoadOp::eLoad )
		checkEqual( attachment->getStencilStoreOp(), crg::AttachmentStoreOp::eDontCare )
		check( attachment->view() == view )
		checkEqual( attachment->getImageLayout( false ), crg::ImageLayout::eDepthStencilReadOnly )
		checkEqual( attachment->getImageLayout( true ), crg::ImageLayout::eDepthStencilReadOnly )
		testEnd()
	}

	TEST( Attachment, InDepthStencilImage )
	{
		testBegin( "testInDepthStencilImage" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eD32_SFLOAT_S8_UINT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addInputDepthStencilTargetImage( view );
		require( !pass.getTargets().empty() )
		auto const & attachment = pass.getTargets()[0];
		check( attachment->isImage() )
		check( attachment->isInput() )
		check( !attachment->isOutput() )
		check( !attachment->isClearable() )
		check( !attachment->isColourImageTarget() )
		check( !attachment->isColourInOutImageTarget() )
		check( !attachment->isColourInputImageTarget() )
		check( !attachment->isColourOutputImageTarget() )
		check( !attachment->isTransitionImageView() )
		check( !attachment->isTransferImageView() )
		check( !attachment->imageAttach.isColourTarget() )
		check( attachment->imageAttach.isDepthStencilTarget() )
		check( attachment->imageAttach.isDepthTarget() )
		check( attachment->imageAttach.isStencilTarget() )
		check( attachment->imageAttach.isStencilInputTarget() )
		check( !attachment->imageAttach.isStencilOutputTarget() )
		checkEqual( attachment->name, pass.getGroupName() + "/" + view.data->name + "/IRds" )
		checkEqual( attachment->getLoadOp(), crg::AttachmentLoadOp::eLoad )
		checkEqual( attachment->getStoreOp(), crg::AttachmentStoreOp::eDontCare )
		checkEqual( attachment->getStencilLoadOp(), crg::AttachmentLoadOp::eLoad )
		checkEqual( attachment->getStencilStoreOp(), crg::AttachmentStoreOp::eDontCare )
		check( attachment->view() == view )
		checkEqual( attachment->getImageLayout( false ), crg::ImageLayout::eDepthStencilReadOnly )
		checkEqual( attachment->getImageLayout( true ), crg::ImageLayout::eDepthStencilReadOnly )
		testEnd()
	}

	TEST( Attachment, OutDepthStencilAttachment )
	{
		testBegin( "testOutDepthStencilAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eD32_SFLOAT_S8_UINT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addOutputDepthStencilTarget( view );
		require( !pass.getTargets().empty() )
		auto const & attachment = pass.getTargets()[0];
		check( attachment->isImage() )
		check( !attachment->isInput() )
		check( attachment->isOutput() )
		check( !attachment->isClearable() )
		check( !attachment->isColourImageTarget() )
		check( !attachment->isColourInOutImageTarget() )
		check( !attachment->isColourInputImageTarget() )
		check( !attachment->isColourOutputImageTarget() )
		check( !attachment->isTransitionImageView() )
		check( !attachment->isTransferImageView() )
		check( !attachment->imageAttach.isColourTarget() )
		check( attachment->imageAttach.isDepthStencilTarget() )
		check( attachment->imageAttach.isDepthTarget() )
		check( attachment->imageAttach.isStencilTarget() )
		check( !attachment->imageAttach.isStencilInputTarget() )
		check( attachment->imageAttach.isStencilOutputTarget() )
		checkEqual( attachment->name, pass.getGroupName() + "/" + view.data->name + "/ORds" )
		checkEqual( attachment->getLoadOp(), crg::AttachmentLoadOp::eClear )
		checkEqual( attachment->getStoreOp(), crg::AttachmentStoreOp::eStore )
		checkEqual( attachment->getStencilLoadOp(), crg::AttachmentLoadOp::eClear )
		checkEqual( attachment->getStencilStoreOp(), crg::AttachmentStoreOp::eStore )
		check( attachment->view() == view )
		checkEqual( attachment->getImageLayout( false ), crg::ImageLayout::eDepthStencilAttachment )
		checkEqual( attachment->getImageLayout( true ), crg::ImageLayout::eDepthStencilAttachment )
		testEnd()
	}

	TEST( Attachment, InOutDepthStencilAttachment )
	{
		testBegin( "testInOutDepthStencilAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eD32_SFLOAT_S8_UINT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		auto viewAttach = crg::Attachment::createDefault( view );
		pass.addInOutDepthStencilTarget( viewAttach );
		require( !pass.getTargets().empty() )
		auto const & attachment = pass.getTargets()[0];
		check( attachment->isImage() )
		check( attachment->isInput() )
		check( attachment->isOutput() )
		check( !attachment->isClearable() )
		check( !attachment->isColourImageTarget() )
		check( !attachment->isColourInOutImageTarget() )
		check( !attachment->isColourInputImageTarget() )
		check( !attachment->isColourOutputImageTarget() )
		check( !attachment->isTransitionImageView() )
		check( !attachment->isTransferImageView() )
		check( !attachment->imageAttach.isColourTarget() )
		check( attachment->imageAttach.isDepthStencilTarget() )
		check( attachment->imageAttach.isDepthTarget() )
		check( attachment->imageAttach.isStencilTarget() )
		check( attachment->imageAttach.isStencilInputTarget() )
		check( attachment->imageAttach.isStencilOutputTarget() )
		checkEqual( attachment->name, pass.getGroupName() + "/" + view.data->name + "/IORds" )
		checkEqual( attachment->getLoadOp(), crg::AttachmentLoadOp::eLoad )
		checkEqual( attachment->getStoreOp(), crg::AttachmentStoreOp::eStore )
		checkEqual( attachment->getStencilLoadOp(), crg::AttachmentLoadOp::eLoad )
		checkEqual( attachment->getStencilStoreOp(), crg::AttachmentStoreOp::eStore )
		check( attachment->view() == view )
		checkEqual( attachment->getImageLayout( false ), crg::ImageLayout::eDepthStencilAttachment )
		checkEqual( attachment->getImageLayout( true ), crg::ImageLayout::eDepthStencilAttachment )
		testEnd()
	}

	TEST( Attachment, InStencilAttachment )
	{
		testBegin( "testInStencilAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eS8_UINT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		auto viewAttach = crg::Attachment::createDefault( view );
		pass.addInputStencilTarget( viewAttach );
		require( !pass.getTargets().empty() )
		auto const & attachment = pass.getTargets()[0];
		check( attachment->isImage() )
		check( attachment->isInput() )
		check( !attachment->isOutput() )
		check( !attachment->isClearable() )
		check( !attachment->isColourImageTarget() )
		check( !attachment->isColourInOutImageTarget() )
		check( !attachment->isColourInputImageTarget() )
		check( !attachment->isColourOutputImageTarget() )
		check( !attachment->isTransitionImageView() )
		check( !attachment->isTransferImageView() )
		check( !attachment->imageAttach.isColourTarget() )
		check( !attachment->imageAttach.isDepthStencilTarget() )
		check( !attachment->imageAttach.isDepthTarget() )
		check( attachment->imageAttach.isStencilTarget() )
		check( attachment->imageAttach.isStencilInputTarget() )
		check( !attachment->imageAttach.isStencilOutputTarget() )
		checkEqual( attachment->name, pass.getGroupName() + "/" + view.data->name + "/IRst" )
		checkEqual( attachment->getLoadOp(), crg::AttachmentLoadOp::eDontCare )
		checkEqual( attachment->getStoreOp(), crg::AttachmentStoreOp::eDontCare )
		checkEqual( attachment->getStencilLoadOp(), crg::AttachmentLoadOp::eLoad )
		checkEqual( attachment->getStencilStoreOp(), crg::AttachmentStoreOp::eDontCare )
		check( attachment->view() == view )
		checkEqual( attachment->getImageLayout( false ), crg::ImageLayout::eDepthStencilReadOnly )
		checkEqual( attachment->getImageLayout( true ), crg::ImageLayout::eStencilReadOnly )
		testEnd()
	}

	TEST( Attachment, InStencilImage )
	{
		testBegin( "testInStencilImage" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eS8_UINT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addInputStencilTargetImage( view );
		require( !pass.getTargets().empty() )
		auto const & attachment = pass.getTargets()[0];
		check( attachment->isImage() )
		check( attachment->isInput() )
		check( !attachment->isOutput() )
		check( !attachment->isClearable() )
		check( !attachment->isColourImageTarget() )
		check( !attachment->isColourInOutImageTarget() )
		check( !attachment->isColourInputImageTarget() )
		check( !attachment->isColourOutputImageTarget() )
		check( !attachment->isTransitionImageView() )
		check( !attachment->isTransferImageView() )
		check( !attachment->imageAttach.isColourTarget() )
		check( !attachment->imageAttach.isDepthStencilTarget() )
		check( !attachment->imageAttach.isDepthTarget() )
		check( attachment->imageAttach.isStencilTarget() )
		check( attachment->imageAttach.isStencilInputTarget() )
		check( !attachment->imageAttach.isStencilOutputTarget() )
		checkEqual( attachment->name, pass.getGroupName() + "/" + view.data->name + "/IRst" )
		checkEqual( attachment->getLoadOp(), crg::AttachmentLoadOp::eDontCare )
		checkEqual( attachment->getStoreOp(), crg::AttachmentStoreOp::eDontCare )
		checkEqual( attachment->getStencilLoadOp(), crg::AttachmentLoadOp::eLoad )
		checkEqual( attachment->getStencilStoreOp(), crg::AttachmentStoreOp::eDontCare )
		check( attachment->view() == view )
		checkEqual( attachment->getImageLayout( false ), crg::ImageLayout::eDepthStencilReadOnly )
		checkEqual( attachment->getImageLayout( true ), crg::ImageLayout::eStencilReadOnly )
		testEnd()
	}

	TEST( Attachment, OutStencilAttachment )
	{
		testBegin( "testOutStencilAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eS8_UINT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addOutputStencilTarget( view );
		require( !pass.getTargets().empty() )
		auto const & attachment = pass.getTargets()[0];
		check( attachment->isImage() )
		check( !attachment->isInput() )
		check( attachment->isOutput() )
		check( !attachment->isClearable() )
		check( !attachment->isColourImageTarget() )
		check( !attachment->isColourInOutImageTarget() )
		check( !attachment->isColourInputImageTarget() )
		check( !attachment->isColourOutputImageTarget() )
		check( !attachment->isTransitionImageView() )
		check( !attachment->isTransferImageView() )
		check( !attachment->imageAttach.isColourTarget() )
		check( !attachment->imageAttach.isDepthStencilTarget() )
		check( !attachment->imageAttach.isDepthTarget() )
		check( attachment->imageAttach.isStencilTarget() )
		check( !attachment->imageAttach.isStencilInputTarget() )
		check( attachment->imageAttach.isStencilOutputTarget() )
		checkEqual( attachment->name, pass.getGroupName() + "/" + view.data->name + "/ORst" )
		checkEqual( attachment->getLoadOp(), crg::AttachmentLoadOp::eDontCare )
		checkEqual( attachment->getStoreOp(), crg::AttachmentStoreOp::eDontCare )
		checkEqual( attachment->getStencilLoadOp(), crg::AttachmentLoadOp::eClear )
		checkEqual( attachment->getStencilStoreOp(), crg::AttachmentStoreOp::eStore )
		check( attachment->view() == view )
		checkEqual( attachment->getImageLayout( false ), crg::ImageLayout::eDepthStencilAttachment )
		checkEqual( attachment->getImageLayout( true ), crg::ImageLayout::eStencilAttachment )
		testEnd()
	}

	TEST( Attachment, InOutStencilAttachment )
	{
		testBegin( "testInOutStencilAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eS8_UINT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		auto viewAttach = crg::Attachment::createDefault( view );
		pass.addInOutStencilTarget( viewAttach );
		require( !pass.getTargets().empty() )
		auto const & attachment = pass.getTargets()[0];
		check( attachment->isImage() )
		check( attachment->isInput() )
		check( attachment->isOutput() )
		check( !attachment->isClearable() )
		check( !attachment->isColourImageTarget() )
		check( !attachment->isColourInOutImageTarget() )
		check( !attachment->isColourInputImageTarget() )
		check( !attachment->isColourOutputImageTarget() )
		check( !attachment->isTransitionImageView() )
		check( !attachment->isTransferImageView() )
		check( !attachment->imageAttach.isColourTarget() )
		check( !attachment->imageAttach.isDepthStencilTarget() )
		check( !attachment->imageAttach.isDepthTarget() )
		check( attachment->imageAttach.isStencilTarget() )
		check( attachment->imageAttach.isStencilInputTarget() )
		check( attachment->imageAttach.isStencilOutputTarget() )
		checkEqual( attachment->name, pass.getGroupName() + "/" + view.data->name + "/IORst" )
		checkEqual( attachment->getLoadOp(), crg::AttachmentLoadOp::eDontCare )
		checkEqual( attachment->getStoreOp(), crg::AttachmentStoreOp::eDontCare )
		checkEqual( attachment->getStencilLoadOp(), crg::AttachmentLoadOp::eLoad )
		checkEqual( attachment->getStencilStoreOp(), crg::AttachmentStoreOp::eStore )
		check( attachment->view() == view )
		checkEqual( attachment->getImageLayout( false ), crg::ImageLayout::eDepthStencilAttachment )
		checkEqual( attachment->getImageLayout( true ), crg::ImageLayout::eStencilAttachment )
		testEnd()
	}

	TEST( Attachment, ImageAttachment )
	{
		testBegin( "testImageAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eS8_UINT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		auto const attachment = crg::Attachment::createDefault( view );
		check( attachment.isImage() )
		check( !attachment.isInput() )
		check( !attachment.isOutput() )
		check( !attachment.isClearable() )
		check( attachment.isColourImageTarget() )
		check( !attachment.isColourInOutImageTarget() )
		check( !attachment.isColourInputImageTarget() )
		check( !attachment.isColourOutputImageTarget() )
		check( !attachment.isTransitionImageView() )
		check( !attachment.isTransferImageView() )
		check( attachment.imageAttach.isColourTarget() )
		check( !attachment.imageAttach.isDepthStencilTarget() )
		check( !attachment.imageAttach.isDepthTarget() )
		check( !attachment.imageAttach.isStencilTarget() )
		check( !attachment.imageAttach.isStencilInputTarget() )
		check( !attachment.imageAttach.isStencilOutputTarget() )
		checkEqual( attachment.getLoadOp(), crg::AttachmentLoadOp::eLoad )
		checkEqual( attachment.getStoreOp(), crg::AttachmentStoreOp::eStore )
		checkEqual( attachment.getStencilLoadOp(), crg::AttachmentLoadOp::eLoad )
		checkEqual( attachment.getStencilStoreOp(), crg::AttachmentStoreOp::eStore )
		check( attachment.view() == view )
		checkEqual( attachment.getImageLayout( false ), crg::ImageLayout::eColorAttachment )
		checkEqual( attachment.getImageLayout( true ), crg::ImageLayout::eColorAttachment )
		testEnd()
	}

	TEST( Attachment, ImplicitBufferAttachment )
	{
		testBegin( "testImplicitBufferAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto buffer = graph.createBuffer( test::createBuffer( "Test" ) );
		auto bufferv = graph.createView( test::createView( "Test", buffer ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		auto bufferAttach = crg::Attachment::createDefault( bufferv );
		crg::AccessState accessState{};
		pass.addImplicit( bufferAttach, accessState );
		require( pass.getInputs().size() == 1u )
		auto attachIt = pass.getInputs().begin();
		auto const & attachment = attachIt->second;
		checkEqual( getSize( buffer ), 1024u )
		checkEqual( getSize( bufferv ), 1024u )
		check( attachment->isBuffer() )
		check( attachment->isInput() )
		check( !attachment->isOutput() )
		check( !attachment->isBufferView() )
		check( !attachment->isTransferBuffer() )
		check( !attachment->isClearableBuffer() )
		check( !attachment->isStorageBuffer() )
		check( !attachment->isStorageBufferView() )
		check( !attachment->isUniformBuffer() )
		check( !attachment->isUniformBufferView() )
		check( attachment->isTransitionBuffer() )
		check( !attachment->isTransitionBufferView() )
		checkEqual( attachment->name, pass.getGroupName() + "/" + buffer.data->name + "/Impl" )
		check( attachment->buffer() == bufferv )
		check( attachment->bufferAttach.buffer() == bufferv )
		checkEqual( attachment->getAccessMask(), accessState.access )
		checkEqual( attachment->getPipelineStageFlags( true ), accessState.pipelineStage )
		testEnd()
	}

	TEST( Attachment, UniformBufferAttachment )
	{
		testBegin( "testUniformBufferAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		auto buffer = graph.createBuffer( test::createBuffer( "Test" ) );
		auto bufferv = graph.createView( test::createView( "Test", buffer ) );
		pass.addInputUniformBuffer( bufferv, 1u );
		require( pass.getUniforms().size() == 1u )
		auto attachIt = pass.getUniforms().begin();
		auto const & attachment = attachIt->second;
		check( attachment->isBuffer() )
		check( attachment->isInput() )
		check( !attachment->isOutput() )
		check( !attachment->isBufferView() )
		check( !attachment->isTransferBuffer() )
		check( !attachment->isClearableBuffer() )
		check( !attachment->isStorageBuffer() )
		check( !attachment->isStorageBufferView() )
		check( attachment->isUniformBuffer() )
		check( !attachment->isUniformBufferView() )
		check( !attachment->isTransitionBuffer() )
		check( !attachment->isTransitionBufferView() )
		check( attachIt->first == 1u )
		checkEqual( attachment->name, pass.getGroupName() + "/" + buffer.data->name + "/UB" )
		check( attachment->buffer() == bufferv )
		check( attachment->bufferAttach.buffer() == bufferv )
		testEnd()
	}

	TEST( Attachment, InputStorageBufferAttachment )
	{
		testBegin( "testInputStorageBufferAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		auto buffer = graph.createBuffer( test::createBuffer( "Test" ) );
		auto bufferv = graph.createView( test::createView( "Test", buffer ) );
		auto bufferAttach = crg::Attachment::createDefault( bufferv );
		pass.addInputStorage( bufferAttach, 1u );
		require( pass.getInputs().size() == 1u )
		auto attachIt = pass.getInputs().begin();
		auto const & attachment = attachIt->second;
		check( attachment->isBuffer() )
		check( attachment->isInput() )
		check( !attachment->isOutput() )
		check( !attachment->isBufferView() )
		check( !attachment->isTransferBuffer() )
		check( !attachment->isClearableBuffer() )
		check( attachment->isStorageBuffer() )
		check( !attachment->isStorageBufferView() )
		check( !attachment->isUniformBuffer() )
		check( !attachment->isUniformBufferView() )
		check( !attachment->isTransitionBuffer() )
		check( !attachment->isTransitionBufferView() )
		check( attachIt->first == 1u )
		checkEqual( attachment->name, pass.getGroupName() + "/" + buffer.data->name + "/IStr" )
		check( attachment->buffer() == bufferv )
		check( attachment->bufferAttach.buffer() == bufferv )
		testEnd()
	}

	TEST( Attachment, OutputStorageBufferAttachment )
	{
		testBegin( "testOutputStorageBufferAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		auto buffer = graph.createBuffer( test::createBuffer( "Test" ) );
		auto bufferv = graph.createView( test::createView( "Test", buffer ) );
		pass.addOutputStorageBuffer( bufferv, 1u );
		require( pass.getOutputs().size() == 1u )
		auto attachIt = pass.getOutputs().begin();
		auto const & attachment = attachIt->second;
		check( attachment->isBuffer() )
		check( !attachment->isInput() )
		check( attachment->isOutput() )
		check( !attachment->isBufferView() )
		check( !attachment->isTransferBuffer() )
		check( !attachment->isClearableBuffer() )
		check( attachment->isStorageBuffer() )
		check( !attachment->isStorageBufferView() )
		check( !attachment->isUniformBuffer() )
		check( !attachment->isUniformBufferView() )
		check( !attachment->isTransitionBuffer() )
		check( !attachment->isTransitionBufferView() )
		check( attachIt->first == 1u )
		checkEqual( attachment->name, pass.getGroupName() + "/" + buffer.data->name + "/OSB" )
		check( attachment->buffer() == bufferv )
		check( attachment->bufferAttach.buffer() == bufferv )
		testEnd()
	}

	TEST( Attachment, ClearableOutputStorageBufferAttachment )
	{
		testBegin( "testClearableOutputStorageBufferAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		auto buffer = graph.createBuffer( test::createBuffer( "Test" ) );
		auto bufferv = graph.createView( test::createView( "Test", buffer ) );
		pass.addClearableOutputStorageBuffer( bufferv, 1u );
		require( pass.getOutputs().size() == 1u )
		auto attachIt = pass.getOutputs().begin();
		auto const & attachment = attachIt->second;
		check( attachment->isBuffer() )
		check( !attachment->isInput() )
		check( attachment->isOutput() )
		check( !attachment->isBufferView() )
		check( !attachment->isTransferBuffer() )
		check( attachment->isClearableBuffer() )
		check( attachment->isStorageBuffer() )
		check( !attachment->isStorageBufferView() )
		check( !attachment->isUniformBuffer() )
		check( !attachment->isUniformBufferView() )
		check( !attachment->isTransitionBuffer() )
		check( !attachment->isTransitionBufferView() )
		check( attachIt->first == 1u )
		checkEqual( attachment->name, pass.getGroupName() + "/" + buffer.data->name + "/OSB" )
		check( attachment->buffer() == bufferv )
		check( attachment->bufferAttach.buffer() == bufferv )
		testEnd()
	}

	TEST( Attachment, InOutStorageBufferAttachment )
	{
		testBegin( "testInOutStorageBufferAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		auto buffer = graph.createBuffer( test::createBuffer( "Test" ) );
		auto bufferv = graph.createView( test::createView( "Test", buffer ) );
		auto bufferAttach = crg::Attachment::createDefault( bufferv );
		pass.addInOutStorage( bufferAttach, 1u );
		require( pass.getInouts().size() == 1u )
		auto attachIt = pass.getInouts().begin();
		auto const & attachment = attachIt->second;
		check( attachment->isBuffer() )
		check( attachment->isInput() )
		check( attachment->isOutput() )
		check( !attachment->isBufferView() )
		check( !attachment->isTransferBuffer() )
		check( !attachment->isClearableBuffer() )
		check( attachment->isStorageBuffer() )
		check( !attachment->isStorageBufferView() )
		check( !attachment->isUniformBuffer() )
		check( !attachment->isUniformBufferView() )
		check( !attachment->isTransitionBuffer() )
		check( !attachment->isTransitionBufferView() )
		check( attachIt->first == 1u )
		checkEqual( attachment->name, pass.getGroupName() + "/" + buffer.data->name + "/IOStr" )
		check( attachment->buffer() == bufferv )
		check( attachment->bufferAttach.buffer() == bufferv )
		testEnd()
	}

	TEST( Attachment, ImplicitBufferViewAttachment )
	{
		testBegin( "testImplicitBufferViewAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		auto buffer = graph.createBuffer( test::createBuffer( "Test" ) );
		auto bufferv = graph.createView( test::createView( "Test", buffer, crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto bufferAttach = crg::Attachment::createDefault( bufferv );
		pass.addImplicit( bufferAttach, crg::AccessState{} );
		require( pass.getInputs().size() == 1u )
		auto attachIt = pass.getInputs().begin();
		auto const & attachment = attachIt->second;
		check( attachment->isBuffer() )
		check( attachment->isInput() )
		check( !attachment->isOutput() )
		check( attachment->isBufferView() )
		check( !attachment->isTransferBuffer() )
		check( !attachment->isClearableBuffer() )
		check( !attachment->isStorageBuffer() )
		check( !attachment->isStorageBufferView() )
		check( !attachment->isUniformBuffer() )
		check( !attachment->isUniformBufferView() )
		check( attachment->isTransitionBuffer() )
		check( attachment->isTransitionBufferView() )
		checkEqual( attachment->name, pass.getGroupName() + "/" + buffer.data->name + "/Impl" )
		check( attachment->buffer() == bufferv )
		check( attachment->bufferAttach.buffer() == bufferv )
		testEnd()
	}

	TEST( Attachment, UniformBufferViewAttachment )
	{
		testBegin( "testUniformBufferViewAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		auto buffer = graph.createBuffer( test::createBuffer( "Test" ) );
		auto bufferv = graph.createView( test::createView( "Test", buffer, crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		pass.addInputUniformBuffer( bufferv, 1u );
		require( !pass.getUniforms().empty() )
		auto attachIt = pass.getUniforms().begin();
		auto const & attachment = attachIt->second;
		check( attachment->isBuffer() )
		check( attachment->isInput() )
		check( !attachment->isOutput() )
		check( attachment->isBufferView() )
		check( !attachment->isTransferBuffer() )
		check( !attachment->isClearableBuffer() )
		check( !attachment->isStorageBuffer() )
		check( !attachment->isStorageBufferView() )
		check( attachment->isUniformBuffer() )
		check( attachment->isUniformBufferView() )
		check( !attachment->isTransitionBuffer() )
		check( !attachment->isTransitionBufferView() )
		check( attachIt->first == 1u )
		checkEqual( attachment->name, pass.getGroupName() + "/" + buffer.data->name + "/UB" )
		check( attachment->buffer() == bufferv )
		check( attachment->bufferAttach.buffer() == bufferv )
		testEnd()
	}

	TEST( Attachment, InputStorageBufferViewAttachment )
	{
		testBegin( "testInputStorageBufferViewAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		auto buffer = graph.createBuffer( test::createBuffer( "Test" ) );
		auto bufferv = graph.createView( test::createView( "Test", buffer, crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto bufferAttach = crg::Attachment::createDefault( bufferv );
		pass.addInputStorage( bufferAttach, 1u );
		require( pass.getInputs().size() == 1u )
		auto attachIt = pass.getInputs().begin();
		auto const & attachment = attachIt->second;
		check( attachment->isBuffer() )
		check( attachment->isInput() )
		check( !attachment->isOutput() )
		check( attachment->isBufferView() )
		check( !attachment->isTransferBuffer() )
		check( !attachment->isClearableBuffer() )
		check( attachment->isStorageBuffer() )
		check( attachment->isStorageBufferView() )
		check( !attachment->isUniformBuffer() )
		check( !attachment->isUniformBufferView() )
		check( !attachment->isTransitionBuffer() )
		check( !attachment->isTransitionBufferView() )
		check( attachIt->first == 1u )
		checkEqual( attachment->name, pass.getGroupName() + "/" + buffer.data->name + "/IStr" )
		check( attachment->buffer() == bufferv )
		check( attachment->bufferAttach.buffer() == bufferv )
		testEnd()
	}

	TEST( Attachment, OutputStorageBufferViewAttachment )
	{
		testBegin( "testOutputStorageBufferViewAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		auto buffer = graph.createBuffer( test::createBuffer( "Test" ) );
		auto bufferv = graph.createView( test::createView( "Test", buffer, crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		pass.addOutputStorageBuffer( bufferv, 1u );
		require( pass.getOutputs().size() == 1u )
		auto attachIt = pass.getOutputs().begin();
		auto const & attachment = attachIt->second;
		check( attachment->isBuffer() )
		check( !attachment->isInput() )
		check( attachment->isOutput() )
		check( attachment->isBufferView() )
		check( !attachment->isTransferBuffer() )
		check( !attachment->isClearableBuffer() )
		check( attachment->isStorageBuffer() )
		check( attachment->isStorageBufferView() )
		check( !attachment->isUniformBuffer() )
		check( !attachment->isUniformBufferView() )
		check( !attachment->isTransitionBuffer() )
		check( !attachment->isTransitionBufferView() )
		check( attachIt->first == 1u )
		checkEqual( attachment->name, pass.getGroupName() + "/" + buffer.data->name + "/OSB" )
		check( attachment->buffer() == bufferv )
		check( attachment->bufferAttach.buffer() == bufferv )
		testEnd()
	}

	TEST( Attachment, ClearableOutputStorageBufferViewAttachment )
	{
		testBegin( "testClearableOutputStorageBufferViewAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		auto buffer = graph.createBuffer( test::createBuffer( "Test" ) );
		auto bufferv = graph.createView( test::createView( "Test", buffer, crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		pass.addClearableOutputStorageBuffer( bufferv, 1u );
		require( pass.getOutputs().size() == 1u )
		auto attachIt = pass.getOutputs().begin();
		auto const & attachment = attachIt->second;
		check( attachment->isBuffer() )
		check( !attachment->isInput() )
		check( attachment->isOutput() )
		check( attachment->isBufferView() )
		check( !attachment->isTransferBuffer() )
		check( attachment->isClearableBuffer() )
		check( attachment->isStorageBuffer() )
		check( attachment->isStorageBufferView() )
		check( !attachment->isUniformBuffer() )
		check( !attachment->isUniformBufferView() )
		check( !attachment->isTransitionBuffer() )
		check( !attachment->isTransitionBufferView() )
		check( attachIt->first == 1u )
		checkEqual( attachment->name, pass.getGroupName() + "/" + buffer.data->name + "/OSB" )
		check( attachment->buffer() == bufferv )
		check( attachment->bufferAttach.buffer() == bufferv )
		testEnd()
	}

	TEST( Attachment, InOutStorageBufferViewAttachment )
	{
		testBegin( "testInOutStorageBufferViewAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		auto buffer = graph.createBuffer( test::createBuffer( "Test" ) );
		auto bufferv = graph.createView( test::createView( "Test", buffer, crg::PixelFormat::eR16G16B16A16_SFLOAT ) );
		auto bufferAttach = crg::Attachment::createDefault( bufferv );
		pass.addInOutStorage( bufferAttach, 1u );
		require( pass.getInouts().size() == 1u )
		auto attachIt = pass.getInouts().begin();
		auto const & attachment = attachIt->second;
		check( attachment->isBuffer() )
		check( attachment->isInput() )
		check( attachment->isOutput() )
		check( attachment->isBufferView() )
		check( !attachment->isTransferBuffer() )
		check( !attachment->isClearableBuffer() )
		check( attachment->isStorageBuffer() )
		check( attachment->isStorageBufferView() )
		check( !attachment->isUniformBuffer() )
		check( !attachment->isUniformBufferView() )
		check( !attachment->isTransitionBuffer() )
		check( !attachment->isTransitionBufferView() )
		check( attachIt->first == 1u )
		checkEqual( attachment->name, pass.getGroupName() + "/" + buffer.data->name + "/IOStr" )
		check( attachment->buffer() == bufferv )
		check( attachment->bufferAttach.buffer() == bufferv )
		testEnd()
	}

	TEST( Attachment, InputTransferBufferAttachment )
	{
		testBegin( "testInputTransferBufferAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		auto buffer = graph.createBuffer( test::createBuffer( "Test" ) );
		auto bufferv = graph.createView( test::createView( "Test", buffer ) );
		auto bufferAttach = crg::Attachment::createDefault( bufferv );
		pass.addInputTransfer( bufferAttach );
		require( pass.getInputs().size() == 1u )
		auto attachIt = pass.getInputs().begin();
		auto const & attachment = attachIt->second;
		check( attachment->isBuffer() )
		check( attachment->isInput() )
		check( !attachment->isOutput() )
		check( !attachment->isBufferView() )
		check( attachment->isTransferBuffer() )
		check( !attachment->isClearableBuffer() )
		check( !attachment->isStorageBuffer() )
		check( !attachment->isStorageBufferView() )
		check( !attachment->isUniformBuffer() )
		check( !attachment->isUniformBufferView() )
		check( !attachment->isTransitionBuffer() )
		check( !attachment->isTransitionBufferView() )
		checkEqual( attachment->name, pass.getGroupName() + "/" + buffer.data->name + "/ITrf" )
		check( attachment->buffer() == bufferv )
		check( attachment->bufferAttach.buffer() == bufferv )
		testEnd()
	}

	TEST( Attachment, OutputTransferBufferAttachment )
	{
		testBegin( "testOutputTransferBufferAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		auto buffer = graph.createBuffer( test::createBuffer( "Test" ) );
		auto bufferv = graph.createView( test::createView( "Test", buffer ) );
		pass.addOutputTransferBuffer( bufferv );
		require( pass.getOutputs().size() == 1u )
		auto attachIt = pass.getOutputs().begin();
		auto const & attachment = attachIt->second;
		check( attachment->isBuffer() )
		check( !attachment->isInput() )
		check( attachment->isOutput() )
		check( !attachment->isBufferView() )
		check( attachment->isTransferBuffer() )
		check( !attachment->isClearableBuffer() )
		check( !attachment->isStorageBuffer() )
		check( !attachment->isStorageBufferView() )
		check( !attachment->isUniformBuffer() )
		check( !attachment->isUniformBufferView() )
		check( !attachment->isTransitionBuffer() )
		check( !attachment->isTransitionBufferView() )
		checkEqual( attachment->name, pass.getGroupName() + "/" + buffer.data->name + "/OTB" )
		check( attachment->buffer() == bufferv )
		check( attachment->bufferAttach.buffer() == bufferv )
		testEnd()
	}

	TEST( Attachment, InOutTransferBufferAttachment )
	{
		testBegin( "testInOutTransferBufferAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		auto buffer = graph.createBuffer( test::createBuffer( "Test" ) );
		auto bufferv = graph.createView( test::createView( "Test", buffer ) );
		auto bufferAttach = crg::Attachment::createDefault( bufferv );
		pass.addInOutTransfer( bufferAttach );
		require( pass.getInouts().size() == 1u )
		auto attachIt = pass.getInouts().begin();
		auto const & attachment = attachIt->second;
		check( attachment->isBuffer() )
		check( attachment->isInput() )
		check( attachment->isOutput() )
		check( !attachment->isBufferView() )
		check( attachment->isTransferBuffer() )
		check( !attachment->isClearableBuffer() )
		check( !attachment->isStorageBuffer() )
		check( !attachment->isStorageBufferView() )
		check( !attachment->isUniformBuffer() )
		check( !attachment->isUniformBufferView() )
		check( !attachment->isTransitionBuffer() )
		check( !attachment->isTransitionBufferView() )
		checkEqual( attachment->name, pass.getGroupName() + "/" + buffer.data->name + "/IOTrf" )
		check( attachment->buffer() == bufferv )
		check( attachment->bufferAttach.buffer() == bufferv )
		testEnd()
	}

	TEST( Attachment, BufferAttachment )
	{
		testBegin( "testBufferAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto buffer = graph.createBuffer( test::createBuffer( "Test" ) );
		auto bufferv = graph.createView( test::createView( "Test", buffer ) );
		auto attachment = crg::Attachment::createDefault( bufferv );
		check( attachment.isBuffer() )
		check( !attachment.isInput() )
		check( !attachment.isOutput() )
		check( !attachment.isBufferView() )
		check( !attachment.isTransferBuffer() )
		check( !attachment.isClearableBuffer() )
		check( !attachment.isStorageBuffer() )
		check( !attachment.isStorageBufferView() )
		check( !attachment.isUniformBuffer() )
		check( !attachment.isUniformBufferView() )
		check( !attachment.isTransitionBuffer() )
		check( !attachment.isTransitionBufferView() )
		check( attachment.buffer() == bufferv )
		check( attachment.bufferAttach.buffer() == bufferv )
		testEnd()
	}

	TEST( Attachment, AttachmentMerge )
	{
		testBegin( "testAttachmentMerge" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto buffer = graph.createBuffer( test::createBuffer( "buffer1" ) );
		auto buffer1v = graph.createView( test::createView( "buffer1v", buffer, 0u, 512u ) );
		auto buffer2v = graph.createView( test::createView( "buffer2v", buffer, 512u, 512u ) );
		auto image = graph.createImage( test::createImage( "image1", crg::PixelFormat::eR32G32B32A32_SFLOAT, 2u, 2u ) );
		auto image1v = graph.createView( test::createView( "image1v", image, 0u, 1u, 0u, 1u ) );
		auto image2v = graph.createView( test::createView( "image2v", image, 1u, 1u, 1u, 1u ) );
		{
			// Empty attachment list
			check( graph.mergeAttachments( {} ) == nullptr )
		}
		{
			// Single buffer attachment in list
			auto attachment = crg::Attachment::createDefault( buffer1v );
			checkThrow( attachment.getSource( 1u ), crg::Exception )
			check( attachment.getSource( 0u ) == &attachment )
			check( graph.mergeAttachments( { &attachment } ) == &attachment )
		}
		{
			// Single image attachment in list
			auto attachment = crg::Attachment::createDefault( image1v );
			check( graph.mergeAttachments( { &attachment } ) == &attachment )
		}
		{
			// Mixed attachments in list
			auto attachment1 = crg::Attachment::createDefault( image1v );
			auto attachment2 = crg::Attachment::createDefault( buffer1v );
			checkThrow( graph.mergeAttachments( { &attachment1, &attachment2 } ), crg::Exception )
		}
		{
			// Empty image attachments in list
			auto attachment1 = crg::Attachment::createDefault( image1v );
			attachment1.imageAttach.views.clear();
			auto attachment2 = crg::Attachment::createDefault( image2v );
			attachment2.imageAttach.views.clear();
			checkThrow( graph.mergeAttachments( { &attachment1, &attachment2 } ), crg::Exception )
		}
		{
			// Empty buffer attachments in list
			auto attachment1 = crg::Attachment::createDefault( buffer1v );
			attachment1.bufferAttach.buffers.clear();
			auto attachment2 = crg::Attachment::createDefault( buffer2v );
			attachment2.bufferAttach.buffers.clear();
			checkThrow( graph.mergeAttachments( { &attachment1, &attachment2 } ), crg::Exception )
		}
		{
			// Image attachments with different pass count
			auto attachment1 = crg::Attachment::createDefault( image1v );
			auto attachment2 = crg::Attachment::createDefault( image2v );
			attachment2.imageAttach.views.clear();
			checkThrow( graph.mergeAttachments( { &attachment1, &attachment2 } ), crg::Exception )
		}
		{
			// Buffer attachments with different pass count
			auto attachment1 = crg::Attachment::createDefault( buffer1v );
			auto attachment2 = crg::Attachment::createDefault( buffer2v );
			attachment2.bufferAttach.buffers.clear();
			checkThrow( graph.mergeAttachments( { &attachment1, &attachment2 } ), crg::Exception )
		}
		{
			// Image attachments
			auto attachment1 = crg::Attachment::createDefault( image1v );
			auto attachment2 = crg::Attachment::createDefault( image2v );
			checkNoThrow( graph.mergeAttachments( { &attachment1, &attachment2 } ) )
			auto & attachment = *graph.mergeAttachments( { &attachment1, &attachment2 } );
			check( attachment.isImage() )
			check( !attachment.isInput() )
			check( !attachment.isOutput() )
			check( !attachment.isClearable() )
			check( attachment.isColourImageTarget() )
			check( !attachment.isColourInOutImageTarget() )
			check( !attachment.isColourInputImageTarget() )
			check( !attachment.isColourOutputImageTarget() )
			check( !attachment.isTransitionImageView() )
			check( !attachment.isTransferImageView() )
			check( attachment.imageAttach.isColourTarget() )
			check( !attachment.imageAttach.isDepthStencilTarget() )
			check( !attachment.imageAttach.isDepthTarget() )
			check( !attachment.imageAttach.isStencilTarget() )
			check( !attachment.imageAttach.isStencilInputTarget() )
			check( !attachment.imageAttach.isStencilOutputTarget() )
			checkEqual( attachment.getLoadOp(), crg::AttachmentLoadOp::eLoad )
			checkEqual( attachment.getStoreOp(), crg::AttachmentStoreOp::eStore )
			checkEqual( attachment.getStencilLoadOp(), crg::AttachmentLoadOp::eLoad )
			checkEqual( attachment.getStencilStoreOp(), crg::AttachmentStoreOp::eStore )
			check( attachment.getImageLayout( false ) == crg::ImageLayout::eColorAttachment )
			check( attachment.getImageLayout( true ) == crg::ImageLayout::eColorAttachment )
			check( attachment.pass == nullptr )
			check( attachment.view().data->source.size() == 2u )
			check( attachment.view().data->source[0] == image1v )
			check( attachment.view().data->source[1] == image2v )
		}
		{
			// Buffer attachments
			auto attachment1 = crg::Attachment::createDefault( buffer1v );
			auto attachment2 = crg::Attachment::createDefault( buffer2v );
			checkNoThrow( graph.mergeAttachments( { &attachment1, &attachment2 } ) )
			auto & attachment = *graph.mergeAttachments( { &attachment1, &attachment2 } );
			check( attachment.isBuffer() )
			check( !attachment.isInput() )
			check( !attachment.isOutput() )
			check( !attachment.isBufferView() )
			check( !attachment.isTransferBuffer() )
			check( !attachment.isClearableBuffer() )
			check( !attachment.isStorageBuffer() )
			check( !attachment.isStorageBufferView() )
			check( !attachment.isUniformBuffer() )
			check( !attachment.isUniformBufferView() )
			check( !attachment.isTransitionBuffer() )
			check( !attachment.isTransitionBufferView() )
			check( attachment.pass == nullptr )
			check( attachment.buffer().data->source.size() == 2u )
			check( attachment.buffer().data->source[0] == buffer1v )
			check( attachment.buffer().data->source[1] == buffer2v )
		}
		testEnd()
	}
}

testSuiteMain()
