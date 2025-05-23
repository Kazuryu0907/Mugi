#pragma once

#include "GuiBase.h"
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <unordered_map>

#include "version.h"
constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);


class Mugi: public BakkesMod::Plugin::BakkesModPlugin
	//,public SettingsWindowBase // Uncomment if you wanna render your own tab in the settings menu
	//,public PluginWindowBase // Uncomment if you want to render your own plugin window
{

	//std::shared_ptr<bool> enabled;

	//Boilerplate
	virtual void onLoad();
	virtual void onUnload();

	void onStatEvent(ServerWrapper caller, void* args);
	void createNameTable(bool);
	void updateTime(std::string);
	void startGame(std::string);
	void scored(std::string);
	void tick(std::string);
	void tickPlayer(std::string);
	void tickBoost(ServerWrapper sw);
	void tickScore(std::string);
	void onGoal(ActorWrapper caller);
	void initSocket();
	void endSocket();
//	void calcSetPoint(ServerWrapper sw);
//	void resetSetPoint(ServerWrapper sw);
	void sendTeamNames(ServerWrapper);
	bool sendSocket(std::string);
	void endGame(std::string);

	std::string split(const std::string& s);
	//std::string split(const std::string& s);
	//void onUnload() override; // Uncomment and implement if you need a unload method

private:
	int PORT = 12345;
	std::string ADDR = "127.0.0.1";
	SOCKET sock;
	struct sockaddr_in server;
	std::unordered_map<std::string, std::shared_ptr<PriWrapper>> PlayerMap;
	bool isBoostWatching = true;
	struct playerData {
		std::string name;
		std::string id;
		unsigned char team;//isOrange
	};
	struct carData {
		std::shared_ptr<CarWrapper> car;
		unsigned char isBot;
	};
	struct s_currentSetPoint {
		int blue = 0;
		int orange = 0;
	};
	struct s_preTeamName {
		std::string blue = "";
		std::string orange = "";
	};
	struct StatEventStruct {
		uintptr_t PRI;
		uintptr_t StatEvent;
		// Count always is int_max. No idea why
		uintptr_t Count;
	};
	std::string preSubScore;
	std::vector<playerData> OwnerMap;
	std::unordered_map<std::string, int> OwnerIndexMap;
	std::unordered_map<std::string, std::string> OwnerTeamMap;
	struct s_currentSetPoint currentSetPoint;
	struct s_preTeamName preTeamName;
	std::string preMatchId;

	int Boosts[10];
	int botIndex[6] = { 0,0,0,0,0,0 };
	std::unordered_map<std::string, std::string> botId2Id;
	std::unordered_map<std::string, std::string> DisplayName2Id;
	std::unordered_map<std::string, std::string> Id2DisplayName;
	std::unordered_map<std::string, std::string> Id2DisplayName_debug;
	std::unordered_map<std::string, std::string> PlayerToDisplayName;
	std::unordered_map<std::string, std::string> UniqueID2DisplayName;
	std::string preActorName = "";
	int currentFocusActorScore = 0;
	std::string preAutoCamActorName = "";
	std::string currentFocusActorName = "";
	std::string preFocusActorName = "";
	int overtimeOffset = 0;
	int preFocusActorScore = 0;
	int dst_socket;
	std::string preMsg = "";
	std::string msg = "";
	bool isSendSocket = true;
	bool isDebug = false;

	//void RenderSettings() override; // Uncomment if you wanna render your own tab in the settings menu
	//void RenderWindow() override; // Uncomment if you want to render your own plugin window
};
