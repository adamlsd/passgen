#include <iostream>
#include <fstream>
#include <vector>
#include <iterator>
#include <algorithm>
#include <string>
#include <exception>
#include <stdexcept>
#include <random>
#include <cstdint>
#include <cctype>
#include <cstdlib>
#include <cstddef>
#include <climits>
#include <cassert>
#include <boost/lexical_cast.hpp>

#undef BITS
#undef DOMAIN
#undef SKIP
#undef DOIT
#undef DEBUG

#define DEBUG SKIP
#define DOIT if( true )
#define SKIP if( false )


using std::begin;
using std::end;

namespace
{
	namespace C
	{
		const std::size_t bits= 4;
		const std::size_t domain= 16;
	}

	class Failure : public std::runtime_error
	{
		public:
			explicit Failure() : std::runtime_error( "Failure" ) {}
			explicit Failure( const std::string &s ) : std::runtime_error( "Failure: " + s ) {}
	};

	auto
	openRandom()
	{
		std::ifstream r( "/dev/urandom" );
		if( r.bad() || r.fail() ) throw Failure();

		return r;
	};

	template< typename T >
	class safe_vector : public std::vector< T >
	{
		public:
			using std::vector< T >::vector;
			template< typename Idx >
			auto operator []( const Idx &i ) const { return this->at( i ); }

			template< typename Idx >
			auto operator []( const Idx &i ) { return this->at( i ); }
	};
}


int
main( const int argcnt, const char *const *const argvec )
try
{
	auto rnd= openRandom();

	const int digitsDesired = ( argcnt == 1 ) ? 8 : boost::lexical_cast< int >( argvec[ 1 ] );

	std::cout << "We are going to make a pin that has " << digitsDesired << " digits." << std::endl;
	
	uint64_t randomness;
	std::string digits;
	std::size_t bitsInRnd= 0;

	do
	{
		if( bitsInRnd < C::bits )
		{
			rnd.read( reinterpret_cast< char * >( &randomness ), sizeof( randomness ) );
			if( rnd.bad() || rnd.fail() || rnd.eof() ) throw Failure();
			bitsInRnd= sizeof( randomness ) * CHAR_BIT;
		}

		const auto digit= randomness % ( C::domain );
		randomness>>= C::bits;
		bitsInRnd-= C::bits;

		if( digit > 9 ) continue;

		DEBUG std::cout << digit << std::endl;

		digits.push_back( digit + '0' );
	}
	while( digits.size() < digitsDesired );

	std::cout << "[32m" << digits << "[0m" << std::endl;
	std::cout << "Your pin has " << digits.size() << " words in its makeup." << std::endl;

	return EXIT_SUCCESS;
}
catch( const std::exception &ex )
{
	std::cerr << "Error: " << ex.what() << std::endl;
	return EXIT_FAILURE;
}
