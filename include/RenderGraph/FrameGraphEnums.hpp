/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "FrameGraphBase.hpp"

#define CRG_MakeFlags( FlagBits )\
	constexpr FlagBits operator|( FlagBits lhs, FlagBits rhs ) { return FlagBits( std::underlying_type_t< FlagBits >( lhs ) | std::underlying_type_t< FlagBits >( rhs ) ); }\
	constexpr FlagBits operator&( FlagBits lhs, FlagBits rhs ) { return FlagBits( std::underlying_type_t< FlagBits >( lhs ) & std::underlying_type_t< FlagBits >( rhs ) ); }\
	constexpr FlagBits operator^( FlagBits lhs, FlagBits rhs ) { return FlagBits( std::underlying_type_t< FlagBits >( lhs ) ^ std::underlying_type_t< FlagBits >( rhs ) ); }\
	constexpr FlagBits & operator|=( FlagBits & lhs, FlagBits rhs ) { return lhs = lhs | rhs; }\
	constexpr FlagBits & operator&=( FlagBits & lhs, FlagBits rhs ) { return lhs = lhs & rhs; }\
	constexpr FlagBits & operator^=( FlagBits & lhs, FlagBits rhs ) { return lhs = lhs ^ rhs; }\
	constexpr bool checkFlag( FlagBits lhs, FlagBits rhs ) { return ( lhs & rhs ) == rhs; }

namespace crg
{
	enum class PixelFormat : int32_t
	{
#define RGPF_ENUM_VALUE( name, value, components, alpha, colour, depth, stencil, compressed ) e##name = value,
#define RGPF_ENUM_NON_VALUE( name, value ) e##name = value,
#include "PixelFormat.inl"
	};

	enum class ImageType : int32_t
	{
		e1D = 0,
		e2D = 1,
		e3D = 2,
	};

	enum class SampleCount : int32_t
	{
		e1 = 0x00000001,
		e2 = 0x00000002,
		e4 = 0x00000004,
		e8 = 0x00000008,
		e16 = 0x00000010,
		e32 = 0x00000020,
		e64 = 0x00000040,
	};

	enum class ImageTiling : int32_t
	{
		eOptimal = 0,
		eLinear = 1,
		eDRMFormatModifier = 1000158000,
	};

	enum class ImageViewType : int32_t
	{
		e1D = 0,
		e2D = 1,
		e3D = 2,
		eCube = 3,
		e1DArray = 4,
		e2DArray = 5,
		eCubeArray = 6,
	};

	enum class ImageLayout : int32_t
	{
		eUndefined = 0,
		eGeneral = 1,
		eColorAttachment = 2,
		eDepthStencilAttachment = 3,
		eDepthStencilReadOnly = 4,
		eShaderReadOnly = 5,
		eTransferSrc = 6,
		eTransferDst = 7,
		ePreinitialized = 8,
		eDepthReadOnlyStencilAttachment = 1000117000,
		eDepthAttachmentStencilReadOnly = 1000117001,
		eDepthAttachment = 1000241000,
		eDepthReadOnly = 1000241001,
		eStencilAttachment = 1000241002,
		eStencilReadOnly = 1000241003,
		eReadOnly = 1000314000,
		eAttachment = 1000314001,
		eRenderingLocalRead = 1000232000,
		ePresentSrc = 1000001002,
		eVideoDecodeDst = 1000024000,
		eVideoDecodeSrc = 1000024001,
		eVideoDecodeDpb = 1000024002,
		eSharedPresent = 1000111000,
		eFragmentDensityMap = 1000218000,
		eFragmentShadingRateAttachment = 1000164003,
		eVideoEncodeDst = 1000299000,
		eVideoEncodeSrc = 1000299001,
		eVideoEncodeDpb = 1000299002,
		eAttachmentFeedbackLoop = 1000339000,
		eVideoEncodeQuantizationMap = 1000553000,
	};

	enum class FilterMode : int32_t
	{
		eNearest,
		eLinear,
	};

	enum class MipmapMode : int32_t
	{
		eNearest,
		eLinear,
	};

	enum class WrapMode : int32_t
	{
		eRepeat,
		eMirroredRepeat,
		eClampToEdge,
		eClampToBorder,
		eMirrorClampToEdge,
	};

	enum class AttachmentLoadOp : int32_t
	{
		eLoad = 0,
		eClear = 1,
		eDontCare = 2,
		eNone = 1000400000,
	};

	enum class AttachmentStoreOp : int32_t
	{
		eStore = 0,
		eDontCare = 1,
		eNone = 1000301000,
	};

	enum class BlendFactor : int32_t
	{
		eZero = 0,
		eOne = 1,
		eSrcColor = 2,
		eOneMinusSrcColor = 3,
		eDstColor = 4,
		eOneMinusDstColor = 5,
		eSrcAlpha = 6,
		eOneMinusSrcAlpha = 7,
		eDstAlpha = 8,
		eOneMinusDstAlpha = 9,
		eConstantColor = 10,
		eOneMinusConstantColor = 11,
		eConstantAlpha = 12,
		eOneMinusConstantAlpha = 13,
		eSrcAlphaSaturate = 14,
		eSrc1Color = 15,
		eOneMinusSrc1Color = 16,
		eSrc1Alpha = 17,
		eOneMinusSrc1Alpha = 18,
	};

	enum class BlendOp : int32_t
	{
		eAdd = 0,
		eSubtract = 1,
		eReverse_subtract = 2,
		eMin = 3,
		eMax = 4,
		eZero = 1000148000,
		eSrc = 1000148001,
		eDst = 1000148002,
		eSrcOver = 1000148003,
		eDstOver = 1000148004,
		eSrcIn = 1000148005,
		eDstIn = 1000148006,
		eSrcOut = 1000148007,
		eDstOut = 1000148008,
		eSrcAtop = 1000148009,
		eDstAtop = 1000148010,
		eXor = 1000148011,
		eMultiply = 1000148012,
		eScreen = 1000148013,
		eOverlay = 1000148014,
		eDarken = 1000148015,
		eLighten = 1000148016,
		eColordodge = 1000148017,
		eColorburn = 1000148018,
		eHardlight = 1000148019,
		eSoftlight = 1000148020,
		eDifference = 1000148021,
		eExclusion = 1000148022,
		eInvert = 1000148023,
		eInvertRGB = 1000148024,
		eLineardodge = 1000148025,
		eLinearburn = 1000148026,
		eVividlight = 1000148027,
		eLinearlight = 1000148028,
		ePinlight = 1000148029,
		eHardmix = 1000148030,
		eHSLHue = 1000148031,
		eHSLSaturation = 1000148032,
		eHSLColor = 1000148033,
		eHSLLuminosity = 1000148034,
		ePlus = 1000148035,
		ePlusClamped = 1000148036,
		ePlusClamped_alpha = 1000148037,
		ePlusDarker = 1000148038,
		eMinus = 1000148039,
		eMinus_clamped = 1000148040,
		eContrast = 1000148041,
		eInvertOVG = 1000148042,
		eRed = 1000148043,
		eGreen = 1000148044,
		eBlue = 1000148045,
	};

	enum class BufferCreateFlags : int32_t
	{
		eNone = 0x00000000,
		eSparseBinding = 0x00000001,
		eSparseResidency = 0x00000002,
		eSparseAliased = 0x00000004,
		eProtected = 0x00000008,
		eDeviceAddressCaptureReplay = 0x00000010,
		eDescriptorBufferCaptureReplay = 0x00000020,
		eVideoProfileIndependent = 0x00000040,
	};
	CRG_MakeFlags( BufferCreateFlags )

	enum class BufferUsageFlags : int32_t
	{
		eNone = 0x00000000,
		eTransferSrc = 0x00000001,
		eTransferDst = 0x00000002,
		eUniformTexelBuffer = 0x00000004,
		eStorageTexelBuffer = 0x00000008,
		eUniformBuffer = 0x00000010,
		eStorageBuffer = 0x00000020,
		eIndexBuffer = 0x00000040,
		eVertexBuffer = 0x00000080,
		eIndirectBuffer = 0x00000100,
		eShaderDeviceAddress = 0x00020000,
		eVideoDecodeSrc = 0x00002000,
		eVideoDecodeDst = 0x00004000,
		eTransformFeedbackBuffer = 0x00000800,
		eTransformFeedbackCounterBuffer = 0x00001000,
		eConditionalRendering = 0x00000200,
		eAccelerationStructureBuildInputReadOnly = 0x00080000,
		eAccelerationStructureStorage = 0x00100000,
		eShaderBindingTable = 0x00000400,
		eVideoEncodeDst = 0x00008000,
		eVideoEncodeSrc = 0x00010000,
		eSamplerDescriptorBuffer = 0x00200000,
		eResourceDescriptorBuffer = 0x00400000,
		ePushDescriptorsDescriptorBuffer = 0x04000000,
		eMicromapBuildInputReadOnly = 0x00800000,
		eMicromapStorage = 0x01000000,
		eTileMemory = 0x08000000,
	};
	CRG_MakeFlags( BufferUsageFlags )

	enum class MemoryPropertyFlags : int32_t
	{
		eNone = 0x0000000,
		eDeviceLocal = 0x00000001,
		eHostVisible = 0x00000002,
		eHostCoherent = 0x00000004,
		eHostCached = 0x00000008,
		eLazilyAllocated = 0x00000010,
		eProtected = 0x00000020,
		eDeviceCoherent = 0x00000040,
		eDeviceUncached = 0x00000080,
		eRdmaCapable = 0x00000100,
	};
	CRG_MakeFlags( MemoryPropertyFlags )

	enum class ImageCreateFlags : int32_t
	{
		eNone = 0,
		eSparseBinding = 0x00000001,
		eSparseResidency = 0x00000002,
		eSparseAliased = 0x00000004,
		eMutableFormat = 0x00000008,
		eCubeCompatible = 0x00000010,
		eAlias = 0x00000400,
		eSplitInstanceBindRegions = 0x00000040,
		e2DArrayCompatible = 0x00000020,
		eBlockTexelViewCompatible = 0x00000080,
		eExtendedUsage = 0x00000100,
		eProtected = 0x00000800,
		eDisjoint = 0x00000200,
		eCornerSampled = 0x00002000,
		eSampleLocationsCompatibleDepth = 0x00001000,
		eSubsampled = 0x00004000,
		eDescriptorBufferCaptureReplay = 0x00010000,
		eMultisampledRenderToSingleSampled = 0x00040000,
		e2DViewCompatible = 0x00020000,
		eVideoProfileIndependent = 0x00100000,
		eFragmentDensityMapOffset = 0x00008000,
	};
	CRG_MakeFlags( ImageCreateFlags )

	enum class ImageUsageFlags : int32_t
	{
		eNone = 0,
		eTransferSrc = 0x00000001,
		eTransferDst = 0x00000002,
		eSampled = 0x00000004,
		eStorage = 0x00000008,
		eColorAttachment = 0x00000010,
		eDepthStencilAttachment = 0x00000020,
		eTransientAttachment = 0x00000040,
		eInputAttachment = 0x00000080,
		eHostTransfer = 0x00400000,
		eVideoDecodeDst = 0x00000400,
		eVideoDecodeSrc = 0x00000800,
		eVideoDecodeDpb = 0x00001000,
		eFragmentDensityMap = 0x00000200,
		eFragmentShadingRateAttachment = 0x00000100,
		eVideoEncodeDst = 0x00002000,
		eVideoEncodeSrc = 0x00004000,
		eVideoEncodeDpb = 0x00008000,
		eAttachmentFeedbackLoop = 0x00080000,
		eInvocationMask = 0x00040000,
		eSampleWeight = 0x00100000,
		eSampleBlockMatch = 0x00200000,
		eTileMemory = 0x08000000,
		eVideoEncodeQuantizationDeltaMap = 0x02000000,
		eVideoEncodeEmphasisMap = 0x04000000,
	};
	CRG_MakeFlags( ImageUsageFlags )

	enum class ImageViewCreateFlags : int32_t
	{
		eNone = 0,
		eFragmentDensityMapDynamic = 0x00000001,
		eDescriptorBufferCaptureReplay = 0x00000004,
		eFragmentDensityMapDeferred = 0x00000002,
	};
	CRG_MakeFlags( ImageViewCreateFlags )

	enum class ImageAspectFlags : int32_t
	{
		eNone = 0,
		eColor = 0x00000001,
		eDepth = 0x00000002,
		eStencil = 0x00000004,
		eDepthStencil = eDepth | eStencil,
		eMetadata = 0x00000008,
		ePlane0 = 0x00000010,
		ePlane1 = 0x00000020,
		ePlane2 = 0x00000040,
		eMemoryPlane0 = 0x00000080,
		eMemoryPlane1 = 0x00000100,
		eMemoryPlane2 = 0x00000200,
		eMemoryPlane3 = 0x00000400,
	};
	CRG_MakeFlags( ImageAspectFlags )

	enum class PipelineStageFlags : int32_t
	{
		eNone = 0,
		eTopOfPipe = 0x00000001,
		eDrawIndirect = 0x00000002,
		eVertexInput = 0x00000004,
		eVertexShader = 0x00000008,
		eTessellationControlShader = 0x00000010,
		eTessellationEvaluationShader = 0x00000020,
		eGeometryShader = 0x00000040,
		eFragmentShader = 0x00000080,
		eEarlyFragmentTests = 0x00000100,
		eLateFragmentTests = 0x00000200,
		eColorAttachmentOutput = 0x00000400,
		eComputeShader = 0x00000800,
		eTransfer = 0x00001000,
		eBottomOfPipe = 0x00002000,
		eHost = 0x00004000,
		eAllGraphics = 0x00008000,
		eAllCommands = 0x00010000,
		eTransformFeedback = 0x01000000,
		eConditionalRendering = 0x00040000,
		eAccelerationStructureBuild = 0x02000000,
		eRayTracingShader = 0x00200000,
		eFragmentDensityProcess = 0x00800000,
		eFragmentShadingRateAttachment = 0x00400000,
		eTaskShader = 0x00080000,
		eMeshShader = 0x00100000,
		eCommandPreprocess = 0x00020000,
	};
	CRG_MakeFlags( PipelineStageFlags )

	enum class AccessFlags : int32_t
	{
		eNone = 0,
		eIndirectCommandRead = 0x00000001,
		eIndexRead = 0x00000002,
		eVertexAttributeRead = 0x00000004,
		eUniformRead = 0x00000008,
		eInputAttachmentRead = 0x00000010,
		eShaderRead = 0x00000020,
		eShaderWrite = 0x00000040,
		eColorAttachmentRead = 0x00000080,
		eColorAttachmentWrite = 0x00000100,
		eDepthStencilAttachmentRead = 0x00000200,
		eDepthStencilAttachmentWrite = 0x00000400,
		eTransferRead = 0x00000800,
		eTransferWrite = 0x00001000,
		eHostRead = 0x00002000,
		eHostWrite = 0x00004000,
		eMemoryRead = 0x00008000,
		eMemoryWrite = 0x00010000,
		eTransformFeedbackWrite = 0x02000000,
		eTransformFeedbackCounterRead = 0x04000000,
		eTransformFeedbackCounterWrite = 0x08000000,
		eConditionalRenderingRead = 0x00100000,
		eColorAttachmentReadNonCoherent = 0x00080000,
		eAccelerationStructureRead = 0x00200000,
		eAccelerationStructureWrite = 0x00400000,
		eFragmentDensityMapRead = 0x01000000,
		eFragmentShadingRateAttachmentRead = 0x00800000,
		eCommandPreprocessRead = 0x00020000,
		eCommandPreprocessWrite = 0x00040000,
	};
	CRG_MakeFlags( AccessFlags )

	enum class ColorComponentFlags : int32_t
	{
		eNone = 0,
		eR = 0x00000001,
		eG = 0x00000002,
		eB = 0x00000004,
		eA = 0x00000008,
	};
	CRG_MakeFlags( ColorComponentFlags )
 }
