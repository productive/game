#ifndef _CORE_STRING_HASH_
#define _CORE_STRING_HASH_
#pragma once

#include <stdio.h>
#include <string.h>

//\brief Class that stores a Cstring along with it's CRC32 based hash for comparison
//		 All hash generation is lower case only so comparisons will be case insensitive
class StringHash
{
public:

	//\brief Create a Cyclic Redundancy Check value for a string
	//\param a_string the cString to generate from
	//\param a_convertToLower to convert to downcase
	unsigned int GenerateCRC(const char * a_string, bool a_convertToLower);

	//\brief Create a Cyclic Redundancy Check value for some binary data
	//\param a_string the cString to generate from
	//\param a_convertToLower to convert to downcase
	unsigned int GenerateCRCBinary(const unsigned int * a_binaryData, unsigned int a_length);

	//\brief No arg constructor so a StringHash can be used in array initialisers
	//		 It's pretty dangerous to do this however as the data will be unset
	StringHash();

	//\brief Construct the class and copy the string characters to a member
	StringHash(const char * a_sourceString);

	//\brief Reset the string and hash as per the const char constructor
	void SetCString(const char * a_newString);

	//\brief Accessor for the original cString data
	//\return pointer to the head of the cstring
	const char * GetCString() { return m_cString; }

	//\brief The most useful part of the string hash is the comparison
	bool operator == (const StringHash & a_compare) const { return m_hash == a_compare.m_hash; }

private:

	static const unsigned int s_stdCRCTable[256];		// Hash table generated using CRC-32 in PKZip, WinZip and Ethernet

	//\brief Convert a character to it's lower case equivalent
	//\param a_char the character to downcase
	//\return the lower case equivalent char
	unsigned char ConvertToLower(unsigned char a_char)
	{
		// CH:TODO
		return a_char;
	}

	unsigned int m_hash;	// Storage for the crc equivalent
	char m_cString[256];	// Storage for the original string
};

#endif //_CORE_STRING_HASH