#ifndef _ENGINE_GAME_OBJECT_
#define _ENGINE_GAME_OBJECT_
#pragma once

#include "../core/Matrix.h"

#include "StringUtils.h"

class GameObjectComponent;
class Model;

//\brief A gameobject is the container for all entities involved in the gameplay.
//		 It is lightweight yet has provisions for all basic game related functions like
//		 2D sprites, 3D models, scripts, events and collision.
class GameObject
{

public:

	//\brief GameObject state determines how the update effects related subsystems
	enum eGameObjectState
	{
		eGameObjectState_New = 0,	// Object is created but not ready for life
		eGameObjectState_Loading,	// Loading resources, scripts, models etc
		eGameObjectState_Active,	// Out and about in the world
		eGameObjectState_Sleep,		// Hibernation, no updates or rendering, can come back from sleep
		eGameObjectState_Death,		// Unloading and cleaning up before destruction, no coming back from death

		eGameObjectState_Count,
	};

	//\brief Creation and destruction
	GameObject(unsigned int a_id) 
		: m_id(a_id) 
		, m_child(NULL)
		, m_next(NULL)
		, m_components(NULL)
		, m_model(NULL)
		, m_state(eGameObjectState_New)
		, m_lifeTime(0.0f)
		{ }

	~GameObject() { Destroy(); }

	//\brief Functionality inherited by children
	virtual bool Update(float a_dt);
	virtual bool Draw();

	//\brief State mutators and accessors
	inline void SetSleeping() { if (m_state == eGameObjectState_Active) m_state = eGameObjectState_Sleep; }
	inline void SetActive()	  { if (m_state == eGameObjectState_Sleep) m_state = eGameObjectState_Active; }
	inline bool IsActive()	  { return m_state == eGameObjectState_Active; }
	inline bool IsSleeping()  { return m_state == eGameObjectState_Sleep; }
	inline unsigned int GetId() { return m_id; }

	//\brief Child object accessors
	inline GameObject * GetChild() { return m_child; }
	inline GameObject * GetNext() { return m_next; }

	//\brief Resource mutators and accessors
	inline void SetModel(Model * a_newModel) { m_model = a_newModel; }
	//inline void SetScript(Script * a_newScript) { m_script = a_newScript; }

private:

	//\brief Destruction is private as it should only be handled by object management
	void Destroy();

	Matrix				  m_worldMat;		///< Position and orientation in the world
	unsigned int		  m_id;				///< Unique identifier, objects can be resolved from ids
	GameObject *		  m_child;			///< Pointer to first child game obhject
	GameObject *		  m_next;			///< Pointer to sibling game objects
	GameObjectComponent * m_components;		///< Purpose built object features live in a list of components
	Model *				  m_model;			///< Pointer to a mesh for display purposes
	//Script			  m_script;			///< The LUA script for user defined behavior
	eGameObjectState	  m_state;			///< What state the object is in
	char				  m_name[StringUtils::s_maxCharsPerName];	///< Every creature needs a name
	float				  m_lifeTime;		///< How long this guy has been active

};

#endif // _ENGINE_GAME_OBJECT_
