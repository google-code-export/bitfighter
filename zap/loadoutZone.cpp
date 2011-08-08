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

#include "loadoutZone.h"
#include "game.h"

namespace Zap
{

TNL_IMPLEMENT_NETOBJECT(LoadoutZone);

const char LoadoutZone::className[] = "LoadoutZone";      // Class name as it appears to Lua scripts

// Define the methods we will expose to Lua
Lunar<LoadoutZone>::RegType LoadoutZone::methods[] =
{
   // Standard gameItem methods
   method(LoadoutZone, getClassID),
   method(LoadoutZone, getLoc),
   method(LoadoutZone, getRad),
   method(LoadoutZone, getVel),
   method(LoadoutZone, getTeamIndx),

   {0,0}    // End method list
};


// C++ constructor
LoadoutZone::LoadoutZone()
{
   mTeam = 0;
   mNetFlags.set(Ghostable);
   mObjectTypeMask = LoadoutZoneType | CommandMapVisType;
   mObjectTypeNumber = LoadoutZoneTypeNumber;
}


LoadoutZone *LoadoutZone::clone() const
{
   return new LoadoutZone(*this);
}


void LoadoutZone::render()
{
   renderLoadoutZone(getGame()->getTeamColor(getTeam()), getOutline(), getFill(), getCentroid(), getLabelAngle());
}


void LoadoutZone::renderEditor(F32 currentScale)
{
   render();
   EditorPolygon::renderEditor(currentScale);
}


void LoadoutZone::renderDock()
{
  renderLoadoutZone(getGame()->getTeamColor(getTeam()), getOutline(), getFill());
}


S32 LoadoutZone::getRenderSortValue()
{
   return -1;
}


// Create objects from parameters stored in level file
bool LoadoutZone::processArguments(S32 argc2, const char **argv2, Game *game)
{
   // Need to handle or ignore arguments that starts with letters,
   // so a possible future version can add parameters without compatibility problem.
   S32 argc = 0;
   const char *argv[65]; // 32 * 2 + 1 = 65
   for(S32 i = 0; i < argc2; i++)  // the idea here is to allow optional R3.5 for rotate at speed of 3.5
   {
      char c = argv2[i][0];
      //switch(c)
      //{
      //case 'A': Something = atof(&argv2[i][1]); break;  // using second char to handle number
      //}
      if((c < 'a' || c > 'z') && (c < 'A' || c > 'Z'))
      {
         if(argc < 65)
         {  argv[argc] = argv2[i];
            argc++;
         }
      }
   }

   if(argc < 7)
      return false;

   mTeam = atoi(argv[0]);     // Team is first arg
   readGeom(argc, argv, 1, game->getGridSize());

   setExtent();

   return true;
}


string LoadoutZone::toString(F32 gridSize) const
{
   return string(getClassName()) + " " + itos(mTeam) + " " + geomToString(gridSize);
}


void LoadoutZone::onAddedToGame(Game *theGame)
{
   Parent::onAddedToGame(theGame);

   if(!isGhost())
      setScopeAlways();
}


// More precise boundary for precise collision detection
bool LoadoutZone::getCollisionPoly(Vector<Point> &polyPoints) const
{
   polyPoints = *getOutline();
   return true;
}

// Only gets run on the server, never on client
bool LoadoutZone::collide(GameObject *hitObject)
{
   // Anyone can use neutral loadout zones (team == -1)
   if(!isGhost() && (hitObject->getTeam() == getTeam() || getTeam() == -1) && hitObject->getObjectTypeMask() & (ShipType | RobotType))
      getGame()->getGameType()->updateShipLoadout(hitObject);      

   return false;
}


U32 LoadoutZone::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   writeThisTeam(stream);
   packGeom(connection, stream);
   return 0;
}


void LoadoutZone::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   readThisTeam(stream);
   unpackGeom(connection, stream);
   setExtent();
}


};


