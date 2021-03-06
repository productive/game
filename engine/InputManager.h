#ifndef _ENGINE_INPUT_MANAGER_
#define _ENGINE_INPUT_MANAGER_
#pragma once

#include <SDL.h>

#include "../core/Delegate.h"
#include "../core/LinkedList.h"
#include "../core/Vector.h"

#include "DebugMenu.h"
#include "Singleton.h"

class InputManager : public Singleton<InputManager>
{

public:

	//\brief Input manager defines its own constants for input events in case there are input
	//		 requirements outside of SDL
	enum eInputType
	{
		eInputTypeNone = -1,
		eInputTypeKeyDown,				// A keyboard key was depressed
		eInputTypeKeyUp,				// A keyboard key was released
		eInputTypeMouseDown,			// Mouse button depressed
		eInputTypeMouseUp,				// Mouse button released
		eInputTypeMouseMotion,			// Mouse moved to some coord

		eInputTypeCount,
	};

	//\brief Easy to use mouse button constants, independant from SDLs numbered buttons
	enum eMouseButton
	{
		eMouseButtonNone = -1,
		eMouseButtonLeft,
		eMouseButtonRight,
		eMouseButtonMiddle,

		eMouseButtonCount,
	};

	InputManager() 
		: m_focus(true)
		, m_fullScreen(false)
		, m_mousePos(0.0f)
		, m_lastKeyPress(SDLK_CLEAR)
		, m_lastKeyRelease(SDLK_CLEAR) 
	{
		// Init the list of depressed keys
		memset(&m_depressedKeys[0], SDLK_UNKNOWN, sizeof(SDLKey) * s_maxDepressedKeys);
	}

	~InputManager() { Shutdown(); }

	//\brief Startup will set the input manager with what state the app window is in
	//\param a_fullScreen bool if the app window is being shown fullscreen
    bool Startup(bool a_fullScreen = false);

	//\brief Shutdown will clean up memory for input event list
	bool Shutdown();
	bool Update(const SDL_Event & a_event);

	//\brief Enable or disable the app for OS focus. This will toggle rendering and input grab
	//\param The new app focus setting
	void SetFocus(bool a_focus);

	//\brief Access for the mouse coords for any part in the engine
	inline Vector2 GetMousePosAbsolute() const { return m_mousePos; }
	Vector2 GetMousePosRelative();

	//\brief Utility function to get the last key pressed or released
	//\param a_keyPress if the last key to be pressed or released is required, optional
	inline SDLKey GetLastKey(bool a_keyPress = true) { return a_keyPress ? m_lastKeyPress : m_lastKeyRelease; }

	//\brief Utility function to find if an arbitrary key is pressed or release
	//\param a_keyVal the SDL key value of the key to check
	//\return bool true if the button is currently down
	bool IsKeyDepressed(SDLKey a_key);

	//\brief Register a function pointer to be called when the app receives a mouse event
	//\param a_callback is the object that contains the member function to call back
	//\param a_button is the button that is being depressed or released
	//\param a_type defaults to mouse up but can be changed to to down or motion
	//\param a_oneShot bool defines if the event should be deleted after the callback function is called
	template <typename TObj, typename TMethod>
	void RegisterMouseCallback(TObj * a_callerObject, TMethod a_callback, eMouseButton a_button, eInputType a_type = eInputTypeMouseUp, bool a_oneShot = false)
	{
		// Add an event to the list of items to be processed
		// TODO memory management! Kill std new with a rusty fork
		InputEventNode * newInputNode = new InputEventNode();
		newInputNode->SetData(new InputEvent());
	
		// Set data for the new event
		InputEvent * newInput = newInputNode->GetData();
		newInput->m_src.m_mouseButton = a_button;
		newInput->m_type = a_type;
		newInput->m_oneShot = a_oneShot;
		newInput->m_delegate.SetCallback(a_callerObject, a_callback);
		m_events.Insert(newInputNode);
	}

	//\brief Register a function pointer to be called when the app receives a keyboard event
	//\param a_callback is the object that contains the member function to call back
	//\param a_key is the keyboard key that is being depressed or released
	//\param a_type defaults to mouse up but can be changed to to down or motion
	//\param a_oneShot bool defines if the event should be deleted after the callback function is called
	template <typename TObj, typename TMethod>
	void RegisterKeyCallback(TObj * a_callerObject, TMethod a_callback, SDLKey a_key, eInputType a_type = eInputTypeKeyDown, bool a_oneShot = false)
	{
		// Add an event to the list of items to be processed
		InputEventNode * newInputNode = new InputEventNode();
		newInputNode->SetData(new InputEvent());
	
		// Set data for the new event
		InputEvent * newInput = newInputNode->GetData();
		newInput->m_src.m_key = a_key;
		newInput->m_type = a_type;
		newInput->m_oneShot = a_oneShot;
		newInput->m_delegate.SetCallback(a_callerObject, a_callback);
		m_events.Insert(newInputNode);
	}

	//\brief Register a function pointer to be called when the app receives a keyboard for any alphanumeric key
	//\param a_callback is the object that contains the member function to call back
	//\param a_oneShot bool defines if the event should be deleted after the callback function is called
	template <typename TObj, typename TMethod>
	void RegisterAlphaKeyCallback(TObj * a_callerObject, TMethod a_callback, eInputType a_type = eInputTypeKeyDown, bool a_oneShot = false)
	{
		// Set data for the new global event
		m_alphaKeys.m_src.m_key = SDLK_CLEAR;
		m_alphaKeys.m_type = a_type;
		m_alphaKeys.m_oneShot = a_oneShot;
		m_alphaKeys.m_delegate.SetCallback(a_callerObject, a_callback);
	}

private:

	//\brief An input event can come from a number of sources but only one at once hence the union
	union InputSource
	{
		SDLKey m_key;				///< A keyboard button
		eMouseButton m_mouseButton;	///< A mouse click
	};

	//\brief Storage for an input event and it's callback
	struct InputEvent
	{
		InputEvent() 
			: m_src(InputSource())
			, m_delegate()
			, m_type(eInputTypeNone)
			, m_oneShot(false) {}

		InputSource m_src;					///< What event happened
		Delegate<bool, bool> m_delegate;	///< Pointer to object to call when it happens
		eInputType	m_type;					///< What type of event to respond to
		bool		m_oneShot;				///< If the event should only be responded to once
	};

	//\brief Input handling helper functions are split up so there is no one huge
	//		 switch statement going on. All these just process SDL input
    bool ProcessKeyDown(SDLKey a_key);
    bool ProcessKeyUp(SDLKey a_key);
	bool ProcessMouseDown(eMouseButton a_button);
	bool ProcessMouseUp(eMouseButton a_button);
    bool ProcessMouseMove();

	//\brief Alias to store a list of registered input events
	typedef LinkedListNode<InputEvent> InputEventNode;
	typedef LinkedList<InputEvent> InputEventList;

	//\brief Helper function to iterate list of events and find first matching event
	//\param a_type the type of event that is being searched for
	//\param a_src the matching input key or button to search for
	//\return a pointer to an input event or NULL if no event found
	InputEvent * GetFirstEvent(eInputType a_type, InputSource a_src);

	//\brief Helper function to iterate list of events and find all matching events
	//\param a_type the type of event that is being searched for
	//\param a_src the matching input key or button to search for
	//\param a_events_OUT is a ref to an event list to be populated by this function
	//\return true if at least one event was found
	bool GetEvents(eInputType a_type, InputSource a_src, InputEventList & a_events_OUT);

	static const unsigned int s_maxDepressedKeys = 8;	///< How many keys can be held on the keyboard at once

	InputEvent m_alphaKeys;		///< Special input event to catch all keys being pressed
	InputEventList m_events;	///< List of events to match up to actions
	bool m_focus;				///< If the app currently has OS focus
	bool m_fullScreen;			///< If the app is fullscreen, input manager needs to handle focus
	Vector2 m_mousePos;			///< Cache of mouse coords for convenience
	SDLKey m_lastKeyPress;		///< Cache off last key for convenience
	SDLKey m_lastKeyRelease;	///< Cache off last key for convenience
	SDLKey m_depressedKeys[s_maxDepressedKeys];		///< List of all the keys that are depressed 
};

#endif // _ENGINE_INPUT_MANAGER_
