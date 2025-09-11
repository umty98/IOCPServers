#pragma once
//#include "Protocol.h"
//#include <list>
//#include <unordered_set>
#include "Content.h"
constexpr int dfSECTOR_MAX_X = 50;
constexpr int dfSECTOR_MAX_Y = 50;
constexpr int dfSECTOR_SIZE_X = dfRANGE_MOVE_RIGHT / dfSECTOR_MAX_X; // 섹터의 가로 크기
constexpr int dfSECTOR_SIZE_Y = dfRANGE_MOVE_BOTTOM / dfSECTOR_MAX_Y;


// 섹터 관리 배열 선언
extern std::list<st_Character*> g_Sector[dfSECTOR_MAX_Y][dfSECTOR_MAX_X];
//extern SRWLOCK g_SectorLock[dfSECTOR_MAX_Y][dfSECTOR_MAX_X];
extern SRWLOCK g_SectorLock[dfSECTOR_MAX_Y][dfSECTOR_MAX_X];


struct st_SECTOR_AROUND
{
    int iCount;
    st_SECTOR_POS Around[9];
};

void GetSectorAround(int iSectorX, int iSectorY, st_SECTOR_AROUND* pSectorAround);

void AddSector(st_SECTOR_POS sectorPos, st_Character* pCharacter);

void RemoveSector(st_SECTOR_POS sectorPos, st_Character* pCharacter);

void InitSectorLock();

void UninitSectorLock();

int CheckTotalSector();

void CheckTotalSectorSize();