#include "floatx.h"
#include <assert.h>
#include <limits.h> // for CHAR_BIT - number of bits per byte
#include <math.h> // for isinf and isnan
#include "bitFields.h"

floatx doubleToFloatx(double val,int totBits,int expBits) {

/*-----------------------------------------------------------------------------------------------
	From the README: 	At a high level, doing the conversion requires several manipulations:
	1. Extracting the sign bit from the double value, and inserting it into the floatx value at
		the correct position.
	2. Extract the biased exponent from the double value.
			Check to see if the double value is sub-normal.
			Check to make sure the floatx exponent won't overflow or underflow).
					If so, handle as a special case.
			If not, rebias the exponent using the floatx bias (which depends on the number of exponent bits)
			and write the result to the correct location in the floatx result.
	3. Extract the fraction bits from the double value.
			Determine how many bits are available for the fraction in the floatx value,
			and truncate or extend the original value,
			and write the resulting bits to the floatx result.
	4. Handle special cases, such as infinity, or not-a-number.		
	5. Return the floatx result.
----------------------------------------------------------------------------------------------------*/

	// First, make some assertions to ensure the totBits and expBits parameters are OK
	assert(totBits >= 3 && totBits <= 64);
    assert(expBits >= 1 && expBits <= totBits - 2);
	// Then, implement the algorithm

    union converter{
        double d;
        unsigned long l;
    } converter;

    unsigned long floatx = 0;
    
    const int doubleExpBits = 11;
    const int doubleFracBits = 52;
    const int expBiasDouble = (1 << (doubleExpBits - 1)) - 1;
    const int fracBits = totBits - expBits - 1; 
    const int expBiasFloatx = (1 << (expBits - 1)) - 1;

    converter.d = val;

    int sign = getBit(63, converter.l);
    unsigned long exponent = getBitFld(52, 11, converter.l);
    unsigned long fraction = getBitFld(0, 52, converter.l);

    setBit(totBits - 1, sign, &floatx);

    int new_exponent;
    if (exponent == 0) {
        if (fraction == 0) {
            new_exponent = 0;
        } else {
            new_exponent = 1 - expBiasDouble + expBiasFloatx;
        }
    } else if (exponent == 2047) {
        new_exponent = (1 << expBits) - 1; 
        fraction = (fraction == 0) ? 0 : (1UL << fracBits) - 1; 
    } else {
        new_exponent = (int)exponent - expBiasDouble + expBiasFloatx;
        if (new_exponent >= (1 << expBits)) {
            new_exponent = (1 << expBits) - 1;
            fraction = 0;
        } else if (new_exponent <= 0) {
            if (new_exponent < -fracBits) {
                new_exponent = 0;
                fraction = 0;
            } else {
                fraction >>= (-new_exponent + 1);
                new_exponent = 0;
            }
        }
    }

    setBitFld(totBits - expBits - 1, expBits, new_exponent, &floatx);

    if (fracBits < doubleFracBits) {
        fraction >>= (doubleFracBits - fracBits); 
    } else {
        fraction <<= (fracBits - doubleFracBits); 
    }
    setBitFld(0, fracBits, fraction, &floatx);

    return floatx;
}
