/* mbed Microcontroller Library
 * Copyright (c) 2015-2016 Nuvoton
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "NuSDFileSystem.h"
#include "mbed_debug.h"
#include "PeripheralPins.h"

#if defined(TARGET_NUMAKER_PFM_NUC472)
#define NU_SDH_CDn          PF_6
#define NU_SDH_CMD          PF_7
#define NU_SDH_CLK          PF_8
#define NU_SDH_DAT0         PF_5
#define NU_SDH_DAT1         PF_4
#define NU_SDH_DAT2         PF_3
#define NU_SDH_DAT3         PF_2
#endif

#if defined(TARGET_NUMAKER_PFM_NUC472)
extern DISK_DATA_T SD_DiskInfo0;
extern DISK_DATA_T SD_DiskInfo1;
extern SD_INFO_T SD0,SD1;
extern int sd0_ok,sd1_ok;
#endif

#define SD_DBG             0

NuSDFileSystem::NuSDFileSystem(const char* name) :
    FATFileSystem(name) {

    init_sdh(NU_SDH_CDn, NU_SDH_CMD, NU_SDH_CLK, NU_SDH_DAT0 ,NU_SDH_DAT1, NU_SDH_DAT2, NU_SDH_DAT3);
}
        
NuSDFileSystem::NuSDFileSystem(PinName SD_CDn, PinName SD_CMD, PinName SD_CLK, PinName SD_DAT0, PinName SD_DAT1, PinName SD_DAT2, PinName SD_DAT3, const char* name) :
    FATFileSystem(name) {

    init_sdh(SD_CDn, SD_CMD, SD_CLK, SD_DAT0, SD_DAT1, SD_DAT2, SD_DAT3);
}

void NuSDFileSystem::init_sdh(PinName SD_CDn, PinName SD_CMD, PinName SD_CLK,
        PinName SD_DAT0, PinName SD_DAT1, PinName SD_DAT2, PinName SD_DAT3) {
    
    debug_if(SD_DBG, "SD MPF Setting & Enable SD IP Clock\n");
    
    // Configure SD IP clock 
    SYS_UnlockReg();
#if defined(TARGET_NUMAKER_PFM_NUC472)
    // Enable IP clock
    CLK->AHBCLK |= CLK_AHBCLK_SDHCKEN_Msk; // SD Card driving clock.
    CLK_SetModuleClock(SDH_MODULE, CLK_CLKSEL0_SDHSEL_PLL, 1);
#endif
    SYS_LockReg();
            
            
    // Configure SD multi-function pins
    uint32_t Sdcdn = pinmap_peripheral(SD_CDn,  PinMap_SD_CD);
    uint32_t Sdcmd = pinmap_peripheral(SD_CMD,  PinMap_SD_CMD);
    uint32_t Sdclk = pinmap_peripheral(SD_CLK,  PinMap_SD_CLK);
    uint32_t Sddt0 = pinmap_peripheral(SD_DAT0, PinMap_SD_DAT0);
    uint32_t Sddt1 = pinmap_peripheral(SD_DAT1, PinMap_SD_DAT1);
    uint32_t Sddt2 = pinmap_peripheral(SD_DAT2, PinMap_SD_DAT2);
    uint32_t Sddt3 = pinmap_peripheral(SD_DAT3, PinMap_SD_DAT3);
        
    uint32_t Sdctl = (SDName) pinmap_merge(Sdclk, Sdcmd);
    uint32_t Sddg0 = (SDName) pinmap_merge(Sddt0, Sddt1);
    uint32_t Sddg1 = (SDName) pinmap_merge(Sddt2, Sddt3);   
    uint32_t Sdct2 = (SDName) pinmap_merge(Sddg0, Sddg1);
    uint32_t Sdct3 = (SDName) pinmap_merge(Sdctl, Sdct2);
    uint32_t Sdct4 = (SDName) pinmap_merge(Sdct3, Sdcdn);
    
    if (Sdct4 == NC) {
        debug("SD pinmap error\n");
        return;
    }
    
    pinmap_pinout(SD_CDn, PinMap_SD_CD);
    pinmap_pinout(SD_CMD, PinMap_SD_CMD);
    pinmap_pinout(SD_CLK, PinMap_SD_CLK);
    pinmap_pinout(SD_DAT0, PinMap_SD_DAT0);
    pinmap_pinout(SD_DAT1, PinMap_SD_DAT1);
    pinmap_pinout(SD_DAT2, PinMap_SD_DAT2);
    pinmap_pinout(SD_DAT3, PinMap_SD_DAT3);

    // 
#if defined(TARGET_NUMAKER_PFM_NUC472)
    if(NU_MODSUBINDEX(Sdct4) == 0)
        _sdport = SD_PORT0;
    else if(NU_MODSUBINDEX(Sdct4) == 1)
        _sdport = SD_PORT1;
#endif
}

void NuSDFileSystem::init_card() {
#if defined(TARGET_NUMAKER_PFM_NUC472)
    SD_Open(_sdport | CardDetect_From_GPIO);
    SD_Probe(_sdport);
#endif
}

bool NuSDFileSystem::card_inited()
{
    bool initialized = false;
    
#if defined(TARGET_NUMAKER_PFM_NUC472)
    switch (_sdport) {
    case SD_PORT0:
        initialized = sd0_ok && (SD0.CardType != SD_TYPE_UNKNOWN);
        break;
    
    case SD_PORT1:
        initialized = sd1_ok && (SD1.CardType != SD_TYPE_UNKNOWN);
        break;
    }
#endif

    return initialized;
}

int NuSDFileSystem::disk_initialize() {
    
    init_card();
    if (! card_inited()) {
        debug("Fail to initialize card\n");
        return 1;
    }
    debug_if(SD_DBG, "init card = %d\n", card_inited());
    
    return 0;
}

int NuSDFileSystem::disk_write(const uint8_t* buffer, uint32_t block_number, uint32_t count) {
    if (! card_inited()) {
        return -1;
    }
    
    if(SD_Write(_sdport, (uint8_t*)buffer, block_number, count)!=0)
        return 1;
    else
        return 0;
        
}

int NuSDFileSystem::disk_read(uint8_t* buffer, uint32_t block_number, uint32_t count) {
    if (! card_inited()) {
        return -1;
    }
    
    if(SD_Read(_sdport, (uint8_t*)buffer, block_number, count)!=0)
        return 1;
    else
        return 0;

}

int NuSDFileSystem::disk_status() {
    // FATFileSystem::disk_status() returns 0 when initialized
    if (card_inited()) {
        return 0;
    } else {
        return 1;
    }
}

uint32_t NuSDFileSystem::disk_sectors() {
    
    uint32_t n_sector = 0;
    
#if defined(TARGET_NUMAKER_PFM_NUC472)
    switch (_sdport) {
    case SD_PORT0:
        n_sector = SD_DiskInfo0.totalSectorN;
        break;
    case SD_PORT1:
        n_sector = SD_DiskInfo1.totalSectorN;
        break;
    }
#endif

    return n_sector;
}
