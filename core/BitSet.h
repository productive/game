#ifndef _CORE_BITSET_
#define _CORE_BITSET_
#pragma once

// Values for bit counting by lookup table
#define B2(n) n,     n+1,     n+1,     n+2
#define B4(n) B2(n), B2(n+1), B2(n+1), B2(n+2)
#define B6(n) B4(n), B4(n+1), B4(n+1), B4(n+2)

//\brief Basic 32 bit set using an unsigned int to store bits with utility functions.
//		 Assuming little endianness.
class BitSet
{
public:

	// Constructors
	inline BitSet() : m_bits(0x0u) {}

	//\brief Bit manipulation functions
	//\param a_bitNo is the number of the bit to change
	inline void Set(unsigned int a_bitNo) { m_bits |= (1 << a_bitNo); }
	inline void Clear(unsigned int a_bitNo) { m_bits &= ~(1 << a_bitNo); }
	inline void Flip(unsigned int a_bitNo) { m_bits ^= (1 << a_bitNo); }
	inline void ClearAll() { m_bits = 0x0u; }

	//\brief Bit testing functions
	//\param a_bitNo is the number of the bit to test
	//\return bool true if the bit is set otherwise false
	inline bool IsBitSet(unsigned int a_bitNo) { return ((1 << a_bitNo) & m_bits) != 0x0u; }
	inline unsigned int GetBits() { return m_bits; }

	//\brief Bit counting functions, slow methods included for comprehensive purposes
	inline unsigned int GetBitCountBasic() const
	{
		unsigned int numBits = 0;
		for (unsigned int i = 0; i < sizeof(unsigned int) * 8; ++i)
		{
			if (((1 << i) & m_bits) != 0x0u) 
			{
				++numBits;
			}
		}

		return numBits;
	}
	//\brief A bit faster than the previous method as it will stop early due to the while loop
	inline unsigned int GetBitCountSlow() const
	{

		int n=0;
		unsigned bCopy = m_bits;
        while (bCopy)
        {
            bCopy &= bCopy-1;
            n++;
        }
        return n;
	}
	inline unsigned int GetBitCountLookupTable() const
	{
		// It's silly having this lookup table generated here and not declared as a static const member.
		// This method is for educational purposes only.
		const unsigned char bits_in_byte[256] = { B6(0), B6(1), B6(1), B6(2) };
		
		// Chop up the 32 bits into 4 single byte values
		char byte0 = m_bits & 0xff;
		char byte1 = (m_bits >> 8)	& 0xff;
		char byte2 = (m_bits >> 16) & 0xff;
		char byte3 = (m_bits >> 24) & 0xff;

		return bits_in_byte[byte0] + bits_in_byte[byte1] + bits_in_byte[byte2] + bits_in_byte[byte3];
	}
	inline unsigned int GetBitCount() const
	{
		// MIT HAKMEM 169 Trickyness
		unsigned int numBits = m_bits - ((m_bits >> 1) & 033333333333) - ((m_bits >> 2) & 011111111111);
		return ((numBits + (numBits >> 3)) & 030707070707) % 63;
	}

private:
	unsigned int m_bits; ///< Storage for 32 of the finest bits
};

#endif _CORE_BITSET_