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

#elif defined(TARGET_NUMAKER_PFM_M487)
#define NU_SDH_CDn          PE_13
#define NU_SDH_CMD          PE_12
#define NU_SDH_CLK          PC_0
#define NU_SDH_DAT0         PC_4
#define NU_SDH_DAT1         PC_3
#define NU_SDH_DAT2         PC_2
#define NU_SDH_DAT3         PC_1

#endif

#if defined(TARGET_NUMAKER_PFM_NUC472)
extern DISK_DATA_T SD_DiskInfo0;
extern DISK_DATA_T SD_DiskInfo1;
extern SD_INFO_T SD0,SD1;
extern int sd0_ok,sd1_ok;

#elif defined(TARGET_NUMAKER_PFM_M487)
extern int SDH_ok;
extern SDH_INFO_T SD0, SD1;

#endif

#define SD_DBG             0

NuSDFileSystem::NuSDFileSystem(const char* name) :
    FATFileSystem(name), _sdh((SDName) NC),
#if defined(TARGET_NUMAKER_PFM_NUC472)
    _sdh_port((uint32_t) -1)
#elif defined(TARGET_NUMAKER_PFM_M487)
    _sdh_base(NULL)
#endif
{

    init_sdh(NU_SDH_CDn, NU_SDH_CMD, NU_SDH_CLK, NU_SDH_DAT0 ,NU_SDH_DAT1, NU_SDH_DAT2, NU_SDH_DAT3);
}
        
NuSDFileSystem::NuSDFileSystem(PinName SD_CDn, PinName SD_CMD, PinName SD_CLK, PinName SD_DAT0, PinName SD_DAT1, PinName SD_DAT2, PinName SD_DAT3, const char* name) :
    FATFileSystem(name), _sdh((SDName) NC),
#if defined(TARGET_NUMAKER_PFM_NUC472)
    _sdh_port((uint32_t) -1)
#elif defined(TARGET_NUMAKER_PFM_M487)
    _sdh_base(NULL)
#endif
{

    init_sdh(SD_CDn, SD_CMD, SD_CLK, SD_DAT0, SD_DAT1, SD_DAT2, SD_DAT3);
}

void NuSDFileSystem::init_sdh(PinName SD_CDn, PinName SD_CMD, PinName SD_CLK,
        PinName SD_DAT0, PinName SD_DAT1, PinName SD_DAT2, PinName SD_DAT3) {
    
    debug_if(SD_DBG, "SD MPF Setting & Enable SD IP Clock\n");
        
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
    
    if (Sdct4 == (uint32_t) NC) {
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

    // Configure SD IP clock 
    SYS_UnlockReg();
    
    // Determine SDH port dependent on passed-in pins
    _sdh = (SDName) Sdct4;
#if defined(TARGET_NUMAKER_PFM_NUC472)
    switch (NU_MODSUBINDEX(_sdh)) {
    case 0:
        _sdh_port = SD_PORT0;
        break;
        
    case 1:
        _sdh_port = SD_PORT1;
        break;
    }
    
    // Enable IP clock
    CLK->AHBCLK |= CLK_AHBCLK_SDHCKEN_Msk; // SD Card driving clock.
    CLK_SetModuleClock(SDH_MODULE, CLK_CLKSEL0_SDHSEL_PLL, 1);
    
#elif defined(TARGET_NUMAKER_PFM_M487)
    _sdh_base = (SDH_T *) NU_MODBASE(_sdh);
    
    switch (NU_MODINDEX(_sdh)) {
    case 0:
        CLK->CLKSEL0 = (CLK->CLKSEL0 & ~CLK_CLKSEL0_SDH0SEL_Msk) | CLK_CLKSEL0_SDH0SEL_HCLK;
        CLK->AHBCLK |= CLK_AHBCLK_SDH0CKEN_Msk; // SD Card driving clock.
        break;
        
    case 1:
        CLK->CLKSEL0 = (CLK->CLKSEL0 & ~CLK_CLKSEL0_SDH1SEL_Msk) | CLK_CLKSEL0_SDH1SEL_HCLK;
        CLK->AHBCLK |= CLK_AHBCLK_SDH1CKEN_Msk; // SD Card driving clock.
        break;
    }
    
#endif

    SYS_LockReg();
}

void NuSDFileSystem::init_card() {

    if (_sdh == (SDName) NC) {
        return;
    }
    
#if defined(TARGET_NUMAKER_PFM_NUC472)
    SD_Open(_sdh_port | CardDetect_From_GPIO);
    SD_Probe(_sdh_port);

#elif defined(TARGET_NUMAKER_PFM_M487)
    SDH_Open(_sdh_base, CardDetect_From_GPIO);
    SDH_Probe(_sdh_base);
    
#endif
}

bool NuSDFileSystem::card_inited()
{
    bool initialized = false;
    
#if defined(TARGET_NUMAKER_PFM_NUC472)
    switch (_sdh_port) {
    case SD_PORT0:
        initialized = sd0_ok && (SD0.CardType != SD_TYPE_UNKNOWN);
        break;
    
    case SD_PORT1:
        initialized = sd1_ok && (SD1.CardType != SD_TYPE_UNKNOWN);
        break;
    }
    
#elif defined(TARGET_NUMAKER_PFM_M487)
    switch (NU_MODINDEX(_sdh)) {
    case 0:
        initialized = SDH_ok && (SD0.CardType != SDH_TYPE_UNKNOWN);
        break;
    
    case 1:
        initialized = SDH_ok && (SD1.CardType != SDH_TYPE_UNKNOWN);
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

#if defined(TARGET_NUMAKER_PFM_NUC472)
    if (SD_Write(_sdh_port, (uint8_t*)buffer, block_number, count) != 0) {
#elif defined(TARGET_NUMAKER_PFM_M487)
    if (SDH_Write(_sdh_base, (uint8_t*)buffer, block_number, count) != 0) {
#endif
        return 1;
    }
    
    return 0;    
}

int NuSDFileSystem::disk_read(uint8_t* buffer, uint32_t block_number, uint32_t count) {
    if (! card_inited()) {
        return -1;
    }
    
#if defined(TARGET_NUMAKER_PFM_NUC472)
    if (SD_Read(_sdh_port, (uint8_t*)buffer, block_number, count) != 0) {
#elif defined(TARGET_NUMAKER_PFM_M487)
    if (SDH_Read(_sdh_base, (uint8_t*)buffer, block_number, count) != 0) {
#endif
        return 1;
    }
    
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
    switch (_sdh_port) {
    case SD_PORT0:
        n_sector = SD_DiskInfo0.totalSectorN;
        break;
    case SD_PORT1:
        n_sector = SD_DiskInfo1.totalSectorN;
        break;
    }
    
#elif defined(TARGET_NUMAKER_PFM_M487)
    switch (NU_MODINDEX(_sdh)) {
    case 0:
        n_sector = SD0.totalSectorN;
        break;
    case 1:
        n_sector = SD1.totalSectorN;
        break;
    }
    
#endif

    return n_sector;
}
