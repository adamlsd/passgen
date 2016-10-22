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

const std::uint64_t BITS= 18;
const std::uint64_t DOMAIN= 1 << BITS;

#define DEBUG SKIP
#define DOIT if( true )
#define SKIP if( false )

using std::begin;
using std::end;

namespace
{
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

	void
	dictStat( const std::vector< std::string > &d )
	{
		std::cout << "Dictionary statistics: " << std::endl;
		for( int i= 1; i < 25; ++i )
		{
			std::cout << i << " character words: ";
			std::cout << std::count_if( begin( d ), end( d ), [i]( auto &&s ) { return s.size() == i; } );
			std::cout << std::endl;
		}
	}


	auto
	getDictionary()
	{
		std::ifstream d( "dictionary" );
		if( d.bad() || d.fail() ) throw Failure();

		// We assume that the dictionary is unique -- it reduces load time.
		safe_vector< std::string > dict{ std::istream_iterator< std::string >( d ),
				std::istream_iterator< std::string >() };

		// dictStat( dict );

		// Remove words which are really small from the dictionary -- it makes for
		// somewhat slightly kinda sorta easier to crack passwords:
		//
		// Consider: each 1 or 2 letter word really is one or 2 bytes or 8 to 16 bits.
		// If we get a bad roll, such that there are maybe three such words, we have
		// 3 * 18 in our randomness pool -- good (54 bits of randomness), but it is
		// encoded in a form which will be covered by a brute-force crack of a much
		// smaller space.  Because short words are REALLY easy to remember, people may
		// bias their runs to select shorter words.  We need to prevent creating passwords
		// which can be in a (partial) collision space of a 2**64 brute-force character
		// search.
		//
		// Thus, 'a big dog in my car' is 6 words (and gives us 6 * 18 or 108 bits),
		// it's 18-characters, and thus really more like 18 base-32 or weaker coding
		// tokens -- in such a degenerate case, we still have like 90 bits, but people
		// may not even take 6 words: 'in my up on' is 11 characters, or maybe like 55
		// bits in base32 (being still generous).  Although it was made from 18-bit
		// random tokens (and thus really represents a 72 bit secret), its encoding is
		// in a 5-bit space, and thus not very secure.  A dumb-brute-force attacker
		// will do 2**88 possible total combinations, but a smart attacker will prioritize
		// the base32 and base64 space, thus shaving 22 or 33 bits off of that space.
		//
		// So to help ensure that a word representing 18-bits is encoded by MORE than
		// 15 possible bits of a base32, we cut out 1 and 2 character words.  For more safety,
		// bringing this to 4 is better.  (Granted that there are only 26 one-letter words,
		// and about 600 two-letter words, total, people will be selective in the passwords
		// that they keep from this program and attempt to memorize.  If we cut out the
		// possibility of hard passwords, we save pain in explaining good designs.
		DOIT dict.erase( std::remove_if( begin( dict ), end( dict ),
				[]( const auto &x ){ return x.size() < 4; } ), end( dict ) );

		// We shuffle before trim so that we aren't quite sure which words get thrown out.
		std::random_device rd;
		std::mt19937 gen( rd() );
		std::shuffle( begin( dict ), end( dict ), gen );

		// Make sure that we can reach the expected domain, and trim to that domain.
		if( ( DOMAIN ) > dict.size() )
				throw Failure( "Dict size: " + boost::lexical_cast< std::string >( dict.size() ) );
		dict.resize( DOMAIN );

		return dict;
	};
}


int
main( const int argcnt, const char *const *const argvec )
try
{
	const auto dict= getDictionary();

	auto rnd= openRandom();

	const auto bitsDesired = ( argcnt == 1 ) ? 64 : boost::lexical_cast< int >( argvec[ 1 ] );

	std::cout << "We are going to make a password at least as strong as a "
			<< bitsDesired << " bit secret" << std::endl;
	
	uint64_t randomness;
	int bits= 0;
	int bitsInRnd= 0;
	std::vector< std::string > words;

	do
	{
		if( bitsInRnd < BITS )
		{
			rnd.read( reinterpret_cast< char * >( &randomness ), sizeof( randomness ) );
			if( rnd.bad() || rnd.fail() || rnd.eof() ) throw Failure();
			bitsInRnd= sizeof( randomness ) * CHAR_BIT;
		}

		const auto &word= dict[ randomness % ( DOMAIN ) ];
		DEBUG std::cout << randomness % ( DOMAIN ) << std::endl;
		DEBUG std::cout << word << std::endl;
		randomness>>= BITS;
		words.push_back( word );

		bitsInRnd-= BITS;
		bits+= BITS;
	}
	while( bits < bitsDesired );

	std::ostringstream pws;
	std::copy( begin( words ), end( words ), std::ostream_iterator< std::string >( pws, " " ) );
	const std::string password= pws.str();
	std::cout << password << std::endl;
	std::cout << "Your password has " << bits << " bits of entropy in its makeup." << std::endl;

	std::cout << "Your password is roughly equivalent to " << password.size() << " base32 elements" << std::endl;

	return EXIT_SUCCESS;
}
catch( const std::exception &ex )
{
	std::cerr << "Error: " << ex.what() << std::endl;
	return EXIT_FAILURE;
}
