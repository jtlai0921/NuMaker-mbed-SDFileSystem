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

#define SD_DBG             0

NuSDFileSystem::NuSDFileSystem(const char* name) :
    FATFileSystem(name), _is_initialized(0) {

#if defined TARGET_NUMAKER_PFM_NUC472
			
	// Set SD MFP Pin
	if(NuSDPinConfig(PF_6, PF_7, PF_8, PF_5 ,PF_4, PF_3, PF_2) == 1)
		debug_if(SD_DBG, "Pin Error\n");
	
	// Enable IP clock
	CLK->AHBCLK |= CLK_AHBCLK_SDHCKEN_Msk; // SD Card driving clock.
			
	debug_if(SD_DBG, "SD MPF Setting & Enable SD IP Clock\n");

#endif
}
		
NuSDFileSystem::NuSDFileSystem(PinName SD_CDn, PinName SD_CMD, PinName SD_CLK, PinName SD_DAT0, PinName SD_DAT1, PinName SD_DAT2, PinName SD_DAT3, const char* name) :
    FATFileSystem(name), _is_initialized(0) {

#if defined TARGET_NUMAKER_PFM_NUC472
		
	// Set SD MFP Pin	
	if(NuSDPinConfig(SD_CDn, SD_CMD, SD_CLK, SD_DAT0, SD_DAT1, SD_DAT2, SD_DAT3) == 1)
		debug_if(SD_DBG, "Pin Error\n");
	
	// Enable IP clock
	CLK->AHBCLK |= CLK_AHBCLK_SDHCKEN_Msk; // SD Card driving clock.
			
	debug_if(SD_DBG, "SD MPF Setting & Enable SD IP Clock\n");

#endif
}
		
int NuSDFileSystem::NuSDPinConfig(PinName SD_CDn, PinName SD_CMD, PinName SD_CLK, PinName SD_DAT0, PinName SD_DAT1, PinName SD_DAT2, PinName SD_DAT3){

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
	
	if(Sdct4 == NC)
		return 1;
	if(NU_MODSUBINDEX(Sdct4) == 0)
		cardport = 0;
	else if(NU_MODSUBINDEX(Sdct4) == 1)
		cardport = 2;
		
	
	pinmap_pinout(SD_CDn, PinMap_SD_CD);
	pinmap_pinout(SD_CMD, PinMap_SD_CMD);
	pinmap_pinout(SD_CLK, PinMap_SD_CLK);
	pinmap_pinout(SD_DAT0, PinMap_SD_DAT0);
	pinmap_pinout(SD_DAT1, PinMap_SD_DAT1);
	pinmap_pinout(SD_DAT2, PinMap_SD_DAT2);
	pinmap_pinout(SD_DAT3, PinMap_SD_DAT3);
	
	return 0;
}

extern DISK_DATA_T SD_DiskInfo0;
extern DISK_DATA_T SD_DiskInfo1;
extern SD_INFO_T SD0,SD1;
extern int sd0_ok,sd1_ok;

int NuSDFileSystem::initialise_card() {
	
	SD_Open((1<<cardport) | CardDetect_From_GPIO);
	SD_Probe(((1<<cardport) | CardDetect_From_GPIO) & 0x00ff);

	if(sd0_ok == 1 && cardport == 0)
		return SD0.CardType;
	else if(sd1_ok == 1 && cardport == 2)
		return SD1.CardType;
	else
		return SD_TYPE_UNKNOWN;
}

int NuSDFileSystem::disk_initialize() {
    
	_is_initialized = initialise_card();
	
	if (_is_initialized == 0) {
		debug("Fail to initialize card\n");
		return 1;
	}
	debug_if(SD_DBG, "init card = %d\n", _is_initialized);
		
	if(cardport == 0)
		_sectors = SD_DiskInfo0.totalSectorN;
  else if(cardport == 2)
		_sectors = SD_DiskInfo1.totalSectorN;
	return 0;
}

int NuSDFileSystem::disk_write(const uint8_t* buffer, uint32_t block_number, uint32_t count) {
	if (!_is_initialized) {
		return -1;
	}
    
	if(SD_Write((1<<cardport), (uint8_t*)buffer, block_number, count)!=0)
		return 1;
	else
		return 0;
		
}

int NuSDFileSystem::disk_read(uint8_t* buffer, uint32_t block_number, uint32_t count) {
	if (!_is_initialized) {
		return -1;
	}
    
	if(SD_Read((1<<cardport), (uint8_t*)buffer, block_number, count)!=0)
		return 1;
	else
		return 0;

}

int NuSDFileSystem::disk_status() {
	// FATFileSystem::disk_status() returns 0 when initialized
	if (_is_initialized) {
		return 0;
	} else {
		return 1;
	}
}

uint32_t NuSDFileSystem::disk_sectors() { return _sectors; }
