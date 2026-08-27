#pragma once
#include "Common.h"
#include "Object.h"
//#include "Player.h"
#include "Scene.h"
#include "OtherPlayer.h" 
#include "Object_Items.h"
#include "CMonster.h"


// 클라이언트에서 서버로 가야할것 -----------
// 
// 플레이어의 좌표(x,y,z)
// 플레이어의 id(user_name)
// 
// 
// 오브젝트의 좌표(ob_x,ob_y,ob_z)
// 
// 
//  <필요한것>
//		플레이어의 정보 저장공간
//		오브젝트의 정보 저장공간
// 
// ----------------------------------------

class Item;

//enum IO_OPERATION { IO_RECV, IO_SEND, IO_CONNECT };
//
//struct OverlappedEx {
//    WSAOVERLAPPED overlapped;
//    WSABUF wsaBuf;
//    char buffer[BUF_SIZE];
//    IO_OPERATION operation;
//};

struct PendingMonsterSpawn {
    int      monsterID;
    XMFLOAT3 position;
    int      state;
};

struct PendingBossSpawn {
    long long bossID;
    XMFLOAT3  position;
    int       hp;
    int       maxHp;
};
struct PendingBossDeath {
    long long bossID;
    long long killerID;
};
enum class PendingMissionUiType {
    Info,
    Progress,
    Complete
};

struct PendingMissionText {
    PendingMissionUiType type;
    std::wstring text;
    int currentCount = 0;
    int targetCount = 0;
};

extern std::mutex                           g_pendingMissionMutex;
extern std::vector<PendingMissionText>      g_pendingMissionTexts;

extern std::mutex                           g_pendingMonsterMutex;
extern std::vector<PendingMonsterSpawn>     g_pendingMonsterSpawns;

extern std::mutex                           g_pendingBossMutex;
extern std::vector<PendingBossSpawn>        g_pendingBossSpawns;
extern std::vector<PendingBossDeath>        g_pendingBossDeaths;

// otherplayer
extern std::unordered_map<long long, OtherPlayer*> g_other_players;
extern std::unordered_map<long long, int> g_other_player_slots;

extern int g_knightIndex;
extern int g_wizardIndex;
extern int g_thiefIndex;

extern HANDLE g_hIOCP;
extern SOCKET ConnectSocket;
extern std::unordered_map<long long, OtherPlayer*> g_other_players;
extern long long g_myid;
extern std::string user_name;

extern std::unordered_map<long long, Item*> g_items; // 아이템 ID로 관리
extern std::mutex g_item_mutex;

//extern CScene* g_pScene;
extern ID3D12Device* g_pd3dDevice;
extern ID3D12GraphicsCommandList* g_pd3dCommandList;
extern ID3D12RootSignature* g_pd3dGraphicsRootSignature;
extern void* g_pContext;

extern std::queue<std::vector<char>> g_sendQueue;
extern std::mutex g_sendMutex;
extern std::condition_variable g_sendCV;

extern std::unordered_map<long long, CMonster*> g_monsters;
extern std::mutex g_monster_mutex;


// Monster
void send_hit_damage(long long monsterID, int damage);

void ProcessPacket(char* ptr);
//void process_data(char* net_buf, size_t io_byte); // ???
void send_packet(void* packet);
void send_position_to_server(const XMFLOAT3& position, const XMFLOAT3& look, const XMFLOAT3& right, const uint8_t& animState);
void InitializeNetwork(char serverIP[]);
void CleanupNetwork();

void LoadingDoneToServer();

void send_shield_block_packet(bool isBlocking);
void send_strike_packet(const XMFLOAT3& position, const XMFLOAT3& look);
void send_taunt_packet(float range);
void send_buff_atk_packet();
void send_buff_hp_packet();
void send_toggle_invincible_packet();
void send_weapon_pos_packet(const XMFLOAT3& weaponPosition, const XMFLOAT3& weaponRotation);
void send_skill_packet(const XMFLOAT3& position, const XMFLOAT3& look);
void send_skill_upgrade(SkillSlot slot);
void send_npc_interact_packet(long long npcID); // <- NPC와 상호작용키를 누르는곳에 넣어야함.

