/****************************************************************************
*																			*
*						cryptlib Bignum Support Routines					*
*						Copyright Peter Gutmann 1995-2014					*
*																			*
****************************************************************************/

#define PKC_CONTEXT		/* Indicate that we're working with PKC contexts */
#if defined( INC_ALL )
  #include "crypt.h"
  #include "context.h"
#else
  #include "crypt.h"
  #include "context/context.h"
#endif /* Compiler-specific includes */

#ifdef USE_PKC

/* The vast numbers of iterated and/or recursive calls to bignum code means 
   that any diagnostic print routines produce an enormous increase in 
   runtime, to deal with this we define a conditional value that can be used 
   to control printing of output.  In addition where possible the diagnostic 
   code itself tries to minimise the conditions under which it produces 
   output */

#ifndef NDEBUG
static const BOOLEAN diagOutput = FALSE;
#endif /* !NDEBUG */

/* If we're not using dynamically-allocated bignums then we need to convert 
   the bn_expand() macros that are used throughout the bignum code into 
   no-ops.  The following value represents a non-null location that can be
   used in the bn_expand() macros */

#ifndef BN_ALLOC
int nonNullAddress;
#endif /* BN_ALLOC */

/* If we're debugging the bignum allocation code then the clBnAlloc() macro
   points to the following function */

#ifdef USE_BN_DEBUG_MALLOC

void *clBnAllocFn( const char *fileName, const char *fnName,
				   const int lineNo, size_t size )
	{
	printf( "BNDEBUG: %s:%s:%d %d bytes.\n", fileName, fnName, lineNo, size );
	return( malloc( size ) );
	}
#endif /* USE_BN_DEBUG_MALLOC */

/****************************************************************************
*																			*
*								Utility Functions							*
*																			*
****************************************************************************/

/* Make sure that a bignum/BN_CTX's metadata is valid */

CHECK_RETVAL_LENGTH_SHORT_NOERROR STDC_NONNULL_ARG( ( 1 ) ) \
int getBNMaxSize( const BIGNUM *bignum )
	{
	assert( isReadPtr( bignum, sizeof( BIGNUM ) ) );

	return( ( bignum->flags & BN_FLG_ALLOC_EXT ) ? BIGNUM_ALLOC_WORDS_EXT : \
			( bignum->flags & BN_FLG_ALLOC_EXT2 ) ? BIGNUM_ALLOC_WORDS_EXT2 : \
			BIGNUM_ALLOC_WORDS );
	}

CHECK_RETVAL_BOOL STDC_NONNULL_ARG( ( 1 ) ) \
BOOLEAN sanityCheckBignum( const BIGNUM *bignum )
	{
	assert( isReadPtr( bignum, sizeof( BIGNUM ) ) );

	if( bignum->dmax < 1 || bignum->dmax > getBNMaxSize( bignum ) )
		return( FALSE );
	if( bignum->top < 0 || bignum->top > bignum->dmax )
		return( FALSE );
	if( bignum->neg != TRUE && bignum->neg != FALSE )
		return( FALSE );
	if( bignum->flags < BN_FLG_NONE || bignum->flags > BN_FLG_MAX )
		return( FALSE );

	return( TRUE );
	}

CHECK_RETVAL_BOOL STDC_NONNULL_ARG( ( 1 ) ) \
BOOLEAN sanityCheckBNCTX( const BN_CTX *bnCTX )
	{
	assert( isReadPtr( bnCTX, sizeof( BN_CTX ) ) );

	if( bnCTX->bnArrayMax < 0 || bnCTX->bnArrayMax > BN_CTX_ARRAY_SIZE )
		return( FALSE );
	if( bnCTX->stackPos < 0 || bnCTX->stackPos >= BN_CTX_ARRAY_SIZE )
		return( FALSE );

	return( TRUE );
	}

CHECK_RETVAL_BOOL STDC_NONNULL_ARG( ( 1 ) ) \
BOOLEAN sanityCheckBNMontCTX( const BN_MONT_CTX *bnMontCTX )
	{
	assert( isReadPtr( bnMontCTX, sizeof( bnMontCTX ) ) );

	if( !sanityCheckBignum( &bnMontCTX->RR ) || \
		!sanityCheckBignum( &bnMontCTX->N ) )
		return( FALSE );
	if( bnMontCTX->ri < 0 || \
		bnMontCTX->ri > bytesToBits( CRYPT_MAX_PKCSIZE ) )
		return( FALSE );
	if( bnMontCTX->flags != 0 && bnMontCTX->flags != BN_FLG_MALLOCED )
		return( FALSE );

	return( TRUE );
	}

/****************************************************************************
*																			*
*						 Miscellaneous Bignum Routines						*
*																			*
****************************************************************************/

/* Allocate/initialise/clear/free bignums.  The original OpenSSL bignum code 
   allocates storage on-demand, which results in both lots of kludgery to 
   deal with array bounds and buffer sizes moving around, and huge numbers 
   of memory-allocation/reallocation calls as bignum data sizes creep slowly 
   upwards until some sort of steady state is reached, whereupon the bignum 
   is destroyed and a new one allocated and the whole cycle begins anew.

   To avoid all of this memory-thrashing we use a fixed-size memory block 
   for each bignum, which is unfortunately somewhat wasteful but saves a 
   lot of memory allocation/reallocation and accompanying heap fragmentation 
   (see also the BN_CTX code comments further down).

   A useful side-effect of the elimination of dynamic memory allocation is 
   that the large number of null pointer dereferences on allocation failure 
   in the OpenSSL bignum code are never triggered because there's always 
   memory allocated for the bignum */

STDC_NONNULL_ARG( ( 1 ) ) \
void BN_clear( INOUT BIGNUM *bignum )
	{
	assert( isWritePtr( bignum, sizeof( BIGNUM ) ) );

	REQUIRES_V( sanityCheckBignum( bignum ) );

	if( !( bignum->flags & BN_FLG_STATIC_DATA ) )
		{
		DEBUG_PRINT_COND( diagOutput && bignum->top > 64, \
						  ( "BN max.size = %d words.\n", bignum->top ) );
		zeroise( bignum->d, bnWordsToBytes( bignum->dmax ) );
		bignum->top = bignum->neg = 0;
		}
	}

STDC_NONNULL_ARG( ( 1 ) ) \
void BN_init( OUT BIGNUM *bignum )
	{
	assert( isWritePtr( bignum, sizeof( BIGNUM ) ) );

	memset( bignum, 0, sizeof( BIGNUM ) );
	bignum->dmax = BIGNUM_ALLOC_WORDS;
	}

STDC_NONNULL_ARG( ( 1 ) ) \
static void BN_init_ext( INOUT void *bignumExtPtr, 
						 const BOOLEAN isExt2Bignum )
	{
	assert( isWritePtr( bignumExtPtr, sizeof( BIGNUM_EXT ) ) );

	if( isExt2Bignum )
		{
		BIGNUM_EXT2 *bignum = bignumExtPtr;

		memset( bignum, 0, sizeof( BIGNUM_EXT2 ) );
		bignum->dmax = BIGNUM_ALLOC_WORDS_EXT2;
		bignum->flags = BN_FLG_ALLOC_EXT2;
		}
	else
		{
		BIGNUM_EXT *bignum = bignumExtPtr;

		memset( bignum, 0, sizeof( BIGNUM_EXT ) );
		bignum->dmax = BIGNUM_ALLOC_WORDS_EXT;
		bignum->flags = BN_FLG_ALLOC_EXT;
		}
	}

CHECK_RETVAL_PTR \
BIGNUM *BN_new( void )
	{
	BIGNUM *bignum;

	bignum = clAlloc( "BN_new", sizeof( BIGNUM ) );
	if( bignum == NULL )
		return( NULL );
	BN_init( bignum );
	bignum->flags = BN_FLG_MALLOCED;

	return( bignum );
	}

STDC_NONNULL_ARG( ( 1 ) ) \
void BN_free( INOUT BIGNUM *bignum )
	{
	assert( isWritePtr( bignum, sizeof( BIGNUM ) ) );

	BN_clear( bignum );
	if( bignum->flags & BN_FLG_MALLOCED )
		clFree( "BN_free", bignum );
	}

/* Duplicate, swap bignums */

CHECK_RETVAL_PTR STDC_NONNULL_ARG( ( 1 ) ) \
BIGNUM *BN_dup( const BIGNUM *bignum )
	{
	BIGNUM *newBignum;

	assert( isReadPtr( bignum, sizeof( BIGNUM ) ) );

	newBignum = BN_new();
	if( newBignum == NULL ) 
		return( NULL );
	if( BN_copy( newBignum, bignum ) == NULL )
		{
		BN_free( newBignum );

		return( NULL );
		}

	return( newBignum );
	}

CHECK_RETVAL_PTR STDC_NONNULL_ARG( ( 1, 2 ) ) \
BIGNUM *BN_copy( INOUT BIGNUM *destBignum, const BIGNUM *srcBignum )
	{
	assert( isWritePtr( destBignum, sizeof( BIGNUM ) ) );
	assert( isReadPtr( srcBignum, sizeof( BIGNUM ) ) );

	REQUIRES_N( sanityCheckBignum( destBignum ) );
	REQUIRES_N( sanityCheckBignum( srcBignum ) );
	REQUIRES_N( destBignum->dmax >= srcBignum->top );

	/* Copy most of the bignum fields.  We don't copy the maximum-size field
	   or the flags field since these may differ for the two bignums (the 
	   flags field will be things like BN_FLG_MALLOCED, BN_FLG_STATIC_DATA,
	   BN_FLG_ALLOC_EXT and BN_FLG_ALLOC_EXT2) */
	memcpy( destBignum->d, srcBignum->d, bnWordsToBytes( srcBignum->top ) );
	destBignum->top = srcBignum->top;
	destBignum->neg = srcBignum->neg;

	return( destBignum );
	}

STDC_NONNULL_ARG( ( 1, 2 ) ) \
void BN_swap( INOUT BIGNUM *bignum1, INOUT BIGNUM *bignum2 )
	{
	BIGNUM tmp, *bnPtr;

	assert( isWritePtr( bignum1, sizeof( BIGNUM ) ) );
	assert( isWritePtr( bignum2, sizeof( BIGNUM ) ) );

	REQUIRES_V( !( bignum1->flags & BN_FLG_STATIC_DATA ) );
	REQUIRES_V( !( bignum1->flags & BN_FLG_STATIC_DATA ) );

	BN_init( &tmp );
	bnPtr = BN_copy( &tmp, bignum1 );
	if( bnPtr != NULL )
		bnPtr = BN_copy( bignum1, bignum2 );
	if( bnPtr != NULL )
		bnPtr = BN_copy( bignum2, &tmp );
	BN_clear( &tmp );

	ENSURES_V( bnPtr != NULL );
	}

/* Get a bignum with the value 1 */

CHECK_RETVAL_PTR \
const BIGNUM *BN_value_one( void )
	{
	static const BIGNUM bignum = { BIGNUM_ALLOC_WORDS, 1, FALSE, 
								   BN_FLG_STATIC_DATA, { 1, 0, 0, 0 } };

	/* Catch problems arising from changes to bignum struct layout */
	assert( bignum.dmax == BIGNUM_ALLOC_WORDS );
	assert( bignum.top == 1 );
	assert( bignum.neg == FALSE );
	assert( bignum.flags == BN_FLG_STATIC_DATA );
	assert( bignum.d[ 0 ] == 1 && bignum.d[ 1 ] == 0 && bignum.d[ 2 ] == 0 );

	return( &bignum );
	}

/****************************************************************************
*																			*
*						 Manipulate Bignum Values/Data						*
*																			*
****************************************************************************/

/* Get/set a bignum as a word value */

STDC_NONNULL_ARG( ( 1 ) ) \
BN_ULONG BN_get_word( const BIGNUM *bignum )
	{
	assert( isReadPtr( bignum, sizeof( BIGNUM ) ) );

	REQUIRES_EXT( sanityCheckBignum( bignum ), BN_NAN );

	/* If the result won't fit in a word, return a NaN indicator */
	if( bignum->top > 1 )
		return( BN_NAN );

	/* Bignums with the value zero have a length of zero so we don't try and
	   read a data value from them */
	if( bignum->top < 1 )
		return( 0 );

	return( bignum->d[ 0 ] );
	}

CHECK_RETVAL_BOOL STDC_NONNULL_ARG( ( 1 ) ) \
BOOLEAN BN_set_word( INOUT BIGNUM *bignum, const BN_ULONG word )
	{
	assert( isWritePtr( bignum, sizeof( BIGNUM ) ) );

	REQUIRES_B( sanityCheckBignum( bignum ) );
	REQUIRES_B( !( bignum->flags & BN_FLG_STATIC_DATA ) );

	BN_clear( bignum );
	bignum->d[ 0 ] = word;
	bignum->top = word ? 1 : 0;

	return( TRUE );
	}

/* Count the number of bits used in a word and in a bignum.  The former is 
   the classic log2 problem for which there are about a million clever hacks 
   (including stuffing them into IEEE-754 64-bit floats and fiddling with 
   the bit-representation of those) but they're all endianness/word-size/
   whatever-dependent and since this is never called in time-critical code 
   we just use a straight loop, which works everywhere */

CHECK_RETVAL_LENGTH_SHORT \
int BN_num_bits_word( const BN_ULONG word )
	{
	BN_ULONG value = word;
	int i;

	for( i = 0; i < 128 && value > 0; i++ )
		value >>= 1;
	ENSURES( i < 128 );

	return( i );
	}

CHECK_RETVAL_LENGTH_SHORT STDC_NONNULL_ARG( ( 1 ) ) \
int BN_num_bits( const BIGNUM *bignum )
	{
	const int lastWordIndex = bignum->top - 1;
	int bits, status;

	assert( isReadPtr( bignum, sizeof( BIGNUM ) ) );

	REQUIRES( sanityCheckBignum( bignum ) );

	/* Bignums with value zero are special-cased since they have a length of
	   zero */
	if( bignum->top <= 0 )
		return( 0 );

	status = bits = BN_num_bits_word( bignum->d[ lastWordIndex ] );
	if( cryptStatusError( status ) )
		return( status );
	return( ( lastWordIndex * BN_BITS2 ) + bits );
	}

/* Bit-manipulation operations */

CHECK_RETVAL_BOOL STDC_NONNULL_ARG( ( 1 ) ) \
BOOLEAN BN_set_bit( INOUT BIGNUM *bignum, 
					IN_RANGE( 0, bytesToBits( CRYPT_MAX_PKCSIZE ) ) \
						int bitNo )
	{
	const int wordIndex = bitNo / BN_BITS2;
	const int bitIndex = bitNo % BN_BITS2;
	const int iterationBound = getBNMaxSize( bignum );

	assert( isWritePtr( bignum, sizeof( BIGNUM ) ) );

	REQUIRES_B( sanityCheckBignum( bignum ) );
	REQUIRES_B( !( bignum->flags & BN_FLG_STATIC_DATA ) );
	REQUIRES_B( bitNo >= 0 && bitNo < bnWordsToBits( bignum->dmax ) );

	/* If we're extending the bignum, clear the words up to where we insert 
	   the bit.
	   
	   Note that the use of the unified BIGNUM type to also represent a 
	   BIGNUM_EXT/BIGNUM_EXT2 can result in false-positive warnings from 
	   bounds-checking applications that apply the d[] array size from a 
	   BIGNUM to the much larger array in a BIGNUM_EXT/BIGNUM_EXT2 */
	if( bignum->top < wordIndex + 1 )
		{
		int index;

		REQUIRES_B( wordIndex < bignum->dmax );
		for( index = bignum->top; index < wordIndex + 1; index++ )
			bignum->d[ index ] = 0;
		ENSURES_B( index < iterationBound );
		bignum->top = wordIndex + 1;
		}

	/* Set the appropriate bit location */
	bignum->d[ wordIndex ] |= ( BN_ULONG ) 1 << bitIndex;

	ENSURES_B( sanityCheckBignum( bignum ) );

	return( TRUE );
	}

CHECK_RETVAL_BOOL STDC_NONNULL_ARG( ( 1 ) ) \
BOOLEAN BN_is_bit_set( const BIGNUM *bignum, /* See comment */ int bitNo )
	{
	const int wordIndex = bitNo / BN_BITS2;
	const int bitIndex = bitNo % BN_BITS2;

	assert( isReadPtr( bignum, sizeof( BIGNUM ) ) );

	REQUIRES_B( sanityCheckBignum( bignum ) );
	REQUIRES_B( bitNo < bnWordsToBits( bignum->dmax ) );	/* See comment below */

	/* The OpenSSL bignum code occasionally calls this with negative values 
	   for the bit to check (e.g. the Montgomery modexp code, which contains 
	   a comment that explicitly says it'll be calling this function with 
	   negative bit values) so we have to special-case this condition */
	if( bitNo < 0 )
		return( 0 );

	/* Bits off the end of the bignum are always zero */
	if( wordIndex >= bignum->top )
		return( 0 );

	return( ( bignum->d[ wordIndex ] & ( ( BN_ULONG ) 1 << bitIndex ) ) ? \
			TRUE : FALSE );
	}

CHECK_RETVAL_BOOL STDC_NONNULL_ARG( ( 1 ) ) \
BOOLEAN BN_high_bit( const BIGNUM *bignum )
	{
	int noBytes = BN_num_bytes( bignum ) - 1;
	BN_ULONG highWord;
	int highByte;

	assert( isReadPtr( bignum, sizeof( BIGNUM ) ) );

	REQUIRES_B( sanityCheckBignum( bignum ) );

	/* Bignums with value zero are special-cased since they have a length of
	   zero */
	if( noBytes < 0 )
		return( 0 );

	/* Extract the topmost nonzero byte in the bignum */
	highWord = bignum->d[ noBytes / BN_BYTES ];
	highByte = ( int ) ( highWord >> ( ( noBytes % BN_BYTES ) * 8 ) );

	return( ( highByte & 0x80 ) ? 1 : 0 );
	}

/* Set the sign flag on a bignum.  This is almost universally ignored by the
   OpenSSL code, which manipulates the sign value directly */

STDC_NONNULL_ARG( ( 1 ) ) \
void BN_set_negative( INOUT BIGNUM *bignum, const int value )
	{
	assert( isWritePtr( bignum, sizeof( BIGNUM ) ) );

	if( BN_is_zero( bignum ) )
		return;
	bignum->neg = value ? TRUE : FALSE;
	}

/* A bignum operation may have reduced the magnitude of the bignum value,
   in which case bignum->top will be left pointing to the head of a long
   string of zeroes.  The following function normalises the representation,
   leaving bignum->top pointing to the first nonzero entry */

RETVAL_BOOL STDC_NONNULL_ARG( ( 1 ) ) \
BOOLEAN BN_normalise( INOUT BIGNUM *bignum )
	{
	const int iterationBound = getBNMaxSize( bignum );
	int iterationCount;

	REQUIRES_B( sanityCheckBignum( bignum ) );

	/* If it's a zero-magnitude bignum then there's nothing to do */
	if( BN_is_zero( bignum ) )
		return( TRUE );

	/* Note that the use of the unified BIGNUM type to also represent a 
	   BIGNUM_EXT/BIGNUM_EXT2 can result in false-positive warnings from 
	   bounds-checking applications that apply the d[] array size from a 
	   BIGNUM to the much larger array in a BIGNUM_EXT/BIGNUM_EXT2 */
	for( iterationCount = 0;
		 bignum->top > 0 && iterationCount < iterationBound;
		 bignum->top--, iterationCount++ )
		{
		if( bignum->d[ bignum->top - 1 ] != 0 )
			break;
		}
	ENSURES_B( iterationCount < iterationBound );

	ENSURES_B( sanityCheckBignum( bignum ) );

	return( TRUE );
	}

/****************************************************************************
*																			*
*							BN_CTX Support Routines 						*
*																			*
****************************************************************************/

/* The BN_CTX code up until about 2000 (or cryptlib 3.21) used to be just an 
   array of BN_CTX_NUM = 32 BIGNUMs, then after that it was replaced by an
   awkward pool/stack combination that makes something relatively 
   straighforward quite complex.  What's needed is a way of stacking and
   unstacking blocks of BN_CTX_get()s in nested functions:

	BN_foo()
		BN_CTX_start();
		foo_a = BN_CTX_get();
		foo_b = BN_CTX_get();
		foo_c = BN_CTX_get();
		BN_bar()
			BN_CTX_start();
			bar_a = BN_CTX_get();
			bar_b = BN_CTX_get();
			BN_CTX_end();
		BN_CTX_end();

   where the first BN_CTX_end() frees up the BNs grabbed in BN_bar() and the
   second frees up the ones grabbed in BN_foo().  This is why we have the
   stack alongside the bignum array, every time we increase the nesting depth
   by calling BN_CTX_start() we remember the stack position that we need to 
   unwind to when BN_CTX_end() is called.

   All of the complex stack/pool manipulations in the original code, 
   possibly meant to "optimise" the strategy of allocating a single fixed-
   size block of values, actually have a negative effect on memory use 
   because the bookkeeping overhead of dozens of tiny little allocations is 
   more than just allocating the fixed-size block.  Since we can measure the 
   deepest that the allocation ever goes we just use a fixed-size array of 
   bignums set to BN_CTX_ARRAY_SIZE.
   
   The init and end functions just set up and clear a BN_CTX */

STDC_NONNULL_ARG( ( 1 ) ) \
void BN_CTX_init( OUT BN_CTX *bnCTX )
	{
	int i;

	assert( isWritePtr( bnCTX, sizeof( BN_CTX ) ) );

	memset( bnCTX, 0, sizeof( BN_CTX ) );
	for( i = 0; i < BN_CTX_ARRAY_SIZE; i++ )
		BN_init( &bnCTX->bnArray[ i ] );
	ENSURES_V( i == BN_CTX_ARRAY_SIZE );
	for( i = 0; i < BN_CTX_EXTARRAY_SIZE; i++ )
		BN_init_ext( &bnCTX->bnExtArray[ i ], FALSE );
	ENSURES_V( i == BN_CTX_EXTARRAY_SIZE );
	for( i = 0; i < BN_CTX_EXT2ARRAY_SIZE; i++ )
		BN_init_ext( &bnCTX->bnExt2Array[ i ], TRUE );
	ENSURES_V( i == BN_CTX_EXT2ARRAY_SIZE );
	}

STDC_NONNULL_ARG( ( 1 ) ) \
void BN_CTX_final( INOUT BN_CTX *bnCTX )
	{
	int i;

	assert( isWritePtr( bnCTX, sizeof( BN_CTX ) ) );

	REQUIRES_V( sanityCheckBNCTX( bnCTX ) );

	/* Clear the overall BN_CTX */
	zeroise( bnCTX, sizeof( BN_CTX ) );

	/* The various bignums were cleared when the BN_CTX was zeroised, we now 
	   have to reset them to their initial state so that they can be 
	   reused */
	for( i = 0; i < BN_CTX_ARRAY_SIZE; i++ )
		BN_init( &bnCTX->bnArray[ i ] );
	ENSURES_V( i == BN_CTX_ARRAY_SIZE );
	for( i = 0; i < BN_CTX_EXTARRAY_SIZE; i++ )
		BN_init_ext( &bnCTX->bnExtArray[ i ], FALSE );
	ENSURES_V( i == BN_CTX_EXTARRAY_SIZE );
	DEBUG_PRINT_COND( diagOutput, ( "EXT_MUL1 freed.\nEXT_MUL2 freed.\n" ));
	for( i = 0; i < BN_CTX_EXT2ARRAY_SIZE; i++ )
		BN_init_ext( &bnCTX->bnExt2Array[ i ], TRUE );
	ENSURES_V( i == BN_CTX_EXT2ARRAY_SIZE );
	DEBUG_PRINT_COND( diagOutput, ( "EXT_MONT freed.\n" ));
	}

/* The start and badly-named end functions (it should be finish()) remember 
   the current stack position and unwind to the last stack position */

STDC_NONNULL_ARG( ( 1 ) ) \
void BN_CTX_start( INOUT BN_CTX *bnCTX )
	{
	assert( isWritePtr( bnCTX, sizeof( BN_CTX ) ) );

	REQUIRES_V( sanityCheckBNCTX( bnCTX ) );

	/* Advance one stack frame */
	bnCTX->stackPos++;
	bnCTX->stack[ bnCTX->stackPos ] = bnCTX->stack[ bnCTX->stackPos - 1 ];

	ENSURES_V( sanityCheckBNCTX( bnCTX ) );
	}

STDC_NONNULL_ARG( ( 1 ) ) \
void BN_CTX_end( INOUT BN_CTX *bnCTX )
	{
	int i;

	assert( isWritePtr( bnCTX, sizeof( BN_CTX ) ) );

	REQUIRES_V( sanityCheckBNCTX( bnCTX ) );
	REQUIRES_V( bnCTX->stack[ bnCTX->stackPos - 1 ] <= \
						bnCTX->stack[ bnCTX->stackPos ] );

	/* Only enable the following when required, the bignum code performs
	   a huge number of stackings and un-stackings for which the following
	   produces an enormous increase in runtime */
#if 0	
	DEBUG_PRINT(( "bnCTX unstacking from %d to %d.\n", 
				  bnCTX->stack[ bnCTX->stackPos - 1 ],
				  bnCTX->stack[ bnCTX->stackPos ] ));
#endif /* 0 */

	/* Clear each bignum in the current stack frame */
	for( i = bnCTX->stack[ bnCTX->stackPos - 1 ]; 
		 i < bnCTX->stack[ bnCTX->stackPos ] && i < BN_CTX_ARRAY_SIZE; i++ )
		{
		BN_clear( &bnCTX->bnArray[ i ] );
		}
	ENSURES_V( i < BN_CTX_ARRAY_SIZE );

	/* Unwind the stack by one frame */
	bnCTX->stack[ bnCTX->stackPos ] = 0;
	bnCTX->stackPos--;

	ENSURES_V( sanityCheckBNCTX( bnCTX ) );
	}

/* Peel another bignum off the BN_CTX array */

CHECK_RETVAL_PTR STDC_NONNULL_ARG( ( 1 ) ) \
BIGNUM *BN_CTX_get( INOUT BN_CTX *bnCTX )
	{
	BIGNUM *bignum;
	const int arrayIndex = bnCTX->stack[ bnCTX->stackPos ] + 1;

	assert( isWritePtr( bnCTX, sizeof( BN_CTX ) ) );

	if( bnCTX->bnArrayMax >= BN_CTX_ARRAY_SIZE )
		{
		assert( DEBUG_WARN );
		DEBUG_PRINT(( "bnCTX array size overflow.\n" ));

		return( NULL );
		}

	REQUIRES_N( sanityCheckBNCTX( bnCTX ) );

	/* Get the element at the previous top-of-stack */
	bignum = &bnCTX->bnArray[ arrayIndex - 1 ];

	/* Advance the top-of-stack element by one, and increase the last-used 
	   postion if it exceeds the existing one */
	bnCTX->stack[ bnCTX->stackPos ] = arrayIndex;
	if( arrayIndex > bnCTX->bnArrayMax )
		bnCTX->bnArrayMax = arrayIndex;

	ENSURES_N( sanityCheckBNCTX( bnCTX ) );

	/* Return the new element at the top of the stack */
	return( bignum );
	}

/* The bignum multiplication code requires a few temporary values that grow 
   to an enormous size, rather than resizing every bignum that we use to 
   deal with this we return fixed extra-size bignums when this is explicitly 
   required */

CHECK_RETVAL_PTR STDC_NONNULL_ARG( ( 1 ) ) \
BIGNUM *BN_CTX_get_ext( INOUT BN_CTX *bnCTX, 
						IN_ENUM( BIGNUM_EXT ) const BIGNUM_EXT_TYPE bnExtType )
	{
	assert( isWritePtr( bnCTX, sizeof( BN_CTX ) ) );

	ENSURES_N( bnExtType > BIGNUM_EXT_NONE && bnExtType < BIGNUM_EXT_LAST );

	switch( bnExtType )
		{
		case BIGNUM_EXT_MONT:
			DEBUG_PRINT_COND( diagOutput, ( "EXT_MONT acquired.\n" ));
			return( ( BIGNUM * ) &bnCTX->bnExtArray[ 0 ] );

		case BIGNUM_EXT_MUL1:
			DEBUG_PRINT_COND( diagOutput, ( "EXT_MUL1 acquired.\n" ));
			return( ( BIGNUM * ) &bnCTX->bnExt2Array[ 0 ] );

		case BIGNUM_EXT_MUL2:
			DEBUG_PRINT_COND( diagOutput, ( "EXT_MUL2 acquired.\n" ));
			return( ( BIGNUM * ) &bnCTX->bnExt2Array[ 1 ] );
		}

	retIntError_Null();
	}

STDC_NONNULL_ARG( ( 1 ) ) \
void BN_CTX_end_ext( INOUT BN_CTX *bnCTX, 
					 IN_ENUM( BIGNUM_EXT ) const BIGNUM_EXT_TYPE bnExtType )
	{
	/* Perform the standard context cleanup */
	BN_CTX_end( bnCTX );

	ENSURES_V( bnExtType == BIGNUM_EXT_MUL1 || \
			   bnExtType == BIGNUM_EXT_MONT );

	/* Clear the extended-size bignums */
	if( bnExtType == BIGNUM_EXT_MUL1 )
		{
		BN_clear( BN_CTX_get_ext( bnCTX, BIGNUM_EXT_MUL1 ) );
		DEBUG_PRINT_COND( diagOutput, ( "EXT_MUL1 cleared.\n" ));
		BN_clear( BN_CTX_get_ext( bnCTX, BIGNUM_EXT_MUL2 ) );
		DEBUG_PRINT_COND( diagOutput, ( "EXT_MUL2 cleared.\n" ));
		}
	else
		{
		BN_clear( BN_CTX_get_ext( bnCTX, BIGNUM_EXT_MONT ) );
		DEBUG_PRINT_COND( diagOutput, ( "EXT_MONT cleared.\n" ));
		}
	}

/* Dynamically allocate a BN_CTX, only needed by the ECC code */

#if defined( USE_ECDH ) || defined( USE_ECDSA )

CHECK_RETVAL_PTR \
BN_CTX *BN_CTX_new( void )
	{
	BN_CTX *bnCTX;

	bnCTX = clAlloc( "BN_CTX_new", sizeof( BN_CTX ) );
	if( bnCTX == NULL )
		return( NULL );
	BN_CTX_init( bnCTX );

	return( bnCTX );
	}

STDC_NONNULL_ARG( ( 1 ) ) \
void BN_CTX_free( INOUT BN_CTX *bnCTX )
	{
	assert( isWritePtr( bnCTX, sizeof( BN_CTX ) ) );

	REQUIRES_V( sanityCheckBNCTX( bnCTX ) );

	BN_CTX_final( bnCTX );
	clFree( "BN_CTX_free", bnCTX );
	}
#endif /* ECDH || ECDSA */

/****************************************************************************
*																			*
*							BN_MONT_CTX Support Routines 					*
*																			*
****************************************************************************/

/* Initialise/clear a BN_MONT_CTX */

STDC_NONNULL_ARG( ( 1 ) ) \
void BN_MONT_CTX_init( OUT BN_MONT_CTX *bnMontCTX )
	{
	assert( isWritePtr( bnMontCTX, sizeof( BN_MONT_CTX ) ) );

	memset( bnMontCTX, 0, sizeof( BN_MONT_CTX ) );
	BN_init( &bnMontCTX->RR );
	BN_init( &bnMontCTX->N );
	}

STDC_NONNULL_ARG( ( 1 ) ) \
void BN_MONT_CTX_free( INOUT BN_MONT_CTX *bnMontCTX )
	{
	assert( isWritePtr( bnMontCTX, sizeof( BN_MONT_CTX ) ) );

	BN_clear( &bnMontCTX->RR );
	BN_clear( &bnMontCTX->N );
#if defined( USE_ECDH ) || defined( USE_ECDSA )
	if( bnMontCTX->flags & BN_FLG_MALLOCED )
		clFree( "BN_MONT_CTX_free", bnMontCTX );
#endif /* ECDH || ECDSA */
	}

/* Dynamically allocate a BN_MONT_CTX, only needed by the ECC code */

#if defined( USE_ECDH ) || defined( USE_ECDSA )

CHECK_RETVAL_PTR \
BN_MONT_CTX *BN_MONT_CTX_new( void )
	{
	BN_MONT_CTX *bnMontCTX;

	bnMontCTX = clAlloc( "BN_MONT_CTX_new", sizeof( BN_MONT_CTX ) );
	if( bnMontCTX == NULL )
		return( NULL );
	BN_MONT_CTX_init( bnMontCTX );
	bnMontCTX->flags = BN_FLG_MALLOCED;

	return( bnMontCTX );
	}
#else

CHECK_RETVAL_PTR \
BN_MONT_CTX *BN_MONT_CTX_new( void )
	{
	assert( DEBUG_WARN );
	return( NULL );
	}
#endif /* ECDH || ECDSA */

/****************************************************************************
*																			*
*							BN_RECP_CTX Support Routines 					*
*																			*
****************************************************************************/

/* Initialise/clear a BN_RECP_CTX */

STDC_NONNULL_ARG( ( 1 ) ) \
void BN_RECP_CTX_init( OUT BN_RECP_CTX *bnRecpCTX )
	{
	assert( isWritePtr( bnRecpCTX, sizeof( BN_RECP_CTX ) ) );

	memset( bnRecpCTX, 0, sizeof( BN_RECP_CTX ) );

	BN_init( &bnRecpCTX->N );
	BN_init( &bnRecpCTX->Nr );
	}

STDC_NONNULL_ARG( ( 1 ) ) \
void BN_RECP_CTX_free( INOUT BN_RECP_CTX *bnRecpCTX )
	{
	assert( isWritePtr( bnRecpCTX, sizeof( BN_RECP_CTX ) ) );

	BN_clear( &bnRecpCTX->N );
	BN_clear( &bnRecpCTX->Nr );
	}

/* Initialise a BN_RECP_CTX.  The BN_CTX isn't used for anything, we keep it
   just to preserve the original function signature */

STDC_NONNULL_ARG( ( 1, 2, 3 ) ) \
int BN_RECP_CTX_set( INOUT BN_RECP_CTX *bnRecpCTX, const BIGNUM *d, 
					 STDC_UNUSED const BN_CTX *bnCTX )
	{
	assert( isWritePtr( bnRecpCTX, sizeof( BN_RECP_CTX ) ) );
	assert( isReadPtr( d, sizeof( BIGNUM ) ) );

	UNUSED_ARG( bnCTX );

	/* Clear context fields.  This should already have been done through an 
	   earlier call to BN_RECP_CTX_init(), but given that this is OpenSSL, 
	   we're extra conservative */
	BN_RECP_CTX_init( bnRecpCTX );

	/* N = bignum, Nr = 0 */
	if( !BN_copy( &bnRecpCTX->N, d ) )
		return( FALSE );
	BN_zero( &bnRecpCTX->Nr );

	/* Initialise metadata fields */
	bnRecpCTX->num_bits = BN_num_bits( d );

	return( TRUE );
	}

/****************************************************************************
*																			*
*								Self-test Routines							*
*																			*
****************************************************************************/

#ifndef NDEBUG

CHECK_RETVAL_BOOL \
BOOLEAN testIntBN( void )
	{
	if( !bnmathSelfTest() )
		return( FALSE );

	return( TRUE );
	}
#endif /* NDEBUG */

#endif /* USE_PKC */