//------------------------------------------------------------------------------------
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

#include "master.h"
#include "database.h"            // For writing to the database
#include "DatabaseAccessThread.h"

#include "../zap/stringUtils.h"  // For itos, replaceString
#include "../zap/IniFile.h"      // For INI reading/writing


namespace Zap
{
   extern string gSqlite;
}


namespace Master 
{


// Constructor
MasterSettings::MasterSettings(const string &iniFile)     
{
   ini.SetPath(iniFile);

   // Note that on the master, our settings are read-only, so there is no need to specify a comment
   //                      Data type  Setting name                       Default value         INI Key                              INI Section                                  
   mSettings.add(new Setting<string>("ServerName",                 "Bitfighter Master Server", "name",                                 "host"));
   mSettings.add(new Setting<string>("JsonOutfile",                      "server.json",        "json_file",                            "host"));
   mSettings.add(new Setting<U32>   ("Port",                                 25955,            "port",                                 "host"));
   mSettings.add(new Setting<U32>   ("LatestReleasedCSProtocol",               0,              "latest_released_cs_protocol",          "host"));
   mSettings.add(new Setting<U32>   ("LatestReleasedBuildVersion",             0,              "latest_released_client_build_version", "host"));
                                                                                               
   // Variables for managing access to MySQL                                                   
   mSettings.add(new Setting<string>("MySqlAddress",                           "",             "phpbb_database_address",               "phpbb"));
   mSettings.add(new Setting<string>("DbUsername",                             "",             "phpbb3_database_username",             "phpbb"));
   mSettings.add(new Setting<string>("DbPassword",                             "",             "phpbb3_database_password",             "phpbb"));
                                                                                               
   // Variables for verifying usernames/passwords in PHPBB3                                    
   mSettings.add(new Setting<string>("Phpbb3Database",                         "",             "phpbb3_database_name",                 "phpbb"));
   mSettings.add(new Setting<string>("Phpbb3TablePrefix",                      "",             "phpbb3_table_prefix",                  "phpbb"));
                                                                                               
   // Stats database credentials                                                               
   mSettings.add(new Setting<YesNo> ("WriteStatsToMySql",                      No,             "write_stats_to_mysql",                 "stats"));
   mSettings.add(new Setting<string>("StatsDatabaseAddress",                   "",             "stats_database_addr",                  "stats"));
   mSettings.add(new Setting<string>("StatsDatabaseName",                      "",             "stats_database_name",                  "stats"));
   mSettings.add(new Setting<string>("StatsDatabaseUsername",                  "",             "stats_database_username",              "stats"));
   mSettings.add(new Setting<string>("StatsDatabasePassword",                  "",             "stats_database_password",              "stats"));
}                  


void MasterSettings::readConfigFile()
{
   if(ini.getPath() == "")
      return;

   // Clear, then read
   ini.Clear();
   ini.ReadFile();

   // Now set up variables -- copies data from ini to settings
   loadSettingsFromINI();

   // Not sure if this should go here...
   if(getVal<U32>("LatestReleasedCSProtocol") == 0 && getVal<U32>("LatestReleasedBuildVersion") == 0)
      logprintf(LogConsumer::LogError, "Unable to find a valid protocol line or build_version in config file... disabling update checks!");
}


extern Vector<string> master_admins;
extern map <U32, string> gMOTDClientMap;


void MasterSettings::loadSettingsFromINI()
{
   // Read all settings defined in the new modern manner
   string sections[] = { "host", "phpbb", "stats", "motd" };

   for(S32 i = 0; i < ARRAYSIZE(sections); i++)
   {
      string section = sections[i];

      // Enumerate all settings we've defined for [section]
      Vector<AbstractSetting *> settings = mSettings.getSettingsInSection(section);

      for(S32 j = 0; j < settings.size(); j++)
         settings[j]->setValFromString(ini.GetValue(section, settings[j]->getKey(), settings[j]->getDefaultValueString()));
   }


   // Got to do something about this!
   string str1 = ini.GetValue("host", "master_admin", "");
   parseString(str1.c_str(), master_admins, ',');


   // [stats] section --> most has been modernized
   Zap::gSqlite = ini.GetValue("stats", "sqlite_file_basename", Zap::gSqlite);


   // [motd_clients] section
   // This section holds each old client build number as a key.  This allows us to set
   // different messages for different versions
   string defaultMessage = "New version available at bitfighter.org";
   Vector<string> keys;
   ini.GetAllKeys("motd_clients", keys);

   gMOTDClientMap.clear();

   for(S32 i = 0; i < keys.size(); i++)
   {
      U32 build_version = (U32)Zap::stoi(keys[i]);    // Avoid conflicts with std::stoi() which is defined for VC++ 10
      string message = ini.GetValue("motd_clients", keys[i], defaultMessage);

      gMOTDClientMap.insert(pair<U32, string>(build_version, message));
   }


   // [motd] section
   // Here we just get the name of the file.  We use a file so the message can be updated
   // externally through the website
   string motdFilename = ini.GetValue("motd", "motd_file", "motd");  // Default 'motd' in current directory

   // Grab the current message and add it to the map as the most recently released build
   gMOTDClientMap[getVal<U32>("LatestReleasedBuildVersion")] = getCurrentMOTDFromFile(motdFilename);
}


string MasterSettings::getCurrentMOTDFromFile(const string &filename) const
{
   string defaultMessage = "Welcome to Bitfighter!";

   FILE *f = fopen(filename.c_str(), "r");
   if(!f)
   {
      logprintf(LogConsumer::LogError, "Unable to open MOTD file \"%s\" -- using default MOTD.", filename.c_str());
      return defaultMessage;
   }

   char message[MOTD_LEN];
   bool ok = fgets(message, MOTD_LEN, f);
   fclose(f);

   if(ok)
      return message;

   return defaultMessage; 
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
MasterServer::MasterServer(MasterSettings *settings)
{
   mSettings = settings;

   mStartTime = Platform::getRealMilliseconds();

   // Initialize our net interface so we can accept connections...  mNetInterface is deleted in destructor
   mNetInterface = createNetInterface();

   mCleanupTimer.reset(TEN_MINUTES);
   mReadConfigTimer.reset(FIVE_SECONDS);     // Reread the config file every 5 seconds... excessive?
   mJsonWriteTimer.reset(0, FIVE_SECONDS);   // Max frequency for writing JSON files -- set current to 0 so we'll write immediately
   mJsonWritingSuspended = false;
   
   mDatabaseAccessThread = new DatabaseAccessThread();    // Deleted in destructor
   mDatabaseAccessThread->start();            

   MasterServerConnection::setMasterServer(this);
}


// Destructor
MasterServer::~MasterServer()
{
   delete mNetInterface;

   // Turn off the database access thread
   mDatabaseAccessThread->terminate();
   //delete mDatabaseAccessThread;
   //mDatabaseAccessThread = NULL;
}


NetInterface *MasterServer::createNetInterface() const
{
   U32 port = mSettings->getVal<U32>("Port");
   NetInterface *netInterface = new NetInterface(Address(IPProtocol, Address::Any, port));

   // Log a welcome message in the main log and to the console
   logprintf("[%s] Master Server %s started - listening on port %d", getTimeStamp().c_str(),
                                                                     getSetting<string>("ServerName").c_str(),
                                                                     port);
   return netInterface;
}


U32 MasterServer::getStartTime() const
{
   return mStartTime;
}


const MasterSettings *MasterServer::getSettings() const
{
   return mSettings;
}


// Will trigger a JSON rewrite after timer has run its full cycle
void MasterServer::writeJsonDelayed()
{
   mJsonWriteTimer.reset();
   mJsonWritingSuspended = false;
}


// Indicates we want to write JSON as soon as possible... but never more 
// frequently than allowed by mJsonWriteTimer, which we don't reset here
void MasterServer::writeJsonNow()
{
   mJsonWritingSuspended = false;
}


const Vector<MasterServerConnection *> *MasterServer::getServerList() const
{
   return &mServerList;
}


const Vector<MasterServerConnection *> *MasterServer::getClientList() const
{
   return &mClientList;
}


void MasterServer::addServer(MasterServerConnection *server)
{
   mServerList.push_back(server);
}


void MasterServer::addClient(MasterServerConnection *client)
{
   mClientList.push_back(client);
}


void MasterServer::removeServer(S32 index)
{
   TNLAssert(index >= 0 && index < mServerList.size(), "Index out of range!");
   mServerList.erase_fast(index);
}


void MasterServer::removeClient(S32 index)
{
   TNLAssert(index >= 0 && index < mClientList.size(), "Index out of range!");
   mClientList.erase_fast(index);
}


NetInterface *MasterServer::getNetInterface() const
{
   return mNetInterface;
}


void MasterServer::idle(U32 timeDelta)
{
   mNetInterface->checkIncomingPackets();
   mNetInterface->processConnections();

   // Reread config file
   if(mReadConfigTimer.update(timeDelta))
   {
      mSettings->readConfigFile();
      mReadConfigTimer.reset();
   }

   // Cleanup, cleanup, everybody cleanup!
   if(mCleanupTimer.update(timeDelta))
   {
      MasterServerConnection::removeOldEntriesFromRatingsCache();    //<== need non-static access
      mCleanupTimer.reset();
   }


   // Handle writing our JSON file
   mJsonWriteTimer.update(timeDelta);

   if(!mJsonWritingSuspended && mJsonWriteTimer.getCurrent() == 0)
   {
      MasterServerConnection::writeClientServerList_JSON();

      mJsonWritingSuspended = true;    // No more writes until this is cleared
      mJsonWriteTimer.reset();         // But reset the timer so it start ticking down even if we aren't writing
   }


   // Process connections -- cycle through them and check if any have timed out
   U32 currentTime = Platform::getRealMilliseconds();

   for(S32 i = MasterServerConnection::gConnectList.size() - 1; i >= 0; i--)     //< Get rid of global here
   {
      GameConnectRequest *request = MasterServerConnection::gConnectList[i];     //< Get rid of global here

      if(currentTime - request->requestTime > FIVE_SECONDS)  
      {
         if(request->initiator.isValid())
         {
            ByteBufferPtr ptr = new ByteBuffer((U8 *)MasterRequestTimedOut, strlen(MasterRequestTimedOut) + 1);

            request->initiator->m2cArrangedConnectionRejected(request->initiatorQueryId, ptr);   // 0 = ReasonTimedOut
            request->initiator->removeConnectRequest(request);
         }

         if(request->host.isValid())
            request->host->removeConnectRequest(request);

         MasterServerConnection::gConnectList.erase_fast(i);
         delete request;
      }
   }

   // Process any delayed disconnects; we use this to avoid repeating and flooding join / leave messages
   for(S32 i = MasterServerConnection::gLeaveChatTimerList.size() - 1; i >= 0; i--)
   {
      MasterServerConnection *c = MasterServerConnection::gLeaveChatTimerList[i];      //< Get rid of global here

      if(!c || c->mLeaveGlobalChatTimer == 0)
         MasterServerConnection::gLeaveChatTimerList.erase(i);                         //< Get rid of global here
      else
      {
         if(currentTime - c->mLeaveGlobalChatTimer > ONE_SECOND)
         {
            c->isInGlobalChat = false;

            const Vector<MasterServerConnection *> *serverList = getServerList();

            for(S32 j = 0; j < serverList->size(); j++)
               if(serverList->get(j) != c && serverList->get(j)->isInGlobalChat)
                  serverList->get(j)->m2cPlayerLeftGlobalChat(c->mPlayerOrServerName);

            MasterServerConnection::gLeaveChatTimerList.erase(i);
         }
      }
   }

   mDatabaseAccessThread->idle();
}


DatabaseAccessThread *MasterServer::getDatabaseAccessThread()
{
   return mDatabaseAccessThread;
}

}  // namespace

