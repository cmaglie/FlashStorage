/*
  EEPROM like API that uses Arduino Zero's flash memory.
  Written by A. Christian

  Copyright (c) 2015-2020 Arduino LLC.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef FLASH_AS_EEPROM_h
#define FLASH_AS_EEPROM_h

#include "FlashStorage.h"

#define TRUE 0x01

class EEPROMClass {

  public:
    EEPROMClass(void);

    /**
     * Read an EEPROM cell
     * @param index
     * @return value or 0 if index is out of EEPROM range
     */
    uint8_t read(int);

    /**
     * Write value to an EEPROM cell. Will do nothing if index is out of EEPROM range.
     * @param index
     * @param value
     */
    void write(int, uint8_t);

    /**
     * Update a EEPROM cell
     * @param index
     * @param value
     */
    void update(int, uint8_t);

    /**
     * Check whether the EEPROM data is valid
     * @return true, if EEPROM data is valid (has been written at least once), false if not
     */
    bool isValid();

    /**
     * Write previously made EEPROM changes to the underlying flash storage
     * Use this with care: Each and every commit will harm the flash and reduce it's lifetime (like with every flash memory)
     */
    void commit();

    /**
     * Returns the size of the EEPROM in bytes
     */
    uint32_t length() { 
      if (_initialized) {
        return flash_class->length(); 
      } else {
        return 0; 
      }
    }

    /**
     * For using the EEPROM, you need to specify the underlying FlashClass pointer. For example, useful
     * <code>
     * Flash(my_eeprom_storage, EEPROM_EMULATION_SIZE);
     * </code>
     * outside setup() and loop() and provide "my_eeprom_storage" as pointer to this method. 
     * EEPROM_EMULATION_SIZE specifies the size in bytes of your "virtual EEPROM" (1024, 2048, 4096, 8192). 
     */
    void setStorage(FlashClass* ptr) { 
      flash_class = ptr; 
      init();
    }

    /**
     * Same as setStorage(), but forced the valid flag (before internal initalization is done) to true.
     * Might be useful if you specify the flash-storage to a specific address of flash and maintain your own valid flag.
     */
    void setStorageForceValid(FlashClass*);

  private:
    void init();
    FlashClass* flash_class;
    bool _initialized;
    byte* _eeprom;
    bool _dirty;
};

extern EEPROMClass EEPROM;

#endif
