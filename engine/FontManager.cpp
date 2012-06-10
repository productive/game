#include <iostream>
#include <fstream>

#include "Log.h"
#include "RenderManager.h"
#include "Texture.h"

#include "FontManager.h"

using namespace std;	//< For fstream operations

template<> FontManager * Singleton<FontManager>::s_instance = NULL;

const float FontManager::s_debugFontSize = 0.01f;

bool FontManager::Startup(const char * a_fontPath)
{
	// Sanity check on input arg
	if (a_fontPath == NULL || a_fontPath[0] == 0)
	{
		return false;
	}

	// Populate a list of font configuration files
	FileManager::FileList fontFiles;
	FileManager::Get().FillFileList(a_fontPath, fontFiles, ".fnt");
	FileManager::FileListNode * curNode = fontFiles.GetHead();

	// Cache off the font path as textures are relative to fonts
	memset(&m_fontPath, 0 , StringUtils::s_maxCharsPerLine);
	strncpy(m_fontPath, a_fontPath, strlen(a_fontPath));

	// Load each font in the directory
	bool loadSuccess = true;
	while(curNode != NULL)
	{
		loadSuccess &= LoadFont(curNode->GetData()->m_name);
		curNode = curNode->GetNext();
	}

	// Clean up the list of fonts
	FileManager::Get().EmptyFileList(fontFiles);

	return loadSuccess;
}

bool FontManager::Shutdown()
{
	FontListNode * next = m_fonts.GetHead();
	while(next != NULL)
	{
		// Cache off next pointer
		FontListNode * cur = next;
		next = cur->GetNext();

		m_fonts.Remove(cur);
		delete cur->GetData();
		delete cur;
	}

	return true;
}

bool FontManager::LoadFont(const char * a_fontName)
{
	// Cstrings for reading filename
	char line[StringUtils::s_maxCharsPerLine];
	char fontFilePath[StringUtils::s_maxCharsPerLine];
	char textureName[StringUtils::s_maxCharsPerLine];
	char texturePath[StringUtils::s_maxCharsPerLine];
	memset(&line, 0, sizeof(char) * StringUtils::s_maxCharsPerLine);
	memset(&fontFilePath, 0, sizeof(char) * StringUtils::s_maxCharsPerLine);
	memset(&textureName, 0, sizeof(char) * StringUtils::s_maxCharsPerLine);
	memset(&texturePath, 0, sizeof(char) * StringUtils::s_maxCharsPerLine);
	sprintf(fontFilePath, "%s%s", m_fontPath, a_fontName);

	// File stream for font config file
	ifstream file(fontFilePath);
	unsigned int lineCount = 0;
	
	// Create a new font to be managed
	FontListNode * newFontNode = new FontListNode();
	newFontNode->SetData(new Font());
	Font * newFont = newFontNode->GetData();

	// Open the file and parse each line 
	if (file.is_open())
	{
		// Font metadata
		unsigned int numChars = 0;
		unsigned int sizeW = 0;
		unsigned int sizeH = 0;
		unsigned int lineHeight, base, pages;

		// Read till the file has more contents or a rule is broken
		while (file.good())
		{
			// Get the number of chars to parse
			file.getline(line, StringUtils::s_maxCharsPerLine);			// info face="fontname" etc
		    char * fontName = strstr(line, "\"") + 1;
			char shortFontName[StringUtils::s_maxCharsPerLine];
			sprintf(shortFontName, "%s", StringUtils::TrimString(StringUtils::ExtractField(line, "\"", 1), true));
			newFont->m_fontName.SetCString(shortFontName);

			file.getline(line, StringUtils::s_maxCharsPerLine);			// common lineHeight=x base=33			
			sscanf_s(line, "common lineHeight=%d base=%d scaleW=%d scaleH=%d pages=%d",
								   &lineHeight,  &base,  &sizeW,   &sizeH,	 &pages);

			file.getline(line, StringUtils::s_maxCharsPerLine);			// page id=0 file="arial.tga"
			sprintf(textureName, "%s", StringUtils::TrimString(StringUtils::ExtractField(line, "=", 2), true));

			file.getline(line, StringUtils::s_maxCharsPerLine);			// chars count=x
			sscanf_s(line, "chars count=%d", &numChars);
			
			// Load texture for font
			newFont->m_texture = new Texture();
			sprintf(texturePath, "%s%s", m_fontPath, textureName);
			newFont->m_texture->Load(texturePath);
			newFont->m_numChars = numChars;

			// Go through each char in the file loading metadata
			for (unsigned int i = 0; i < numChars; ++i)
			{
				file.getline(line, StringUtils::s_maxCharsPerLine);
				int charId, x, y, width, height, xoffset, yoffset, xadvance, page, chnl;
				sscanf_s(line, "char id=%d   x=%d    y=%d    width=%d     height=%d     xoffset=%d     yoffset=%d    xadvance=%d     page=%d  chnl=%d", 
								&charId,	 &x,	 &y,	 &width,	  &height,		&xoffset,	   &yoffset,	 &xadvance,		 &page,   &chnl);
				newFont->m_chars[charId].m_x = (float)x;
				newFont->m_chars[charId].m_y = (float)y;
				newFont->m_chars[charId].m_width = (float)width;
				newFont->m_chars[charId].m_height = (float)height;
				newFont->m_chars[charId].m_xoffset = (float)xoffset;
				newFont->m_chars[charId].m_yoffset = (float)yoffset;
				newFont->m_chars[charId].m_xadvance = (float)xadvance;
			}

			// There is more info such as kerning here but we don't care about it
			break;
		}

		file.close();

		m_fonts.Insert(newFontNode);

		return true;
	}
	else
	{
		Log::Get().Write(Log::LL_ERROR, Log::LC_CORE, "Could not find font file %s ", fontFilePath);

		// Clean up the font that was allocated
		delete newFontNode->GetData();
		delete newFontNode;

		return false;
	}
	
	return false;
}

bool FontManager::DrawString(const char * a_string, const char * a_fontName, float a_size, Vector2 a_pos, Colour a_colour)
{
	FontListNode * curFont = m_fonts.GetHead();
	while(curFont != NULL)
	{
		if (curFont->GetData()->m_fontName == StringHash(a_fontName))
		{
			// Draw each character in the string
			Font * font = curFont->GetData();
			RenderManager & renderMan = RenderManager::Get();
			float xAdvance = 0.0f;

			unsigned int textLength = strlen(a_string);
			for (unsigned int j = 0; j < textLength; ++j)
			{
				// Non space character
				if (a_string[j] != ' ') 
				{
					int charLetter = a_string[j];
					const FontChar & curChar = font->m_chars[charLetter];
					
					// Change the 256 to the font dims from the file
					float fontDimX = 256.0f;
					float fontDimY = 256.0f;

					TexCoord texSize(curChar.m_width/fontDimX, curChar.m_height/fontDimY);
					Vector2 charSize((curChar.m_width + curChar.m_xoffset)/fontDimX, (curChar.m_height + curChar.m_yoffset)/fontDimY);
					TexCoord texCoord(curChar.m_x/fontDimX, curChar.m_y/fontDimY);

					renderMan.AddQuad2D(RenderManager::eBatchGui, 
										Vector2(a_pos.GetX() + xAdvance, a_pos.GetY()), 
										charSize, 
										font->m_texture,
										texCoord,
										texSize);
					
					xAdvance += (float)curChar.m_xadvance / fontDimX;
				}
				else // Just move the x advance along for a space
				{
					xAdvance += 15.0f; //wtf here...
				}
			}
		
			return true;
		}
		curFont = curFont->GetNext();
	}

	// Could not find the font to draw with
	return false;
}


bool FontManager::DrawDebugString(const char * a_string, Vector2 a_pos, Colour a_colour)
{
	// Use the first loaded font as the debug font
	if (m_fonts.GetLength() > 0)
	{
		return DrawString(a_string, m_fonts.GetHead()->GetData()->m_fontName.GetCString(), 0.01f, a_pos, a_colour);
	}

	return false;
}