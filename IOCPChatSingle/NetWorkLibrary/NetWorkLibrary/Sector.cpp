//#include "Content.h"
//#include "Sector.h"
//#include <list>
#include "pch.h"
#include "Content.h"

std::list<st_Character*> g_Sector[dfSECTOR_MAX_Y][dfSECTOR_MAX_X];

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

void GetUpdateSectorAround(st_Character* pCharacter, st_SECTOR_AROUND* pRemoveSector, st_SECTOR_AROUND* pAddSector)
{
    st_SECTOR_AROUND oldSector, newSector;
    GetSectorAround(pCharacter->OldSector.iX, pCharacter->OldSector.iY, &oldSector);
    GetSectorAround(pCharacter->CurSector.iX, pCharacter->CurSector.iY, &newSector);

    pRemoveSector->iCount = 0;
    pAddSector->iCount = 0;

    for (int i = 0; i < oldSector.iCount; i++)
    {
        bool isInNewSector = false;
        for (int j = 0; j < newSector.iCount; j++)
        {
            if (oldSector.Around[i].iX == newSector.Around[j].iX && oldSector.Around[i].iY == newSector.Around[j].iY)
            {
                isInNewSector = true;
                break;
            }
        }

        if (!isInNewSector)
        {
            pRemoveSector->Around[pRemoveSector->iCount++] = oldSector.Around[i];
        }
    }

    for (int i = 0; i < newSector.iCount; i++)
    {
        bool isInOldSector = false;
        for (int j = 0; j < oldSector.iCount; j++)
        {
            if (newSector.Around[i].iX == oldSector.Around[j].iX && newSector.Around[i].iY == oldSector.Around[j].iY)
            {
                isInOldSector = true;
                break;
            }
        }
        if (!isInOldSector)
        {
            pAddSector->Around[pAddSector->iCount++] = newSector.Around[i];
        }
    }


    /*GetSectorAround(pCharacter->OldSector.iX, pCharacter->OldSector.iY, pRemoveSector);
    GetSectorAround(pCharacter->CurSector.iX, pCharacter->CurSector.iY, pAddSector);*/
}

void AddSector(st_SECTOR_POS sectorPos, st_Character* pCharacter)
{
    if (sectorPos.iX >= 0 && sectorPos.iX < dfSECTOR_MAX_X &&
        sectorPos.iY >= 0 && sectorPos.iY < dfSECTOR_MAX_Y)
    {
        g_Sector[sectorPos.iY][sectorPos.iX].emplace_back(pCharacter);
    }
}

void RemoveSector(st_SECTOR_POS sectorPos, st_Character* pCharacter)
{
    if (sectorPos.iX >= 0 && sectorPos.iX < dfSECTOR_MAX_X &&
        sectorPos.iY >= 0 && sectorPos.iY < dfSECTOR_MAX_Y)
    {
        auto& sectorList = g_Sector[sectorPos.iY][sectorPos.iX];

        auto it = std::find(sectorList.begin(), sectorList.end(), pCharacter);
        if (it != sectorList.end())
        {
            sectorList.erase(it);
        }
        //sectorList.remove(pCharacter);

    }
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
