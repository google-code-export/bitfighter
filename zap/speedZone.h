//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _GO_FAST_H_
#define _GO_FAST_H_

#include "SimpleLine.h"    // For SimpleLine def
#include "BfObject.h"
#include "gameType.h"
#include "gameNetInterface.h"
#include "gameObjectRender.h"

namespace Zap
{
class EditorAttributeMenuUI;

class SpeedZone : public SimpleLine
{
   typedef SimpleLine Parent;

private:
   Vector<Point> mPolyBounds, mOutline;
   U16 mSpeed;             // Speed at which ship is propelled, defaults to defaultSpeed
   bool mSnapLocation;     // If true, ship will be snapped to center of speedzone before being ejected
   
   // Take our basic inputs, pos and dir, and expand them into a three element
   // vector (the three points of our triangle graphic), and compute its extent
   void preparePoints();

public:
   enum {
      halfWidth = 25,
      height = 64,
      defaultSnap = 0,     // 0 = false, 1 = true

      InitMask     = BIT(0),
      HitMask      = BIT(1),
   };

   explicit SpeedZone(lua_State *L = NULL);     // Combined C++/Lua constructor
   virtual ~SpeedZone();               // Destructor
      
   SpeedZone *clone() const;

   static const U16 minSpeed;      // How slow can you go?
   static const U16 maxSpeed;      // Max speed for the goFast
   static const U16 defaultSpeed;  // Default speed if none specified

   U16 getSpeed();
   void setSpeed(U16 speed);

   bool getSnapping();
   void setSnapping(bool snapping);

   F32 mRotateSpeed;
   U32 mUnpackInit;  // Some form of counter, to know that it is a rotating speed zone.

   static void generatePoints(const Point &pos, const Point &dir, Vector<Point> &points, Vector<Point> &outline);
   void render() const;
   S32 getRenderSortValue();

   bool processArguments(S32 argc, const char **argv, Level *level);  // Create objects from parameters stored in level file
   string toLevelCode() const;

   void onAddedToGame(Game *game);

   const Vector<Point> *getOutline() const;
   const Vector<Point> *getEditorHitPoly() const;
   const Vector<Point> *getCollisionPoly() const;          // More precise boundary for precise collision detection
   bool collide(BfObject *hitObject);
   bool collided(BfObject *s, U32 stateIndex);
   void idle(BfObject::IdleCallPath path);
   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   ///// Editor methods 
   const Color &getEditorRenderColor() const;

   void renderEditor(F32 currentScale, bool snappingToWallCornersEnabled, bool renderVertices = false) const;

   void onAttrsChanging();
   void onGeomChanging();
   void onGeomChanged();
   Rect calcExtents() const;


#ifndef ZAP_DEDICATED
   bool startEditingAttrs(EditorAttributeMenuUI *attributeMenu);    // Called when we start editing to get menus populated
   void doneEditingAttrs(EditorAttributeMenuUI *attributeMenu);     // Called when we're done to retrieve values set by the menu

   void fillAttributesVectors(Vector<string> &keys, Vector<string> &values);
#endif

   // Some properties about the item that will be needed in the editor
   const char *getEditorHelpString() const;
   const char *getPrettyNamePlural() const;
   const char *getOnDockName() const;
   const char *getOnScreenName() const;

   bool hasTeam();
   bool canBeHostile();
   bool canBeNeutral();

   TNL_DECLARE_CLASS(SpeedZone);


   ///// Lua Interface
   LUAW_DECLARE_CLASS_CUSTOM_CONSTRUCTOR(SpeedZone);

   static const char *luaClassName;
   static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];

   S32 lua_setDir(lua_State *L);
   S32 lua_getDir(lua_State *L);
   S32 lua_setSpeed(lua_State *L);
   S32 lua_getSpeed(lua_State *L);
   S32 lua_setSnapping(lua_State *L);
   S32 lua_getSnapping(lua_State *L);
};


};

#endif


