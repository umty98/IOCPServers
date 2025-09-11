//#include "Content.h"
//#include "Sector.h"
//#include <list>
#include "pch.h"
#include "Content.h"

std::list<st_Character*> g_Sector[dfSECTOR_MAX_Y][dfSECTOR_MAX_X];
//SRWLOCK g_SectorLock[dfSECTOR_MAX_Y][dfSECTOR_MAX_X];
SRWLOCK g_SectorLock[dfSECTOR_MAX_Y][dfSECTOR_MAX_X];


void GetSectorAround(int iSectorX, int iSectorY, st_SECTOR_AROUND* pSectorAround)
{
    pSectorAround->iCount = 0;

    //섹터 범위 예외처리해줘야될듯

    for (int dy = -1; dy <= 1; dy++)
    {
        for (int dx = -1; dx <= 1; dx++)
        {
            int newX = iSectorX + dx;
            int newY = iSectorY + dy;

            if (newX >= 0 && newX < dfSECTOR_MAX_X && newY >= 0 && newY < dfSECTOR_MAX_Y)
            {
                pSectorAround->Around[pSectorAround->iCount++] = { newX, newY };
            }
        }
    }
}


void AddSector(st_SECTOR_POS sectorPos, st_Character* pCharacter)
{
    if (sectorPos.iX >= 0 && sectorPos.iX < dfSECTOR_MAX_X &&
        sectorPos.iY >= 0 && sectorPos.iY < dfSECTOR_MAX_Y)
    {
        auto& lock = g_SectorLock[sectorPos.iY][sectorPos.iX];
        AcquireSRWLockExclusive(&lock);
        //EnterCriticalSection(&lock);
        g_Sector[sectorPos.iY][sectorPos.iX].emplace_back(pCharacter);
        //LeaveCriticalSection(&lock);
        ReleaseSRWLockExclusive(&lock);
    }
}

void RemoveSector(st_SECTOR_POS sectorPos, st_Character* pCharacter)
{
    if (sectorPos.iX >= 0 && sectorPos.iX < dfSECTOR_MAX_X &&
        sectorPos.iY >= 0 && sectorPos.iY < dfSECTOR_MAX_Y)
    {
        auto& sectorList = g_Sector[sectorPos.iY][sectorPos.iX];
        auto& lock = g_SectorLock[sectorPos.iY][sectorPos.iX];
        AcquireSRWLockExclusive(&lock);
        //EnterCriticalSection(&lock);
        auto it = std::find(sectorList.begin(), sectorList.end(), pCharacter);
        if (it != sectorList.end())
        {
            sectorList.erase(it);
        }
        //LeaveCriticalSection(&lock);
        ReleaseSRWLockExclusive(&lock);
        //sectorList.remove(pCharacter);
    }
}

void InitSectorLock()
{
    for (int y = 0; y < dfSECTOR_MAX_Y; y++)
    {
        for (int x = 0; x < dfSECTOR_MAX_X; x++)
        {
            InitializeSRWLock(&g_SectorLock[y][x]);
        }
    }
}

void UninitSectorLock()
{
    //for (int y = 0; y < dfSECTOR_MAX_Y; y++)
    //{
    //    for (int x = 0; x < dfSECTOR_MAX_X; x++)
    //    {
    //        DeleteCriticalSection(&g_SectorLock[y][x]);
    //    }
    //}
}

int CheckTotalSector()
{
    int cnt = 0;

    for (int y = 0; y < dfSECTOR_MAX_Y; y++)
    {
        for (int x = 0; x < dfSECTOR_MAX_X; x++)
        {
            cnt += g_Sector[y][x].size();
        }
    }
    return cnt;
}

void CheckTotalSectorSize()
{
    for (int y = 0; y < dfSECTOR_MAX_Y; y++)
    {
        for (int x = 0; x < dfSECTOR_MAX_X; x++)
        {
            if (g_Sector[y][x].size() == 0)
                continue;
            wprintf(L"y : %d / x : %d / cnt : %d\n", y, x, (int)g_Sector[y][x].size());
        }
    }
}
