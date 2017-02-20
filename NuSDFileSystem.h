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

#ifndef MBED_SDFILESYSTEM_H
#define MBED_SDFILESYSTEM_H

#include "mbed.h"
#include "FATFileSystem.h"
#include <stdint.h>

class NuSDFileSystem : public FATFileSystem {
public:

    NuSDFileSystem(const char* name);
    NuSDFileSystem(PinName SD_CDn, PinName SD_CMD, PinName SD_CLK, PinName SD_DAT0, PinName SD_DAT1, PinName SD_DAT2, PinName SD_DAT3, const char* name);
    virtual int disk_initialize();
    virtual int disk_status();
    virtual int disk_read(uint8_t* buffer, uint32_t block_number, uint32_t count);
    virtual int disk_write(const uint8_t* buffer, uint32_t block_number, uint32_t count);
    virtual uint32_t disk_sectors();

protected:

    void init_sdh(PinName SD_CDn, PinName SD_CMD, PinName SD_CLK,
        PinName SD_DAT0, PinName SD_DAT1, PinName SD_DAT2, PinName SD_DAT3);
    void init_card();
    bool card_inited();
    int cdv;
#if defined(TARGET_NUMAKER_PFM_NUC472)
    int _sdport;
#endif

private:
    int NuSDPinConfig(PinName SD_CDn, PinName SD_CMD, PinName SD_CLK, PinName SD_DAT0, PinName SD_DAT1, PinName SD_DAT2, PinName SD_DAT3);
};

#endif
