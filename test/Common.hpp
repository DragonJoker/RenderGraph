#pragma once

#include <RenderGraph/FrameGraphPrerequisites.hpp>
#include <RenderGraph/RunnablePass.hpp>

#include "BaseTest.hpp"

#include <sstream>

namespace test
{
	crg::BufferData createBuffer( std::string name );
	crg::BufferViewData createView( std::string name
		, crg::BufferId buffer
		, crg::PixelFormat format = crg::PixelFormat::eUNDEFINED );
	crg::BufferViewData createView( std::string name
		, crg::BufferId buffer
		, crg::DeviceSize offset, crg::DeviceSize size
		, crg::PixelFormat format = crg::PixelFormat::eUNDEFINED );
	crg::ImageData createImage( std::string name
		, crg::PixelFormat format
		, uint32_t mipLevels = 1u
		, uint32_t arrayLayers = 1u );
	crg::ImageData createImage1D( std::string name
		, crg::PixelFormat format
		, uint32_t mipLevels = 1u
		, uint32_t arrayLayers = 1u );
	crg::ImageData createImage3D( std::string name
		, crg::PixelFormat format
		, uint32_t mipLevels = 1u
		, uint32_t arrayLayers = 1u );
	crg::ImageData createImageCube( std::string name
		, crg::PixelFormat format
		, uint32_t mipLevels = 1u
		, uint32_t arrayLayers = 1u );
	crg::ImageViewData createView( std::string name
		, crg::ImageId image
		, uint32_t baseMipLevel = 0u
		, uint32_t levelCount = 1u
		, uint32_t baseArrayLayer = 0u
		, uint32_t layerCount = 1u );
	crg::ImageViewData createView( std::string name
		, crg::ImageId image
		, crg::PixelFormat format
		, uint32_t baseMipLevel = 0u
		, uint32_t levelCount = 1u
		, uint32_t baseArrayLayer = 0u
		, uint32_t layerCount = 1u );
	crg::GraphContext & getDummyContext();
	std::string checkRunnable( TestCounts & testCounts
		, crg::RunnableGraph * runnable );

	inline std::string checkRunnable( TestCounts & testCounts
		, crg::RunnableGraphPtr const & runnable )
	{
		return checkRunnable( testCounts, runnable.get() );
	}

	void display( TestCounts const & testCounts
		, std::ostream & stream
		, crg::RunnableGraph const & value
		, bool withColours = {}
		, bool withIds = {}
		, bool withGroups = {} );
	void display( TestCounts const & testCounts
		, crg::RunnableGraph const & value
		, bool withColours = {}
		, bool withIds = {}
		, bool withGroups = {} );

	template< typename TypeT >
	crg::Id< TypeT > makeId( TypeT const & data )
	{
		return { 0u, nullptr };
	}

	using CheckViews = std::function< void( test::TestCounts &
		, crg::FramePass const &
		, crg::RunnableGraph const &
		, crg::RecordContext &
		, uint32_t ) >;

	crg::RunnablePassPtr createDummy( test::TestCounts & testCounts
		, crg::FramePass const & framePass
		, crg::GraphContext & context
		, crg::RunnableGraph & runGraph
		, crg::PipelineStageFlags pipelineStageFlags
		, CheckViews checkViews
		, uint32_t index
		, bool enabled
		, crg::ru::Config config = {} );
	crg::RunnablePassPtr createDummy( test::TestCounts & testCounts
		, crg::FramePass const & framePass
		, crg::GraphContext & context
		, crg::RunnableGraph & runGraph
		, crg::PipelineStageFlags pipelineStageFlags
		, CheckViews checkViews
		, uint32_t index
		, crg::ru::Config config = {} );
	crg::RunnablePassPtr createDummy( test::TestCounts & testCounts
		, crg::FramePass const & framePass
		, crg::GraphContext & context
		, crg::RunnableGraph & runGraph
		, crg::PipelineStageFlags pipelineStageFlags
		, CheckViews checkViews
		, crg::ru::Config config = {} );
	crg::RunnablePassPtr createDummy( test::TestCounts & testCounts
		, crg::FramePass const & framePass
		, crg::GraphContext & context
		, crg::RunnableGraph & runGraph
		, crg::PipelineStageFlags pipelineStageFlags
		, crg::ru::Config config = {} );
	crg::RunnablePassPtr createDummyNoRecord( crg::FramePass const & framePass
		, crg::GraphContext & context
		, crg::RunnableGraph & runGraph
		, crg::PipelineStageFlags pipelineStageFlags
		, crg::ru::Config config = {} );
	void checkDummy( test::TestCounts & testCounts
		, crg::FramePass const & framePass
		, crg::RunnableGraph const & graph
		, crg::RecordContext const & context
		, uint32_t index );
}

namespace crg
{
	std::ostream & operator<<( std::ostream & stream, AccessFlags const & v );
	std::ostream & operator<<( std::ostream & stream, ImageLayout const & v );
	std::ostream & operator<<( std::ostream & stream, AttachmentLoadOp const & v );
	std::ostream & operator<<( std::ostream & stream, AttachmentStoreOp const & v );
}
