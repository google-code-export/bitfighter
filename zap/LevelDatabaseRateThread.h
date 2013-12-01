//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _LEVEL_DATABASE_RATE_THREAD_H_
#define _LEVEL_DATABASE_RATE_THREAD_H_

#include "tnlThread.h"
#include <string>
#include "../master/DatabaseAccessThread.h"

namespace Zap
{

class ClientGame;
class LevelDatabaseRateThread : public Master::ThreadEntry
{
   string username;
   string user_password;
   string reqURL;
   string responseBody;
   S32 responseCode;
   S32 errorNumber;
public:

#define LEVEL_RATINGS_TABLE           \
   LEVEL_RATING(PlusOne,  "up" )      \
   LEVEL_RATING(Neutral,  "neutral" ) \
   LEVEL_RATING(MinusOne, "down" )    \


   enum LevelRating {
#define LEVEL_RATING(val, b) val,
    LEVEL_RATINGS_TABLE
#undef LEVEL_RATING
    RatingsCount
};


   static const string RatingStrings[];
      
   static const string LevelDatabaseRateUrl;
   LevelDatabaseRateThread(ClientGame* game, LevelRating rating);
   virtual ~LevelDatabaseRateThread();
   void run();
   void finish();

   ClientGame* mGame;
   string mLevelId;
   LevelRating mRating;
};

} /* namespace Zap */
#endif /* _LEVEL_DATABASE_RATE_THREAD_H_ */
