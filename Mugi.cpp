﻿#include "pch.h"
#include "Mugi.h"
#include "nlohmann/json.hpp"

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
	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.OnGameTimeUpdated", std::bind(&Mugi::updateTime, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded", std::bind(&Mugi::endGame, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.ReplayDirector_TA.Tick", std::bind(&Mugi::tick, this, std::placeholders::_1));
	//Function GameEvent_Soccar_TA.Active.StartRound
	gameWrapper->HookEvent("Function GameEvent_Soccar_TA.Active.BeginState", std::bind(&Mugi::startGame, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.EventPlayerScored", std::bind(&Mugi::scored, this, std::placeholders::_1));
	gameWrapper->HookEventWithCallerPost<ActorWrapper>("Function TAGame.ReplayDirector_TA.OnScoreDataChanged", std::bind(&Mugi::onGoal, this, std::placeholders::_1));
	gameWrapper->HookEventWithCallerPost<ServerWrapper>("Function TAGame.PRI_TA.ClientNotifyStatTickerMessage", std::bind(&Mugi::onStatEvent, this, std::placeholders::_1, std::placeholders::_2));

	struct DemolishData {
		uintptr_t attacker;
	};
	struct DemolishParams {
		uintptr_t attacker;
		struct DemolishData;
	};
	gameWrapper->HookEventPost("Function GameEvent_Soccar_TA.ReplayPlayback.EndState", [this](std::string eventName) {
		json root;
		root["cmd"] = "endReplay";
		sendSocket(root.dump());
		});
	gameWrapper->HookEventWithCaller<CarWrapper>("Function TAGame.GameEvent_TA.OnReplicatedDemolish", [this](CarWrapper caller, void* params, std::string eventname) {
		cvarManager->log("demolished by ");
		DemolishParams* d_params = (DemolishParams*)params;
		CarWrapper attacker = CarWrapper(d_params->attacker);
		if (attacker.IsNull())return;
		std::string attackerName = "Player_" + attacker.GetOwnerName();//player�̂ݍl��
		if (OwnerIndexMap.count(attackerName) != 0) {
			cvarManager->log(attackerName);
			// sendSocket("?d:" + TOS(OwnerIndexMap[attackerName]));//demo
		}
		});
	gameWrapper->HookEvent("Function ReplayDirector_TA.PlayingHighlights.PlayRandomHighlight", [this](std::string eventName) {
		json root;
		root["cmd"] = "endStats";
		sendSocket(root.dump());
		});

	// To Get TeamNames
	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.ShowSeasonIntroScene", [this](std::string eventname) {
		ServerWrapper sw = gameWrapper->GetOnlineGame();
		if (sw.IsNull())return;
		std::string matchId = sw.GetMatchGUID();
		// if room change
		if (preMatchId != matchId) {
			cvarManager->log("Show SeasonIntroScene");
			auto teams = sw.GetTeams();
			// Send TeamNames
			std::string blueTeamName = teams.Get(0).GetCustomTeamName().ToString();
			std::string orangeTeamName = teams.Get(1).GetCustomTeamName().ToString();
			json root, j;
			root["cmd"] = "teamNames";
			j["blue"] = blueTeamName;
			j["orange"] = orangeTeamName;
			j["matchId"] = matchId;
			root["data"] = j;
			sendSocket(root.dump());

			cvarManager->log("roomID:" + matchId + " blue:" + blueTeamName + " orange:" + orangeTeamName);
			preMatchId = matchId;
		}
		});

	gameWrapper->HookEventWithCallerPost<ServerWrapper>("Function TAGame.GFxHUD_TA.HandleStatTickerMessage",
		[this](ServerWrapper caller, void* params, std::string eventname) {
			struct StatTickerParams {
				uintptr_t Receiver;
				uintptr_t Victim;
				uintptr_t StatEvent;
			};

			StatTickerParams* pStruct = (StatTickerParams*)params;
			PriWrapper receiver = PriWrapper(pStruct->Receiver);
			PriWrapper victim = PriWrapper(pStruct->Victim);
			StatEventWrapper statEvent = StatEventWrapper(pStruct->StatEvent);

			if (statEvent.GetEventName() == "Demolish") {
				cvarManager->log("!!demolished by ");
				if (!receiver) { cvarManager->log("receiver is Null"); return; }
				if (!victim) { cvarManager->log("victim is Null"); return; }
				//観戦時のプレイヤー名に合わせるため
				int receiverIndex, victimIndex;
				std::string receiverDisplayName, victimDisplayName;
				if(isDebug){
					if (receiver.GetbBot())receiverDisplayName = "Player_Bot_" + receiver.GetOldName().ToString();
					if (victim.GetbBot())victimDisplayName = "Player_Bot_" + victim.GetOldName().ToString();
					receiverIndex = OwnerIndexMap[receiverDisplayName];
					victimIndex = OwnerIndexMap[victimDisplayName];
				} else {
					receiverIndex = OwnerIndexMap[split("Player_" + receiver.GetUniqueIdWrapper().GetIdString())];
					victimIndex = OwnerIndexMap[split("Player_" + victim.GetUniqueIdWrapper().GetIdString())];
				}

				cvarManager->log("receiver:" + TOS(receiverIndex) + " victim:" + TOS(victimIndex));
				json root, j;
				root["cmd"] = "demolished";
				j["receiverIndex"] = receiverIndex;
				j["victimIndex"] = victimIndex;
				root["data"] = j;
				sendSocket(root.dump());
			}
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
	gameWrapper->UnhookEvent("Function TAGame.GameEvent_Soccar_TA.OnGameTimeUpdated");
	gameWrapper->UnhookEvent("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded");
	gameWrapper->UnhookEvent("Function TAGame.ReplayDirector_TA.Tick");
	gameWrapper->UnhookEvent("Function GameEvent_Soccar_TA.Active.BeginState");
	gameWrapper->UnhookEvent("Function TAGame.GameEvent_Soccar_TA.EventPlayerScored");
	gameWrapper->UnhookEvent("Function TAGame.ReplayDirector_TA.OnScoreDataChanged");
	gameWrapper->UnhookEvent("Function TAGame.GFxHUD_TA.HandleStatEvent");
	
	gameWrapper->UnhookEvent("Function GameEvent_Soccar_TA.ReplayPlayback.EndState");
	gameWrapper->UnhookEvent("Function TAGame.GameEvent_TA.OnReplicatedDemolish");
	gameWrapper->UnhookEvent("Function ReplayDirector_TA.PlayingHighlights.PlayRandomHighlight");
	gameWrapper->UnhookEvent("Function TAGame.GameEvent_Soccar_TA.ShowSeasonIntroScene");
	gameWrapper->UnhookEvent("Function TAGame.PRI_TA.ClientNotifyStatTickerMessage");

}

void Mugi::onStatEvent(ServerWrapper caller, void* args){
	auto tArgs = (StatEventStruct*)args;

	auto statEvent = StatEventWrapper(tArgs->StatEvent);
	std::string eventString = statEvent.GetEventName();
	cvarManager->log(eventString);
   // https://github.com/sruon/bakelite/blob/efb0eb7be2a13c7d8764a2b88609b57c6cb31e15/source/bakelite.cpp#L300
	if (eventString == "EpicSave") {
		json root;
		root["cmd"] = "epicSave";
		sendSocket(root.dump());
		cvarManager->log("onStatEvent:" + eventString);
	}
}

void Mugi::initSocket() {
	WSADATA wsaData;
	struct sockaddr_in server;
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
	struct sockaddr_in server2;
	sock2 = socket(AF_INET, SOCK_DGRAM, 0);
	server2.sin_family = AF_INET;
	server2.sin_port = htons(12344);
	inet_pton(server2.sin_family, ADDR.c_str(), &server2.sin_addr.s_addr);
	connect(sock2, (struct sockaddr*)&server2, sizeof(server2));
}

bool Mugi::sendSocket(std::string str) {
	bool res = send(sock, str.c_str(), str.length(), 0);
	send(sock2, str.c_str(), str.length(), 0);
	return res;
}

void Mugi::endSocket() {
	closesocket(sock);
	closesocket(sock2);
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
	std::string matchId = sw.GetMatchGUID();
	//+4はマジックナンバー　試合開始からカウント開始までの時間
	if (sw.GetbOverTime())overtimeOffset = int(sw.GetSecondsElapsed()) + 4;
	json root,j;
	root["cmd"] = "matchId";
	j["matchId"] = matchId;
	root["data"] = j;
	sendSocket(root.dump());
	
	//only run first
	cvarManager->log("getTOtal:" + TOS(sw.GetTotalScore()));
	if (sw.GetTotalScore() == 0) {
		//resetSetPoint(sw);
	}
	root["cmd"] = "start";
	root["data"] = 0;
	if (sw.GetTotalScore() == 0)sendSocket(root.dump());
}



void Mugi::endGame(std::string eventName) {
	isBoostWatching = false;

	ServerWrapper sw = gameWrapper->GetOnlineGame();
	if (sw.IsNull())return;
	//setPoint
	//calcSetPoint(sw);
	//0にしとかないと次のMatchのtimeがバグる
	overtimeOffset = 0;
	//勝利mv流すため，この順番
	json root;
	root["cmd"] = "end";
	sendSocket(root.dump());
	std::vector<json> Stats;
	ArrayWrapper<PriWrapper> pls = sw.GetPRIs();
	cvarManager->log(TOS(sw.GetMatchWinner().GetScore()));
	int debugIndex = 0;
	for (int i = 0; i < pls.Count(); i++) {
		json j;

		auto pl = pls.Get(i);
		if (pl.IsNull())continue;
		if (pl.GetTeamNum() == 255)continue;

		if(!isDebug)j["id"] = split("Player_" + pl.GetUniqueIdWrapper().GetIdString());
		else j["id"] = TOS(botIndex[debugIndex]);
		j["teams"] = (pl.GetTeamNum());
		j["scores"] = (pl.GetMatchScore());
		j["goals"] = (pl.GetMatchGoals());
		j["assists"] = (pl.GetMatchAssists());
		j["saves"] = (pl.GetMatchSaves());
		j["shots"] = (pl.GetMatchShots());
		j["demos"] = (pl.GetMatchDemolishes());
		j["ballTouches"] = (pl.GetBallTouches());
		Stats.push_back(j);
		debugIndex++;
	}
	//orange H->L -> blue H->L
	std::sort(Stats.begin(), Stats.end(), [](const json& a, const json& b) {return(a["scores"] > b["scores"]); });
	std::sort(Stats.begin(), Stats.end(), [](const json& a, const json& b) {return(a["teams"] < b["teams"]); });

	root["cmd"] = "stats";
	root["data"] = Stats;
	sendSocket(root.dump());
}

void Mugi::onGoal(ActorWrapper caller) {
	ReplayDirectorWrapper RDW(caller.memory_address);
	ReplayScoreData ScoreData = RDW.GetReplayScoreData();
	if (ScoreData.ScoredBy == 0)return;
	PriWrapper pl(ScoreData.ScoredBy);
	PriWrapper assistPl(ScoreData.AssistedBy);
	if (pl.IsNull())return;
	int team = ScoreData.ScoreTeam;
	std::string scorerId = pl.GetbBot() ? botId2Id[pl.GetOldName().ToString()] : split(pl.GetUniqueIdWrapper().GetIdString());
	std::string assisterId = "";
	if (!assistPl.IsNull()) {
		assisterId = assistPl.GetbBot() ? botId2Id[assistPl.GetOldName().ToString()] : split(assistPl.GetUniqueIdWrapper().GetIdString());
	}
	ServerWrapper sw = gameWrapper->GetOnlineGame();
	cvarManager->log("0score" + TOS(sw.GetTeams().Get(0).GetScore()));
	json j;
	j["team"] = team == 0 ? "blue" : "orange";
	j["scoreId"] = scorerId;
	j["assistId"] = assisterId;
	json root;
	root["cmd"] = "goals";
	root["data"] = j;
	sendSocket(root.dump());
	cvarManager->log(ScoreData.ScoreTeam == 0 ? "blue" : "orange");
}



void Mugi::createNameTable(bool isForcedRun)
{
	ServerWrapper sw = gameWrapper->GetOnlineGame();
	if (sw.IsNull())return;
	ArrayWrapper<PriWrapper> pls = sw.GetPRIs();
	ArrayWrapper<CarWrapper> cars = sw.GetCars();
	ArrayWrapper<ControllerWrapper> players = sw.GetPlayers();
	
	//only run first or onload
	if (!isForcedRun && sw.GetTotalScore() != 0)return;
	json _j; _j["cmd"] = "start";
	if (isForcedRun)sendSocket(_j.dump());
	cvarManager->log("PLS:" + TOS(pls.Count()));

	//-----clear------//
	PlayerMap.clear();
	OwnerMap.clear();
	DisplayName2Id.clear();
	UniqueID2DisplayName.clear();
	OwnerIndexMap.clear();
	//----------------//
	int botBlueNum = 0;
	int botOrangeNum = 0;
	int botCount = 0;
	std::vector<std::string> DisplayNames;
	
	for (int i = 0; i < pls.Count(); i++) {
		cvarManager->log("---------");
		auto pl = pls.Get(i);
		if (pl.IsNull())continue;
		std::string displayName = "";
		//本来はuniqueID
		int botId = 0;
		if (pl.GetbBot()) {
			if (pl.GetTeamNum() == 0) {//blue
				botId = botBlueNum;
				botBlueNum++;
			}
			else if (pl.GetTeamNum() == 1) {//orange
				botId = botOrangeNum + 3;
				botOrangeNum++;
			}
			botIndex[botCount] = botId;
			botId2Id[pl.GetOldName().ToString()] = botId;
			botCount++;
			cvarManager->log("botIndex:" + TOS(botId));
		}
		std::string playerId = TOS(botId);

		// if human
		if (!pl.GetbBot())playerId = split("Player_" + pl.GetUniqueIdWrapper().GetIdString());


		//観戦時のプレイヤー名に合わせるため
		if (pl.GetbBot())displayName = "Player_Bot_" + pl.GetOldName().ToString();
		else			 displayName = pl.GetPlayerName().ToString();
		cvarManager->log(displayName);
		
		auto ppl = std::make_shared<PriWrapper>(pl);

		if (pl.GetbBot())PlayerMap[displayName] = ppl;
		else            PlayerMap[playerId] = ppl;

		DisplayName2Id[displayName] = playerId;
		Id2DisplayName[playerId] = displayName;
		cvarManager->log(":=" + playerId+":"+TOS(pl.GetTeamNum()));
		json root;
		root["cmd"] = "dbg";
		root["data"] = playerId;
		sendSocket(root.dump());
		if (pl.GetTeamNum() != 255) {//not �ϐ��
			playerData p = { displayName,playerId ,pl.GetTeamNum() };//isblue
			
			OwnerMap.push_back(p);
		}

	}

	//�`�[����sort
	//koujun ni henkou
	std::sort(OwnerMap.begin(), OwnerMap.end(), [](const playerData& a, const playerData& b) {return(a.team < b.team); });
	std::vector<std::string> PlayerIndexs;
	for (int i = 0; i < OwnerMap.size(); i++) {
		auto p = OwnerMap[i];
		if (isDebug) {
			OwnerIndexMap[p.name] = i;
			OwnerTeamMap[p.name] = p.team == 1 ? "orange" : "blue";
		}
		else {
			OwnerIndexMap[p.id] = i;
			OwnerTeamMap[p.id] = p.team == 1 ? "orange" : "blue";
		}
		cvarManager->log("name!"+p.name);
		//後々p.nameからidに代わる予定
		if (isDebug) PlayerIndexs.push_back(TOS(i));
		else PlayerIndexs.push_back(p.id);
		DisplayNames.push_back(p.name);
	}
	json root;
	root["cmd"] = "displayNames";
	root["data"] = DisplayNames;
	sendSocket(root.dump());
	//json root;
	root["cmd"] = "playerTable";
	root["data"] = PlayerIndexs;
	sendSocket(root.dump());
}

void Mugi::updateTime(std::string eventName)
{
	ServerWrapper sw = gameWrapper->GetOnlineGame();
	if (sw.IsNull())return;
	json root,j;
	root["cmd"] = "time";
	//overtimeOffsetの値setのタイミングと合わせるため，GetbOvertime()から変更
	int time = overtimeOffset == 0 ? sw.GetSecondsRemaining() : int(sw.GetSecondsElapsed()) - overtimeOffset ;
	j["time"] = time;
	j["isOvertime"] = sw.GetbOverTime();
	root["data"] = j;
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
			j["index"] = OwnerIndexMap[playerId];
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
	json subScore;
	subScore["goals"] = pl->GetMatchGoals();
	subScore["shots"] = pl->GetMatchShots();
	subScore["assists"] = pl->GetMatchAssists();
	subScore["saves"] = pl->GetMatchSaves();
	if (preSubScore != subScore.dump()) {
		json root;
		root["cmd"] = "subScore";
		root["data"] = subScore;
		sendSocket(root.dump());
	}
	if (currentFocusActorScore != preFocusActorScore) {
		json root;
		json j;
		j["score"] = currentFocusActorScore;
		root["cmd"] = "score";
		root["data"] = j;
		sendSocket(root.dump());
	}
	preFocusActorScore = currentFocusActorScore;
	preSubScore = subScore.dump();
}

void Mugi::tickPlayer(std::string actorName) {
	if (actorName != preActorName) {
		if (actorName == "") {//if flying
			//sendSocket("f0");
		}
		//空白でもcountに引っかからないはず...
		//send FOCUS
		if (OwnerIndexMap.count(actorName) != 0) {
			//cvarManager->log(actorName);
			json root;
			json j;
			root["cmd"] = "player";
			j["playerIndex"] = TOS(OwnerIndexMap[actorName]);
			j["team"] = OwnerTeamMap[actorName];
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
	if (!isDebug)actorName = split(camera.GetFocusActor());
	//----------tick------------//
	tickPlayer(actorName);
	tickBoost(gw);
	tickScore(actorName);
}


std::string Mugi::split(const std::string& s) {
	std::vector<std::string> elems;
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, '|')) {
		if (!item.empty()) {
			elems.push_back(item);
		}
	}
	if (elems.size() != 3)return "";
	return elems[1];
}