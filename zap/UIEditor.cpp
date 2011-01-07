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

#include "UIEditor.h"
#include "UINameEntry.h"
#include "UIEditorInstructions.h"
#include "UIChat.h"
#include "UIDiagnostics.h"
#include "UITeamDefMenu.h"
#include "UIGameParameters.h"
#include "UIErrorMessage.h"
#include "UIYesNo.h"
#include "gameObjectRender.h"
#include "gameType.h"
#include "soccerGame.h"          // For soccer ball radius
#include "engineeredObjects.h"   // For Turret properties
#include "barrier.h"             // For BarrierWidth
#include "speedZone.h"           // For default speed
#include "gameItems.h"           // For Asteroid defs
#include "teleporter.h"          // For teleporter radius
#include "config.h"
#include "SweptEllipsoid.h"
#include "textItem.h"            // For MAX_TEXTITEM_LEN and MAX_TEXT_SIZE
#include "luaLevelGenerator.h"
#include "../glut/glutInclude.h"

#include "oglconsole.h"          // Our console object

#include <ctype.h>
#include <exception>

namespace Zap
{

EditorUserInterface gEditorUserInterface;

const S32 DOCK_WIDTH = 50;
const S32 MIN_SCALE = 300;    // Most zoomed-in scale
const S32 MAX_SCALE = 10;     // Most zoomed-out scale

// Some colors

extern Color gNexusOpenColor;
extern Color GAME_WALL_FILL_COLOR;
extern Color EDITOR_WALL_FILL_COLOR;


static const Color inactiveSpecialAttributeColor = Color(.6, .6, .6);
static const Color white = Color(1,1,1);
static const Color red = Color(1,0,0);
static const Color yellow = Color(1,1,0);
static const Color blue = Color(0,0,1);
static const Color cyan = Color(0,1,1);
static const Color green = Color(0,1,0);
static const Color magenta = Color(1,0,1);
static const Color black = Color(0,0,0);

static const Color SELECT_COLOR = yellow;
static const Color HIGHLIGHT_COLOR = white;

static const S32 TEAM_NEUTRAL = Item::TEAM_NEUTRAL;
static const S32 TEAM_HOSTILE = Item::TEAM_HOSTILE;

static Vector<WorldItem> *mLoadTarget;

enum EntryMode {
   EntryID,          // Entering an objectID
   EntryAngle,       // Entering an angle
   EntryScale,       // Entering a scale
   EntryNone         // Not in a special entry mode
};


static EntryMode entryMode;
static Vector<ZoneBorder> zoneBorders;

void saveLevelCallback()
{
   if(gEditorUserInterface.saveLevel(true, true))
      UserInterface::reactivateMenu(gMainMenuUserInterface);
   else
      gEditorUserInterface.reactivate();
}


void backToMainMenuCallback()
{
   UserInterface::reactivateMenu(gMainMenuUserInterface);
}


const S32 NONE = -1;

// Constructor
EditorUserInterface::EditorUserInterface() : mGridDatabase(GridDatabase(false))
{
   setMenuID(EditorUI);

   // Create some items for the dock...  One of each, please!
   mShowMode = ShowAllObjects; 
   mWasTesting = false;

   mSnapVertex_i = NONE;
   mSnapVertex_j = NONE;
   mItemHit = NONE;
   mEdgeHit = NONE;

   mLastUndoStateWasBarrierWidthChange = false;

   mUndoItems.setSize(UNDO_STATES);
}


static const S32 DOCK_POLY_HEIGHT = 20;
static const S32 DOCK_POLY_WIDTH = DOCK_WIDTH - 10;

void EditorUserInterface::populateDock()
{
   mDockItems.clear();
   if(mShowMode == ShowAllObjects || mShowMode == ShowAllButNavZones)
   {
      S32 xPos = gScreenInfo.getGameCanvasWidth() - horizMargin - DOCK_WIDTH / 2;
      S32 yPos = 35;
      const S32 spacer = 35;

      // Reinstate in 014!
      mDockItems.push_back(WorldItem(ItemRepair, Point(xPos /*- 10*/, yPos), mCurrentTeam, true, 0, 0));
      //mDockItems.push_back(WorldItem(ItemEnergy, Point(xPos + 10, yPos), mCurrentTeam, true, 0, 0));

      yPos += spacer;
      mDockItems.push_back(WorldItem(ItemForceField, Point(xPos, yPos), mCurrentTeam, true, 0, 0));
      yPos += spacer;
      mDockItems.push_back(WorldItem(ItemSpawn, Point(xPos, yPos), mCurrentTeam, true, 0, 0));
      yPos += spacer;
      mDockItems.push_back(WorldItem(ItemTurret, Point(xPos, yPos), mCurrentTeam, true, 0, 0));
      yPos += spacer;
      mDockItems.push_back(WorldItem(ItemTeleporter, Point(xPos, yPos), mCurrentTeam, true, 0, 0));
      yPos += spacer;
      mDockItems.push_back(WorldItem(ItemSpeedZone, Point(xPos, yPos), mCurrentTeam, true, 0, 0));
      yPos += spacer;
      mDockItems.push_back(WorldItem(ItemTextItem, Point(xPos, yPos), mCurrentTeam, true, 0, 0));
      yPos += spacer;

      if(!strcmp(mGameType, "SoccerGameType"))
         mDockItems.push_back(WorldItem(ItemSoccerBall, Point(xPos, yPos), mCurrentTeam, true, 0, 0));
      else
         mDockItems.push_back(WorldItem(ItemFlag, Point(xPos, yPos), mCurrentTeam, true, 0, 0));
      yPos += spacer;

      mDockItems.push_back(WorldItem(ItemFlagSpawn, Point(xPos, yPos), mCurrentTeam, true, 0, 0));


      yPos += spacer;
      mDockItems.push_back(WorldItem(ItemMine, Point(xPos, yPos), mCurrentTeam, true, 0, 0));
      yPos += spacer;
      mDockItems.push_back(WorldItem(ItemSpyBug, Point(xPos, yPos), mCurrentTeam, true, 0, 0));
      yPos += spacer;

      // These two will share a line
      mDockItems.push_back(WorldItem(ItemAsteroid, Point(xPos - 10, yPos), mCurrentTeam, true, 0, 0));
      mDockItems.push_back(WorldItem(ItemAsteroidSpawn, Point(xPos + 10, yPos), mCurrentTeam, true, 0, 0));

      yPos += spacer;

      // These two will share a line
      mDockItems.push_back(WorldItem(ItemBouncyBall, Point(xPos - 10, yPos), mCurrentTeam, true, 0, 0));
      mDockItems.push_back(WorldItem(ItemResource, Point(xPos + 10, yPos), mCurrentTeam, true, 0, 0));

      yPos += 25;
      mDockItems.push_back(WorldItem(ItemLoadoutZone, Point(gScreenInfo.getGameCanvasWidth() - horizMargin - DOCK_WIDTH + 5, yPos), 
                                     mCurrentTeam, true, DOCK_POLY_WIDTH, DOCK_POLY_HEIGHT));
      yPos += 25;

      if(!strcmp(mGameType, "HuntersGameType"))
      {
         mDockItems.push_back(WorldItem(ItemNexus, Point(gScreenInfo.getGameCanvasWidth() - horizMargin - DOCK_WIDTH + 5, yPos), 
                                        mCurrentTeam, true, DOCK_POLY_WIDTH, DOCK_POLY_HEIGHT));
         yPos += 25;
      }
      else
      {
         mDockItems.push_back(WorldItem(ItemGoalZone, Point(gScreenInfo.getGameCanvasWidth() - horizMargin - DOCK_WIDTH + 5, yPos), 
                                        mCurrentTeam, true, DOCK_POLY_WIDTH, DOCK_POLY_HEIGHT));
         yPos += spacer;
      }
   }
   else if(mShowMode == NavZoneMode)
   {
      mDockItems.push_back(WorldItem(ItemNavMeshZone, Point(gScreenInfo.getGameCanvasWidth() - horizMargin - DOCK_WIDTH + 5, 
                                                            gScreenInfo.getGameCanvasHeight() - vertMargin - 82), 
                                     TEAM_NEUTRAL, true, DOCK_POLY_WIDTH, DOCK_POLY_HEIGHT));
   }
}


struct GameItemRec
{
   const char *name;    // Item's name, as seen in save file
   bool hasWidth;       // Does item have width?
   bool hasTeam;        // Item can be associated with team
   bool canBeNeutral;   // Item can be neutral 
   bool canBeHostile;   // Item can be hostile 
   bool hasText;        // Item has a text string attached to it
   bool hasRepop;       // Item has a repop delay that can be set
   GeomType geom;
   char letter;         // How item is represented by editor.  Some items are drawn with a letter others have custom symbols.
   bool specialTabKeyRendering;  // true if item is rendered in a special way when tab is down
   const char *prettyNamePlural;
   const char *onDockName;       // Briefer, pretty looking label to label things on dock
   const char *onScreenName;     // Brief, pretty looking label to label things on screen
   const char *helpText;         // Help string displayed when hovering over item on the dock
};


// Remember to keep these properly aligned with GameItems enum                                       display
//   Name,                 hasWidth, hasTeam, canBeNeut, canBeHos, hasText, hasRepop,   geom,        letter, special, prettyNamePlural        onDockName   onScreenName      description
GameItemRec itemDef[] = {
   { "Spawn",               false,    true,      true,     false,   false,   false,   geomPoint,      'S',    true,   "Spawn points",           "Spawn",    "Spawn",        "Location where ships start.  At least one per team is required. [G]" },
   { "SpeedZone",           false,    false,     true,     true,    false,   false,   geomSimpleLine,  0,     false,  "GoFasts",                "GoFast",   "GoFast",       "Makes ships go fast in direction of arrow. [P]" },
   { "SoccerBallItem",      false,    false,     false,    false,   false,   false,   geomPoint,       0,     true,   "Soccer balls",           "Ball",     "Ball",         "Soccer ball, can only be used in Soccer games." },
   { "FlagItem",            false,    true,      true,     true,    false,   false,   geomPoint,       0,     false,  "Flags",                  "Flag",     "Flag",         "Flag item, used by a variety of game types." },
   { "FlagSpawn",           false,    true,      true,     true,    false,   true,    geomPoint,       0,     true,   "Flag spawn points",      "FlagSpawn","FlagSpawn",    "Location where flags (or balls in Soccer) spawn after capture." },
   { "BarrierMaker",        true,     false,     false,    false,   false,   false,   geomLine,        0,     false,  "Barrier makers",         "Wall",     "Wall",         "Run-of-the-mill wall item." },
   { "LineItem",            true,     true,      true,     true,    false,   false,   geomLine,        0,     false,  "Decorative Lines",       "LineItem", "LineItem",     "Decorative linework." },
   { "Teleporter",          false,    false,     false,    false,   false,   false,   geomSimpleLine,  0,     false,  "Teleporters",            "Teleport", "Teleport",     "Teleports ships from one place to another. [T]" },
   { "RepairItem",          false,    false,     false,    false,   false,   true,    geomPoint,       0,     false,  "Repair items",           "Repair",      "Repair",       "Repairs damage to ships. [B]" },
   { "EnergyItem",          false,    false,     false,    false,   false,   true,    geomPoint,       0,     false,  "Energy items",           "Enrg",     "Energy",       "Restores energy to ships" },
   { "TestItem",            false,    false,     false,    false,   false,   false,   geomPoint,       0,     true,   "Test items",             "Test",     "Test Item",    "Bouncy object that floats around and gets in the way." },
   { "Asteroid",            false,    false,     false,    false,   false,   false,   geomPoint,       0,     true,   "Asteroids",              "Ast.",     "Asteroid",     "Shootable asteroid object.  Just like the arcade game." },
   { "AsteroidSpawn",       false,    false,     false,    false,   false,   true,    geomPoint,       0,     true,   "Asteroid spawn points",  "ASP",      "AsteroidSpawn","Periodically spawns a new asteroid." },
   { "Mine",                false,    false,     true,     true,    false,   false,   geomPoint,      'M',    true,   "Mines",                  "Mine",     "Mine",         "Mines can be prepositioned, and are are \"hostile to all\". [M]" },
   { "SpyBug",              false,    true,      true,     true,    false,   false,   geomPoint,      'S',    false,  "Spy bugs",               "Spy Bug",  "Spy Bug",      "Remote monitoring device that shows enemy ships on the commander's map. [Ctrl-B]" },
   { "ResourceItem",        false,    false,     false,    false,   false,   false,   geomPoint,       0,     true,   "Resource items",         "Res.",     "Resource",     "Small bouncy object that floats around and gets in the way." },
   { "LoadoutZone",         false,    true,      true,     true,    false,   false,   geomPoly,        0,     false,  "Loadout zones",          "Loadout",  "Loadout",      "Area to finalize ship modifications.  Each team should have at least one." },
   { "HuntersNexusObject",  false,    false,     true,     true,    false,   false,   geomPoly,        0,     false,  "Nexus zones",            "Nexus",    "Nexus",        "Area to bring flags in Hunter game.  Cannot be used in other games." },
   { "SlipZone",            false,    false,     true,     true,    false,   false,   geomPoly,       'z',    false,  "Slip zones",             "Slip Zone","Slip Zone",    "Not yet implemented." },
   { "Turret",              false,    true,      true,     true,    false,   true,    geomPoint,      'T',    false,  "Turrets",                "Turret",   "Turret",       "Creates shooting turret.  Can be on a team, neutral, or \"hostile to all\". [Y]" },
   { "ForceFieldProjector", false,    true,      true,     true,    false,   true ,   geomPoint,      '>',    false,  "Force field projectors", "ForceFld", "ForceFld",     "Creates a force field that lets only team members pass. [F]" },
   { "GoalZone",            false,    true,      true,     true,    false,   false,   geomPoly,        0,     false,  "Goal zones",             "Goal",     "Goal",         "Target area used in a variety of games." },
   { "TextItem",            false,    true,      true,     true,    true,    false,   geomSimpleLine,  0,     false,  "Text Items",             "TextItem", "Text",         "Draws a bit of text on the map.  Visible only to team, or to all if neutral." },
   { "BotNavMeshZone",      false,    false,     true,     true,    false,   false,   geomPoly,        0,     false,  "NavMesh Zones",          "NavMesh",  "NavMesh",      "Creates navigational mesh zone for robots." },

   { NULL,                  false,    false,     false,   false,    false,   false,   geomNone,        0,     false,  "",                       "",         "",             "" },
};


// Destructor -- unwind things in an orderly fashion
EditorUserInterface::~EditorUserInterface()
{
   // Delete all forcefields first so app doesn't get wrapped up in recalculating their end points when
   // the segment they're terminating at gets deleted.
   for(S32 i = 0; i < mItems.size(); i++)
      if(mItems[i].index == ItemForceField)
      {
         mItems.erase_fast(i);
         i--;
      }
}


void EditorUserInterface::saveUndoState()
{
   saveUndoState(mItems, false);
}


// Save the current state of the editor objects for later undoing
void EditorUserInterface::saveUndoState(const Vector<WorldItem> &items, bool cameFromRedo)
{
   if(!cameFromRedo)
      mLastRedoIndex = mLastUndoIndex;

   // Use case: We do 5 actions, save, undo 2, redo 1, then do some new action.  
   // Our "no need to save" undo point is lost forever.
   if(mAllUndoneUndoLevel > mLastRedoIndex)     
      mAllUndoneUndoLevel = NONE;

   mUndoItems[mLastUndoIndex % UNDO_STATES] = items;

   mLastUndoIndex++;
   mLastRedoIndex++; 

   if(mLastUndoIndex % UNDO_STATES == mFirstUndoIndex % UNDO_STATES)           // Undo buffer now full...
   {
      mFirstUndoIndex++;
      mAllUndoneUndoLevel -= 1;     // If this falls below 0, then we can't undo our way out of needing to save
   }
   
   mNeedToSave = (mAllUndoneUndoLevel != mLastUndoIndex);
   mRedoingAnUndo = false;
   mLastUndoStateWasBarrierWidthChange = false;
}


void EditorUserInterface::autoSave()
{
   saveLevel(false, false, true);
}


void EditorUserInterface::undo(bool addToRedoStack)
{
   if(!undoAvailable())
      return;

   mSnapVertex_i = NONE;
   mSnapVertex_j = NONE;

   if(mLastUndoIndex == mLastRedoIndex && !mRedoingAnUndo)
   {
      saveUndoState(mItems);
      mLastUndoIndex--;
      mLastRedoIndex--;
      mRedoingAnUndo = true;
   }

   mLastUndoIndex--;
   mItems = mUndoItems[mLastUndoIndex % UNDO_STATES];      // Restore state from undo buffer

   rebuildEverything();

   mLastUndoStateWasBarrierWidthChange = false;
}
   

void EditorUserInterface::redo()
{
   if(mLastRedoIndex != mLastUndoIndex)      // If there's a redo state available...
   {
      mSnapVertex_i = NONE;
      mSnapVertex_j = NONE;

      mLastUndoIndex++;
      mItems = mUndoItems[mLastUndoIndex % UNDO_STATES];      // Restore state from undo buffer
   
     rebuildEverything();
   }
}


void EditorUserInterface::rebuildEverything()
{
   wallSegmentManager.recomputeAllWallGeometry();
   recomputeAllEngineeredItems();
   rebuildAllBorderSegs();

   mNeedToSave = (mAllUndoneUndoLevel != mLastUndoIndex);
   itemToLightUp = NONE;
   autoSave();
}


void EditorUserInterface::recomputeAllEngineeredItems()
{
   for(S32 i = 0; i < mItems.size(); i++)
      if(mItems[i].index == ItemTurret || mItems[i].index == ItemForceField)
         mItems[i].snapEngineeredObject(mItems[i].vert(0));
}


bool EditorUserInterface::undoAvailable()
{
   return mLastUndoIndex - mFirstUndoIndex != 1;
}


// Wipe undo/redo history
void EditorUserInterface::clearUndoHistory()
{
   mFirstUndoIndex = 0;
   mLastUndoIndex = 1;
   mLastRedoIndex = 1;
   mRedoingAnUndo = false;
}


bool EditorUserInterface::isFlagGame(char *mGameType)
{
   TNL::Object *theObject = TNL::Object::create(mGameType);  // Instantiate a gameType object
   GameType *gameType = dynamic_cast<GameType*>(theObject);  // and cast it

   if(!gameType)
      return false;
   else
      return gameType->isFlagGame();
}


bool EditorUserInterface::isTeamFlagGame(char *mGameType)
{
   TNL::Object *theObject = TNL::Object::create(mGameType);  // Instantiate a gameType object
   GameType *gameType = dynamic_cast<GameType*>(theObject);  // and cast it

   if(!gameType)
      return false;
   else
      return gameType->isTeamFlagGame();
}


extern TeamPreset gTeamPresets[];

void EditorUserInterface::setLevelFileName(string name)
{
   if(name == "")
      mEditFileName = "";
   else
      if(name.find('.') == std::string::npos)      // Append extension, if one is needed
         mEditFileName = name + ".level";
      // else... what?
}


void EditorUserInterface::setLevelGenScriptName(string line)
{
   mScriptLine = trim(line);
}


void EditorUserInterface::makeSureThereIsAtLeastOneTeam()
{
   if(mTeams.size() == 0)
   {
      TeamEditor t;
      t.setName(gTeamPresets[0].name);
      t.color.set(gTeamPresets[0].r, gTeamPresets[0].g, gTeamPresets[0].b);
      mTeams.push_back(t);
   }
}


// This sort will put points on top of lines on top of polygons...  as they should be
// NavMeshZones are now drawn on top, to make them easier to see.  Disable with Ctrl-A!
// We'll also put walls on the bottom, as this seems to work best in practice
S32 QSORT_CALLBACK geometricSort(WorldItem *a, WorldItem *b)
{
   if(a->index == ItemBarrierMaker)
      return false;
   if(b->index == ItemBarrierMaker)
      return true;

   return( (a->geomType() < b->geomType() ) );
}


extern const char *gGameTypeNames[];
extern S32 gDefaultGameTypeIndex;
extern S32 gMaxPolygonPoints;
extern ConfigDirectories gConfigDirs;

// Loads a level
void EditorUserInterface::loadLevel()
{
   // Initialize
   mItems.clear();
   mTeams.clear();
   mSnapVertex_i = NONE;
   mSnapVertex_j = NONE;
   clearLevelGenItems();
   mLoadTarget = &mItems;
   mGameTypeArgs.clear();
   gGameParamUserInterface.gameParams.clear();
   gGameParamUserInterface.savedMenuItems.clear();          // clear() because this is not a pointer vector
   gGameParamUserInterface.menuItems.deleteAndClear();      // Keeps interface from using our menuItems to rebuild savedMenuItems
   mGridSize = Game::DefaultGridSize;                       // Used in editor for scaling walls and text items appropriately

   mGameType[0] = 0;                   // Clear mGameType
   char fileBuffer[1024];
   dSprintf(fileBuffer, sizeof(fileBuffer), "%s/%s", gConfigDirs.levelDir.c_str(), mEditFileName.c_str());

   if(initLevelFromFile(fileBuffer))   // Process level file --> returns true if file found and loaded, false if not (assume it's a new level)
   {
      makeSureThereIsAtLeastOneTeam(); // Make sure we at least have one team
      validateTeams();                 // Make sure every item has a valid team
      validateLevel();                 // Check level for errors (like too few spawns)
      mItems.sort(geometricSort);
      gGameParamUserInterface.ignoreGameParams = false;
   }
   else
   {
      makeSureThereIsAtLeastOneTeam();                               // Make sure we at least have one team, like the man said.
      strcpy(mGameType, gGameTypeNames[gDefaultGameTypeIndex]);
      gGameParamUserInterface.gameParams.push_back("GameType 10 8"); // A nice, generic game type that we can default to
      gGameParamUserInterface.ignoreGameParams = true;               // Don't rely on the above for populating GameParameters menus... only to make sure something is there if we save
   }
   clearUndoHistory();                 // Clean out undo/redo buffers
   clearSelection();                   // Nothing starts selected
   mShowMode = ShowAllObjects;         // Turn everything on
   mNeedToSave = false;                // Why save when we just loaded?
   mAllUndoneUndoLevel = mLastUndoIndex;
   populateDock();                     // Add game-specific items to the dock

   // Bulk-process new items, walls first
   for(S32 i = 0; i < mItems.size(); i++)
      mItems[i].processEndPoints();

   wallSegmentManager.recomputeAllWallGeometry();
   
   // Bulk-process bot nav mesh zone boundaries
   for(S32 i = 0; i < mItems.size(); i++)
      if(mItems[i].index == ItemNavMeshZone)
          mItems[i].initializeGeom();

   gEditorUserInterface.rebuildAllBorderSegs();


   for(S32 i = 0; i < mItems.size(); i++)
      if(mItems[i].index == ItemTurret || mItems[i].index == ItemForceField)
		  mItems[i].snapEngineeredObject(mItems[i].vert(0));

   // And hand-process all other items
   for(S32 i = 0; i < mItems.size(); i++)
      if(mItems[i].index != ItemBarrierMaker && mItems[i].index != ItemNavMeshZone)
         mItems[i].onGeomChanged();
}


//extern S32 gMaxPlayers;

// Process a line read from level file
void EditorUserInterface::processLevelLoadLine(U32 argc, U32 id, const char **argv)
{
   S32 strlenCmd = (S32) strlen(argv[0]);

   // Parse GameType line... All game types are of form XXXXGameType
   if(strlenCmd >= 8 && !strcmp(argv[0] + strlenCmd - 8, "GameType"))
   {
      strcpy(gEditorUserInterface.mGameType, GameType::validateGameType(argv[0]) );    // validateGameType will return a valid game type, regradless of what's put in

      if(strcmp(gEditorUserInterface.mGameType, argv[0]))      // If these differ, then what we put in was invalid
         gEditorUserInterface.setWarnMessage("Invalid or missing GameType parameter", "Press [F3] to configure level");

      // Save the args (which we already have split out) for easier handling in the Game Parameter Editor
      for(U32 i = 1; i < argc; i++)
         gEditorUserInterface.mGameTypeArgs.push_back(argv[i]);
   }

   else if(!strcmp(argv[0], "GridSize"))
   {
      if(argc >= 1)
         gEditorUserInterface.setGridSize((F32) atof(argv[1]));
   }

   else if(!strcmp(argv[0], "Script"))
   {
      gEditorUserInterface.mScriptLine = "";
      // Munge everything into a string.  We'll have to parse after editing in GameParamsMenu anyway.
      for(U32 i = 1; i < argc; i++)
      {
         if(i > 1)
            gEditorUserInterface.mScriptLine += " ";

         gEditorUserInterface.mScriptLine += argv[i];
      }
   }

   // Parse Team definition line
   else if(!strcmp(argv[0], "Team"))
   {
      if(mTeams.size() >= GameType::gMaxTeams)     // Ignore teams with numbers higher than 9
         return;

      TeamEditor team;
      team.readTeamFromLevelLine(argc, argv);

      // If team was read and processed properly, numPlayers will be 0
      if(team.numPlayers != -1)
         mTeams.push_back(team);
   }

   else
   {
      GameItems itemType = ItemInvalid;

      for(S32 index = 0; itemDef[index].name != NULL; index++)
         if(!strcmp(argv[0], itemDef[index].name))
         {
            itemType = static_cast<GameItems>(index);
            break;
         }

      if(itemType != ItemInvalid)     
      {
         WorldItem newItem(itemType, id);
         if(newItem.processArguments(argc - 1, argv + 1))
            mLoadTarget->push_back(newItem);           //don't add to editor if not valid...
         return;
      }
   }

   // What remains are various game parameters...  Note that we will hit this block even if we already looked at gridSize and such...
   // Before copying, we'll make a dumb copy, which will be overwritten if the user goes into the GameParameters menu
   // This will cover us if the user comes in, edits the level, saves, and exits without visiting the GameParameters menu
   // by simply echoing all the parameters back out to the level file without further processing or review.
   string temp;
   for(U32 i = 0; i < argc; i++)
   {
      temp += argv[i];
      if(i < argc - 1)
         temp += " ";
   }

   gGameParamUserInterface.gameParams.push_back(temp);
}    


extern OGLCONSOLE_Console gConsole;

void EditorUserInterface::clearLevelGenItems()
{
   mLevelGenItems.clear();
}


extern void removeCollinearPoints(Vector<Point> &points, bool isPolygon);

void EditorUserInterface::copyScriptItemsToEditor()
{
   if(mLevelGenItems.size() == 0)
      return;     // Print error message?

   saveUndoState(mItems);

   Vector<WorldItem> zones;

   for(S32 i = 0; i < mLevelGenItems.size(); i++)
      mItems.push_back(mLevelGenItems[i]);

   mLevelGenItems.clear();

   rebuildEverything();

   mLastUndoStateWasBarrierWidthChange = false;
}


static S32 findZoneContaining( Vector<WorldItem> &zones, const Point &p)
{
   for(S32 i = 0; i < zones.size(); i++)
   {
      // First a quick, crude elimination check then more comprehensive one
      // Since our zones are convex, we can use the faster method!  Yay!
      // Actually, we can't, as it is not reliable... reverting to more comprehensive (and working) version.
      if( zones[i].getExtent().contains(p) &&
                  PolygonContains2(zones[i].getVerts().address(), zones[i].getVerts().size(), p) )
         return i;
   }
   return -1;
}


void EditorUserInterface::removeUnusedNavMeshZones(Vector<WorldItem> &zones)
{
   Vector<S16> inProcessList; 

   for(S32 i = 0; i < zones.size(); i++)
      zones[i].flag = false;

   // Start with list of all spawns and teleport outtakes 
   for(S32 i = 0; i < mItems.size(); i++)
   {
      if(mItems[i].index == ItemSpawn)
      {
         S32 zoneIndex = findZoneContaining(zones, mItems[i].getVerts()[0]);
         if(zoneIndex >= 0)
         {
            zones[zoneIndex].flag = true;        // Mark zone as processed
            inProcessList.push_back(zoneIndex);
         }
      }
      else if(mItems[i].index == ItemTeleporter)
      {
         S32 zoneIndex = findZoneContaining(zones, mItems[i].getVerts()[1]);    // Intake = vert0, outtake = vert1
         if(zoneIndex >= 0)
         {
            zones[zoneIndex].flag = true;        // Mark zone as processed
            inProcessList.push_back(zoneIndex);
         }
      }
   }

   // From here on down, very inefficient, but ok for testing the idea.  Need to precompute some of these things!
   // Since the order in which we process the zones doesn't matter, work from the end of the last towards the front; it's more efficient 
   // that way...
   while(inProcessList.size() > 0)
   {
      S32 zoneIndex = inProcessList.last();
      inProcessList.erase(inProcessList.size() - 1);     // Remove last element      

      // Visit all neighboring zones
      for(S32 i = 0; i < zones.size(); i++)
      {
         if(i == zoneIndex)
            continue;      // Don't check self...

         // Do zones i and j touch?  First a quick and dirty bounds check:
         if(!zones[zoneIndex].getExtent().intersectsOrBorders(zones[i].getExtent()))
            continue;

         static Point start, end;
         if(zonesTouch(zones[zoneIndex].getVerts(), zones[i].getVerts(), 1 / mGridSize, start, end))
         {
            if(!zones[i].flag)           // If zone hasn't been processed...
            {
               inProcessList.push_back(i);
               zones[i].flag = true;     // Mark zone as "in"
            }
         }
      }
   }

   // Anything not marked as in at this point is out.  Delete it.
   // TODO: This is TOTALLY INEFFICIENT!!!  Find a better way to delete unmarked zones -- perhaps create a container for zone that
   // has some pointer back to mItems index.  Could put flag on that container, rather than in BotNavMeshZone as I did to test this idea.
   for(S32 i = 0; i < zones.size(); i++)
   {
      if(!zones[i].flag)
      {
         zones.erase_fast(i);
         i--;
      }
   }
}


void EditorUserInterface::runLevelGenScript()
{
   // Parse mScriptLine 
   if(mScriptLine == "")      // No script included!!
      return;

   OGLCONSOLE_Output(gConsole, "Running script %s\n", mScriptLine.c_str());

   Vector<string> scriptArgs = parseString(mScriptLine);
   
   string scriptName = scriptArgs[0];
   scriptArgs.erase(0);

   clearLevelGenItems();      // Clear out any items from the last run

   // Set the load target to the levelgen list, as that's where we want our items stored
   mLoadTarget = &mLevelGenItems;

   runScript(scriptName, scriptArgs);

   // Reset the target
   mLoadTarget = &mItems;
}


// Runs an arbitrary lua script.  Command is first item in cmdAndArgs, subsequent items are the args, if any
void EditorUserInterface::runScript(const string &scriptName, const Vector<string> &args)
{
   string name = ConfigDirectories::findLevelGenScript(scriptName);  // Find full name of levelgen script

   if(name == "")
   {
      logprintf(LogConsumer::LogWarning, "Warning: Could not find script \"%s\"",  scriptName.c_str());
      // TODO: Show an error to the user
      return;
   }

   // Load the items
   LuaLevelGenerator(name, args, mGridSize, getGridDatabase(), this, gConsole);
   
   // Process new items
   // Not sure about all this... may need to test
   // Bulk-process new items, walls first
   for(S32 i = 0; i < mLoadTarget->size(); i++)
      if((*mLoadTarget)[i].index == ItemBarrierMaker)
      {
         if((*mLoadTarget)[i].vertCount() < 2)      // Invalid item; delete
         {
            (*mLoadTarget).erase_fast(i);
            i--;
         }

         (*mLoadTarget)[i].processEndPoints();
      }
}


void EditorUserInterface::validateLevel()
{
   bool hasError = false;
   mLevelErrorMsgs.clear();
   bool foundSoccerBall = false;
   bool foundNexus = false;
   bool foundTeamFlags = false;
   bool foundTeamFlagSpawns = false;
   bool foundFlags = false;
   bool foundNeutralSpawn = false;

   Vector<bool> foundSpawn;
   char buf[32];

   string teamList, teams;

   // First, catalog items in level
   for(S32 i = 0; i < mTeams.size(); i++)      // Initialize vector
      foundSpawn.push_back(false);

   for(S32 i = 0; i < mItems.size(); i++)
   {
      if(mItems[i].index == ItemSpawn && mItems[i].team == TEAM_NEUTRAL)
         foundNeutralSpawn = true;
      else if(mItems[i].index == ItemSpawn && mItems[i].team >= 0)
         foundSpawn[mItems[i].team] = true;
      else if(mItems[i].index == ItemSoccerBall)
         foundSoccerBall = true;
      else if(mItems[i].index == ItemNexus)
         foundNexus = true;
      else if(mItems[i].index == ItemFlag)
      {
         foundFlags = true;
         if(mItems[i].team >= 0)
            foundTeamFlags = true;
      }
      else if(mItems[i].index == ItemFlagSpawn)
      {
         if(mItems[i].team >= 0)
            foundTeamFlagSpawns = true;
      }
   }


   // "Unversal errors" -- levelgens can't (yet) change gametype

   // The other thing that can cause a crash is a soccer ball in a a game other than SoccerGameType
   if(foundSoccerBall && strcmp(mGameType, "SoccerGameType"))
      mLevelErrorMsgs.push_back("ERROR: Soccer ball can only be used in soccer game.");

   // Check for the nexus object in a non-hunter game.  This cause endless grief too!
   if(foundNexus && strcmp(mGameType, "HuntersGameType"))
      mLevelErrorMsgs.push_back("ERROR: Nexus object can only be used in Hunters game.");

   // Check for missing nexus object in a hunter game.  This cause mucho dolor!
   if(!foundNexus && !strcmp(mGameType, "HuntersGameType"))
      mLevelErrorMsgs.push_back("ERROR: Nexus game must have a Nexus.");

   if(foundFlags && !isFlagGame(mGameType))
      mLevelErrorMsgs.push_back("WARNING: This game type does not use flags.");
   else if(foundTeamFlags && !isTeamFlagGame(mGameType))
      mLevelErrorMsgs.push_back("WARNING: This game type does not use team flags.");

   // Check for team flag spawns on games with no team flags
   if(foundTeamFlagSpawns && !foundTeamFlags)
      mLevelErrorMsgs.push_back("WARNING: Found team flag spawns but no team flags.");


   // Errors that may be corrected by levelgen -- script could add spawns
   // Neutral spawns work for all; if there's one, then that will satisfy our need for spawns for all teams
   if(mScriptLine == "" && !foundNeutralSpawn)
   {
      // Make sure each team has a spawn point
      for(S32 i = 0; i < foundSpawn.size(); i++)
         if(!foundSpawn[i])
         {
            dSprintf(buf, sizeof(buf), "%d", i+1);

            if(!hasError)     // This is our first error
            {
               teams = "team ";
               teamList = buf;
            }
            else
            {
               teams = "teams ";
               teamList += ", ";
               teamList += buf;
            }
            hasError = true;
         }
   }

   if(hasError)     // Compose error message
      mLevelErrorMsgs.push_back("ERROR: Need spawn point for " + teams + teamList);
}


// Check that each item has a valid team  (fixes any problems it finds)
void EditorUserInterface::validateTeams()
{
   S32 teams = mTeams.size();

   for(S32 i = 0; i < mItems.size(); i++)
   {
      S32 team = mItems[i].team;

      if(itemDef[mItems[i].index].hasTeam && ((team >= 0 && team < teams) || team == TEAM_NEUTRAL || team == TEAM_HOSTILE))  
         continue;      // This one's OK

      if(team == TEAM_NEUTRAL && itemDef[mItems[i].index].canBeNeutral)
         continue;      // This one too

      if(team == TEAM_HOSTILE && itemDef[mItems[i].index].canBeHostile)
         continue;      // This one too

      if(itemDef[mItems[i].index].hasTeam)
         mItems[i].team = 0;               // We know there's at least one team, so there will always be a team 0
      else if(itemDef[mItems[i].index].canBeHostile && !itemDef[mItems[i].index].canBeNeutral)
         mItems[i].team = TEAM_HOSTILE; 
      else
         mItems[i].team = TEAM_NEUTRAL;    // We won't consider the case where hasTeam == canBeNeutral == canBeHostile == false
   }
}


// Search through editor objects, to make sure everything still has a valid team.  If not, we'll assign it a default one.
// Note that neutral/hostile items are on team -1/-2, and will be unaffected by this loop or by the number of teams we have.
void EditorUserInterface::teamsHaveChanged()
{
   bool teamsChanged = false;

   if(mTeams.size() != mOldTeams.size())
      teamsChanged = true;
   else
      for(S32 i = 0; i < mTeams.size(); i++)
         if(mTeams[i].color != mOldTeams[i].color || mTeams[i].getName() != mOldTeams[i].getName())
         {
            teamsChanged = true;
            break;
         }

   if(!teamsChanged)       // Nothing changed, we're done here
      return;

   for (S32 i = 0; i < mItems.size(); i++)
      if(mItems[i].team >= mTeams.size())       // Team no longer valid?
         mItems[i].team = 0;                    // Then set it to first team

   // And the dock items too...
   for (S32 i = 0; i < mDockItems.size(); i++)
      if(mDockItems[i].team >= mTeams.size())
         mDockItems[i].team = 0;

   validateLevel();          // Revalidate level -- if teams have changed, requirements for spawns have too
   mNeedToSave = true;
   autoSave();
   mAllUndoneUndoLevel = -1; // This change can't be undone
}


extern Color gNeutralTeamColor;
extern Color gHostileTeamColor;

Color EditorUserInterface::getTeamColor(S32 team)
{
   if(team == TEAM_NEUTRAL)
      return gNeutralTeamColor;
   else if(team == TEAM_HOSTILE)
      return gHostileTeamColor;
   else
      return mTeams[team].color;
}


string EditorUserInterface::getLevelFileName()
{
   return mEditFileName;
}


// Run script to generate bot zones
void EditorUserInterface::generateBotZones()
{
   // First, remove any existing nav mesh zones
   for(S32 i = 0; i < mItems.size(); i++)
      if(mItems[i].index == ItemNavMeshZone)
      {
         mItems.erase_fast(i);
         i--;
      }

   // For the moment, base zones are generated via Sam's Lua script, now parked in Lua folder
   Vector<string> scriptArgs;

   Vector<WorldItem> zones;
   mLoadTarget = &zones;

   LuaLevelGenerator("build_navzones.lua", scriptArgs, getGridSize(), getGridDatabase(), this, gConsole);

   for(S32 i = 0; i < zones.size(); i++)
      zones[i].setExtent(Rect(zones[i].getVerts()));

   removeUnusedNavMeshZones(zones);    // Culls unreachable zones

   Point anchor, nrml;

   // Snap zone boundaries to walls ==> doesn't work well with diagonal lines
  /* for(S32 i = 0; i < zones.size(); i++)
   {
      for(S32 j = 0; j < zones[i].getVerts().size(); j++)
      {
         DatabaseObject *mountSeg = EngineeredObject::
               findAnchorPointAndNormal(getGridDatabase(), zones[i].getVerts()[j], .1, false, anchor, nrml);

         if(mountSeg)
            zones[i].getVerts()[j].set(anchor);
      }
   }
*/
   // Remove collinear points -- reduce number of points needed to represent a zone
   for(S32 i = 0; i < zones.size(); i++)
      removeCollinearPoints(zones[i].getVerts(), true);

   // "Squarify" zones (helps clean up some of the junk points in some edge zones)
   for(S32 i = 0; i < zones.size(); i++)
   {
      if(zones[i].getVerts().size() <= 4)      // Square or triangles can't be squarified
         continue;
      
      F32 ulX = zones[i].getExtent().min.x;
      F32 ulY = zones[i].getExtent().min.y;
      F32 lrX = zones[i].getExtent().max.x;
      F32 lrY = zones[i].getExtent().max.y;

      for(S32 j = 0; j < zones[i].getVerts().size(); j++)
      {
         if(!(zones[i].getVerts()[j].x == ulX || zones[i].getVerts()[j].x == lrX || 
              zones[i].getVerts()[j].y == ulY || zones[i].getVerts()[j].y == lrY))
         {
            zones[i].getVerts().erase(j);
            j--;
         }
      }
   }

   // Initialize the zones
   for(S32 i = 0; i < zones.size(); i++)
      zones[i].initializeGeom();

   // Add, finally, add them to the editor
   for(S32 i = 0; i < zones.size(); i++)
      mItems.push_back(zones[i]);

   gEditorUserInterface.rebuildAllBorderSegs();
}


// Handle console input
// Valid commands: help, run, clear, quit, exit
void processEditorConsoleCommand(OGLCONSOLE_Console console, char *cmdline)
{
   Vector<string> words = parseString(cmdline);
   if(words.size() == 0)
      return;

   string cmd = lcase(words[0]);

   if(cmd == "quit" || cmd == "exit") 
      OGLCONSOLE_HideConsole();

   else if(cmd == "help" || cmd == "?") 
      OGLCONSOLE_Output(console, "Commands: help; run; clear; genzones; quit\n");

   else if(cmd == "run")
   {
      if(words.size() == 1)      // Too few args
         OGLCONSOLE_Output(console, "Usage: run <script_name> {args}\n");
      else
      {
         gEditorUserInterface.saveUndoState();
         words.erase(0);         // Get rid of "run", leaving script name and args

         string name = words[0];
         words.erase(0);

         gEditorUserInterface.onBeforeRunScriptFromConsole();
         gEditorUserInterface.runScript(name, words);
         gEditorUserInterface.onAfterRunScriptFromConsole();
      }
   }   

   else if(cmd == "clear")
      gEditorUserInterface.clearLevelGenItems();

   else if(cmd == "genzones")
      gEditorUserInterface.generateBotZones();

    else
      OGLCONSOLE_Output(console, "Unknown command: %s\n", cmd.c_str());
}


void EditorUserInterface::onBeforeRunScriptFromConsole()
{
   // Use selection as a marker -- will have to change in future
   for(S32 i = 0; i < mItems.size(); i++)
      mItems[i].selected = true;
}


void EditorUserInterface::onAfterRunScriptFromConsole()
{
   for(S32 i = 0; i < mItems.size(); i++)
      mItems[i].selected = !mItems[i].selected;

   rebuildEverything();
   syncUnmovedItems();
}


extern void actualizeScreenMode(bool, bool = false);

void EditorUserInterface::onActivate()
{
   if(gConfigDirs.levelDir == "")      // Never did resolve a leveldir... no editing for you!
   {
      gEditorUserInterface.reactivatePrevUI();     // Must come before the error msg, so it will become the previous UI when that one exits

      gErrorMsgUserInterface.reset();
      gErrorMsgUserInterface.setTitle("HOUSTON, WE HAVE A PROBLEM");
      gErrorMsgUserInterface.setMessage(1, "No valid level folder was found..."); 
      gErrorMsgUserInterface.setMessage(2, "cannot start the level editor");
      gErrorMsgUserInterface.setMessage(4, "Check the LevelDir parameter in your INI file,");
      gErrorMsgUserInterface.setMessage(5, "or your command-line parameters to make sure");
      gErrorMsgUserInterface.setMessage(6, "you have correctly specified a valid folder.");
      gErrorMsgUserInterface.activate();

      return;
   }

   // Check if we have a level name:
   if(getLevelFileName() == "")         // We need to take a detour to get a level name
   {
      // Don't save this menu (false, below).  That way, if the user escapes out, and is returned to the "previous"
      // UI, they will get back to where they were before (prob. the main menu system), not back to here.
      gLevelNameEntryUserInterface.activate(false);

      return;
   }

   mLevelErrorMsgs.clear();
   mSaveMsgTimer.clear();

   mGameTypeArgs.clear();

   mHasBotNavZones = false;

   loadLevel();
   setCurrentTeam(0);

   syncUnmovedItems();

   mSnapDisabled = false;      // Hold [space] to temporarily disable snapping

   // Reset display parameters...
   centerView();
   mDragSelecting = false;
   mUp = mDown = mLeft = mRight = mIn = mOut = false;
   mCreatingPoly = false;
   mCreatingPolyline = false;
   mDraggingObjects = false;
   mDraggingDockItem = NONE;
   mCurrentTeam = 0;
   mShowingReferenceShip = false;
   entryMode = EntryNone;

   itemToLightUp = NONE;     // Index to keep track of which item is litUp
   vertexToLightUp = NONE;

   mEditingSpecialAttrItem = NONE;
   mSpecialAttribute = NoAttribute;

   mSaveMsgTimer = 0;

   OGLCONSOLE_EnterKey(processEditorConsoleCommand);     // Setup callback for processing console commands


   actualizeScreenMode(false); 
}


void EditorUserInterface::syncUnmovedItems()
{
   mUnmovedItems = mItems;
}

void EditorUserInterface::onDeactivate()
{
   actualizeScreenMode(true);
}


void EditorUserInterface::onReactivate()
{
   mDraggingObjects = false;  

   mEditingSpecialAttrItem = NONE;     // Probably not necessary
   mSpecialAttribute = NoAttribute;

   if(mWasTesting)
   {
      mWasTesting = false;
      mSaveMsgTimer.clear();
   }

   remove("editor.tmp");      // Delete temp file

   if(mCurrentTeam >= mTeams.size())
      mCurrentTeam = 0;

   OGLCONSOLE_EnterKey(processEditorConsoleCommand);     // Restore callback for processing console commands

   actualizeScreenMode(true);
}


static Point sCenter;

// Called when we shift between windowed and fullscreen mode, before change is made
void EditorUserInterface::onPreDisplayModeChange()
{
   sCenter.set(mCurrentOffset.x - gScreenInfo.getGameCanvasWidth() / 2, mCurrentOffset.y - gScreenInfo.getGameCanvasHeight() / 2);
}

// Called when we shift between windowed and fullscreen mode, after change is made
void EditorUserInterface::onDisplayModeChange()
{
   // Recenter canvas -- note that canvasWidth may change during displayMode change
   mCurrentOffset.set(sCenter.x + gScreenInfo.getGameCanvasWidth() / 2, sCenter.y + gScreenInfo.getGameCanvasHeight() / 2);

   populateDock();               // If game type has changed, items on dock will change
}


Point EditorUserInterface::snapPointToLevelGrid(Point const &p)
{
   if(mSnapDisabled)
      return p;

   // First, find a snap point based on our grid
   F32 mulFactor, divFactor;
   if(mCurrentScale >= 100)
   {
      mulFactor = 10;
      divFactor = 0.1;
   }
   else
   {
      mulFactor = 2;
      divFactor = 0.5;
   }

   return Point(floor(p.x * mulFactor + 0.5) * divFactor, floor(p.y * mulFactor + 0.5) * divFactor);
}


Point EditorUserInterface::snapPoint(Point const &p, bool snapWhileOnDock)
{
   if(mouseOnDock() && !snapWhileOnDock) 
      return p;      // No snapping!

   Point snapPoint(p);

   if(mDraggingObjects)
   {
      // Mark all items being dragged as no longer being snapped -- only our primary "focus" item will be snapped
      for(S32 i = 0; i < mItems.size(); i++)
         if(mItems[i].selected)
            mItems[i].snapped = false;
   }
   
   // Turrets & forcefields: Snap to a wall edge as first (and only) choice
   if(mDraggingObjects &&
            (mItems[mSnapVertex_i].index == ItemTurret || mItems[mSnapVertex_i].index == ItemForceField))
      return mItems[mSnapVertex_i].snapEngineeredObject(snapPointToLevelGrid(p));


   F32 maxSnapDist = 100 / (mCurrentScale * mCurrentScale);
   F32 minDist = maxSnapDist;

   // Where will we be snapping things?
   bool snapToWallCorners = mDraggingObjects && mItems[mSnapVertex_i].index != ItemBarrierMaker;
   bool snapToWallEdges = mSnapVertex_i != NONE && mItems[mSnapVertex_i].geomType() == geomPoly;
   bool snapToNavZoneEdges = mSnapVertex_i != NONE && mItems[mSnapVertex_i].index == ItemNavMeshZone;
   bool snapToLevelGrid = !snapToNavZoneEdges && !mSnapDisabled;


   if(snapToLevelGrid)     // Lowest priority
   {
      snapPoint = snapPointToLevelGrid(p);
      minDist = snapPoint.distSquared(p);
   }


   // Now look for other things we might want to snap to
   for(S32 i = 0; i < mItems.size(); i++)
   {
      // Don't snap to selected items or items with selected verts
      if(mItems[i].selected || mItems[i].anyVertsSelected())    
         continue;

      for(S32 j = 0; j < mItems[i].vertCount(); j++)
      {
         F32 dist = mItems[i].vert(j).distSquared(p);
         if(dist < minDist)
         {
            minDist = dist;
            snapPoint.set(mItems[i].vert(j));
         }
      }
   }


   // Build a list of walls we might be snapping to if we're snapping to either the edges or corners
   static Vector<DatabaseObject *> foundObjects;

   if(snapToWallCorners || snapToWallEdges)
   {
      foundObjects.clear();
      mGridDatabase.findObjects(BarrierType, foundObjects, Rect(p, sqrt(minDist) * 2));   // minDist is dist squared
   }


   // Search for a corner to snap to - by using segment ends, we'll also look for intersections between segments
   if(snapToWallCorners)
   {
      for(S32 i = 0; i < foundObjects.size(); i++)
      {
         WallSegment *seg = dynamic_cast<WallSegment *>(foundObjects[i]);

         checkCornersForSnap(p, seg->edges, minDist, snapPoint);
      }
   }

   // If we're editing a vertex of a polygon, and if we're outside of some threshold distance, see if we can 
   // snap to the edge of a another zone or wall.  Decreasing value in minDist test will favor snapping to walls, 
   // decreasing(increasing??) it will require being closer to a wall to snap to it.
   if(minDist >= 90 / (mCurrentScale * mCurrentScale))
   {
      if(snapToWallEdges)
      {   
         // Check the edges of walls -- we'll reuse the list of walls we found earlier when looking for corners
         for(S32 i = 0; i < foundObjects.size(); i++)
         {
            WallSegment *seg = dynamic_cast<WallSegment *>(foundObjects[i]);
            checkEdgesForSnap(p, seg->edges, false, minDist, snapPoint);
         }
      }
   }

   // Will overwrite snapPoint if a zone corner or edge is found, thus prioritizing zone edges to other things when 
   // snapToNavZoneEdges is true
   if(snapToNavZoneEdges)
   {
      Rect vertexRect(snapPoint, .25); 
      Vector<WorldItem> candidates;
      Point edgeSnapPoint;
      F32 minCornerDist = maxSnapDist;
      F32 minEdgeDist = maxSnapDist;

      for(S32 i = 0; i < mItems.size(); i++)
      {
         if(mItems[i].index != ItemNavMeshZone || mItems[i].selected || mItems[i].anyVertsSelected())
            continue;

         if(!mItems[i].getExtent().intersectsOrBorders(vertexRect))
            continue;
         
         // To close the polygon, we need to repeat our first point at the end
         Vector<Point> verts = mItems[i].getVerts();     // Makes copy -- TODO: alter checkEdgesforsnap to make
                                                         // copy unnecessary, only needed when 3rd param is true
         verts.push_back(verts.first());

         // Combine these two checks in an awkward manner to reduce cost of checks above
         checkEdgesForSnap(p, verts, true, minEdgeDist, edgeSnapPoint);
         checkCornersForSnap(p, mItems[i].getVerts(), minCornerDist, snapPoint);

         if(minCornerDist == maxSnapDist && minEdgeDist < maxSnapDist)     // i.e. found edge, but not corner
            snapPoint.set(edgeSnapPoint);
      }
   }

   return snapPoint;
}


extern bool findNormalPoint(const Point &p, const Point &s1, const Point &s2, Point &closest);

// Checks for snapping against a series of edges defined by verts in A-B-C-D format if abcFormat is true, or A-B B-C C-D if false
// Sets snapPoint and minDist.  Returns index of closest segment found if closer than minDist.
S32 EditorUserInterface::checkEdgesForSnap(const Point &clickPoint, const Vector<Point> &verts, bool abcFormat,
                                           F32 &minDist, Point &snapPoint )
{
   S32 inc = abcFormat ? 1 : 2;   
   S32 segFound = NONE;
   Point closest;

   for(S32 i = 0; i < verts.size() - 1; i += inc)
      if(findNormalPoint(clickPoint, verts[i], verts[i+1], closest))    // closest is point on line where clickPoint normal intersects
      {
         F32 dist = closest.distSquared(clickPoint);
         if(dist < minDist)
         {
            minDist = dist;
            snapPoint.set(closest);   
            segFound = i;
         }
      }

   return segFound;
}


S32 EditorUserInterface::checkCornersForSnap(const Point &clickPoint, const Vector<Point> &verts, F32 &minDist, Point &snapPoint)
{
   S32 vertFound = NONE;

   for(S32 i = 0; i < verts.size(); i++)
   {
      F32 dist = verts[i].distSquared(clickPoint);
      if(dist < minDist)
      {
         minDist = dist;
         snapPoint.set(verts[i]);
         vertFound = i;
      }
   }

   return vertFound;
}


////////////////////////////////////
////////////////////////////////////
// Rendering routines

extern Color gErrorMessageTextColor;

static const Color grayedOutColorBright = Color(.5, .5, .5);
static const Color grayedOutColorDim = Color(.25, .25, .25);
static const S32 NO_NUMBER = -1;
static bool fillRendered = false;

// Render background snap grid
void EditorUserInterface::renderGrid()
{
   if(mShowingReferenceShip)
      return;   

   F32 colorFact = mSnapDisabled ? .5 : 1;
   if(mCurrentScale >= 100)
   {
      F32 gridScale = mCurrentScale * 0.1;      // Draw tenths

      F32 xStart = fmod(mCurrentOffset.x, gridScale);
      F32 yStart = fmod(mCurrentOffset.y, gridScale);

      glColor3f(.2 * colorFact, .2 * colorFact, .2 * colorFact);
      glBegin(GL_LINES);
         while(yStart < gScreenInfo.getGameCanvasHeight())
         {
            glVertex2f(0, yStart);
            glVertex2f(gScreenInfo.getGameCanvasWidth(), yStart);
            yStart += gridScale;
         }
         while(xStart < gScreenInfo.getGameCanvasWidth())
         {
            glVertex2f(xStart, 0);
            glVertex2f(xStart, gScreenInfo.getGameCanvasHeight());
            xStart += gridScale;
         }
      glEnd();
   }

   if(mCurrentScale >= 10)
   {
      F32 xStart = fmod(mCurrentOffset.x, mCurrentScale);
      F32 yStart = fmod(mCurrentOffset.y, mCurrentScale);

      glColor3f(0.4 * colorFact, 0.4 * colorFact, 0.4 * colorFact);
      glBegin(GL_LINES);
         while(yStart < gScreenInfo.getGameCanvasHeight())
         {
            glVertex2f(0, yStart);
            glVertex2f(gScreenInfo.getGameCanvasWidth(), yStart);
            yStart += mCurrentScale;
         }
         while(xStart < gScreenInfo.getGameCanvasWidth())
         {
            glVertex2f(xStart, 0);
            glVertex2f(xStart, gScreenInfo.getGameCanvasHeight());
            xStart += mCurrentScale;
         }
      glEnd();
   }

   glColor3f(0.7 * colorFact, 0.7 * colorFact, 0.7 * colorFact);
   glLineWidth(gLineWidth3);
   Point origin = convertLevelToCanvasCoord(Point(0,0));
   glBegin(GL_LINES );
      glVertex2f(0, origin.y);
      glVertex2f(gScreenInfo.getGameCanvasWidth(), origin.y);
      glVertex2f(origin.x, 0);
      glVertex2f(origin.x, gScreenInfo.getGameCanvasHeight());
   glEnd();
   glLineWidth(gDefaultLineWidth);
}


extern ScreenInfo gScreenInfo;

S32 getDockHeight(ShowMode mode)
{
   if(mode == ShowWallsOnly)
      return 62;
   else if(mode == NavZoneMode)
      return 92;
   else  // mShowMode == ShowAllObjects || mShowMode == ShowAllButNavZones
      return gScreenInfo.getGameCanvasHeight() - 2 * EditorUserInterface::vertMargin;
}


void EditorUserInterface::renderDock(F32 width)    // width is current wall width, used for displaying info on dock
{
   // Render item dock down RHS of screen
   const S32 canvasWidth = gScreenInfo.getGameCanvasWidth();
   const S32 canvasHeight = gScreenInfo.getGameCanvasHeight();

   S32 dockHeight = getDockHeight(mShowMode);

   for(S32 i = 1; i >= 0; i--)
   {
      glColor(i ? black : (mouseOnDock() ? yellow : white));       

      glBegin(i ? GL_POLYGON : GL_LINE_LOOP);
         glVertex2f(canvasWidth - DOCK_WIDTH - horizMargin, canvasHeight - vertMargin);
         glVertex2f(canvasWidth - horizMargin,              canvasHeight - vertMargin);
         glVertex2f(canvasWidth - horizMargin,              canvasHeight - vertMargin - dockHeight);
         glVertex2f(canvasWidth - DOCK_WIDTH - horizMargin, canvasHeight - vertMargin - dockHeight);
      glEnd();
   }

   // Draw coordinates on dock -- if we're moving an item, show the coords of the snap vertex, otherwise show the coords of the
   // snapped mouse position
   Point pos;
   if(mSnapVertex_i != NONE)
      pos = mItems[mSnapVertex_i].vert(mSnapVertex_j);
   else
      pos = snapPoint(convertCanvasToLevelCoord(mMousePos));

   F32 xpos = gScreenInfo.getGameCanvasWidth() - horizMargin - DOCK_WIDTH / 2;

   char text[50];
   glColor(white);
   dSprintf(text, sizeof(text), "%2.2f|%2.2f", pos.x, pos.y);
   drawStringc(xpos, gScreenInfo.getGameCanvasHeight() - vertMargin - 15, 8, text);

   // And scale
   dSprintf(text, sizeof(text), "%2.2f", mCurrentScale);
   drawStringc(xpos, gScreenInfo.getGameCanvasHeight() - vertMargin - 25, 8, text);

   // Show number of teams
   dSprintf(text, sizeof(text), "Teams: %d",  mTeams.size());
   drawStringc(xpos, gScreenInfo.getGameCanvasHeight() - vertMargin - 35, 8, text);

   glColor(mNeedToSave ? red : green);     // Color level name by whether it needs to be saved or not
   dSprintf(text, sizeof(text), "%s%s", mNeedToSave ? "*" : "", mEditFileName.substr(0, mEditFileName.find_last_of('.')).c_str());    // Chop off extension
   drawStringc(xpos, gScreenInfo.getGameCanvasHeight() - vertMargin - 45, 8, text);

   // And wall width as needed
   if(width != NONE)
   {
      glColor(white);
      dSprintf(text, sizeof(text), "Width: %2.0f", width);
      drawStringc(xpos, gScreenInfo.getGameCanvasHeight() - vertMargin - 55, 8, text);
   }
}


void EditorUserInterface::renderTextEntryOverlay()
{
   // Render id-editing overlay
   if(entryMode != EntryNone)
   {
      static const U32 fontsize = 16;
      static const S32 inset = 9;
      static const S32 boxheight = fontsize + 2 * inset;
      static const Color color(0.9, 0.9, 0.9);
      static const Color errorColor(1, 0, 0);

      bool errorFound = false;

      // Check for duplicate IDs if we're in ID entry mode
      if(entryMode == EntryID)
      {
         U32 id = atoi(mEntryBox.c_str());      // mEntryBox has digits only filter applied; ids can only be positive ints

         if(id != 0)    // Check for duplicates
         {
            for(S32 i = 0; i < mItems.size(); i++)
               if(mItems[i].id == id && !mItems[i].selected)
               {
                  errorFound = true;
                  break;
               }
         }
      }

      // Calculate box width
      S32 boxwidth = 2 * inset + getStringWidth(fontsize, mEntryBox.getPrompt().c_str()) + 
          mEntryBox.getMaxLen() * getStringWidth(fontsize, "-") + 25;

      // Render entry box    
      glEnableBlend;
      S32 xpos = (gScreenInfo.getGameCanvasWidth()  - boxwidth) / 2;
      S32 ypos = (gScreenInfo.getGameCanvasHeight() - boxheight) / 2;

      for(S32 i = 1; i >= 0; i--)
      {
         glColor(Color(.3,.6,.3), i ? .85 : 1);

         glBegin(i ? GL_POLYGON : GL_LINE_LOOP);
            glVertex2f(xpos,            ypos);
            glVertex2f(xpos + boxwidth, ypos);
            glVertex2f(xpos + boxwidth, ypos + boxheight);
            glVertex2f(xpos,            ypos + boxheight);
         glEnd();
      }
      glDisableBlend;

      xpos += inset;
      ypos += inset;
      glColor(errorFound ? errorColor : color);
      xpos += drawStringAndGetWidthf(xpos, ypos, fontsize, "%s ", mEntryBox.getPrompt().c_str());
      drawString(xpos, ypos, fontsize, mEntryBox.c_str());
      mEntryBox.drawCursor(xpos, ypos, fontsize);
   }
}


void EditorUserInterface::renderReferenceShip()
{
   // Render ship at cursor to show scale
   static F32 thrusts[4] =  { 1, 0, 0, 0 };

   glPushMatrix();
      glTranslatef(mMousePos.x, mMousePos.y, 0);
      glScalef(mCurrentScale / mGridSize, mCurrentScale / mGridSize, 1);
      glRotatef(90, 0, 0, 1);
      renderShip(red, 1, thrusts, 1, 5, false, false);
      glRotatef(-90, 0, 0, 1);

      // And show how far it can see
      F32 horizDist = Game::PlayerHorizVisDistance;
      F32 vertDist = Game::PlayerVertVisDistance;

      glEnableBlend;     // Enable transparency
      glColor4f(.5, .5, 1, .35);
      glBegin(GL_POLYGON);
         glVertex2f(-horizDist, -vertDist);
         glVertex2f(horizDist, -vertDist);
         glVertex2f(horizDist, vertDist);
         glVertex2f(-horizDist, vertDist);
      glEnd();
      glDisableBlend;

   glPopMatrix();
}


const char *getModeMessage(ShowMode mode)
{
   if(mode == ShowWallsOnly)
      return "Wall editing mode.  Hit Ctrl-A to change.";
   else if(mode == ShowAllButNavZones)
      return "NavMesh zones hidden.  Hit Ctrl-A to change.";
   else if(mode == NavZoneMode)
      return "NavMesh editing mode.  Hit Ctrl-A to change.";
   else     // Normal mode
      return "";
}


void EditorUserInterface::render()
{
   renderGrid();     // Render first, so it's at the bottom

   // Render any items generated by the levelgen script... these will be rendered below normal items. 
   glPushMatrix();
   setLevelToCanvasCoordConversion();

   glColor(Color(0,.25,0));
   for(S32 i = 0; i < mLevelGenItems.size(); i++)
      if(mLevelGenItems[i].index == ItemBarrierMaker)
         for(S32 j = 0; j < mLevelGenItems[i].extendedEndPoints.size(); j+=2)
            renderTwoPointPolygon(mLevelGenItems[i].extendedEndPoints[j], mLevelGenItems[i].extendedEndPoints[j+1], 
                                  mLevelGenItems[i].width / mGridSize / 2, GL_POLYGON);
   glPopMatrix();

   for(S32 i = 0; i < mLevelGenItems.size(); i++)
      renderItem(mLevelGenItems[i], -1, false, true);

   // Draw map items (teleporters, etc.) that are not being dragged, and won't have any text labels  (below the dock)
   for(S32 i = 0; i < mItems.size(); i++)
      if(!(mDraggingObjects && mItems[i].selected))
         renderItem(mItems[i], i, mEditingSpecialAttrItem == i, false);

   // Draw map items (teleporters, etc.) that are are selected and/or lit up, so label is readable (still below the dock)
   // Do this as a separate operation to ensure that these are drawn on top of those drawn above.
   for(S32 i = 0; i < mItems.size(); i++)
      if(mItems[i].selected || mItems[i].litUp)
         renderItem(mItems[i], i, mEditingSpecialAttrItem == i, false);


   // Go through and render any borders between navMeshZones -- these need to be rendered after the zones themselves so they
   // don't get covered up.
   if(showingNavZones())
   {
      glPushMatrix();  
         setLevelToCanvasCoordConversion();
         renderNavMeshBorders(zoneBorders, 1 / mGridSize);
      glPopMatrix();

      for(S32 i = 0; i < mItems.size(); i++)
         if(mItems[i].index == ItemNavMeshZone)
            renderLinePolyVertices(mItems[i], i); 
   }

   fillRendered = false;
   F32 width = NONE;

   if(mCreatingPoly || mCreatingPolyline)    // Draw geomLine features under construction
   {
      mNewItem.addVert(snapPoint(convertCanvasToLevelCoord(mMousePos)));
      glLineWidth(gLineWidth3);

      if(mCreatingPoly) // Wall
         glColor(SELECT_COLOR);
      else              // LineItem
         glColor(getTeamColor(mNewItem.team));

      mNewItem.renderPolyline();

      glLineWidth(gDefaultLineWidth);

      for(S32 j = mNewItem.vertCount() - 1; j >= 0; j--)      // Go in reverse order so that placed vertices are drawn atop unplaced ones
      {
         Point v = convertLevelToCanvasCoord(mNewItem.vert(j));

         // Draw vertices
         if(j == mNewItem.vertCount() - 1)           // This is our most current vertex
            renderVertex(HighlightedVertex, v, NO_NUMBER);
         else
            renderVertex(SelectedItemVertex, v, j);
      }
      mNewItem.deleteVert(mNewItem.vertCount() - 1);
   }
   // Since we're not constructing a barrier, if there are any barriers or lineItems selected, 
   // get the width for display at bottom of dock
   else  
   {
      for(S32 i = 0; i < mItems.size(); i++)
         if(mItems[i].hasWidth() && (mItems[i].selected || (mItems[i].litUp && vertexToLightUp == NONE)) )
         {
            width =  mItems[i].width;
            break;
         }
   }


   if(mShowMode == NavZoneMode)
   {
      glPushMatrix();  
         setLevelToCanvasCoordConversion();
  
         for(S32 i = 0; i < zoneBorders.size(); i++)
            renderNavMeshBorder(zoneBorders[i], 1 / mGridSize, yellow, .5, .05 * mGridSize);
      glPopMatrix();
   }


   if(mShowingReferenceShip)
      renderReferenceShip();
   else
      renderDock(width);

   // Draw map items (teleporters, etc.) that are being dragged  (above the dock).  But don't draw walls here, or
   // we'll lose our wall centernlines.
   for(S32 i = 0; i < mItems.size(); i++)
      if(mItems[i].index != ItemBarrierMaker && mDraggingObjects && mItems[i].selected)
         renderItem(mItems[i], i, mEditingSpecialAttrItem == i, false);

   if(mDragSelecting)      // Draw box for selecting items
   {
      glColor(white);
      Point downPos = convertLevelToCanvasCoord(mMouseDownPos);
      glBegin(GL_LINE_LOOP);
         glVertex2f(downPos.x,   downPos.y);
         glVertex2f(mMousePos.x, downPos.y);
         glVertex2f(mMousePos.x, mMousePos.y);
         glVertex2f(downPos.x,   mMousePos.y);
      glEnd();
   }

   // Render messages at bottom of screen
   if(mouseOnDock())    // On the dock?  If so, render help string if hovering over item
   {
      S32 hoverItem = findHitItemOnDock(mMousePos);

      if(hoverItem != NONE)
      {
         mDockItems[hoverItem].litUp = true;    // Will trigger a selection highlight to appear around dock item
         //mDockItems[hoverItem].selectVert(0);   // Ensures simpleLineItems get lit up on dock

         glColor3f(.1, 1, .1);
         // Center string between left side of screen and edge of dock
         S32 x = (S32)(gScreenInfo.getGameCanvasWidth() - horizMargin - DOCK_WIDTH - 
                       getStringWidth(15, itemDef[mDockItems[hoverItem].index].helpText)) / 2;
         drawString(x, gScreenInfo.getGameCanvasHeight() - vertMargin - 15, 15, itemDef[mDockItems[hoverItem].index].helpText);
      }
   }

   // Render dock items
   if(!mShowingReferenceShip)
      for(S32 i = 0; i < mDockItems.size(); i++)
      {
         renderItem(mDockItems[i], -1, false, false);
         mDockItems[i].litUp = false;
         //mDockItems[i].unselectVert(0);
      }

   if(mSaveMsgTimer.getCurrent())
   {
      F32 alpha = 1.0;
      if (mSaveMsgTimer.getCurrent() < 1000)
         alpha = (F32) mSaveMsgTimer.getCurrent() / 1000;

      glEnableBlend;
         glColor4f(mSaveMsgColor.r, mSaveMsgColor.g, mSaveMsgColor.b, alpha);
         drawCenteredString(gScreenInfo.getGameCanvasHeight() - vertMargin - 65, 25, mSaveMsg.c_str());
      glDisableBlend;
   }

   if(mWarnMsgTimer.getCurrent())
   {
      F32 alpha = 1.0;
      if (mWarnMsgTimer.getCurrent() < 1000)
         alpha = (F32) mWarnMsgTimer.getCurrent() / 1000;

      glEnableBlend;
         glColor4f(mWarnMsgColor.r, mWarnMsgColor.g, mWarnMsgColor.b, alpha);
         drawCenteredString(gScreenInfo.getGameCanvasHeight() / 4, 25, mWarnMsg1.c_str());
         drawCenteredString(gScreenInfo.getGameCanvasHeight() / 4 + 30, 25, mWarnMsg2.c_str());
      glDisableBlend;
   }

   if(mLevelErrorMsgs.size())
   {
      glColor(gErrorMessageTextColor);
      S32 ypos = vertMargin + 50;

      for(S32 i = 0; i < mLevelErrorMsgs.size(); i++)
      {
         drawCenteredString(ypos, 20, mLevelErrorMsgs[i].c_str());
         ypos += 25;
      }
   }

   glColor3f(0,1,1);
   drawCenteredString(vertMargin, 14, getModeMessage(mShowMode));

   renderTextEntryOverlay();

   renderConsole();  // Rendered last, so it's always on top
}


// Draw the vertices for a polygon or line item (i.e. walls)
void EditorUserInterface::renderLinePolyVertices(WorldItem &item, F32 alpha)
{
   if(mShowingReferenceShip)
      return;

   // Draw the vertices of the wall or the polygon area
   for(S32 j = 0; j < item.vertCount(); j++)
   {
      Point v = convertLevelToCanvasCoord(item.vert(j));

      if(item.vertSelected(j))
         renderVertex(SelectedVertex, v, j, alpha);             // Hollow yellow boxes with number
      else if(item.litUp && j == vertexToLightUp)
         renderVertex(HighlightedVertex, v, j, alpha);          // Hollow yellow boxes with number
      else if(item.selected || item.litUp || item.anyVertsSelected())
         renderVertex(SelectedItemVertex, v, j, alpha);         // Hollow red boxes with number
      else
         renderVertex(UnselectedItemVertex, v, NO_NUMBER, alpha, mCurrentScale > 35 ? 2 : 1);   // Solid red boxes, no number
   }
}


// Draw a vertex of a selected editor item
void EditorUserInterface::renderVertex(VertexRenderStyles style, Point v, S32 number, F32 alpha, S32 size)
{
   bool hollow = style == HighlightedVertex || style == SelectedVertex || style == SelectedItemVertex || style == SnappingVertex;

   // Fill the box with a dark gray to make the number easier to read
   if(hollow && number != NO_NUMBER)
   {
      glColor3f(.25, .25, .25);
      drawFilledSquare(v, size);
   }
      
   if(style == HighlightedVertex)
      glColor(HIGHLIGHT_COLOR, alpha);
   else if(style == SelectedVertex)
      glColor(SELECT_COLOR, alpha);
   else if(style == SnappingVertex)
      glColor(magenta, alpha);
   else
      glColor(red, alpha);

   drawSquare(v, size, !hollow);

   if(number != NO_NUMBER)     // Draw vertex numbers
   {
      glColor(white, alpha);
      drawStringf(v.x - getStringWidthf(6, "%d", number) / 2, v.y - 3, 6, "%d", number);
   }
}


////////////////////////////////////////
////////////////////////////////////////
/*
1. User creates wall by drawing line
2. Each line segment is converted to a series of endpoints, who's location is adjusted to improve rendering (extended)
3. Those endpoints are used to generate a series of WallSegment objects, each with 4 corners
4. Those corners are used to generate a series of edges on each WallSegment.  Initially, each segment has 4 corners
   and 4 edges.
5. Segments are intsersected with one another, punching "holes", and creating a series of shorter edges that represent
   the dark blue outlines seen in the game and in the editor.

If wall shape or location is changed steps 1-5 need to be repeated
If intersecting wall is changed, only steps 4 and 5 need to be repeated
If wall thickness is changed, steps 3-5 need to be repeated
*/

inline Point convertLevelToCanvasCoord(const Point &point, bool convert = true) 
{ 
   return gEditorUserInterface.convertLevelToCanvasCoord(point, convert); 
}

inline F32 getGridSize()
{
   return gEditorUserInterface.getGridSize();
}


static inline void labelSimpleLineItem(Point pos, U32 labelSize, const char *itemLabelTop, const char *itemLabelBottom)
{
   UserInterface::drawStringc(pos.x, pos.y + labelSize + 2, labelSize, itemLabelTop);
   UserInterface::drawStringc(pos.x, pos.y + 2 * labelSize + 5, labelSize, itemLabelBottom);
}


bool EditorUserInterface::showingNavZones()
{
   return (mShowMode == ShowAllObjects || mShowMode == NavZoneMode) && !mShowingReferenceShip;
}


extern void renderTriangulatedPolygonFill(const Vector<Point> &fill);
extern void renderPolygonOutline(const Vector<Point> &outline);

static const S32 asteroidDesign = 2;      // Design we'll use for all asteroids in editor

// Items are rendered in index order, so those with a higher index get drawn later, and hence, on top
void EditorUserInterface::renderItem(WorldItem &item, S32 index, bool isBeingEdited, bool isScriptItem)
{
   Point pos, dest;
   const S32 labelSize = 9;      // Size to label items we're hovering over
   const S32 instrSize = 9;      // Size of instructions for special items
   const S32 attrSize = 10;
   const F32 alpha = isScriptItem ? .6 : 1;     // Script items will appear somewhat translucent

   bool hideit = (mShowMode == ShowWallsOnly) && !(mShowingReferenceShip && !item.mDockItem);

   Color drawColor;
   if(hideit)
         glColor(grayedOutColorBright, alpha);
   else if(item.selected)
      drawColor = SELECT_COLOR;
   else if(item.litUp, alpha)
      drawColor = HIGHLIGHT_COLOR;
   else  // Normal
      drawColor = Color(.75, .75, .75);

   pos = convertLevelToCanvasCoord(item.vert(0), !item.mDockItem);

   bool showLetter = true;    // In a few cases, we want to disable rendering of letters on objects

   glEnableBlend;        // Enable transparency

   // Render snapping vertex; if it is the same as a highlighted vertex, highlight will overwrite this
   if(mSnapVertex_i != NONE && mSnapVertex_j != NONE && mItems[mSnapVertex_i].selected && !mShowingReferenceShip)
      // Render snapping vertex as hollow magenta box
      renderVertex(SnappingVertex, convertLevelToCanvasCoord(mItems[mSnapVertex_i].vert(mSnapVertex_j)), 
                   NO_NUMBER, alpha);   

   // Draw "two-point" items
   if(item.geomType() == geomSimpleLine && (mShowMode != ShowWallsOnly || item.mDockItem || mShowingReferenceShip))    
   {
      if(item.mDockItem)
         dest = item.vert(1);
      else
         dest = convertLevelToCanvasCoord(item.vert(1));

      Color itemColor;

      // Draw the tail end of the object, which will also be visible on the dock
      if(hideit)
         itemColor = grayedOutColorDim;
      else if(item.index == ItemTeleporter)
         itemColor = green;
      else if(item.index == ItemSpeedZone)
         itemColor = red;
      else if(item.index == ItemTextItem)
         itemColor = blue;
     
      if(item.mDockItem)
      {
         glColor(itemColor, alpha);
         drawFilledSquare(pos, 5);    // Draw origin of item to give user something to grab on the dock
      }

      // Override drawColor for this special case
      if(item.anyVertsSelected())
         drawColor = SELECT_COLOR;

      if(item.index == ItemTeleporter || item.index == ItemSpeedZone || item.index == ItemTextItem)
      {
         if(!item.mDockItem)
         {
            if(!mShowingReferenceShip)
            {
               glColor(itemColor, ((item.selected || item.litUp || item.anyVertsSelected()) && !isBeingEdited) ? alpha : .25 * alpha);
               drawFilledSquare(pos, 5);            // Draw origin of item (square box)

               for(S32 i = 1; i >= 0; i--)
               {  
                  // Draw heavy colored line with colored core
                  glLineWidth(i ? gLineWidth4 : gDefaultLineWidth);                

                  F32 ang = pos.angleTo(dest);
                  const F32 al = 15;                // Length of arrow-head
                  const F32 angoff = .5;            // Pitch of arrow-head prongs

                  glBegin(GL_LINES);
                     glVertex2f(dest.x, dest.y);    // Draw arrow-head
                     glVertex2f(dest.x - cos(ang + angoff) * al, dest.y - sin(ang + angoff) * al);
                     glVertex2f(dest.x, dest.y);
                     glVertex2f(dest.x - cos(ang - angoff) * al, dest.y - sin(ang - angoff) * al);

                     // Draw highlight on 2nd pass if item is selected, but not while it's being edited
                     if(!i && (item.selected || item.litUp) && !isBeingEdited)
                        glColor(drawColor);

                     glVertex(pos);                 // Draw connecting line
                     glVertex(dest);
                  glEnd();
               }

               glLineWidth(gDefaultLineWidth);      // Restore default value
            }

            glPushMatrix();
               setTranslationAndScale(pos);
               if(item.index == ItemTeleporter)
               {
                  Vector<Point> dest;
                  dest.push_back(convertLevelToCanvasCoord(item.vert(1)));

				     if(mShowingReferenceShip)
				        renderTeleporter(pos, 0, true, gClientGame->getCurrentTime(), 1, Teleporter::TELEPORTER_RADIUS, 1, dest, false);
				     else
				     {
                     glColor(green);
                     glLineWidth(gLineWidth3);
                     drawPolygon(pos, 12, Teleporter::TELEPORTER_RADIUS, 0);
                     glLineWidth(gDefaultLineWidth);
                  }
               }
               else if(item.index == ItemSpeedZone)
                  renderSpeedZone(SpeedZone::generatePoints(pos, convertLevelToCanvasCoord(item.vert(1))), gClientGame->getCurrentTime());

            glPopMatrix();

            if(item.index == ItemTextItem)
               renderTextItem(item, 1);
         
            // If this is a textItem, and either the item or either vertex is selected, draw the text
            if(!item.mDockItem && itemDef[item.index].hasText)
            {
               F32 txtSize = renderTextItem(item, alpha);

               if(isBeingEdited)
                  item.lineEditor.drawCursorAngle(pos.x, pos.y, txtSize, pos.angleTo(dest));

               if(item.selected && mSpecialAttribute == NoAttribute)
               {
                  glColor(white);
                  drawStringf_2pt(pos, dest, instrSize, -22, "[Enter] to edit text");
               }
            }
            else if(!item.mDockItem && item.index == ItemSpeedZone)      // Special labeling for speedzones
            {
               if((item.selected && mEditingSpecialAttrItem == NONE) || isBeingEdited)
               {
                  glColor((mSpecialAttribute != GoFastSnap) ? white : inactiveSpecialAttributeColor);
                  drawStringf_2pt(pos, dest, attrSize, 10, "Speed: %d", item.speed);

                  glColor((mSpecialAttribute != GoFastSpeed) ? white : inactiveSpecialAttributeColor);
                  drawStringf_2pt(pos, dest, attrSize, -2, "Snapping: %s", item.boolattr ? "On" : "Off");

                  glColor(white);

                  // TODO: This block should be moved to WorldItem
                  const char *msg, *instr;

                  if(mSpecialAttribute == NoAttribute)
                  {
                     msg = "[Enter] to edit speed";
                     instr = "";
                  }
                  else if(mSpecialAttribute == GoFastSpeed)
                  {
                     msg = "[Enter] to edit snapping";
                     instr = "Up/Dn to change speed";
                  }
                  else if(mSpecialAttribute == GoFastSnap)
                  {
                     msg = "[Enter] to stop editing";
                     instr = "Up/Dn to toggle snapping";
                  }
                  else
                  {
                     msg = "???";
                     instr = "???";
                  }

                  drawStringf_2pt(pos, dest, instrSize, -22, msg);
                  drawStringf_2pt(pos, dest, instrSize, -22 - instrSize - 2, instr);
               }
            }
         }

         // If either end is selected, draw a little white box around it.  Will do this on and off dock.
         if(item.vertSelected(0) || (item.litUp && vertexToLightUp == 0))
         {
            glColor(drawColor, alpha);
            drawSquare(pos, 7);

            if(!item.mDockItem)
               labelSimpleLineItem(pos, labelSize, itemDef[item.index].onScreenName, item.getOriginBottomLabel());
         }
         else if(item.vertSelected(1) || (item.litUp && vertexToLightUp == 1))
         {
            glColor(drawColor, alpha);
            drawSquare(dest, 7);

            labelSimpleLineItem(dest, labelSize, itemDef[item.index].onScreenName, item.getDestinationBottomLabel());
         }

         if(item.mDockItem && item.litUp)     // Highlight handle on dock when hovering over it
         {
            glColor(HIGHLIGHT_COLOR);
            drawSquare(pos, 8);
         }
      }
   }
   else if(item.index == ItemLineItem)
   {
      glColor(getTeamColor(item.team), alpha);
      item.renderPolylineCenterline(alpha);

      renderLinePolyVertices(item, alpha);
   }
   else if(item.index == ItemBarrierMaker)  
   {
      if(!fillRendered)    // All walls need to be rendered at the same time, but we only want to do them once
      {
         wallSegmentManager.renderWalls(true, alpha);

         fillRendered = true;  // Prevent re-rendering of walls this pass
      }  

      if(!mShowingReferenceShip)
         item.renderPolylineCenterline(alpha);

      renderLinePolyVertices(item, alpha);
   } 
   else if(item.geomType() == geomPoly)    // Draw regular line objects and poly objects
   {
      // Hide everything in ShowWallsOnly mode, and hide navMeshZones in ShowAllButNavZones mode, 
      // unless it's a dock item or we're showing the reference ship.  NavMeshZones are hidden when reference ship is shown
      if((mShowMode != ShowWallsOnly && (showingNavZones() && item.index == ItemNavMeshZone || item.index != ItemNavMeshZone)) &&
            !mShowingReferenceShip || item.mDockItem || mShowingReferenceShip && item.index != ItemNavMeshZone)   
      {
         // A few items will get custom colors; most will get their team color
         if(hideit)
            glColor(grayedOutColorDim, alpha);
         else if(item.index == ItemNavMeshZone)
            glColor(item.isConvex() ? green : red, alpha * .5);
         else if(item.index == ItemNexus)
            glColor(gNexusOpenColor, alpha);      // Render Nexus items in pale green to match the actual thing
         else
            glColor(getTeamColor(item.team), alpha);


         F32 ang = angleOfLongestSide(item.getVerts());

         if(item.mDockItem)    // Old school rendering on the dock
         {
            glPushMatrix();
               setLevelToCanvasCoordConversion(!item.mDockItem);

               // Render the fill triangles
               renderTriangulatedPolygonFill(item.fillPoints);

               glColor(hideit ? grayedOutColorBright : drawColor, alpha);
               glLineWidth(gLineWidth3);  
               renderPolygonOutline(item.getVerts());
               glLineWidth(gDefaultLineWidth);        // Restore line width
            glPopMatrix();

            // Let's add a label
            glColor(hideit ? grayedOutColorBright : drawColor, alpha);
            renderPolygonLabel(convertLevelToCanvasCoord(item.centroid, !item.mDockItem), ang, labelSize, 
                               itemDef[item.index].onScreenName);
         }
         else
         {
            glPushMatrix();  
               setLevelToCanvasCoordConversion();

               if(item.index == ItemLoadoutZone)
                  renderLoadoutZone(getTeamColor(item.team), item.getVerts(), item.fillPoints, 
                                    item.centroid * mGridSize, ang, 1 / mGridSize);

               else if(item.index == ItemGoalZone)
                  renderGoalZone(getTeamColor(item.team), item.getVerts(), item.fillPoints,  
                                    item.centroid * mGridSize, ang, false, 0, item.mScore, 1 / mGridSize);

               else if(item.index == ItemNexus)
                  renderNexus(item.getVerts(), item.fillPoints, 
                                    item.centroid * mGridSize, ang, true, 0, 1 / mGridSize);

               else if(item.index == ItemNavMeshZone)
                  renderNavMeshZone(item.getVerts(), item.fillPoints, item.centroid, mShowMode == NavZoneMode ? -2 : -1, 
                                    item.isConvex(), item.selected);

               // If item is selected, and we're not in preview mode, draw a border highlight
               if(!mShowingReferenceShip && (item.selected || item.litUp))
               {        
                  glColor(hideit ? grayedOutColorBright : drawColor, alpha);
                  glLineWidth(gLineWidth3);  
                  renderPolygonOutline(item.getVerts());
                  glLineWidth(gDefaultLineWidth);        // Restore line width
               }

            glPopMatrix();
         }
      }

      // NavMeshZone verts will be drawn elsewhere
      if((item.geomType() == geomLine || mShowMode != ShowWallsOnly) && 
                  !item.mDockItem && item.index != ItemNavMeshZone)  
         renderLinePolyVertices(item, alpha);                               
   }
 
   else if(mShowMode != ShowWallsOnly || 
                        item.mDockItem || mShowingReferenceShip)   // Draw the various point items
   {
      Color c = hideit ? grayedOutColorDim : getTeamColor(item.team);           // And a color (based on team affiliation)

      // TODO: This should be on WorldItem
      if(item.index == ItemFlag)             // Draw flag
      {
         glPushMatrix();
            glTranslatef(pos.x, pos.y, 0);
            glScalef(0.6, 0.6, 1);
            renderFlag(0, 0, c, hideit ? grayedOutColorDim : 0, alpha);
         glPopMatrix();
      }
      else if(item.index == ItemFlagSpawn)    // Draw flag spawn point
      {
         if(mShowingReferenceShip && !item.mDockItem)
         {
            // Do nothing -- hidden in preview mode
         }
         else
         {
            glPushMatrix();
               glTranslatef(pos.x+1, pos.y, 0);
               glScalef(0.4, 0.4, 1);
               renderFlag(0, 0, c, hideit ? grayedOutColorDim : 0, alpha);

               glColor(hideit ? grayedOutColorDim : white, alpha);
               drawCircle(Point(-4,0), 26);
            glPopMatrix();
         }
      }
      else if(item.index == ItemAsteroidSpawn)    // Draw asteroid spawn point
      {
         if(mShowingReferenceShip && !item.mDockItem)
         {
            // Do nothing -- hidden in preview mode
         }
         else
         {
            glPushMatrix();
               glTranslatef(pos.x, pos.y, 0);
               glScalef(0.8, 0.8, 1);
               renderAsteroid(Point(0,0), asteroidDesign, .1, hideit ? grayedOutColorDim : 0, alpha);

               glColor(hideit ? grayedOutColorDim : white, alpha);
               drawCircle(Point(0, 0), 13);
            glPopMatrix();
         }
      }
      else if(item.index == ItemBouncyBall)   // Draw testitem
      {
         if(!item.mDockItem)
         {
            glPushMatrix();
               setTranslationAndScale(pos);
               renderTestItem(pos, alpha);
            glPopMatrix();
         }
         else     // Dock item rendering
         {
            glColor(hideit ? grayedOutColorBright : Color(.7,.7,.7), alpha);
            drawPolygon(pos, 7, 8, 0);
         }
      }
      else if(item.index == ItemAsteroid)   // Draw asteroid
      {
         if(!item.mDockItem)
         {
            glPushMatrix();
               setTranslationAndScale(pos);
               renderAsteroid(pos, asteroidDesign, asteroidRenderSize[0], alpha);
            glPopMatrix();
         }
         else     // Dock item rendering
            renderAsteroid(pos, asteroidDesign, .1, hideit ? grayedOutColorDim : 0, alpha);
      }

      else if(item.index == ItemResource)   // Draw resourceItem
      {
         if(!item.mDockItem)
         {
            glPushMatrix();
               setTranslationAndScale(pos);
               renderResourceItem(pos, alpha);
            glPopMatrix();
         }
         else     // Dock item rendering
             renderResourceItem(pos, .4, hideit ? grayedOutColorDim : 0, alpha);
      }
      else if(item.index == ItemSoccerBall)  // Soccer ball, obviously
      {
         if(!item.mDockItem)
         {
            glPushMatrix();
               setTranslationAndScale(pos);
               renderSoccerBall(pos, alpha);
            glPopMatrix();
         }
         else
         {
            glColor(hideit ? grayedOutColorBright : Color(.7,.7,.7), alpha);
            drawCircle(pos, 9);
         }
      }
      else if(item.index == ItemMine)  // And a mine
      {
         if(mShowingReferenceShip && !item.mDockItem) 
         {
             glPushMatrix();
               setTranslationAndScale(pos);
               renderMine(pos, true, true);
            glPopMatrix();
         }
         else
         {
            glColor(hideit ? grayedOutColorDim : Color(.7,.7,.7), alpha);
            drawCircle(pos, 9);

            glColor(hideit ? grayedOutColorDim : Color(.1,.3,.3), alpha);
            drawCircle(pos, 5);
         }
      }
      else if(item.index == ItemSpyBug)  // And a spy bug
      {
         glColor(hideit ? grayedOutColorDim : Color(.7,.7,.7), alpha);
         drawCircle(pos, 9);

         glColor(hideit ? grayedOutColorDim : getTeamColor(item.team), alpha);
         drawCircle(pos, 5);

         // And show how far it can see... unless, of course, it's on the dock, and assuming the tab key has been pressed
         if(!item.mDockItem && mShowingReferenceShip && (item.selected || item.litUp))
         {
            glColor(getTeamColor(item.team), .25 * alpha);

            U32 size = U32(mCurrentScale / mGridSize * F32(gSpyBugRange));

            drawFilledSquare(pos, size);
         }
      }

      else if(item.index == ItemRepair)
         renderRepairItem(pos, true, hideit ? grayedOutColorDim : 0, alpha);

      else if(item.index == ItemEnergy)
         renderEnergyItem(pos, true, hideit ? grayedOutColorDim : 0, alpha);

      else if(item.index == ItemTurret || item.index == ItemForceField)
      { 
         if(item.renderFull(mCurrentScale))      
         {
            if(item.index == ItemTurret)
            {
               glPushMatrix();
                  setTranslationAndScale(pos);
                  renderTurret(c, pos, item.normal, true, 1.0, item.normal.ATAN2());
               glPopMatrix();
            }
            else   
            {
               glPushMatrix();
                  setTranslationAndScale(pos);
                  renderForceFieldProjector(pos, item.normal, c, true);
               glPopMatrix();

               F32 scaleFact = 1 / mGridSize; 

               glPushMatrix();
                  setLevelToCanvasCoordConversion();

                  renderForceField(ForceFieldProjector::getForceFieldStartPoint(item.vert(0), item.normal, scaleFact), 
                                   item.forceFieldEnd, c, true, scaleFact);
               glPopMatrix();
            }

            showLetter = false;
         }

         else renderGenericItem(pos, c, alpha);  
      }

      else                             // Draw anything else
      {
         if(mShowingReferenceShip && !item.mDockItem)
         {
            // Do nothing
         }
         else
            renderGenericItem(pos, c, alpha);
      }


      // If this is an item that has a repop attribute, and the item is selected, draw the text
      if(!item.mDockItem && itemDef[item.index].hasRepop)
      {
         if(mShowMode != ShowWallsOnly && 
            ((item.selected || item.litUp) &&  mEditingSpecialAttrItem == NONE) &&
            (item.index != ItemFlagSpawn || !strcmp(mGameType, "HuntersGameType")) || isBeingEdited)
         {
            glColor(white);

            const char *healword = (item.index == ItemTurret || item.index == ItemForceField) ? "10% Heal" : 
                                   ((item.index == ItemFlagSpawn || item.index == ItemAsteroidSpawn) ? "Spawn Time" : "Regen");

            Point offset = item.getSelectionOffset(mCurrentScale).rotate(item.normal.ATAN2()) * mCurrentScale;

            S32 radius = item.getRadius(mCurrentScale);
            offset.y += ((radius == NONE || item.mDockItem) ? 10 : (radius * mCurrentScale / mGridSize)) - 6;

            if(item.repopDelay == 0)
               drawStringfc(pos.x + offset.x, pos.y + offset.y + attrSize, attrSize, "%s: Disabled", healword);
            else
               drawStringfc(pos.x + offset.x, pos.y + offset.y + 10, attrSize, "%s: %d sec%c", healword, item.repopDelay, item.repopDelay != 1 ? 's' : 0);


            const char *msg;

            if(mSpecialAttribute == NoAttribute)
               msg = "[Enter] to edit";
            else if(isBeingEdited && mSpecialAttribute == RepopDelay)
               msg = "Up/Dn to change";
            else
               msg = "???";
            drawStringc(pos.x + offset.x, pos.y + offset.y + instrSize + 13, instrSize, msg);
         }
      }

      // If we have a turret, render it's range (if tab is depressed)
      if(item.index == ItemTurret)
      {
         if(!item.mDockItem && mShowingReferenceShip && (item.selected || item.litUp))
         {
            glColor(getTeamColor(item.team), .25 * alpha);

            F32 size = mCurrentScale / mGridSize * (gWeapons[WeaponTurret].projLiveTime * gWeapons[WeaponTurret].projVelocity / 1000);
            drawFilledSquare(pos, size);
         }
      }

      // Draw highlighted border around item if selected
      if(mShowMode != ShowWallsOnly && (item.selected || item.litUp))  
      {
         // Dock items are never selected, but they can be highlighted
         Point pos = item.mDockItem ? item.vert(0) : convertLevelToCanvasCoord(item.vert(0));   

         glColor(drawColor);

         S32 radius = item.getRadius(mCurrentScale);
         S32 highlightRadius = (radius == NONE || item.mDockItem) ? 10 : S32(radius * mCurrentScale / mGridSize + 0.5f);

         Point ctr = pos + item.getSelectionOffset(mCurrentScale).rotate(item.normal.ATAN2()) * mCurrentScale;   

         drawSquare(ctr, highlightRadius);
      }

      if(item.mDockItem || showLetter && !(itemDef[item.index].specialTabKeyRendering && mShowingReferenceShip)) 
      {
         char letter = itemDef[item.index].letter;    // Get letter to represent object

         // Mark the item with a letter, unless we're showing the reference ship
         if(letter)
         {
            S32 vertOffset = 8;
            if (letter >= 'a' && letter <= 'z')    // Better position lowercase letters
               vertOffset = 10;

            glColor(hideit ? grayedOutColorBright : drawColor, alpha);
            drawStringf(pos.x - getStringWidthf(15, "%c", letter) / 2, pos.y - vertOffset, 15, "%c", letter);
         }
      }

      // Add a label if we're hovering over it (or not, unless it's on the dock, where we've already labeled our items)
      // For the moment, we need special handling for turrets & forcefields :-(
      if(mShowMode != ShowWallsOnly && (item.selected || item.litUp) && 
            itemDef[item.index].onScreenName && !item.mDockItem &&
            !((item.index == ItemTurret || item.index == ItemForceField) && item.renderFull(mCurrentScale)))
      {
         glColor(drawColor);
         drawStringc(pos.x, pos.y - labelSize * 2 - 5, labelSize, itemDef[item.index].onScreenName); // Label on top
      }
   }

   // Label our dock items
   if(item.mDockItem && item.geomType() != geomPoly)      // Polys are already labeled internally
   {
      glColor(hideit ? grayedOutColorBright : drawColor);
      F32 maxy = -F32_MAX;
      for(S32 j = 0; j < item.vertCount(); j++)
         if (item.vert(j).y > maxy)
            maxy = item.vert(j).y;

      // Make some label position adjustments
      if(item.geomType() == geomSimpleLine)
         maxy -= 2;
      else if(item.index == ItemSoccerBall)
         maxy += 1;
      drawString(pos.x - getStringWidth(labelSize, itemDef[item.index].onDockName)/2, maxy + 8, labelSize, itemDef[item.index].onDockName);
   }

   glDisableBlend;
}


// TODO: Make this a method on WorldItem
F32 EditorUserInterface::renderTextItem(WorldItem &item, F32 alpha)
{
   // Use this more precise F32 calculation of size for smoother interactive rendering.
   // We'll use U32 approximation in game.
   glColor(getTeamColor(item.team), alpha);
   F32 txtSize = item.textSize / mGridSize * mCurrentScale;

   // wrap?
   Point pos  = convertLevelToCanvasCoord(item.vert(0));
   Point dest = convertLevelToCanvasCoord(item.vert(1));

   drawAngleString_fixed(pos.x, pos.y, txtSize, pos.angleTo(dest), item.lineEditor.c_str());

   return txtSize;
}


void EditorUserInterface::renderGenericItem(Point pos, Color c, F32 alpha)
{
   glColor(c, alpha);
   drawFilledSquare(pos, 8);  // Draw filled box in which we'll put our letter
}


// Will set the correct translation and scale to render items at correct location and scale as if it were a real level.
// Unclear enough??
void EditorUserInterface::setTranslationAndScale(const Point &pos)
{
   glScalef(mCurrentScale / mGridSize, mCurrentScale / mGridSize, 1);
   glTranslatef(pos.x * mGridSize / mCurrentScale - pos.x, pos.y * mGridSize / mCurrentScale - pos.y, 0);
}


// Replaces the need to do a convertLevelToCanvasCoord on every point before rendering
void EditorUserInterface::setLevelToCanvasCoordConversion(bool convert)
{
   if(convert)
   {
      glScalef(mCurrentScale, mCurrentScale, 1);
      glTranslatef(mCurrentOffset.x / mCurrentScale, mCurrentOffset.y / mCurrentScale, 0);
   }
} 


void EditorUserInterface::clearSelection()
{
   for(S32 i = 0; i < mItems.size(); i++)
      unselectItem(i);
}


void EditorUserInterface::unselectItem(S32 i)
{
   mItems[i].selected = false;
   mItems[i].litUp = false;

   mItems[i].unselectVerts();
}


S32 EditorUserInterface::countSelectedItems()
{
   S32 count = 0;
   for(S32 i = 0; i < mItems.size(); i++)
      if(mItems[i].selected)
         count++;
   return count;
}


static S32 getNextItemId()
{
   static S32 nextItemId = 0;
   return nextItemId++;
}


// Paste items on the clipboard
void EditorUserInterface::pasteSelection()
{
   if(mDraggingObjects)     // Pasting while dragging can cause crashes!!
      return;

   S32 itemCount = mClipboard.size();

    if(!itemCount)         // Nothing on clipboard, nothing to do
      return;

   saveUndoState(mItems);  // So we can undo the paste

   clearSelection();       // Only the pasted items should be selected

   Point pos = snapPoint(convertCanvasToLevelCoord(mMousePos));

   // Diff between mouse pos and original object (item will be pasted such that the first vertex is at mouse pos)
   Point offset = pos - mClipboard[0].vert(0);    

   for(S32 i = 0; i < itemCount; i++)
   {
      mItems.push_back(mClipboard[i]);
      mItems.last().mId = getNextItemId();
      mItems.last().selected = true;
      for(S32 j = 0; j < mItems.last().vertCount(); j++)
         mItems.last().setVert(mItems.last().vert(j) += offset, j);
      mItems.last().onGeomChanged();
   }
   mItems.sort(geometricSort);
   validateLevel();
   mNeedToSave = true;
   autoSave();
}


// Copy selection to the clipboard
void EditorUserInterface::copySelection()
{
   if(!anyItemsSelected())
      return;

   bool alreadyCleared = false;

   S32 itemCount = mItems.size();
   for(S32 i = 0; i < itemCount; i++)
   {
      if(mItems[i].selected)
      {
         WorldItem newItem = mItems[i];
         newItem.selected = false;
         for(S32 j = 0; j < newItem.vertCount(); j++)
            newItem.vert(j) += Point(0.5, 0.5);

         if(!alreadyCleared)  // Make sure we only purge the existing clipboard if we'll be putting someting new there
         {
            mClipboard.clear();
            alreadyCleared = true;
         }
         mClipboard.push_back(newItem);
      }
   }
}


// Expand or contract selection by scale
void EditorUserInterface::scaleSelection(F32 scale)
{
   if(!anyItemsSelected() || scale < .01 || scale == 1)    // Apply some sanity checks
      return;

   saveUndoState(mItems);

   // Find center of selection
   Point min, max;                        
   computeSelectionMinMax(min, max);
   Point ctr = (min + max) * 0.5;

   if(scale > 1 && min.distanceTo(max) * scale > 50)    // If walls get too big, they'll bog down the db
      return;

   for(S32 i = 0; i < mItems.size(); i++)
      if(mItems[i].selected)
         mItems[i].scale(ctr, scale);

   mNeedToSave = true;
   autoSave();
}


// Rotate selected objects around their center point by angle
void EditorUserInterface::rotateSelection(F32 angle)
{
   if(!anyItemsSelected())
      return;

   saveUndoState(mItems);

   for(S32 i = 0; i < mItems.size(); i++)
   {
      if(mItems[i].selected)
         mItems[i].rotateAboutPoint(Point(0,0), angle);
   }
   mNeedToSave = true;
   autoSave();
}


void EditorUserInterface::computeSelectionMinMax(Point &min, Point &max)
{
   min.set(F32_MAX, F32_MAX);
   max.set(F32_MIN, F32_MIN);

   for(S32 i = 0; i < mItems.size(); i++)
   {
      if(mItems[i].selected)
      {
         WorldItem &item = mItems[i];
         for(S32 j = 0; j < item.vertCount(); j++)
         {
            Point v = item.vert(j);

            if(v.x < min.x)   min.x = v.x;
            if(v.x > max.x)   max.x = v.x;
            if(v.y < min.y)   min.y = v.y;
            if(v.y > max.y)   max.y = v.y;
         }
      }
   }
}

// Set the team affiliation of any selected items
void EditorUserInterface::setCurrentTeam(S32 currentTeam)
{
   mCurrentTeam = currentTeam;
   bool anyOK = false;
   bool anyChanged = false;

   Vector<WorldItem> undoItems = mItems;      // Create a snapshot so we can later undo if we do anything here

   if(currentTeam >= mTeams.size())
   {
      char msg[255];
      if(mTeams.size() == 1)
         dSprintf(msg, sizeof(msg), "Only 1 team has been configured.");
      else
         dSprintf(msg, sizeof(msg), "Only %d teams have been configured.", mTeams.size());
      gEditorUserInterface.setWarnMessage(msg, "Hit [F2] to configure teams.");
      return;
   }

   // Update all dock items to reflect new current team
   for(S32 i = 0; i < mDockItems.size(); i++)
   {
      if(!itemDef[mDockItems[i].index].hasTeam)
         continue;

      if(currentTeam == TEAM_NEUTRAL && !itemDef[mDockItems[i].index].canBeNeutral)
         continue;

      if(currentTeam == TEAM_HOSTILE && !itemDef[mDockItems[i].index].canBeHostile)
         continue;

      mDockItems[i].team = currentTeam;
   }

   for(S32 i = 0; i < mItems.size(); i++)
   {
      if(mItems[i].selected /*|| (mItems[i].litUp && vertexToLightUp == NONE)*/)
      {
         if(!itemDef[mItems[i].index].hasTeam)
            continue;

         if(currentTeam == TEAM_NEUTRAL && !itemDef[mItems[i].index].canBeNeutral)
            continue;

         if(currentTeam == TEAM_HOSTILE && !itemDef[mItems[i].index].canBeHostile)
            continue;

         mItems[i].team = currentTeam;
         anyChanged = true;
      }
   }

   // Overwrite any warnings set above.  If we have a group of items selected, it makes no sense to show a
   // warning if one of those items has the team set improperly.  The warnings are more appropriate if only
   // one item is selected, or none of the items are given a valid team setting.

   if(anyChanged)
   {
      gEditorUserInterface.setWarnMessage("", "");
      saveUndoState(undoItems);      // If anything changed, push our temp state onto the undo stack
      validateLevel();
      mNeedToSave = true;
      autoSave();
   }
}


void EditorUserInterface::flipSelectionHorizontal()
{
   if(!anyItemsSelected())
      return;

   saveUndoState(mItems);

   Point min, max;
   computeSelectionMinMax(min, max);
   for(S32 i = 0; i < mItems.size(); i++)
      if(mItems[i].selected)
         mItems[i].flipHorizontal(min, max);

   mNeedToSave = true;
   autoSave();
}


void EditorUserInterface::flipSelectionVertical()
{
   if(!anyItemsSelected())
      return;

   saveUndoState(mItems);

   Point min, max;
   computeSelectionMinMax(min, max);

   for(S32 i = 0; i < mItems.size(); i++)
      if(mItems[i].selected)
         mItems[i].flipVertical(min, max);

   mNeedToSave = true;
   autoSave();
}


void EditorUserInterface::findHitVertex(Point canvasPos, S32 &hitItem, S32 &hitVertex)
{
   const S32 VERTEX_HIT_RADIUS = 8;
   hitItem = NONE;
   hitVertex = NONE;

   if(mEditingSpecialAttrItem != NONE)    // If we're editing a text special attribute, disable this functionality
      return;

   for(S32 x = 1; x >= 0; x--)    // Two passes... first for selected item, second for all items
   {
      for(S32 i = mItems.size() - 1; i >= 0; i--)     // Reverse order so we get items "from the top down"
      { 
         if(x && !mItems[i].selected && !mItems[i].anyVertsSelected())
            continue;

         if(mShowMode == ShowWallsOnly && mItems[i].index != ItemBarrierMaker ||
            mShowMode == NavZoneMode && mItems[i].index != ItemNavMeshZone )        // Only select walls in CTRL-A mode
            continue;

         if(mItems[i].geomType() <= geomPoint)
            continue;

         for(S32 j = mItems[i].vertCount() - 1; j >= 0; j--)
         {
            Point v = convertLevelToCanvasCoord(mItems[i].vert(j));
            if(fabs(v.x - canvasPos.x) < VERTEX_HIT_RADIUS && fabs(v.y - canvasPos.y) < VERTEX_HIT_RADIUS)
            {
               hitItem = i;
               hitVertex = j;
               return;
            }
         }
      }
   }
}


static const S32 POINT_HIT_RADIUS = 9;
static const S32 EDGE_HIT_RADIUS = 6;

void EditorUserInterface::findHitItemAndEdge()
{

   mItemHit = NONE;
   mEdgeHit = NONE;

   if(mEditingSpecialAttrItem  != NONE)              // If we're editing special attributes, disable this functionality
      return;

   // Do this in two passes -- the first we only consider selected items, the second pass will consider all targets.
   // This will give priority to moving vertices of selected items
   for(S32 x = 1; x >= 0; x--)      // x will be true the first time through, false the second time
   {
      for(S32 i = mItems.size() - 1; i >= 0; i--)     // Go in reverse order to prioritize items drawn on top
      {
         if(x && !mItems[i].selected && !mItems[i].anyVertsSelected())     // First pass is for selected items only
            continue;

         if(mShowMode == ShowWallsOnly && mItems[i].index != ItemBarrierMaker)     // Only select walls in CTRL-A mode...
            continue;                                                              // ...so if it's not a wall, proceed to next item

         if(mShowMode == NavZoneMode && mItems[i].index != ItemNavMeshZone)        // Only select zones in CTRL-A mode...
            continue;                                                              // ...so if it's not a bot nav zone, proceed to next item

         if(mItems[i].geomType() == geomPoint)
         {
            S32 radius = mItems[i].getRadius(mCurrentScale);
            S32 targetRadius = (radius == NONE) ? POINT_HIT_RADIUS : S32(radius * mCurrentScale / mGridSize + 0.5f);

            F32 ang = mItems[i].normal.ATAN2();
            Point pos = convertLevelToCanvasCoord(mItems[i].vert(0) + mItems[i].getSelectionOffset(mCurrentScale).rotate(ang));

            if(fabs(mMousePos.x - pos.x) < targetRadius && fabs(mMousePos.y - pos.y) < targetRadius)
            {
               mItemHit = i;
               return;
            }
         }

         // Make a copy of the items vertices that we can add to in the case of a loop
         Vector<Point> verts = mItems[i].getVerts();    

         if(mItems[i].geomType() == geomPoly)   // Add first point to the end to create last side on poly
            verts.push_back(verts.first());

         Point p1 = convertLevelToCanvasCoord(verts[0]);
         Point closest;
         
         for(S32 j = 0; j < verts.size() - 1; j++)
         {
            Point p2 = convertLevelToCanvasCoord(verts[j+1]);
            
            if(findNormalPoint(mMousePos, p1, p2, closest))
            {
               F32 distance = (mMousePos - closest).len();
               if(distance < EDGE_HIT_RADIUS)
               {
                  mEdgeHit = j;
                  mItemHit = i;
                  return;
               }
            }
            p1.set(p2);
         }
      }
   }

   if(mShowMode == ShowWallsOnly) 
      return;

   // If we're still here, it means we didn't find anything yet.  Make one more pass, and see if we're in any polys.
   // This time we'll loop forward, though I don't think it really matters.
   for(S32 i = 0; i < mItems.size(); i++)
   {
      if(mShowMode == ShowAllButNavZones && mItems[i].index == ItemNavMeshZone)     // Don't select NavMeshZones while they're hidden
         continue;
      if(mShowMode == NavZoneMode && mItems[i].index != ItemNavMeshZone)
         continue;

      if(mItems[i].geomType() == geomPoly)
      {
         Vector<Point> verts;
         for(S32 j = 0; j < mItems[i].vertCount(); j++)
            verts.push_back(convertLevelToCanvasCoord(mItems[i].vert(j)));

         if(PolygonContains2(verts.address(), verts.size(), mMousePos))
         {
            mItemHit = i;
            return;
         }
      }
   }
}


S32 EditorUserInterface::findHitItemOnDock(Point canvasPos)
{
   if(mShowMode == ShowWallsOnly)           // Only add dock items when objects are visible
      return NONE;

   if(mEditingSpecialAttrItem != NONE)    // If we're editing a text item, disable this functionality
      return NONE;

   for(S32 i = mDockItems.size() - 1; i >= 0; i--)     // Go in reverse order because the code we copied did ;-)
   {
      Point pos = mDockItems[i].vert(0);

      if(fabs(canvasPos.x - pos.x) < POINT_HIT_RADIUS && fabs(canvasPos.y - pos.y) < POINT_HIT_RADIUS)
         return i;
   }

   // Now check for polygon interior hits
   for(S32 i = 0; i < mDockItems.size(); i++)
      if(mDockItems[i].geomType() == geomPoly)
      {
         Vector<Point> verts;
         for(S32 j = 0; j < mDockItems[i].vertCount(); j++)
            verts.push_back(mDockItems[i].vert(j));

         if(PolygonContains2(verts.address(),verts.size(), canvasPos))
            return i;
      }

   return NONE;
}


void EditorUserInterface::onMouseMoved(S32 x, S32 y)
{
   onMouseMoved();
}


void EditorUserInterface::onMouseMoved()
{
   mMousePos.set(gScreenInfo.getMousePos());

   if(mCreatingPoly || mCreatingPolyline)
      return;

   S32 vertexHit, vertexHitPoly;

   findHitVertex(mMousePos, vertexHitPoly, vertexHit);
   findHitItemAndEdge();

   if(itemToLightUp != NONE)
   {
      TNLAssert(itemToLightUp < mItems.size(), "Index out of bounds!");
      if(itemToLightUp >= mItems.size())     // Just in case...
         return;

      mItems[itemToLightUp].litUp = false;
   }
   vertexToLightUp = NONE;
   itemToLightUp = NONE;

   // Hit a vertex that wasn't already selected
   if(vertexHit != NONE && !mItems[vertexHitPoly].vertSelected(vertexHit))   
   {
      vertexToLightUp = vertexHit;
      itemToLightUp = vertexHitPoly;
   }

   // We hit an item that wasn't already selected
   else if(mItemHit != NONE && !mItems[mItemHit].selected)                   
   {
      itemToLightUp = mItemHit;
   }

   // Check again, and take a point object in preference to a vertex
   if(mItemHit != NONE && !mItems[mItemHit].selected && mItems[mItemHit].geomType() == geomPoint)  
   {
      itemToLightUp = mItemHit;
      vertexToLightUp = NONE;
   }

   if(itemToLightUp != NONE)
      mItems[itemToLightUp].litUp = true;

   bool showMoveCursor = (vertexHitPoly != NONE || vertexHit != NONE || mItemHit != NONE || mEdgeHit != NONE || 
                         (mouseOnDock() && findHitItemOnDock(mMousePos) != NONE));

   findSnapVertex();

   glutSetCursor((showMoveCursor && !mShowingReferenceShip) ? GLUT_CURSOR_SPRAY : GLUT_CURSOR_RIGHT_ARROW);
}


void EditorUserInterface::onMouseDragged(S32 x, S32 y)
{
   mMousePos.set(gScreenInfo.getMousePos());

   if(mCreatingPoly || mCreatingPolyline || mDragSelecting || mEditingSpecialAttrItem != NONE)
      return;

   if(mDraggingDockItem != NONE)      // We just started dragging an item off the dock
   {
      // Offset lets us drag an item out from the dock by an amount offset from the 0th vertex.  This makes placement
      // seem more natural.
      Point offset;

      if(mDockItems[mDraggingDockItem].geomType() == geomPoly)
         offset.set(.25, .15);
      else if(mDockItems[mDraggingDockItem].index == ItemSpeedZone)
         offset.set(.15, 0);
      else if(mDockItems[mDraggingDockItem].index == ItemTextItem)     
         offset.set(.4, 0);

      // Instantiate object so we are in essence dragging a non-dock item
      Point pos = snapPoint(convertCanvasToLevelCoord(mMousePos) - offset, true);

      // Gross struct avoids extra construction
      WorldItem item =
         (mDockItems[mDraggingDockItem].geomType() == geomPoly) ?
         // For polygon items, try to match proportions of the dock rendering while ensuring all corners snap.  
         WorldItem(mDockItems[mDraggingDockItem].index, pos, mDockItems[mDraggingDockItem].team, false, .7, .4) :
         // Non polygon item --> size only used for geomSimpleLine items (teleport et al), ignored for geomPoints
         WorldItem(mDockItems[mDraggingDockItem].index, pos, mDockItems[mDraggingDockItem].team, false, 1, 0);
  
      clearSelection();            // No items are selected...
      item.selected = true;        // ...except for the new one
      mItems.push_back(item);      // Add our new item to the master item list
      mItems.sort(geometricSort);  // So things will render in the proper order
      mDraggingDockItem = NONE;    // Because now we're dragging a real item
      syncUnmovedItems();          // So we know where things were so we know where to render them while being dragged
      validateLevel();             // Check level for errors

      // Because we sometimes have trouble finding an item when we drag it off the dock, after it's been sorted,
      // we'll manually set mItemHit based on the selected item, which will always be the one we just added.
      mEdgeHit = NONE;
      for(S32 i = 0; i < mItems.size(); i++)
         if(mItems[i].selected)
         {
            mItemHit = i;
            break;
         }
   } // if(draggingDockItem)


   findSnapVertex();
   if(mSnapVertex_i == NONE || mSnapVertex_j == NONE)
      return;

   mDraggingObjects = true;

   const Point &origPoint = mUnmovedItems[mSnapVertex_i].vert(mSnapVertex_j);

   Point delta;

   // The thinking here is that for large items -- walls, polygons, etc., we may grab an item far from its snap vertex, and we
   // want to factor that offset into our calculations.  For point items (and vertices), we don't really care about any slop
   // in the selection, and we just want the damn thing where we put it.
   // (*origPoint - mMouseDownPos) represents distance from item's snap vertex where we "grabbed" it
   if(mItems[mSnapVertex_i].geomType() == geomPoint || (mItemHit != NONE && mItems[mItemHit].anyVertsSelected()))
      delta = snapPoint(convertCanvasToLevelCoord(mMousePos))  - origPoint;
   else
      delta = snapPoint(convertCanvasToLevelCoord(mMousePos) + origPoint - mMouseDownPos) - origPoint;

   // Update the locations of all items we're moving to show them being dragged.  Note that an item cannot be
   // selected if one of its vertices are.
   for(S32 i = 0; i < mItems.size(); i++)
   {
      // Skip selected item if it's a turret or forcefield -- snapping will have already been applied
      if(i == mSnapVertex_i && (mItems[i].index == ItemForceField || mItems[i].index == ItemTurret) && mItems[i].snapped)
         continue;

      for(S32 j = 0; j < mItems[i].vertCount(); j++)
         if(mItems[i].selected || mItems[i].vertSelected(j))
         {
            mItems[i].setVert(mUnmovedItems[i].vert(j) + delta, j);

            // If we are dragging a vertex, and not the entire item, we are changing the geom, so notify the item
            if(mItems[i].vertSelected(j))
               mItems[i].onGeomChanging();     
         }

      if(mItems[i].selected)     
         mItems[i].onItemDragging();      // Make sure this gets run after we've updated the item's location
   }
}


// Sets mSnapVertex_i and mSnapVertex_j based on the vertex closest to the cursor that is part of the selected set
// What we really want is the closest vertex in the closest feature
void EditorUserInterface::findSnapVertex()
{
   F32 closestDist = F32_MAX;

   if(mDraggingObjects)    // Don't change snap vertex once we're dragging
      return;

   mSnapVertex_i = NONE;
   mSnapVertex_j = NONE;

   Point mouseLevelCoord = convertCanvasToLevelCoord(mMousePos);

   // If we have a hit item, and it's selected, find the closest vertex in the item
   if(mItemHit != NONE && mItems[mItemHit].selected)   
   {
      // If we've hit an edge, restrict our search to the two verts that make up that edge
      if(mEdgeHit != NONE)
      {
         mSnapVertex_i = mItemHit;     // Regardless of vertex, this is our hit item
         S32 v1 = mEdgeHit;
         S32 v2 = mEdgeHit + 1;

         // Handle special case of looping item
         if(mEdgeHit + 1 == mItems[mItemHit].vertCount())
            v2 = 0;

         if( mItems[mItemHit].vert(v1).distSquared(mouseLevelCoord) < 
             mItems[mItemHit].vert(v2).distSquared(mouseLevelCoord) )
            mSnapVertex_j = v1;
         else     // Second vertex is closer
            mSnapVertex_j = v2;

         return;
      }

      // Didn't hit an edge... find the closest vertex anywhere in the item
      for(S32 j = 0; j < mItems[mItemHit].vertCount(); j++)
      {
         F32 dist = mItems[mItemHit].vert(j).distSquared(mouseLevelCoord);

         if(dist < closestDist)
         {
            closestDist = dist;
            mSnapVertex_i = mItemHit;
            mSnapVertex_j = j;
         }
      }
      return;
   } 

   // Otherwise, we don't have a selected hitItem -- look for a selected vertex
   for(S32 i = 0; i < mItems.size(); i++)
      for(S32 j = 0; j < mItems[i].vertCount(); j++)
      {
         // If we find a selected vertex, there will be only one, and this is our snap point
         if(mItems[i].vertSelected(j))
         {
            mSnapVertex_i = i;
            mSnapVertex_j = j;
            return;     
         }
      }
}


void EditorUserInterface::deleteSelection(bool objectsOnly)
{
   if(mDraggingObjects)     // No deleting while we're dragging, please...
      return;

   if(!anythingSelected())  // Nothing to delete
      return;

   Vector<WorldItem> items = mItems;
   bool deleted = false;

   for(S32 i = 0; i < mItems.size(); i++)
   {
      if(mItems[i].selected)
      {  
         // Since indices change as items are deleted, this will keep incorrect items from being deleted
         if(mItems[i].litUp)
            itemToLightUp = NONE;
         else
            itemToLightUp--;

         deleteItem(i);
         deleted = true;
         i--;

      }
      else if(!objectsOnly)      // Deleted any selected vertices
      {
         bool geomChanged = false;

         for(S32 j = 0; j < mItems[i].vertCount(); j++) 
         {
            if(mItems[i].vertSelected(j))
            {
               mItems[i].deleteVert(j);
               deleted = true;
               geomChanged = true;
               j--;
            }
         }

         // Deleted last vertex, or item can't lose a vertex... it must go!
         if(mItems[i].vertCount() == 0 || (mItems[i].geomType() == geomSimpleLine && mItems[i].vertCount() < 2)
                                       || (mItems[i].geomType() == geomLine && mItems[i].vertCount() < 2)
                                       || (mItems[i].geomType() == geomPoly && mItems[i].vertCount() < 2))
         {
            deleteItem(i);
            deleted = true;
            i--;
         }
         else if(geomChanged)
            mItems[i].onGeomChanged();

      }  // else if(!objectsOnly) 
   }  // for

   if(deleted)
   {
      saveUndoState(items);
      validateLevel();
      mNeedToSave = true;
      autoSave();

      itemToLightUp = NONE;     // In case we just deleted a lit item
      vertexToLightUp = NONE;
   }
}

// Increase selected wall thickness by amt
void EditorUserInterface::incBarrierWidth(S32 amt)
{
   if(!mLastUndoStateWasBarrierWidthChange)
      saveUndoState(mItems); 

   for(S32 i = 0; i < mItems.size(); i++)
      if(mItems[i].selected)
         mItems[i].increaseWidth(amt);

   mLastUndoStateWasBarrierWidthChange = true;
}


// Decrease selected wall thickness by amt
void EditorUserInterface::decBarrierWidth(S32 amt)
{
   if(!mLastUndoStateWasBarrierWidthChange)
      saveUndoState(mItems); 

   for(S32 i = 0; i < mItems.size(); i++)
      if(mItems[i].selected)
         mItems[i].decreaseWidth(amt);

   mLastUndoStateWasBarrierWidthChange = true;
}


// Split wall/barrier on currently selected vertex/vertices
void EditorUserInterface::splitBarrier()
{
   bool split = false;

   Vector<WorldItem> undoItems = mItems;      // Create a snapshot so we can later undo if we do anything here

   for(S32 i = 0; i < mItems.size(); i++)
      if(mItems[i].geomType() == geomLine)
          for(S32 j = 1; j < mItems[i].vertCount() - 1; j++)     // Can't split on end vertices!
            if(mItems[i].vertSelected(j))
            {
               split = true;

               // Create a poor man's copy
               WorldItem newItem;
               newItem.index = mItems[i].index;
               newItem.team = -1;
               newItem.width = mItems[i].width;

               for(S32 k = j; k < mItems[i].vertCount(); k++) 
               {
                  newItem.addVert(mItems[i].vert(k));
                  if (k > j)
                  {
                     mItems[i].deleteVert(k);     // Don't delete j == k vertex -- it needs to remain as the final vertex of the old wall
                     k--;
                  }
               }

               mItems.push_back(newItem);

               // Tell the new segments that they have new geometry
               mItems[i].onGeomChanged();
               mItems.last().onGeomChanged();

               // And get them in the right order
               mItems.sort(geometricSort);         
               goto done2;                         // Yes, gotos are naughty, but they just work so well sometimes...
            }
done2:
   if(split)
   {
      clearSelection();
      saveUndoState(undoItems);
      mNeedToSave = true;
      autoSave();
   }
}


// Join two or more sections of wall that have coincident end points.  Will ignore invalid join attempts.
void EditorUserInterface::joinBarrier()
{
   S32 joinedItem = NONE;

   Vector<WorldItem> undoItems = mItems;      // Create a snapshot so we can later undo if we do anything here

   for(S32 i = 0; i < mItems.size()-1; i++)
      if(mItems[i].geomType() == geomLine && (mItems[i].selected /*|| (mItems[i].litUp && vertexToLightUp == NONE)*/))
      {
         for(S32 j = i + 1; j < mItems.size(); j++)
         {
            if(mItems[j].index == mItems[i].index && (mItems[j].selected /*|| (mItems[j].litUp && vertexToLightUp == NONE)*/))
            {
               if(mItems[i].vert(0).distanceTo(mItems[j].vert(0)) < .01)    // First vertices are the same  1 2 3 | 1 4 5
               {
                  joinedItem = i;

                  for(S32 a = 1; a < mItems[j].vertCount(); a++)             // Skip first vertex, because it would be a dupe
                     mItems[i].addVertFront(mItems[j].vert(a));

                  deleteItem(j);
                  i--;  j--;
                  if(itemToLightUp > j)
                     itemToLightUp--;
               }
               // First vertex conincides with final vertex 3 2 1 | 5 4 3
               else if(mItems[i].vert(0).distanceTo(mItems[j].vert(mItems[j].vertCount()-1)) < .01)     
               {
                  joinedItem = i;
                  for(S32 a = mItems[j].vertCount()-2; a >= 0; a--)
                     mItems[i].addVertFront(mItems[j].vert(a));

                  deleteItem(j);
                  i--;  j--;
                  if(itemToLightUp > j)
                     itemToLightUp--;

               }
               // Last vertex conincides with first 1 2 3 | 3 4 5
               else if(mItems[i].vert(mItems[i].vertCount()-1).distanceTo(mItems[j].vert(0)) < .01)     
               {
                  joinedItem = i;

                  for(S32 a = 1; a < mItems[j].vertCount(); a++)  // Skip first vertex, because it would be a dupe         
                     mItems[i].addVert(mItems[j].vert(a));

                  deleteItem(j);
                  i--;  j--;
                  if(itemToLightUp > j)
                     itemToLightUp--;

               }
               else if(mItems[i].vert(mItems[i].vertCount()-1).distanceTo(mItems[j].vert(mItems[j].vertCount()-1)) < .01)     // Last vertices coincide  1 2 3 | 5 4 3
               {
                  joinedItem = i;
                  for(S32 a = mItems[j].vertCount()-2; a >= 0; a--)
                     mItems[i].addVert(mItems[j].vert(a));

                  deleteItem(j);
                  i--;  j--;
                  if(itemToLightUp > j)
                     itemToLightUp--;
               }
            }
         }
      }

   if(joinedItem != NONE)
   {
      clearSelection();
      saveUndoState(undoItems);
      mNeedToSave = true;
      autoSave();
      mItems[joinedItem].onGeomChanged();
   }
}


void EditorUserInterface::deleteItem(S32 itemIndex)
{
   if(mItems[itemIndex].index == ItemBarrierMaker)
   {
      // Need to recompute boundaries of any intersecting walls
      wallSegmentManager.invalidateIntersectingSegments(&mItems[itemIndex]);  // Mark intersecting segments invalid
      wallSegmentManager.deleteSegments(mItems[itemIndex].mId);               // Delete the segments associated with the wall
      wallSegmentManager.recomputeInvalidWallSegmentIntersections();          // Recompute intersections
      recomputeAllEngineeredItems();         // Really only need to recompute items that were attached to deleted wall... but we
                                             // don't yet have a method to do that, and I'm feeling lazy at the moment
   }
   else if(mItems[itemIndex].index == ItemNavMeshZone)
      deleteBorderSegs(mItems[itemIndex].mId);

   //else
   //   mItems[itemIndex].removeFromDatabase();

   mItems.erase(itemIndex);

   // Reset a bunch of things
   mSnapVertex_i = NONE;
   mSnapVertex_j = NONE;
   itemToLightUp = NONE;
   mUnmovedItems.erase(itemIndex);

   onMouseMoved();   // Reset cursor  
}


void EditorUserInterface::insertNewItem(GameItems itemType)
{
   if(mShowMode == ShowWallsOnly || mDraggingObjects)     // No inserting when items are hidden or being dragged!
      return;

   clearSelection();
   saveUndoState(mItems);

   Point pos = snapPoint(convertCanvasToLevelCoord(mMousePos));
   S32 team = -1;

   // Get team affiliation from dockItem of same type
   for(S32 i = 0; i < mDockItems.size(); i++)
      if (mDockItems[i].index == itemType)
      {
         team = mDockItems[i].team;
         break;
      }

   mItems.push_back(WorldItem(itemType, pos, team, false, 1, 1));

   mItems.sort(geometricSort);
   validateLevel();
   mNeedToSave = true;
   autoSave();
}


static LineEditor getNewEntryBox(string value, string prompt, S32 length, LineEditor::LineEditorFilter filter)
{
   LineEditor entryBox(length);
   entryBox.setPrompt(prompt);
   entryBox.setString(value);
   entryBox.setFilter(filter);

   return entryBox;
}


void EditorUserInterface::centerView()
{
   if(mItems.size() || mLevelGenItems.size())
   {
      F32 minx =  F32_MAX,   miny =  F32_MAX;
      F32 maxx = -F32_MAX,   maxy = -F32_MAX;

      for(S32 i = 0; i < mItems.size(); i++)
         for(S32 j = 0; j < mItems[i].vertCount(); j++)
         {
            if(mItems[i].vert(j).x < minx)
               minx = mItems[i].vert(j).x;
            if(mItems[i].vert(j).x > maxx)
               maxx = mItems[i].vert(j).x;
            if(mItems[i].vert(j).y < miny)
               miny = mItems[i].vert(j).y;
            if(mItems[i].vert(j).y > maxy)
               maxy = mItems[i].vert(j).y;
         }

      for(S32 i = 0; i < mLevelGenItems.size(); i++)
         for(S32 j = 0; j < mLevelGenItems[i].vertCount(); j++)
         {
            if(mLevelGenItems[i].vert(j).x < minx)
               minx = mLevelGenItems[i].vert(j).x;
            if(mLevelGenItems[i].vert(j).x > maxx)
               maxx = mLevelGenItems[i].vert(j).x;
            if(mLevelGenItems[i].vert(j).y < miny)
               miny = mLevelGenItems[i].vert(j).y;
            if(mLevelGenItems[i].vert(j).y > maxy)
               maxy = mLevelGenItems[i].vert(j).y;
         }

      // If we have only one point object in our level, the following will correct
      // for any display weirdness.
      if(minx == maxx && miny == maxy)    // i.e. a single point item
      {
         mCurrentScale = MIN_SCALE;
         mCurrentOffset.set(gScreenInfo.getGameCanvasWidth() / 2  - mCurrentScale * minx, 
                            gScreenInfo.getGameCanvasHeight() / 2 - mCurrentScale * miny);
      }
      else
      {
         F32 midx = (minx + maxx) / 2;
         F32 midy = (miny + maxy) / 2;

         mCurrentScale = min(gScreenInfo.getGameCanvasWidth() / (maxx - minx), gScreenInfo.getGameCanvasHeight() / (maxy - miny));
         mCurrentScale /= 1.3;      // Zoom out a bit
         mCurrentOffset.set(gScreenInfo.getGameCanvasWidth() / 2  - mCurrentScale * midx, 
                            gScreenInfo.getGameCanvasHeight() / 2 - mCurrentScale * midy);
      }
   }
   else     // Put (0,0) at center of screen
   {
      mCurrentScale = 100;
      mCurrentOffset.set(gScreenInfo.getGameCanvasWidth() / 2, gScreenInfo.getGameCanvasHeight() / 2);
   }
}


// Save selection mask, which can be retrieved later, as long as mItems hasn't changed.
void EditorUserInterface::saveSelection()
{
   mSelectedSet = Selection(mItems);
}


// Restore selection mask
void EditorUserInterface::restoreSelection()
{
   mSelectedSet.restore(mItems);
}


U32 EditorUserInterface::getNextAttr(S32 item)       // Not sure why this fn can't return a SpecialAttribute...  hrm...
{
   // Advance to the next attribute. If we were at NoAttribute, start with the first.
   U32 curr = (mSpecialAttribute == NoAttribute) ? 0 : mSpecialAttribute + 1;

   // Find next attribute that applies to selected object
   for(U32 i = curr; i <= NoAttribute; i++)
   {
      if( ((i == Text) && itemDef[mItems[item].index].hasText) ||
          ((i == RepopDelay) && itemDef[mItems[item].index].hasRepop) ||
          ((i == GoFastSpeed || i == GoFastSnap) && mItems[item].index == ItemSpeedZone) ||  
          (i == NoAttribute ) )
         return i;
   }
   return NoAttribute;      // Should never get here...
}


// Gets run when user exits special-item editing mode
void EditorUserInterface::doneEditingSpecialItem(bool saveChanges)
{
   // Find any other selected items of the same type of the item we just edited, and update their values too

   if(!saveChanges)
      undo(false);
   else
      for(S32 i = 0; i < mItems.size(); i++)
      {
         if(i == mEditingSpecialAttrItem)
            continue;
         else if(mItems[i].selected && mItems[i].index == mItems[mEditingSpecialAttrItem].index)
         {
            // We'll ignore text here, because that really makes less sense
            mItems[i].repopDelay = mItems[mEditingSpecialAttrItem].repopDelay;
            mItems[i].speed = mItems[mEditingSpecialAttrItem].speed;
            mItems[i].boolattr = mItems[mEditingSpecialAttrItem].boolattr;
            mItems[i].onAttrsChanged();
         }
      }

   mEditingSpecialAttrItem = NONE;
   mSpecialAttribute = NoAttribute;
}

   
extern string itos(S32);
extern string ftos(F32, S32);

// Handle key presses
void EditorUserInterface::onKeyDown(KeyCode keyCode, char ascii)
{
   if(OGLCONSOLE_ProcessBitfighterKeyEvent(keyCode, ascii))      // Pass the key on to the console for processing
      return;

   if(entryMode != EntryNone)
      textEntryKeyHandler(keyCode, ascii);

   else if(keyCode == KEY_ENTER)       // Enter - Edit props
      itemPropertiesEnterKeyHandler();

   // This first section is the key handlers for when we're editing the special attributes of an item.  Regular
   // key actions are handled below.
   else if(mEditingSpecialAttrItem != NONE)
      specialAttributeKeyHandler(keyCode, ascii);

   // Regular key handling from here on down
   else if(getKeyState(KEY_SHIFT) && keyCode == KEY_0)  // Shift-0 -> Set team to hostile
      setCurrentTeam(-2);

   else if(ascii == '#' || ascii == '!')
   {
      S32 selected = NONE;

      // Find first selected item, and just work with that.  Unselect the rest.
      for(S32 i = 0; i < mItems.size(); i++)
      {
         if(mItems[i].selected)
         {
            if(selected == NONE)
            {
               selected = i;
               continue;
            }
            else
               mItems[i].selected = false;
         }
      }

      if(selected == NONE)      // Nothing selected, nothing to do!
         return;

      mEntryBox = getNewEntryBox(mItems[selected].id <= 0 ? "" : itos(mItems[selected].id), 
                                 "Item ID:", 10, LineEditor::digitsOnlyFilter);
      entryMode = EntryID;
   }

   else if(ascii >= '0' && ascii <= '9')           // Change team affiliation of selection with 0-9 keys
   {
      setCurrentTeam(ascii - '1');
      return;
   }

   // Ctrl-left click is same as right click for Mac users
   else if(keyCode == MOUSE_RIGHT || (keyCode == MOUSE_LEFT && getKeyState(KEY_CTRL)))
   {
      if(getKeyState(MOUSE_LEFT) && !getKeyState(KEY_CTRL))         // Prevent weirdness
         return;  

      mMousePos.set(gScreenInfo.getMousePos());

      if(mCreatingPoly || mCreatingPolyline)
      {
         if(mNewItem.vertCount() >= gMaxPolygonPoints)     // Limit number of points in a polygon/polyline
            return;
         //else
         mNewItem.addVert(snapPoint(convertCanvasToLevelCoord(mMousePos)));
         mNewItem.onGeomChanging();
         return;
      }

      //saveUndoState(mItems);     // Save undo state before we clear the selection
      clearSelection();          // Unselect anything currently selected

      // Can only add new vertices by clicking on item's edge, not it's interior (for polygons, that is)
      if(mEdgeHit != NONE && mItemHit != NONE && (mItems[mItemHit].geomType() == geomLine ||
                                                  mItems[mItemHit].geomType() >= geomPoly   ))
      {
         if(mItems[mItemHit].vertCount() >= gMaxPolygonPoints)     // Polygon full -- can't add more
            return;

         mMostRecentState = mItems;
         Point newVertex = snapPoint(convertCanvasToLevelCoord(mMousePos));      // adding vertex w/ right-mouse

         // Insert an extra vertex at the mouse clicked point, and then select it.
         mItems[mItemHit].insertVert(newVertex, mEdgeHit + 1);
         mItems[mItemHit].selectVert(mEdgeHit + 1);

         // Do the same for the mUnmovedItems list, to keep it in sync with mItems,
         // which allows us to drag our vertex around without wierd snapping action
         mUnmovedItems[mItemHit].insertVert(newVertex, mEdgeHit + 1);
         mUnmovedItems[mItemHit].selectVert(mEdgeHit + 1);

         // Alert the item that its geometry is changing
         mItems[mItemHit].onGeomChanging();

         mMouseDownPos = newVertex;
         
      }
      else     // Start creating a new poly or new polyline (tilda key + right-click ==> start polyline)
      {
         if(mShowMode != NavZoneMode)     // Unless we're in navZoneMode, that is
         {
            GameItems type;

            if(getKeyState(KEY_TILDE))
            {
               mCreatingPolyline = true;
               type = ItemLineItem;
            }
            else
            {
               mCreatingPoly = true;
               type = ItemBarrierMaker;
            }

            mNewItem = WorldItem(type, snapPoint(convertCanvasToLevelCoord(mMousePos)), TEAM_NEUTRAL, false,
                                 type == ItemBarrierMaker ? Barrier::BarrierWidth : 2);
         }
      }
   }
   else if(keyCode == MOUSE_LEFT)
   {
      if(getKeyState(MOUSE_RIGHT))          // Prevent weirdness
         return;

      mDraggingDockItem = NONE;
      mMousePos.set(gScreenInfo.getMousePos());

      if(mCreatingPoly || mCreatingPolyline)          // Save any polygon/polyline we might be creating
      {
         saveUndoState(mItems);             // Save state prior to addition of new polygon

         if(mNewItem.vertCount() > 1)
         {
            mItems.push_back(mNewItem);
            mItems.last().onGeomChanged();  // Walls need to be added to mItems BEFORE onGeomChanged() is run!
            mItems.sort(geometricSort);
         }
         mNewItem.invalidate();
         mCreatingPoly = false;
         mCreatingPolyline = false;
      }

      mMouseDownPos = convertCanvasToLevelCoord(mMousePos);
      mMostRecentState = mItems;    // For later saving to the undo stack if need be

      if(mouseOnDock())    // On the dock?  Did we hit something to start dragging off the dock?
      {
         clearSelection();
         mDraggingDockItem = findHitItemOnDock(mMousePos);
      }
      else                 // Mouse is not on dock
      {
         mDraggingDockItem = NONE;

         // rules for mouse down:
         // if the click has no shift- modifier, then
         //   if the click was on something that was selected
         //     do nothing
         //   else
         //     clear the selection
         //     add what was clicked to the selection
         //  else
         //    toggle the selection of what was clicked

         S32 vertexHit, vertexHitPoly;

         findHitVertex(mMousePos, vertexHitPoly, vertexHit);

         if(!getKeyState(KEY_SHIFT))      // Shift key is not down
         {
            // If we hit a vertex of an already selected item --> now we can move that vertex w/o losing our selection
            if(vertexHit != NONE && mItems[vertexHitPoly].selected)    
            {
               clearSelection();
               mItems[vertexHitPoly].selectVert(vertexHit);
            }
            if(mItemHit != NONE && mItems[mItemHit].selected)   // Hit an already selected item
            {
               // Do nothing
            }
            else if(mItemHit != NONE && mItems[mItemHit].geomType() == geomPoint)  // Hit a point item
            {
               clearSelection();
               mItems[mItemHit].selected = true;
            }
            else if(vertexHit != NONE && (mItemHit == NONE || !mItems[mItemHit].selected))      // Hit a vertex of an unselected item
            {        // (braces required)
               if(!mItems[vertexHitPoly].vertSelected(vertexHit))
               {
                  clearSelection();
                  mItems[vertexHitPoly].selectVert(vertexHit);
               }
            }
            else if(mItemHit != NONE)                                                        // Hit a non-point item, but not a vertex
            {
               clearSelection();
               mItems[mItemHit].selected = true;
            }
            else     // Clicked off in space.  Starting to draw a bounding rectangle?
            {
               mDragSelecting = true;
               clearSelection();
            }
         }
         else     // Shift key is down
         {
            if(vertexHit != NONE)
            {
               if(mItems[vertexHitPoly].vertSelected(vertexHit))
                  mItems[vertexHitPoly].unselectVert(vertexHit);
               else
                  mItems[vertexHitPoly].aselectVert(vertexHit);
            }
            else if(mItemHit != NONE)
               mItems[mItemHit].selected = !mItems[mItemHit].selected;    // Toggle selection of hit item
            else
               mDragSelecting = true;
         }
     }     // end mouse not on dock block, doc

     syncUnmovedItems();
     findSnapVertex();     // Update snap vertex in the event an item was selected

   }     // end if keyCode == MOUSE_LEFT

   // Neither mouse button, let's try some keys
   else if(keyCode == KEY_D)              // D - Pan right
      mRight = true;
   else if(keyCode == KEY_RIGHT)          // Right - Pan right
      mRight = true;
   else if(keyCode == KEY_H)              // H - Flip horizontal
      flipSelectionHorizontal();
   else if(keyCode == KEY_V && getKeyState(KEY_CTRL))    // Ctrl-V - Paste selection
      pasteSelection();
   else if(keyCode == KEY_V)              // V - Flip vertical
      flipSelectionVertical();
   else if(keyCode == KEY_SLASH)
      OGLCONSOLE_ShowConsole();

   else if(keyCode == KEY_L && getKeyState(KEY_CTRL) && getKeyState(KEY_SHIFT))
   {
      loadLevel();                        // Ctrl-Shift-L - Reload level
      gEditorUserInterface.setSaveMessage("Reloaded " + getLevelFileName(), true);
   }
   else if(keyCode == KEY_Z)
   {
      if(getKeyState(KEY_CTRL) && getKeyState(KEY_SHIFT))   // Ctrl-Shift-Z - Redo
         redo();
      else if(getKeyState(KEY_CTRL))    // Ctrl-Z - Undo
         undo(true);
      else                              // Z - Reset veiw
        centerView();
   }
   else if(keyCode == KEY_R)
      if(getKeyState(KEY_CTRL) && getKeyState(KEY_SHIFT))      // Ctrl-Shift-R - Rotate by arbitrary amount
      {
         if(!anyItemsSelected())
            return;

         mEntryBox = getNewEntryBox("", "Rotation angle:", 10, LineEditor::numericFilter);
         entryMode = EntryAngle;
      }
      else if(getKeyState(KEY_CTRL))        // Ctrl-R - Run levelgen script, or clear last results
      {
         if(mLevelGenItems.size() == 0)
            runLevelGenScript();
         else
            clearLevelGenItems();
      }
      else
         rotateSelection(getKeyState(KEY_SHIFT) ? 15 : -15); // Shift-R - Rotate CW, R - Rotate CCW
   else if((keyCode == KEY_I) && getKeyState(KEY_CTRL))  // Ctrl-I - Insert items generated with script into editor
   {
      copyScriptItemsToEditor();
   }

   else if((keyCode == KEY_UP) && !getKeyState(KEY_CTRL) || keyCode == KEY_W)  // W or Up - Pan up
      mUp = true;
   else if(keyCode == KEY_UP && getKeyState(KEY_CTRL))      // Ctrl-Up - Zoom in
      mIn = true;
   else if(keyCode == KEY_DOWN)
   { /* braces required */
      if(getKeyState(KEY_CTRL))           // Ctrl-Down - Zoom out
         mOut = true;
      else                                // Down - Pan down
         mDown = true;
   }
   else if(keyCode == KEY_S)
   {
      if(getKeyState(KEY_CTRL))           // Ctrl-S - Save
         gEditorUserInterface.saveLevel(true, true);
      else                                // S - Pan down
         mDown = true;
   }
   else if(keyCode == KEY_A && getKeyState(KEY_CTRL))            // Ctrl-A - toggle see all objects
   {
      mShowMode = (ShowMode) ((U32)mShowMode + 1);
      //if(mShowMode == ShowAllButNavZones && !mHasBotNavZones)    // Skip hiding NavZones if we don't have any
      //   mShowMode = (ShowMode) ((U32)mShowMode + 1);

      if(mShowMode == ShowModesCount)
         mShowMode = (ShowMode) 0;     // First mode

      if(mShowMode == ShowWallsOnly && !mDraggingObjects)
         glutSetCursor(GLUT_CURSOR_RIGHT_ARROW);

      populateDock();   // Different modes have different items

      onMouseMoved();   // Reset mouse to spray if appropriate
   }
   else if(keyCode == KEY_LEFT || keyCode == KEY_A)   // Left or A - Pan left
      mLeft = true;
   else if(keyCode == KEY_EQUALS)         // Plus (+) - Increase barrier width
   {
      if(getKeyState(KEY_SHIFT))          // SHIFT --> by 1
         incBarrierWidth(1);
      else                                // unshifted --> by 5
         incBarrierWidth(5);
   }
   else if(keyCode == KEY_MINUS)          // Minus (-)  - Decrease barrier width
   {
      if(getKeyState(KEY_SHIFT))          // SHIFT --> by 1
         decBarrierWidth(1);
      else                                // unshifted --> by 5
         decBarrierWidth(5);
   }

   else if(keyCode == KEY_E)              // E - Zoom In
         mIn = true;
   else if(keyCode == KEY_BACKSLASH)      // \ - Split barrier on selected vertex
      splitBarrier();
   else if(keyCode == KEY_J)
      joinBarrier();
   else if(keyCode == KEY_X && getKeyState(KEY_CTRL) && getKeyState(KEY_SHIFT)) // Ctrl-Shift-X - Resize selection
   {
      if(!anyItemsSelected())
         return;

      mEntryBox = getNewEntryBox("", "Resize factor:", 10, LineEditor::numericFilter);
      entryMode = EntryScale;
   }
   else if(keyCode == KEY_X && getKeyState(KEY_CTRL))     // Ctrl-X - Cut selection
   {
      copySelection();
      deleteSelection(true);
   }
   else if(keyCode == KEY_C && getKeyState(KEY_CTRL))    // Ctrl-C - Copy selection to clipboard
      copySelection();
   else if(keyCode == KEY_C )             // C - Zoom out
      mOut = true;
   else if(keyCode == KEY_F3)             // F3 - Level Parameter Editor
   {
      UserInterface::playBoop();
      gGameParamUserInterface.activate();
   }
   else if(keyCode == KEY_F2)             // F2 - Team Editor Menu
   {
      gTeamDefUserInterface.activate();
      UserInterface::playBoop();
   }
   else if(keyCode == KEY_T)              // T - Teleporter
      insertNewItem(ItemTeleporter);
   else if(keyCode == KEY_P)              // P - Speed Zone
      insertNewItem(ItemSpeedZone);
   else if(keyCode == KEY_G)              // G - Spawn
      insertNewItem(ItemSpawn);
   else if(keyCode == KEY_B && getKeyState(KEY_CTRL)) // Ctrl-B - Spy Bug
      insertNewItem(ItemSpyBug);
   else if(keyCode == KEY_B)              // B - Repair
      insertNewItem(ItemRepair);
   else if(keyCode == KEY_Y)              // Y - Turret
      insertNewItem(ItemTurret);
   else if(keyCode == KEY_M)              // M - Mine
      insertNewItem(ItemMine);
   else if(keyCode == KEY_F)              // F - Force Field
      insertNewItem(ItemForceField);
   else if(keyCode == KEY_BACKSPACE || keyCode == KEY_DELETE)
         deleteSelection(false);
   else if(keyCode == keyHELP)            // Turn on help screen
   {
      gEditorInstructionsUserInterface.activate();
      UserInterface::playBoop();
   }
   else if(keyCode == keyOUTGAMECHAT)     // Turn on Global Chat overlay
      gChatInterface.activate();
//   else if(keyCode == keyDIAG)            // Turn on diagnostic overlay -- why not here??
//      gDiagnosticInterface.activate();
   else if(keyCode == KEY_ESCAPE)           // Activate the menu
   {
      UserInterface::playBoop();
      gEditorMenuUserInterface.activate();
   }
   else if(keyCode == KEY_SPACE)
      mSnapDisabled = true;
   else if(keyCode == KEY_TAB)
      mShowingReferenceShip = true;
}


// Handle keyboard activity when we're editing an item's attributes
void EditorUserInterface::textEntryKeyHandler(KeyCode keyCode, char ascii)
{
   if(keyCode == KEY_ENTER)
   {
      if(entryMode == EntryID)
      {
         for(S32 i = 0; i < mItems.size(); i++)
            if(mItems[i].selected)          // Should only be one
            {
               U32 id = atoi(mEntryBox.c_str());
               if(mItems[i].id != id)       // Did the id actually change?
               {
                  mItems[i].id = id;
                  mAllUndoneUndoLevel = -1; // If so, it can't be undone
               }
               break;
            }
      }
      else if(entryMode == EntryAngle)
      {
         F32 angle = (F32) atof(mEntryBox.c_str());
         rotateSelection(-angle);      // Positive angle should rotate CW, negative makes that happen
      }
      else if(entryMode == EntryScale)
      {
         F32 scale = (F32) atof(mEntryBox.c_str());
         scaleSelection(scale);
      }

      entryMode = EntryNone;
   }
   else if(keyCode == KEY_ESCAPE)
   {
      entryMode = EntryNone;
   }
   else if(keyCode == KEY_BACKSPACE || keyCode == KEY_DELETE)
      mEntryBox.handleBackspace(keyCode);

   else
      mEntryBox.addChar(ascii);

   // else ignore keystroke
}


void EditorUserInterface::specialAttributeKeyHandler(KeyCode keyCode, char ascii)
{
   if( keyCode == KEY_J && getKeyState(KEY_CTRL) )
   { /* Do nothing */ }
   else if(keyCode == KEY_ESCAPE || keyCode == MOUSE_LEFT || keyCode == MOUSE_RIGHT)      // End editing, revert
   {
      doneEditingSpecialItem(false); 
      return;
   }
   else if(mSpecialAttribute == Text)
   {
      if(keyCode == KEY_BACKSPACE || keyCode == KEY_DELETE)
         mItems[mEditingSpecialAttrItem].lineEditor.handleBackspace(keyCode);

      else if(ascii)       // User typed a character -- add it to the string
         mItems[mEditingSpecialAttrItem].lineEditor.addChar(ascii);
   }
   else if(mSpecialAttribute == RepopDelay)
   {
      if(keyCode == KEY_UP)         // Up - increase delay
      {  /* braces required */
         if(mItems[mEditingSpecialAttrItem].repopDelay < 120)
            mItems[mEditingSpecialAttrItem].repopDelay++;
      }
      else if(keyCode == KEY_DOWN)  // Down - decrease delay
      {  /* braces required */
         if(mItems[mEditingSpecialAttrItem].repopDelay > 0)
            mItems[mEditingSpecialAttrItem].repopDelay--;
      }
   }
   else if(mSpecialAttribute == GoFastSpeed)
   {
      if(keyCode == KEY_UP)         // Up - increase speed
      {  /* braces required */
         if(mItems[mEditingSpecialAttrItem].speed < SpeedZone::maxSpeed)
            mItems[mEditingSpecialAttrItem].speed += 10;
      }
      else if(keyCode == KEY_DOWN)  // Down - decrease speed
      {  /* braces required */
         if(mItems[mEditingSpecialAttrItem].speed > SpeedZone::minSpeed)
            mItems[mEditingSpecialAttrItem].speed -= 10;
      }
   }
   else if(mSpecialAttribute == GoFastSnap)
   {
      if(keyCode == KEY_UP || keyCode == KEY_DOWN)   // Up/Down - toggle snapping
      {  /* braces required */
         mItems[mEditingSpecialAttrItem].boolattr = !mItems[mEditingSpecialAttrItem].boolattr;
      }
   }

   mItems[mEditingSpecialAttrItem].onAttrsChanging();
}


void EditorUserInterface::itemPropertiesEnterKeyHandler()
{
   for(S32 i = 0; i < mItems.size(); i++)
   {
      if(mItems[i].selected)
      {
         // Force item i to be the one and only selected item type.  This will clear up some problems that
         // might otherwise occur.  If you have multiple items selected, all will end up with the same values
         mItems[i].selected = true;

         for(S32 j = 0; j < mItems.size(); j++)
            if(mItems[j].selected && mItems[j].index != mItems[i].index)
               unselectItem(j);

         mEditingSpecialAttrItem = i;
         mSpecialAttribute = (SpecialAttribute) getNextAttr(i);

         if(mSpecialAttribute != NoAttribute)
         {
            mEditingSpecialAttrItem = i;
            saveUndoState(mItems);
         }
         else
            doneEditingSpecialItem(true);

         break;
      }
   }
}


void EditorUserInterface::onKeyUp(KeyCode keyCode)
{
   switch(keyCode)
   {
      case KEY_UP:
         mIn = false;
         // fall-through OK
      case KEY_W:
         mUp = false;
         break;
      case KEY_DOWN:
         mOut = false;
         // fall-through OK
      case KEY_S:
         mDown = false;
         break;
      case KEY_LEFT:
      case KEY_A:
         mLeft = false;
         break;
      case KEY_RIGHT:
      case KEY_D:
         mRight = false;
         break;
      case KEY_E:
         mIn = false;
         break;
      case KEY_C:
         mOut = false;
         break;
      case KEY_SPACE:
         mSnapDisabled = false;
         break;
      case KEY_TAB:
         mShowingReferenceShip = false;
         break;
      case MOUSE_LEFT:
      case MOUSE_RIGHT:    // test
         mMousePos.set(gScreenInfo.getMousePos());

         if(mDragSelecting)      // We were drawing a rubberband selection box
         {
            Rect r(convertCanvasToLevelCoord(mMousePos), mMouseDownPos);
            S32 j;

            for(S32 i = 0; i < mItems.size(); i++)
            {
               // Skip hidden items
               if(mShowMode == ShowWallsOnly)
               {
                  if(mItems[i].index != ItemBarrierMaker)
                     continue;
               }
               else if(mShowMode == ShowAllButNavZones)
               {
                  if(mItems[i].index == ItemNavMeshZone)
                     continue;
               }
               else if(mShowMode == NavZoneMode)
               {
                  if(mItems[i].index != ItemNavMeshZone)
                     continue;
               }

               // Make sure that all vertices of an item are inside the selection box; basically means that the entire 
               // item needs to be surrounded to be included in the selection
               for(j = 0; j < mItems[i].vertCount(); j++)
                  if(!r.contains(mItems[i].vert(j)))
                     break;
               if(j == mItems[i].vertCount())
                  mItems[i].selected = true;
            }
            mDragSelecting = false;
         }
         else
         {
            if(mDraggingObjects)    // We were dragging and dropping.  Could have been a move or a delete (by dragging to dock).
               finishedDragging();
         }
         break;
   }     // case
}


// Called when user has been dragging an object and then releases it
void EditorUserInterface::finishedDragging()
{
   mDraggingObjects = false;

   if(mouseOnDock())             // This was really a delete (item dragged to dock)
   {
      if(mDraggingDockItem == NONE)
      {
         for(S32 i = 0; i < mItems.size(); i++)
            if(mItems[i].selected)
            {
               deleteItem(i);
               break;
            }
         itemToLightUp = NONE;
      }
      else        // Dragged item off the dock, then back on  ==> nothing really changed
         mItems = mMostRecentState; // Essential undoes the dragging, so if we undo delete, 
                                    // our object will be back where it was before the delete
   }

   // Mouse not on dock... we were either dragging from the dock or moving something, 
   // need to save an undo state if anything changed
   else      
   {
      if(mDraggingDockItem == NONE)    // Not dragging from dock - user is moving object around screen
      {
         // Size can change if we somehow insert/paste/whatever while dragging.  Shouldn't happen, but used to...
         TNLAssert(mItems.size() == mUnmovedItems.size(), "Selection size changed while dragging!");   
         if(mItems.size() != mUnmovedItems.size())
            return;     // It's this or crash...

         bool anyItemsChanged = false;

         // Check if anything changed... (i.e. did we move?)
         for(S32 i = 0; i < mItems.size(); i++)
         {
            bool itemChanged = false;

            for(S32 j = 0; j < mItems[i].vertCount(); j++)
               if(mItems[i].vert(j) != mUnmovedItems[i].vert(j))
               {
                  itemChanged = true;
                  anyItemsChanged = true;
                  break;
               }

            if(itemChanged)
               mItems[i].onGeomChanged();
         }

         if(anyItemsChanged)
         {
            saveUndoState(mMostRecentState);    // Something changed... save an undo state!
            mNeedToSave = true;
            autoSave();
            return;
         }
      }
   }
}


bool EditorUserInterface::mouseOnDock()
{
   return (mMousePos.x >= gScreenInfo.getGameCanvasWidth() - DOCK_WIDTH - horizMargin &&
           mMousePos.x <= gScreenInfo.getGameCanvasWidth() - horizMargin &&
           mMousePos.y >= gScreenInfo.getGameCanvasHeight() - vertMargin - getDockHeight(mShowMode) &&
           mMousePos.y <= gScreenInfo.getGameCanvasHeight() - vertMargin);
}


bool EditorUserInterface::anyItemsSelected()
{
   for(S32 i = 0; i < mItems.size(); i++)
      if(mItems[i].selected /*|| mItems[i].litUp*/ )
         return true;

   return false;
}


bool EditorUserInterface::anythingSelected()
{
   for(S32 i = 0; i < mItems.size(); i++)
   {
      if(mItems[i].selected || mItems[i].anyVertsSelected() )
         return true;
   }

   return false;
}


void EditorUserInterface::idle(U32 timeDelta)
{
   F32 pixelsToScroll = timeDelta * (getKeyState(KEY_SHIFT) ? 1.0f : 0.5f);    // Double speed when shift held down

   if(mLeft && !mRight)
      mCurrentOffset.x += pixelsToScroll;
   else if(mRight && !mLeft)
      mCurrentOffset.x -= pixelsToScroll;
   if(mUp && !mDown)
      mCurrentOffset.y += pixelsToScroll;
   else if(mDown && !mUp)
      mCurrentOffset.y -= pixelsToScroll;

   Point mouseLevelPoint = convertCanvasToLevelCoord(mMousePos);
   if(mIn && !mOut)
      mCurrentScale *= 1 + timeDelta * 0.002;
   if(mOut && !mIn)
      mCurrentScale *= 1 - timeDelta * 0.002;
   if(mCurrentScale > MIN_SCALE)
     mCurrentScale = MIN_SCALE;
   else if(mCurrentScale < MAX_SCALE)
      mCurrentScale = MAX_SCALE;

   Point newMousePoint = convertLevelToCanvasCoord(mouseLevelPoint);
   mCurrentOffset += mMousePos - newMousePoint;

   mSaveMsgTimer.update(timeDelta);
   mWarnMsgTimer.update(timeDelta);

   LineEditor::updateCursorBlink(timeDelta);
}

// Unused??
static void escapeString(const char *src, char dest[1024])
{
   S32 i;
   for(i = 0; src[i]; i++)
      if(src[i] == '\"' || src[i] == ' ' || src[i] == '\n' || src[i] == '\t')
         break;

   if(!src[i])
   {
      strcpy(dest, src);
      return;
   }
   char *dptr = dest;
   *dptr++ = '\"';
   char c;
   while((c = *src++) != 0)
   {
      switch(c)
      {
         case '\"':
            *dptr++ = '\\';
            *dptr++ = '\"';
            break;
         case '\n':
            *dptr++ = '\\';
            *dptr++ = 'n';
            break;
         case '\t':
            *dptr++ = '\\';
            *dptr++ = 't';
            break;
         default:
            *dptr++ = c;
      }
   }
   *dptr++ = '\"';
   *dptr++ = 0;
}

void EditorUserInterface::setSaveMessage(string msg, bool savedOK)
{
   mSaveMsg = msg;
   mSaveMsgTimer = saveMsgDisplayTime;
   mSaveMsgColor = (savedOK ? Color(0, 1, 0) : red);
}

void EditorUserInterface::setWarnMessage(string msg1, string msg2)
{
   mWarnMsg1 = msg1;
   mWarnMsg2 = msg2;
   mWarnMsgTimer = warnMsgDisplayTime;
   mWarnMsgColor = gErrorMessageTextColor;
}


// Safe fprintf ==> throws exception if writing fails
static void s_fprintf(FILE *stream, const char *format, ...)
{
    va_list args;
    va_start(args, format);

    char buffer[2048];
    vsnprintf(buffer, sizeof(buffer), format, args);

    va_end(args);

    if(fprintf(stream, "%s", buffer) < 0)     // Uh-oh...
    {
       throw(SaveException("Error writing to file"));
    }
}


bool EditorUserInterface::saveLevel(bool showFailMessages, bool showSuccessMessages, bool autosave)
{
   string saveName = autosave ? "auto.save" : mEditFileName;

   try
   {
      // Check if we have a valid (i.e. non-null) filename
      if(saveName == "")
      {
         gErrorMsgUserInterface.reset();
         gErrorMsgUserInterface.setTitle("INVALID FILE NAME");
         gErrorMsgUserInterface.setMessage(1, "The level file name is invalid or empty.  The level cannot be saved.");
         gErrorMsgUserInterface.setMessage(2, "To correct the problem, please change the file name using the");
         gErrorMsgUserInterface.setMessage(3, "Game Parameters menu, which you can access by pressing [F3].");

         gErrorMsgUserInterface.activate();

         return false;
      }

      char fileNameBuffer[256];
      dSprintf(fileNameBuffer, sizeof(fileNameBuffer), "%s/%s", gConfigDirs.levelDir.c_str(), saveName.c_str());
      FILE *f = fopen(fileNameBuffer, "w");
      if(!f)
         throw(SaveException("Could not open file for writing"));

      // Write out our game parameters --> first one will be the gameType, along with all required parameters
      for(S32 i = 0; i < gGameParamUserInterface.gameParams.size(); i++)
         if(gGameParamUserInterface.gameParams[i].substr(0, 5) != "Team ")  // Don't write out teams here... do it below!
            s_fprintf(f, "%s\n", gGameParamUserInterface.gameParams[i].c_str());

      for(S32 i = 0; i < mTeams.size(); i++)
         s_fprintf(f, "Team %s %g %g %g\n", mTeams[i].getName().getString(),
            mTeams[i].color.r, mTeams[i].color.g, mTeams[i].color.b);

      // Write out all level items (do two passes; walls first, non-walls next, so turrets & forcefields have something to grab onto)
      for(S32 j = 0; j < 2; j++)
         for(S32 i = 0; i < mItems.size(); i++)
         {
            WorldItem &p = mItems[i];

            // Make sure we are writing wall items on first pass, non-wall items next
            if((p.index == ItemBarrierMaker) != (j == 0))
               continue;

            s_fprintf(f, "%s", itemDef[mItems[i].index].name);

            // Write id if it's not 0
            if(mItems[i].id > 0)
               s_fprintf(f, "!%d", mItems[i].id);

            if(itemDef[mItems[i].index].hasTeam)
               s_fprintf(f, " %d", mItems[i].team);
            if(mItems[i].hasWidth())
               s_fprintf(f, " %g", mItems[i].width);
            for(S32 j = 0; j < p.vertCount(); j++)
               s_fprintf(f, " %g %g ", p.vert(j).x, p.vert(j).y);
            if(itemDef[mItems[i].index].hasText)
               s_fprintf(f, " %d %s", mItems[i].textSize, mItems[i].lineEditor.c_str());
            if(itemDef[mItems[i].index].hasRepop && mItems[i].repopDelay != -1)
               s_fprintf(f, " %d", mItems[i].repopDelay);
            if(mItems[i].index == ItemSpeedZone)
               s_fprintf(f, " %d %s", mItems[i].speed, mItems[i].boolattr ? "SnapEnabled" : "");

            s_fprintf(f, "\n");
         }
      fclose(f);
   }
   catch (SaveException &e)
   {
      if(showFailMessages)
         gEditorUserInterface.setSaveMessage("Error Saving: " + string(e.what()), false);
      return false;
   }

   if(!autosave)     // Doesn't count as a save!
   {
      mNeedToSave = false;
      mAllUndoneUndoLevel = mLastUndoIndex;     // If we undo to this point, we won't need to save
   }

   if(showSuccessMessages)
      gEditorUserInterface.setSaveMessage("Saved " + getLevelFileName(), true);

   return true;
}


extern void initHostGame(Address bindAddress, Vector<string> &levelList, bool testMode);
extern CmdLineSettings gCmdLineSettings;

void EditorUserInterface::testLevel()
{
   bool gameTypeError = false;

   if(strcmp(mGameType, GameType::validateGameType(mGameType)))
      gameTypeError = true;

   validateLevel();
   if(mLevelErrorMsgs.size() || gameTypeError)
   {
      S32 i;
      gErrorMsgUserInterface.reset();
      gErrorMsgUserInterface.setTitle("LEVEL HAS CRITICAL PROBLEMS");

      for(i = 0; i < mLevelErrorMsgs.size(); i++)
         gErrorMsgUserInterface.setMessage(i + 1, mLevelErrorMsgs[i].c_str());
      if(gameTypeError)
      {
         gErrorMsgUserInterface.setMessage(i + 1, "ERROR: GameType is invalid.");
         gErrorMsgUserInterface.setMessage(i + 2, "(Fix in Level Parameters screen [F3])");
         i+=2;
      }
      gErrorMsgUserInterface.setMessage(i + 3, "Loading this level may cause the game to crash.");
      gErrorMsgUserInterface.activate();
      return;
   }

   string tmpFileName = mEditFileName;
   mEditFileName = "editor.tmp";

   glutSetCursor(GLUT_CURSOR_NONE);    // Turn off cursor
   bool nts = mNeedToSave;             // Save these parameters because they are normally reset when a level is saved.
   S32 auul = mAllUndoneUndoLevel;     // Since we're only saving a temp copy, we really shouldn't reset them...

   if(saveLevel(true, false))
   {
      mEditFileName = tmpFileName;     // Restore the level name

      mWasTesting = true;

      Vector<string> levelList;
      levelList.push_back("editor.tmp");
      initHostGame(Address(IPProtocol, Address::Any, 28000), levelList, true);
   }

   mNeedToSave = nts;                  // Restore saved parameters
   mAllUndoneUndoLevel = auul;
}


void EditorUserInterface::buildAllWallSegmentEdgesAndPoints()
{
   wallSegmentManager.deleteAllSegments();

   for(S32 i = 0; i < mItems.size(); i++)
      if(mItems[i].index == ItemBarrierMaker)
         wallSegmentManager.buildWallSegmentEdgesAndPoints(&mItems[i]);
}

////////////////////////////////////////
////////////////////////////////////////

EditorMenuUserInterface gEditorMenuUserInterface;

// Constructor
EditorMenuUserInterface::EditorMenuUserInterface()
{
   setMenuID(EditorMenuUI);
   mMenuTitle = "EDITOR MENU";
}


void EditorMenuUserInterface::onActivate()
{
   Parent::onActivate();
   setupMenus();
}


extern IniSettings gIniSettings;
extern MenuItem *getWindowModeMenuItem();

//////////
// Editor menu callbacks
//////////

void reactivatePrevUICallback(U32 unused)
{
   gEditorUserInterface.reactivatePrevUI();
}

static void testLevelCallback(U32 unused)
{
   gEditorUserInterface.testLevel();
}


void returnToEditorCallback(U32 unused)
{
   gEditorUserInterface.saveLevel(true, true);  // Save level
   gEditorUserInterface.setSaveMessage("Saved " + gEditorUserInterface.getLevelFileName(), true);
   gEditorUserInterface.reactivatePrevUI();        // Return to editor
}

static void activateHelpCallback(U32 unused)
{
   gEditorInstructionsUserInterface.activate();
}

static void activateLevelParamsCallback(U32 unused)
{
   gGameParamUserInterface.activate();
}

static void activateTeamDefCallback(U32 unused)
{
   gTeamDefUserInterface.activate();
}

void quitEditorCallback(U32 unused)
{
   if(gEditorUserInterface.mNeedToSave)
   {
      gYesNoUserInterface.reset();
      gYesNoUserInterface.setInstr("Press [Y] to save, [N] to quit [ESC] to cancel");
      gYesNoUserInterface.setTitle("SAVE YOUR EDITS?");
      gYesNoUserInterface.setMessage(1, "You have not saved your edits to the level.");
      gYesNoUserInterface.setMessage(3, "Do you want to?");
      gYesNoUserInterface.registerYesFunction(saveLevelCallback);
      gYesNoUserInterface.registerNoFunction(backToMainMenuCallback);
      gYesNoUserInterface.activate();
   }
   else
      gEditorUserInterface.reactivateMenu(gMainMenuUserInterface);

   gEditorUserInterface.clearUndoHistory();        // Clear up a little memory
}

//////////

void EditorMenuUserInterface::setupMenus()
{
   menuItems.deleteAndClear();
   menuItems.push_back(new MenuItem(0, "RETURN TO EDITOR", reactivatePrevUICallback,    "", KEY_R));
   menuItems.push_back(getWindowModeMenuItem());
   menuItems.push_back(new MenuItem(0, "TEST LEVEL",       testLevelCallback,           "", KEY_T));
   menuItems.push_back(new MenuItem(0, "SAVE LEVEL",       returnToEditorCallback,      "", KEY_S));
   menuItems.push_back(new MenuItem(0, "INSTRUCTIONS",     activateHelpCallback,        "", KEY_I, keyHELP));
   menuItems.push_back(new MenuItem(0, "LEVEL PARAMETERS", activateLevelParamsCallback, "", KEY_L, KEY_F3));
   menuItems.push_back(new MenuItem(0, "MANAGE TEAMS",     activateTeamDefCallback,     "", KEY_M, KEY_F2));
   menuItems.push_back(new MenuItem(0, "QUIT",             quitEditorCallback,          "", KEY_Q, KEY_UNKNOWN));
}


void EditorMenuUserInterface::onEscape()
{
   glutSetCursor(GLUT_CURSOR_NONE);
   reactivatePrevUI();
}

void EditorMenuUserInterface::render()
{
   Parent::render();
}



// See if item with specified id is selected
bool EditorUserInterface::itemIsSelected(U32 id)
{
   for(S32 i = 0; i < mItems.size(); i++)
      if(mItems[i].selected && mItems[i].mId == id)
         return true;

   return false;
}


// Clear any borders associated with the specified zone
void EditorUserInterface::deleteBorderSegs(S32 zoneId)
{
 
   for(S32 i = 0; i < zoneBorders.size(); i++)
      if(zoneBorders[i].mOwner1 == zoneId || zoneBorders[i].mOwner2 == zoneId)
      {
         zoneBorders.erase_fast(i);
         i--;
      }
}


void EditorUserInterface::rebuildBorderSegs(U32 zoneId)
{
   deleteBorderSegs(zoneId);
   S32 i = NONE;

   for(S32 j = 0; j < mItems.size(); j++)
      if(mItems[j].mId == zoneId)
      {
         i = j;
         break;
      }

   if(i == NONE)  // Couldn't find any matching zones...
      return;

   for(S32 j = 0; j < mItems.size(); j++)
      checkZones(i, j);
}


void EditorUserInterface::rebuildAllBorderSegs()
{
   zoneBorders.clear();

   for(S32 i = 0; i < mItems.size(); i++)
   {
      if(mItems[i].index != ItemNavMeshZone)
         continue;

      for(S32 j = i + 1; j < mItems.size(); j++)
         checkZones(i, j);   
   }
}


void EditorUserInterface::checkZones(S32 i, S32 j)
{
   static ZoneBorder zoneBorder;

   if(i == j || mItems[j].index != ItemNavMeshZone)
      return;      // Don't check self...

   // Do zones i and j touch?  First a quick and dirty bounds check:
   if(!mItems[i].getExtent().intersectsOrBorders(mItems[j].getExtent()))
      return;

   if(zonesTouch(mItems[i].getVerts(), mItems[j].getVerts(), 1 / mGridSize, zoneBorder.borderStart, zoneBorder.borderEnd))
   {
      zoneBorder.mOwner1 = mItems[i].mId;
      zoneBorder.mOwner2 = mItems[j].mId;

      zoneBorders.push_back(zoneBorder);
   }
}


////////////////////////////////////////
////////////////////////////////////////

GridDatabase *getGridDatabase() 
{ 
   return gEditorUserInterface.getGridDatabase(); 
}


void WallSegmentManager::recomputeAllWallGeometry()
{
   gEditorUserInterface.buildAllWallSegmentEdgesAndPoints();
   clipAllWallEdges(wallSegments);
}


// Static method, used above and from instructions
void WallSegmentManager::clipAllWallEdges(Vector<WallSegment *> &wallSegments)
{
   for(S32 i = 0; i < wallSegments.size() - 1; i++)
      for(S32 j = i + 1; j < wallSegments.size(); j++)
         if(wallSegments[i]->getExtent().intersects(wallSegments[j]->getExtent()))
         {
            Barrier::clipRenderLinesToPoly(wallSegments[i]->corners, wallSegments[j]->edges);
            Barrier::clipRenderLinesToPoly(wallSegments[j]->corners, wallSegments[i]->edges);
         }
}


void WallSegmentManager::buildWallSegmentEdgesAndPoints(WorldItem *item)
{
   // Find any forcefields that terminate on this wall, and mark them for recalculation later
   Vector<WorldItem *> items;    // A list of forcefields terminating on the wall segment that we'll be deleting

   S32 count = wallSegments.size();                
   for(S32 i = 0; i < count; i++)
      if(wallSegments[i]->mOwner == item->mId)     // Segment belongs to item
         for(S32 j = 0; j < gEditorUserInterface.mItems.size(); j++)
            if(gEditorUserInterface.mItems[j].index == ItemForceField && 
               ( gEditorUserInterface.mItems[j].forceFieldEndSegment == wallSegments[i] ||
                 gEditorUserInterface.mItems[j].forceFieldMountSegment == wallSegments[i] ) )
               items.push_back(&gEditorUserInterface.mItems[j]);

   // Get rid of any existing segments that correspond to our item; we'll be building new ones
   deleteSegments(item->mId);

   Rect allSegExtent;

   // Create a series of WallSegments, each representing a sequential pair of vertices on our wall
   for(S32 i = 0; i < item->extendedEndPoints.size(); i += 2)
   {
      WallSegment *newSegment = new WallSegment(item->extendedEndPoints[i], item->extendedEndPoints[i+1], 
                                                item->width / getGridSize(), item->mId );    // Create the segment
      wallSegments.push_back(newSegment);       // And add it to our master segment list
   }

   for(S32 i = 0; i < wallSegments.size(); i++)
   {
      wallSegments[i]->addToDatabase();              // Add it to our spatial database

      Rect segExtent(wallSegments[i]->corners);      // Calculate a bounding box around the segment
      if(i == 0)
         allSegExtent.set(segExtent);
      else
         allSegExtent.unionRect(segExtent);
   }

   item->setExtent(allSegExtent);

   // Alert all forcefields terminating on any of the wall segments we deleted and potentially recreated
   for(S32 j = 0; j < items.size(); j++)  
      items[j]->onGeomChanged();
}


// Takes a wall, finds all intersecting segments, and marks them invalid
void WallSegmentManager::invalidateIntersectingSegments(WorldItem *item)
{
   static Vector<DatabaseObject *> intersectingSegments;

   intersectingSegments.clear();

   // Before we update our edges, we need to mark all intersecting segments using the invalid flag.
   // These will need new walls after we've moved our segment.
   for(S32 i = 0; i < wallSegments.size(); i++)
      if(wallSegments[i]->mOwner == item->mId)      // Segment belongs to our item; look it up in the database
         getGridDatabase()->findObjects(BarrierType, intersectingSegments, wallSegments[i]->getExtent());

   for(S32 i = 0; i < intersectingSegments.size(); i++)
   {
      WallSegment *intersectingSegment = dynamic_cast<WallSegment *>(intersectingSegments[i]);

      // Reset the edges of all invalidated segments to their factory settings
      intersectingSegment->resetEdges();   
      intersectingSegment->invalid = true;
   }

   buildWallSegmentEdgesAndPoints(item);

   // Invalidate all segments that potentially intersect the changed segment in its new location
   intersectingSegments.clear();
   for(S32 i = 0; i < wallSegments.size(); i++)
      if(wallSegments[i]->mOwner == item->mId)      // Segment belongs to our item, compare to all others
         getGridDatabase()->findObjects(BarrierType, intersectingSegments, wallSegments[i]->getExtent());

   for(S32 i = 0; i < intersectingSegments.size(); i++)
      dynamic_cast<WallSegment *>(intersectingSegments[i])->invalid = true;

}


// Look at all our wallSegments, and recompute boundary geometry for any items marked as invalid 
void WallSegmentManager::recomputeInvalidWallSegmentIntersections()
{
   for(S32 i = 0; i < wallSegments.size(); i++)
   {
      if(!wallSegments[i]->invalid)
         continue;

      for(S32 j = 0; j < wallSegments.size(); j++)
      {
         if(i == j)     // Don't process against self
            continue;

         if(wallSegments[i]->getExtent().intersects(wallSegments[j]->getExtent()))
         {
            Barrier::clipRenderLinesToPoly(wallSegments[i]->corners, wallSegments[j]->edges);
            Barrier::clipRenderLinesToPoly(wallSegments[j]->corners, wallSegments[i]->edges);
         }
      }
      
      wallSegments[i]->invalid = false;
   }
}


// Called when a wall segment has somehow changed.  All current and previously intersecting segments 
// need to be recomputed.
void WallSegmentManager::computeWallSegmentIntersections(WorldItem *item)
{
   invalidateIntersectingSegments(item);
   recomputeInvalidWallSegmentIntersections();
}


void WallSegmentManager::deleteAllSegments()
{
   for(S32 i = 0; i < wallSegments.size(); i++)
      delete wallSegments[i];

   wallSegments.clear();
}


// Delete all wall segments owned by specified owner.
void WallSegmentManager::deleteSegments(U32 owner)
{
   S32 count = wallSegments.size();

   for(S32 i = 0; i < count; i++)
      if(wallSegments[i]->mOwner == owner)
      {
         delete wallSegments[i];    // Destructor will remove segment from database
         wallSegments.erase_fast(i);
         i--;
         count--;
      }
}


void WallSegmentManager::renderWalls(bool convert, F32 alpha)
{
   glPushMatrix();
      gEditorUserInterface.setLevelToCanvasCoordConversion(convert);
   
      for(S32 i = 0; i < wallSegments.size(); i++)
      {
         bool beingDragged = gEditorUserInterface.mDraggingObjects && gEditorUserInterface.itemIsSelected(wallSegments[i]->mOwner);

         wallSegments[i]->renderFill(beingDragged);
      }

      // Render the exterior outlines -- these are stored as a sequence of lines, rather than individual points
      for(S32 i = 0; i < wallSegments.size(); i++)
         wallSegments[i]->renderOutline(alpha);

   glPopMatrix();
}


////////////////////////////////////////
////////////////////////////////////////

// Default constructor -- only used when loading a file
WorldItem::WorldItem(GameItems itemType, S32 itemId) : lineEditor(MAX_TEXTITEM_LEN)
{
   init(itemType, TEAM_NEUTRAL, 1, itemId, false);
}


// Primary constructor -- used when constructing dock items
WorldItem::WorldItem(GameItems itemType, Point pos, S32 xteam, bool isDockItem, F32 width, F32 height, U32 itemId) : lineEditor(MAX_TEXTITEM_LEN)
{
   init(itemType, xteam, width, itemId, isDockItem);

   addVert(pos);

   // Handle multiple-point items
   if(itemDef[itemType].geom == geomSimpleLine)       // Start with diagonal line
      addVert(pos + Point(width, height));

   else if(geomType() == geomPoly)                    // Start with a height x width rect
   {
      addVert(pos + Point(width, 0));
      addVert(pos + Point(width, height));
      addVert(pos + Point(0, height));

      initializeGeom();
   }
}


void WorldItem::init(GameItems itemType, S32 xteam, F32 xwidth, U32 itemid, bool isDockItem)
{
   index = itemType;
   team = xteam;
   id = itemid;
   selected = false;
   mAnyVertsSelected = false;
   litUp = false;
   width = xwidth;
   mId = getNextItemId();
   snapped = false;
   mDockItem = isDockItem;
   mScore = 1;

   if(itemDef[itemType].hasText)
   {
      textSize = 30;
      lineEditor.setString("Your text here");
   }

   repopDelay = getDefaultRepopDelay(itemType);

   if(itemType == ItemSpeedZone)
   {
      speed = SpeedZone::defaultSpeed;
      boolattr = SpeedZone::defaultSnap;
   }
   else
   {
      speed = -1;
      boolattr = false;
   }
}


void WorldItem::initializeGeom()
{
   if(geomType() == geomPoly)
   {
      Triangulate::Process(mVerts, fillPoints);   // Populates fillPoints from polygon outline
      centroid = findCentroid(mVerts);
      setExtent(Rect(mVerts));
   }
}


bool WorldItem::hasWidth()
{
   return itemDef[index].hasWidth;
}


// True if we should use the in-game rendering, false if we should use iconified editor rendering
bool WorldItem::renderFull(F32 scale)
{
   if(mDockItem)
      return false;

   if(index == ItemTurret || index == ItemForceField)
      return(snapped && scale > 70);
   else
      return true;
}


GeomType WorldItem::geomType()
{
   return itemDef[index].geom;
}


void WorldItem::increaseWidth(S32 amt)
{
   if(index != ItemBarrierMaker)
      return;

   width += amt - (S32) width % amt;    // Handles rounding

   if(width > Barrier::MAX_BARRIER_WIDTH)
      width = Barrier::MAX_BARRIER_WIDTH;

   onGeomChanged();
}


void WorldItem::decreaseWidth(S32 amt)
{
   if(index != ItemBarrierMaker)
      return;
   
   width -= ((S32) width % amt) ? (S32) width % amt : amt;      // Dirty, ugly thing

   if(width < Barrier::MIN_BARRIER_WIDTH)
      width = Barrier::MIN_BARRIER_WIDTH;

   onGeomChanged();
}


// Following two methods are for labeling simpleLineItems only
const char *WorldItem::getOriginBottomLabel()
{
   if(index == ItemTeleporter)
      return "Intake Vortex";
   else if(index == ItemSpeedZone)
      return "Location";
   else if(index == ItemTextItem)
      return "Start";
   else
      return "";
}


const char *WorldItem::getDestinationBottomLabel()
{
   if(index == ItemTeleporter)
      return "Destination";
   else if(index == ItemSpeedZone || index == ItemTextItem)
      return "Direction";
   else
      return "";
}


// Radius of item in editor
S32 WorldItem::getRadius(F32 scale)
{
   if(index == ItemBouncyBall)
      return TestItem::TEST_ITEM_RADIUS;
   else if(index == ItemResource)
      return ResourceItem::RESOURCE_ITEM_RADIUS;
   else if(index == ItemAsteroid)
      return S32((F32)Asteroid::ASTEROID_RADIUS * 0.75f);
   else if(index == ItemSoccerBall)
      return SoccerBallItem::SOCCER_BALL_RADIUS;
   else if(index == ItemTurret && renderFull(scale))
      return 25;
   else return NONE;    // Use default
}


// Account for the fact that the apparent selection center and actual object center are not quite aligned
Point WorldItem::getSelectionOffset(F32 scale)
{
   if(index == ItemTurret && renderFull(scale))
      return Point(0.0, .075);
   else if(index == ItemForceField && renderFull(scale))
      return Point(0.0, .035);     
   else
      return Point(0, 0);     // No offset for most items
}

S32 WorldItem::getDefaultRepopDelay(GameItems itemType)  
{
   if(itemType == ItemFlagSpawn)
      return FlagSpawn::defaultRespawnTime;
   else if(itemType == ItemAsteroidSpawn)
      return AsteroidSpawn::defaultRespawnTime;
   else if(itemType == ItemTurret)
      return Turret::defaultRespawnTime;
   else if(itemType == ItemForceField)
      return ForceFieldProjector::defaultRespawnTime;
   else if(itemType == ItemRepair)
      return RepairItem::defaultRespawnTime;
   else if(itemType == ItemEnergy)
      return EnergyItem::defaultRespawnTime;
   else
      return -1;
}


// First argument has already been removed before we get this.  This is just the parameter list.
bool WorldItem::processArguments(S32 argc, const char **argv)
{
   // Figure out how many arguments an item should have
   S32 minArgs = 2;
   if(itemDef[index].geom >= geomLine)
      minArgs += 2;
   if(itemDef[index].hasTeam)
      minArgs++;
   if(itemDef[index].hasText)
      minArgs += 2;        // Size and message
   if(argc < minArgs)      // Not enough args, time to bail
      return false;

   if(index == ItemNavMeshZone)
      gEditorUserInterface.setHasNavMeshZones(true);

   // Parse most game objects
   if(itemDef[index].name)       // Item is listed in itemDef, near top of this file
   {
      index = static_cast<GameItems>(index);
      S32 arg = 0;

      // Should the following be moved to the constructor?  Probably...
      team = TEAM_NEUTRAL;
      selected = false;

      if(itemDef[index].hasTeam)
      {
         team = atoi(argv[arg]);
         arg++;
      }
      if(itemDef[index].hasWidth)
      {
         width = atof(argv[arg]);

         // Enforce max wall width so things are consistent between editor and game
         if(index == ItemBarrierMaker && width > Barrier::MAX_BARRIER_WIDTH)
            width = Barrier::MAX_BARRIER_WIDTH;

         arg++;
      }

      if(index == ItemTextItem)
      {
         for(S32 j = 0; j < 2; j++)       // Read two points...
         {
            Point p;
            p.read(argv + arg);
            addVert(p);
            arg+=2;
         }

         textSize = atoi(argv[arg]);    // ...and a textsize...
         arg++;

         string str;
         for(;arg < argc; arg ++)         // (no first part of for)
         {
            str += argv[arg];             // ...and glob the rest together as a string
            if (arg < argc - 1)
               str += " ";
         }

         lineEditor.setString(str);
      }
      else if(index == ItemNexus && argc == 4)     // Old-school Zap! style Nexus definition --> note parallel code in HuntersNexusObject::processArguments
      {
         // Arg 0 will be HuntersNexusObject
         Point pos;
         pos.read(argv + 1);

         Point ext(50, 50);
         ext.set(atoi(argv[2]), atoi(argv[3]));
         ext /= gEditorUserInterface.getGridSize();

         addVert(Point(pos.x - ext.x, pos.y - ext.y));   // UL corner
         addVert(Point(pos.x + ext.x, pos.y - ext.y));   // UR corner
         addVert(Point(pos.x + ext.x, pos.y + ext.y));   // LR corner
         addVert(Point(pos.x - ext.x, pos.y + ext.y));   // LL corner
      }
      else        // Anything but a textItem or old-school NexusObject
      {
         S32 coords = argc;
         if(index == ItemSpeedZone)
            coords = 4;    // 2 pairs of coords = 2 * 2 = 4

         for(;arg < coords; arg += 2) // (no first arg)
         {
            // Put a cap on the number of vertices in a polygon
            if(itemDef[index].geom == geomPoly && mVerts.size() >= gMaxPolygonPoints)
               break;
            
            if(arg != argc - 1)
            {
               Point p;
               p.read(argv + arg);
               addVert(p);
            }
         }
      }

      // Add a default spawn time, which may well be overridden below
      repopDelay = getDefaultRepopDelay(index);

      // Repair, Energy, Turrets, Forcefields, FlagSpawns, AsteroidSpawns all have optional additional argument dealing with repair or repopulation
      if((index == ItemRepair || index == ItemEnergy || index == ItemAsteroidSpawn) && argc == 3)
         repopDelay = atoi(argv[2]);

      if( (index == ItemTurret || index == ItemForceField || index == ItemFlagSpawn) && argc == 4)
         repopDelay = atoi(argv[3]);

      // SpeedZones have 2 optional extra arguments
      if(index == ItemSpeedZone)
      {
         if(argc >= 5)
            speed = atoi(argv[4]);
         else
            speed = SpeedZone::defaultSpeed;

         if(argc >= 6)
            boolattr = true;
       }
   }

   return true;
}


void WorldItem::processEndPoints()
{
   if(index != ItemBarrierMaker)
      return;

   Barrier::constructBarrierEndPoints(mVerts, width / getGridSize(), extendedEndPoints);
}


Point WorldItem::snapEngineeredObject(const Point &pos)
{  
   Point anchor, nrml;
   DatabaseObject *mountSeg = EngineeredObject::
               findAnchorPointAndNormal(getGridDatabase(), pos, EngineeredObject::MAX_SNAP_DISTANCE / getGridSize(), false, anchor, nrml);

   if(mountSeg)
   {
      mVerts[0].set(anchor);
      normal.set(nrml);
      findForceFieldEnd();
      forceFieldMountSegment = dynamic_cast<WallSegment *>(mountSeg);
      snapped = true;
      return anchor;
   }
   else
   {
      snapped = false;
      return pos;
   }
}


// Draw barrier centerlines; wraps renderPolyline()
void WorldItem::renderPolylineCenterline(F32 alpha)
{
   // Render wall centerlines
   if(selected)
      glColor(SELECT_COLOR, alpha);
   else if(litUp && !mAnyVertsSelected)
      glColor(HIGHLIGHT_COLOR, alpha);
   else
      glColor(gEditorUserInterface.getTeamColor(team), alpha);

   glLineWidth(WALL_SPINE_WIDTH);
   renderPolyline();
   glLineWidth(gDefaultLineWidth);
}


// Draws a line connecting points in mVerts
void WorldItem::renderPolyline()
{
   glPushMatrix();
      gEditorUserInterface.setLevelToCanvasCoordConversion();

      glBegin(GL_LINE_STRIP);
         for(S32 j = 0; j < mVerts.size(); j++)
            glVertex(mVerts[j]);
      glEnd();

   glPopMatrix();
}


// Select a single vertex.  This is the default selection we use most of the time
void WorldItem::selectVert(S32 vertIndex) 
{ 
   unselectVerts();
   aselectVert(vertIndex);
}


// Select an additional vertex (remember command line ArcInfo?)
void WorldItem::aselectVert(S32 vertIndex)
{
   mVertSelected[vertIndex] = true;
   mAnyVertsSelected = true;
}


// Unselect a single vertex, considering the possibility that there may be other selected vertices as well
void WorldItem::unselectVert(S32 vertIndex) 
{ 
   mVertSelected[vertIndex] = false;

   bool anySelected = false;
   for(S32 j = 0; j < mVertSelected.size(); j++)
      if(mVertSelected[j])
      {
         anySelected = true;
         break;
      }
   mAnyVertsSelected = anySelected;
}


GridDatabase *WorldItem::getGridDatabase() 
{ 
   return gEditorUserInterface.getGridDatabase(); 
}


bool WorldItem::getCollisionPoly(Vector<Point> &polyPoints)
{  /// ????
   if(index == ItemBarrierMaker)    // Barrier items will be represented in by their segments in the database
      return false;

   return false;
}


bool WorldItem::getCollisionCircle(U32 stateIndex, Point &point, float &radius)
{
   return false;     // For now...
}


// Unselect all vertices
void WorldItem::unselectVerts() 
{ 
   for(S32 j = 0; j < mVerts.size(); j++) 
      mVertSelected[j] = false; 
   mAnyVertsSelected = false;
}


bool WorldItem::vertSelected(S32 vertIndex) 
{ 
   return mVertSelected[vertIndex]; 
}


void WorldItem::addVert(Point vert)
{
   mVerts.push_back(vert);
   mVertSelected.push_back(false);
}


void WorldItem::addVertFront(Point vert)
{
   mVerts.push_front(vert);
   mVertSelected.push_front(false);
}


void WorldItem::insertVert(Point vert, S32 vertIndex)
{
   mVerts.insert(vertIndex);
   mVerts[vertIndex] = vert;

   mVertSelected.insert(vertIndex);
   mVertSelected[vertIndex] = false;
}


void WorldItem::setVert(Point vert, S32 vertIndex)
{
   mVerts[vertIndex] = vert;
}


void WorldItem::deleteVert(S32 vertIndex)
{
   mVerts.erase(vertIndex);
   mVertSelected.erase(vertIndex);
}


void WorldItem::flipHorizontal(const Point &boundingBoxMin, const Point &boundingBoxMax)
{
   for(S32 j = 0; j < mVerts.size(); j++)
      mVerts[j].x = boundingBoxMin.x + (boundingBoxMax.x - mVerts[j].x);
   onGeomChanged();
}


void WorldItem::flipVertical(const Point &boundingBoxMin, const Point &boundingBoxMax)
{
   for(S32 j = 0; j < mVerts.size(); j++)
      mVerts[j].y = boundingBoxMax.y + (boundingBoxMax.y - mVerts[j].y);
   onGeomChanged();
}


void WorldItem::onGeomChanging()
{
   if(index == ItemTextItem)
      onGeomChanged();
   else if(geomType() == geomPoly)
      onGeomChanged();     // Allows poly fill to get reshaped as vertices move
}


// Item is being activel dragged
void WorldItem::onItemDragging()
{
   if(index == ItemForceField)
     onGeomChanged();

   else if(geomType() == geomPoly)
      onGeomChanged();     // Allows poly fill to get dragged around with outline
}


WallSegmentManager *getWallSegmentManager()
{
   return gEditorUserInterface.getWallSegmentManager();
}


void WorldItem::onGeomChanged()
{
  if(index == ItemTextItem)
   {
      F32 strWidth = (F32)UserInterface::getStringWidth(120, lineEditor.c_str());
      F32 lineLen = mVerts[0].distanceTo(mVerts[1]);
      F32 size = 120.0f * lineLen * gEditorUserInterface.getGridSize() / max(strWidth, 80.0f);

      // Compute text size subject to min and max defined in TextItem
      textSize = max(min(U32(size), TextItem::MAX_TEXT_SIZE), TextItem::MIN_TEXT_SIZE); 
   }
   
   else if(index == ItemBarrierMaker)
   {  
      // Fill extendedEndPoints from the vertices of our wall's centerline
      processEndPoints();
      getWallSegmentManager()->computeWallSegmentIntersections(this);

      gEditorUserInterface.recomputeAllEngineeredItems();      // Seems awfully lazy...  should only recompute items attached to altered wall

      // But if we're doing the above, we don't need to bother with the below... unless we stop being lazy
      //// Find any forcefields that might intersect our new wall segment and recalc them
      //for(S32 i = 0; i < gEditorUserInterface.mItems.size(); i++)
      //   if(gEditorUserInterface.mItems[i].index == ItemForceField &&
      //                           gEditorUserInterface.mItems[i].getExtent().intersects(getExtent()))
      //      gEditorUserInterface.mItems[i].findForceFieldEnd();
   }

   else if(index == ItemForceField)
   {
      findForceFieldEnd();    // Find the end-point of the projected forcefield
   }

   else if(geomType() == geomPoly)
      initializeGeom();

   if(index == ItemNavMeshZone)
      gEditorUserInterface.rebuildBorderSegs(mId);
}


void WorldItem::onAttrsChanging()
{
   if(index == ItemTextItem)
      onGeomChanged();
}


void WorldItem::onAttrsChanged()
{
   // Do nothing for the moment
}


void WorldItem::findForceFieldEnd()
{
   // Load the corner points of a maximum-length forcefield into geom
   Vector<Point> geom;
   DatabaseObject *collObj;

   F32 scale = 1 / gEditorUserInterface.getGridSize();
   
   Point start = ForceFieldProjector::getForceFieldStartPoint(mVerts[0], normal, scale);

   if(ForceField::findForceFieldEnd(getGridDatabase(), start, normal, scale, forceFieldEnd, &collObj))
      forceFieldEndSegment = dynamic_cast<WallSegment *>(collObj);
   else
      forceFieldEndSegment = NULL;

   ForceField::getGeom(start, forceFieldEnd, geom, scale);    
   setExtent(Rect(geom));
}


void WorldItem::rotateAboutPoint(const Point &center, F32 angle)
{
   F32 sinTheta = sin(angle * Float2Pi / 360.0f);
   F32 cosTheta = cos(angle * Float2Pi / 360.0f);

   for(S32 j = 0; j < mVerts.size(); j++)
   {
      Point v = mVerts[j] - center;
      Point n(v.x * cosTheta + v.y * sinTheta, v.y * cosTheta - v.x * sinTheta);

      mVerts[j] = n + center;
   }

   onGeomChanged();
}


void WorldItem::scale(const Point &center, F32 scale)
{
   for(S32 j = 0; j < mVerts.size(); j++)
      mVerts[j].set((mVerts[j] - center) * scale + center);

   // Scale the wall width, within limits
   if(index == ItemBarrierMaker)
      width = min(max(width * scale, static_cast<float>(LineItem::MIN_LINE_WIDTH)), static_cast<float>(LineItem::MAX_LINE_WIDTH));

   onGeomChanged();
}

////////////////////////////////////////
////////////////////////////////////////

GridDatabase *WallSegment::getGridDatabase()
{ 
   return gEditorUserInterface.getGridDatabase(); 
}


// Constructor
WallSegment::WallSegment(const Point &start, const Point &end, F32 width, S32 owner) 
{ 
   // Calculate segment corners by expanding the extended end points into a rectangle
   Barrier::expandCenterlineToOutline(start, end, width, corners);   

   // Recompute the edges based on our new corner points
   resetEdges();                                            

   Rect extent(corners);
   setExtent(extent); 

   mOwner = owner; 
   invalid = false; 

   // Set some things required by DatabaseObject
   mObjectTypeMask = BarrierType;
}


// Destructor
WallSegment::~WallSegment()
{ 
   // Make sure object is out of the database
   getGridDatabase()->removeFromDatabase(this, this->getExtent()); 

   // Find any forcefields that were using this as an end point and let them know the segment is gone.  Since 
   // segment is no longer in database, when we recalculate the forcefield, our endSegmentPointer will be reset.
   // This is a last-ditch effort to ensure that the pointers point at something real.
   for(S32 i = 0; i < gEditorUserInterface.mItems.size(); i++)
      if(gEditorUserInterface.mItems[i].index == ItemForceField && 
               (gEditorUserInterface.mItems[i].forceFieldEndSegment == this || 
                gEditorUserInterface.mItems[i].forceFieldMountSegment == this) )
         gEditorUserInterface.mItems[i].onGeomChanged();       // Will force recalculation of mount and endpoint
   }

 
// Resets edges of a wall segment to their factory settings; i.e. 4 simple walls representing a simple outline
void WallSegment::resetEdges()
{
   Barrier::resetEdges(corners, edges);
}


void WallSegment::renderOutline(F32 alpha)
{
   renderWallEdges(edges, alpha);
}


void WallSegment::renderFill(bool renderLight)
{
   // We'll use the editor color most of the time; only in preview mode in the editor do we use the game color
   bool useGameColor = UserInterface::current && UserInterface::current->getMenuID() == EditorUI && 
                       gEditorUserInterface.isShowingReferenceShip();

   glColor((useGameColor ? GAME_WALL_FILL_COLOR : EDITOR_WALL_FILL_COLOR) * (renderLight ? 0.5 : 1));
   
   glDisableBlendfromLineSmooth;
   glBegin(GL_POLYGON);
      for(S32 i = 0; i < corners.size(); i++)
         glVertex(corners[i]);
   glEnd();
   glEnableBlendfromLineSmooth;
}


////////////////////////////////////////
////////////////////////////////////////
// Stores the selection state of a particular WorldItem
// Primary constructor
SelectionItem::SelectionItem(WorldItem &item)
{
   mSelected = item.selected;

   for(S32 i = 0; i < item.vertCount(); i++)
      mVertSelected.push_back(item.vertSelected(i));
}


void SelectionItem::restore(WorldItem &item)
{
   item.selected = mSelected;
   item.unselectVerts();

   for(S32 i = 0; i < item.vertCount(); i++)
      item.aselectVert(mVertSelected[i]);
}


////////////////////////////////////////
////////////////////////////////////////
// Selection stores the selection state of group of WorldItems
// Constructor
Selection::Selection(Vector<WorldItem> &items)
{
   for(S32 i = 0; i < items.size(); i++)
      mSelection.push_back(SelectionItem(items[i]));
}

void Selection::restore(Vector<WorldItem> &items)
{
   for(S32 i = 0; i < items.size(); i++)
      mSelection[i].restore(items[i]);
}

////////////////////////////////////////
////////////////////////////////////////

};

