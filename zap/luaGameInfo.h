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

#ifndef _LUAGAME_H_
#define _LUAGAME_H_


#include "luaObject.h"
#include "gameWeapons.h"
#include "gameType.h"      // For Team def
#include "shipItems.h"     // For module defs
#include "point.h"         // For LuaPoint

namespace Zap
{

class LuaGameInfo : public LuaObject
{
public:
  LuaGameInfo(lua_State *L);      // Constructor
  ~LuaGameInfo();                 // Destructor

   static const char className[];

   static Lunar<LuaGameInfo>::RegType methods[];

   S32 getGameType(lua_State *L);
   S32 getGameTypeName(lua_State *L);
   S32 getFlagCount(lua_State *L);
   S32 getWinningScore(lua_State *L);
   S32 getGameTimeTotal(lua_State *L);
   S32 getGameTimeRemaining(lua_State *L);
   S32 getLeadingScore(lua_State *L);
   S32 getLeadingTeam(lua_State *L);
   S32 getTeamCount(lua_State *L);

   S32 getLevelName(lua_State *L);
   S32 getGridSize(lua_State *L);
   S32 getIsTeamGame(lua_State *L);

   S32 getEventScore(lua_State *L);
};

///////////////////////////////

class LuaTeamInfo : public LuaObject
{

private:
   U32 mTeamIndex;
   Team mTeam;

public:
   LuaTeamInfo(lua_State *L);      // Lua constructor
   LuaTeamInfo(Team team);         // C++ constructor

   ~LuaTeamInfo();                 // Destructor

   static const char className[];

   static Lunar<LuaTeamInfo>::RegType methods[];

   S32 getName(lua_State *L);
   S32 getIndex(lua_State *L);
   S32 getPlayerCount(lua_State *L);
   S32 getScore(lua_State *L);
};

///////////////////////////////

class LuaWeaponInfo : public LuaObject
{

private:
   U32 mWeaponIndex;

public:
   // Initialize the pointer
   LuaWeaponInfo(lua_State *L);                      // Lua constructor
   LuaWeaponInfo::LuaWeaponInfo(WeaponType weapon);  // C++ constructor

   ~LuaWeaponInfo();                                 // Destructor

   static const char className[];

   static Lunar<LuaWeaponInfo>::RegType methods[];

   S32 getName(lua_State *L);
   S32 getID(lua_State *L);

   S32 getRange(lua_State *L);
   S32 getFireDelay(lua_State *L);
   S32 getMinEnergy(lua_State *L);
   S32 getEnergyDrain(lua_State *L);
   S32 getProjVel(lua_State *L);
   S32 getProjLife(lua_State *L);
   S32 getDamage(lua_State *L);
   S32 getCanDamageSelf(lua_State *L);
   S32 getCanDamageTeammate(lua_State *L);
};

///////////////////////////////

class LuaModuleInfo : public LuaObject
{
private:
   U32 mModuleIndex;

public:
   LuaModuleInfo(lua_State *L);      // Constructor
   ~LuaModuleInfo();                 // Destructor

   static const char className[];

   static Lunar<LuaModuleInfo>::RegType methods[];

   S32 getName(lua_State *L);
   S32 getID(lua_State *L);

};

///////////////////////////////

class LuaLoadout : public LuaObject
{
private:
   U32 mLoadout[ShipModuleCount + ShipWeaponCount];

public:
   LuaLoadout(lua_State *L);        // Lua constructor
   LuaLoadout(U32 loadoutItems[]);  // C++ constructor

   ~LuaLoadout();                   // Destructor

   static const char className[];

   static Lunar<LuaLoadout>::RegType methods[];

   S32 setWeapon(lua_State *L);     // setWeapon(i, mod) ==> Set weapon at index i
   S32 setModule(lua_State *L);     // setModule(i, mod) ==> Set module at index i
   S32 isValid(lua_State *L);       // isValid() ==> Is loadout config valid?
   S32 equals(lua_State *L);        // equals(Loadout) ==> is loadout the same as Loadout?
   S32 getWeapon(lua_State *L);     // getWeapon(i) ==> return weapon at index i
   S32 getModule(lua_State *L);     // getModule(i) ==> return module at index i

   U32 getLoadoutItem(S32 indx);    // Helper function, not accessible from Lua
};


///////////////////////////////

class LuaPoint : public LuaObject
{
private:
   Point mPoint;                  // Underlying point container
   
public:
   LuaPoint(lua_State *L);        // Lua constructor
   LuaPoint(Point point);         // C++ constructor

   ~LuaPoint();                   // Destructor

   static const char className[];

   static Lunar<LuaPoint>::RegType methods[];

   Point getPoint() { return mPoint; }

   S32 x(lua_State *L);
   S32 y(lua_State *L);

   S32 setxy(lua_State *L);
   S32 setx(lua_State *L);
   S32 sety(lua_State *L);

   S32 equals(lua_State *L);     // Does point equal another point?

   // Wrap some of our standard point methods
   S32 distanceTo(lua_State *L);
   S32 distSquared(lua_State *L);
   S32 angleTo(lua_State *L);
};

///////////////////////////////

class LuaTimer : public LuaObject      // Basically wraps our Bitfighter timer class
{
private:
   Timer mTimer;

public:
   LuaTimer(lua_State *L);          // Lua constructor
   ~LuaTimer() { /* Do nohting */ }
   static const char className[];

   static Lunar<LuaTimer>::RegType methods[];

   S32 reset(lua_State *L);
   S32 update(lua_State *L);
   S32 getTime(lua_State *L);
   S32 getFraction(lua_State *L);
   S32 setPeriod(lua_State *L);
};


};

#endif
