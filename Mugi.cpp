#include "pch.h"
#include "Mugi.h"
#define TOS(i) std::to_string(i)

BAKKESMOD_PLUGIN(Mugi, "Mugi is Moca's friend's cat.", plugin_version, PLUGINTYPE_FREEPLAY)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

void Mugi::onLoad()
{
	createNameTable(true);
	cvarManager->log("init Sock");
	initSocket();
	sendSocket("init");
	//gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.OnGameTimeUpdated", std::bind(&Mugi::updateScore, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.ReplayDirector_TA.Tick", std::bind(&Mugi::tick, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.Camera_Replay_TA.SetFocusActor", std::bind(&Mugi::updateAutoCam, this, std::placeholders::_1));
	gameWrapper->HookEvent("Function TAGame.Camera_TA.OnViewTargetChanged", std::bind(&Mugi::updatePlayerCam, this, std::placeholders::_1));
	//Function GameEvent_Soccar_TA.Active.StartRound
	gameWrapper->HookEvent("Function GameEvent_Soccar_TA.Active.BeginState", std::bind(&Mugi::startGame, this, std::placeholders::_1));
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
			sendSocket("?d:" + TOS(OwnerIndexMap[attackerName]));//demo
		}
		});
	gameWrapper->HookEvent("Function ReplayDirector_TA.PlayingHighlights.PlayRandomHighlight", [this](std::string eventName) {sendSocket("endStats"); });
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
	std::string msg = "scored";
	sendSocket("scored");
}
void Mugi::startGame(std::string eventName) {
	createNameTable(false);
	ServerWrapper sw = gameWrapper->GetOnlineGame();
	if (sw.IsNull())return;
	if (sw.GetTotalScore() == 0)sendSocket("start");
}
void Mugi::endGame(std::string eventName) {
	isBoostWatching = false;
	sendSocket("end");

	ServerWrapper sw = gameWrapper->GetOnlineGame();
	if (sw.IsNull())return;
	MatchResults.clear();
	ArrayWrapper<PriWrapper> pls = sw.GetPRIs();
	for (int i = 0; i < pls.Count(); i++) {
		auto pl = pls.Get(i);
		if (pl.IsNull())continue;
		if (pl.GetTeamNum() == 255)continue;
		resultData res = {
			pl.GetPlayerName().ToString(),
			pl.GetTeamNum(),
			pl.GetMatchScore(),
			pl.GetMatchGoals(),
			pl.GetMatchAssists(),
			pl.GetMatchSaves(),
			pl.GetMatchShots(),
			pl.GetMatchDemolishes(),
			pl.GetBallTouches(),
		};
		MatchResults.push_back(res);
	}
	//orange H->L -> blue H->L
	std::sort(MatchResults.begin(), MatchResults.end(), [](const resultData& a, const resultData& b) {return(a.score > b.score); });
	std::sort(MatchResults.begin(), MatchResults.end(), [](const resultData& a, const resultData& b) {return(a.team > b.team); });
	int i = 0;
	for (const resultData r : MatchResults) {
		cvarManager->log(r.name + ":" + TOS(r.score));
		cvarManager->log(r.name + ":" + TOS(r.goals));
		cvarManager->log(r.name + ":" + TOS(r.assists));
		cvarManager->log(r.name + ":" + TOS(r.saves));
		cvarManager->log(r.name + ":" + TOS(r.shots));
		cvarManager->log(r.name + ":" + TOS(r.demos));
		cvarManager->log(r.name + ":" + TOS(r.touches));
		sendSocket("en:" + r.name + ":" + TOS(i));
		sendSocket("es:" + TOS(r.score) + ":" + TOS(i));
		sendSocket("eg:" + TOS(r.goals) + ":" + TOS(i));
		sendSocket("ea:" + TOS(r.assists) + ":" + TOS(i));
		sendSocket("esv:" + TOS(r.saves) + ":" + TOS(i));
		sendSocket("esh:" + TOS(r.shots) + ":" + TOS(i));
		sendSocket("ed:" + TOS(r.demos) + ":" + TOS(i));
		sendSocket("eb:" + TOS(r.touches) + ":" + TOS(i));
		i++;
	}
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
			playerData p = { displayName, pl.GetTeamNum() };//isblue
			OwnerMap.push_back(p);
		}

	}
	//�`�[����sort
	std::sort(OwnerMap.begin(), OwnerMap.end(), [](const playerData& a, const playerData& b) {return(a.team > b.team); });

	for (int i = 0; i < OwnerMap.size(); i++) {
		auto p = OwnerMap[i];
		OwnerIndexMap[p.name] = i;
		sendSocket("t" + p.name + ":" + TOS(i));
	}
}

void Mugi::tick(std::string eventName) {
	auto gw = gameWrapper->GetOnlineGame();
	if (gw.IsNull())return;
	CameraWrapper camera = gameWrapper->GetCamera();
	auto actorName = camera.GetFocusActor();
	if (actorName != preActorName) {
		//send FOCUS
		if (UniqueID2DisplayName.count(actorName) != 0) {
			sendSocket("p" + actorName + ":" + TOS(OwnerIndexMap[UniqueID2DisplayName[actorName]]));
			preActorName = actorName;
		}
	}

	//----------boost-------------//
	if (isBoostWatching) {
		auto cars = gw.GetCars();
		for (int i = 0; i < cars.Count(); i++) {
			auto car = cars.Get(i);
			//!!!!!!!!!----only noBot--!!!!!!!!//
			std::string name = "Player_" + car.GetOwnerName();
			if (car.IsNull())continue;
			auto boostCom = car.GetBoostComponent();
			if (boostCom.IsNull())continue;
			int boost = int(boostCom.GetCurrentBoostAmount() * 100);
			if (OwnerIndexMap.count(name) == 0)return;
			if (boost != Boosts[i])sendSocket("b" + TOS(boost) + ":" + TOS(OwnerIndexMap[name]));
			Boosts[i] = boost;
		}
	}
	//---------------------------//
	if (PlayerMap.count(actorName) == 0) return;

	auto pl = PlayerMap[actorName];
	currentFocusActorScore = pl->GetMatchScore();
	if (currentFocusActorScore != preFocusActorScore) {
		std::string msg = currentFocusActorName + ":" + std::to_string(currentFocusActorScore);
		sendSocket(PlayerToDisplayName[currentFocusActorName] + ":" + std::to_string(currentFocusActorScore));
	}
	preFocusActorScore = currentFocusActorScore;
	//cvarManager->log(currentFocusActorName);
}
void Mugi::updatePlayerCam(std::string eventName) {
	//cvarManager->log("fire");
	ServerWrapper server = gameWrapper->GetOnlineGame();
	CameraWrapper camera = gameWrapper->GetCamera();
	auto actorName = camera.GetFocusActor();
	auto cameraState = camera.GetCameraState();
	if (cameraState.find("Car") != std::string::npos) {
		currentFocusActorName = actorName;
		//cvarManager->log(cameraState +actorName);
	}
}

void Mugi::updateAutoCam(std::string eventName) {
	ServerWrapper server = gameWrapper->GetOnlineGame();
	CameraWrapper camera = gameWrapper->GetCamera();
	auto actorName = camera.GetFocusActor();
	auto cameraState = camera.GetCameraState();
	if (cameraState.find("Car") == std::string::npos) {
		//Found
		if (actorName == preAutoCamActorName) {
			//cvarManager->log("double");
			//cvarManager->log("new:"+ camera.GetFocusActor());
		}
		currentFocusActorName = actorName;
		//cvarManager->log(cameraState + actorName);
	}
	preAutoCamActorName = actorName;
}
