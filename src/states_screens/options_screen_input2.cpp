//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2009 Marianne Gagnon
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include "states_screens/options_screen_input2.hpp"

#include "graphics/irr_driver.hpp"
#include "guiengine/CGUISpriteBank.h"
#include "guiengine/scalable_font.hpp"
#include "guiengine/screen.hpp"
#include "guiengine/widget.hpp"
#include "guiengine/widgets/button_widget.hpp"
#include "guiengine/widgets/label_widget.hpp"
#include "guiengine/widgets/list_widget.hpp"
#include "guiengine/widgets/ribbon_widget.hpp"
#include "input/input_manager.hpp"
#include "input/device_manager.hpp"
#include "io/file_manager.hpp"
#include "states_screens/dialogs/press_a_key_dialog.hpp"
#include "states_screens/options_screen_audio.hpp"
#include "states_screens/options_screen_input.hpp"
#include "states_screens/options_screen_players.hpp"
#include "states_screens/options_screen_video.hpp"
#include "states_screens/options_screen_ui.hpp"
#include "states_screens/state_manager.hpp"
#include "utils/translation.hpp"

#include <iostream>
#include <sstream>
#include <set>

using namespace GUIEngine;

DEFINE_SCREEN_SINGLETON( OptionsScreenInput2 );

// -----------------------------------------------------------------------------

OptionsScreenInput2::OptionsScreenInput2() : Screen("options_device.stkgui")
{
    m_config = NULL;
}   // OptionsScreenInput2

// -----------------------------------------------------------------------------

void OptionsScreenInput2::loadedFromFile()
{    
}   // loadedFromFile

// -----------------------------------------------------------------------------

void OptionsScreenInput2::init()
{
    Screen::init();
    RibbonWidget* tabBar = this->getWidget<RibbonWidget>("options_choice");
    if (tabBar != NULL)  tabBar->select( "tab_controls", PLAYER_ID_GAME_MASTER );
    
    tabBar->getRibbonChildren()[0].setTooltip( _("Graphics") );
    tabBar->getRibbonChildren()[1].setTooltip( _("Audio") );
    tabBar->getRibbonChildren()[2].setTooltip( _("User Interface") );
    tabBar->getRibbonChildren()[3].setTooltip( _("Players") );
    
    
    ButtonWidget* deleteBtn = this->getWidget<ButtonWidget>("delete");
    if (m_config->getType() != DEVICE_CONFIG_TYPE_KEYBOARD)
    {
        core::stringw label = (m_config->isEnabled() ? //I18N: button to disable a gamepad configuration
                                                      _("Disable Device")
                                                     : //I18N: button to enable a gamepad configuration
                                                      _("Enable Device"));
        
        // Make sure button is wide enough as the text is being changed away from the original value
        core::dimension2d<u32> size = GUIEngine::getFont()->getDimension(label.c_str());
        const int needed = size.Width + deleteBtn->getWidthNeededAroundLabel();
        if (deleteBtn->m_w < needed) deleteBtn->m_w = needed;
        
        deleteBtn->setLabel(label);
    }
    else
    {
        deleteBtn->setLabel(_("Delete Configuration"));
        
        if (input_manager->getDeviceList()->getKeyboardAmount() < 2)
        {
            // don't allow deleting the last config
            deleteBtn->setDeactivated();
        }
        else
        {
            deleteBtn->setActivated();
        }
    }
    
    // Make the two buttons the same length, not strictly needed but will look nicer...
    ButtonWidget* backBtn = this->getWidget<ButtonWidget>("back_to_device_list");
    if (backBtn->m_w < deleteBtn->m_w) backBtn->m_w   = deleteBtn->m_w;
    else                               deleteBtn->m_w = backBtn->m_w;
    
    backBtn->moveIrrlichtElement();
    deleteBtn->moveIrrlichtElement();
    
    LabelWidget* label = this->getWidget<LabelWidget>("title");
    label->setText( m_config->getName().c_str(), false );
    
    GUIEngine::ListWidget* actions = this->getWidget<GUIEngine::ListWidget>("actions");
    assert( actions != NULL );
   
    // ---- create list skeleton (right number of items, right internal names)
    //      their actualy contents will be adapted as needed after
    
    //I18N: Key binding section
    actions->addItem("game_keys_section", _("Game Keys") );
    actions->addItem(KartActionStrings[PA_STEER_LEFT],  L"" );
    actions->addItem(KartActionStrings[PA_STEER_RIGHT], L"" );
    actions->addItem(KartActionStrings[PA_ACCEL],       L"" );
    actions->addItem(KartActionStrings[PA_BRAKE],       L"" );
    actions->addItem(KartActionStrings[PA_FIRE],        L"" );
    actions->addItem(KartActionStrings[PA_NITRO],       L"" );
    actions->addItem(KartActionStrings[PA_DRIFT],       L"" );
    actions->addItem(KartActionStrings[PA_LOOK_BACK],   L"" );
    actions->addItem(KartActionStrings[PA_RESCUE],      L"" );
    

    //I18N: Key binding section
    actions->addItem("menu_keys_section", _("Menu Keys") );
    actions->addItem(KartActionStrings[PA_MENU_UP],     L"" );
    actions->addItem(KartActionStrings[PA_MENU_DOWN],   L"" );
    actions->addItem(KartActionStrings[PA_MENU_LEFT],   L"" );
    actions->addItem(KartActionStrings[PA_MENU_RIGHT],  L"" );
    actions->addItem(KartActionStrings[PA_MENU_SELECT], L"");
    actions->addItem(KartActionStrings[PA_MENU_CANCEL], L"" );
    
    updateInputButtons();
}   // init

// -----------------------------------------------------------------------------

irr::core::stringw OptionsScreenInput2::makeLabel(const irr::core::stringw translatedName,
                                                  PlayerAction action) const
{
    //hack: one tab character is supported by out font object, it moves the cursor to the middle of the area
    core::stringw out = irr::core::stringw("    ") + translatedName + L"\t";
    
    out += m_config->getBindingAsString(action);
    return out;
}   // makeLabel

// -----------------------------------------------------------------------------

void OptionsScreenInput2::updateInputButtons()
{
    assert(m_config != NULL);
    
    //TODO: detect duplicates
    
    GUIEngine::ListWidget* actions = this->getWidget<GUIEngine::ListWidget>("actions");
    assert( actions != NULL );
    
    // item 0 is a section header
    
    //I18N: Key binding name
    actions->renameItem(1, makeLabel( _("Steer Left"), PA_STEER_LEFT) );
    
    //I18N: Key binding name
    actions->renameItem(2, makeLabel( _("Steer Right"), PA_STEER_RIGHT) );
    
    //I18N: Key binding name
    actions->renameItem(3, makeLabel( _("Accelerate"), PA_ACCEL) );
    
    //I18N: Key binding name
    actions->renameItem(4, makeLabel( _("Brake"), PA_BRAKE) );
    
    //I18N: Key binding name
    actions->renameItem(5, makeLabel( _("Fire"), PA_FIRE) );
    
    //I18N: Key binding name
    actions->renameItem(6, makeLabel( _("Nitro"), PA_NITRO) );
    
    //I18N: Key binding name
    actions->renameItem(7, makeLabel( _("Sharp Turn"), PA_DRIFT) );
    
    //I18N: Key binding name
    actions->renameItem(8, makeLabel( _("Look Back"), PA_LOOK_BACK) );
    
    //I18N: Key binding name
    actions->renameItem(9, makeLabel( _("Rescue"), PA_RESCUE) );
    
    
    // item 10 is a section header
    
    //I18N: Key binding name
    actions->renameItem(11, makeLabel( _("Up"), PA_MENU_UP) );
    
    //I18N: Key binding name
    actions->renameItem(12, makeLabel( _("Down"), PA_MENU_DOWN) );
    
    //I18N: Key binding name
    actions->renameItem(13, makeLabel( _("Left"), PA_MENU_LEFT) );
    
    //I18N: Key binding name
    actions->renameItem(14, makeLabel( _("Right"), PA_MENU_RIGHT) );
    
    //I18N: Key binding name
    actions->renameItem(15, makeLabel( _("Select"), PA_MENU_SELECT) );
    
    //I18N: Key binding name
    actions->renameItem(16, makeLabel( _("Cancel/Back"), PA_MENU_CANCEL) );
    
    
    // ---- make sure there are no binding conflicts (same key used for two actions)
    std::set<irr::core::stringw> currentlyUsedKeys;
    for (PlayerAction action = PA_FIRST_GAME_ACTION;
         action <= PA_LAST_GAME_ACTION;
         action=PlayerAction(action+1))
    {
        const irr::core::stringw item = m_config->getMappingIdString(action);
        if (currentlyUsedKeys.find(item) == currentlyUsedKeys.end())
        {
            currentlyUsedKeys.insert( item );
        }
        else
        {            
            // binding conflict!
            actions->markItemRed( KartActionStrings[action] );
            
            // also mark others
            for (PlayerAction others = PA_FIRST_GAME_ACTION;
                 others < action; others=PlayerAction(others+1))
            {
                const irr::core::stringw others_item = m_config->getMappingIdString(others);
                if (others_item == item)
                {
                    actions->markItemRed( KartActionStrings[others] );
                }
            }
            
            //actions->renameItem( KartActionStrings[action], _("Binding Conflict!") ); 
        }
    }
    
    // menu keys and game keys can overlap, no problem, so forget game keys before checking menu keys
    currentlyUsedKeys.clear();
    for (PlayerAction action = PA_FIRST_MENU_ACTION;
         action <= PA_LAST_MENU_ACTION;
         action=PlayerAction(action+1))
    {
        const irr::core::stringw item = m_config->getBindingAsString(action);
        if (currentlyUsedKeys.find(item) == currentlyUsedKeys.end())
        {
            currentlyUsedKeys.insert( item );
        }
        else
        {            
            // binding conflict!
            actions->markItemRed( KartActionStrings[action] );

            // also mark others
            for (PlayerAction others = PA_FIRST_MENU_ACTION;
                 others < action; others=PlayerAction(others+1))
            {
                const irr::core::stringw others_item = m_config->getBindingAsString(others);
                if (others_item == item)
                {
                    actions->markItemRed( KartActionStrings[others] );
                }
            }
            
            //actions->renameItem( KartActionStrings[action], _("Binding Conflict!") ); 
        }
    }
}   // updateInputButtons

// -----------------------------------------------------------------------------

static PlayerAction binding_to_set;
static std::string binding_to_set_button;

void OptionsScreenInput2::gotSensedInput(const Input& sensed_input)
{
    const bool keyboard = (m_config->getType() == DEVICE_CONFIG_TYPE_KEYBOARD &&
                           sensed_input.m_type == Input::IT_KEYBOARD);
    const bool gamepad =  (sensed_input.m_type == Input::IT_STICKMOTION ||
                           sensed_input.m_type == Input::IT_STICKBUTTON ||
                           sensed_input.m_type == Input::IT_STICKHAT) &&
                           m_config->getType() == DEVICE_CONFIG_TYPE_GAMEPAD;
    
    if (keyboard)
    {
		if (UserConfigParams::m_verbosity>=5)
        {
			std::cout << "% Binding " << KartActionStrings[binding_to_set] 
                << " : setting to keyboard key " << sensed_input.m_button_id
                << " \n\n";
        }
        
        KeyboardConfig* keyboard = (KeyboardConfig*)m_config;
        keyboard->setBinding(binding_to_set, Input::IT_KEYBOARD, 
                             sensed_input.m_button_id, Input::AD_NEUTRAL,
                             sensed_input.m_character);
        
        // refresh display
        updateInputButtons();
    }
    else if (gamepad)
    {
		if (UserConfigParams::m_verbosity>=5)
        {
			std::cout << "% Binding " << KartActionStrings[binding_to_set] 
                      << " : setting to gamepad #" 
                      << sensed_input.m_device_id<< " : ";
        
            if (sensed_input.m_type == Input::IT_STICKMOTION)
            {
                std::cout << "axis " << sensed_input.m_button_id<< " direction "
                          << (sensed_input.m_axis_direction== Input::AD_NEGATIVE
                              ? "-" : "+") 
                          << "\n\n";
            }
            else if (sensed_input.m_type == Input::IT_STICKBUTTON)
            {
                std::cout << "button " << sensed_input.m_button_id<< "\n\n";
            }
            else if (sensed_input.m_type == Input::IT_STICKHAT)
            {
                std::cout << "Hat " << sensed_input.m_button_id << "\n\n";
            }
            else
            {
                std::cout << "Sensed unknown gamepad event type??\n";
            }
        }
        
        GamepadConfig* config =  (GamepadConfig*)m_config;
        config->setBinding(binding_to_set, sensed_input.m_type, 
                           sensed_input.m_button_id,
                           (Input::AxisDirection)sensed_input.m_axis_direction);
        
        // refresh display
        updateInputButtons();
    }
    else
    {
        return;
    }
    
    ModalDialog::dismiss();
    input_manager->setMode(InputManager::MENU);
        
    if (keyboard && (sensed_input.m_button_id == irr::KEY_SHIFT || sensed_input.m_button_id == irr::KEY_LSHIFT || 
                     sensed_input.m_button_id == irr::KEY_RSHIFT))
    {
        new MessageDialog(_("Warning, 'Shift' is not a recommended key : when shift is pressed down, all keys that contain a character that is different in upper-case will stop working."));
    }
    
    // re-select the previous button (TODO!)
    //ButtonWidget* btn = this->getWidget<ButtonWidget>(binding_to_set_button.c_str());
    //if(btn != NULL) btn->setFocusForPlayer(PLAYER_ID_GAME_MASTER);
    
    // save new binding to file
    input_manager->getDeviceList()->serialize();
}   // gotSensedInput


// -----------------------------------------------------------------------------

void OptionsScreenInput2::eventCallback(Widget* widget, const std::string& name, const int playerID)
{
    //const std::string& screen_name = this->getName();
    
    if (name == "options_choice")
    {
        std::string selection = ((RibbonWidget*)widget)->getSelectionIDString(PLAYER_ID_GAME_MASTER).c_str();
        
        if (selection == "tab_audio")        StateManager::get()->replaceTopMostScreen(OptionsScreenAudio::getInstance());
        else if (selection == "tab_video")   StateManager::get()->replaceTopMostScreen(OptionsScreenVideo::getInstance());
        else if (selection == "tab_players") StateManager::get()->replaceTopMostScreen(OptionsScreenPlayers::getInstance());
        else if (selection == "tab_ui") StateManager::get()->replaceTopMostScreen(OptionsScreenUI::getInstance());
        else if (selection == "tab_controls") {}
    }
    else if (name == "back_to_device_list")
    {
        StateManager::get()->replaceTopMostScreen(OptionsScreenInput::getInstance());
    }
    else if (name == "back")
    {
        StateManager::get()->popMenu();
    }
    else if (name == "actions")
    {
        GUIEngine::ListWidget* actions = this->getWidget<GUIEngine::ListWidget>("actions");
        assert( actions != NULL );
        
        // a player action in the list was clicked. find which one
        const std::string& clicked = actions->getSelectionInternalName();
        for (int n=PA_BEFORE_FIRST+1; n<PA_COUNT; n++)
        {
            if (KartActionStrings[n] == clicked)
            {                
                // we found which one. show the "press a key" dialog.
                if (UserConfigParams::m_verbosity>=5)
                {
                    std::cout << "\n% Entering sensing mode for " 
                              << m_config->getName().c_str() 
                              << std::endl;
                }
                
                binding_to_set = (PlayerAction)n;
                
                new PressAKeyDialog(0.4f, 0.4f);
                
                if (m_config->getType() == DEVICE_CONFIG_TYPE_KEYBOARD)
                {
                    input_manager->setMode(InputManager::INPUT_SENSE_KEYBOARD);
                }
                else if (m_config->getType() == DEVICE_CONFIG_TYPE_GAMEPAD)
                {
                    input_manager->setMode(InputManager::INPUT_SENSE_GAMEPAD);
                }
                else
                {
                    std::cerr << "unknown selection device in options : "
                              << m_config->getName().c_str() << std::endl;
                }                
                break;
            }
        }
    }
    else if (name == "delete")
    {
        if (m_config->getType() == DEVICE_CONFIG_TYPE_KEYBOARD)
        {
           // keyboard configs may be deleted 
           //I18N: shown before deleting an input configuration
            new MessageDialog( _("Are you sure you want to permanently delete this configuration?"), this, false );
        }
        else
        {
            // gamepad configs may be disabled
            if (m_config->isEnabled())  m_config->setEnabled(false);
            else                        m_config->setEnabled(true);
            
            // update widget label
            ButtonWidget* deleteBtn = this->getWidget<ButtonWidget>("delete");
            if (m_config->isEnabled())  deleteBtn->setLabel(_("Disable Device"));
            else                        deleteBtn->setLabel(_("Enable Device"));
            
            input_manager->getDeviceList()->serialize();
        }
    }
    
}   // eventCallback

// -----------------------------------------------------------------------------

void OptionsScreenInput2::unloaded()
{
}   // unloaded

// -----------------------------------------------------------------------------

bool OptionsScreenInput2::onEscapePressed()
{
    StateManager::get()->replaceTopMostScreen(OptionsScreenInput::getInstance());
    return false; // don't use standard escape key handler, we handled it differently
}   // onEscapePressed

// -----------------------------------------------------------------------------

void OptionsScreenInput2::onConfirm()
{
    const bool success = input_manager->getDeviceList()->deleteConfig(m_config);
    assert(success);
    if (!success) fprintf(stderr, "Failed to delete config!\n");

    m_config = NULL;
    input_manager->getDeviceList()->serialize();
    ModalDialog::dismiss();
    StateManager::get()->replaceTopMostScreen(OptionsScreenInput::getInstance());
}

// -----------------------------------------------------------------------------
