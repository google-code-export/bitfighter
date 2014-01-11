//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _GAME_JOLT_CONNECTOR_H_
#define _GAME_JOLT_CONNECTOR_H_

#include "tnlVector.h"
#include "tnlLog.h"

namespace Master {
   class MasterSettings;
   class MasterServerConnection;
}

using namespace TNL;
using namespace Master;


namespace GameJolt
{
   void onPlayerAuthenticated     (const MasterSettings *settings, const MasterServerConnection *client);
   void onPlayerQuit              (const MasterSettings *settings, const MasterServerConnection *client);
   void ping                      (const MasterSettings *settings, const Vector<MasterServerConnection *> *clientList);
   void onPlayerAwardedAchievement(const MasterSettings *settings, const MasterServerConnection *client, S32 achievement);
}

#endif
