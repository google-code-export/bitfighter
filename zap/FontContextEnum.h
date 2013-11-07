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

#ifndef _FONT_CONTEXT_ENUM_H_
#define _FONT_CONTEXT_ENUM_H_

namespace Zap
{     
   enum FontContext {
      BigMessageContext,       // Press any key to respawn, etc.
      HelpItemContext,         // In-game help messages
      LevelInfoContext,        // Display info about the level (at beginning of game, and when F2 pressed)
      LevelInfoHeadlineContext,
      MenuContext,             // Menu font (main game menus)
      FPSContext,              // FPS display
      HelpContext,             // For Help screens
      ErrorMsgContext,         // For big red boxes and such
      KeyContext,              // For keyboard keys
      LoadoutIndicatorContext, // For the obvious
      HelperMenuContext,       // For Loadout Menus and such
      HelperMenuHeadlineContext,
      TextEffectContext,       // Yard Sale!!! text and the like
      ScoreboardContext,       // In-game scoreboard font
      ScoreboardHeadlineContext, // Headline items on scoreboard
      MotdContext,             // Scrolling MOTD on Main Menu
      InputContext,            // Input value font
      ReleaseVersionContext,   // Version number on title screen
      WebDingContext,          // Font for rendering database icon
      ChatMessageContext,      // Font for rendering in-game chat messages
      OldSkoolContext,         // Render things like in the good ol' days
      TimeLeftHeadlineContext, // Big text on indicator in lower right corner of game  
      TimeLeftIndicatorContext // Smaller text on same indicator
   };


   // Please keep internal fonts separate at the top, away from the externally defined fonts
   enum FontId {
      // Internal fonts:
      FontRoman,                 // Our classic stroke font
      FontOrbitronLightStroke,
      FontOrbitronMedStroke,

      // External fonts:
      FontDroidSansMono,
      FontOrbitronLight,
      FontOrbitronMedium,
      HUD,
      FontWebDings,
      KeyCaps,
      FontPlay,
      FontPlayBold,
      FontModernVision,

      FontCount,
      FirstExternalFont = FontDroidSansMono
   };

};


#endif
