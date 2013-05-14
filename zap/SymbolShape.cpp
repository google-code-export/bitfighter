﻿//-----------------------------------------------------------------------------------
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
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

#include "SymbolShape.h"

#include "FontManager.h"
#include "InputCode.h"

#include "gameObjectRender.h"
#include "Colors.h"

#include "OpenglUtils.h"
#include "RenderUtils.h"

using namespace TNL;


namespace Zap { namespace UI {


// Destructor
SymbolShape::~SymbolShape()
{
   // Do nothing
}


S32 SymbolShape::getWidth() const
{
   return mWidth;
}


void SymbolShape::updateWidth(S32 fontSize, FontContext fontContext)
{
   // Do nothing (is overridden)
}



// Constructor
SymbolRoundedRect::SymbolRoundedRect(S32 width, S32 height, S32 radius)
{
   mWidth = width;
   mHeight = height;
   mRadius = radius;
}

// Destructor
SymbolRoundedRect::~SymbolRoundedRect()
{
   // Do nothing
}


void SymbolRoundedRect::render(const Point &center, S32 fontSize, FontContext fontContext) const
{
   drawRoundedRect(center, mWidth, mHeight, mRadius);
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
SymbolHorizEllipse::SymbolHorizEllipse(S32 width, S32 height)
{
   mWidth = width;
   mHeight = height;
}

// Destructor
SymbolHorizEllipse::~SymbolHorizEllipse()
{
   // Do nothing
}


void SymbolHorizEllipse::render(const Point &center, S32 fontSize, FontContext fontContext) const
{
   // First the fill
   drawFilledEllipse(center, mWidth, mHeight, 0);

   // Outline in white
   glColor(Colors::white);
   drawEllipse(center, mWidth, mHeight, 0);
}


////////////////////////////////////////
////////////////////////////////////////


// Constructor
SymbolRightTriangle::SymbolRightTriangle(S32 width)
{
   mWidth = width;
}

// Destructor
SymbolRightTriangle::~SymbolRightTriangle()
{
   // Do nothing
}


static void drawButtonRightTriangle(const Point &center)
{
   Point p1(center + Point(-15, -9));
   Point p2(center + Point(-15, 10));
   Point p3(center + Point(12, 0));

   F32 vertices[] = {
         p1.x, p1.y,
         p2.x, p2.y,
         p3.x, p3.y
   };
   renderVertexArray(vertices, ARRAYSIZE(vertices) / 2, GL_LINE_LOOP);
}


void SymbolRightTriangle::render(const Point &center, S32 fontSize, FontContext fontContext) const
{
   Point cen(center.x -mWidth / 4, center.y);  // Need to off-center the label slightly for this button
   drawButtonRightTriangle(cen);
}


////////////////////////////////////////
////////////////////////////////////////


// Constructor
SymbolCircle::SymbolCircle(S32 radius)
{
   mWidth = radius * 2;
   mHeight = radius * 2;
}

// Destructor
SymbolCircle::~SymbolCircle()
{
   // Do nothing
}

void SymbolCircle::render(const Point &center, S32 fontSize, FontContext fontContext) const
{
   drawCircle(center, (F32)mWidth / 2);
}


////////////////////////////////////////
////////////////////////////////////////


// Constructor
SymbolGear::SymbolGear() : Parent(0)
{
   // Do nothing
}

// Destructor
SymbolGear::~SymbolGear()
{
   // Do nothing
}


void SymbolGear::updateWidth(S32 fontSize, FontContext fontContext)
{
   mWidth = S32(1.333f * fontSize);    // mWidth is effectively a diameter; we'll use mWidth / 2 for our rendering radius
   mHeight = mWidth;
}


void SymbolGear::render(const Point &center, S32 fontSize, FontContext fontContext) const
{
   renderLoadoutZoneIcon(center + Point(0,2), mWidth / 2);    // Slight downward adjustment to position to better align with text
}


////////////////////////////////////////
////////////////////////////////////////


SymbolText::SymbolText(const string &text)
{
   mText = text;
   mWidth = -1;
}

// Destructor
SymbolText::~SymbolText()
{
   // Do nothing
}


void SymbolText::updateWidth(S32 fontSize, FontContext fontContext)
{
   mWidth = getStringWidth(fontContext, fontSize, mText.c_str());
}


void SymbolText::render(const Point &center, S32 fontSize, FontContext fontContext) const
{
   FontManager::pushFontContext(fontContext);
   drawStringc(center.x, center.y + fontSize / 2, (F32)fontSize, mText.c_str());
   FontManager::popFontContext();
}


////////////////////////////////////////
////////////////////////////////////////


SymbolKey::SymbolKey(const string &text) : Parent(text)
{
   // Do nothing
}

// Destructor
SymbolKey::~SymbolKey()
{
   // Do nothing
}


static S32 Margin = 3;              // Buffer within key around text
static S32 Gap = 3;                 // Distance between keys
static S32 FontSizeReduction = 4;   // How much smaller keycap font is than surrounding text
static S32 VertAdj = 2;             // To help with vertical centering

void SymbolKey::updateWidth(S32 fontSize, FontContext fontContext)
{
   fontSize -= FontSizeReduction;

   S32 width = getStringWidth(fontContext, fontSize, mText.c_str()) + Margin * 2;

   mHeight = fontSize + Margin * 2;
   mWidth = max(width, mHeight) + VertAdj * Gap;
}


void SymbolKey::render(const Point &center, S32 fontSize, FontContext fontContext) const
{
   static const Point vertAdj(0, VertAdj);
   Parent::render(center + vertAdj, fontSize - FontSizeReduction, fontContext);

   S32 width =  max(mWidth - 2 * Gap, mHeight);

   drawHollowRect(center + vertAdj, width, mHeight);
}


////////////////////////////////////////
////////////////////////////////////////


SymbolStringSet::SymbolStringSet(S32 fontSize, S32 gap)
{
   mFontSize = fontSize;
   mGap = gap;
}


void SymbolStringSet::clear()
{
   mSymbolStrings.clear();
}


void SymbolStringSet::add(const SymbolString &symbolString)
{
   mSymbolStrings.push_back(symbolString);
}


void SymbolStringSet::renderLL(S32 x, S32 y) const
{
   for(S32 i = 0; i < mSymbolStrings.size(); i++)
   {
      mSymbolStrings[i].renderLL(x, y);
      y += mFontSize + mGap;
   }
}


////////////////////////////////////////
////////////////////////////////////////

static S32 computeWidth(const Vector<SymbolShape *> &symbols, S32 fontSize, FontContext fontContext)
{
   S32 width = 0;

   for(S32 i = 0; i < symbols.size(); i++)
   {
      symbols[i]->updateWidth(fontSize, fontContext);
      width += symbols[i]->getWidth();
   }

   return width;
}


SymbolString::SymbolString(const Vector<SymbolShape *> &symbols, S32 fontSize, FontContext fontContext) : mSymbols(symbols)
{
   mFontSize    = fontSize;
   mFontContext = fontContext;
   mReady = true;

   mWidth = computeWidth(symbols, fontSize, fontContext);
}


SymbolString::SymbolString(S32 fontSize, FontContext fontContext)
{
   mFontSize    = fontSize;
   mFontContext = fontContext;
   mReady = false;

   mWidth = 0;
}

// Destructor
SymbolString::~SymbolString()
{
   mSymbols.clear();    // Clean up those pointers
}


void SymbolString::setSymbols(const Vector<SymbolShape *> &symbols)
{
   mSymbols = symbols;

   mWidth = computeWidth(symbols, mFontSize, mFontContext);
   mReady = true;
}


S32 SymbolString::getWidth() const
{ 
   TNLAssert(mReady, "Not ready!");

   return mWidth;
}


// x & y are coordinates of lower left corner of where we want to render
void SymbolString::renderLL(S32 x, S32 y) const
{
   TNLAssert(mReady, "Not ready!");

   renderCC(Point(x + mWidth / 2, y - mFontSize / 2));
}


// Center is the point where we want the string centered, vertically and horizontally
void SymbolString::renderCC(const Point &center) const
{
   TNLAssert(mReady, "Not ready!");

   S32 x = (S32)center.x - mWidth / 2;
   S32 y = (S32)center.y + mFontSize / 2;

   FontManager::pushFontContext(mFontContext);

   for(S32 i = 0; i < mSymbols.size(); i++)
   {
      mSymbols[i]->render(Point(x + mSymbols[i]->getWidth() / 2, y), mFontSize, mFontContext);
      x += mSymbols[i]->getWidth();
   }

   FontManager::popFontContext();
}


// Locally defined class, used only here, and only for ensuring objects are cleaned up on exit
class SymbolHolder
{
private:
   static Vector<SymbolShape *> mKeySymbols;
   static SymbolGear *mSymbolGear;

public:
   SymbolHolder() 
   {
      if(mKeySymbols.size() == 0)                     // Should always be the case
         mKeySymbols.resize(LAST_KEYBOARD_KEY + 1);   // Values will be initialized to NULL
   }


   ~SymbolHolder()
   {
      mKeySymbols.deleteAndClear();                   // Delete objects created in getSymbol
      delete mSymbolGear;
   }


   SymbolShape *getSymbol(InputCode inputCode)
   {
      // Lazily initialize -- we're unlikely to actually need more than a few of these during a session
      if(!mKeySymbols[inputCode])
         mKeySymbols[inputCode] = new SymbolKey(InputCodeManager::inputCodeToString(inputCode));

      return mKeySymbols[inputCode];
   }


   SymbolShape *getSymbolGear()
   {
      if(!mSymbolGear)
         mSymbolGear = new SymbolGear();

      return mSymbolGear;
   }

};

// Statics used by SymbolHolder
Vector<SymbolShape *> SymbolHolder::mKeySymbols;      
SymbolGear *SymbolHolder::mSymbolGear = NULL;

static SymbolHolder symbolHolder;


// Static method
SymbolShape *SymbolString::getControlSymbol(InputCode inputCode)
{
   if(InputCodeManager::isKeyboardKey(inputCode))
      return symbolHolder.getSymbol(inputCode);
   else
   { 
      //TNLAssert(false, "Deal with it!");
      return symbolHolder.getSymbol(KEY_WORLD_0);
   }
}


// Static method
SymbolShape *SymbolString::getSymbolGear()
{
   return symbolHolder.getSymbolGear();
}


} } // Nested namespace

