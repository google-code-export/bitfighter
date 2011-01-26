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


#include "database.h"
#include "tnlTypes.h"
#include "tnlLog.h"
#include "../zap/stringUtils.h"            // For replaceString()

#ifdef CHRIS
#include "../mysql++/mysql++.h"
#elif SAM_ONLY
#include "mysql++.h"
#else
#include "../mysql++/lib/mysql++.h"
#endif

using namespace std;
using namespace mysqlpp;
using namespace TNL;


// Default constructor -- don't use this one!
DatabaseWriter::DatabaseWriter()
{
   mIsValid = false;
}


// Constructor
DatabaseWriter::DatabaseWriter(const char *server, const char *db, const char *user, const char *password)
{
   initialize(server, db, user, password);
}


// Constructor -- defaults to local machine for db server
DatabaseWriter::DatabaseWriter(const char *db, const char *user, const char *password)
{
   initialize("127.0.0.1", db, user, password);
}


void DatabaseWriter::initialize(const char *server, const char *db, const char *user, const char *password)
{
   mServer = server;
   mDb = db;
   mUser = user;
   mPassword = password;
   mIsValid = true;
}


static string sanitize(const string &value)     
{
   return replaceString(replaceString(value, "\\", "\\\\"), "'", "''");
}


static void doInsertStatsShots(Query query, const string &playerId, const string &weapon, S32 shots, S32 hits)
{
   string sql = "INSERT INTO stats_player_shots(stats_player_id, weapon, shots, shots_struck)  \
                 VALUES(" + playerId + ", '" + weapon + "', " + itos(shots) + ", " + itos(hits) + ");";

   SimpleResult result = query.execute(sql);
}


static void insertStatsShots(Query query, const string &playerId, const Vector<WeaponStats> weaponStats)
{
   for(S32 i = 0; i < weaponStats.size(); i++)
      doInsertStatsShots(query, playerId, WeaponInfo::getWeaponName(weaponStats[i].weaponType), weaponStats[i].shots, weaponStats[i].hits);
}


#define btos(value) (value ? "1" : "0")

void DatabaseWriter::insertStats(const GameStats &gameStats) 
{
   Connection conn;    // Connect to the database

   string sql;
   try
   {
      if(!mIsValid)
      {
         logprintf("Invalid DatabaseWriter!");
         return;
      }

      conn.connect(mDb, mServer, mUser, mPassword);    // Will throw error if it fails
      
      Query query(&conn);
      SimpleResult result;
      U64 serverId_int = U64_MAX;

      for(S32 i = cachedServers.size() - 1; i >= 0; i--)
		{
			if(cachedServers[i].ip == gameStats.serverIP	&&
			   cachedServers[i].name == gameStats.serverName )
			{
				serverId_int = cachedServers[i].id;
				break;
			}
		}

      if(serverId_int == U64_MAX)  // Not in cache
      {
         // Find server in database
			sql = "SELECT server_id FROM test.server AS server WHERE server_name = '" + sanitize(gameStats.serverName)
				+ "' AND ip_address = '" + gameStats.serverIP + "'";
         StoreQueryResult results = query.store(sql.c_str(),sql.length());

         if(results.num_rows() >= 1)
            serverId_int = results[0][0];

         if(serverId_int == U64_MAX)      // Not in database
         {
            sql = "INSERT INTO server(server_name, ip_address) \
                  VALUES('" + sanitize(gameStats.serverName) + "', '" + gameStats.serverIP + "');";
            result = query.execute(sql);
            serverId_int = result.insert_id();
         }

         // Limit cache growth
         if(cachedServers.size() > 20) 
            cachedServers.erase(0);

         cachedServers.push_back(ServerInformation(serverId_int, gameStats.serverName, gameStats.serverIP));
      }

      string serverId = itos(serverId_int);

      if(gameStats.isTeamGame)
      {
         sql = "INSERT INTO stats_game(server_id, game_type, is_official, player_count, \
                     duration_seconds, level_name, is_team_game, \
                     team_count, is_tied) \
                VALUES( " + serverId + ", '" + gameStats.gameType + "', " + btos(gameStats.isOfficial) + ", " + itos(gameStats.playerCount) + ", " +
                       itos(gameStats.duration) + ", '" + sanitize(gameStats.levelName) + "', 1, " + 
                       itos(gameStats.teamCount) + ", " + btos(gameStats.isTied) + ");";

         result = query.execute(sql);
			U64 gameID = result.insert_id();

         if(gameID <= lastGameID)      // ID should only go bigger, if not, database might have changed        
            while(cachedServers.size() != 0) 
               cachedServers.erase_fast(0);  

         lastGameID = gameID;
         string gameId = itos(gameID);


         for(S32 i = 0; i < gameStats.teamStats.size(); i++)
         {
            const TeamStats *teamStats = &gameStats.teamStats[i];
            sql = "INSERT INTO stats_team(stats_game_id, team_name, team_score, result, color) \
                   VALUES(" + gameId + ", '" + sanitize(teamStats->name) + "', " + itos(teamStats->score) + " ,'" + 
                              teamStats->gameResult + "' ,'" + teamStats->color + "');";   // TODO: fix color_hex here

            query = conn.query(sql);
            result = query.execute();
            string teamId = itos(result.insert_id());
            
            for(S32 j = 0; j < teamStats->playerStats.size(); j++)
            {
               const PlayerStats *playerStats = &teamStats->playerStats[j];
               sql = "INSERT INTO stats_player(stats_game_id, stats_team_id, player_name, \
                                               is_authenticated, is_robot, \
                                               result, points, kill_count, \
                                               death_count, \
                                               suicide_count, switched_team) \
                      VALUES(" + gameId + ", " + teamId + ", '" + sanitize(playerStats->name) + "', " + 
                                 btos(playerStats->isAuthenticated) + ", '" + btos(playerStats->isRobot), 
                                 playerStats->gameResult + "', " + itos(playerStats->points) + ", " + itos(playerStats->kills) + ", " + 
                                 itos(playerStats->deaths),
                                 itos(playerStats->suicides) + ", " + btos(playerStats->switchedTeams) + ");";

               result = query.execute(sql);
               string playerId = itos(result.insert_id());

               insertStatsShots(query, playerId, playerStats->weaponStats);
            }
         }
      }
      else     // Not team game
      {
         sql = "INSERT INTO stats_game(server_id, game_type, is_official, player_count, \
                                       duration_seconds, level_name, is_team_game, team_count, is_tied) \
                VALUES(" + serverId + ", '" + gameStats.gameType + "', " + btos(gameStats.isOfficial) + ", " + itos(gameStats.playerCount) + ", " + 
                           itos(gameStats.duration) + ", '" + sanitize(gameStats.levelName) + "', 0, NULL, " +  btos(gameStats.isTied) + ");";

         query = conn.query(sql);
         result = query.execute();
			U64 gameID = result.insert_id();

         if(gameID <= lastGameID)         // ID should only go bigger, if not, database might have changed
            while(cachedServers.size() != 0) 
               cachedServers.erase_fast(0);  

         lastGameID = gameID;
         string gameId = itos(gameID);

         for(S32 i = 0; i < gameStats.teamStats[0].playerStats.size(); i++)
         {
            const PlayerStats *playerStats = &gameStats.teamStats[0].playerStats[i];
            sql = "INSERT INTO stats_player(stats_game_id, player_name, is_authenticated, \
                                            result, points, \
                                            kill_count, suicide_count) \
                   VALUES(" + gameId + ", '" + sanitize(playerStats->name) + "', " + btos(playerStats->isAuthenticated) + ", '" + 
                              playerStats->gameResult + "', " + itos(playerStats->points) + ", " + 
                              itos(playerStats->kills) + ", " + itos(playerStats->suicides) + ");";

            result = query.execute(sql);
            string playerId = itos(result.insert_id());

            insertStatsShots(query, playerId, playerStats->weaponStats);
         }
      }
   }

   catch (const BadOption &ex) {
      logprintf("Bad connection option: %s", ex.what());
      return;
   }
   catch (const ConnectionFailed &ex) {
      logprintf("Connection failed: %s", ex.what());        
      return;
   }
   catch (const Exception &ex) {
      // Catch-all for any other MySQL++ exceptions
		logprintf("General connection failure: %s Query: %s", ex.what(), sql.c_str());
      return;
    }
}


