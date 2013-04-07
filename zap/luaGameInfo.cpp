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

#include "luaGameInfo.h"
#include "playerInfo.h"
#include "gameType.h"
#include "NexusGame.h"
#include "ServerGame.h"
#include "ClientInfo.h"
#include "robot.h"


namespace Zap
{

// Constructor
LuaGameInfo::LuaGameInfo(lua_State *L)
{
   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


// Destructor
LuaGameInfo::~LuaGameInfo()
{
   LUAW_DESTRUCTOR_CLEANUP;
}


/**
 *  @luaclass GameInfo
 *  @brief    Get information about the current game.
 *  @descr    You can get information about the current game with the %GameInfo object.
 *
 *  You only need get this object once, then you can use it as often as you like. It will
 *  always reflect the latest data.
 */
//                Fn name                  Param profiles            Profile count
#define LUA_METHODS(CLASS, METHOD) \
   METHOD(CLASS, getGameType,          ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getGameTypeName,      ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getFlagCount,         ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getWinningScore,      ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getGameTimeTotal,     ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getGameTimeRemaining, ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getLeadingScore,      ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getLeadingTeam,       ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getTeamCount,         ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getLevelName,         ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getGridSize,          ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, isTeamGame,           ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getEventScore,        ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getPlayers,           ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, isNexusOpen,          ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getNexusTimeLeft,     ARRAYDEF({{ END }}), 1 ) \
   METHOD(CLASS, getTeam,              ARRAYDEF({{ TEAM_INDX, END }}), 1 ) \

GENERATE_LUA_FUNARGS_TABLE(LuaGameInfo, LUA_METHODS);
GENERATE_LUA_METHODS_TABLE(LuaGameInfo, LUA_METHODS);

#undef LUA_METHODS


const char *LuaGameInfo::luaClassName = "GameInfo";  // Class name as it appears to Lua scripts
REGISTER_LUA_CLASS(LuaGameInfo);


extern ServerGame *gServerGame;

S32 LuaGameInfo::lua_getGameType(lua_State *L)
{
   TNLAssert(gServerGame->getGameType(), "Need Gametype check in getGameType");
   return returnInt(L, gServerGame->getGameType()->getGameTypeId());
}


S32 LuaGameInfo::lua_getGameTypeName(lua_State *L)      { return returnString(L, gServerGame->getGameType()->getGameTypeName()); }
S32 LuaGameInfo::lua_getFlagCount(lua_State *L)         { return returnInt   (L, gServerGame->getGameType()->getFlagCount()); }
S32 LuaGameInfo::lua_getWinningScore(lua_State *L)      { return returnInt   (L, gServerGame->getGameType()->getWinningScore()); }
S32 LuaGameInfo::lua_getGameTimeTotal(lua_State *L)     { return returnInt   (L, gServerGame->getGameType()->getTotalGameTime()); }
S32 LuaGameInfo::lua_getGameTimeRemaining(lua_State *L) { return returnInt   (L, gServerGame->getGameType()->getRemainingGameTime()); }
S32 LuaGameInfo::lua_getLeadingScore(lua_State *L)      { return returnInt   (L, gServerGame->getGameType()->getLeadingScore()); }
S32 LuaGameInfo::lua_getLeadingTeam(lua_State *L)       { return returnInt   (L, gServerGame->getGameType()->getLeadingTeam() + 1); }
                                                                         
S32 LuaGameInfo::lua_getTeamCount(lua_State *L)         { return returnInt   (L, gServerGame->getTeamCount()); }

S32 LuaGameInfo::lua_getLevelName(lua_State *L)         { return returnString(L, gServerGame->getGameType()->getLevelName()->getString()); }
S32 LuaGameInfo::lua_getGridSize(lua_State *L)          { return returnFloat (L, gServerGame->getGridSize()); }
S32 LuaGameInfo::lua_isTeamGame(lua_State *L)           { return returnBool  (L, gServerGame->getGameType()->isTeamGame()); }


S32 LuaGameInfo::lua_isNexusOpen(lua_State *L)
{
   GameType *gameType = gServerGame->getGameType();
   if(!gameType || gameType->getGameTypeId() != NexusGame)
      return returnNil(L);

   return returnBool(L, static_cast<NexusGameType *>(gameType)->mNexusIsOpen);
}


S32 LuaGameInfo::lua_getNexusTimeLeft(lua_State *L)
{
   GameType *gameType = gServerGame->getGameType();
   if(!gameType || gameType->getGameTypeId() != NexusGame)
      return returnNil(L);

   return returnInt(L, static_cast<NexusGameType *>(gameType)->getNexusTimeLeft());
}


S32 LuaGameInfo::lua_getEventScore(lua_State *L)
{
   static const char *methodName = "GameInfo:getEventScore()";
   checkArgCount(L, 1, methodName);
   GameType::ScoringEvent scoringEvent = (GameType::ScoringEvent) getInt(L, 1, methodName, 0, GameType::ScoringEventsCount - 1);

   return returnInt(L, gServerGame->getGameType()->getEventScore(GameType::TeamScore, scoringEvent, 0));
};


// Return a table listing all players on this team
S32 LuaGameInfo::lua_getPlayers(lua_State *L) 
{
   ServerGame *game = gServerGame;

   S32 pushed = 0;     // Count of pushed objects

   lua_newtable(L);    // Create a table, with no slots pre-allocated for our data

   for(S32 i = 0; i < game->getClientCount(); i++)
   {
      ClientInfo *clientInfo = game->getClientInfo(i);

      if(clientInfo->getPlayerInfo() == NULL || clientInfo->isRobot())     // Skip defunct players and bots
         continue;
      
      clientInfo->getPlayerInfo()->push(L);
      pushed++;      // Increment pushed before using it because Lua uses 1-based arrays
      lua_rawseti(L, 1, pushed);
   }

   for(S32 i = 0; i < game->getRobotCount(); i ++)
   {
      game->getBot(i)->getPlayerInfo()->push(L);
      pushed++;      // Increment pushed before using it because Lua uses 1-based arrays
      lua_rawseti(L, 1, pushed);
   }

   return 1;
}


S32 LuaGameInfo::lua_getTeam(lua_State *L)
{
   checkArgList(L, functionArgs, "GameInfo", "setTeam");

   S32 index = getTeamIndex(L, 1);

   return returnTeam(L, static_cast<Team*>(gServerGame->getTeam(index)));
}

////////////////////////////////////
////////////////////////////////////

const char LuaWeaponInfo::className[] = "WeaponInfo";      // Class name as it appears to Lua scripts


// Lua constructor
LuaWeaponInfo::LuaWeaponInfo(lua_State *L)
{
   static const char *methodName = "WeaponInfo constructor";

   checkArgCount(L, 1, methodName);
   mWeaponIndex = (U32) getInt(L, 1, methodName, 0, WeaponCount - 1);
}

// C++ constructor
LuaWeaponInfo::LuaWeaponInfo(WeaponType weapon)
{
   mWeaponIndex = (U32) weapon;
}

// Destructor
LuaWeaponInfo::~LuaWeaponInfo()
{
   logprintf(LogConsumer::LogLuaObjectLifecycle, "Deleted LuaWeaponInfo object (%p)\n", this);     // Never gets run...
}


// Define the methods we will expose to Lua
Lunar<LuaWeaponInfo>::RegType LuaWeaponInfo::methods[] =
{
   method(LuaWeaponInfo, getName),
   method(LuaWeaponInfo, getID),

   method(LuaWeaponInfo, getRange),
   method(LuaWeaponInfo, getFireDelay),
   method(LuaWeaponInfo, getMinEnergy),
   method(LuaWeaponInfo, getEnergyDrain),
   method(LuaWeaponInfo, getProjVel),
   method(LuaWeaponInfo, getProjLife),
   method(LuaWeaponInfo, getDamage),
   method(LuaWeaponInfo, getDamageSelf),
   method(LuaWeaponInfo, getCanDamageTeammate),

   {0,0}    // End method list
};


S32 LuaWeaponInfo::getName(lua_State *L) { return returnString(L, GameWeapon::weaponInfo[mWeaponIndex].name.getString()); }              // Name of weapon ("Phaser", "Triple", etc.) (string)
S32 LuaWeaponInfo::getID(lua_State *L) { return returnInt(L, mWeaponIndex); }                                              // ID of module (WeaponPhaser, WeaponTriple, etc.) (integer)

S32 LuaWeaponInfo::getRange(lua_State *L) { return returnInt(L, GameWeapon::weaponInfo[mWeaponIndex].projVelocity * GameWeapon::weaponInfo[mWeaponIndex].projLiveTime / 1000 ); }                // Range of projectile (units) (integer)
S32 LuaWeaponInfo::getFireDelay(lua_State *L) { return returnInt(L, GameWeapon::weaponInfo[mWeaponIndex].fireDelay); }                   // Delay between shots in ms (integer)
S32 LuaWeaponInfo::getMinEnergy(lua_State *L) { return returnInt(L, GameWeapon::weaponInfo[mWeaponIndex].minEnergy); }                   // Minimum energy needed to use (integer)
S32 LuaWeaponInfo::getEnergyDrain(lua_State *L) { return returnInt(L, GameWeapon::weaponInfo[mWeaponIndex].drainEnergy); }               // Amount of energy weapon consumes (integer)
S32 LuaWeaponInfo::getProjVel(lua_State *L) { return returnInt(L, GameWeapon::weaponInfo[mWeaponIndex].projVelocity); }                  // Speed of projectile (units/sec) (integer)
S32 LuaWeaponInfo::getProjLife(lua_State *L) { return returnInt(L, GameWeapon::weaponInfo[mWeaponIndex].projLiveTime); }                 // Time projectile will live (ms) (integer, -1 == live forever)
S32 LuaWeaponInfo::getDamage(lua_State *L) { return returnFloat(L, GameWeapon::weaponInfo[mWeaponIndex].damageAmount); }                 // Damage projectile does (0-1, where 1 = total destruction) (float)
S32 LuaWeaponInfo::getDamageSelf(lua_State *L) { return returnFloat(L, GameWeapon::weaponInfo[mWeaponIndex].damageAmount * 
                                                                       GameWeapon::weaponInfo[mWeaponIndex].damageSelfMultiplier); }     // How much damage if you shoot yourself? (float)
S32 LuaWeaponInfo::getCanDamageTeammate(lua_State *L) { return returnBool(L, GameWeapon::weaponInfo[mWeaponIndex].canDamageTeammate); }  // Will weapon damage teammates? (boolean)


////////////////////////////////////
////////////////////////////////////

const char LuaModuleInfo::className[] = "ModuleInfo";      // Class name as it appears to Lua scripts

// Constructor
LuaModuleInfo::LuaModuleInfo(lua_State *L)
{
   static const char *methodName = "ModuleInfo constructor";
   checkArgCount(L, 1, methodName);
   mModuleIndex = (U32) getInt(L, 1, methodName, 0, ModuleCount - 1);
}


// Destructor
LuaModuleInfo::~LuaModuleInfo()
{
   // Do nothing
}


// Define the methods we will expose to Lua
Lunar<LuaModuleInfo>::RegType LuaModuleInfo::methods[] =
{
   method(LuaModuleInfo, getName),
   method(LuaModuleInfo, getID),

   {0,0}    // End method list
};


// Name of module ("Shield", "Turbo", etc.) (string)
S32 LuaModuleInfo::getName(lua_State *L) { return returnString(L, gServerGame->getModuleInfo((ShipModule) mModuleIndex)->getName()); }   

// ID of module (ModuleShield, ModuleBoost, etc.) (integer)
S32 LuaModuleInfo::getID(lua_State *L) { return returnInt(L, mModuleIndex); }                                       


////////////////////////////////////
////////////////////////////////////

REGISTER_LUA_CLASS(LuaLoadout);

const char *LuaLoadout::luaClassName = "Loadout";     // Class name as it appears to Lua scripts

// Will almost certainly need to be uncommented!!!
// Lua Constructor
//LuaLoadout::LuaLoadout(lua_State *L)
//{
//   // When creating a new loadout object, load it up with the default items
//   for(S32 i = 0; i < ShipModuleCount + ShipWeaponCount; i++)
//      mLoadout[i] = DefaultLoadout[i];
//}


// C++ Constructor -- specify items
LuaLoadout::LuaLoadout(const U8 loadoutItems[])
{
   // When creating a new loadout object, load it up with the
   for(S32 i = 0; i < ShipModuleCount + ShipWeaponCount; i++)
      mLoadout[i] = loadoutItems[i];

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}


// Destructor
LuaLoadout::~LuaLoadout()
{
   LUAW_DESTRUCTOR_CLEANUP;
   logprintf(LogConsumer::LogLuaObjectLifecycle, "Deleted LuaLoadout object (%p)\n", this);     // Never gets run...
}


const luaL_reg LuaLoadout::luaMethods[] =
{
   { "setWeapon", luaW_doMethod<LuaLoadout, &LuaLoadout::setWeapon> },
   { "setModule", luaW_doMethod<LuaLoadout, &LuaLoadout::setModule> },
   { "isValid",   luaW_doMethod<LuaLoadout, &LuaLoadout::isValid>   },
   { "equals",    luaW_doMethod<LuaLoadout, &LuaLoadout::equals>    },
   { "getWeapon", luaW_doMethod<LuaLoadout, &LuaLoadout::getWeapon> },
   { "getModule", luaW_doMethod<LuaLoadout, &LuaLoadout::getModule> },

   {0,0}    // End method list
};


const LuaFunctionProfile LuaLoadout::functionArgs[] =
{
   { NULL, {{{ }}, 0 } }
};



S32 LuaLoadout::setWeapon(lua_State *L)     // setWeapon(i, mod) ==> Set weapon at index i
{
   static const char *methodName = "Loadout:setWeapon()";

   checkArgCount(L, 2, methodName);
   U32 indx = (U32) getInt(L, 1, methodName, 1, ShipWeaponCount);
   U32 weap = (U32) getInt(L, 2, methodName, 0, WeaponCount - 1);

   mLoadout[indx + ShipModuleCount - 1] = weap;
   return 0;
}


S32 LuaLoadout::setModule(lua_State *L)     // setModule(i, mod) ==> Set module at index i
{
   static const char *methodName = "Loadout:setModule()";

   checkArgCount(L, 2, methodName);
   U32 indx = (U32) getInt(L, 1, methodName, 1, ShipModuleCount);
   U32 mod  = (U32) getInt(L, 2, methodName, 0, ModuleCount - 1);

   mLoadout[indx - 1] = mod;
   return 0;
}


S32 LuaLoadout::isValid(lua_State *L)       // isValid() ==> Is loadout config valid?
{
   U32 mod[ShipModuleCount];
   bool hasSensor = false;

   for(S32 i = 0; i < ShipModuleCount; i++)
   {
      for(S32 j = 0; j < i; j++)
         if(mod[j] == mLoadout[i])     // Duplicate entry!
            return returnBool(L, false);
      mod[i] = mLoadout[i];
      if(mLoadout[i] == ModuleSensor)
         hasSensor = true;
   }

   U32 weap[ShipWeaponCount];
   bool hasSpyBug = false;

   for(S32 i = 0; i < ShipWeaponCount; i++)
   {
      for(S32 j = 0; j < i; j++)
         if(weap[j] == mLoadout[i + ShipModuleCount])     // Duplicate entry!
            return returnBool(L, false);
      weap[i] = mLoadout[i + ShipModuleCount];
      if(mLoadout[i] == WeaponSpyBug)
         hasSpyBug = true;
   }

   // Make sure we don't have any invalid combos
   if(hasSpyBug && !hasSensor)
      return returnBool(L, false);

   return returnBool(L, true);
}


S32 LuaLoadout::equals(lua_State *L)        // equals(Loadout) ==> is loadout the same as Loadout?
{
   checkArgCount(L, 1, "Loadout:equals()");

   LuaLoadout *loadout = luaW_check<LuaLoadout>(L, 1);

   for(S32 i = 0; i < ShipModuleCount + ShipWeaponCount; i++)
      if(mLoadout[i] != loadout->getLoadoutItem(i))
         return returnBool(L, false);

   return returnBool(L, true);
}


// Private helper function for above
U8 LuaLoadout::getLoadoutItem(S32 indx)
{
   return mLoadout[indx];
}


S32 LuaLoadout::getWeapon(lua_State *L)     // getWeapon(i) ==> return weapon at index i (1, 2, 3)
{
   static const char *methodName = "Loadout:getWeapon()";
   checkArgCount(L, 1, methodName);
   U32 weap = (U32) getInt(L, 1, methodName, 1, ShipWeaponCount);
   return returnInt(L, mLoadout[weap + ShipModuleCount - 1]);
}


S32 LuaLoadout::getModule(lua_State *L)     // getModule(i) ==> return module at index i (1, 2)
{
   static const char *methodName = "Loadout:getModule()";
   checkArgCount(L, 1, methodName);
   U32 mod = (U32) getInt(L, 1, methodName, 1, ShipModuleCount);
   return returnInt(L, mLoadout[mod - 1]);
}


};
