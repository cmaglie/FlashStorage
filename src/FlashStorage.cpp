/*
  Copyright (c) 2015 Arduino LLC.  All right reserved.
  Written by Cristian Maglie

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

#include "FlashStorage.h"

static const uint32_t pageSizes[] = { 8, 16, 32, 64, 128, 256, 512, 1024 };

FlashClass::FlashClass(const void *flash_addr, uint32_t size) :
  PAGE_SIZE(pageSizes[NVMCTRL->PARAM.bit.PSZ]),
  PAGES(NVMCTRL->PARAM.bit.NVMP),
  MAX_FLASH(PAGE_SIZE * PAGES),
#if defined(__SAMD51__)
  ROW_SIZE(MAX_FLASH / 64),
#else
  ROW_SIZE(PAGE_SIZE * 4),
#endif
  flash_address((volatile void *)flash_addr),
  flash_size(size)
{
}

static inline uint32_t read_unaligned_uint32(const void *data)
{
  union {
    uint32_t u32;
    uint8_t u8[4];
  } res;
  const uint8_t *d = (const uint8_t *)data;
  res.u8[0] = d[0];
  res.u8[1] = d[1];
  res.u8[2] = d[2];
  res.u8[3] = d[3];
  return res.u32;
}

#if defined(__SAMD51__)
// Invalidate all CMCC cache entries if CMCC cache is enabled.
static void invalidate_CMCC_cache()
{
  if (CMCC->SR.bit.CSTS) {
    CMCC->CTRL.bit.CEN = 0;
    while (CMCC->SR.bit.CSTS) {}
    CMCC->MAINT0.bit.INVALL = 1;
    CMCC->CTRL.bit.CEN = 1;
  }
}
#endif

void FlashClass::write(const volatile void *flash_ptr, const void *data, uint32_t size)
{
  // Calculate data boundaries
  size = (size + 3) / 4;
  volatile uint32_t *dst_addr = (volatile uint32_t *)flash_ptr;
  const uint8_t *src_addr = (uint8_t *)data;

  // Disable automatic page write
#if defined(__SAMD51__)
  NVMCTRL->CTRLA.bit.WMODE = 0;
  while (NVMCTRL->STATUS.bit.READY == 0) { }
  // Disable NVMCTRL cache while writing, per SAMD51 errata.
  bool original_CACHEDIS0 = NVMCTRL->CTRLA.bit.CACHEDIS0;
  bool original_CACHEDIS1 = NVMCTRL->CTRLA.bit.CACHEDIS1;
  NVMCTRL->CTRLA.bit.CACHEDIS0 = true;
  NVMCTRL->CTRLA.bit.CACHEDIS1 = true;
#else
  NVMCTRL->CTRLB.bit.MANW = 1;
#endif

  // Do writes in pages
  while (size) {
    // Execute "PBC" Page Buffer Clear
#if defined(__SAMD51__)
    NVMCTRL->CTRLB.reg = NVMCTRL_CTRLB_CMDEX_KEY | NVMCTRL_CTRLB_CMD_PBC;
    while (NVMCTRL->INTFLAG.bit.DONE == 0) { }
#else
    NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_PBC;
    while (NVMCTRL->INTFLAG.bit.READY == 0) { }
#endif

    // Fill page buffer
    uint32_t i;
    for (i=0; i<(PAGE_SIZE/4) && size; i++) {
      *dst_addr = read_unaligned_uint32(src_addr);
      src_addr += 4;
      dst_addr++;
      size--;
    }

    // Execute "WP" Write Page
#if defined(__SAMD51__)
    NVMCTRL->CTRLB.reg = NVMCTRL_CTRLB_CMDEX_KEY | NVMCTRL_CTRLB_CMD_WP;
    while (NVMCTRL->INTFLAG.bit.DONE == 0) { }
    invalidate_CMCC_cache();
    // Restore original NVMCTRL cache settings.
    NVMCTRL->CTRLA.bit.CACHEDIS0 = original_CACHEDIS0;
    NVMCTRL->CTRLA.bit.CACHEDIS1 = original_CACHEDIS1;
#else
    NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_WP;
    while (NVMCTRL->INTFLAG.bit.READY == 0) { }
#endif
  }
}

void FlashClass::erase(const volatile void *flash_ptr, uint32_t size)
{
  const uint8_t *ptr = (const uint8_t *)flash_ptr;
  while (size > ROW_SIZE) {
    erase(ptr);
    ptr += ROW_SIZE;
    size -= ROW_SIZE;
  }
  erase(ptr);
}

void FlashClass::erase(const volatile void *flash_ptr)
{
#if defined(__SAMD51__)
  NVMCTRL->ADDR.reg = ((uint32_t)flash_ptr);
  NVMCTRL->CTRLB.reg = NVMCTRL_CTRLB_CMDEX_KEY | NVMCTRL_CTRLB_CMD_EB;
  while (!NVMCTRL->INTFLAG.bit.DONE) { }
  invalidate_CMCC_cache();
#else
  NVMCTRL->ADDR.reg = ((uint32_t)flash_ptr) / 2;
  NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_ER;
  while (!NVMCTRL->INTFLAG.bit.READY) { }
#endif
}

void FlashClass::read(const volatile void *flash_ptr, void *data, uint32_t size)
{
  memcpy(data, (const void *)flash_ptr, size);
}

