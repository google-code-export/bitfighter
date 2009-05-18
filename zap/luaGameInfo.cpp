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

#include "luaObject.h"
#include "luaGameInfo.h"
#include "gameType.h"


namespace Zap
{

const char LuaGameInfo::className[] = "GameInfo";      // Class name as it appears to Lua scripts

// Constructor
LuaGameInfo::LuaGameInfo(lua_State *L)
{
   // Do nothing
}


// Destructor
LuaGameInfo::~LuaGameInfo()
{
   logprintf("deleted LuaGameObject (%p)\n", this);     // Never gets run...
}


// Define the methods we will expose to Lua
Lunar<LuaGameInfo>::RegType LuaGameInfo::methods[] =
{
   method(LuaGameInfo, getGameType),
   method(LuaGameInfo, getGameTypeName),
   method(LuaGameInfo, getFlagCount),
   method(LuaGameInfo, getWinningScore),
   method(LuaGameInfo, getGameTimeTotal),
   method(LuaGameInfo, getGameTimeRemaining),
   method(LuaGameInfo, getLeadingScore),
   method(LuaGameInfo, getLeadingTeam),
   method(LuaGameInfo, getTeamCount),
   method(LuaGameInfo, getLevelName),
   method(LuaGameInfo, getGridSize),
   method(LuaGameInfo, getIsTeamGame),
   method(LuaGameInfo, getEventScore),

   {0,0}    // End method list
};


extern ServerGame *gServerGame;
extern const char *gGameTypeNames[];

S32 LuaGameInfo::getGameType(lua_State *L)
{
   TNLAssert(gServerGame->getGameType(), "Need Gametype check in getGameType");
   return returnInt(L, gServerGame->getGameType()->getGameType());
}


S32 LuaGameInfo::getGameTypeName(lua_State *L)      { return returnString(L, gGameTypeNames[gServerGame->getGameType()->getGameType()]); }
S32 LuaGameInfo::getFlagCount(lua_State *L)         { return returnInt(L, gServerGame->getGameType()->getFlagCount()); }
S32 LuaGameInfo::getWinningScore(lua_State *L)      { return returnInt(L, gServerGame->getGameType()->getWinningScore()); }
S32 LuaGameInfo::getGameTimeTotal(lua_State *L)     { return returnInt(L, gServerGame->getGameType()->getTotalGameTime()); }
S32 LuaGameInfo::getGameTimeRemaining(lua_State *L) { return returnInt(L, gServerGame->getGameType()->getRemainingGameTime()); }
S32 LuaGameInfo::getLeadingScore(lua_State *L)      { return returnInt(L, gServerGame->getGameType()->getLeadingScore()); }
S32 LuaGameInfo::getLeadingTeam(lua_State *L)       { return returnInt(L, gServerGame->getGameType()->getLeadingTeam() + 1); }

S32 LuaGameInfo::getTeamCount(lua_State *L)         { return returnInt(L, gServerGame->getGameType()->mTeams.size()); }

S32 LuaGameInfo::getLevelName(lua_State *L)         { return returnString(L, gServerGame->getGameType()->mLevelName.getString()); }
S32 LuaGameInfo::getGridSize(lua_State *L)          { return returnFloat(L, gServerGame->getGridSize()); }
S32 LuaGameInfo::getIsTeamGame(lua_State *L)        { return returnBool(L, gServerGame->getGameType()->isTeamGame()); }


S32 LuaGameInfo::getEventScore(lua_State *L)
{
   static const char *methodName = "GameInfo:getEventScore()";
   checkArgCount(L, 1, methodName);
   GameType::ScoringEvent scoringEvent = (GameType::ScoringEvent) getInt(L, 1, methodName, 0, GameType::ScoringEventsCount - 1);

   return returnInt(L, gServerGame->getGameType()->getEventScore(GameType::TeamScore, scoringEvent, 0));
};


////////////////////////////////////
////////////////////////////////////

const char LuaTeamInfo::className[] = "TeamInfo";      // Class name as it appears to Lua scripts

// Lua constructor
LuaTeamInfo::LuaTeamInfo(lua_State *L)
{
   static const char *methodName = "TeamInfo constructor";
   checkArgCount(L, 1, methodName);

   // Lua thinks first team has index 1... we know better, but we need to play along...
   U32 teamIndx = (U32) getInt(L, 1, methodName, 1, gServerGame->getGameType()->mTeams.size()) - 1;

   mTeam = gServerGame->getGameType()->mTeams[teamIndx];
   mTeamIndex = teamIndx;
}


// C++ constructor
LuaTeamInfo::LuaTeamInfo(Team team)
{
   mTeam = team;

   const char *teamName = team.name.getString();

   Vector<Team> teams = gServerGame->getGameType()->mTeams;

   for(S32 i = 0; i < teams.size(); i ++)
      if(!strcmp(teams[i].name.getString(), teamName))
      {
         mTeamIndex = i;
         break;
      }
}

// Destructor
LuaTeamInfo::~LuaTeamInfo()
{
   logprintf("deleted LuaTeamInfo (%p)\n", this);     // Never gets run...
}


// Define the methods we will expose to Lua
Lunar<LuaTeamInfo>::RegType LuaTeamInfo::methods[] =
{
   method(LuaTeamInfo, getName),
   method(LuaTeamInfo, getIndex),
   method(LuaTeamInfo, getPlayerCount),
   method(LuaTeamInfo, getScore),

   {0,0}    // End method list
};

// We'll add 1 to the index to allow the first team in Lua to have index of 1, and the first team in C++ to have an index of 0
S32 LuaTeamInfo::getIndex(lua_State *L) { return returnInt(L, mTeamIndex + 1); }             // getTeamIndex() ==> return team's index (returns int)
S32 LuaTeamInfo::getName(lua_State *L)  { return returnString(L, mTeam.name.getString()); }  // getTeamName() ==> return team name (returns string)
S32 LuaTeamInfo::getScore(lua_State *L) { return returnInt(L, mTeam.score); }                // getScore() ==> return team score (returns int)

S32 LuaTeamInfo::getPlayerCount(lua_State *L)         // number getPlayerCount() ==> return player count
{
   gServerGame->getGameType()->countTeamPlayers();    // Make sure player counts are up-to-date
   return returnInt(L, mTeam.numPlayers);
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
   logprintf("deleted LuaWeaponInfo object (%p)\n", this);     // Never gets run...
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
   method(LuaWeaponInfo, getCanDamageSelf),
   method(LuaWeaponInfo, getCanDamageTeammate),

   {0,0}    // End method list
};


S32 LuaWeaponInfo::getName(lua_State *L) { return returnString(L, gWeapons[mWeaponIndex].name.getString()); }              // Name of weapon ("Phaser", "Triple", etc.) (string)
S32 LuaWeaponInfo::getID(lua_State *L) { return returnInt(L, mWeaponIndex); }                                              // ID of module (WeaponPhaser, WeaponTriple, etc.) (integer)

S32 LuaWeaponInfo::getRange(lua_State *L) { return returnInt(L, gWeapons[mWeaponIndex].projVelocity * gWeapons[mWeaponIndex].projLiveTime / 1000 ); }                // Range of projectile (units) (integer)
S32 LuaWeaponInfo::getFireDelay(lua_State *L) { return returnInt(L, gWeapons[mWeaponIndex].fireDelay); }                   // Delay between shots in ms (integer)
S32 LuaWeaponInfo::getMinEnergy(lua_State *L) { return returnInt(L, gWeapons[mWeaponIndex].minEnergy); }                   // Minimum energy needed to use (integer)
S32 LuaWeaponInfo::getEnergyDrain(lua_State *L) { return returnInt(L, gWeapons[mWeaponIndex].drainEnergy); }               // Amount of energy weapon consumes (integer)
S32 LuaWeaponInfo::getProjVel(lua_State *L) { return returnInt(L, gWeapons[mWeaponIndex].projVelocity); }                  // Speed of projectile (units/sec) (integer)
S32 LuaWeaponInfo::getProjLife(lua_State *L) { return returnInt(L, gWeapons[mWeaponIndex].projLiveTime); }                 // Time projectile will live (ms) (integer, -1 == live forever)
S32 LuaWeaponInfo::getDamage(lua_State *L) { return returnFloat(L, gWeapons[mWeaponIndex].damageAmount); }                 // Damage projectile does (0-1, where 1 = total destruction) (float)
S32 LuaWeaponInfo::getCanDamageSelf(lua_State *L) { return returnBool(L, gWeapons[mWeaponIndex].canDamageSelf); }          // Will weapon damage self? (boolean)
S32 LuaWeaponInfo::getCanDamageTeammate(lua_State *L) { return returnBool(L, gWeapons[mWeaponIndex].canDamageTeammate); }  // Will weapon damage teammates? (boolean)


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
   logprintf("deleted LuaModuleInfo object (%p)\n", this);     // Never gets run...
}


// Define the methods we will expose to Lua
Lunar<LuaModuleInfo>::RegType LuaModuleInfo::methods[] =
{
   method(LuaModuleInfo, getName),
   method(LuaModuleInfo, getID),

   {0,0}    // End method list
};


extern const char *gModuleShortName[] ;

S32 LuaModuleInfo::getName(lua_State *L) { return returnString(L, gModuleShortName[mModuleIndex]); }              // Name of module ("Shield", "Turbo", etc.) (string)
S32 LuaModuleInfo::getID(lua_State *L) { return returnInt(L, mModuleIndex); }                                     // ID of module (ModuleShield, ModuleBoost, etc.) (integer)


////////////////////////////////////
////////////////////////////////////

const char LuaLoadout::className[] = "Loadout";      // Class name as it appears to Lua scripts

// Lua Constructor
LuaLoadout::LuaLoadout(lua_State *L)
{
   // When creating a new loadout object, load it up with the default items
   for(S32 i = 0; i < ShipModuleCount + ShipWeaponCount; i++)
      mLoadout[i] = DefaultLoadout[i];
}

// C++ Constructor -- specify items
LuaLoadout::LuaLoadout(U32 loadoutItems[])
{
   // When creating a new loadout object, load it up with the
   for(S32 i = 0; i < ShipModuleCount + ShipWeaponCount; i++)
      mLoadout[i] = loadoutItems[i];
}


// Destructor
LuaLoadout::~LuaLoadout()
{
   logprintf("deleted LuaLoadout object (%p)\n", this);     // Never gets run...
}


// Define the methods we will expose to Lua
Lunar<LuaLoadout>::RegType LuaLoadout::methods[] =
{
   method(LuaLoadout, setWeapon),
   method(LuaLoadout, setModule),
   method(LuaLoadout, isValid),
   method(LuaLoadout, equals),
   method(LuaLoadout, getWeapon),
   method(LuaLoadout, getModule),

   {0,0}    // End method list
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

   LuaLoadout *loadout = Lunar<LuaLoadout>::check(L, 1);

   for(S32 i = 0; i < ShipModuleCount + ShipWeaponCount; i++)
      if(mLoadout[i] != loadout->getLoadoutItem(i))
         return returnBool(L, false);

   return returnBool(L, true);
}


// Private helper function for above
U32 LuaLoadout::getLoadoutItem(S32 indx)
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


////////////////////////////////////
////////////////////////////////////

const char LuaPoint::className[] = "Point";      // Class name as it appears to Lua scripts

// Lua Constructor
LuaPoint::LuaPoint(lua_State *L)
{
   static const char *methodName = "LuaPoint constructor";

   checkArgCount(L, 2, methodName);
   F32 x =  getFloat(L, 1, methodName);
   F32 y =  getFloat(L, 2, methodName);

   mPoint = Point(x, y);
}


// C++ Constructor -- specify items
LuaPoint::LuaPoint(Point point)
{
   mPoint = point;
}


// Destructor
LuaPoint::~LuaPoint()
{
   // logprintf("deleted LuaPoint object (%p)\n", this);    
}


// Define the methods we will expose to Lua
Lunar<LuaPoint>::RegType LuaPoint::methods[] =
{
   method(LuaPoint, x),
   method(LuaPoint, y),
   
   method(LuaPoint, setx),
   method(LuaPoint, sety),
   method(LuaPoint, setxy),

   method(LuaPoint, equals),
   method(LuaPoint, distanceTo),
   method(LuaPoint, distSquared),
   method(LuaPoint, angleTo),
   method(LuaPoint, len),
   method(LuaPoint, lenSquared),

   {0,0}    // End method list
};


S32 LuaPoint::x(lua_State *L)  { return returnFloat(L, mPoint.x); }
S32 LuaPoint::y(lua_State *L)  { return returnFloat(L, mPoint.y); }


S32 LuaPoint::setxy(lua_State *L)  
{ 
   static const char *methodName = "LuaPoint:setxy()";
   checkArgCount(L, 2, methodName);
   F32 x =  getFloat(L, 1, methodName);
   F32 y =  getFloat(L, 2, methodName);

   mPoint = Point(x, y);
   return 0;
}

S32 LuaPoint::setx(lua_State *L)  
{ 
   static const char *methodName = "LuaPoint:setx()";
   checkArgCount(L, 1, methodName);
   F32 x =  getFloat(L, 1, methodName);

   mPoint.x = x;
   return 0;
}


S32 LuaPoint::sety(lua_State *L)  
{ 
   static const char *methodName = "LuaPoint:sety()";
   checkArgCount(L, 1, methodName);
   F32 y =  getFloat(L, 1, methodName);

   mPoint.y = y;
   return 0;
}


// Are two points equal?
S32 LuaPoint::equals(lua_State *L)
{
   checkArgCount(L, 1, "LuaPoint:equals()");

   LuaPoint *point = Lunar<LuaPoint>::check(L, 1);

   double EPSILON = .000000001;
   return returnBool(L, ((mPoint.x - point->mPoint.x < EPSILON) || (mPoint.y - point->mPoint.y < EPSILON)) );
}


S32 LuaPoint::len(lua_State *L)
{
   checkArgCount(L, 0, "LuaPoint:len()");
   return returnFloat(L, mPoint.len());
}


S32 LuaPoint::lenSquared(lua_State *L)
{
   checkArgCount(L, 0, "LuaPoint:lenSquared()");
   return returnFloat(L, mPoint.lenSquared());
}

S32 LuaPoint::distanceTo(lua_State *L)
{
   checkArgCount(L, 1, "LuaPoint:distanceTo()");
   LuaPoint *point = Lunar<LuaPoint>::check(L, 1);

   return returnFloat(L, mPoint.distanceTo(point->getPoint()));
}


S32 LuaPoint::distSquared(lua_State *L)
{
   checkArgCount(L, 1, "LuaPoint:distSquared()");
   LuaPoint *point = Lunar<LuaPoint>::check(L, 1);

   return returnFloat(L, mPoint.distSquared(point->getPoint()));
}


S32 LuaPoint::angleTo(lua_State *L)
{
   checkArgCount(L, 1, "LuaPoint:angleTo()");
   LuaPoint *point = Lunar<LuaPoint>::check(L, 1);

   return returnFloat(L, mPoint.angleTo(point->getPoint()));
}

////////////////////////////////////
////////////////////////////////////

const char LuaTimer::className[] = "Timer";      // Class name as it appears to Lua scripts

// Lua Constructor
LuaTimer::LuaTimer(lua_State *L)
{
   static const char *methodName = "Timer() constructor";
   checkArgCount(L, 1, methodName);
   mTimer.setPeriod((U32) getInt(L, 1, methodName));
   mTimer.reset();
}


// Define the methods we will expose to Lua
Lunar<LuaTimer>::RegType LuaTimer::methods[] =
{
   method(LuaTimer, reset),
   method(LuaTimer, update),
   method(LuaTimer, getTime),
   method(LuaTimer, getFraction),
   method(LuaTimer, setPeriod),

   {0,0}    // End method list
};


S32 LuaTimer::reset(lua_State *L)
{
   mTimer.reset();
   return 0;
}


S32 LuaTimer::update(lua_State *L)
{
   static const char *methodName = "Timer:update()";
   checkArgCount(L, 1, methodName);
   return returnBool(L, mTimer.update((U32) getInt(L, 1, methodName)));
}

S32 LuaTimer::getTime(lua_State *L)
{
   return returnInt(L, mTimer.getCurrent());
}

S32 LuaTimer::getFraction(lua_State *L)
{
   return returnFloat(L, mTimer.getFraction());
}

S32 LuaTimer::setPeriod(lua_State *L)
{
   static const char *methodName = "Timer:setPeriod()";
   checkArgCount(L, 1, methodName);
   mTimer.setPeriod((U32) getInt(L, 1, methodName));
   mTimer.reset();
   return 0;
}


};