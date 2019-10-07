/*
See LICENSE file in root folder
*/
#ifndef ___Utils_UTILS_PREREQUISITES_H___
#define ___Utils_UTILS_PREREQUISITES_H___

#include <array>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <list>
#include <locale>
#include <map>
#include <map>
#include <memory>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <deque>
#include <sstream>
#include <vector>

#include "Utils/Config/SmartPtr.hpp"
#include "Utils/Log/ELogType.hpp"

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#if defined( min )
#	undef min
#endif
#if defined( max )
#	undef max
#endif
#if defined( abs )
#	undef abs
#endif

namespace utils
{
	/*!
	\author Sylvain DOREMUS
	\version 0.6.1.0
	\date 19/10/2011
	\~english
	\brief utils::Loader supported file types enumeration
	\~french
	\brief Enumération des types de fichier supportés par utils::Loader
	*/
	enum class FileType
		: uint8_t
	{
		//! Binary file
		eBinary,
		//! Text file
		eText,
		Utils_ScopedEnumBounds( eBinary )
	};
	/*!
	\author		Sylvain DOREMUS
	\version	0.9.0
	\date		24/05/2016
	\~english
	\brief		Helper structure to find File type (BinaryFile or TextFile) from FileType.
	\~french
	\brief		Structure d'aide pour trouver le type de File (BinaryFile ou TextFile) à partir d'un FileType.
	*/
	template< FileType FT >
	struct FileTyper;
	/*!
	\author		Sylvain DOREMUS
	\version	0.6.1.0
	\date		03/01/2011
	\~english
	\brief		Intersection types
	\remark		Enumerates the different intersection types.
	\~french
	\brief		Types d'intersection
	\remark		Enumère les différents types d'intersection.
	*/
	enum class Intersection
		: uint8_t
	{
		//!\~english	Completely inside.
		//!\~french		Complètement à l'intérieur.
		eIn,
		//!\~english	Completely outside.
		//!\~french		Complètement à l'extérieur.
		eOut,
		//!\~english	Intersection.
		//!\~french		Intersection
		eIntersect,
		Utils_ScopedEnumBounds( eIn )
	};

	using real = float;
	using String = std::basic_string< xchar >;
	using Regex = std::basic_regex< xchar >;
	using RegexIterator = std::regex_iterator< String::const_iterator >;
	using MatchResults = std::match_results< String::const_iterator >;
	using StringStream = std::basic_stringstream< xchar >;
	using OutputStringStream = std::basic_ostringstream< xchar >;
	using InputStringStream = std::basic_istringstream< xchar >;
	using OutputStream = std::basic_ostream< xchar >;
	using InputStream = std::basic_istream< xchar >;
	using Seconds = std::chrono::seconds;
	using Milliseconds = std::chrono::milliseconds;
	using Microseconds = std::chrono::microseconds;
	using Nanoseconds = std::chrono::nanoseconds;
	using Position = VkOffset2D;
	using Size = VkExtent2D;

	/*!
	\author		Sylvain DOREMUS
	\version	0.8.0
	\date		09/11/2015
	\see		QuaternionT
	\~english
	\brief		Structure used to construct objects but without initialisation.
	\~french
	\brief		Structure utilisée pour les constructeurs d'objets à ne pas initialiser.
	*/
	struct NoInit {};
	static NoInit constexpr noInit;

	template< typename Type >
	class AngleT;
	template< size_t Size >
	struct BaseTypeFromSize;
	class BinaryFile;
	template< class T >
	class BinaryLoader;
	template< typename T, typename Key >
	class Collection;
	template< uint8_t Dimension >
	class BoundingContainer;
	template< typename T, uint32_t Count >
	class Coords;
	class BoundingBox;
	template< typename BlockType >
	class DynamicBitsetT;
	class File;
	class FileParser;
	class FileParserContext;
	template< typename FlagType
		, typename BaseType = typename BaseTypeFromSize< sizeof( FlagType ) >::Type >
	class FlagCombination;
	class Font;
	class FontCache;
	class Glyph;
	class Image;
	class ImageCache;
	class ColourComponent;
	class HdrColourComponent;
	template< typename ComponentType >
	class RgbColourT;
	template< typename ComponentType >
	class RgbaColourT;
	using RgbColour = RgbColourT< ColourComponent >;
	using RgbaColour = RgbaColourT< ColourComponent >;
	using HdrRgbColour = RgbColourT< HdrColourComponent >;
	using HdrRgbaColour = RgbaColourT< HdrColourComponent >;
	template< typename T >
	class Line2D;
	template< typename T >
	class Line3D;
	template< class T, FileType FT >
	class Loader;
	class ILoggerImpl;
	class Logger;
	template< typename T, uint32_t Rows, uint32_t Columns >
	class Matrix;
	template< class Owmer >
	class OwnedBy;
	class ParserParameterBase;
	template< VkFormat FT >
	class Pixel;
	class PlaneEquation;
	template< typename T, uint32_t Count >
	class Point;
	class PxBufferBase;
	template< VkFormat FT >
	class PxBuffer;
	template< typename T >
	class QuaternionT;
	struct Rectangle;
	template< typename T >
	class Resource;
	class BoundingSphere;
	class SphericalVertex;
	template< typename T, uint32_t Rows >
	class SquareMatrix;
	class TextFile;
	template< class T >
	class TextLoader;
	template< class Obj
		, class Key
		, class PtrType = std::shared_ptr< Obj >
		, typename PFNCreate = std::function< PtrType() >
		, class Predicate = std::less< Key > >
	class Factory;
	class Path;
	class DynamicLibrary;
	struct Message;
	class Logger;
	class LoggerImpl;
	class ProgramConsole;

	using PixelBuffer = PxBuffer< VK_FORMAT_R8G8B8A8_UNORM >;
	using DynamicBitset = DynamicBitsetT< uint32_t >;

	template< typename T > using Point2 = Point< T, 2 >;
	template< typename T > using Point3 = Point< T, 3 >;
	template< typename T > using Point4 = Point< T, 4 >;

	template< typename T > using Coords2 = Coords< T, 2 >;
	template< typename T > using Coords3 = Coords< T, 3 >;
	template< typename T > using Coords4 = Coords< T, 4 >;

	template< typename T > using Matrix2x2 = SquareMatrix< T, 2 >;
	template< typename T > using Matrix3x3 = SquareMatrix< T, 3 >;
	template< typename T > using Matrix4x4 = SquareMatrix< T, 4 >;

	using Angle = AngleT< real >;
	using Quaternion = QuaternionT< real >;

	DECLARE_POINT( bool,		4, b );
	DECLARE_POINT( bool,		3, b );
	DECLARE_POINT( bool,		2, b );
	DECLARE_POINT( int8_t,		4, c );
	DECLARE_POINT( int8_t,		3, c );
	DECLARE_POINT( int8_t,		2, c );
	DECLARE_POINT( uint8_t,		4, ub );
	DECLARE_POINT( uint8_t,		3, ub );
	DECLARE_POINT( uint8_t,		2, ub );
	DECLARE_POINT( int16_t,		4, s );
	DECLARE_POINT( int16_t,		3, s );
	DECLARE_POINT( int16_t,		2, s );
	DECLARE_POINT( uint16_t,	4, us );
	DECLARE_POINT( uint16_t,	3, us );
	DECLARE_POINT( uint16_t,	2, us );
	DECLARE_POINT( int32_t,		4, i );
	DECLARE_POINT( int32_t,		3, i );
	DECLARE_POINT( int32_t,		2, i );
	DECLARE_POINT( uint32_t,	4, ui );
	DECLARE_POINT( uint32_t,	3, ui );
	DECLARE_POINT( uint32_t,	2, ui );
	DECLARE_POINT( real,		4, r );
	DECLARE_POINT( real,		3, r );
	DECLARE_POINT( real,		2, r );
	DECLARE_POINT( float,		4, f );
	DECLARE_POINT( float,		3, f );
	DECLARE_POINT( float,		2, f );
	DECLARE_POINT( double,		4, d );
	DECLARE_POINT( double,		3, d );
	DECLARE_POINT( double,		2, d );

	DECLARE_COORD( bool,		4, b );
	DECLARE_COORD( bool,		3, b );
	DECLARE_COORD( bool,		2, b );
	DECLARE_COORD( int8_t,		4, c );
	DECLARE_COORD( int8_t,		3, c );
	DECLARE_COORD( int8_t,		2, c );
	DECLARE_COORD( uint8_t,		4, ub );
	DECLARE_COORD( uint8_t,		3, ub );
	DECLARE_COORD( uint8_t,		2, ub );
	DECLARE_COORD( int16_t,		4, s );
	DECLARE_COORD( int16_t,		3, s );
	DECLARE_COORD( int16_t,		2, s );
	DECLARE_COORD( uint16_t,	4, us );
	DECLARE_COORD( uint16_t,	3, us );
	DECLARE_COORD( uint16_t,	2, us );
	DECLARE_COORD( int32_t,		4, i );
	DECLARE_COORD( int32_t,		3, i );
	DECLARE_COORD( int32_t,		2, i );
	DECLARE_COORD( uint32_t,	4, ui );
	DECLARE_COORD( uint32_t,	3, ui );
	DECLARE_COORD( uint32_t,	2, ui );
	DECLARE_COORD( int32_t,		4, i );
	DECLARE_COORD( int32_t,		3, i );
	DECLARE_COORD( int32_t,		2, i );
	DECLARE_COORD( real,		4, r );
	DECLARE_COORD( real,		3, r );
	DECLARE_COORD( real,		2, r );
	DECLARE_COORD( float,		4, f );
	DECLARE_COORD( float,		3, f );
	DECLARE_COORD( float,		2, f );
	DECLARE_COORD( double,		4, d );
	DECLARE_COORD( double,		3, d );
	DECLARE_COORD( double,		2, d );
	DECLARE_COORD( uint8_t,		4, ub );
	DECLARE_COORD( uint8_t,		3, ub );
	DECLARE_COORD( uint8_t,		2, ub );

	DECLARE_CONST_COORD( bool,		4, b );
	DECLARE_CONST_COORD( bool,		3, b );
	DECLARE_CONST_COORD( bool,		2, b );
	DECLARE_CONST_COORD( int8_t,	4, c );
	DECLARE_CONST_COORD( int8_t,	3, c );
	DECLARE_CONST_COORD( int8_t,	2, c );
	DECLARE_CONST_COORD( uint8_t,	4, ub );
	DECLARE_CONST_COORD( uint8_t,	3, ub );
	DECLARE_CONST_COORD( uint8_t,	2, ub );
	DECLARE_CONST_COORD( int16_t,	4, s );
	DECLARE_CONST_COORD( int16_t,	3, s );
	DECLARE_CONST_COORD( int16_t,	2, s );
	DECLARE_CONST_COORD( uint16_t,	4, us );
	DECLARE_CONST_COORD( uint16_t,	3, us );
	DECLARE_CONST_COORD( uint16_t,	2, us );
	DECLARE_CONST_COORD( int32_t,	4, i );
	DECLARE_CONST_COORD( int32_t,	3, i );
	DECLARE_CONST_COORD( int32_t,	2, i );
	DECLARE_CONST_COORD( uint32_t,	4, ui );
	DECLARE_CONST_COORD( uint32_t,	3, ui );
	DECLARE_CONST_COORD( uint32_t,	2, ui );
	DECLARE_CONST_COORD( int32_t,	4, i );
	DECLARE_CONST_COORD( int32_t,	3, i );
	DECLARE_CONST_COORD( int32_t,	2, i );
	DECLARE_CONST_COORD( real,		4, r );
	DECLARE_CONST_COORD( real,		3, r );
	DECLARE_CONST_COORD( real,		2, r );
	DECLARE_CONST_COORD( float,		4, f );
	DECLARE_CONST_COORD( float,		3, f );
	DECLARE_CONST_COORD( float,		2, f );
	DECLARE_CONST_COORD( double,	4, d );
	DECLARE_CONST_COORD( double,	3, d );
	DECLARE_CONST_COORD( double,	2, d );
	DECLARE_CONST_COORD( uint8_t,	4, ub );
	DECLARE_CONST_COORD( uint8_t,	3, ub );
	DECLARE_CONST_COORD( uint8_t,	2, ub );
	
	DECLARE_SQMTX( bool,	4, b );
	DECLARE_SQMTX( bool,	3, b );
	DECLARE_SQMTX( bool,	2, b );
	DECLARE_SQMTX( int32_t,	4, i );
	DECLARE_SQMTX( int32_t,	3, i );
	DECLARE_SQMTX( int32_t,	2, i );
	DECLARE_SQMTX( uint32_t,4, ui );
	DECLARE_SQMTX( uint32_t,3, ui );
	DECLARE_SQMTX( uint32_t,2, ui );
	DECLARE_SQMTX( real,	4, r );
	DECLARE_SQMTX( real,	3, r );
	DECLARE_SQMTX( real,	2, r );
	DECLARE_SQMTX( float,	4, f );
	DECLARE_SQMTX( float,	3, f );
	DECLARE_SQMTX( float,	2, f );
	DECLARE_SQMTX( double,	4, d );
	DECLARE_SQMTX( double,	3, d );
	DECLARE_SQMTX( double,	2, d );

	DECLARE_MTX( bool,		2, 3, b );
	DECLARE_MTX( bool,		2, 4, b );
	DECLARE_MTX( bool,		3, 2, b );
	DECLARE_MTX( bool,		3, 4, b );
	DECLARE_MTX( bool,		4, 2, b );
	DECLARE_MTX( bool,		4, 3, b );
	DECLARE_MTX( int32_t,	2, 3, i );
	DECLARE_MTX( int32_t,	2, 4, i );
	DECLARE_MTX( int32_t,	3, 2, i );
	DECLARE_MTX( int32_t,	3, 4, i );
	DECLARE_MTX( int32_t,	4, 2, i );
	DECLARE_MTX( int32_t,	4, 3, i );
	DECLARE_MTX( uint32_t,	2, 3, ui );
	DECLARE_MTX( uint32_t,	2, 4, ui );
	DECLARE_MTX( uint32_t,	3, 2, ui );
	DECLARE_MTX( uint32_t,	3, 4, ui );
	DECLARE_MTX( uint32_t,	4, 2, ui );
	DECLARE_MTX( uint32_t,	4, 3, ui );
	DECLARE_MTX( real,		2, 3, r );
	DECLARE_MTX( real,		2, 4, r );
	DECLARE_MTX( real,		3, 2, r );
	DECLARE_MTX( real,		3, 4, r );
	DECLARE_MTX( real,		4, 2, r );
	DECLARE_MTX( real,		4, 3, r );
	DECLARE_MTX( float,		2, 3, f );
	DECLARE_MTX( float,		2, 4, f );
	DECLARE_MTX( float,		3, 2, f );
	DECLARE_MTX( float,		3, 4, f );
	DECLARE_MTX( float,		4, 2, f );
	DECLARE_MTX( float,		4, 3, f );
	DECLARE_MTX( double,	2, 3, d );
	DECLARE_MTX( double,	2, 4, d );
	DECLARE_MTX( double,	3, 2, d );
	DECLARE_MTX( double,	3, 4, d );
	DECLARE_MTX( double,	4, 2, d );
	DECLARE_MTX( double,	4, 3, d );

	DECLARE_SMART_PTR( Quaternion );
	DECLARE_SMART_PTR( SphericalVertex );
	DECLARE_SMART_PTR( BoundingBox );
	DECLARE_SMART_PTR( BoundingSphere );
	DECLARE_SMART_PTR( Image );
	DECLARE_SMART_PTR( Font );
	DECLARE_SMART_PTR( PxBufferBase );
	DECLARE_SMART_PTR( FileParserContext );
	DECLARE_SMART_PTR( ParserParameterBase );
	DECLARE_SMART_PTR( DynamicLibrary );

	DECLARE_VECTOR( uint8_t, Byte );
	DECLARE_VECTOR( SphericalVertexSPtr, SphericalVertexPtr );
	DECLARE_VECTOR( int32_t, Int32 );
	DECLARE_VECTOR( String, String );
	DECLARE_VECTOR( Path, Path );
	DECLARE_VECTOR( ParserParameterBaseSPtr, ParserParameter );
	DECLARE_MAP( String, uint32_t, UInt32Str );
	DECLARE_MAP( String, uint64_t, UInt64Str );
	DECLARE_MAP( String, bool, BoolStr );
	DECLARE_MAP( String, String, StrStr );
	DECLARE_SET( String, Str );
	DECLARE_MAP( uint32_t, String, StrUInt32 );
	/*!
	\author 	Sylvain DOREMUS
	\date 		27/08/2012
	\version	0.7.0.0
	\~english
	\brief		Message representation
	\~french
	\brief		Représentation d'un message
	*/
	struct Message
	{
		//! The message type.
		LogType m_type;
		//! The message text.
		std::string m_message;
		//! Tells if the new line character is printed.
		bool m_newLine;
	};
	//! The message queue.
	using MessageQueue = std::deque< Message >;
	/**
	 *\~english
	 *\brief		Logging callback function.
	 *\param[in]	text	The logged text.
	 *\param[in]	type	The log type.
	 *\param[in]	newLine	Tells if we add the end line character.
	 *\~french
	 *\brief		Fonction de callback de log.
	 *\param[in]	text	Le texte écrit.
	 *\param[in]	type	Le type de log.
	 *\param[in]	newLine	Dit si on ajoute le caractère de fin de ligne.
	 */
	using LogCallback = std::function< void ( String const & text, LogType type, bool newLine ) >;

	/*!
	\~english
	\brief		Supported MemoryData types.
	\~french
	\brief		Types de MemoryData supportés.
	*/
	enum class MemoryDataType
	{
		eFixed,
		eMarked,
		eFixedGrowing,
		eFixedGrowingMarked,
		Utils_ScopedEnumBounds( eFixed )
	};

	template< typename Object, MemoryDataType MemDataType > class PoolManagedObject;

	class NonAlignedMemoryAllocator;
	template< size_t Align > class AlignedMemoryAllocator;

	template< typename Object, typename MemoryAllocator = NonAlignedMemoryAllocator > class FixedSizeMemoryData;
	template< typename Object, typename MemoryAllocator = NonAlignedMemoryAllocator > class FixedGrowingSizeMemoryData;
	template< typename Object > class FixedSizeMarkedMemoryData;
	template< typename Object > class FixedGrowingSizeMarkedMemoryData;

	inline StringStream makeStringStream()
	{
		static std::locale const loc{ "C" };
		StringStream result;
		result.imbue( loc );
		return result;
	}
}

constexpr utils::real operator "" _r( long double value )
{
	return utils::real( value );
}

constexpr utils::Seconds operator "" _s( unsigned long long value )
{
	return utils::Seconds( int64_t( value ) );
}

constexpr utils::Milliseconds operator "" _ms( unsigned long long value )
{
	return utils::Milliseconds( int64_t( value ) );
}

constexpr utils::Microseconds operator "" _us( unsigned long long value )
{
	return utils::Microseconds( int64_t( value ) );
}

constexpr utils::Nanoseconds operator "" _ns( unsigned long long value )
{
	return utils::Nanoseconds( int64_t( value ) );
}

#include "Utils/Miscellaneous/Debug.hpp"

#endif
