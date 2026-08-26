#include "stdafx.h"
#include "Network.h"
#include "CMonster.h"
#include "CBossMonster.h"
#include "GameFramework.h"
#include "SoundManager.h"
#include <iostream>

// 클라이언트 부분 (not server)
CScene* g_pScene = nullptr;
ID3D12Device* g_pd3dDevice = nullptr;
ID3D12GraphicsCommandList* g_pd3dCommandList = nullptr;
ID3D12RootSignature* g_pd3dGraphicsRootSignature = nullptr;
void* g_pContext = nullptr;


extern CGameFramework gGameFramework;
CGameObject object;

// 전역 변수 정의
SOCKET ConnectSocket = INVALID_SOCKET;
std::atomic<bool> g_running{ true };
//std::string user_name;
long long g_myid = 0;

std::queue<std::vector<char>> g_sendQueue;
std::mutex g_sendMutex;
std::condition_variable g_sendCV;

WSADATA wsaData;

// 객체 관리 맵
std::unordered_map<long long, OtherPlayer*> g_other_players;
std::mutex g_player_mutex;
std::unordered_map<long long, Item*> g_items;
std::mutex g_item_mutex;


std::mutex                          g_pendingMonsterMutex;
std::vector<PendingMonsterSpawn>    g_pendingMonsterSpawns;
std::mutex                          g_pendingBossMutex;
std::vector<PendingBossSpawn>       g_pendingBossSpawns;
std::vector<PendingBossDeath>       g_pendingBossDeaths;
std::mutex                          g_pendingMissionMutex;
std::vector<PendingMissionText>     g_pendingMissionTexts;

// =================================================================
//           otherplayer player 렌더링을 위한 other player 오브젝트 및 관리
// =================================================================
int g_knightIndex = 0; // 0~1
int g_wizardIndex = 2; // 2~3
int g_thiefIndex = 4; // 4~5
std::unordered_map<long long, int> g_other_player_slots;
struct PendingEnterInfo {
    long long player_id;
    uint8_t   job;
};
std::vector<PendingEnterInfo> g_pendingEnters;
std::mutex g_pendingEnterMutex;
void ProcessEnterPacket(long long player_id, uint8_t job)
{
    CScene* scene = gGameFramework.GetCurrentScene();
    if (!scene || !scene->m_ppOtherPlayers) return;
    if (g_other_players.find(player_id) != g_other_players.end()) return;

    int slot = -1;
    switch (job)
    {
    case 0: if (g_knightIndex < 2) slot = g_knightIndex++; break;
    case 1: if (g_wizardIndex < 4) slot = g_wizardIndex++; break;
    case 2: if (g_thiefIndex < 6) slot = g_thiefIndex++;  break;
    }
    if (slot == -1) return;

    OtherPlayer* target = scene->m_ppOtherPlayers[slot];
    if (!target) return;

    target->isConnedted = true;
    g_other_players[player_id] = target;
    g_other_player_slots[player_id] = slot;

    std::cout << "[ENTER] success: id=" << player_id << " slot=" << slot << "\n";
}
// =================================================================
//           몬스터 렌더링을 위한 몬스터 오브젝트 및 관리 
// =================================================================

// 아래의 저장소는 몬스터(CMonster) 고유ID를 저장
std::unordered_map<long long, CMonster*> g_monsters;
std::mutex g_monster_mutex;


void send_hit_damage(long long monsterID, int damage) // 이 함수를 플레이어가 공격하는 곳에 넣으면 된다. 일단 한번 해보고 안되면 수정ㄱㄱ
{
    cs_packet_hit_damage pkt{};
    pkt.size = sizeof(pkt);
    pkt.type = CS_P_HIT_DAMAGE;
    pkt.monsterID = monsterID;
    pkt.damage = damage;
    send_packet(&pkt);

    std::cout << "[HIT] monster ID=" << monsterID << " damage=" << damage << " sent\n";
}


// =================================================================
//                           스킬 관리
// =================================================================

void send_skill_upgrade(SkillSlot slot)
{
    cs_packet_skill_upgrade pkt{};
    pkt.size = sizeof(pkt);
    pkt.type = CS_P_SKILL_UPGRADE;
    pkt.slot = slot;
    send_packet(&pkt);
}

// 기사
void send_shield_block_packet(bool isBlocking)
{
    cs_packet_shield_block pkt{};
    pkt.size = sizeof(pkt);
    pkt.type = CS_P_SHIELD_BLOCK;
    pkt.isBlocking = isBlocking;
    send_packet(&pkt);

    std::cout << "[SHIELD] 방패막기 전송 | isBlocking=" << isBlocking << "\n";
}

void send_strike_packet(const XMFLOAT3& position, const XMFLOAT3& look)
{
    cs_packet_skill_strike pkt{};
    pkt.size = sizeof(pkt);
    pkt.type = CS_P_SKILL_STRIKE;
    pkt.position = position;
    pkt.look = look;
    send_packet(&pkt);

    std::cout << "[STRIKE] 강타 전송 | pos=(" << position.x << ","
        << position.y << "," << position.z << ")\n";
}

void send_taunt_packet(float range)
{
    cs_packet_taunt pkt{};
    pkt.size = sizeof(pkt);
    pkt.type = CS_P_TAUNT;
    pkt.range = range;
    send_packet(&pkt);

    std::cout << "[TAUNT] 도발 전송 | range=" << range << "\n";
}

// 법사
void send_skill_packet(const XMFLOAT3& position, const XMFLOAT3& look)
{
    cs_packet_skill pkt{};
    pkt.size = sizeof(pkt);
    pkt.type = CS_P_SKILL;
    pkt.position = position;
    pkt.look = look;
    send_packet(&pkt);

    std::cout << "[FIREBALL] cs_packet_skill 전송 | size=" << (int)pkt.size << "\n";
}

void send_buff_atk_packet()
{
    cs_packet_buff_atk pkt{};
    pkt.size = sizeof(pkt);
    pkt.type = CS_P_BUFF_ATK;
    send_packet(&pkt);

    std::cout << "[BUFF_ATK] 공격력 버프 전송\n";
}

void send_buff_hp_packet()
{
    cs_packet_buff_hp pkt{};
    pkt.size = sizeof(pkt);
    pkt.type = CS_P_BUFF_HP;
    send_packet(&pkt);

    std::cout << "[BUFF_HP] 체력 버프 전송\n";
}

// 도적 
void send_weapon_pos_packet(const XMFLOAT3& weaponPosition, const XMFLOAT3& weaponRotation)
{
    cs_packet_weapon_pos pkt{};
    pkt.size = sizeof(pkt);
    pkt.type = CS_P_WEAPON_POS;
    pkt.weaponPosition = weaponPosition;
    pkt.weaponRotation = weaponRotation;
    send_packet(&pkt);

    std::cout << "[WEAPON_POS] 도끼 위치 전송 | pos=(" << weaponPosition.x << ","
        << weaponPosition.y << "," << weaponPosition.z << ")\n";
}

// =================================================================
//                          NPC 관리
// =================================================================

void send_npc_interact_packet(long long npcID)
{
    cs_packet_npc_interact pkt{};
    pkt.size = sizeof(pkt);
    pkt.type = CS_P_NPC_INTERACT;
    pkt.npcID = npcID;
    send_packet(&pkt);

    cout << "[NPC] 상호작용 패킷 전송 | npcID=" << npcID << "\n";
}

// =================================================================
//                          파티클 관리
// =================================================================


// =================================================================
//                      네트워크 코어 로직
// =================================================================

void SendThread() {
    while (g_running) {
        std::vector<char> packet;
        {
            std::unique_lock<std::mutex> lock(g_sendMutex);
            g_sendCV.wait(lock, [] { return !g_sendQueue.empty() || !g_running; });
            if (!g_running) break;
            packet = std::move(g_sendQueue.front());
            g_sendQueue.pop();
        }

        WSABUF wsaBuf = { static_cast<ULONG>(packet.size()), packet.data() };
        WSAOVERLAPPED overlapped = {};
        overlapped.hEvent = WSACreateEvent();

        DWORD sent = 0;
        int ret = WSASend(ConnectSocket, &wsaBuf, 1, &sent, 0, &overlapped, nullptr);

        if (ret == SOCKET_ERROR) {
            if (WSAGetLastError() == WSA_IO_PENDING) {
                DWORD result = WSAWaitForMultipleEvents(1, &overlapped.hEvent, TRUE, 1000, FALSE);
                if (result == WSA_WAIT_FAILED) {
                    std::cerr << "송신 오류: " << WSAGetLastError() << std::endl;
                    break;
                }
                WSAGetOverlappedResult(ConnectSocket, &overlapped, &sent, TRUE, nullptr);
            }
            else {
                std::cerr << "송신 실패: " << WSAGetLastError() << std::endl;
            }
        }
        WSACloseEvent(overlapped.hEvent);
    }
}

void RecvThread() {
    thread_local size_t saved_packet_size = 0;      // 지금까지 packet_buffer에 채워진 바이트 수
    thread_local size_t expected_packet_size = 0;   // 현재 조립 중인 패킷의 전체 크기(헤더에서 읽은 값). 0이면 "새 패킷 대기 중"
    thread_local char packet_buffer[BUF_SIZE];

    while (g_running) {
        char buffer[MAX_PACKET_SIZE];
        WSABUF wsaBuf = { MAX_PACKET_SIZE, buffer };
        DWORD flags = 0, recvBytes = 0;
        WSAOVERLAPPED overlapped = {};
        overlapped.hEvent = WSACreateEvent();

        int ret = WSARecv(ConnectSocket, &wsaBuf, 1, &recvBytes, &flags, &overlapped, nullptr);

        if (ret == SOCKET_ERROR) {
            if (WSAGetLastError() == WSA_IO_PENDING) {
                DWORD wait = WSAWaitForMultipleEvents(1, &overlapped.hEvent, TRUE, INFINITE, FALSE);
                if (wait == WSA_WAIT_FAILED || !WSAGetOverlappedResult(ConnectSocket, &overlapped, &recvBytes, FALSE, &flags)) {
                    WSACloseEvent(overlapped.hEvent);
                    break;
                }
            }
            else {
                WSACloseEvent(overlapped.hEvent);
                break;
            }
        }

        WSACloseEvent(overlapped.hEvent);

        if (recvBytes == 0) {
            std::cout << "서버 연결 종료" << std::endl;
            g_running = false;
            break;
        }

        // 패킷 처리
        char* ptr = buffer;
        size_t remaining = recvBytes;
        while (remaining > 0) {
            // 새 패킷의 시작일 때만 헤더(size 바이트)를 읽는다.
            // 이전 recv에서 이어지는 중간 데이터라면 절대 다시 읽으면 안 됨.
            if (saved_packet_size == 0) {
                expected_packet_size = static_cast<unsigned char>(ptr[0]);
                if (expected_packet_size == 0) break;

                if (expected_packet_size > sizeof(packet_buffer)) {
                    // 비정상 패킷 크기 - 스트림이 깨졌다고 보고 이번 recv 버퍼는 폐기
                    std::cerr << "[Network] 비정상 패킷 크기 감지: " << expected_packet_size << std::endl;
                    expected_packet_size = 0;
                    saved_packet_size = 0;
                    break;
                }
            }

            size_t need = expected_packet_size - saved_packet_size;

            if (remaining >= need) {
                memcpy(packet_buffer + saved_packet_size, ptr, need);
                ProcessPacket(packet_buffer);
                ptr += need;
                remaining -= need;
                saved_packet_size = 0;
                expected_packet_size = 0;
            }
            else {
                memcpy(packet_buffer + saved_packet_size, ptr, remaining);
                saved_packet_size += remaining;
                remaining = 0;
            }
        }
    }
}

// =================================================================
//                      유틸리티 함수
// =================================================================

void send_packet(void* packet) {
    unsigned char* p = static_cast<unsigned char*>(packet);
    size_t packet_size = p[0];

    std::vector<char> buf(p, p + packet_size);
    {
        std::lock_guard<std::mutex> lock(g_sendMutex);
        g_sendQueue.push(std::move(buf));
    }
    g_sendCV.notify_one();
}

void InitializeNetwork(char serverIP[]) {

    WSAStartup(MAKEWORD(2, 2), &wsaData);

    /*   char serverIP[16];
       std::cout << "server IP : ";
       std::cin >> serverIP;*/


    ConnectSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);

    // 비동기 연결 설정
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, serverIP, &serverAddr.sin_addr);  //serverIP.c_str()


    WSAOVERLAPPED connectOverlapped{};
    connectOverlapped.hEvent = WSACreateEvent();


    if (connect(ConnectSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Connect Fail: " << WSAGetLastError() << std::endl;
        closesocket(ConnectSocket);
        WSACleanup();
        exit(1);
    }

    WSACloseEvent(connectOverlapped.hEvent);


    std::cout << "Sever Connect" << std::endl;



    //cs_packet_login p{};
    //p.size = sizeof(p);
    //p.type = CS_P_LOGIN;
    //strcpy_s(p.name, sizeof(p.name), user_name.c_str());
    ////p.job = gGameFramework.GetSelectedJob();
    //send_packet(&p);

    //std::cout << "[Client] Login Packet Send : Name=" << p.name << std::endl;

    std::thread(RecvThread).detach();
    std::thread(SendThread).detach();
}

void ProcessPacket(char* ptr)
{

    const unsigned char packet_type = ptr[1];

    //std::cout << "[Client] Packet - Type : " << (int)packet_type << std::endl;

    switch (packet_type)
    {
        // 서버 : 인벤토리 관련된거 만들게 되면 여기에도 정보 추가 해야함
    case SC_P_USER_INFO: // 클라이언트의 정보를 가지고 있는 패킷 타입
    {
        sc_packet_user_info* packet = reinterpret_cast<sc_packet_user_info*>(ptr);
        g_myid = packet->id;
        gGameFramework.UpdatePlayerHP(packet->hp);

        //여기에 리스폰 관련해서 랜더링 해야할듯?? (이부분)
        std::cout << "[SPAWN][CLIENT][RECV] job=" << static_cast<int>(packet->job)
            << " pos=(" << packet->position.x << "," << packet->position.y << "," << packet->position.z << ")\n";
        gGameFramework.UpdateMyPlayerPosition(packet->position, packet->job);
        if (packet->hp <= 0)
        {
            CScene* scene = gGameFramework.GetCurrentScene();
            if (scene && scene->m_pBoss)
                scene->m_pBoss->SetHpbarVisible(false);
        }
        cout << "myid: " << packet->id << endl;

        break;
    }

    case SC_P_ENTER: // 새로 들어온 플레이어의 정보를 포함하고 있는 패킷 타입
    {
        sc_packet_enter* packet = reinterpret_cast<sc_packet_enter*>(ptr);
        long long player_id = packet->id;
        if (player_id == g_myid) break;

        // 로딩 중이면 버리지 말고 보관
        if (gGameFramework.isLoading || gGameFramework.isStartScene) {
            std::lock_guard<std::mutex> lock(g_pendingEnterMutex);
            g_pendingEnters.push_back({ player_id, packet->job });
            cout << "[ENTER] loadinging: id=" << player_id << "\n";
            break;
        }
        ProcessEnterPacket(player_id, packet->job);

        std::cout << "[Client] New Player " << player_id << "Connect " << "\n";

        break;
    }

    case SC_P_MOVE: // 상대 플레이어 (움직이면) 좌표 받기
    {
        sc_packet_move* packet = reinterpret_cast<sc_packet_move*>(ptr);
        long long other_id = packet->id;

        if (other_id == g_myid) break;

        // OtherPlayer의 위치를 반영한다
        auto it = g_other_player_slots.find(other_id);
        if (it == g_other_player_slots.end()) break;


        int slot = it->second;

        gGameFramework.UpdateOtherPlayerPosition(slot, packet->position);
        gGameFramework.UpdateOtherPlayerLook(slot, packet->look, packet->right);
        gGameFramework.UpdateOtherPlayerAnimation(slot, packet->animState);
        gGameFramework.UpdateOtherPlayerRotate(slot, packet->right, packet->look);

        /*        std::cout << "[Client] New Player Information Recv " << "PlayerNo : " << packet->id << ", "
                    << " Position(" << packet->position.x << "," << packet->position.y << "," << packet->position.z << ")"
                    << " Look(" << packet->look.x << "," << packet->look.y << "," << packet->look.z << ")"
                    << " Right(" << packet->right.x << "," << packet->right.y << "," << packet->right.z << ")"
                    << "Animation : " << static_cast<int>(packet->animState)
                    << std::endl;*/

        break;
    }

    case SC_P_LEAVE: // 서버가 클라에게 다른 플레이어가 게임을 떠났음을 알려주는 패킷 타입
    {
        sc_packet_leave* packet = reinterpret_cast<sc_packet_leave*>(ptr);
        int other_id = packet->id;

        std::cout << "[Client] Player Remove: ID=" << other_id << std::endl;

        auto slotIt = g_other_player_slots.find(other_id);
        if (slotIt != g_other_player_slots.end())
        {
            int slot = slotIt->second;

            CScene* scene = gGameFramework.GetCurrentScene();
            if (scene && scene->m_ppOtherPlayers)
            {
                OtherPlayer* target = scene->m_ppOtherPlayers[slot];
                if (target)
                    target->isConnedted = false;
            }

            // 슬롯 인덱스 반환 (job 별로 slot 범위를 역추산)
            if (slot < 2) { g_knightIndex = min(g_knightIndex, slot); }
            else if (slot < 4) { g_wizardIndex = min(g_wizardIndex, slot); }
            else { g_thiefIndex = min(g_thiefIndex, slot); }

            g_other_player_slots.erase(slotIt);
            g_other_players.erase(other_id);
        }

        break;
    }

    case SC_P_RESPAWN:
    {
        sc_packet_respawn* packet = reinterpret_cast<sc_packet_respawn*>(ptr);

        cout << "[수신] SC_P_RESPAWN | playerID=" << packet->playerID << " HP=" << packet->hp
            << " pos=(" << packet->position.x << "," << packet->position.y << "," << packet->position.z << ")\n";

        // 랜더링 만 하면 될듯
        break;
    }

    case SC_P_SKILL_UPGRADE:
    {
        sc_packet_skill_upgrade* packet = reinterpret_cast<sc_packet_skill_upgrade*>(ptr);

        // 쿨타임 감소는 클라 스킬 쿨타임 변수에 직접 적용
        cout << "[강화완료] slot=" << (int)packet->slot << " newValue=" << packet->newValue << "\n";

        break;
    }

    case SC_P_SKILL:
    {
        sc_packet_skill* packet = reinterpret_cast<sc_packet_skill*>(ptr);

        std::cout << "[SKILL] SC_P_SKILL 수신 | playerID=" << packet->playerID << " pos=(" << packet->position.x << ", " << packet->position.y << ", " << packet->position.z << ")\n";


        if (packet->playerID == g_myid) {
            std::cout << "[SKILL] 내 패킷 루프백 → 무시\n";
            break;
        }

        CScene* scene = gGameFramework.GetCurrentScene();
        if (!scene) break;
        scene->m_pFireballSystem->Emit(packet->position, packet->look);


    }

    case SC_P_SHIELD_BLOCK: // 방패막기
    {
        sc_packet_shield_block* packet = reinterpret_cast<sc_packet_shield_block*>(ptr);

        std::cout << "[수신] SC_P_SHIELD_BLOCK | playerID=" << packet->playerID << " isBlocking=" << (int)packet->isBlocking << "\n";
        break;
    }

    case SC_P_SKILL_STRIKE: // 강타
    {
        sc_packet_skill_strike* packet = reinterpret_cast<sc_packet_skill_strike*>(ptr);

        if (packet->playerID == g_myid) break;

        CScene* scene = gGameFramework.GetCurrentScene();
        if (!scene) break;

        if (scene->m_pGroundCrackEffect)
        {
            scene->m_pGroundCrackEffect->Trigger(packet->position, packet->look);
        }

        std::cout << "[수신] SC_P_SKILL_STRIKE | playerID=" << packet->playerID
            << " pos=(" << packet->position.x << "," << packet->position.y << "," << packet->position.z << ")\n";
        break;
    }

    case SC_P_TAUNT: // 도발
    {
        sc_packet_taunt* packet = reinterpret_cast<sc_packet_taunt*>(ptr);

        std::cout << "[수신] SC_P_TAUNT | playerID=" << packet->playerID << "\n";
        break;
    }

    case SC_P_BUFF_ATK: // 공격력 빔
    {
        sc_packet_buff_atk* packet = reinterpret_cast<sc_packet_buff_atk*>(ptr);
        //if (packet->playerID == g_myid) break;

        CScene* scene = gGameFramework.GetCurrentScene();
        if (!scene) break;

        XMFLOAT3 casterPos;
        XMFLOAT3 targetPos;

        // 시전자 위치
        if (packet->playerID == g_myid)
        {
            casterPos = scene->m_pPlayer->GetPosition();
        }
        else
        {
            auto casterIt = g_other_player_slots.find(packet->playerID);
            if (casterIt == g_other_player_slots.end()) break;

            OtherPlayer* caster = scene->m_ppOtherPlayers[casterIt->second];
            if (!caster) break;

            casterPos = caster->GetPosition();
        }

        // 대상 위치
        if (packet->targetID == g_myid)
        {
            targetPos = scene->m_pPlayer->GetPosition();
        }
        else
        {
            auto targetIt = g_other_player_slots.find(packet->targetID);
            if (targetIt == g_other_player_slots.end()) break;

            OtherPlayer* target = scene->m_ppOtherPlayers[targetIt->second];
            if (!target) break;

            targetPos = target->GetPosition();
        }

        scene->m_pBeamSystem->Emit(casterPos, targetPos);
        std::cout << "[SC_P_BUFF_ATK 버프] Emit: casterPos=(" << casterPos.x << "," << casterPos.y << "," << casterPos.z
            << ") targetPos=(" << targetPos.x << "," << targetPos.y << "," << targetPos.z << ")\n";


        if (scene && scene->m_pPlayer)
        {
            if (packet->targetID == g_myid)
            {
                scene->m_pPlayer->damage = packet->newDamage;
                scene->m_pPlayer->m_bIsAtkBuffed = (packet->newDamage > 10);
                cout << "[BUFF_ATK] 내 공격력 갱신: " << packet->newDamage << "\n";
            }
        }

        std::cout << "[SC_P_BUFF_ATK 버프] | playerID=" << packet->playerID << " newDamage=" << packet->newDamage << "\n";
        break;
    }

    case SC_P_BUFF_HP: // 체력회복
    {
        sc_packet_buff_hp* packet = reinterpret_cast<sc_packet_buff_hp*>(ptr);

        CScene* scene = gGameFramework.GetCurrentScene();
        if (!scene) break;

        if (scene->m_pPlayer)
        {
            scene->m_pGreenSpiritSystem->Emit(scene->m_pPlayer->GetPosition());
        }

        // 접속 중인 모든 OtherPlayer
        for (auto& kv : g_other_player_slots)
        {
            int slot = kv.second;
            OtherPlayer* otherPlayer = scene->m_ppOtherPlayers[slot];

            if (!otherPlayer || !otherPlayer->isConnedted) continue;
            scene->m_pGreenSpiritSystem->Emit(otherPlayer->GetPosition());
            gGameFramework.UpdateOtherPlayerHP(slot, packet->newHp); // HP 갱신
        }

        // 내 hp 갱신 + other player hp 갱신 필요
        if (packet->playerID == g_myid)
        {
            gGameFramework.UpdatePlayerHP(packet->newHp);
        }

        std::cout << "[수신] SC_P_BUFF_HP | playerID=" << packet->playerID << " newHp=" << packet->newHp << "\n";
        break;
    }

    case SC_P_WEAPON_POS: // 도끼 무기 위치 (도적)
    {
        sc_packet_weapon_pos* packet = reinterpret_cast<sc_packet_weapon_pos*>(ptr);

        CScene* scene = gGameFramework.GetCurrentScene();
        if (!scene || !scene->m_pWeaponThrowSystem) break;

        scene->m_pWeaponThrowSystem->Emit(packet->weaponPosition, packet->weaponRotation, 30.0f);

        break;
    }

    case SC_P_MONSTER_SPAWN:
    {
        sc_packet_monster_spawn* packet = reinterpret_cast<sc_packet_monster_spawn*>(ptr);

        // 중요: OnMonsterSpawned()는 새 몬스터일 때 D3D12 디바이스/커맨드리스트 작업
        // (m_pd3dCommandList->Reset/Close, ExecuteCommandLists 등)을 수행하고,
        // g_monsters 맵에도 직접 write한다. 이 커맨드리스트/커맨드큐/g_monsters는
        // 메인 스레드(렌더 루프, AnimateObjects)도 매 프레임 그대로 사용/순회하는 것과
        // "동일한 객체"라서, 여기(네트워크 수신 스레드)에서 바로 호출하면
        // 두 스레드가 같은 ID3D12GraphicsCommandList/CommandAllocator를 동시에 건드리게 되어
        // 정의되지 않은 동작(랜덤 힙 손상/크래시)으로 이어진다.
        // 로딩 중이든 아니든 항상 큐에 쌓아두고, 메인 스레드가 매 프레임 드레인하도록 한다.
        {
            std::lock_guard<std::mutex> lock(g_pendingMonsterMutex);
            g_pendingMonsterSpawns.push_back({ (int)packet->monsterID, packet->position, packet->state });
        }

        break;
    }

    case SC_P_MONSTER_MOVE:
    {
        sc_packet_monster_move* packet = reinterpret_cast<sc_packet_monster_move*>(ptr);

        gGameFramework.UpdateMonsterPosition(packet->monsterID, packet->position, packet->rotation, packet->state);

        break;
    }

    case SC_P_UPDATE_MONSTER_HP:
    {
        sc_packet_update_monster_hp* packet = reinterpret_cast<sc_packet_update_monster_hp*>(ptr);

        std::cout << "[몬스터] HP 갱신 수신 | ID=" << packet->monsterID << " HP=" << packet->hp << "\n";

        auto it = g_monsters.find(packet->monsterID);
        if (it != g_monsters.end())
        {
            it->second->SetHP((float)packet->hp);
        }

        break;
    }

    case SC_P_MONSTER_DIE:
    {
        sc_packet_monster_die* packet = reinterpret_cast<sc_packet_monster_die*>(ptr);

        std::cout << "[몬스터] SC_P_MONSTER_DIE 수신 | ID=" << packet->monsterID << " 처치자=" << packet->killerID << "\n";

        int randomIndex = (rand() % 2) + 1;
        string sfxName = "monster_die_" + to_string(randomIndex);
        CSoundManager::GetInstance()->PlaySFX(sfxName);

        auto it = g_monsters.find(packet->monsterID);
        if (it != g_monsters.end())
        {
            CMonster* pMonster = it->second;
            for (int i = 0; i < 5; ++i)
                pMonster->m_pSkinnedAnimationController->SetTrackEnable(i, false);
            pMonster->m_pSkinnedAnimationController->SetTrackPosition(4, 0.0f);
            pMonster->m_pSkinnedAnimationController->SetTrackEnable(4, true);
        }

        break;
    }

    case SC_P_GOLD_REWARD:
    {
        sc_packet_gold_reward* packet = reinterpret_cast<sc_packet_gold_reward*>(ptr);

        gGameFramework.UpdatePlayerGold(packet->totalGold);

        std::cout << "[골드] SC_P_GOLD_REWARD 수신 | +" << packet->amount << "G (현재=" << packet->totalGold << "G)\n";

        break;

    }

    case SC_P_BOSS_SPAWN:
    {
        sc_packet_boss_spawn* packet = reinterpret_cast<sc_packet_boss_spawn*>(ptr);

        if (gGameFramework.isLoading || gGameFramework.isStartScene) {
            std::lock_guard<std::mutex> lock(g_pendingBossMutex);
            g_pendingBossSpawns.push_back({ packet->bossID, packet->position, packet->hp, packet->maxHp });
            cout << "[BOSS] 로딩중 보관 ID=" << packet->bossID << "\n";
            break;
        }

        gGameFramework.OnBossSpawned(packet->bossID, packet->position, packet->hp, packet->maxHp);
        break;
    }

    case SC_P_BOSS_MOVE:
    {
        sc_packet_boss_move* packet = reinterpret_cast<sc_packet_boss_move*>(ptr);

        CScene* scene = gGameFramework.GetCurrentScene();
        if (!scene || !scene->m_pBoss) break;

		scene->m_pBoss->SetPositionOnTerrain(packet->position);
        scene->m_pBoss->SetLookDirection(packet->look);

        BossState cur = scene->m_pBoss->GetState();

        // 공격/사망 모션 재생 중에는 Move 패킷으로 끼어들지 않음
        bool isBusy = (cur == BossState::Attack01 || cur == BossState::Attack02 ||
            cur == BossState::Taunt || cur == BossState::Death);

        // Death만 보호. Attack/Taunt는 ONCE라서 끝나면 멈춰있을 뿐이므로
        // Move 패킷이 오면 그 시점에 풀어줘도 안전함 (서버가 어차피 패턴 종료 후에만 보냄)
        if (cur != BossState::Death) {
            if (packet->isMoving && cur != BossState::Walk)
                scene->m_pBoss->TransitionTo(BossState::Walk);
            else if (!packet->isMoving && cur != BossState::Idle)
                scene->m_pBoss->TransitionTo(BossState::Idle);
        }
        break;
    }

    case SC_P_BOSS_DEATH:
    {
        sc_packet_boss_death* packet = reinterpret_cast<sc_packet_boss_death*>(ptr);
        {
            std::lock_guard<std::mutex> lock(g_pendingBossMutex);
            g_pendingBossDeaths.push_back({ packet->bossID, packet->killerID });
        }
        cout << "[BOSS] Death queued boss=" << packet->bossID << " killer=" << packet->killerID << "\n";
        break;
    }

    case SC_P_BOSS_HP:
    {
        sc_packet_boss_hp* packet = reinterpret_cast<sc_packet_boss_hp*>(ptr);

        CScene* scene = gGameFramework.GetCurrentScene();
        if (scene && scene->m_pBoss) {
            scene->m_pBoss->SetHP((float)packet->hp);
        }

        //cout << "[BOSS] SC_P_BOSS_HP 수신 HP=" << packet->hp << "/" << packet->maxHp << "\n";

        break;
    }

    case SC_P_BOSS_PATTERN:
    {
        sc_packet_boss_pattern* packet = reinterpret_cast<sc_packet_boss_pattern*>(ptr);
        //cout << "[BOSS] 패턴 수신 패턴=" << (int)packet->patternType << "\n";

        CScene* scene = gGameFramework.GetCurrentScene();
        if (!scene || !scene->m_pBoss) break;

        CCamera* camera = gGameFramework.GetCamera();


        // 애니메이션 전환 + 공격범위 이펙트(모양/색상/웜업 결정)는 모두 CBossMonster가 처리한다.
        // (보스가 어떤 트랙으로 들어가는지에 따라 스스로 이펙트를 스폰함 - PlayAttackPattern 참고)
        switch (packet->patternType) {
        case 0: scene->m_pBoss->PlayAttackPattern(BossState::Attack01, packet->attackCenter, packet->look, packet->attackRange); camera->StartShake(0.5f, 1.0f, 25.0f);  break; // NORMAL
        case 1: scene->m_pBoss->PlayAttackPattern(BossState::Attack02, packet->attackCenter, packet->look, packet->attackRange); camera->StartShake(0.5f, 1.0f, 25.0f); break; // SLAM
        case 2: scene->m_pBoss->PlayAttackPattern(BossState::Taunt, packet->attackCenter, packet->look, packet->attackRange, packet->sweepAngle); camera->StartShake(1.5f, 1.0f, 50.0f); break; // SWEEP
        }


        break;
    }

    case SC_P_NPC_MISSION:
    {
        sc_packet_npc_mission* packet = reinterpret_cast<sc_packet_npc_mission*>(ptr);

        // 방어적 코딩: 혹시라도 description이 널종료가 안 된 상태로 오면
        // MultiByteToWideChar가 packet_buffer 밖을 읽어버릴 수 있으므로 강제 종료 처리
        packet->description[sizeof(packet->description) - 1] = '\0';

        cout << "[NPC] 미션 수신 | missionID=" << packet->missionID << " desc=" << packet->description << " target=" << packet->targetCount << " reward=" << packet->rewardGold << "G\n";

        // description은 서버(NPC.cpp)에서 CP949(EUC-KR)로 작성된 멀티바이트 문자열이므로
        // 해당 코드페이지 기준으로 wstring 변환 (클라이언트가 UTF-8 소스라도 이 값만은 949 기준)
        int wlen = MultiByteToWideChar(949, 0, packet->description, -1, nullptr, 0);
        std::wstring wDesc(wlen > 0 ? wlen - 1 : 0, L'\0');
        if (wlen > 0)
            MultiByteToWideChar(949, 0, packet->description, -1, &wDesc[0], wlen);

        // 여기(네트워크 스레드)에서 바로 scene->ShowMissionText()를 부르면
        // D3D12 리소스 생성 + m_GameObjects 변경이 렌더 스레드의 순회와 겹칠 수 있으므로
        // 큐에만 넣어두고 실제 반영은 CScene::AnimateObjects(메인 스레드)에서 한다.
        {
            std::lock_guard<std::mutex> lock(g_pendingMissionMutex);
            g_pendingMissionTexts.push_back({ PendingMissionUiType::Info, wDesc });
        }

        break;
    }

    case SC_P_MISSION_COMPLETE:
    {
        sc_packet_mission_complete* packet = reinterpret_cast<sc_packet_mission_complete*>(ptr);

        //cout << "[NPC] 미션 완료 | missionID=" << packet->missionID  << " reward=" << packet->rewardGold  << " totalGold=" << packet->totalGold << "\n";

        gGameFramework.UpdatePlayerGold(packet->totalGold);

        // mission.dds 위 텍스트를 완료 문구로 전환 (배경은 계속 보여줌).
        // 이후 NPC 근처에서 F를 다시 누르면 SC_P_NPC_MISSION이 다시 와서 이 텍스트를 새 미션으로 덮어씀.
        // 이것도 마찬가지로 네트워크 스레드에서 바로 호출하지 않고 큐에 적재한다.
        {
            std::lock_guard<std::mutex> lock(g_pendingMissionMutex);
            g_pendingMissionTexts.push_back({ PendingMissionUiType::Complete, L"Mission Complete!" });
        }

        break;
    }

    case SC_P_MISSION_PROGRESS:
    {
        sc_packet_mission_progress* packet = reinterpret_cast<sc_packet_mission_progress*>(ptr);

        std::lock_guard<std::mutex> lock(g_pendingMissionMutex);
        g_pendingMissionTexts.push_back({ PendingMissionUiType::Progress, L"", packet->currentCount, packet->targetCount });
        break;
    }

    default:

        std::cout << "Unknown Packet Type [" << ptr[1] << "]" << std::endl;
    }
}

// process_data() 함수 개선
void process_data(char* net_buf, size_t io_byte) {

    char* ptr = net_buf;
    static size_t in_packet_size = 0;
    static size_t saved_packet_size = 0;
    static char packet_buffer[BUF_SIZE];

    while (0 != io_byte) {
        if (0 == in_packet_size) in_packet_size = ptr[0];
        if (io_byte + saved_packet_size >= in_packet_size) {
            memcpy(packet_buffer + saved_packet_size, ptr, in_packet_size - saved_packet_size);
            ProcessPacket(packet_buffer);
            ptr += in_packet_size - saved_packet_size;
            io_byte -= in_packet_size - saved_packet_size;
            in_packet_size = 0;
            saved_packet_size = 0;
        }
        else {
            memcpy(packet_buffer + saved_packet_size, ptr, io_byte);
            saved_packet_size += io_byte;
            io_byte = 0;
        }
    }
}

void send_position_to_server(const XMFLOAT3& position, const XMFLOAT3& look, const XMFLOAT3& right, const uint8_t& animState)
{

    cs_packet_move p;
    p.size = sizeof(p);
    p.type = CS_P_MOVE;
    p.position = position;
    p.look = look;
    p.right = right;
    p.animState = animState;
    send_packet(&p);

}

void CleanupNetwork() {
    g_running = false;
    closesocket(ConnectSocket);
    WSACleanup();
    g_sendCV.notify_all(); // 송신 스레드 깨우기
}

void LoadingDoneToServer()
{
    cs_packet_loading_done pkt{};
    pkt.size = sizeof(pkt);
    pkt.type = CS_P_LOADING_DONE;
    send_packet(&pkt);
    std::cout << "[Client] LoadingDone send\n";
    // 로딩 중에 못 처리한 ENTER 패킷 재처리
    {
        std::lock_guard<std::mutex> lock(g_pendingEnterMutex);
        for (auto& p : g_pendingEnters) {
            std::cout << "[ENTER] Queue reprocessing: id=" << p.player_id << "\n";
            ProcessEnterPacket(p.player_id, p.job);
        }
        g_pendingEnters.clear();
    }

    {
        std::lock_guard<std::mutex> lock(g_pendingMonsterMutex);
        for (auto& m : g_pendingMonsterSpawns) {
            std::cout << "[MONSTER] Queue reprocessing: id=" << m.monsterID << "\n";
            gGameFramework.OnMonsterSpawned(m.monsterID, m.position, m.state);
        }
        g_pendingMonsterSpawns.clear();
    }

    {
        std::lock_guard<std::mutex> lock(g_pendingBossMutex);
        for (auto& b : g_pendingBossSpawns) {
            cout << "[BOSS] pending 처리 ID=" << b.bossID << "\n";
            gGameFramework.OnBossSpawned(b.bossID, b.position, b.hp, b.maxHp);
        }
        g_pendingBossSpawns.clear();
    }
    std::cout << "[Client] LodingDone ! " << std::endl;
}
