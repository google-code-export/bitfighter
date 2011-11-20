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

#ifndef _GRIDDB_H_
#define _GRIDDB_H_


#include "tnlTypes.h"
#include "tnlDataChunker.h"
#include "tnlVector.h"

#include "Rect.h"
#include "GeomUtils.h"


using namespace TNL;

namespace Zap
{

typedef bool (*TestFunc)(U8);

// Interface for dealing with objects that can be in our spatial database.  Can be either GameObjects or
// items in te
class  GridDatabase;

class DatabaseObject
{
friend class GridDatabase;

private:
   U32 mLastQueryId;
   Rect mExtent;
   GridDatabase *mDatabase;

protected:
   U8 mObjectTypeNumber;

public:
   DatabaseObject();                            // Constructor
   DatabaseObject(const DatabaseObject &t);     // Copy constructor

   void initialize();

   U8 getObjectTypeNumber() { return mObjectTypeNumber; }
   void setObjectTypeNumber(U8 objectTypeNumber) { mObjectTypeNumber = objectTypeNumber; }

   GridDatabase *getDatabase() { return mDatabase; }     // Returns the database in which this object is stored, NULL if not in any database
   void setDatabase(GridDatabase *database) { mDatabase = database; }

   Rect getExtent() const { return mExtent; }
   void setExtent(const Rect &extentRect);

   virtual bool getCollisionPoly(Vector<Point> &polyPoints) const = 0;
   virtual bool getCollisionCircle(U32 stateIndex, Point &point, float &radius) const = 0;
   virtual bool getCollisionRect(U32 stateIndex, Rect &rect) const = 0;

   virtual bool isCollisionEnabled() { return true; }

   bool isInDatabase() { return mDatabase != NULL; }
   bool isDeleted();

   void addToDatabase(GridDatabase *database);
   void addToDatabase(GridDatabase *database, const Rect &extent);

   void removeFromDatabase();

   virtual bool isDatabasable() { return true; }      // Can this item actually be inserted into a database?
};

////////////////////////////////////////
////////////////////////////////////////

class GridDatabase
{
private:

   void findObjects(U8 typeNumber, Vector<DatabaseObject *> &fillVector, const Rect *extents, S32 minx, S32 miny, S32 maxx, S32 maxy);
   void findObjects(TestFunc testFunc, Vector<DatabaseObject *> &fillVector, const Rect *extents, S32 minx, S32 miny, S32 maxx, S32 maxy);
   static U32 mQueryId;
   static U32 mCountGridDatabase;

protected:
   Vector<DatabaseObject *> mAllObjects;

public:
   enum {
      BucketRowCount = 16,    // Number of buckets per grid row, and number of rows; should be power of 2
      BucketMask = BucketRowCount - 1,
   };

   struct BucketEntry
   {
      DatabaseObject *theObject;
      BucketEntry *nextInBucket;
   };

   static ClassChunker<BucketEntry> *mChunker;  // if not a pointer, then somehow static get destroyed first then non-static when game is quitting, crashing it.

   BucketEntry *mBuckets[BucketRowCount][BucketRowCount];

   GridDatabase();                              // Constructor
   virtual ~GridDatabase();                     // Destructor

   static const S32 BucketWidthBitShift = 8;    // Width/height of each bucket in pixels, in a form of 2 ^ n, 8 is 256 pixels

   DatabaseObject *findObjectLOS(U8 typeNumber, U32 stateIndex, bool format, const Point &rayStart, const Point &rayEnd,
                                 float &collisionTime, Point &surfaceNormal);
   DatabaseObject *findObjectLOS(U8 typeNumber, U32 stateIndex, const Point &rayStart, const Point &rayEnd,
                                 float &collisionTime, Point &surfaceNormal);
   DatabaseObject *findObjectLOS(TestFunc testFunc, U32 stateIndex, bool format, const Point &rayStart, const Point &rayEnd,
                                 float &collisionTime, Point &surfaceNormal);
   DatabaseObject *findObjectLOS(TestFunc testFunc, U32 stateIndex, const Point &rayStart, const Point &rayEnd,
                                 float &collisionTime, Point &surfaceNormal);
   bool pointCanSeePoint(const Point &point1, const Point &point2);

   void findObjects(Vector<DatabaseObject *> &fillVector);     // Returns all objects in the database
   void findObjects(U8 typeNumber, Vector<DatabaseObject *> &fillVector);
   void findObjects(U8 typeNumber, Vector<DatabaseObject *> &fillVector, const Rect &extents);
   void findObjects(TestFunc testFunc, Vector<DatabaseObject *> &fillVector);
   void findObjects(TestFunc testFunc, Vector<DatabaseObject *> &fillVector, const Rect &extents);

   void dumpObjects();     // For debugging purposes

   
   virtual void addToDatabase(DatabaseObject *theObject, const Rect &extents);
   virtual void removeFromDatabase(DatabaseObject *theObject, const Rect &extents);
   void removeEverythingFromDatabase();

   S32 getObjectCount() { return mAllObjects.size(); }      // Return the number of objects currently in the database
   DatabaseObject *getObjectByIndex(S32 index);             // Kind of hacky, kind of useful
};


////////////////////////////////////////
////////////////////////////////////////

class EditorObject;

class EditorObjectDatabase : public GridDatabase
{
   typedef GridDatabase Parent;

private:
   Vector<EditorObject *> mAllEditorObjects;

public:
   EditorObjectDatabase();      // Constructor
   EditorObjectDatabase(const EditorObjectDatabase &database);    // Copy constructor
   EditorObjectDatabase &operator= (const EditorObjectDatabase &database);

   void copy(const EditorObjectDatabase &database);       // Copy contents of source into this

   const Vector<EditorObject *> *getObjectList();     

   void addToDatabase(DatabaseObject *theObject, const Rect &extents);

   void removeFromDatabase(DatabaseObject *theObject, const Rect &extents);
};

};

// Reusable container for searching gridDatabases
// putting it outside of Zap namespace seems to help with debugging showing whats inside fillVector  (debugger forgets to add Zap::)
extern Vector<Zap::DatabaseObject *> fillVector;
extern Vector<Zap::DatabaseObject *> fillVector2;

#endif


