#include "Window.hpp"

#include <RenderGraph/Attachment.hpp>

#include <RenderPass/AttachmentDescription.hpp>

namespace
{
	ashes::TexturePtr createImage( test::AppInstance const & inst
		, ashes::Format format )
	{
		ashes::ImageCreateInfo imageCreate{};
		imageCreate.extent = { inst.width, inst.height, 1u };
		imageCreate.format = format;
		imageCreate.imageType = ashes::TextureType::e2D;
		imageCreate.usage = ashes::ImageUsageFlag::eColourAttachment
			| ashes::ImageUsageFlag::eSampled;
		return inst.device->createTexture( imageCreate, ashes::MemoryPropertyFlag::eDeviceLocal );
	}
	
	ashes::TextureViewPtr createView( ashes::Texture const & image )
	{
		ashes::ImageViewCreateInfo viewCreate{};
		viewCreate.format = image.getFormat();
		viewCreate.viewType = ashes::TextureViewType::e2D;
		viewCreate.subresourceRange.aspectMask = getAspectMask( viewCreate.format );
		return image.createView( viewCreate );
	}

	void testColourAttachment( test::AppInstance & inst )
	{
		testBegin( "testColourAttachment" );
		auto colImage = createImage( inst, ashes::Format::eR32G32B32A32_SFLOAT );
		auto colView = createView( *colImage );
		auto colAttachment = crg::Attachment::createColour( "Colour"
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, *colView );

		check( colAttachment.name == "Colour" );
		check( colAttachment.loadOp == ashes::AttachmentLoadOp::eClear );
		check( colAttachment.storeOp == ashes::AttachmentStoreOp::eStore );
		check( colAttachment.stencilLoadOp == ashes::AttachmentLoadOp::eDontCare );
		check( colAttachment.stencilStoreOp == ashes::AttachmentStoreOp::eDontCare );
		check( &colAttachment.view == colView.get() );
		testEnd();
	}

	void testDepthStencilAttachment( test::AppInstance & inst )
	{
		testBegin( "testDepthStencilAttachment" );
		auto dsImage = createImage( inst, ashes::Format::eD32_SFLOAT );
		auto dsView = createView( *dsImage );
		auto dsAttachment = crg::Attachment::createDepthStencil( "DepthStencil"
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, *dsView );

		check( dsAttachment.name == "DepthStencil" );
		check( dsAttachment.loadOp == ashes::AttachmentLoadOp::eClear );
		check( dsAttachment.storeOp == ashes::AttachmentStoreOp::eStore );
		check( dsAttachment.stencilLoadOp == ashes::AttachmentLoadOp::eClear );
		check( dsAttachment.stencilStoreOp == ashes::AttachmentStoreOp::eStore );
		check( &dsAttachment.view == dsView.get() );
		testEnd();
	}
}

int main( int argc, char ** argv )
{
	test::AppInstance inst;
	testSuiteBegin( "TestAttachment", inst );
	testColourAttachment( inst );
	testDepthStencilAttachment( inst );
	testSuiteEnd();
}
