/*
 * Copyright (c) 2019 Watchdog System
 * All rights reserved
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

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "mgos_i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MGOS_BQ27421_ADDR 0x55

struct mgos_bq27421 {
  struct mgos_i2c *bus;
  bool unsealed;
  bool cfg_update;
};

enum mgos_bq27421_reg {
  MGOS_BQ27421_REG_CTL = 0x00,
  MGOS_BQ27421_REG_TEMP = 0x02,
  MGOS_BQ27421_REG_VOLT = 0x04,
  MGOS_BQ27421_REG_FLAGS = 0x06,
  MGOS_BQ27421_REG_CAP_NOM = 0x08,
  MGOS_BQ27421_REG_CAP_FULL = 0x0a,
  MGOS_BQ27421_REG_CAP_RM = 0x0c,
  MGOS_BQ27421_REG_CAP_FCC = 0x0e,
  MGOS_BQ27421_REG_CUR_AVG = 0x10,
  MGOS_BQ27421_REG_CUR_SB = 0x12,
  MGOS_BQ27421_REG_CUR_MAX = 0x14,
  MGOS_BQ27421_REG_PWR_AVG = 0x18,
  MGOS_BQ27421_REG_SOC = 0x1c,
  MGOS_BQ27421_REG_TEMP_INT = 0x1e,
  MGOS_BQ27421_REG_SOH = 0x20,
  MGOS_BQ27421_REG_CAP_RM_UNF = 0x28,
  MGOS_BQ27421_REG_CAP_RM_F = 0x2a,
  MGOS_BQ27421_REG_FCC_UNF = 0x2c,
  MGOS_BQ27421_REG_FCC_F = 0x2e,
  MGOS_BQ27421_REG_SOC_UNF = 0x30,
  MGOS_BQ27421_REG_OP_CONFIG = 0x3a,
  MGOS_BQ27421_REG_CAP_DESIGN = 0x3c,
  MGOS_BQ27421_REG_DATA_CLASS = 0x3e,
  MGOS_BQ27421_REG_DATA_BLOCK = 0x3f,
  MGOS_BQ27421_REG_BLOCK_DATA = 0x40,  // 0x40 - 0x5f
  MGOS_BQ27421_REG_BLOCK_DATA_CHECKSUM = 0x60,
  MGOS_BQ27421_REG_BLOCK_DATA_CTL = 0x61,
};

enum mgos_bq27421_ctl_cmd {
  MGOS_BQ27421_CTL_STATUS = 0x0000,
  MGOS_BQ27421_CTL_DEVICE_TYPE = 0x0001,
  MGOS_BQ27421_CTL_FW_VERSION = 0x0002,
  MGOS_BQ27421_CTL_DM_CODE = 0x0004,
  MGOS_BQ27421_CTL_PREV_MACWRITE = 0x0007,
  MGOS_BQ27421_CTL_CHEM_ID = 0x0008,
  MGOS_BQ27421_CTL_BAT_INSERT = 0x000c,
  MGOS_BQ27421_CTL_BAT_REMOVE = 0x000d,
  MGOS_BQ27421_CTL_SET_HIBERNATE = 0x0011,
  MGOS_BQ27421_CTL_CLEAR_HIBERNATE = 0x0012,
  MGOS_BQ27421_CTL_SET_CFGUPDATE = 0x0013,
  MGOS_BQ27421_CTL_SHUTDOWN_ENABLE = 0x001B,
  MGOS_BQ27421_CTL_SHUTDOWN = 0x001C,
  MGOS_BQ27421_CTL_SEALED = 0x0020,
  MGOS_BQ27421_CTL_PULSE_SOC_INT = 0x0023,
  MGOS_BQ27421_CTL_RESET = 0x0041,
  MGOS_BQ27421_CTL_SOFT_RESET = 0x0042,
  MGOS_BQ27421_CTL_EXIT_CFGUPDATE = 0x0043,
  MGOS_BQ27421_CTL_EXIT_RESIM = 0x0044,
  MGOS_BQ27421_CTL_UNSEAL = 0x8000,  // Must be sent twice.
};

#define MGOS_BQ27421_FLAG_CFGUPMODE (1 << 4)

struct mgos_bq27421 *mgos_bq27421_create(struct mgos_i2c *bus);

void mgos_bq27421_init_ctx(struct mgos_bq27421 *ctx, struct mgos_i2c *bus);

// Read register value.
bool mgos_bq27421_reg_read(struct mgos_bq27421 *ctx, enum mgos_bq27421_reg reg,
                           uint16_t *val);

// Issue a control command.
bool mgos_bq27421_ctl(struct mgos_bq27421 *ctx, enum mgos_bq27421_ctl_cmd cmd);

// Issue a control command and read back the value.
bool mgos_bq27421_ctl_read(struct mgos_bq27421 *ctx,
                           enum mgos_bq27421_ctl_cmd cmd, uint16_t *res);

// Unseal the device and put it into config update mode (if not already).
bool mgos_bq27421_enter_cfg_update(struct mgos_bq27421 *ctx);

// Exit the config mode and optionally seal the device.
bool mgos_bq27421_exit_cfg_update(struct mgos_bq27421 *ctx, bool seal);

// Data memory interface.

// Read len data bytes from certain class, block and offset.
// Device must be unsealed.
// Note: offset + len must not cross block boundary (32 bytes).
bool mgos_bq27421_data_read(struct mgos_bq27421 *ctx, uint8_t data_class,
                            uint8_t offset, uint8_t len, uint8_t *data);

// Helpers to read individual data fields.
bool mgos_bq27421_data_read_i1(struct mgos_bq27421 *ctx, uint8_t data_class,
                               uint8_t offset, int8_t *val);
bool mgos_bq27421_data_read_i2(struct mgos_bq27421 *ctx, uint8_t data_class,
                               uint8_t offset, int16_t *val);
bool mgos_bq27421_data_read_i4(struct mgos_bq27421 *ctx, uint8_t data_class,
                               uint8_t offset, int32_t *val);
bool mgos_bq27421_data_read_u1(struct mgos_bq27421 *ctx, uint8_t data_class,
                               uint8_t offset, uint8_t *val);
bool mgos_bq27421_data_read_u2(struct mgos_bq27421 *ctx, uint8_t data_class,
                               uint8_t offset, uint16_t *val);
bool mgos_bq27421_data_read_u4(struct mgos_bq27421 *ctx, uint8_t data_class,
                               uint8_t offset, uint32_t *val);

// Write an entire data block. Calculates and updates checksum as needed.
// Will enter config update mode if necessary.
// Note: offset + len must not cross block boundary (32 bytes).
bool mgos_bq27421_data_write(struct mgos_bq27421 *ctx, uint8_t data_class,
                             uint8_t offset, uint8_t len, const uint8_t *data);

// Helpers to update individual data fields.
bool mgos_bq27421_data_write_i1(struct mgos_bq27421 *ctx, uint8_t data_class,
                                uint8_t offset, int8_t val);
bool mgos_bq27421_data_write_i2(struct mgos_bq27421 *ctx, uint8_t data_class,
                                uint8_t offset, int16_t val);
bool mgos_bq27421_data_write_i4(struct mgos_bq27421 *ctx, uint8_t data_class,
                                uint8_t offset, int32_t val);
bool mgos_bq27421_data_write_u1(struct mgos_bq27421 *ctx, uint8_t data_class,
                                uint8_t offset, uint8_t val);
bool mgos_bq27421_data_write_u2(struct mgos_bq27421 *ctx, uint8_t data_class,
                                uint8_t offset, uint16_t val);
bool mgos_bq27421_data_write_u4(struct mgos_bq27421 *ctx, uint8_t data_class,
                                uint8_t offset, uint32_t val);

void mgos_bq27421_free(struct mgos_bq27421 *ctx);

#ifdef __cplusplus
}
#endif
