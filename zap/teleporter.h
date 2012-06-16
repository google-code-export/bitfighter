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

#ifndef _TELEPORTER_H_
#define _TELEPORTER_H_

#include "SimpleLine.h"    // For SimpleLine def
#include "gameConnection.h"
#include "gameObject.h"
#include "projectile.h"    // For LuaItem
#include "Point.h"
#include "Colors.h"
#include "Engineerable.h"

#include "tnlNetObject.h"

namespace Zap
{

////////////////////////////////////////
////////////////////////////////////////

class Teleporter : public SimpleLine, public Engineerable
{
   typedef SimpleLine Parent;

public:
      enum {
      InitMask     = BIT(0),
      TeleportMask = BIT(1),
      ExitPointChangedMask = BIT(2),

      TeleporterTriggerRadius = 50,
      TeleporterDelay = 1500,                // Time teleporter remains idle after it has been used
      TeleporterExpandTime = 1350,
      TeleportInExpandTime = 750,
      TeleportInRadius = 120,
   };

private:
   S32 mLastDest;    // Destination of last ship through
   bool mNeedsEndpoint;

   void computeExtent();

public:
   Teleporter(Point pos = Point(), Point dest = Point());           // Constructor
   virtual ~Teleporter();  // Destructor

   Teleporter *clone() const;
   //void copyAttrs(Teleporter *target);

   bool doSplash;
   U32 timeout;
   U32 mTime;

   U32 mTeleporterDelay;

   static const S32 TELEPORTER_RADIUS = 75;  // Overall size of the teleporter

   static bool checkDeploymentPosition(const Point &position, GridDatabase *gb);

   virtual bool processArguments(S32 argc, const char **argv, Game *game);
   string toString(F32 gridSize) const;

   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   void idle(BfObject::IdleCallPath path);
   void render();

   void onAddedToGame(Game *theGame);

   Vector<Point> mDests;   // need public for BotNavMeshZones

   TNL_DECLARE_CLASS(Teleporter);


   ///// Editor Methods
   Color getEditorRenderColor();

   void renderEditorItem();

   void onAttrsChanging();
   void onGeomChanging();

   void onConstructed();

   bool needsEndpoint();
   void setEndpoint(const Point &point);

   // Some properties about the item that will be needed in the editor
   const char *getEditorHelpString();
   const char *getPrettyNamePlural();
   const char *getOnDockName();
   const char *getOnScreenName();
   bool hasTeam();
   bool canBeHostile();
   bool canBeNeutral();


   ///// Lua Interface

   Teleporter(lua_State *L);       //  Lua constructor
   static const char className[];
   static Lunar<Teleporter>::RegType methods[];

   S32 getClassID(lua_State *L);   // Object's class
   void push(lua_State *L);        // Push item onto stack

   S32 getRad(lua_State *L);       // Radius of item (returns number)
   S32 getVel(lua_State *L);       // Speed of item (returns point)
};


};

#endif
