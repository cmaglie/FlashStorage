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

#include "FlashAsEEPROM.h"

// separate flash storage for eeprom valid flag
FlashStorage(eeprom_valid, int);

EEPROMClass::EEPROMClass(void) : _initialized(false), _dirty(false) {
  // Empty
}

uint8_t EEPROMClass::read(int address) {
  // if no storage has been set or address is out of range -> just do nothing
  // and return 0
  if (flash_class == NULL || address >= flash_class->length())
    return 0;

  return _eeprom[address];
}

void EEPROMClass::write(int address, uint8_t value) { update(address, value); }

void EEPROMClass::update(int address, uint8_t value) {
  // if no storage has been set or address is out of range -> just do nothing
  if (flash_class == NULL || address >= flash_class->length())
    return;

  if (_eeprom[address] != value) {
    _dirty = true;
    _eeprom[address] = value;
  }
}

bool EEPROMClass::isValid() { return eeprom_valid.read() == TRUE; }

void EEPROMClass::commit() {

  // if no storage has been set, just do nothing
  if (flash_class == NULL)
    return;

  // if something changed and thus dirty-flag is set ...
  if (_dirty) {

    // do the actual commit by erasing and writing data
    flash_class->erase();
    flash_class->write(_eeprom);

    // set valid flag, if not yet done.
    if (!isValid()) {
      eeprom_valid.write((int)TRUE);
    }

    _dirty = false; // toggle dirty back to non-dirty
  }
}

void EEPROMClass::setStorageForceValid(FlashClass *ptr) {
  if (!isValid()) {
    eeprom_valid.write((int)TRUE); // force valid flag to true
  }
  setStorage(ptr);               // continue with normal setStorage() ...
}

// PRIVATE begins here

void EEPROMClass::init() {

  // if no storage has been set, just do nothing
  if (flash_class == NULL)
    return;

  // init the local RAM version of EEPROM with correct size
  _eeprom = new byte[flash_class->length()];

  // read the flash and return eeprom data
  flash_class->read(_eeprom);

  // if EEPROM value is not valid, clear EEPROM with 0xFF
  if (!isValid()) {
    memset(_eeprom, 0xFF, flash_class->length());
  }

  _initialized = true;
}

EEPROMClass EEPROM;
