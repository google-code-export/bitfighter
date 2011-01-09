//-----------------------------------------------------------------------------------
//
// Bitfighter - A multiplayer vector graphics space game
// Based on Zap demo released for Torque Network Library by GarageGames.com
//
// Derivative work copyright (C) 2008-2009 Chris Eykamp
// Original work copyright (C) 2004 GarageGames.com, Inc.
// Other code copyright as noted
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful (and fun!),
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

#include "input.h"
#include "IniFile.h"
#include "config.h"
#include "quickChat.h"
#include "gameLoader.h"    // For LevelListLoader::levelList

#ifdef _MSC_VER
#pragma warning (disable: 4996)     // Disable POSIX deprecation, certain security warnings that seem to be specific to VC++
#endif

#ifndef min
#define min(a,b) ((a) <= (b) ? (a) : (b))
#define max(a,b) ((a) >= (b) ? (a) : (b))
#endif


namespace Zap
{

extern CIniFile gINI;
extern IniSettings gIniSettings;
extern string lcase(string strToConvert);

// Sorts alphanumerically
extern S32 QSORT_CALLBACK alphaSort(string *a, string *b);


// Splits inputString into a series of words using the specified separator
void parseString(const char *inputString, Vector<string> &words, char seperator)
{
	char word[128];
	S32 wn = 0;       // Where we are in the word we're creating
	S32 isn = 0;      // Where we are in the inputString we're parsing

	words.clear();

	while(inputString[isn] != 0)
   {
		if(inputString[isn] == seperator) 
      {
			word[wn] = 0;    // Add terminating NULL
			if(wn > 0) 
            words.push_back(word);
			wn = 0;
		}
      else
      {
			if(wn < 126)   // Avoid overflows
         {
            word[wn] = inputString[isn]; 
            wn++; 
         }
		}
		isn++;
	}
    word[wn] = 0;
    if(wn > 0) 
       words.push_back(word);
}


extern Vector<string> prevServerListFromMaster;
extern Vector<string> alwaysPingList;

static void loadForeignServerInfo()
{
	// AlwaysPingList will default to broadcast, can modify the list in the INI
   // http://learn-networking.com/network-design/how-a-broadcast-address-works
	parseString(gINI.GetValue("Connections", "AlwaysPingList", "IP:Broadcast:28000").c_str(), alwaysPingList, ',');

   // These are the servers we found last time we were able to contact the master.
	// In case the master server fails, we can use this list to try to find some game servers. 
	//parseString(gINI.GetValue("ForeignServers", "ForeignServerList").c_str(), prevServerListFromMaster, ',');
   gINI.GetAllValues("RecentForeignServers", prevServerListFromMaster);
}

static void writeConnectionsInfo()
{
   if(gINI.numSectionComments("Connections") == 0)
   {
      gINI.sectionComment("Connections", "----------------");
      gINI.sectionComment("Connections", " AlwaysPingList - Always try to contact these servers (comma separated list); Format: IP:IPAddress:Port");
      gINI.sectionComment("Connections", "                  Include 'IP:Broadcast:28000' to search LAN for local servers on default port");
      gINI.sectionComment("Connections", "----------------");
   }

   // Creates comma delimited list
	string str = "";
   for(S32 i = 0; i < alwaysPingList.size(); i++)
        str += alwaysPingList[i] + ((i < alwaysPingList.size() - 1) ? "," : "");
	gINI.SetValue("Connections", "AlwaysPingList", str);
}


static void writeForeignServerInfo()
{
   if(gINI.numSectionComments("RecentForeignServers") == 0)
   {
   
      gINI.sectionComment("RecentForeignServers", "----------------");
      gINI.sectionComment("RecentForeignServers", " This section contains a list of the most recent servers seen; used as a fallback if we can't reach the master");
      gINI.sectionComment("RecentForeignServers", " Please be aware that this section will be automatically regenerated, and any changes you make will be overwritten");
      gINI.sectionComment("RecentForeignServers", "----------------");
   }

   gINI.SetAllValues("RecentForeignServers", "Server", prevServerListFromMaster);
}


// Read levels, if there are any...
 void loadLevels()
{
   if(gINI.findSection("Levels") != gINI.noID)
   {
      S32 numLevels = gINI.NumValues("Levels");
      Vector<string> levelValNames(numLevels);

      for(S32 i = 0; i < numLevels; i++)
         levelValNames.push_back(gINI.ValueName("Levels", i));

      levelValNames.sort(alphaSort);

      string level;
      for(S32 i = 0; i < numLevels; i++)
      {
         level = gINI.GetValue("Levels", levelValNames[i], "");
         if (level != "")
            gIniSettings.levelList.push_back(StringTableEntry(level.c_str()));
      }
   }
}


extern Vector<StringTableEntry> gLevelSkipList;

// Read level deleteList, if there are any.  This could probably be made more efficient by not reading the
// valnames in first, but what the heck...
static void loadLevelSkipList()
{
   Vector<string> skipList;

   gINI.GetAllValues("LevelSkipList", skipList);

   for(S32 i = 0; i < skipList.size(); i++)
      gLevelSkipList.push_back(StringTableEntry(skipList[i].c_str()));
}


// Convert a string value to a DisplayMode enum value
static DisplayMode stringToDisplayMode(string mode)
{
   if(lcase(mode) == "fullscreen-stretch")
      return DISPLAY_MODE_FULL_SCREEN_STRETCHED;
   else if(lcase(mode) == "fullscreen")
      return DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED;
   else 
      return DISPLAY_MODE_WINDOWED;
}


// Convert a string value to our sfxSets enum
static string displayModeToString(DisplayMode mode)
{
   if(mode == DISPLAY_MODE_FULL_SCREEN_STRETCHED)
      return "Fullscreen-Stretch";
   else if(mode == DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED)
      return "Fullscreen";
   else
      return "Window";
}


static void loadGeneralSettings()
{
   gIniSettings.displayMode = stringToDisplayMode( gINI.GetValue("Settings", "WindowMode", displayModeToString(gIniSettings.displayMode)));
   gIniSettings.oldDisplayMode = gIniSettings.displayMode;

   gIniSettings.controlsRelative = (lcase(gINI.GetValue("Settings", "ControlMode", (gIniSettings.controlsRelative ? "Relative" : "Absolute"))) == "relative");
   gIniSettings.echoVoice = (lcase(gINI.GetValue("Settings", "VoiceEcho",(gIniSettings.echoVoice ? "Yes" : "No"))) == "yes");
   gIniSettings.showWeaponIndicators = (lcase(gINI.GetValue("Settings", "LoadoutIndicators", (gIniSettings.showWeaponIndicators ? "Yes" : "No"))) == "yes");
   gIniSettings.verboseHelpMessages = (lcase(gINI.GetValue("Settings", "VerboseHelpMessages", (gIniSettings.verboseHelpMessages ? "Yes" : "No"))) == "yes");
   gIniSettings.showKeyboardKeys = (lcase(gINI.GetValue("Settings", "ShowKeyboardKeysInStickMode", (gIniSettings.showKeyboardKeys ? "Yes" : "No"))) == "yes");
   gIniSettings.joystickType = stringToJoystickType(gINI.GetValue("Settings", "JoystickType", joystickTypeToString(gIniSettings.joystickType)));
   gIniSettings.winXPos = max(gINI.GetValueI("Settings", "WindowXPos", gIniSettings.winXPos), 0);    // Restore window location
   gIniSettings.winYPos = max(gINI.GetValueI("Settings", "WindowYPos", gIniSettings.winYPos), 0);
   gIniSettings.winSizeFact = (F32) gINI.GetValueF("Settings", "WindowScalingFactor", gIniSettings.winSizeFact);
   gIniSettings.masterAddress = gINI.GetValue("Settings", "MasterServerAddress", gIniSettings.masterAddress);
   
   gIniSettings.name = gINI.GetValue("Settings", "Nickname", gIniSettings.name);
   gIniSettings.password = gINI.GetValue("Settings", "Password", gIniSettings.password);

   gIniSettings.defaultName = gINI.GetValue("Settings", "DefaultName", gIniSettings.defaultName);
   gIniSettings.lastName = gINI.GetValue("Settings", "LastName", gIniSettings.lastName);
   gIniSettings.lastPassword = gINI.GetValue("Settings", "LastPassword", gIniSettings.lastPassword);
   gIniSettings.lastEditorName = gINI.GetValue("Settings", "LastEditorName", gIniSettings.lastEditorName);

   gIniSettings.enableExperimentalAimMode = (lcase(gINI.GetValue("Settings", "EnableExperimentalAimMode", (gIniSettings.enableExperimentalAimMode ? "Yes" : "No"))) == "yes");
   gIniSettings.minSleepTimeClient = gINI.GetValueI("Settings", "MinClientDelay", gIniSettings.minSleepTimeClient);

   gDefaultLineWidth = (F32) gINI.GetValueF("Settings", "LineWidth", 2);
   gLineWidth1 = gDefaultLineWidth * 0.5f;
   gLineWidth3 = gDefaultLineWidth * 1.5f;
   gLineWidth4 = gDefaultLineWidth * 2;
}


static void loadDiagnostics()
{
   gIniSettings.diagnosticKeyDumpMode = (lcase(gINI.GetValue("Diagnostics", "DumpKeys",              (gIniSettings.diagnosticKeyDumpMode ? "Yes" : "No"))) == "yes");

   gIniSettings.logConnectionProtocol = (lcase(gINI.GetValue("Diagnostics", "LogConnectionProtocol", (gIniSettings.logConnectionProtocol ? "Yes" : "No"))) == "yes");
   gIniSettings.logNetConnection      = (lcase(gINI.GetValue("Diagnostics", "LogNetConnection",      (gIniSettings.logNetConnection      ? "Yes" : "No"))) == "yes");
   gIniSettings.logEventConnection    = (lcase(gINI.GetValue("Diagnostics", "LogEventConnection",    (gIniSettings.logEventConnection    ? "Yes" : "No"))) == "yes");
   gIniSettings.logGhostConnection    = (lcase(gINI.GetValue("Diagnostics", "LogGhostConnection",    (gIniSettings.logGhostConnection    ? "Yes" : "No"))) == "yes");
   gIniSettings.logNetInterface       = (lcase(gINI.GetValue("Diagnostics", "LogNetInterface",       (gIniSettings.logNetInterface       ? "Yes" : "No"))) == "yes");
   gIniSettings.logPlatform           = (lcase(gINI.GetValue("Diagnostics", "LogPlatform",           (gIniSettings.logPlatform           ? "Yes" : "No"))) == "yes");
   gIniSettings.logNetBase            = (lcase(gINI.GetValue("Diagnostics", "LogNetBase",            (gIniSettings.logNetBase            ? "Yes" : "No"))) == "yes");
   gIniSettings.logUDP                = (lcase(gINI.GetValue("Diagnostics", "LogUDP",                (gIniSettings.logUDP                ? "Yes" : "No"))) == "yes");

   gIniSettings.logFatalError         = (lcase(gINI.GetValue("Diagnostics", "LogFatalError",          (gIniSettings.logFatalError        ? "Yes" : "No"))) == "yes");
   gIniSettings.logError              = (lcase(gINI.GetValue("Diagnostics", "LogError",               (gIniSettings.logError             ? "Yes" : "No"))) == "yes");
   gIniSettings.logWarning            = (lcase(gINI.GetValue("Diagnostics", "LogWarning",             (gIniSettings.logWarning           ? "Yes" : "No"))) == "yes");
   gIniSettings.logConnection         = (lcase(gINI.GetValue("Diagnostics", "LogConnection",          (gIniSettings.logConnection        ? "Yes" : "No"))) == "yes");

   gIniSettings.logLevelLoaded        = (lcase(gINI.GetValue("Diagnostics", "LogLevelLoaded",          (gIniSettings.logLevelLoaded        ? "Yes" : "No"))) == "yes");
   gIniSettings.logLuaObjectLifecycle = (lcase(gINI.GetValue("Diagnostics", "LogLuaObjectLifecycle",   (gIniSettings.logLuaObjectLifecycle ? "Yes" : "No"))) == "yes");
   gIniSettings.luaLevelGenerator     = (lcase(gINI.GetValue("Diagnostics", "LuaLevelGenerator",       (gIniSettings.luaLevelGenerator     ? "Yes" : "No"))) == "yes");
   gIniSettings.luaBotMessage         = (lcase(gINI.GetValue("Diagnostics", "LuaBotMessage",           (gIniSettings.luaBotMessage         ? "Yes" : "No"))) == "yes");
   gIniSettings.serverFilter          = (lcase(gINI.GetValue("Diagnostics", "ServerFilter",            (gIniSettings.serverFilter          ? "Yes" : "No"))) == "yes");
}


static void loadTestSettings()
{
   gIniSettings.burstGraphicsMode = max(gINI.GetValueI("Testing", "BurstGraphics", gIniSettings.burstGraphicsMode), 0);
}

static void loadEffectsSettings()
{
   gIniSettings.starsInDistance  = (lcase(gINI.GetValue("Effects", "StarsInDistance", (gIniSettings.starsInDistance ? "Yes" : "No"))) == "yes");
   gIniSettings.useLineSmoothing = (lcase(gINI.GetValue("Effects", "LineSmoothing", "No")) == "yes");
}

// Convert a string value to our sfxSets enum
static sfxSets stringToSFXSet(string sfxSet)
{
   return (lcase(sfxSet) == "classic") ? sfxClassicSet : sfxModernSet;
}


static void loadSoundSettings()
{
   gIniSettings.sfxVolLevel = (float) gINI.GetValueI("Sounds", "EffectsVolume", (S32) (gIniSettings.sfxVolLevel * 10)) / 10.0f;
   gIniSettings.musicVolLevel = (float) gINI.GetValueI("Sounds", "MusicVolume", (S32) (gIniSettings.musicVolLevel * 10)) / 10.0f;
   gIniSettings.voiceChatVolLevel = (float) gINI.GetValueI("Sounds", "VoiceChatVolume", (S32) (gIniSettings.voiceChatVolLevel * 10)) / 10.0f;

   string sfxSet = gINI.GetValue("Sounds", "SFXSet", "Modern");
   gIniSettings.sfxSet = stringToSFXSet(sfxSet);

   // Bounds checking
   if(gIniSettings.sfxVolLevel > 1.0)
      gIniSettings.sfxVolLevel = 1.0;
   else if(gIniSettings.sfxVolLevel < 0)
      gIniSettings.sfxVolLevel = 0;

   if(gIniSettings.musicVolLevel > 1.0)
      gIniSettings.musicVolLevel = 1.0;
   else if(gIniSettings.musicVolLevel < 0)
      gIniSettings.musicVolLevel = 0;

   if(gIniSettings.voiceChatVolLevel > 1.0)
      gIniSettings.voiceChatVolLevel = 1.0;
   else if(gIniSettings.voiceChatVolLevel < 0)
      gIniSettings.voiceChatVolLevel = 0;

   if(gIniSettings.alertsVolLevel > 1.0)
      gIniSettings.alertsVolLevel = 1.0;
   else if(gIniSettings.alertsVolLevel < 0)
      gIniSettings.alertsVolLevel = 0;
}


static void loadHostConfiguration()
{
   gIniSettings.hostname = gINI.GetValue("Host", "ServerName", gIniSettings.hostname);
   gIniSettings.hostaddr = gINI.GetValue("Host", "ServerAddress", gIniSettings.hostaddr);
   gIniSettings.hostdescr = gINI.GetValue("Host", "ServerDescription", gIniSettings.hostdescr);

   gIniSettings.serverPassword = gINI.GetValue("Host", "ServerPassword", gIniSettings.serverPassword);
   gIniSettings.adminPassword = gINI.GetValue("Host", "AdminPassword", gIniSettings.adminPassword);
   gIniSettings.levelChangePassword = gINI.GetValue("Host", "LevelChangePassword", gIniSettings.levelChangePassword);
   gIniSettings.levelDir = gINI.GetValue("Host", "LevelDir", gIniSettings.levelDir);
   gIniSettings.maxplayers = gINI.GetValueI("Host", "MaxPlayers", gIniSettings.maxplayers);

   gIniSettings.alertsVolLevel = (float) gINI.GetValueI("Host", "AlertsVolume", (S32) (gIniSettings.alertsVolLevel * 10)) / 10.0f;
   gIniSettings.allowGetMap = (lcase(gINI.GetValue("Host", "AllowGetMap", "No")) == "yes");
   gIniSettings.allowDataConnections = (lcase(gINI.GetValue("Host", "AllowDataConnections", (gIniSettings.allowDataConnections ? "Yes" : "No"))) == "yes");
   gIniSettings.minSleepTimeDedicatedServer = gINI.GetValueI("Host", "MinDedicatedDelay", 10);
}


void loadUpdaterSettings()
{
   gIniSettings.useUpdater = gINI.GetValueB("Updater", "UseUpdater", gIniSettings.useUpdater);
}


// Remember: If you change any of these defaults, you'll need to rebuild your INI file to see the results!
static void loadKeyBindings()
{                                // Whew!  This is quite the dense block of code!!
   keySELWEAP1[Keyboard] = stringToKeyCode(gINI.GetValue("KeyboardKeyBindings", "SelWeapon1", keyCodeToString(KEY_1)).c_str());
   keySELWEAP2[Keyboard] = stringToKeyCode(gINI.GetValue("KeyboardKeyBindings", "SelWeapon2", keyCodeToString(KEY_2)).c_str());
   keySELWEAP3[Keyboard] = stringToKeyCode(gINI.GetValue("KeyboardKeyBindings", "SelWeapon3", keyCodeToString(KEY_3)).c_str());
   keyADVWEAP[Keyboard] = stringToKeyCode(gINI.GetValue("KeyboardKeyBindings", "SelNextWeapon", keyCodeToString(KEY_E)).c_str());
   keyCMDRMAP[Keyboard] = stringToKeyCode(gINI.GetValue("KeyboardKeyBindings", "ShowCmdrMap", keyCodeToString(KEY_C)).c_str());
   keyTEAMCHAT[Keyboard] = stringToKeyCode(gINI.GetValue("KeyboardKeyBindings", "TeamChat", keyCodeToString(KEY_T)).c_str());
   keyGLOBCHAT[Keyboard] = stringToKeyCode(gINI.GetValue("KeyboardKeyBindings", "GlobalChat", keyCodeToString(KEY_G)).c_str());
   keyQUICKCHAT[Keyboard] = stringToKeyCode(gINI.GetValue("KeyboardKeyBindings", "QuickChat", keyCodeToString(KEY_V)).c_str());
   keyCMDCHAT[Keyboard] = stringToKeyCode(gINI.GetValue("KeyboardKeyBindings", "Command", keyCodeToString(KEY_SLASH)).c_str());
   keyLOADOUT[Keyboard] = stringToKeyCode(gINI.GetValue("KeyboardKeyBindings", "ShowLoadoutMenu", keyCodeToString(KEY_Z)).c_str());
   keyMOD1[Keyboard] = stringToKeyCode(gINI.GetValue("KeyboardKeyBindings", "ActivateModule1", keyCodeToString(KEY_SPACE)).c_str());
   keyMOD2[Keyboard] = stringToKeyCode(gINI.GetValue("KeyboardKeyBindings", "ActivateModule2", keyCodeToString(MOUSE_RIGHT)).c_str());
   keyFIRE[Keyboard] = stringToKeyCode(gINI.GetValue("KeyboardKeyBindings", "Fire", keyCodeToString(MOUSE_LEFT)).c_str());
   keyDROPITEM[Keyboard] = stringToKeyCode(gINI.GetValue("KeyboardKeyBindings", "DropItem", keyCodeToString(KEY_B)).c_str());

   keyTOGVOICE[Keyboard] = stringToKeyCode(gINI.GetValue("KeyboardKeyBindings", "VoiceChat", keyCodeToString(KEY_R)).c_str());
   keyUP[Keyboard] = stringToKeyCode(gINI.GetValue("KeyboardKeyBindings", "ShipUp", keyCodeToString(KEY_W)).c_str());
   keyDOWN[Keyboard] = stringToKeyCode(gINI.GetValue("KeyboardKeyBindings", "ShipDown", keyCodeToString(KEY_S)).c_str());
   keyLEFT[Keyboard] = stringToKeyCode(gINI.GetValue("KeyboardKeyBindings", "ShipLeft", keyCodeToString(KEY_A)).c_str());
   keyRIGHT[Keyboard] = stringToKeyCode(gINI.GetValue("KeyboardKeyBindings", "ShipRight", keyCodeToString(KEY_D)).c_str());
   keySCRBRD[Keyboard] = stringToKeyCode(gINI.GetValue("KeyboardKeyBindings", "ShowScoreboard", keyCodeToString(KEY_TAB)).c_str());

   keySELWEAP1[Joystick] = stringToKeyCode(gINI.GetValue("JoystickKeyBindings", "SelWeapon1", keyCodeToString(KEY_1)).c_str());
   keySELWEAP2[Joystick] = stringToKeyCode(gINI.GetValue("JoystickKeyBindings", "SelWeapon2", keyCodeToString(KEY_2)).c_str());
   keySELWEAP3[Joystick] = stringToKeyCode(gINI.GetValue("JoystickKeyBindings", "SelWeapon3", keyCodeToString(KEY_3)).c_str());
   keyADVWEAP[Joystick] = stringToKeyCode(gINI.GetValue("JoystickKeyBindings", "SelNextWeapon", keyCodeToString(BUTTON_1)).c_str());
   keyCMDRMAP[Joystick] = stringToKeyCode(gINI.GetValue("JoystickKeyBindings", "ShowCmdrMap", keyCodeToString(BUTTON_2)).c_str());
   keyTEAMCHAT[Joystick] = stringToKeyCode(gINI.GetValue("JoystickKeyBindings", "TeamChat", keyCodeToString(KEY_T)).c_str());
   keyGLOBCHAT[Joystick] = stringToKeyCode(gINI.GetValue("JoystickKeyBindings", "GlobalChat", keyCodeToString(KEY_G)).c_str());
   keyQUICKCHAT[Joystick] = stringToKeyCode(gINI.GetValue("JoystickKeyBindings", "QuickChat", keyCodeToString(BUTTON_3)).c_str());
   keyCMDCHAT[Joystick] = stringToKeyCode(gINI.GetValue("JoystickKeyBindings", "Command", keyCodeToString(KEY_SLASH)).c_str());
   keyLOADOUT[Joystick] = stringToKeyCode(gINI.GetValue("JoystickKeyBindings", "ShowLoadoutMenu", keyCodeToString(BUTTON_4)).c_str());
   keyMOD1[Joystick] = stringToKeyCode(gINI.GetValue("JoystickKeyBindings", "ActivateModule1", keyCodeToString(BUTTON_7)).c_str());
   keyMOD2[Joystick] = stringToKeyCode(gINI.GetValue("JoystickKeyBindings", "ActivateModule2", keyCodeToString(BUTTON_6)).c_str());
   keyFIRE[Joystick] = stringToKeyCode(gINI.GetValue("JoystickKeyBindings", "Fire", keyCodeToString(MOUSE_LEFT)).c_str());
   keyDROPITEM[Joystick] = stringToKeyCode(gINI.GetValue("JoystickKeyBindings", "DropItem", keyCodeToString(BUTTON_8)).c_str());
   keyTOGVOICE[Joystick] = stringToKeyCode(gINI.GetValue("JoystickKeyBindings", "VoiceChat", keyCodeToString(KEY_R)).c_str());
   keyUP[Joystick] = stringToKeyCode(gINI.GetValue("JoystickKeyBindings", "ShipUp", keyCodeToString(KEY_UP)).c_str());
   keyDOWN[Joystick] = stringToKeyCode(gINI.GetValue("JoystickKeyBindings", "ShipDown", keyCodeToString(KEY_DOWN)).c_str());
   keyLEFT[Joystick] = stringToKeyCode(gINI.GetValue("JoystickKeyBindings", "ShipLeft", keyCodeToString(KEY_LEFT)).c_str());
   keyRIGHT[Joystick] = stringToKeyCode(gINI.GetValue("JoystickKeyBindings", "ShipRight", keyCodeToString(KEY_RIGHT)).c_str());
   keySCRBRD[Joystick] = stringToKeyCode(gINI.GetValue("JoystickKeyBindings", "ShowScoreboard", keyCodeToString(BUTTON_5)).c_str());

   // The following key bindings are not user-defineable at the moment, mostly because we want consistency
   // throughout the game, and that would require some real constraints on what keys users could choose.
   //keyHELP = KEY_F1;
   //keyMISSION = KEY_F2;
   //keyOUTGAMECHAT = KEY_F5;
   //keyFPS = KEY_F6;
   //keyDIAG = KEY_F7;
   // These were moved to main.cpp to get them defined before the menus
}

static void writeKeyBindings()
{
   const char *section;

   section = "KeyboardKeyBindings";

   gINI.SetValue(section, "SelWeapon1",      keyCodeToString(keySELWEAP1[Keyboard]));
   gINI.SetValue(section, "SelWeapon2",      keyCodeToString(keySELWEAP2[Keyboard]));
   gINI.SetValue(section, "SelWeapon3",      keyCodeToString(keySELWEAP3[Keyboard]));
   gINI.SetValue(section, "SelNextWeapon",   keyCodeToString(keyADVWEAP[Keyboard]));
   gINI.SetValue(section, "ShowCmdrMap",     keyCodeToString(keyCMDRMAP[Keyboard]));
   gINI.SetValue(section, "TeamChat",        keyCodeToString(keyTEAMCHAT[Keyboard]));
   gINI.SetValue(section, "GlobalChat",      keyCodeToString(keyGLOBCHAT[Keyboard]));
   gINI.SetValue(section, "QuickChat",       keyCodeToString(keyQUICKCHAT[Keyboard]));
   gINI.SetValue(section, "Command",         keyCodeToString(keyCMDCHAT[Keyboard]));
   gINI.SetValue(section, "ShowLoadoutMenu", keyCodeToString(keyLOADOUT[Keyboard]));
   gINI.SetValue(section, "ActivateModule1", keyCodeToString(keyMOD1[Keyboard]));
   gINI.SetValue(section, "ActivateModule2", keyCodeToString(keyMOD2[Keyboard]));
   gINI.SetValue(section, "Fire",            keyCodeToString(keyFIRE[Keyboard]));
   gINI.SetValue(section, "DropItem",        keyCodeToString(keyDROPITEM[Keyboard]));
   gINI.SetValue(section, "VoiceChat",       keyCodeToString(keyTOGVOICE[Keyboard]));
   gINI.SetValue(section, "ShipUp",          keyCodeToString(keyUP[Keyboard]));
   gINI.SetValue(section, "ShipDown",        keyCodeToString(keyDOWN[Keyboard]));
   gINI.SetValue(section, "ShipLeft",        keyCodeToString(keyLEFT[Keyboard]));
   gINI.SetValue(section, "ShipRight",       keyCodeToString(keyRIGHT[Keyboard]));
   gINI.SetValue(section, "ShowScoreboard",  keyCodeToString(keySCRBRD[Keyboard]));

   section = "JoystickKeyBindings";

   gINI.SetValue(section, "SelWeapon1",      keyCodeToString(keySELWEAP1[Joystick]));
   gINI.SetValue(section, "SelWeapon2",      keyCodeToString(keySELWEAP2[Joystick]));
   gINI.SetValue(section, "SelWeapon3",      keyCodeToString(keySELWEAP3[Joystick]));
   gINI.SetValue(section, "SelNextWeapon",   keyCodeToString(keyADVWEAP[Joystick]));
   gINI.SetValue(section, "ShowCmdrMap",     keyCodeToString(keyCMDRMAP[Joystick]));
   gINI.SetValue(section, "TeamChat",        keyCodeToString(keyTEAMCHAT[Joystick]));
   gINI.SetValue(section, "GlobalChat",      keyCodeToString(keyGLOBCHAT[Joystick]));
   gINI.SetValue(section, "QuickChat",       keyCodeToString(keyQUICKCHAT[Joystick]));
   gINI.SetValue(section, "Command",         keyCodeToString(keyCMDCHAT[Joystick]));
   gINI.SetValue(section, "ShowLoadoutMenu", keyCodeToString(keyLOADOUT[Joystick]));
   gINI.SetValue(section, "ActivateModule1", keyCodeToString(keyMOD1[Joystick]));
   gINI.SetValue(section, "ActivateModule2", keyCodeToString(keyMOD2[Joystick]));
   gINI.SetValue(section, "Fire",            keyCodeToString(keyFIRE[Joystick]));
   gINI.SetValue(section, "DropItem",        keyCodeToString(keyDROPITEM[Joystick]));
   gINI.SetValue(section, "VoiceChat",       keyCodeToString(keyTOGVOICE[Joystick]));
   gINI.SetValue(section, "ShipUp",          keyCodeToString(keyUP[Joystick]));
   gINI.SetValue(section, "ShipDown",        keyCodeToString(keyDOWN[Joystick]));
   gINI.SetValue(section, "ShipLeft",        keyCodeToString(keyLEFT[Joystick]));
   gINI.SetValue(section, "ShipRight",       keyCodeToString(keyRIGHT[Joystick]));
   gINI.SetValue(section, "ShowScoreboard",  keyCodeToString(keySCRBRD[Joystick]));
}


/*  INI file looks a little like this:
   [QuickChatMessagesGroup1]
   Key=F
   Button=1
   Caption=Flag

   [QuickChatMessagesGroup1_Message1]
   Key=G
   Button=Button 1
   Caption=Flag Gone!
   Message=Our flag is not in the base!
   MessageType=Team     -or-     MessageType=Global

   == or, a top tiered message might look like this ==

   [QuickChat_Message1]
   Key=A
   Button=Button 1
   Caption=Hello
   MessageType=Hello there!

*/

static void loadQuickChatMessages()
{
   // Add initial node
   QuickChatNode emptynode;
   emptynode.depth = 0;    // This is a beginning or ending node
   emptynode.keyCode = KEY_UNKNOWN;
   emptynode.buttonCode = KEY_UNKNOWN;
   emptynode.teamOnly = false;
   emptynode.caption = "";
   emptynode.msg = "";
   gQuickChatTree.push_back(emptynode);
   emptynode.isMsgItem = false;

   // Read QuickChat messages -- first search for keys matching "QuickChatMessagesGroup123"
   S32 keys = gINI.GetNumKeys();
   Vector<string> groups;

   // Next, read any top-level messages
   Vector<string> messages;
   for(S32 i = 0; i < keys; i++)
   {
      string keyName = gINI.getSectionName(i);
      if(keyName.substr(0, 17) == "QuickChat_Message")   // Found message group
         messages.push_back(keyName);
   }

   messages.sort(alphaSort);

   for(S32 i = messages.size()-1; i >= 0; i--)
   {
      QuickChatNode node;
      node.depth = 1;   // This is a top-level message node
      node.keyCode = stringToKeyCode(gINI.GetValue(messages[i], "Key", "A").c_str());
      node.buttonCode = stringToKeyCode(gINI.GetValue(messages[i], "Button", "Button 1").c_str());
      node.teamOnly = lcase(gINI.GetValue(messages[i], "MessageType", "Team")) == "team";          // lcase for case insensitivity
      node.caption = gINI.GetValue(messages[i], "Caption", "Caption");
      node.msg = gINI.GetValue(messages[i], "Message", "Message");
      node.isMsgItem = true;
      gQuickChatTree.push_back(node);
   }

   for(S32 i = 0; i < keys; i++)
   {
      string keyName = gINI.getSectionName(i);
      if(keyName.substr(0, 22) == "QuickChatMessagesGroup" && keyName.find("_") == string::npos)   // Found message group
         groups.push_back(keyName);
   }

   groups.sort(alphaSort);

   // Now find all the individual message definitions for each key -- match "QuickChatMessagesGroup123_Message456"
   // quickChat render functions were designed to work with the messages sorted in reverse.  Rather than
   // reenigneer those, let's just iterate backwards and leave the render functions alone.

   for(S32 i = groups.size()-1; i >= 0; i--)
   {
      Vector<string> messages;
      for(S32 j = 0; j < keys; j++)
      {
         string keyName = gINI.getSectionName(j);
         if(keyName.substr(0, groups[i].length() + 1) == groups[i] + "_")
            messages.push_back(keyName);
      }

      messages.sort(alphaSort);

      QuickChatNode node;
      node.depth = 1;      // This is a group node
      node.keyCode = stringToKeyCode(gINI.GetValue(groups[i], "Key", "A").c_str());
      node.buttonCode = stringToKeyCode(gINI.GetValue(groups[i], "Button", "Button 1").c_str());
      node.teamOnly = lcase(gINI.GetValue(groups[i], "MessageType", "Team")) == "team";
      node.caption = gINI.GetValue(groups[i], "Caption", "Caption");
      node.msg = "";
      node.isMsgItem = false;
      gQuickChatTree.push_back(node);

      for(S32 j = messages.size()-1; j >= 0; j--)
      {
         node.depth = 2;   // This is a message node
         node.keyCode = stringToKeyCode(gINI.GetValue(messages[j], "Key", "A").c_str());
         node.buttonCode = stringToKeyCode(gINI.GetValue(messages[j], "Button", "Button 1").c_str());
         node.teamOnly = lcase(gINI.GetValue(messages[j], "MessageType", "Team")) == "team";          // lcase for case insensitivity
         node.caption = gINI.GetValue(messages[j], "Caption", "Caption");
         node.msg = gINI.GetValue(messages[j], "Message", "Message");
         node.isMsgItem = true;
         gQuickChatTree.push_back(node);
      }
   }

   // Add final node.  Last verse, same as the first.
   gQuickChatTree.push_back(emptynode);
}


static void writeDefaultQuickChatMessages()
{
   // Are there any QuickChatMessageGroups?  If not, we'll write the defaults.
   S32 keys = gINI.GetNumKeys();

   for(S32 i = 0; i < keys; i++)
   {
      string keyName = gINI.getSectionName(i);
      if(keyName.substr(0, 22) == "QuickChatMessagesGroup" && keyName.find("_") == string::npos)
         return;
   }

   gINI.addSection("QuickChatMessages");
   if(gINI.numSectionComments("QuickChatMessages") == 0)
   {
      gINI.sectionComment("QuickChatMessages", "----------------");
      gINI.sectionComment("QuickChatMessages", " The structure of the QuickChatMessages sections is a bit complicated.  The structure reflects the way the messages are");
      gINI.sectionComment("QuickChatMessages", " displayed in the QuickChat menu, so make sure you are familiar with that before you start modifying these items.");
      gINI.sectionComment("QuickChatMessages", " Messages are grouped, and each group has a Caption (short name shown on screen), a Key (the shortcut key used to select");
      gINI.sectionComment("QuickChatMessages", " the group), and a Button (a shortcut button used when in joystick mode).  If the Button is \"Undefined key\", then that");
      gINI.sectionComment("QuickChatMessages", " item will not be shown in joystick mode, unless the ShowKeyboardKeysInStickMode setting is true.  Groups can be defined ");
      gINI.sectionComment("QuickChatMessages", " in any order, but will be displayed sorted by [section] name.  Groups are designated by the [QuickChatMessagesGroupXXX]");
      gINI.sectionComment("QuickChatMessages", " sections, where XXX is a unique suffix, usually a number.");
      gINI.sectionComment("QuickChatMessages", " ");
      gINI.sectionComment("QuickChatMessages", " Each group can have one or more messages, as specified by the [QuickChatMessagesGroupXXX_MessageYYY] sections, where XXX");
      gINI.sectionComment("QuickChatMessages", " is the unique group suffix, and YYY is a unique message suffix.  Again, messages can be defined in any order, and will");
      gINI.sectionComment("QuickChatMessages", " appear sorted by their [section] name.  Key, Button, and Caption serve the same purposes as in the group definitions.");
      gINI.sectionComment("QuickChatMessages", " Message is the actual message text that is sent, and MessageType should be either \"Team\" or \"Global\", depending on which");
      gINI.sectionComment("QuickChatMessages", " users the message should be sent to.  You can mix Team and Global messages in the same section, but it may be less");
      gINI.sectionComment("QuickChatMessages", " confusing not to do so.");
      gINI.sectionComment("QuickChatMessages", " ");
      gINI.sectionComment("QuickChatMessages", " Messages can also be added to the top-tier of items, by specifying a section like [QuickChat_MessageZZZ].");
      gINI.sectionComment("QuickChatMessages", " ");
      gINI.sectionComment("QuickChatMessages", " Note that no quotes are required around Messages or Captions, and if included, they will be sent as part");
      gINI.sectionComment("QuickChatMessages", " of the message.  Also, if you bullocks things up too badly, simply delete all QuickChatMessage sections,");
      gINI.sectionComment("QuickChatMessages", " and they will be regenerated the next time you run the game (though your modifications will be lost).");
      gINI.sectionComment("QuickChatMessages", "----------------");
   }

   gINI.SetValue("QuickChatMessagesGroup1", "Key", keyCodeToString(KEY_G));
   gINI.SetValue("QuickChatMessagesGroup1", "Button", keyCodeToString(BUTTON_6));
   gINI.SetValue("QuickChatMessagesGroup1", "Caption", "Global");
   gINI.SetValue("QuickChatMessagesGroup1", "MessageType", "Global");

      gINI.SetValue("QuickChatMessagesGroup1_Message1", "Key", keyCodeToString(KEY_A));
      gINI.SetValue("QuickChatMessagesGroup1_Message1", "Button", keyCodeToString(BUTTON_1));
      gINI.SetValue("QuickChatMessagesGroup1_Message1", "MessageType", "Global");
      gINI.SetValue("QuickChatMessagesGroup1_Message1", "Caption", "No Problem");
      gINI.SetValue("QuickChatMessagesGroup1_Message1", "Message", "No Problemo.");

      gINI.SetValue("QuickChatMessagesGroup1_Message2", "Key", keyCodeToString(KEY_T));
      gINI.SetValue("QuickChatMessagesGroup1_Message2", "Button", keyCodeToString(BUTTON_2));
      gINI.SetValue("QuickChatMessagesGroup1_Message2", "MessageType", "Global");
      gINI.SetValue("QuickChatMessagesGroup1_Message2", "Caption", "Thanks");
      gINI.SetValue("QuickChatMessagesGroup1_Message2", "Message", "Thanks.");

      gINI.SetValue("QuickChatMessagesGroup1_Message3", "Key", keyCodeToString(KEY_X));
      gINI.SetValue("QuickChatMessagesGroup1_Message3", "Button", keyCodeToString(KEY_UNKNOWN));
      gINI.SetValue("QuickChatMessagesGroup1_Message3", "MessageType", "Global");
      gINI.SetValue("QuickChatMessagesGroup1_Message3", "Caption", "You idiot!");
      gINI.SetValue("QuickChatMessagesGroup1_Message3", "Message", "You idiot!");

      gINI.SetValue("QuickChatMessagesGroup1_Message4", "Key", keyCodeToString(KEY_E));
      gINI.SetValue("QuickChatMessagesGroup1_Message4", "Button", keyCodeToString(BUTTON_3));
      gINI.SetValue("QuickChatMessagesGroup1_Message4", "MessageType", "Global");
      gINI.SetValue("QuickChatMessagesGroup1_Message4", "Caption", "Duh");
      gINI.SetValue("QuickChatMessagesGroup1_Message4", "Message", "Duh.");

      gINI.SetValue("QuickChatMessagesGroup1_Message5", "Key", keyCodeToString(KEY_C));
      gINI.SetValue("QuickChatMessagesGroup1_Message5", "Button", keyCodeToString(KEY_UNKNOWN));
      gINI.SetValue("QuickChatMessagesGroup1_Message5", "MessageType", "Global");
      gINI.SetValue("QuickChatMessagesGroup1_Message5", "Caption", "Crap");
      gINI.SetValue("QuickChatMessagesGroup1_Message5", "Message", "Ah Crap!");

      gINI.SetValue("QuickChatMessagesGroup1_Message6", "Key", keyCodeToString(KEY_D));
      gINI.SetValue("QuickChatMessagesGroup1_Message6", "Button", keyCodeToString(BUTTON_4));
      gINI.SetValue("QuickChatMessagesGroup1_Message6", "MessageType", "Global");
      gINI.SetValue("QuickChatMessagesGroup1_Message6", "Caption", "Damnit");
      gINI.SetValue("QuickChatMessagesGroup1_Message6", "Message", "Dammit!");

      gINI.SetValue("QuickChatMessagesGroup1_Message7", "Key", keyCodeToString(KEY_S));
      gINI.SetValue("QuickChatMessagesGroup1_Message7", "Button", keyCodeToString(BUTTON_5));
      gINI.SetValue("QuickChatMessagesGroup1_Message7", "MessageType", "Global");
      gINI.SetValue("QuickChatMessagesGroup1_Message7", "Caption", "Shazbot");
      gINI.SetValue("QuickChatMessagesGroup1_Message7", "Message", "Shazbot!");

      gINI.SetValue("QuickChatMessagesGroup1_Message8", "Key", keyCodeToString(KEY_Z));
      gINI.SetValue("QuickChatMessagesGroup1_Message8", "Button", keyCodeToString(BUTTON_6));
      gINI.SetValue("QuickChatMessagesGroup1_Message8", "MessageType", "Global");
      gINI.SetValue("QuickChatMessagesGroup1_Message8", "Caption", "Doh");
      gINI.SetValue("QuickChatMessagesGroup1_Message8", "Message", "Doh!");

   gINI.SetValue("QuickChatMessagesGroup2", "Key", keyCodeToString(KEY_D));
   gINI.SetValue("QuickChatMessagesGroup2", "Button", keyCodeToString(BUTTON_5));
   gINI.SetValue("QuickChatMessagesGroup2", "MessageType", "Team");
   gINI.SetValue("QuickChatMessagesGroup2", "Caption", "Defense");

      gINI.SetValue("QuickChatMessagesGroup2_Message1", "Key", keyCodeToString(KEY_G));
      gINI.SetValue("QuickChatMessagesGroup2_Message1", "Button", keyCodeToString(KEY_UNKNOWN));
      gINI.SetValue("QuickChatMessagesGroup2_Message1", "MessageType", "Team");
      gINI.SetValue("QuickChatMessagesGroup2_Message1", "Caption", "Defend Our Base");
      gINI.SetValue("QuickChatMessagesGroup2_Message1", "Message", "Defend our base.");

      gINI.SetValue("QuickChatMessagesGroup2_Message2", "Key", keyCodeToString(KEY_D));
      gINI.SetValue("QuickChatMessagesGroup2_Message2", "Button", keyCodeToString(BUTTON_1));
      gINI.SetValue("QuickChatMessagesGroup2_Message2", "MessageType", "Team");
      gINI.SetValue("QuickChatMessagesGroup2_Message2", "Caption", "Defending Base");
      gINI.SetValue("QuickChatMessagesGroup2_Message2", "Message", "Defending our base.");

      gINI.SetValue("QuickChatMessagesGroup2_Message3", "Key", keyCodeToString(KEY_Q));
      gINI.SetValue("QuickChatMessagesGroup2_Message3", "Button", keyCodeToString(BUTTON_2));
      gINI.SetValue("QuickChatMessagesGroup2_Message3", "MessageType", "Team");
      gINI.SetValue("QuickChatMessagesGroup2_Message3", "Caption", "Is Base Clear?");
      gINI.SetValue("QuickChatMessagesGroup2_Message3", "Message", "Is our base clear?");

      gINI.SetValue("QuickChatMessagesGroup2_Message4", "Key", keyCodeToString(KEY_C));
      gINI.SetValue("QuickChatMessagesGroup2_Message4", "Button", keyCodeToString(BUTTON_3));
      gINI.SetValue("QuickChatMessagesGroup2_Message4", "MessageType", "Team");
      gINI.SetValue("QuickChatMessagesGroup2_Message4", "Caption", "Base Clear");
      gINI.SetValue("QuickChatMessagesGroup2_Message4", "Message", "Base is secured.");

      gINI.SetValue("QuickChatMessagesGroup2_Message5", "Key", keyCodeToString(KEY_T));
      gINI.SetValue("QuickChatMessagesGroup2_Message5", "Button", keyCodeToString(BUTTON_4));
      gINI.SetValue("QuickChatMessagesGroup2_Message5", "MessageType", "Team");
      gINI.SetValue("QuickChatMessagesGroup2_Message5", "Caption", "Base Taken");
      gINI.SetValue("QuickChatMessagesGroup2_Message5", "Message", "Base is taken.");

      gINI.SetValue("QuickChatMessagesGroup2_Message6", "Key", keyCodeToString(KEY_N));
      gINI.SetValue("QuickChatMessagesGroup2_Message6", "Button", keyCodeToString(BUTTON_5));
      gINI.SetValue("QuickChatMessagesGroup2_Message6", "MessageType", "Team");
      gINI.SetValue("QuickChatMessagesGroup2_Message6", "Caption", "Need More Defense");
      gINI.SetValue("QuickChatMessagesGroup2_Message6", "Message", "We need more defense.");

      gINI.SetValue("QuickChatMessagesGroup2_Message7", "Key", keyCodeToString(KEY_E));
      gINI.SetValue("QuickChatMessagesGroup2_Message7", "Button", keyCodeToString(BUTTON_6));
      gINI.SetValue("QuickChatMessagesGroup2_Message7", "MessageType", "Team");
      gINI.SetValue("QuickChatMessagesGroup2_Message7", "Caption", "Enemy Attacking Base");
      gINI.SetValue("QuickChatMessagesGroup2_Message7", "Message", "The enemy is attacking our base.");

      gINI.SetValue("QuickChatMessagesGroup2_Message8", "Key", keyCodeToString(KEY_A));
      gINI.SetValue("QuickChatMessagesGroup2_Message8", "Button", keyCodeToString(KEY_UNKNOWN));
      gINI.SetValue("QuickChatMessagesGroup2_Message8", "MessageType", "Team");
      gINI.SetValue("QuickChatMessagesGroup2_Message8", "Caption", "Attacked");
      gINI.SetValue("QuickChatMessagesGroup2_Message8", "Message", "We are being attacked.");

   gINI.SetValue("QuickChatMessagesGroup3", "Key", keyCodeToString(KEY_F));
   gINI.SetValue("QuickChatMessagesGroup3", "Button", keyCodeToString(BUTTON_4));
   gINI.SetValue("QuickChatMessagesGroup3", "MessageType", "Team");
   gINI.SetValue("QuickChatMessagesGroup3", "Caption", "Flag");

      gINI.SetValue("QuickChatMessagesGroup3_Message1", "Key", keyCodeToString(KEY_F));
      gINI.SetValue("QuickChatMessagesGroup3_Message1", "Button", keyCodeToString(BUTTON_1));
      gINI.SetValue("QuickChatMessagesGroup3_Message1", "MessageType", "Team");
      gINI.SetValue("QuickChatMessagesGroup3_Message1", "Caption", "Get enemy flag");
      gINI.SetValue("QuickChatMessagesGroup3_Message1", "Message", "Get the enemy flag.");

      gINI.SetValue("QuickChatMessagesGroup3_Message2", "Key", keyCodeToString(KEY_R));
      gINI.SetValue("QuickChatMessagesGroup3_Message2", "Button", keyCodeToString(BUTTON_2));
      gINI.SetValue("QuickChatMessagesGroup3_Message2", "MessageType", "Team");
      gINI.SetValue("QuickChatMessagesGroup3_Message2", "Caption", "Return our flag");
      gINI.SetValue("QuickChatMessagesGroup3_Message2", "Message", "Return our flag to base.");

      gINI.SetValue("QuickChatMessagesGroup3_Message3", "Key", keyCodeToString(KEY_S));
      gINI.SetValue("QuickChatMessagesGroup3_Message3", "Button", keyCodeToString(BUTTON_3));
      gINI.SetValue("QuickChatMessagesGroup3_Message3", "MessageType", "Team");
      gINI.SetValue("QuickChatMessagesGroup3_Message3", "Caption", "Flag secure");
      gINI.SetValue("QuickChatMessagesGroup3_Message3", "Message", "Our flag is secure.");

      gINI.SetValue("QuickChatMessagesGroup3_Message4", "Key", keyCodeToString(KEY_H));
      gINI.SetValue("QuickChatMessagesGroup3_Message4", "Button", keyCodeToString(BUTTON_4));
      gINI.SetValue("QuickChatMessagesGroup3_Message4", "MessageType", "Team");
      gINI.SetValue("QuickChatMessagesGroup3_Message4", "Caption", "Have enemy flag");
      gINI.SetValue("QuickChatMessagesGroup3_Message4", "Message", "I have the enemy flag.");

      gINI.SetValue("QuickChatMessagesGroup3_Message5", "Key", keyCodeToString(KEY_E));
      gINI.SetValue("QuickChatMessagesGroup3_Message5", "Button", keyCodeToString(BUTTON_5));
      gINI.SetValue("QuickChatMessagesGroup3_Message5", "MessageType", "Team");
      gINI.SetValue("QuickChatMessagesGroup3_Message5", "Caption", "Enemy has flag");
      gINI.SetValue("QuickChatMessagesGroup3_Message5", "Message", "The enemy has our flag!");

      gINI.SetValue("QuickChatMessagesGroup3_Message6", "Key", keyCodeToString(KEY_G));
      gINI.SetValue("QuickChatMessagesGroup3_Message6", "Button", keyCodeToString(BUTTON_6));
      gINI.SetValue("QuickChatMessagesGroup3_Message6", "MessageType", "Team");
      gINI.SetValue("QuickChatMessagesGroup3_Message6", "Caption", "Flag gone");
      gINI.SetValue("QuickChatMessagesGroup3_Message6", "Message", "Our flag is not in the base!");

   gINI.SetValue("QuickChatMessagesGroup4", "Key", keyCodeToString(KEY_S));
   gINI.SetValue("QuickChatMessagesGroup4", "Button", keyCodeToString(KEY_UNKNOWN));
   gINI.SetValue("QuickChatMessagesGroup4", "MessageType", "Team");
   gINI.SetValue("QuickChatMessagesGroup4", "Caption", "Incoming Enemies - Direction");

      gINI.SetValue("QuickChatMessagesGroup4_Message1", "Key", keyCodeToString(KEY_S));
      gINI.SetValue("QuickChatMessagesGroup4_Message1", "Button", keyCodeToString(KEY_UNKNOWN));
      gINI.SetValue("QuickChatMessagesGroup4_Message1", "MessageType", "Team");
      gINI.SetValue("QuickChatMessagesGroup4_Message1", "Caption", "Incoming South");
      gINI.SetValue("QuickChatMessagesGroup4_Message1", "Message", "*** INCOMING SOUTH ***");

      gINI.SetValue("QuickChatMessagesGroup4_Message2", "Key", keyCodeToString(KEY_E));
      gINI.SetValue("QuickChatMessagesGroup4_Message2", "Button", keyCodeToString(KEY_UNKNOWN));
      gINI.SetValue("QuickChatMessagesGroup4_Message2", "MessageType", "Team");
      gINI.SetValue("QuickChatMessagesGroup4_Message2", "Caption", "Incoming East");
      gINI.SetValue("QuickChatMessagesGroup4_Message2", "Message", "*** INCOMING EAST  ***");

      gINI.SetValue("QuickChatMessagesGroup4_Message3", "Key", keyCodeToString(KEY_W));
      gINI.SetValue("QuickChatMessagesGroup4_Message3", "Button", keyCodeToString(KEY_UNKNOWN));
      gINI.SetValue("QuickChatMessagesGroup4_Message3", "MessageType", "Team");
      gINI.SetValue("QuickChatMessagesGroup4_Message3", "Caption", "Incoming West");
      gINI.SetValue("QuickChatMessagesGroup4_Message3", "Message", "*** INCOMING WEST  ***");

      gINI.SetValue("QuickChatMessagesGroup4_Message4", "Key", keyCodeToString(KEY_N));
      gINI.SetValue("QuickChatMessagesGroup4_Message4", "Button", keyCodeToString(KEY_UNKNOWN));
      gINI.SetValue("QuickChatMessagesGroup4_Message4", "MessageType", "Team");
      gINI.SetValue("QuickChatMessagesGroup4_Message4", "Caption", "Incoming North");
      gINI.SetValue("QuickChatMessagesGroup4_Message4", "Message", "*** INCOMING NORTH ***");

      gINI.SetValue("QuickChatMessagesGroup4_Message5", "Key", keyCodeToString(KEY_V));
      gINI.SetValue("QuickChatMessagesGroup4_Message5", "Button", keyCodeToString(KEY_UNKNOWN));
      gINI.SetValue("QuickChatMessagesGroup4_Message5", "MessageType", "Team");
      gINI.SetValue("QuickChatMessagesGroup4_Message5", "Caption", "Incoming Enemies");
      gINI.SetValue("QuickChatMessagesGroup4_Message5", "Message", "Incoming enemies!");

   gINI.SetValue("QuickChatMessagesGroup5", "Key", keyCodeToString(KEY_V));
   gINI.SetValue("QuickChatMessagesGroup5", "Button", keyCodeToString(BUTTON_3));
   gINI.SetValue("QuickChatMessagesGroup5", "MessageType", "Team");
   gINI.SetValue("QuickChatMessagesGroup5", "Caption", "Quick");

      gINI.SetValue("QuickChatMessagesGroup5_Message1", "Key", keyCodeToString(KEY_J));
      gINI.SetValue("QuickChatMessagesGroup5_Message1", "Button", keyCodeToString(KEY_UNKNOWN));
      gINI.SetValue("QuickChatMessagesGroup5_Message1", "MessageType", "Team");
      gINI.SetValue("QuickChatMessagesGroup5_Message1", "Caption", "Capture the objective");
      gINI.SetValue("QuickChatMessagesGroup5_Message1", "Message", "Capture the objective.");

      gINI.SetValue("QuickChatMessagesGroup5_Message2", "Key", keyCodeToString(KEY_O));
      gINI.SetValue("QuickChatMessagesGroup5_Message2", "Button", keyCodeToString(KEY_UNKNOWN));
      gINI.SetValue("QuickChatMessagesGroup5_Message2", "MessageType", "Team");
      gINI.SetValue("QuickChatMessagesGroup5_Message2", "Caption", "Go on the offensive");
      gINI.SetValue("QuickChatMessagesGroup5_Message2", "Message", "Go on the offensive.");

      gINI.SetValue("QuickChatMessagesGroup5_Message3", "Key", keyCodeToString(KEY_A));
      gINI.SetValue("QuickChatMessagesGroup5_Message3", "Button", keyCodeToString(BUTTON_1));
      gINI.SetValue("QuickChatMessagesGroup5_Message3", "MessageType", "Team");
      gINI.SetValue("QuickChatMessagesGroup5_Message3", "Caption", "Attack!");
      gINI.SetValue("QuickChatMessagesGroup5_Message3", "Message", "Attack!");

      gINI.SetValue("QuickChatMessagesGroup5_Message4", "Key", keyCodeToString(KEY_W));
      gINI.SetValue("QuickChatMessagesGroup5_Message4", "Button", keyCodeToString(BUTTON_2));
      gINI.SetValue("QuickChatMessagesGroup5_Message4", "MessageType", "Team");
      gINI.SetValue("QuickChatMessagesGroup5_Message4", "Caption", "Wait for signal");
      gINI.SetValue("QuickChatMessagesGroup5_Message4", "Message", "Wait for my signal to attack.");

      gINI.SetValue("QuickChatMessagesGroup5_Message5", "Key", keyCodeToString(KEY_V));
      gINI.SetValue("QuickChatMessagesGroup5_Message5", "Button", keyCodeToString(BUTTON_3));
      gINI.SetValue("QuickChatMessagesGroup5_Message5", "MessageType", "Team");
      gINI.SetValue("QuickChatMessagesGroup5_Message5", "Caption", "Help!");
      gINI.SetValue("QuickChatMessagesGroup5_Message5", "Message", "Help!");

      gINI.SetValue("QuickChatMessagesGroup5_Message6", "Key", keyCodeToString(KEY_E));
      gINI.SetValue("QuickChatMessagesGroup5_Message6", "Button", keyCodeToString(BUTTON_4));
      gINI.SetValue("QuickChatMessagesGroup5_Message6", "MessageType", "Team");
      gINI.SetValue("QuickChatMessagesGroup5_Message6", "Caption", "Regroup");
      gINI.SetValue("QuickChatMessagesGroup5_Message6", "Message", "Regroup.");

      gINI.SetValue("QuickChatMessagesGroup5_Message7", "Key", keyCodeToString(KEY_G));
      gINI.SetValue("QuickChatMessagesGroup5_Message7", "Button", keyCodeToString(BUTTON_5));
      gINI.SetValue("QuickChatMessagesGroup5_Message7", "MessageType", "Team");
      gINI.SetValue("QuickChatMessagesGroup5_Message7", "Caption", "Going offense");
      gINI.SetValue("QuickChatMessagesGroup5_Message7", "Message", "Going offense.");

      gINI.SetValue("QuickChatMessagesGroup5_Message8", "Key", keyCodeToString(KEY_Z));
      gINI.SetValue("QuickChatMessagesGroup5_Message8", "Button", keyCodeToString(BUTTON_6));
      gINI.SetValue("QuickChatMessagesGroup5_Message8", "MessageType", "Team");
      gINI.SetValue("QuickChatMessagesGroup5_Message8", "Caption", "Move out");
      gINI.SetValue("QuickChatMessagesGroup5_Message8", "Message", "Move out.");

   gINI.SetValue("QuickChatMessagesGroup6", "Key", keyCodeToString(KEY_R));
   gINI.SetValue("QuickChatMessagesGroup6", "Button", keyCodeToString(BUTTON_2));
   gINI.SetValue("QuickChatMessagesGroup6", "MessageType", "Team");
   gINI.SetValue("QuickChatMessagesGroup6", "Caption", "Reponses");

      gINI.SetValue("QuickChatMessagesGroup6_Message1", "Key", keyCodeToString(KEY_A));
      gINI.SetValue("QuickChatMessagesGroup6_Message1", "Button", keyCodeToString(BUTTON_1));
      gINI.SetValue("QuickChatMessagesGroup6_Message1", "MessageType", "Team");
      gINI.SetValue("QuickChatMessagesGroup6_Message1", "Caption", "Acknowledge");
      gINI.SetValue("QuickChatMessagesGroup6_Message1", "Message", "Acknowledged.");

      gINI.SetValue("QuickChatMessagesGroup6_Message2", "Key", keyCodeToString(KEY_N));
      gINI.SetValue("QuickChatMessagesGroup6_Message2", "Button", keyCodeToString(BUTTON_2));
      gINI.SetValue("QuickChatMessagesGroup6_Message2", "MessageType", "Team");
      gINI.SetValue("QuickChatMessagesGroup6_Message2", "Caption", "No");
      gINI.SetValue("QuickChatMessagesGroup6_Message2", "Message", "No.");

      gINI.SetValue("QuickChatMessagesGroup6_Message3", "Key", keyCodeToString(KEY_Y));
      gINI.SetValue("QuickChatMessagesGroup6_Message3", "Button", keyCodeToString(BUTTON_3));
      gINI.SetValue("QuickChatMessagesGroup6_Message3", "MessageType", "Team");
      gINI.SetValue("QuickChatMessagesGroup6_Message3", "Caption", "Yes");
      gINI.SetValue("QuickChatMessagesGroup6_Message3", "Message", "Yes.");

      gINI.SetValue("QuickChatMessagesGroup6_Message4", "Key", keyCodeToString(KEY_S));
      gINI.SetValue("QuickChatMessagesGroup6_Message4", "Button", keyCodeToString(BUTTON_4));
      gINI.SetValue("QuickChatMessagesGroup6_Message4", "MessageType", "Team");
      gINI.SetValue("QuickChatMessagesGroup6_Message4", "Caption", "Sorry");
      gINI.SetValue("QuickChatMessagesGroup6_Message4", "Message", "Sorry.");

      gINI.SetValue("QuickChatMessagesGroup6_Message5", "Key", keyCodeToString(KEY_T));
      gINI.SetValue("QuickChatMessagesGroup6_Message5", "Button", keyCodeToString(BUTTON_5));
      gINI.SetValue("QuickChatMessagesGroup6_Message5", "MessageType", "Team");
      gINI.SetValue("QuickChatMessagesGroup6_Message5", "Caption", "Thanks");
      gINI.SetValue("QuickChatMessagesGroup6_Message5", "Message", "Thanks.");

      gINI.SetValue("QuickChatMessagesGroup6_Message6", "Key", keyCodeToString(KEY_D));
      gINI.SetValue("QuickChatMessagesGroup6_Message6", "Button", keyCodeToString(BUTTON_6));
      gINI.SetValue("QuickChatMessagesGroup6_Message6", "MessageType", "Team");
      gINI.SetValue("QuickChatMessagesGroup6_Message6", "Caption", "Don't know");
      gINI.SetValue("QuickChatMessagesGroup6_Message6", "Message", "I don't know.");

   gINI.SetValue("QuickChatMessagesGroup7", "Key", keyCodeToString(KEY_T));
   gINI.SetValue("QuickChatMessagesGroup7", "Button", keyCodeToString(BUTTON_1));
   gINI.SetValue("QuickChatMessagesGroup7", "MessageType", "Global");
   gINI.SetValue("QuickChatMessagesGroup7", "Caption", "Taunts");

      gINI.SetValue("QuickChatMessagesGroup7_Message1", "Key", keyCodeToString(KEY_R));
      gINI.SetValue("QuickChatMessagesGroup7_Message1", "Button", keyCodeToString(KEY_UNKNOWN));
      gINI.SetValue("QuickChatMessagesGroup7_Message1", "MessageType", "Global");
      gINI.SetValue("QuickChatMessagesGroup7_Message1", "Caption", "Rawr");
      gINI.SetValue("QuickChatMessagesGroup7_Message1", "Message", "RAWR!");

      gINI.SetValue("QuickChatMessagesGroup7_Message2", "Key", keyCodeToString(KEY_C));
      gINI.SetValue("QuickChatMessagesGroup7_Message2", "Button", keyCodeToString(BUTTON_1));
      gINI.SetValue("QuickChatMessagesGroup7_Message2", "MessageType", "Global");
      gINI.SetValue("QuickChatMessagesGroup7_Message2", "Caption", "Come get some!");
      gINI.SetValue("QuickChatMessagesGroup7_Message2", "Message", "Come get some!");

      gINI.SetValue("QuickChatMessagesGroup7_Message3", "Key", keyCodeToString(KEY_D));
      gINI.SetValue("QuickChatMessagesGroup7_Message3", "Button", keyCodeToString(BUTTON_2));
      gINI.SetValue("QuickChatMessagesGroup7_Message3", "MessageType", "Global");
      gINI.SetValue("QuickChatMessagesGroup7_Message3", "Caption", "Dance!");
      gINI.SetValue("QuickChatMessagesGroup7_Message3", "Message", "Dance!");

      gINI.SetValue("QuickChatMessagesGroup7_Message4", "Key", keyCodeToString(KEY_X));
      gINI.SetValue("QuickChatMessagesGroup7_Message4", "Button", keyCodeToString(BUTTON_3));
      gINI.SetValue("QuickChatMessagesGroup7_Message4", "MessageType", "Global");
      gINI.SetValue("QuickChatMessagesGroup7_Message4", "Caption", "Missed me!");
      gINI.SetValue("QuickChatMessagesGroup7_Message4", "Message", "Missed me!");

      gINI.SetValue("QuickChatMessagesGroup7_Message5", "Key", keyCodeToString(KEY_W));
      gINI.SetValue("QuickChatMessagesGroup7_Message5", "Button", keyCodeToString(BUTTON_4));
      gINI.SetValue("QuickChatMessagesGroup7_Message5", "MessageType", "Global");
      gINI.SetValue("QuickChatMessagesGroup7_Message5", "Caption", "I've had worse...");
      gINI.SetValue("QuickChatMessagesGroup7_Message5", "Message", "I've had worse...");

      gINI.SetValue("QuickChatMessagesGroup7_Message6", "Key", keyCodeToString(KEY_Q));
      gINI.SetValue("QuickChatMessagesGroup7_Message6", "Button", keyCodeToString(BUTTON_5));
      gINI.SetValue("QuickChatMessagesGroup7_Message6", "MessageType", "Global");
      gINI.SetValue("QuickChatMessagesGroup7_Message6", "Caption", "How'd THAT feel?");
      gINI.SetValue("QuickChatMessagesGroup7_Message6", "Message", "How'd THAT feel?");

      gINI.SetValue("QuickChatMessagesGroup7_Message7", "Key", keyCodeToString(KEY_E));
      gINI.SetValue("QuickChatMessagesGroup7_Message7", "Button", keyCodeToString(BUTTON_6));
      gINI.SetValue("QuickChatMessagesGroup7_Message7", "MessageType", "Global");
      gINI.SetValue("QuickChatMessagesGroup7_Message7", "Caption", "Yoohoo!");
      gINI.SetValue("QuickChatMessagesGroup7_Message7", "Message", "Yoohoo!");
}


// Option default values are stored here, in the 3rd prarm of the GetValue call
// This is only called once, during initial initialization
void loadSettingsFromINI()
{
   gINI.ReadFile();        // Read the INI file  (journaling of read lines happens within)

   loadSoundSettings();
   loadEffectsSettings();
   loadGeneralSettings();
   loadHostConfiguration();
   loadUpdaterSettings();
   loadDiagnostics();

   loadTestSettings();

   loadKeyBindings();
   loadForeignServerInfo();    // Info about other servers
   loadLevels();               // Read levels, if there are any
   loadLevelSkipList();        // Read level skipList, if there are any

   loadQuickChatMessages();

   saveSettingsToINI();      // Save to fill in any missing settings
}


static void writeDiagnostics()
{
   const char *section = "Diagnostics";
   gINI.addSection(section);

   if (gINI.numSectionComments(section) == 0)
   {
      gINI.sectionComment(section, "----------------");
      gINI.sectionComment(section, " Diagnostic entries can be used to enable or disable particular actions for debugging purposes.");
      gINI.sectionComment(section, " You probably can't use any of these settings to enhance your gameplay experience!");
      gINI.sectionComment(section, " DumpKeys - Enable this to dump raw input to the screen (Yes/No)");
      gINI.sectionComment(section, " LogConnectionProtocol - Log ConnectionProtocol events (Yes/No)");
      gINI.sectionComment(section, " LogNetConnection - Log NetConnectionEvents (Yes/No)");
      gINI.sectionComment(section, " LogEventConnection - Log EventConnection events (Yes/No)");
      gINI.sectionComment(section, " LogGhostConnection - Log GhostConnection events (Yes/No)");
      gINI.sectionComment(section, " LogNetInterface - Log NetInterface events (Yes/No)");
      gINI.sectionComment(section, " LogPlatform - Log Platform events (Yes/No)");
      gINI.sectionComment(section, " LogNetBase - Log NetBase events (Yes/No)");
      gINI.sectionComment(section, " LogUDP - Log UDP events (Yes/No)");

      gINI.sectionComment(section, " LogFatalError - Log fatal errors; should be left on (Yes/No)");
      gINI.sectionComment(section, " LogError - Log serious errors; should be left on (Yes/No)");
      gINI.sectionComment(section, " LogWarning - Log less serious errors (Yes/No)");
      gINI.sectionComment(section, " LogConnection - High level logging connections with remote machines (Yes/No)");
      gINI.sectionComment(section, " LogLevelLoaded - Write a log entry when a level is loaded (Yes/No)");
      gINI.sectionComment(section, " LogLuaObjectLifecycle - Creation and destruciton of lua objects (Yes/No)");
      gINI.sectionComment(section, " LuaLevelGenerator - Messages from the LuaLevelGenerator (Yes/No)");
      gINI.sectionComment(section, " LuaBotMessage - Message from a bot (Yes/No)");
      gINI.sectionComment(section, " ServerFilter - For logging messages specific to hosting games (Yes/No)");
      gINI.sectionComment(section, "                (Note: these messages will go to bitfighter_server.log regardless of this setting) ");
      gINI.sectionComment(section, "----------------");
   }

   gINI.setValueYN(section, "DumpKeys", gIniSettings.diagnosticKeyDumpMode);
   gINI.setValueYN(section, "LogConnectionProtocol", gIniSettings.logConnectionProtocol);
   gINI.setValueYN(section, "LogNetConnection",      gIniSettings.logNetConnection);
   gINI.setValueYN(section, "LogEventConnection",    gIniSettings.logEventConnection);
   gINI.setValueYN(section, "LogGhostConnection",    gIniSettings.logGhostConnection);
   gINI.setValueYN(section, "LogNetInterface",       gIniSettings.logNetInterface);
   gINI.setValueYN(section, "LogPlatform",           gIniSettings.logPlatform);
   gINI.setValueYN(section, "LogNetBase",            gIniSettings.logNetBase);
   gINI.setValueYN(section, "LogUDP",                gIniSettings.logUDP);

   gINI.setValueYN(section, "LogFatalError",         gIniSettings.logFatalError);
   gINI.setValueYN(section, "LogError",              gIniSettings.logError);
   gINI.setValueYN(section, "LogWarning",            gIniSettings.logWarning);
   gINI.setValueYN(section, "LogConnection",         gIniSettings.logConnection);
   gINI.setValueYN(section, "LogLevelLoaded",        gIniSettings.logLevelLoaded);
   gINI.setValueYN(section, "LogLuaObjectLifecycle", gIniSettings.logLuaObjectLifecycle);
   gINI.setValueYN(section, "LuaLevelGenerator",     gIniSettings.luaLevelGenerator);
   gINI.setValueYN(section, "LuaBotMessage",         gIniSettings.luaBotMessage);
   gINI.setValueYN(section, "ServerFilter",          gIniSettings.serverFilter);
}


static void writeEffects()
{
   const char *section = "Effects";
   gINI.addSection(section);

   if (gINI.numSectionComments(section) == 0)
   {
      gINI.sectionComment(section, "----------------");
      gINI.sectionComment(section, " Various visual effects");
      gINI.sectionComment(section, " StarsInDistance - Yes gives the game a floating, 3-D effect.  No gives the flat 'classic zap' mode.");
      gINI.sectionComment(section, " LineSmoothing - Yes activates anti-aliased rendering.  This may be a little slower on some machines.");
      gINI.sectionComment(section, "----------------");
   }

   gINI.setValueYN(section, "StarsInDistance", gIniSettings.starsInDistance);
   gINI.setValueYN(section, "LineSmoothing",   gIniSettings.useLineSmoothing);
}

static void writeSounds()
{
   gINI.addSection("Sounds");

   if (gINI.numSectionComments("Sounds") == 0)
   {
      gINI.sectionComment("Sounds", "----------------");
      gINI.sectionComment("Sounds", " Sound settings");
      gINI.sectionComment("Sounds", " EffectsVolume - Volume of sound effects from 0 (mute) to 10 (full bore)");
      gINI.sectionComment("Sounds", " MusicVolume - Volume of sound effects from 0 (mute) to 10 (full bore)");
      gINI.sectionComment("Sounds", " VoiceChatVolume - Volume of incoming voice chat messages from 0 (mute) to 10 (full bore)");
      gINI.sectionComment("Sounds", " SFXSet - Select which set of sounds you want: Classic or Modern");
      gINI.sectionComment("Sounds", "----------------");
   }

   gINI.SetValueI("Sounds", "EffectsVolume", (S32) (gIniSettings.sfxVolLevel * 10));
   gINI.SetValueI("Sounds", "MusicVolume",   (S32) (gIniSettings.musicVolLevel * 10));
   gINI.SetValueI("Sounds", "VoiceChatVolume",   (S32) (gIniSettings.voiceChatVolLevel * 10));

   gINI.SetValue("Sounds", "SFXSet", gIniSettings.sfxSet == sfxClassicSet ? "Classic" : "Modern");
}


void saveWindowMode()
{
   gINI.SetValue("Settings",  "WindowMode", displayModeToString(gIniSettings.displayMode));
}


void saveWindowPosition(S32 x, S32 y)
{
   gINI.SetValueI("Settings", "WindowXPos", x);
   gINI.SetValueI("Settings", "WindowYPos", y);
}


static void writeSettings()
{
   const char *section = "Settings";
   gINI.addSection(section);

   if (gINI.numSectionComments(section) == 0)
   {
      gINI.sectionComment(section, "----------------");
      gINI.sectionComment(section, " Settings entries contain a number of different options");
      gINI.sectionComment(section, " WindowMode - Fullscreen, Fullscreen-Stretch or Window");
      gINI.sectionComment(section, " WindowXPos, WindowYPos - Position of window in window mode (will overwritten if you move your window)");
      gINI.sectionComment(section, " WindowScalingFactor - Used to set size of window.  1.0 = 800x600. Best to let the program manage this setting.");
      gINI.sectionComment(section, " VoiceEcho - Play echo when recording a voice message? Yes/No");
      gINI.sectionComment(section, " ControlMode - Use Relative or Absolute controls (Relative means left is ship's left, Absolute means left is screen left)");
      gINI.sectionComment(section, " LoadoutIndicators - Display indicators showing current weapon?  Yes/No");
      gINI.sectionComment(section, " VerboseHelpMessages - Display additional on-screen messages while learning the game?  Yes/No");
      gINI.sectionComment(section, " ShowKeyboardKeysInStickMode - If you are using a joystick, also show keyboard shortcuts in Loadout and QuickChat menus");
      gINI.sectionComment(section, " JoystickType - Type of joystick to use if auto-detect doesn't recognize your controller");
      gINI.sectionComment(section, " MasterServerAddress - Address of master server, in form: IP:67.18.11.66:25955 or IP:myMaster.org:25955");
      gINI.sectionComment(section, " DefaultName - Name that will be used if user hits <enter> on name entry screen without entering one");
      gINI.sectionComment(section, " Nickname - Specify your nickname to bypass the name entry screen altogether");
      gINI.sectionComment(section, " Password - Password to use if your nickname has been reserved in the forums");
      gINI.sectionComment(section, " EnableExperimentalAimMode - Use experimental aiming system (works only with controller) Yes/No");
      gINI.sectionComment(section, " LastName - Name user entered when game last run (may be overwritten if you enter a different name on startup screen)");
      gINI.sectionComment(section, " LastPassword - Password user entered when game last run (may be overwritten if you enter a different pw on startup screen)");
      gINI.sectionComment(section, " LastEditorName - Last edited file name");
      gINI.sectionComment(section, " MinClientDelay -  in millisecs, lower use more CPU, higher will lose performance/reduce FPS (delay 10 = max 100 FPS) (using 1000 / delay = fps)");
      gINI.sectionComment(section, " LineWidth - default 2, width in pixels, use /LineWidth in game");
      gINI.sectionComment(section, "----------------");
   }
   saveWindowMode();
   saveWindowPosition(gIniSettings.winXPos, gIniSettings.winYPos);

   gINI.SetValueF(section, "WindowScalingFactor", gIniSettings.winSizeFact);
   gINI.setValueYN(section, "VoiceEcho", gIniSettings.echoVoice );
   gINI.SetValue(section,  "ControlMode", (gIniSettings.controlsRelative ? "Relative" : "Absolute"));

   // inputMode is not saved, but rather determined at runtime by whether a joystick is attached

   gINI.setValueYN(section, "LoadoutIndicators", gIniSettings.showWeaponIndicators);
   gINI.setValueYN(section, "VerboseHelpMessages", gIniSettings.verboseHelpMessages);
   gINI.setValueYN(section, "ShowKeyboardKeysInStickMode", gIniSettings.showKeyboardKeys);

   gINI.SetValue  (section, "JoystickType", joystickTypeToString(gIniSettings.joystickType));
   gINI.SetValue  (section, "MasterServerAddress", gIniSettings.masterAddress);
   gINI.SetValue  (section, "DefaultName", gIniSettings.defaultName);
   gINI.SetValue  (section, "LastName", gIniSettings.lastName);
   gINI.SetValue  (section, "LastPassword", gIniSettings.lastPassword);
   gINI.SetValue  (section, "LastEditorName", gIniSettings.lastEditorName);

   gINI.SetValue(section, "EnableExperimentalAimMode", (gIniSettings.enableExperimentalAimMode ? "Yes" : "No"));
   if(gIniSettings.minSleepTimeClient < 100)    // Don't save if too high
      gINI.SetValueI(section, "MinClientDelay", gIniSettings.minSleepTimeClient);  
   //gINI.SetValueF(section,"LineWidth",gDefaultLineWidth,true);     //Allow load, but not save, a user can screw up with /LineWidth command
}


static void writeUpdater()
{
   gINI.addSection("Updater");

   if(gINI.numSectionComments("Updater") == 0)
   {
      gINI.sectionComment("Updater", "----------------");
      gINI.sectionComment("Updater", " The Updater section contains entries that control how game updates are handled");
      gINI.sectionComment("Updater", " UseUpdater - Enable or disable process that installs updates (WINDOWS ONLY)");
      gINI.sectionComment("Updater", "----------------");

      gINI.SetValueB("Updater", "UseUpdater", gIniSettings.useUpdater, true);
   }
}

// TEST!!
// Does this macro def make it easier to read the code?
#define addComment(comment) gINI.sectionComment(section, comment);

static void writeHost()
{
   const char *section = "Host";
   gINI.addSection(section);

   if(gINI.numSectionComments(section) == 0)
   {
      addComment("----------------");
      addComment(" The Host section contains entries that configure the game when you are hosting");
      addComment(" ServerName - The name others will see when they are browsing for servers (max 20 chars)");
      addComment(" ServerAddress - The address of your server, e.g. IP:localhost:1234 or IP:54.35.110.99:8000 or IP:bitfighter.org:8888 (leave blank to let the system decide)");
      addComment(" ServerDescription - A one line description of your server.  Please include nickname and physical location!");
      addComment(" ServerPassword - You can require players to use a password to play on your server.  Leave blank to grant access to all.");
      addComment(" AdminPassword - Use this password to manage players & change levels on your server.");
      addComment(" LevelChangePassword - Use this password to change levels on your server.  Leave blank to grant access to all.");
      addComment(" LevelDir - Specify where level files are stored; can be overridden on command line with -leveldir param.");
      addComment(" MaxPlayers - The max number of players that can play on your server.");
      addComment(" AlertsVolume - Volume of audio alerts when players join or leave game from 0 (mute) to 10 (full bore).");
      addComment(" MinDedicatedDelay - (Dedicated only) default 10, in milliseconds, lower use more CPU, higher may increase lag.");
      addComment(" AllowGetMap - When getmap is allowed, anyone can download the current level using the /getmap command.");
      addComment(" AllowDataConnections - When data connections are allowed, anyone with the admin password can upload or download levels, bots, or");
      addComment("                        levelGen scripts.  This feature is probably insecure, and should be DISABLED unless you require the functionality.");
      addComment("----------------");
   }
   gINI.SetValue  (section, "ServerName", gIniSettings.hostname);
   gINI.SetValue  (section, "ServerAddress", gIniSettings.hostaddr);
   gINI.SetValue  (section, "ServerDescription", gIniSettings.hostdescr);
   gINI.SetValue  (section, "ServerPassword", gIniSettings.serverPassword);
   gINI.SetValue  (section, "AdminPassword", gIniSettings.adminPassword);
   gINI.SetValue  (section, "LevelChangePassword", gIniSettings.levelChangePassword);
   gINI.SetValue  (section, "LevelDir", gIniSettings.levelDir);
   gINI.SetValueI (section, "MaxPlayers", gIniSettings.maxplayers);
   gINI.SetValueI (section, "AlertsVolume", (S32) (gIniSettings.alertsVolLevel * 10));
   gINI.setValueYN(section, "AllowGetMap", gIniSettings.allowGetMap);
   gINI.setValueYN(section, "AllowDataConnections", gIniSettings.allowDataConnections);
   gINI.SetValueI (section, "MinDedicatedDelay", gIniSettings.minSleepTimeDedicatedServer);
}


static void writeLevels()
{
   // If there is no Levels key, we'll add it here.  Otherwise, we'll do nothing so as not to clobber an existing value
   // We'll write the default level list (which may have been overridden by the cmd line) because there are no levels in the INI
   if(gINI.findSection("Levels") == gINI.noID)    // Section doesn't exist... let's write one
      gINI.addSection("Levels");              

   if(gINI.numSectionComments("Levels") == 0)
   {
      gINI.sectionComment("Levels", "----------------");
      gINI.sectionComment("Levels", " All levels in this section will be loaded when you host a game in Server mode.");
      gINI.sectionComment("Levels", " You can call the level keys anything you want (within reason), and the levels will be sorted");
      gINI.sectionComment("Levels", " by key name and will appear in that order, regardless of the order the items are listed in.");
      gINI.sectionComment("Levels", " Example:");
      gINI.sectionComment("Levels", " Level1=ctf.level");
      gINI.sectionComment("Levels", " Level2=zonecontrol.level");
      gINI.sectionComment("Levels", " ... etc ...");
      gINI.sectionComment("Levels", "This list can be overidden on the command line with the -leveldir, -rootdatadir, or -levels parameters.");
      gINI.sectionComment("Levels", "----------------");

      /*
      char levelName[256];
      for(S32 i = 0; i < gLevelList.size(); i++)
      {
         dSprintf(levelName, 255, "Level%d", i);
         gINI.SetValue("Levels", string(levelName), gLevelList[i].getString(), true);
      }
      */
   }
}



static void writeTesting()
{
   gINI.addSection("Testing");
   if (gINI.numSectionComments("Testing") == 0)
   {
      gINI.sectionComment("Testing", "----------------");
      gINI.sectionComment("Testing", " These settings are here to enable/disable certain items for testing.  They are by their nature");
      gINI.sectionComment("Testing", " short lived, and may well be removed in the next version of Bitfighter.");
      gINI.sectionComment("Testing", " BurstGraphics - Select which graphic to use for bursts (1-5)");
      gINI.sectionComment("Testing", "----------------");
   }
   gINI.SetValueI("Testing", "BurstGraphics",  (S32) (gIniSettings.burstGraphicsMode), true);
}


static void writePasswordSection_helper(string section)
{
   gINI.addSection(section);
   if (gINI.numSectionComments(section) == 0)
   {
      gINI.sectionComment(section, "----------------");
      gINI.sectionComment(section, " This section holds passwords you've entered to gain access to various servers.");
      gINI.sectionComment(section, "----------------");
   }
}

static void writePasswordSection()
{
   writePasswordSection_helper("SavedLevelChangePasswords");
   writePasswordSection_helper("SavedAdminPasswords");
   writePasswordSection_helper("SavedServerPasswords");
}


static void writeINIHeader()
{
   if(!gINI.NumHeaderComments())
   {
      gINI.HeaderComment("Bitfighter configuration file");
      gINI.HeaderComment("=============================");
      gINI.HeaderComment(" This file is intended to be user-editable, but some settings here may be overwritten by the game.");
      gINI.HeaderComment(" If you specify any cmd line parameters that conflict with these settings, the cmd line options will be used.");
      gINI.HeaderComment(" First, some basic terminology:");
      gINI.HeaderComment(" [section]");
      gINI.HeaderComment(" key=value");
      gINI.HeaderComment("");
   }
}


// Save more commonly altered settings first to make them easier to find
void saveSettingsToINI()
{
   writeINIHeader();

   writeHost();
   writeForeignServerInfo();
   writeConnectionsInfo();
   writeEffects();
   writeSounds();
   writeSettings();
   writeDiagnostics();
   writeLevels();
   writeSkipList();
   writeUpdater();
   writeTesting();
   writePasswordSection();
   writeKeyBindings();
   
   writeDefaultQuickChatMessages();    // Does nothing if there are already chat messages in the INI

   gINI.WriteFile();
}


void writeSkipList()
{
   // If there is no LevelSkipList key, we'll add it here.  Otherwise, we'll do nothing so as not to clobber an existing value
   // We'll write our current skip list (which may have been specified via remote server management tools)

   gINI.deleteSection("LevelSkipList");   // Delete all current entries to prevent user renumberings to be corrected from tripping us up
                                          // This may the unfortunate side-effect of pushing this section to the bottom of the INI file

   gINI.addSection("LevelSkipList");      // Create the key, then provide some comments for documentation purposes

   gINI.sectionComment("LevelSkipList", "----------------");
   gINI.sectionComment("LevelSkipList", " Levels listed here will be skipped and will NOT be loaded, even when they are specified in");
   gINI.sectionComment("LevelSkipList", " another section or on the command line.  You can edit this section, but it is really intended");
   gINI.sectionComment("LevelSkipList", " for remote server management.  You will experience slightly better load times if you clean");
   gINI.sectionComment("LevelSkipList", " this section out from time to time.  The names of the keys are not important, and may be changed.");
   gINI.sectionComment("LevelSkipList", " Example:");
   gINI.sectionComment("LevelSkipList", " SkipLevel1=skip_me.level");
   gINI.sectionComment("LevelSkipList", " SkipLevel2=dont_load_me_either.level");
   gINI.sectionComment("LevelSkipList", " ... etc ...");
   gINI.sectionComment("LevelSkipList", "----------------");

   Vector<string> skipList;

   for(S32 i = 0; i < gLevelSkipList.size(); i++)
   {
      // "Normalize" the name a little before writing it
      string filename = lcase(gLevelSkipList[i].getString());
      if(filename.find(".level") == string::npos)
         filename += ".level";

      skipList.push_back(filename);
   }

   gINI.SetAllValues("LevelSkipList", "SkipLevel", skipList);
}

//////////////////////////////////
//////////////////////////////////

extern string joindir(const string &path, const string &filename);
extern CmdLineSettings gCmdLineSettings;

string resolutionHelper(const string &cmdLineDir, const string &rootDataDir, const string &subdir)
{
   if(cmdLineDir != "")             // Direct specification of ini path takes precedence...
      return cmdLineDir;
   else if(rootDataDir != "")       // ...over specification via rootdatadir param
      return joindir(rootDataDir, subdir);
   else 
      return subdir;
}


extern ConfigDirectories gConfigDirs;

// Doesn't handle leveldir -- that one is handled separately, later, because it requires input from the INI file
void ConfigDirectories::resolveDirs()
{
   string rootDataDir = gCmdLineSettings.dirs.rootDataDir;

   // rootDataDir used to specify the following folders
   gConfigDirs.robotDir      = resolutionHelper(gCmdLineSettings.dirs.robotDir,      rootDataDir, "robots");
   gConfigDirs.iniDir        = resolutionHelper(gCmdLineSettings.dirs.iniDir,        rootDataDir, "");
   gConfigDirs.logDir        = resolutionHelper(gCmdLineSettings.dirs.logDir,        rootDataDir, "");
   gConfigDirs.screenshotDir = resolutionHelper(gCmdLineSettings.dirs.screenshotDir, rootDataDir, "screenshots");

   // rootDataDir not used for sfx or lua folders
   gConfigDirs.sfxDir        = resolutionHelper(gCmdLineSettings.dirs.sfxDir,        "", "sfx");   
   gConfigDirs.luaDir        = resolutionHelper(gCmdLineSettings.dirs.luaDir,        "", "scripts");      
}


static bool assignifexists(const string &path)
{
   if(fileExists(path))
   {
      gConfigDirs.levelDir = path;
      return true;
   }

   return false;
}


// Figure out where the levels are.  This is exceedingly complex.
//
// Here are the rules:
//
// rootDataDir is specified on the command line via the -rootdatadir parameter
// levelDir is specified on the command line via the -leveldir parameter
// iniLevelDir is specified in the INI file
//
// Prioritize command line over INI setting, and -leveldir over -rootdatadir
//
// If levelDir exists, just use it (ignoring rootDataDir)
// ...Otherwise...
//
// If rootDataDir is specified then try
//	    If levelDir is also specified try
//      		rootDataDir/levels/levelDir
//      		rootDataDir/levelDir
//	    End
//	
//	    rootDataDir/levels
// End	   ==> Don't use rootDataDir
//      
// If iniLevelDir is specified
//	    If levelDir is also specified try
//      		iniLevelDir/levelDir
//     End	
//     iniLevelDir
// End	 ==> Don't use iniLevelDir
//      
// levels
//
// If none of the above work, no hosting/editing for you!
//
// NOTE: See above for full explanation of what this function is doing
static void doResolveLevelDir(const string &rootDataDir, const string &levelDir, const string &iniLevelDir)
{
   if(levelDir != "")
      if(assignifexists(levelDir))
         return;

   if(rootDataDir != "")
   {
      if(levelDir != "")
      {
         if(assignifexists(strictjoindir(rootDataDir, "levels", levelDir)))
            return;
         else if(assignifexists(strictjoindir(rootDataDir, levelDir)))
            return;
      }

      if(assignifexists(strictjoindir(rootDataDir, "levels")))
         return;
   }

   // rootDataDir is blank, or nothing using it worked
   if(iniLevelDir != "")
   {
      if(levelDir != "")
      {
         if(assignifexists(strictjoindir(iniLevelDir, levelDir)))
            return;
      }

      if(assignifexists(iniLevelDir))
         return;
   }

   if(assignifexists("levels"))
      return;

   gConfigDirs.levelDir = "";    // Surrender
}

#ifdef false
static void testLevelDirResolution()
{
   // These need to exist!
   // c:\temp\leveldir
   // c:\temp\leveldir2\levels\leveldir
   // For last test to work, need to run from folder that has no levels subdir

   /* 
   cd \temp
   mkdir leveldir
   mkdir leveldir2
   cd leveldir2
   mkdir levels
   cd levels
   mkdir leveldir
   */
   
   // rootDataDir, levelDir, iniLevelDir
   doResolveLevelDir("c:/temp", "leveldir", "c:/ini/level/dir/");       // rootDataDir/levelDir
   TNLAssertV(gConfigDirs.levelDir == "c:/temp/leveldir", ("Bad leveldir: %s", gConfigDirs.levelDir.c_str()));

   doResolveLevelDir("c:/temp/leveldir2", "leveldir", "c:/ini/level/dir/");       // rootDataDir/levels/levelDir
   TNLAssertV(gConfigDirs.levelDir == "c:/temp/leveldir2/levels/leveldir", ("Bad leveldir: %s", gConfigDirs.levelDir.c_str()));

   doResolveLevelDir("c:/temp/leveldir2", "c:/temp/leveldir", "c:/ini/level/dir/");       // levelDir
   TNLAssertV(gConfigDirs.levelDir == "c:/temp/leveldir", ("Bad leveldir: %s", gConfigDirs.levelDir.c_str()));

   doResolveLevelDir("c:/temp/leveldir2", "nosuchfolder", "c:/ini/level/dir/");       // rootDataDir/levels
   TNLAssertV(gConfigDirs.levelDir == "c:/temp/leveldir2/levels", ("Bad leveldir: %s", gConfigDirs.levelDir.c_str()));

   doResolveLevelDir("c:/temp/nosuchfolder", "leveldir", "c:/temp/");       // iniLevelDir/levelDir
   TNLAssertV(gConfigDirs.levelDir == "c:/temp/leveldir", ("Bad leveldir: %s", gConfigDirs.levelDir.c_str()));

   doResolveLevelDir("c:/temp/nosuchfolder", "nosuchfolder", "c:/temp");       // iniLevelDir
   TNLAssertV(gConfigDirs.levelDir == "c:/temp", ("Bad leveldir: %s", gConfigDirs.levelDir.c_str()));

   doResolveLevelDir("c:/temp/nosuchfolder", "nosuchfolder", "c:/does/not/exist");       // total failure
   TNLAssertV(gConfigDirs.levelDir == "", ("Bad leveldir: %s", gConfigDirs.levelDir.c_str()));

   printf("passed leveldir resolution tests!\n");
}
#endif


void ConfigDirectories::resolveLevelDir()
{
   //testLevelDirResolution();
   doResolveLevelDir(gCmdLineSettings.dirs.rootDataDir, gCmdLineSettings.dirs.levelDir, gIniSettings.levelDir);
}


static string checkName(const string &filename, const char *folders[], const char *extensions[])
{
   string name;
   if(filename.find('.') != string::npos)       // filename has an extension
   {
      S32 i = 0;
      while(strcmp(folders[i], ""))
      {
         name = strictjoindir(folders[i], filename);
         if(fileExists(name))
            return name;
         i++;
      }
   }
   else
   {
      S32 i = 0; 
      while(strcmp(extensions[i], ""))
      {
         S32 j = 0;
         while(strcmp(folders[j], ""))
         {
            name = strictjoindir(folders[j], filename + extensions[i]);
            if(fileExists(name))
               return name;
            j++;
         }
         i++;
      }
   }

   return "";
}


string ConfigDirectories::findLevelFile(const string &filename)         // static
{
#ifdef TNL_OS_XBOX         // This logic completely untested for OS_XBOX... basically disables -leveldir param
   const char *folders[] = { "d:\\media\\levels\\", "" };
#else
   const char *folders[] = { gConfigDirs.levelDir.c_str(), "" };
#endif
   const char *extensions[] = { ".level", "" };

   return checkName(filename, folders, extensions);
}


string ConfigDirectories::findLevelGenScript(const string &filename)    // static
{
   const char *folders[] = { gConfigDirs.levelDir.c_str(), gConfigDirs.luaDir.c_str(), "" };
   const char *extensions[] = { ".levelgen", ".lua", "" };

   return checkName(filename, folders, extensions);
}


string ConfigDirectories::findBotFile(const string &filename)           // static
{
   const char *folders[] = { gConfigDirs.robotDir.c_str(), "" };
   const char *extensions[] = { ".bot", ".lua", "" };

   return checkName(filename, folders, extensions);
}


};


