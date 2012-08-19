#include "Log.h"

#include "GameFile.h"

using namespace std;	//< For fstream operations

bool GameFile::Load(const char * a_filePath)
{
	char line[StringUtils::s_maxCharsPerLine];
	memset(&line, 0, sizeof(char) * StringUtils::s_maxCharsPerLine);
	Object * currentObject = NULL;
	ifstream file(a_filePath);
	unsigned int lineCount = 0;
	
	// Open the file and parse each line 
	if (file.is_open())
	{
		// Read till the file has more contents or a rule is broken
		while (file.good())
		{
			file.getline(line, StringUtils::s_maxCharsPerLine);
			lineCount++;
			
			// Parse any comment lines
			if (strstr(line, "//"))
			{
				continue;
			}

			// Parse any empty lines
			if (strlen(line) <= 0)
			{
				continue;
			}

			// A line without any symbols means a new object
			if (IsLineNewObject(line))
			{
				lineCount += ReadObjectAndProperties(line, file);
			}
			else // Bad formatting
			{
				Log::Get().Write(Log::LL_ERROR, Log::LC_ENGINE, "Bad game file format, expecting an object declaration at line %d.", lineCount);
			}
		}

		file.close();
		return true;
	}
	else
	{
		Log::Get().Write(Log::LL_ERROR, Log::LC_ENGINE, "Could not open game file resource at path %s", a_filePath);
		return false;
	}
}

unsigned int GameFile::ReadObjectAndProperties(const char * a_objectName, ifstream & a_stream, Object * a_parent)
{
	char line[StringUtils::s_maxCharsPerLine];
	memset(&line, 0, sizeof(char) * StringUtils::s_maxCharsPerLine);
	unsigned int lineCount = 0;

	Object * currentObject = AddObject(StringUtils::TrimString(a_objectName));
	a_stream.getline(line, StringUtils::s_maxCharsPerLine);
	lineCount++;

	// Parse any comment line
	if (strstr(line, "//"))
	{
		a_stream.getline(line, StringUtils::s_maxCharsPerLine);
		lineCount++;;
	}

	// Next line should be a brace
	unsigned int braceCount = 0;
	if (strstr(line, "{"))
	{
		braceCount++;
		a_stream.getline(line, StringUtils::s_maxCharsPerLine);
		lineCount++;

		// Parse any comment line
		if (strstr(line, "//"))
		{
			a_stream.getline(line, StringUtils::s_maxCharsPerLine);
			lineCount++;
		}

		// Now properties of that object
		while(!strstr(line, "}"))
		{ 
			// Look for child object
			if (IsLineNewObject(line))
			{
				// Read the child object
				lineCount += ReadObjectAndProperties(line, a_stream, currentObject);
				a_stream.getline(line, StringUtils::s_maxCharsPerLine);
				lineCount++;
			}
			else // Otherwise normal property
			{
				AddProperty(currentObject, StringUtils::ExtractPropertyName(line, ":"), StringUtils::ExtractValue(line, ":"));
				a_stream.getline(line, StringUtils::s_maxCharsPerLine);
				lineCount++;
			}
		}
		braceCount--;
	}
	else if (braceCount > 0) // Mismatched number of braces
	{
		Log::Get().Write(Log::LL_ERROR, Log::LC_ENGINE, "Bad game file format, expecting an open brace after object declaration on line %u.", lineCount);
	}

	return lineCount;
}

void GameFile::Reset()
{
	// Iterate through all objects and delete inclusive of properties
	LinkedListNode<Object> * nextObject = m_objects.GetHead();
	while(nextObject != NULL)
	{
		LinkedListNode<Property> * nextProperty = nextObject->GetData()->m_properties.GetHead();	
		while(nextProperty != NULL)
		{
			// Cache off the working node so we can delete it and it's data
			LinkedListNode<Property> * curProperty = nextProperty;
			nextProperty = nextProperty->GetNext();

			nextObject->GetData()->m_properties.Remove(curProperty);
			delete curProperty->GetData();
			delete curProperty;
		}

		// Cache off working node as above
		LinkedListNode<Object> * curObject = nextObject;
		nextObject = nextObject->GetNext();

		m_objects.Remove(curObject);
		delete curObject->GetData();
		delete curObject;	
	}	
}

const char * GameFile::GetString(const char * a_object, const char * a_property)
{
	// First find the object
	if (Object * parentObject = GetObject(a_object))
	{
		// Then the property
		if (Property * prop = GetProperty(parentObject, a_property))
		{
			return (const char *)prop->m_data;
		}
	}
	return NULL;
}

int GameFile::GetInt(const char * a_object, const char * a_property)
{
	// First find the object
	if (Object * parentObject = GetObject(a_object))
	{
		// Then the property
		if (Property * prop = GetProperty(parentObject, a_property))
		{
			return atoi((const char *)prop->m_data);
		}
	}
	return -1;
}

float GameFile::GetFloat(const char * a_object, const char * a_property)
{
	return 0.0f;
}

bool GameFile::GetBool(const char * a_object, const char * a_property)
{
	return false;
}

GameFile::Object * GameFile::AddObject(const char * a_objectName, Object * a_parent)
{
	LinkedListNode<Object> * newObject = new LinkedListNode<Object>();
	newObject->SetData(new Object());

	// Set name
	ALLOC_CSTRING_COPY(newObject->GetData()->m_name, a_objectName);

	// Set parent
	if (a_parent != NULL)
	{
		newObject->GetData()->m_parent = a_parent;
	}
	
	m_objects.Insert(newObject);

	return newObject->GetData();
}

GameFile::Property * GameFile::AddProperty(GameFile::Object * a_parentObject, const char * a_propertyName, const char * a_value)
{
	LinkedListNode<Property> * newProperty = new LinkedListNode<Property>();
	newProperty->SetData(new Property());
	ALLOC_CSTRING_COPY(newProperty->GetData()->m_name, a_propertyName);
	ALLOC_CSTRING_COPY(newProperty->GetData()->m_data, a_value);

	a_parentObject->m_properties.Insert(newProperty);

	return newProperty->GetData();
}

GameFile::Object * GameFile::GetObject(const char * a_name)
{
	// Iterate through all objects in this file looking for a name match
	LinkedListNode<Object> * cur = m_objects.GetHead();
	while(cur != NULL)
	{
		// Quit out of the loop if found
		if (_stricmp(cur->GetData()->m_name, a_name) == 0)
		{
			return cur->GetData();
		}
		cur = cur->GetNext();
	}

	return NULL;
}

GameFile::Property * GameFile::GetProperty(Object * a_parent, const char * a_propertyName)
{
	// Iterate through all objects in the parent's property list till a name match is found
	LinkedListNode<Property> * cur = a_parent->m_properties.GetHead();
	while(cur != NULL)
	{
		// Quit out of the loop if found
		if (_stricmp(cur->GetData()->m_name, a_propertyName) == 0)
		{
			return cur->GetData();
		}
		cur = cur->GetNext();
	}

	// Not found
	return NULL;
}
