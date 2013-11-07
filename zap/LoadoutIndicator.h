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

#ifndef _LOADOUT_INDICATOR_H_
#define _LOADOUT_INDICATOR_H_

#include "AToBScroller.h"     // Parent
#include "LoadoutTracker.h"

using namespace TNL;

namespace Zap { namespace UI {

static const S32 indicatorFontSize = 15;
static const S32 indicatorHorizPadding = 5;       // Gap between text and box
static const S32 indicatorVertPadding = 3;


class LoadoutIndicator : public AToBScroller
{
   typedef AToBScroller Parent;

private:
   LoadoutTracker mCurrLoadout;
   LoadoutTracker mPrevLoadout;

public:
   LoadoutIndicator();     // Constructor
   virtual ~LoadoutIndicator();

   static const S32 LoadoutIndicatorTopPos = 10;    // Gap between top of screen and top of indicator
   static const S32 LoadoutIndicatorLeftPos = 10;
   static const S32 LoadoutIndicatorHeight = indicatorFontSize + indicatorVertPadding * 2;
   static const S32 LoadoutIndicatorBottomPos = LoadoutIndicatorTopPos + LoadoutIndicatorHeight + 1;  // 1 accounts for line widths and such

   void newLoadoutHasArrived(const LoadoutTracker &loadout);
   void setActiveWeapon(U32 weaponIndex);

   void setModulePrimary(ShipModule module, bool isActive);
   void setModuleSecondary(ShipModule module, bool isActive);

   const LoadoutTracker *getLoadout() const;

   S32 render(ClientGame *game) const;
   S32 getWidth() const;
};

} } // Nested namespace

#endif  // _LOADOUT_INDICATOR_H_
