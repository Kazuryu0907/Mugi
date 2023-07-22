#include "pch.h"
#include "Mugi.h"
#include <nlohmann/json.hpp>

#define TOS(i) std::to_string(i)

BAKKESMOD_PLUGIN(Mugi, "Mugi is Moca's friend's cat.", plugin_version, PLUGINTYPE_FREEPLAY)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;
using json = nlohmann::json;

void Mugi::onLoad()
{
	cvarManager->log("init Sock");
	initSocket();
	json root;
	root["cmd"] = "init";
	sendSocket(root.dump());
	createNameTable(true);
	//gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.OnGameTimeUpdated", std::bind(&Mugi::updateScore, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded", std::bind(&Mugi::endGame, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.ReplayDirector_TA.Tick", std::bind(&Mugi::tick, this, std::placeholders::_1));
	//Function GameEvent_Soccar_TA.Active.StartRound
	gameWrapper->HookEvent("Function GameEvent_Soccar_TA.Active.BeginState", std::bind(&Mugi::startGame, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.EventPlayerScored", std::bind(&Mugi::scored, this, std::placeholders::_1));
	struct DemolishData {
		uintptr_t attacker;
	};
	struct DemolishParams {
		uintptr_t attacker;
		struct DemolishData;
	};
	gameWrapper->HookEventWithCaller<CarWrapper>("Function TAGame.GameEvent_TA.OnReplicatedDemolish", [this](CarWrapper caller, void* params, std::string eventname) {
		cvarManager->log("demolished by ");
		DemolishParams* d_params = (DemolishParams*)params;
		CarWrapper attacker = CarWrapper(d_params->attacker);
		if (attacker.IsNull())return;
		std::string attackerName = "Player_" + attacker.GetOwnerName();//player�̂ݍl��
		if (OwnerIndexMap.count(attackerName) != 0) {
			// sendSocket("?d:" + TOS(OwnerIndexMap[attackerName]));//demo
		}
		});
	gameWrapper->HookEvent("Function ReplayDirector_TA.PlayingHighlights.PlayRandomHighlight", [this](std::string eventName) {
		json root;
		root["cmd"] = "endStats";
		sendSocket(root.dump());
		});
	//Function TAGame.GameEvent_Soccar_TA.EventPlayerScored
	//Function GameFramework.GameThirdPersonCamera.GetFocusActor
	//
	//Function TAGame.Camera_Replay_TA.UpdateCameraState already

	//Function TAGame.CameraState_DirectorPlayerView_TA.FindFocusCar
	//Function TAGame.GFxData_ReplayViewer_TA.SetFocusActorString
	//Function TAGame.CameraState_Director_TA.UpdateSelector
	//Function TAGame.ReplayDirector_TA.EventScoreDataChanged

}


void Mugi::onUnload()
{
	endSocket();
	gameWrapper->UnhookEvent("Function TAGame.ReplayDirector_TA.Tick");
	gameWrapper->UnhookEvent("Function TAGame.Camera_Replay_TA.SetFocusActor");
	gameWrapper->UnhookEvent("Function TAGame.Camera_TA.OnViewTargetChanged");
	//Function GameEvent_Soccar_TA.Active.StartRound
	gameWrapper->UnhookEvent("Function GameEvent_Soccar_TA.Active.BeginState");
	gameWrapper->UnhookEvent("Function TAGame.GameEvent_Soccar_TA.EventPlayerScored");
}

void Mugi::initSocket() {
	WSADATA wsaData;
	struct sockaddr_in server;
	char buf[32];
	cvarManager->log("initilizing socket...");
	if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0) {
		cvarManager->log("send error");
		return;
	}
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	server.sin_family = AF_INET;
	server.sin_port = htons(PORT);
	inet_pton(server.sin_family, ADDR.c_str(), &server.sin_addr.s_addr);
	connect(sock, (struct sockaddr*)&server, sizeof(server));
}

bool Mugi::sendSocket(std::string str) {
	bool res = send(sock, str.c_str(), str.length(), 0);
	return res;
}
void Mugi::endSocket() {
	closesocket(dst_socket);
	WSACleanup();
}
void Mugi::scored(std::string eventName) {
	json root;
	root["cmd"] = "scored";
	sendSocket(root.dump());
}
void Mugi::startGame(std::string eventName) {
	createNameTable(false);
	isBoostWatching = true;
	ServerWrapper sw = gameWrapper->GetOnlineGame();
	if (sw.IsNull())return;
	json root;
	root["cmd"] = "start";
	if (sw.GetTotalScore() == 0)sendSocket(root.dump());
}
void Mugi::endGame(std::string eventName) {
	isBoostWatching = false;
	json root;
	root["cmd"] = "end";
	sendSocket(root.dump());
	ServerWrapper sw = gameWrapper->GetOnlineGame();
	if (sw.IsNull())return;
	std::vector<json> Stats;

	ArrayWrapper<PriWrapper> pls = sw.GetPRIs();
	for (int i = 0; i < pls.Count(); i++) {
		json j;

		auto pl = pls.Get(i);
		if (pl.IsNull())continue;
		if (pl.GetTeamNum() == 255)continue;

		j["names"] = (pl.GetPlayerName().ToString());
		j["teams"] = (pl.GetTeamNum());
		j["scores"] = (pl.GetMatchScore());
		j["goals"] = (pl.GetMatchGoals());
		j["assists"] = (pl.GetMatchAssists());
		j["saves"] = (pl.GetMatchSaves());
		j["shots"] = (pl.GetMatchShots());
		j["demos"] = (pl.GetMatchDemolishes());
		j["ballTouches"] = (pl.GetBallTouches());
		Stats.push_back(j);
	}
	//orange H->L -> blue H->L
	std::sort(Stats.begin(), Stats.end(), [](const json& a, const json& b) {return(a["scores"] > b["scores"]); });
	std::sort(Stats.begin(), Stats.end(), [](const json& a, const json& b) {return(a["teams"] > b["teams"]); });

	root["cmd"] = "stats";
	root["data"] = Stats;
	sendSocket(root.dump());
}

void Mugi::createNameTable(bool isForcedRun)
{
	ServerWrapper sw = gameWrapper->GetOnlineGame();
	if (sw.IsNull())return;
	ArrayWrapper<PriWrapper> pls = sw.GetPRIs();
	ArrayWrapper<CarWrapper> cars = sw.GetCars();
	//only run first or onload
	if (!isForcedRun && sw.GetTotalScore() != 0)return;
	cvarManager->log("PLS:" + TOS(pls.Count()));

	//-----clear------//
	PlayerMap.clear();
	OwnerMap.clear();
	DisplayName2Id.clear();
	UniqueID2DisplayName.clear();
	OwnerIndexMap.clear();
	//----------------//
	for (int i = 0; i < pls.Count(); i++) {
		auto pl = pls.Get(i);
		if (pl.IsNull())continue;
		std::string displayName = "";
		//本来はuniqueID
		std::string playerId = TOS(i);
		// if human
		if (!pl.GetbBot())playerId = "Player_" + pl.GetUniqueIdWrapper().GetIdString();


		//観戦時のプレイヤー名に合わせるため
		if (pl.GetbBot())displayName = "Player_Bot_" + pl.GetOldName().ToString();
		else			 displayName = pl.GetPlayerName().ToString();
		cvarManager->log(displayName);
		auto ppl = std::make_shared<PriWrapper>(pl);

		if (pl.GetbBot())PlayerMap[displayName] = ppl;
		else            PlayerMap[playerId] = ppl;

		DisplayName2Id[displayName] = playerId;
		Id2DisplayName[playerId] = displayName;
		cvarManager->log(":=" + playerId);
		if (pl.GetTeamNum() != 255) {//not �ϐ��
			playerData p = { displayName,playerId ,pl.GetTeamNum() };//isblue
			OwnerMap.push_back(p);
		}

	}
	//�`�[����sort
	std::sort(OwnerMap.begin(), OwnerMap.end(), [](const playerData& a, const playerData& b) {return(a.team > b.team); });
	std::vector<std::string> PlayerIndexs;
	for (int i = 0; i < OwnerMap.size(); i++) {
		auto p = OwnerMap[i];
		if (isDebug)OwnerIndexMap[p.name] = i;
		else OwnerIndexMap[p.id] = i;
		cvarManager->log("name!"+p.name);
		//後々p.nameからidに代わる予定
		if (isDebug) PlayerIndexs.push_back(TOS(i));
		else PlayerIndexs.push_back(p.id);
	}
	json root;
	root["cmd"] = "playerTable";
	root["data"] = PlayerIndexs;
	sendSocket(root.dump());
}

void Mugi::tickBoost(ServerWrapper gw) {
	if (isBoostWatching) {
		auto cars = gw.GetCars();
		for (int i = 0; i < cars.Count(); i++) {
			auto car = cars.Get(i);
			//!!!!!!!!!----only noBot--!!!!!!!!//
			std::string playerId = DisplayName2Id[car.GetOwnerName()];
			if (isDebug)playerId = "Player_Bot_" + car.GetOwnerName();
			if (car.IsNull())continue;
			auto boostCom = car.GetBoostComponent();
			if (boostCom.IsNull())continue;
			int boost = int(boostCom.GetCurrentBoostAmount() * 100);
			if (OwnerIndexMap.count(playerId) == 0)continue;
			json root;
			json j;
			j["boost"] = boost;
			j["index"] = i;
			root["cmd"] = "boost";
			root["data"] = j;
			if (boost != Boosts[i])sendSocket(root.dump());
			Boosts[i] = boost;
		}
	}
}

void Mugi::tickScore(std::string actorName) {
	if (PlayerMap.count(actorName) == 0) return;
	auto pl = PlayerMap[actorName];
	currentFocusActorScore = pl->GetMatchScore();
	if (currentFocusActorScore != preFocusActorScore) {
		json root;
		json j;
		j["score"] = currentFocusActorScore;
		root["cmd"] = "score";
		root["data"] = j;
		sendSocket(root.dump());
	}
	preFocusActorScore = currentFocusActorScore;
}

void Mugi::tickPlayer(std::string actorName) {
	if (actorName != preActorName) {
		if (actorName == "") {//if flying
			//sendSocket("f0");
		}
		//空白でもcountに引っかからないはず...
		//send FOCUS
		if (OwnerIndexMap.count(actorName) != 0) {
			cvarManager->log(actorName);
			json root;
			json j;
			root["cmd"] = "player";
			j["playerIndex"] = TOS(OwnerIndexMap[actorName]);
			if (isDebug) {
				j["playerName"] = actorName;
			}
			else {
				j["playerName"] = Id2DisplayName[actorName];
			}
			root["data"] = j;
			if (isSendSocket)sendSocket(root.dump());
			preActorName = actorName;
		}
	}
}

void Mugi::tick(std::string eventName) {
	auto gw = gameWrapper->GetOnlineGame();
	if (gw.IsNull())return;
	CameraWrapper camera = gameWrapper->GetCamera();
	std::string actorName = camera.GetFocusActor();
	//----------player------------//
	tickPlayer(actorName);
	//----------boost-------------//
	tickBoost(gw);
	//----------score-------------//
	tickScore(actorName);
}
