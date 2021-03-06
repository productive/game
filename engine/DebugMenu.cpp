#include "../core/Colour.h"

#include "CameraManager.h"
#include "FontManager.h"
#include "InputManager.h"
#include "Log.h"
#include "ModelManager.h"
#include "RenderManager.h"
#include "TextureManager.h"
#include "Widget.h"
#include "WorldManager.h"

#include "DebugMenu.h"

template<> DebugMenu * Singleton<DebugMenu>::s_instance = NULL;

// Cursor definitions
const float DebugMenu::sc_cursorSize = 0.075f;
Vector2 DebugMenu::sc_vectorCursor[4] = 
{ 
	Vector2(0.0f, 0.0f),
	Vector2(sc_cursorSize, -sc_cursorSize),
	Vector2(sc_cursorSize*0.3f, -sc_cursorSize*0.7f),
	Vector2(0.0f, -sc_cursorSize)
};

DebugMenu::DebugMenu()
: m_enabled(false) 
, m_handledCommand(false)
, m_dirtyFlags()
, m_lastMousePosRelative(0.0f)
, m_editType(eEditTypeNone)
, m_editMode(eEditModeNone)
, m_widgetToEdit(NULL)
, m_gameObjectToEdit(NULL)
, m_btnCreateRoot(NULL)
, m_btnCreateWidget(NULL)
, m_btnCreateGameObject(NULL)
, m_btnCreateGameObjectFromTemplate(NULL)
, m_btnCreateGameObjectNew(NULL)
, m_btnChangeGUIRoot(NULL)
, m_btnChangeGUIPos(NULL)
, m_btnChangeGUIShape(NULL)
, m_btnChangeGUIName(NULL)
, m_btnChangeGUITexture(NULL)
, m_btnDeleteGUI(NULL)
, m_btnChangeObjectRoot(NULL)
, m_btnChangeObjectModel(NULL)
, m_btnChangeObjectName(NULL)
, m_btnSaveObjectTemplate(NULL)
, m_btnDeleteObject(NULL)
, m_resourceSelect(NULL)
, m_resourceSelectList(NULL)
, m_btnResourceSelectOk(NULL)
, m_btnResourceSelectCancel(NULL)
, m_textInput(NULL)
, m_textInputField(NULL)
, m_btnTextInputOk(NULL)
, m_btnTextInputCancel(NULL)
{
	if (!Startup())
	{
		Log::Get().Write(Log::LL_ERROR, Log::LC_ENGINE, "DebugMenu failed to startup correctly!");
	}
}

bool DebugMenu::Startup()
{
	Gui & gui = Gui::Get();
	InputManager & inMan = InputManager::Get();

	// Create the root of the create menu buttons and use it as the parent to each button
	m_btnCreateRoot = CreateButton("Create!", sc_colourRed, gui.GetDebugRoot());
	m_btnCreateWidget = CreateButton("Widget", sc_colourPurple, m_btnCreateRoot);
	m_btnCreateGameObject = CreateButton("Game Object", sc_colourGreen, m_btnCreateRoot);
	m_btnCreateGameObjectFromTemplate = CreateButton("From Template", sc_colourOrange, m_btnCreateRoot);
	m_btnCreateGameObjectNew = CreateButton("New Object", sc_colourSkyBlue, m_btnCreateRoot);

	// Change 2D objects
	m_btnChangeGUIRoot = CreateButton("Change GUI", sc_colourRed, gui.GetDebugRoot());
	m_btnChangeGUIPos = CreateButton("Position", sc_colourPurple, m_btnChangeGUIRoot);
	m_btnChangeGUIShape = CreateButton("Shape", sc_colourBlue, m_btnChangeGUIRoot);
	m_btnChangeGUIName = CreateButton("Name", sc_colourOrange, m_btnChangeGUIRoot);
	m_btnChangeGUITexture = CreateButton("Texture", sc_colourYellow, m_btnChangeGUIRoot);
	m_btnDeleteGUI = CreateButton("Delete", sc_colourGrey, m_btnChangeGUIRoot);

	// Change 3D objects
	m_btnChangeObjectRoot = CreateButton("Change Object", sc_colourRed, gui.GetDebugRoot());
	m_btnChangeObjectModel = CreateButton("Model", sc_colourGreen, m_btnChangeObjectRoot);
	m_btnChangeObjectName = CreateButton("Name", sc_colourOrange, m_btnChangeObjectRoot);
	m_btnSaveObjectTemplate = CreateButton("Template", sc_colourPurple, m_btnChangeObjectRoot);
	m_btnDeleteObject = CreateButton("Delete", sc_colourGrey, m_btnChangeObjectRoot);

	// Create the resource selection dialog
	Widget::WidgetDef curItem;
	curItem.m_size = WidgetVector(0.95f, 1.5f);
	curItem.m_pos = WidgetVector(10.0f, 10.0f);

	// Check for a loaded debug font
	if (StringHash * debugFont = FontManager::Get().GetDebugFontName())
	{
		curItem.m_fontNameHash = debugFont->GetHash();
	}

	curItem.m_selectFlags = Widget::eSelectionNone;
	curItem.m_colour = sc_colourBlue;
	curItem.m_name = "Resource Select";
	m_resourceSelect = gui.CreateWidget(curItem, gui.GetDebugRoot(), false);
	m_resourceSelect->SetDebugWidget();

	// Create list box for resources
	curItem.m_size = WidgetVector(0.85f, 1.2f);
	curItem.m_selectFlags = Widget::eSelectionRollover;
	curItem.m_colour = sc_colourPurple;
	curItem.m_name="Resource List";
	m_resourceSelectList = gui.CreateWidget(curItem, m_resourceSelect, false);
	m_resourceSelectList->SetDebugWidget();
	m_resourceSelectList->SetAction(this, &DebugMenu::OnMenuItemMouseUp);

	// Ok and Cancel buttons on the resource select dialog
	m_btnResourceSelectOk = CreateButton("Ok", sc_colourOrange, m_resourceSelect);
	m_btnResourceSelectCancel = CreateButton("Cancel", sc_colourGrey, m_resourceSelect);

	// Create text input box for naming objects and files
	curItem.m_size = WidgetVector(0.85f, 0.4f);
	curItem.m_colour = sc_colourBlue;
	curItem.m_name = "Text Entry";
	m_textInput = gui.CreateWidget(curItem, gui.GetDebugRoot(), false);
	m_textInput->SetDebugWidget();

	curItem.m_size = WidgetVector(0.8f, 0.15f);
	curItem.m_colour = sc_colourGrey;
	curItem.m_name = "Enter Name";
	m_textInputField = gui.CreateWidget(curItem, m_textInput, false);
	m_textInputField->SetDebugWidget();
	m_textInputField->SetShowFilePath();

	// Ok and Cancel buttons on the text input dialog
	m_btnTextInputOk = CreateButton("Ok", sc_colourOrange, m_textInput);
	m_btnTextInputCancel = CreateButton("Cancel", sc_colourGrey, m_textInput);
	
	// Register global key and mnouse listeners - note these will be processed after the button callbacks
	inMan.RegisterKeyCallback(this, &DebugMenu::OnEnable, SDLK_TAB);
	inMan.RegisterAlphaKeyCallback(this, &DebugMenu::OnAlphaKey, InputManager::eInputTypeKeyDown); 
	inMan.RegisterMouseCallback(this, &DebugMenu::OnActivate, InputManager::eMouseButtonRight);
	inMan.RegisterMouseCallback(this, &DebugMenu::OnSelect, InputManager::eMouseButtonLeft);

	// Process vector cursor vertices for display aspect
	for (unsigned int i = 0; i < 4; ++i)
	{
		sc_vectorCursor[i].SetY(sc_vectorCursor[i].GetY() * RenderManager::Get().GetViewAspect());
	}

	return true;
}

void DebugMenu::Update(float a_dt)
{
	// Handle editing actions tied to mouse move
	InputManager & inMan = InputManager::Get();
	if (m_editType == eEditTypeWidget && m_widgetToEdit != NULL)
	{
		Vector2 mousePos = inMan.GetMousePosRelative();
		switch (m_editMode)
		{
			case eEditModePos:
			{	
				m_widgetToEdit->SetPos(mousePos);
				m_dirtyFlags.Set(eDirtyFlagGUI);
				break;
			}
			case eEditModeShape:	
			{
				m_widgetToEdit->SetSize(Vector2(mousePos.GetX() - m_widgetToEdit->GetPos().GetX(),
												m_widgetToEdit->GetPos().GetY() - mousePos.GetY()));
				m_dirtyFlags.Set(eDirtyFlagGUI);
				break;
			}
			default: break;
		}
	}
	else if (m_gameObjectToEdit != NULL)
	{
		// Don't move while a menu is up
		if (!IsDebugMenuActive())
		{
			// Move object in all dimensions separately
			Vector curPos = m_gameObjectToEdit->GetPos();
			Vector2 amountToMove = (inMan.GetMousePosRelative() - m_lastMousePosRelative);
			if (inMan.IsKeyDepressed(SDLK_x))
			{
				m_gameObjectToEdit->SetPos(Vector(curPos.GetX() + amountToMove.GetX(), curPos.GetY(), curPos.GetZ()));
				m_dirtyFlags.Set(eDirtyFlagScene);
			} 
			else if (inMan.IsKeyDepressed(SDLK_y))
			{
				m_gameObjectToEdit->SetPos(Vector(curPos.GetX(), curPos.GetY() + amountToMove.GetY(), curPos.GetZ()));
				m_dirtyFlags.Set(eDirtyFlagScene);
			}
			else if (inMan.IsKeyDepressed(SDLK_z))
			{
				m_gameObjectToEdit->SetPos(Vector(curPos.GetX(), curPos.GetY(), curPos.GetZ() + amountToMove.GetY()));
				m_dirtyFlags.Set(eDirtyFlagScene);
			}
		}
	}

	// Draw all widgets with updated coords
	Draw();
	
	// Cache off the last mouse pos
	m_lastMousePosRelative = inMan.GetMousePosRelative();
}

bool DebugMenu::SaveChanges()
{
	// Save resources to disk if dirty
	bool changesSaved = false;
	for (unsigned int i = 0; i < eDirtyFlagCount; ++i)
	{
		eDirtyFlag curFlag = (eDirtyFlag)i;
		if (m_dirtyFlags.IsBitSet(curFlag))
		{
			// Handle each flag type
			switch (curFlag)
			{
				case eDirtyFlagGUI:		Gui::Get().GetActiveMenu()->Serialise();				changesSaved = true; break;
				case eDirtyFlagScene:	WorldManager::Get().GetCurrentScene()->Serialise();		changesSaved = true; break;
				default: break;
			}
			m_dirtyFlags.Clear(curFlag);
		}
	}

	return changesSaved;
}

bool DebugMenu::OnMenuItemMouseUp(Widget * a_widget)
{
	// Commands can be handled by the menu items here or in the key/button handlers
	m_handledCommand = false;

	// Do nothing if the debug menu isn't enabled
	if (!IsDebugMenuEnabled())
	{
		return false;
	}
	
	// Set visibility and position for the debug
	return HandleMenuAction(a_widget);
}

bool DebugMenu::HandleMenuAction(Widget * a_widget)
{
	// Check the widget that was activated matches and we don't have other menus up
	Gui & gui = Gui::Get();
	m_handledCommand = false;

	if (a_widget == m_btnCreateRoot)
	{
		// Show menu options on the right of the menu
		WidgetVector right = m_btnCreateRoot->GetPos() + WidgetVector(m_btnCreateRoot->GetSize().GetX(), 0.0f);
		WidgetVector height = m_btnCreateRoot->GetSize();
		height.SetX(0.0f);
		m_btnCreateWidget->SetPos(right);
		m_btnCreateGameObject->SetPos(right - height);

		height.SetY(height.GetY() - m_btnCreateRoot->GetSize().GetY() * 2.0f);
		m_btnCreateGameObject->SetPos(right + height);

		ShowCreateMenu(true);
		m_handledCommand = true;
	}
	else if (a_widget == m_btnCreateWidget)
	{
		// Make a new widget
		Widget::WidgetDef curItem;
		curItem.m_colour = sc_colourWhite;
		curItem.m_size = WidgetVector(0.35f, 0.35f);
	
		// Check for a loaded debug font
		if (StringHash * debugFont = FontManager::Get().GetDebugFontName())
		{
			curItem.m_fontNameHash = debugFont->GetHash();
		}

		curItem.m_selectFlags = Widget::eSelectionRollover;
		curItem.m_name = "NEW_WIDGET";

		// Parent is the active menu
		Widget * parentWidget = m_widgetToEdit != NULL ? m_widgetToEdit : gui.GetActiveMenu();
		Widget * newWidget = Gui::Get().CreateWidget(curItem, parentWidget);
		newWidget->SetPos(m_btnCreateRoot->GetPos());
		m_dirtyFlags.Set(eDirtyFlagGUI);
		
		// Cancel menu display
		ShowCreateMenu(false);
		m_handledCommand = true;
	}
	else if (a_widget == m_btnCreateGameObject)
	{
		// Position the create object submenu buttons
		WidgetVector right = m_btnCreateGameObject->GetPos() + WidgetVector(m_btnCreateGameObject->GetSize().GetX(), -m_btnCreateGameObject->GetSize().GetY());
		WidgetVector height = m_btnCreateGameObject->GetSize();
		height.SetX(0.0f);
		m_btnCreateGameObjectFromTemplate->SetPos(right);
		m_btnCreateGameObjectNew->SetPos(right + height);

		m_btnCreateGameObjectFromTemplate->SetActive(true);
		m_btnCreateGameObjectNew->SetActive(true);
		m_handledCommand = true;
	}
	else if (a_widget == m_btnCreateGameObjectFromTemplate)
	{
		m_editType = eEditTypeGameObject;
		m_editMode = eEditModeTemplate;
		ShowResourceSelect(WorldManager::Get().GetTemplatePath(), "tmp");

		ShowCreateMenu(false);
		m_handledCommand = true;
	}
	else if (a_widget == m_btnCreateGameObjectNew)
	{
		// Create a game object
		if (m_btnCreateGameObject->IsActive())
		{
			WorldManager::Get().CreateObject<GameObject>();
		}		
		ShowCreateMenu(false);
		m_handledCommand = true;
	}
	else if (a_widget == m_btnChangeGUIRoot)
	{
		// Show menu options on the right of the menu
		WidgetVector right = m_btnChangeGUIRoot->GetPos() + WidgetVector(m_btnChangeGUIRoot->GetSize().GetX(), -m_btnChangeGUIRoot->GetSize().GetY());
		WidgetVector height = m_btnChangeGUIRoot->GetSize();
		height.SetX(0.0f);
		m_btnChangeGUIPos->SetPos(right);
		m_btnChangeGUIShape->SetPos(right + height);

		height.SetY(height.GetY() - m_btnChangeGUIRoot->GetSize().GetY() * 2.0f);
		m_btnChangeGUIName->SetPos(right + height);

		height.SetY(height.GetY() - m_btnChangeGUIRoot->GetSize().GetY());
		m_btnChangeGUITexture->SetPos(right + height);

		height.SetY(height.GetY() - m_btnChangeGUIRoot->GetSize().GetY());
		m_btnDeleteGUI->SetPos(right + height);

		ShowChangeGUIMenu(true);
		m_handledCommand = true;
	}
	else if (a_widget == m_btnChangeGUIPos)
	{
		m_editMode = eEditModePos;
		ShowChangeGUIMenu(false);
		m_handledCommand = true;
	}
	else if (a_widget == m_btnChangeGUIShape)
	{
		m_editMode = eEditModeShape;
		ShowChangeGUIMenu(false);
		m_handledCommand = true;
	}
	else if (a_widget == m_btnChangeGUIName)
	{		
		m_editMode = eEditModeName;
		ShowTextInput(m_widgetToEdit->GetName());
		ShowChangeGUIMenu(false);
		m_handledCommand = true;
	}
	else if (a_widget == m_btnChangeGUITexture)
	{
		m_editMode = eEditModeTexture;
		ShowChangeGUIMenu(false);
		
		// Bring up the resource selection dialog
		ShowResourceSelect(TextureManager::Get().GetTexturePath(), "tga");
		m_handledCommand = true;
	}
	else if (a_widget == m_btnDeleteGUI)
	{
		Gui::Get().DestroyWidget(m_widgetToEdit);
		m_widgetToEdit = NULL;

		m_editMode = eEditModeNone;
		m_editType = eEditTypeNone;
		ShowChangeGUIMenu(false);
		m_handledCommand = true;
	}
	else if (a_widget == m_btnChangeObjectRoot)
	{
		// Show menu options on the right of the menu
		WidgetVector right = m_btnChangeObjectRoot->GetPos() + WidgetVector(m_btnChangeObjectRoot->GetSize().GetX(), -m_btnChangeObjectRoot->GetSize().GetY());
		WidgetVector height = m_btnChangeObjectRoot->GetSize();
		height.SetX(0.0f);
		m_btnChangeObjectName->SetPos(right);
		m_btnChangeObjectModel->SetPos(right + height);
		
		height.SetY(height.GetY() - m_btnChangeGUIRoot->GetSize().GetY() * 2.0f);
		m_btnSaveObjectTemplate->SetPos(right + height);

		height.SetY(height.GetY() - m_btnChangeGUIRoot->GetSize().GetY());
		m_btnDeleteObject->SetPos(right + height);

		ShowChangeObjectMenu(true);
		m_handledCommand = true;
	}
	else if (a_widget == m_btnChangeObjectName)
	{
		m_editMode = eEditModeName;
		ShowTextInput(m_gameObjectToEdit->GetName());
		ShowChangeObjectMenu(false);
		m_handledCommand = true;
	}
	else if (a_widget == m_btnChangeObjectModel)
	{
		m_editMode = eEditModeModel;
		ShowResourceSelect(ModelManager::Get().GetModelPath(), "obj");
		ShowChangeObjectMenu(false);
		m_handledCommand = true;
	}
	else if (a_widget == m_btnSaveObjectTemplate)
	{
		m_editMode = eEditModeSaveTemplate;
		ShowTextInput(m_gameObjectToEdit->GetTemplate());
		ShowChangeObjectMenu(false);
		m_handledCommand = true;
	}
	else if (a_widget == m_btnDeleteObject)
	{
		WorldManager::Get().DestroyObject(m_gameObjectToEdit->GetId());
		m_gameObjectToEdit = NULL;
		m_editMode = eEditModeNone;
		m_editType = eEditTypeNone;
		ShowChangeObjectMenu(false);
		m_handledCommand = true;
	}
	else if (a_widget == m_btnResourceSelectOk)
	{
		// Have just selected a resource for some object
		switch (m_editType)
		{
			case eEditTypeWidget: 
			{
				// Early out for no widget selected
				if (m_widgetToEdit == NULL) { break; }

				// Setting a texture on a widget
				if (m_editMode == eEditModeTexture)
				{
					char tgaBuf[StringUtils::s_maxCharsPerLine];
					sprintf(tgaBuf, "%s%s", TextureManager::Get().GetTexturePath(), m_resourceSelectList->GetSelectedListItem());
					m_widgetToEdit->SetTexture(TextureManager::Get().GetTexture(tgaBuf, TextureManager::eCategoryGui));
					m_dirtyFlags.Set(eDirtyFlagGUI);
				}
				break;
			}
			case eEditTypeGameObject:
			{
				WorldManager & worldMan = WorldManager::Get();
				if (m_editMode == eEditModeTemplate)
				{
					// Delete the old object
					if (m_gameObjectToEdit)
					{
						worldMan.DestroyObject(m_gameObjectToEdit->GetId());
					}
					worldMan.CreateObject<GameObject>(m_resourceSelectList->GetSelectedListItem());
					m_dirtyFlags.Set(eDirtyFlagScene);
				}

				// Early out for no object
				if (m_gameObjectToEdit == NULL) { break; }

				// Setting a model on a game object
				if (m_editMode == eEditModeModel)
				{
					char objBuf[StringUtils::s_maxCharsPerLine];
					sprintf(objBuf, "%s%s", ModelManager::Get().GetModelPath(), m_resourceSelectList->GetSelectedListItem());
					
					// Load the model and set it as the current model to edit
					if (Model * newModel = ModelManager::Get().GetModel(objBuf))
					{
						m_gameObjectToEdit->SetModel(newModel);
						m_dirtyFlags.Set(eDirtyFlagScene);
					}
				}
				break;
			}
			default: break;
		}

		HideResoureSelect();
		m_editType = eEditTypeNone;
		m_editMode = eEditModeNone;

		m_handledCommand = true;
	}
	else if (a_widget == m_btnResourceSelectCancel)
	{
		HideResoureSelect();
		m_editType = eEditTypeNone;
		m_editMode = eEditModeNone;

		m_handledCommand = true;
	}
	else if (a_widget == m_btnTextInputOk)
	{
		if (m_editType == eEditTypeWidget)
		{
			if (m_editMode == eEditModeName)
			{
				// Editing the name of a widget
				if (m_widgetToEdit != NULL)
				{
					m_widgetToEdit->SetName(m_textInputField->GetFilePath());
					m_dirtyFlags.Set(eDirtyFlagGUI);
				}
			}
		}
		else if (m_editType == eEditTypeGameObject)
		{
			// Editing the name of an object
			if (m_editMode == eEditModeName)
			{
				if (m_gameObjectToEdit != NULL)
				{
					m_gameObjectToEdit->SetName(m_textInputField->GetFilePath());
					m_dirtyFlags.Set(eDirtyFlagScene);
				}
			}
			else if (m_editMode == eEditModeSaveTemplate)
			{
				// Editing the template of an object
				if (m_gameObjectToEdit != NULL)
				{
					m_gameObjectToEdit->SetTemplate(m_textInputField->GetFilePath());
					m_dirtyFlags.Set(eDirtyFlagScene);
				}
			}
		}

		HideTextInput();
		m_editType = eEditTypeNone;
		m_editMode = eEditModeNone;
		m_handledCommand = true;
	}
	else if (a_widget == m_btnTextInputCancel)
	{
		HideTextInput();
		m_editType = eEditTypeNone;
		m_editMode = eEditModeNone;
		m_handledCommand = true;
	}

	// Save anything dirty to file
	SaveChanges();

	return m_handledCommand;
}

bool DebugMenu::OnActivate(bool a_active)
{
	// Do nothing if the debug menu isn't enabled
	if (!m_enabled)
	{
		return false;
	}

	// If there is both a widget and object selected then we are in an error state
	if (m_widgetToEdit != NULL && m_gameObjectToEdit != NULL)
	{
		m_gameObjectToEdit = NULL;
	}

	// Set the creation root element to visible if it isn't already
	InputManager & inMan = InputManager::Get();
	if (m_widgetToEdit != NULL)
	{
		if (!IsDebugMenuActive())
		{
			m_btnChangeGUIRoot->SetPos(inMan.GetMousePosRelative());
			m_btnChangeGUIRoot->SetActive(a_active);
		}
	}
	else if (m_gameObjectToEdit != NULL)
	{
		if (!IsDebugMenuActive())
		{
			// TODO Get object pos from world pos of object
			m_btnChangeObjectRoot->SetPos(Vector2(0.0f, 0.0f));
			m_btnChangeObjectRoot->SetActive(a_active);
		}
	}
	else if (!m_btnCreateRoot->IsActive())
	{
		m_btnCreateRoot->SetPos(inMan.GetMousePosRelative());
		m_btnCreateRoot->SetActive(a_active);
	}

	return true;
}

bool DebugMenu::OnSelect(bool a_active)
{
	// Do not respond to a click if it's been handled by a menu item
	if (m_handledCommand == true)
	{
		m_handledCommand = false;
		return true;
	}

	// Stop any mouse bound editing on click
	if (m_editMode == eEditModePos  || m_editMode == eEditModeShape)
	{
		m_editMode = eEditModeNone;

		// Changed a property, save the file
		m_dirtyFlags.Set(eDirtyFlagGUI);
	}

	// Don't play around with widget selection while a menu is up
	if (m_textInput->IsActive() || m_resourceSelect->IsActive())
	{
		return false;
	}

	// Cancel previous selection
	if (!IsDebugMenuActive() && m_editMode == eEditModeNone)
	{
		if (m_widgetToEdit != NULL)
		{
			m_editType = eEditTypeNone;
			m_editMode = eEditModeNone;
			m_widgetToEdit->ClearSelection();
			m_widgetToEdit = NULL;
		}
	}

	// Find the first widget that is rolled over in edit mode
	if (Widget * newSelectedWidget = Gui::Get().GetActiveWidget())
	{
		// Clear selection of old widget
		if (m_widgetToEdit != NULL && m_widgetToEdit != newSelectedWidget)
		{
			m_widgetToEdit->ClearSelection();
		}
		m_editType = eEditTypeWidget;
		m_widgetToEdit = newSelectedWidget;
		m_widgetToEdit->SetSelection(Widget::eSelectionEditSelected);
	}
	else // Cancel selections
	{
		if (m_widgetToEdit != NULL)
		{
			m_editType = eEditTypeNone;
			m_editMode = eEditModeNone;
			m_widgetToEdit->ClearSelection();
			m_widgetToEdit = NULL;
		}
	}

	// Do picking with all the game objects in the scene
	if (Scene * curScene = WorldManager::Get().GetCurrentScene())
	{
		// Picking point is the mouse cursor transformed to 3D space in cam direction
		const float pickDepth = 100.0f;
		const float persp = 0.47f;
		RenderManager & renMan = RenderManager::Get();
		CameraManager & camMan = CameraManager::Get();
		Vector2 mousePos = InputManager::Get().GetMousePosRelative();
		Matrix camMat = camMan.GetViewMatrix();
		Vector camPos = camMan.GetWorldPos();
		Vector mouseInput = Vector(	mousePos.GetX() * renMan.GetViewAspect() * pickDepth * persp, 
									0.0f, 
									mousePos.GetY() * pickDepth * persp);
		Vector pickEnd = camPos + camMat.GetLook() * pickDepth;
		pickEnd += camMat.Transform(mouseInput);

		// Pick an arbitrary object (would have to sort to get the closest)
		if (m_gameObjectToEdit = curScene->GetSceneObject(camPos, pickEnd))
		{
			m_editType = eEditTypeGameObject;
		}
	}

	// Cancel all menu display
	if (m_gameObjectToEdit == NULL && m_widgetToEdit == NULL)
	{
		ShowCreateMenu(false);
		ShowChangeGUIMenu(false);
		ShowChangeObjectMenu(false);
	}

	return false;
}

bool DebugMenu::OnEnable(bool a_toggle)
{
	m_enabled = !m_enabled;
	return m_enabled;
}

bool DebugMenu::OnAlphaKey(bool a_unused)
{
	// Only useful if typing in a text input box
	if (m_textInput->IsActive())
	{
		InputManager & inMan = InputManager::Get();
		char newName[StringUtils::s_maxCharsPerName];
		memset(newName, 0, StringUtils::s_maxCharsPerName);
		strncpy(newName, m_textInputField->GetFilePath(), strlen(m_textInputField->GetFilePath()));

		SDLKey lastKey = inMan.GetLastKey();
		if (lastKey == SDLK_BACKSPACE)
		{
			// Delete a character off the end of the name
			unsigned int nameLength = strlen(newName);
			if (nameLength > 0)
			{
				newName[nameLength - 1] = '\0';
				m_textInputField->SetFilePath(newName);
			}
		}
		else // Some other alpha key, append to the name
		{
			// TODO move this check to string utils and input manager
			int keyVal = (int)lastKey;
			const unsigned int alphaStart = 97; // ASCII for a key
			const unsigned int alphaEnd = 122;  // ASCII for z key
			const unsigned int numStart = 48;	// ASCII for 0
			const unsigned int numEnd = 57;		// ASCII for 9
			if ((keyVal >= alphaStart && keyVal <= alphaEnd) ||
				(keyVal >= numStart && keyVal <= numEnd) ||
				lastKey == SDLK_UNDERSCORE)
			{
				// Handle upper case letters when a shift is held
				if ((keyVal >= alphaStart && keyVal <= alphaEnd) && 
					(inMan.IsKeyDepressed(SDLK_LSHIFT) || inMan.IsKeyDepressed(SDLK_RSHIFT)))
				{
					keyVal -= 32;
				}

				sprintf(newName, "%s%c", m_textInputField->GetFilePath(), keyVal);
				m_textInputField->SetFilePath(newName);
			}
		}

		return true;
	}

	return false;
}

void DebugMenu::ShowResourceSelect(const char * a_startingPath, const char * a_fileExtensionFilter)
{
	// Position and display the elements of the dialog
	m_resourceSelect->SetActive();
	m_resourceSelectList->SetActive();
	m_btnResourceSelectOk->SetActive();
	m_btnResourceSelectCancel->SetActive();

	// Position buttons on the panel
	const float buttonSpacingX = 0.025f;
	const float buttonSpacingY = buttonSpacingX * RenderManager::Get().GetViewAspect();
	Vector2 parentSize = m_resourceSelect->GetSize();
	Vector2 parentPos = Vector2(-parentSize.GetX()*0.5f, 0.75f);
	m_resourceSelect->SetPos(parentPos);

	// Position the list of resources
	m_resourceSelectList->SetPos(Vector2(parentPos.GetX() + buttonSpacingX*2.0f, parentPos.GetY() - buttonSpacingY*2.0f));

	// Position the Ok and Cancel buttons
	Vector2 buttonSize = m_btnResourceSelectOk->GetSize();
	Vector2 buttonPos = Vector2(parentPos.GetX() + buttonSpacingX,
								parentPos.GetY() - parentSize.GetY() + buttonSize.GetY() + buttonSpacingY);
	m_btnResourceSelectOk->SetPos(buttonPos);

	buttonPos.SetX(parentPos.GetX() + parentSize.GetX() - buttonSize.GetX() - buttonSpacingX);
	m_btnResourceSelectCancel->SetPos(buttonPos);

	// Add resource list to widget
	m_resourceSelectList->ClearListItems();
	FileManager & fileMan = FileManager::Get();
	FileManager::FileList resourceFiles;
	FileManager::Get().FillFileList(a_startingPath, resourceFiles, a_fileExtensionFilter);

	// Add each resource file in the directory
	FileManager::FileListNode * curNode = resourceFiles.GetHead();
	while(curNode != NULL)
	{
		m_resourceSelectList->AddListItem(curNode->GetData()->m_name);

		curNode = curNode->GetNext();
	}

	// Clean up file list
	FileManager::Get().EmptyFileList(resourceFiles);
}

void DebugMenu::ShowTextInput(const char * a_startingText)
{
	// Position and display the elements of the dialog
	m_textInput->SetActive();
	m_textInputField->SetActive();
	m_btnTextInputOk->SetActive();
	m_btnTextInputCancel->SetActive();

	// Position buttons on the panel
	const float buttonSpacingX = 0.025f;
	const float buttonSpacingY = buttonSpacingX * RenderManager::Get().GetViewAspect();
	Vector2 parentSize = m_textInput->GetSize();
	Vector2 parentPos = Vector2(-parentSize.GetX()*0.5f, 0.75f);
	m_textInput->SetPos(parentPos);

	// Position the list of resources
	m_textInputField->SetPos(Vector2(parentPos.GetX() + buttonSpacingX*2.0f, parentPos.GetY() - buttonSpacingY*2.0f));

	// Position the Ok and Cancel buttons
	Vector2 buttonSize = m_btnTextInputOk->GetSize();
	Vector2 buttonPos = Vector2(parentPos.GetX() + buttonSpacingX,
								parentPos.GetY() - parentSize.GetY() + buttonSize.GetY() + buttonSpacingY);
	m_btnTextInputOk->SetPos(buttonPos);

	buttonPos.SetX(parentPos.GetX() + parentSize.GetX() - buttonSize.GetX() - buttonSpacingX);
	m_btnTextInputCancel->SetPos(buttonPos);

	// Show the starting text if reqd
	if (a_startingText != NULL)
	{
		m_textInputField->SetFilePath(a_startingText);
	}
}

void DebugMenu::Draw()
{
	// Draw nothing if the debug menu isn't enabled
	if (!m_enabled)
	{
		return;
	}

	RenderManager & renMan = RenderManager::Get();
	FontManager & fontMan = FontManager::Get();

	// Don't draw lines over the menu
	if (!IsDebugMenuActive())
	{
		// Draw 2D gridlines
		renMan.AddLine2D(RenderManager::eBatchDebug2D, Vector2(-1.0f, 0.0f), Vector2(1.0f, 0.0f), sc_colourGreyAlpha);
		renMan.AddLine2D(RenderManager::eBatchDebug2D, Vector2(0.0f, 1.0f),  Vector2(0.0f, -1.0f), sc_colourGreyAlpha);

		// Draw 3D gridlines
		const unsigned int gridSize = 10;
		const float gridMeasurement = 1.0f;
		Vector gridStart(-((float)gridSize * 0.5f) * gridMeasurement, -((float)gridSize * 0.5f) * gridMeasurement, 0.0f);	
	
		// Gridlines on the X axis
		for (unsigned int x = 0; x < gridSize+1; ++x)
		{
			Vector curLineX = gridStart + Vector(x*gridMeasurement, 0.0f, 0.0f);
			renMan.AddLine(RenderManager::eBatchDebug3D, curLineX, curLineX + Vector(0.0f, gridMeasurement * (float)(gridSize), 0.0f), sc_colourGreyAlpha);	
		}

		// Gridlines on the Y axis
		for (unsigned int y = 0; y < gridSize+1; ++y)
		{
			Vector curLineY = gridStart + Vector(0.0f, y*gridMeasurement, 0.0f);
			renMan.AddLine(RenderManager::eBatchDebug3D, curLineY, curLineY + Vector(gridMeasurement * (float)(gridSize), 0.0f, 0.0f), sc_colourGreyAlpha);
		}

		// Draw an identity matrix at the origin
		renMan.AddDebugMatrix(Matrix::Identity());

		// Draw selection box around objects
		if (m_gameObjectToEdit != NULL)
		{
			renMan.AddDebugAxisBox(m_gameObjectToEdit->GetPos(), m_gameObjectToEdit->GetClipSize(), sc_colourRed);
		}
	}
	
	// Show mouse pos at cursor
	char mouseBuf[16];
	Vector2 mousePos = InputManager::Get().GetMousePosRelative();
	sprintf(mouseBuf, "%.2f, %.2f", mousePos.GetX(), mousePos.GetY());
	Vector2 displayPos(mousePos.GetX() + sc_cursorSize, mousePos.GetY() - sc_cursorSize);
	fontMan.DrawDebugString2D(mouseBuf, displayPos, sc_colourGreen);

	// Draw mouse cursor
	for (int i = 0; i < 3; ++i)
	{
		renMan.AddLine2D(RenderManager::eBatchDebug2D, mousePos+sc_vectorCursor[i], mousePos+sc_vectorCursor[i+1], sc_colourGreen);
	}
	renMan.AddLine2D(RenderManager::eBatchDebug2D, mousePos+sc_vectorCursor[3], mousePos+sc_vectorCursor[0], sc_colourGreen);
}

Widget * DebugMenu::CreateButton(const char * a_name, Colour a_colour, Widget * a_parent)
{
	// All debug menu elements are created roughly equal
	Widget::WidgetDef curItem;
	curItem.m_size = WidgetVector(0.2f, 0.1f);
	curItem.m_pos = WidgetVector(10.0f, 10.0f);
	curItem.m_selectFlags = Widget::eSelectionRollover;
	curItem.m_colour = a_colour;
	curItem.m_name = a_name;

	// Check for a loaded debug font
	if (StringHash * debugFont = FontManager::Get().GetDebugFontName())
	{
		curItem.m_fontNameHash = debugFont->GetHash();
	}

	Widget * newWidget = Gui::Get().CreateWidget(curItem, a_parent, false);
	newWidget->SetDebugWidget();
	newWidget->SetAction(this, &DebugMenu::OnMenuItemMouseUp);
	newWidget->SetActive(false);

	return newWidget;
}

void DebugMenu::ShowCreateMenu(bool a_show)
{
	// Set the create menu and first children visible
	if (a_show)
	{
		m_btnCreateRoot->SetActive(a_show);
		m_btnCreateWidget->SetActive(a_show);
		m_btnCreateGameObject->SetActive(a_show);
	}
	else // Hide everything
	{
		m_btnCreateRoot->SetActive(a_show);
		m_btnCreateWidget->SetActive(a_show);
		m_btnCreateGameObject->SetActive(a_show);
		m_btnCreateGameObjectFromTemplate->SetActive(a_show);
		m_btnCreateGameObjectNew->SetActive(a_show);
	}
}

void DebugMenu::ShowChangeGUIMenu(bool a_show)
{
	// Set the change menu and all children visible
	m_btnChangeGUIRoot->SetActive(a_show);
	m_btnChangeGUIPos->SetActive(a_show);
	m_btnChangeGUIShape->SetActive(a_show);
	m_btnChangeGUIName->SetActive(a_show);
	m_btnChangeGUITexture->SetActive(a_show);
	m_btnDeleteGUI->SetActive(a_show);
}

void DebugMenu::ShowChangeObjectMenu(bool a_show)
{
	// Set the change menu and all children visible
	m_btnChangeObjectRoot->SetActive(a_show);
	m_btnChangeObjectName->SetActive(a_show);
	m_btnChangeObjectModel->SetActive(a_show);
	m_btnSaveObjectTemplate->SetActive(a_show);
	m_btnDeleteObject->SetActive(a_show);
}

void DebugMenu::HideResoureSelect()
{
	m_resourceSelect->SetActive(false);
	m_resourceSelectList->SetActive(false);
	m_btnResourceSelectOk->SetActive(false);
	m_btnResourceSelectCancel->SetActive(false);
}

void DebugMenu::HideTextInput()
{
	m_textInput->SetActive(false);
	m_textInputField->SetActive(false);
	m_btnTextInputOk->SetActive(false);
	m_btnTextInputCancel->SetActive(false);
}