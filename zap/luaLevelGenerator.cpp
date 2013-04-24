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

#include "luaLevelGenerator.h"
#include "gameType.h"
#include "config.h"              // For definition of FolderManager struct
#include "game.h"
#include "stringUtils.h"
#include "ServerGame.h"
#include "LuaWrapper.h"
#include "barrier.h"             // For PolyWall def

namespace Zap
{

// Default constructor
LuaLevelGenerator::LuaLevelGenerator() { TNLAssert(false, "Don't use this constructor!"); }

// Standard constructor
LuaLevelGenerator::LuaLevelGenerator(const string &scriptName, const Vector<string> &scriptArgs, F32 gridSize, 
                                     GridDatabase *gridDatabase, Game *game)
{
   TNLAssert(fileExists(scriptName), "Files should be checked before we get here -- something has gone wrong!");

   mScriptName = scriptName;
   mScriptArgs = scriptArgs;

   mGridDatabase = gridDatabase;
   mGame = game;

   mGridSize = gridSize;

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


// Destructor
LuaLevelGenerator::~LuaLevelGenerator()
{
   logprintf(LogConsumer::LogLuaObjectLifecycle, "deleted LuaLevelGenerator (%p)\n", this);
   LUAW_DESTRUCTOR_CLEANUP;
}


const char *LuaLevelGenerator::getErrorMessagePrefix() { return "***LEVELGEN ERROR***"; }


string LuaLevelGenerator::getScriptName()
{
   return mScriptName;
}


// Used in addItem() below...
static const char *argv[LevelLoader::MAX_LEVEL_LINE_ARGS];



// TODO: Provide mechanism to modify basic level parameters like game length and teams.



// Let someone else do the work!
void LuaLevelGenerator::processLevelLoadLine(S32 argc, S32 id, const char **argv, GridDatabase *database, const string &levelFileName)
{
   mGame->processLevelLoadLine(argc, id, argv, database, levelFileName);
}


///// Initialize levelgen specific stuff
bool LuaLevelGenerator::prepareEnvironment()
{
   if(!LuaScriptRunner::prepareEnvironment())
      return false;

   if(!loadAndRunGlobalFunction(L, LUA_HELPER_FUNCTIONS_KEY, LevelgenContext) || !loadAndRunGlobalFunction(L, LEVELGEN_HELPER_FUNCTIONS_KEY, LevelgenContext))
      return false;

   setSelf(L, this, "levelgen");

   return true;
}


// This will need to run on both client (from editor) and server (in game)
void LuaLevelGenerator::killScript()
{
   mGame->deleteLevelGen(this);
}


static Point getPointFromTable(lua_State *L, int tableIndex, int key, const char *methodName)
{
   lua_rawgeti(L, tableIndex, key);    // Push Point onto stack
   if(lua_isnil(L, -1))
   {
      lua_pop(L, 1);
      return Point(0,0);
   }

   Point point = LuaBase::getCheckedVec(L, -1, methodName);
   lua_pop(L, 1);    // Clear value from stack

   return point;
}


// Deprecated 
// TODO: Needs documentation
S32 LuaLevelGenerator::addWall(lua_State *L)
{
   static const char *methodName = "LevelGeneratorEditor:addWall()";

   string line = "BarrierMaker";

   try
   {
      F32 width = getCheckedFloat(L, 1, methodName);      // Width is first arg
      line += " " + ftos(width, 1);

      // Third arg is a table of coordinate values in "editor space" (i.e. these will be multiplied by gridsize before being used)
      S32 points = (S32)lua_objlen(L, 3);    // Get the number of points in our table of points

      for(S32 i = 1; i <= points; i++)       // Remember, Lua tables start with index 1
      {
         Point p = getPointFromTable(L, 3, i, methodName);
         line = line + " " + ftos(p.x, 2) + " " + ftos(p.y, 2);
      }
   }

   catch(LuaException &e)
   {
      logError(e.what());
      LuaObject::clearStack(L);
   }

   mGame->parseLevelLine(line.c_str(), mGridDatabase, "Levelgen script: " + mScriptName);

   return 0;
}


// Simply grabs parameters from the Lua stack, and passes them off to processLevelLoadLine().  Unfortunately,
// this involves packing everything into an array of char strings, which is frightfully prone to programmer
// error and buffer overflows and such...
// TODO: Needs documentation
S32 LuaLevelGenerator::addItem(lua_State *L)
{
   static const char *methodName = "LevelGenerator:addItem()";

   // First check to see if item is a BfObject
   BfObject *obj = luaW_check<BfObject>(L, 1);

   if(obj)
   {
      // Silently ignore illegal items when being run from the editor.  For the moment, if mGame is not a server, then
      // we are running from the editor.  This could conceivably change, but for the moment it seems to hold true.
      if(mGame->isServer() || obj->canAddToEditor())
      {
         // Some objects require special handling
         if(obj->getObjectTypeNumber() == PolyWallTypeNumber)
            mGame->addPolyWall(static_cast<PolyWall *>(obj), mGridDatabase);
         else if(obj->getObjectTypeNumber() == WallItemTypeNumber)
            mGame->addWallItem(static_cast<WallItem *>(obj), mGridDatabase);
         else
         {
            obj->addToGame(mGame, mGridDatabase);

            mAddedObjects.push_back(obj);
         }
      }

      return 0;
   }

   // Otherwise, try older method of converting stack items to params to be passed to processLevelLoadLine

   S32 argc = min((S32)lua_gettop(L), (S32)LevelLoader::MAX_LEVEL_LINE_ARGS);     // Never more that MaxArgc args, please!

   if(argc == 0)
   {
      logError("Object had no parameters, ignoring...");
      return 0;
   }

   for(S32 i = 0; i < argc; i++)      // argc was already bounds checked above
      argv[i] = getCheckedString(L, i + 1, methodName);

   processLevelLoadLine(argc, 0, argv, mGridDatabase, "Levelgen script: " + mScriptName);      // For now, all ids are 0!

   return 0;
}


/**
 * @luafunc    Levelgen::addLevelLine(levelLine)
 * @brief      Adds an object to the editor by passing a line from a level file.
 * @deprecated This method is deprecated and will be removed in the future.  As an alternative, construct a BfObject directly and 
 *             add it to the game using the addItem() method.
 * @param      levelLine - string containing the line of levelcode.
 */
S32 LuaLevelGenerator::addLevelLine(lua_State *L)
{
   static const char *methodName = "LevelGenerator:addLevelLine()";

   checkArgCount(L, 1, methodName);
   const char *line = getCheckedString(L, 1, methodName);

   mGame->parseLevelLine(line, mGridDatabase, "Levelgen script: " + mScriptName);

   return 0;
}


/**
 * @luafunc    Levelgen::setGameTime(timeInMinutes)
 * @brief      Sets the time remaining in the current game to the specified value
 * @param      num timeInMinutes - Time, in minutes, that the game should continue.  Can be fractional.
 */
S32 LuaLevelGenerator::setGameTime(lua_State *L)
{
   static const char *methodName = "Levelgen:setGameTime()";

   checkArgCount(L, 1, methodName);
   F32 time = getCheckedFloat(L, 1, methodName);

   mGame->setGameTime(time);

   return 0;
}


/**
 * @luafunc Levelgen::pointCanSeePoint(object1, object2)
 * @brief   Returns true if the two specified objects can see one another.
 * @param   \e BfObject object1 - First object.
 * @param   \e BfObject object2 - Second object.
 * @return  \e bool - True if objects have LOS from one to the other.
 */
S32 LuaLevelGenerator::pointCanSeePoint(lua_State *L)
{
   static const char *methodName = "Levelgen:pointCanSeePoint()";

   checkArgCount(L, 2, methodName);

   // Still need mGridSize because we deal with the coordinates used in the level file, which have to be multiplied by
   // GridSize to get in-game coordinates
   Point p1 = LuaBase::getCheckedVec(L, 1, methodName) *= mGridSize;   // Only use of getCheckedVec is here -- if remove here, can delete function 
   Point p2 = LuaBase::getCheckedVec(L, 2, methodName) *= mGridSize;

   return returnBool(L, mGridDatabase->pointCanSeePoint(p1, p2));
}


/**
 * @luafunc    Levelgen::logprint(text)
 * @brief      Writes a line of text to the console and the system log.
 * @descr      Function can be called without Levelgen: prefix;  For example:
 *  \code
 *    logprint("Hello world!")
 *  \endcode
 * @param      \e string text - Message to write.
 */
S32 LuaLevelGenerator::logprint(lua_State *L)
{
   static const char *methodName = "Levelgen:logprint()";
   checkArgCount(L, 1, methodName);

   logprintf(LogConsumer::LuaLevelGenerator, "Levelgen: %s", getCheckedString(L, 1, methodName));
   return 0;
}


/**
 * @luafunc    Levelgen::findObjectById(id)
 * @brief      Returns an object with the given id, or nil if none exists.
 * @descr      Finds an object with the specified user-assigned id.  If there are multiple objects with the same id (shouldn't happen, 
 *             but could, especially if the passed id is 0), this method will return the first object it finds with the given id.  
 *             Currently, all objects that have not been explicitly assigned an id have an id of 0.
 *
 * Note that ids can be assigned in the editor using the ! or # keys.
 *
 * @param      id - int id to search for.
 * @return     \e BfObject - Found object, or nil if no objects with the specified id could be found.
 */
S32 LuaLevelGenerator::findObjectById(lua_State *L)
{
   checkArgList(L, functionArgs, "Levelgen", "findObjectById");

   return LuaScriptRunner::findObjectById(L, mGame->getGameObjDatabase()->findObjects_fast());
}


/**
  *   @luafunc Levelgen::findGlobalObjects(table, itemType, ...)
  *   @brief   Finds all items of the specified type anywhere on the level.
  *   @descr   Can specify multiple types.  The \e table argument is optional, but levelgens that call this function frequently will perform
  *            better if they provide a reusable table in which found objects can be stored.  By providing a table, you will avoid
  *            incurring the overhead of construction and destruction of a new one.
  *
  *   If a table is not provided, the function will create a table and return it on the stack.
  *
  *   @param  table - (Optional) Reusable table into which results can be written.
  *   @param  itemType - One or more itemTypes specifying what types of objects to find.
  *   @return resultsTable - Will either be a reference back to the passed \e table, or a new table if one was not provided.
  *
  *   @code items = { }     -- Reusable container for findGlobalObjects.  Because it is defined outside
  *                         -- any functions, it will have global scope.
  *
  *         function countObjects(objType, ...)                -- Pass one or more object types
  *           table.clear(items)                               -- Remove any items in table from previous use
  *           levelgen:findGlobalObjects(items, objType, ...)  -- Put all items of specified type(s) into items table
  *           print(#items)                                    -- Print the number of items found to the console
  *         end
  */
S32 LuaLevelGenerator::findGlobalObjects(lua_State *L)
{
   checkArgList(L, functionArgs, "Levelgen", "findGlobalObjects");

   return LuaScriptRunner::findObjects(L, mGame->getGameObjDatabase(), NULL, NULL);
}


/**
 * @luafunc Levelgen::getGridSize()
 * @brief   Returns current gridSize setting.
 * @descr   Note that non-default gridSizes are rare in modern level design.
 * @return  \e num - Current gridSize.
 */
S32 LuaLevelGenerator::getGridSize(lua_State *L)
{
   return returnFloat(L, mGridSize);
}


/**
 * @luafunc Levelgen::getPlayerCount()
 * @brief   Returns current number of players.
 * @return  \e int - Current number of players.
 */
S32 LuaLevelGenerator::getPlayerCount(lua_State *L)
{
   return returnInt(L, mGame ? mGame->getPlayerCount() : 1 );
}


/**
 * @luafunc Levelgen::globalMsg(message)
 * @brief   Broadcast a message to all players.
 * @param   \e string message - Message to broadcast.
 */
S32 LuaLevelGenerator::globalMsg(lua_State *L)
{
   static const char *methodName = "Levelgen:globalMsg()";
   checkArgCount(L, 1, methodName);

   const char *message = getCheckedString(L, 1, methodName);

   GameType *gt = mGame->getGameType();
   if(gt)
   {
      gt->sendChatFromController(message);

      // Fire our event handler
      EventManager::get()->fireEvent(this, EventManager::MsgReceivedEvent, message, NULL, true);
   }

   return 0;
}


// Register our connector types with Lua
void LuaLevelGenerator::registerClasses()
{
   // General classes
   LuaScriptRunner::registerClasses();    // LuaScriptRunner is a parent class
}


//// Lua methods
const char *LuaLevelGenerator::luaClassName = "LuaLevelGenerator";

const luaL_reg LuaLevelGenerator::luaMethods[] =
{
   { "addWall",          luaW_doMethod<LuaLevelGenerator, &LuaLevelGenerator::addWall>          },
   { "addItem",          luaW_doMethod<LuaLevelGenerator, &LuaLevelGenerator::addItem>          },
   { "addLevelLine",     luaW_doMethod<LuaLevelGenerator, &LuaLevelGenerator::addLevelLine>     },

   { "findObjectById",   luaW_doMethod<LuaLevelGenerator, &LuaLevelGenerator::findObjectById>   },
   { "findGlobalObjects",luaW_doMethod<LuaLevelGenerator, &LuaLevelGenerator::findGlobalObjects>},
                                                                                                
   { "getGridSize",      luaW_doMethod<LuaLevelGenerator, &LuaLevelGenerator::getGridSize>      },
   { "getPlayerCount",   luaW_doMethod<LuaLevelGenerator, &LuaLevelGenerator::getPlayerCount>   },
   { "setGameTime",      luaW_doMethod<LuaLevelGenerator, &LuaLevelGenerator::setGameTime>      },
   { "pointCanSeePoint", luaW_doMethod<LuaLevelGenerator, &LuaLevelGenerator::pointCanSeePoint> },

   { "globalMsg",        luaW_doMethod<LuaLevelGenerator, &LuaLevelGenerator::globalMsg>        },

   { "subscribe",        luaW_doMethod<LuaLevelGenerator, &LuaLevelGenerator::subscribe>        },
   { "unsubscribe",      luaW_doMethod<LuaLevelGenerator, &LuaLevelGenerator::unsubscribe>      },

   { NULL, NULL }   
};


S32 LuaLevelGenerator::subscribe(lua_State *L)   { return doSubscribe(L, LevelgenContext);   }
S32 LuaLevelGenerator::unsubscribe(lua_State *L) { return doUnsubscribe(L); }


const LuaFunctionProfile LuaLevelGenerator::functionArgs[] =
{
      { NULL, {{{ }}, 0 } }
};

REGISTER_LUA_CLASS(LuaLevelGenerator);

};

