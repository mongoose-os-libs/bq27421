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

#include "mgos_bq27421.h"

#include <stdlib.h>

#include "mgos.h"

static inline bool mgos_bq27421_sleep(void) {
  mgos_usleep(100);
  return true;
}

struct mgos_bq27421 *mgos_bq27421_create(struct mgos_i2c *bus) {
  if (bus == NULL) return NULL;
  struct mgos_bq27421 *ctx = (struct mgos_bq27421 *) calloc(1, sizeof(*ctx));
  if (ctx != NULL) mgos_bq27421_init_ctx(ctx, bus);
  return ctx;
}

void mgos_bq27421_init_ctx(struct mgos_bq27421 *ctx, struct mgos_i2c *bus) {
  memset(ctx, 0, sizeof(*ctx));
  ctx->bus = bus;
}

bool mgos_bq27421_reg_read(struct mgos_bq27421 *ctx, enum mgos_bq27421_reg reg,
                           uint16_t *val) {
  if (ctx == NULL || ctx->bus == NULL) return false;
  int res = mgos_i2c_read_reg_w(ctx->bus, MGOS_BQ27421_ADDR, reg);
  if (res < 0) return false;
  *val = ((res & 0xff) << 8) | (res >> 8);  // Swap bytes.
  return true;
}

bool mgos_bq27421_ctl(struct mgos_bq27421 *ctx, enum mgos_bq27421_ctl_cmd cmd) {
  if (ctx == NULL || ctx->bus == NULL) return false;
  uint16_t cmd_le = ((((uint16_t) cmd) << 8) | (((uint16_t) cmd) >> 8));
  if (!mgos_i2c_write_reg_w(ctx->bus, MGOS_BQ27421_ADDR, MGOS_BQ27421_REG_CTL,
                            cmd_le)) {
    return false;
  }
  switch (cmd) {
    case MGOS_BQ27421_CTL_SEALED:
      ctx->unsealed = false;
      break;
    case MGOS_BQ27421_CTL_UNSEAL:
      // Should count two of these, but... meh.
      ctx->unsealed = true;
      break;
    default:
      break;
  }
  return true;
}

bool mgos_bq27421_ctl_read(struct mgos_bq27421 *ctx,
                           enum mgos_bq27421_ctl_cmd cmd, uint16_t *res) {
  *res = 0;
  if (!mgos_bq27421_ctl(ctx, cmd)) return false;
  return mgos_bq27421_reg_read(ctx, MGOS_BQ27421_REG_CTL, res);
}

static bool mgos_bq27421_unseal(struct mgos_bq27421 *ctx) {
  if (ctx->unsealed) return true;
  // Send the 0x8000 ctl twice to unseal.
  if (!mgos_bq27421_ctl(ctx, MGOS_BQ27421_CTL_UNSEAL)) return false;
  if (!mgos_bq27421_ctl(ctx, MGOS_BQ27421_CTL_UNSEAL)) return false;
  ctx->unsealed = true;
  return true;
}

bool mgos_bq27421_enter_cfg_update(struct mgos_bq27421 *ctx) {
  if (ctx->cfg_update) return true;
  uint16_t flags = 0;
  if (!mgos_bq27421_reg_read(ctx, MGOS_BQ27421_REG_FLAGS, &flags)) return false;
  if (flags & MGOS_BQ27421_FLAG_CFGUPMODE) goto out;
  if (!mgos_bq27421_unseal(ctx)) return false;
  if (!mgos_bq27421_ctl(ctx, MGOS_BQ27421_CTL_SET_CFGUPDATE)) return false;
  while (!(flags & MGOS_BQ27421_FLAG_CFGUPMODE)) {
    mgos_bq27421_sleep();
    if (!mgos_bq27421_reg_read(ctx, MGOS_BQ27421_REG_FLAGS, &flags))
      return false;
  }
  // Enable block data access.
  if (!mgos_i2c_write_reg_b(ctx->bus, MGOS_BQ27421_ADDR,
                            MGOS_BQ27421_REG_BLOCK_DATA_CTL, 0)) {
    return false;
  }
out:
  ctx->cfg_update = true;
  return true;
}

bool mgos_bq27421_exit_cfg_update(struct mgos_bq27421 *ctx, bool seal) {
  uint16_t flags = 0;
  bool reset = true;
  do {
    if (!mgos_bq27421_reg_read(ctx, MGOS_BQ27421_REG_FLAGS, &flags)) {
      return false;
    }
    if (reset && !mgos_bq27421_ctl(ctx, MGOS_BQ27421_CTL_SOFT_RESET)) {
      return false;
    }
    reset = false;
    mgos_bq27421_sleep();
  } while (flags & MGOS_BQ27421_FLAG_CFGUPMODE);
  if (seal && !mgos_bq27421_ctl(ctx, MGOS_BQ27421_CTL_SEALED)) return false;
  return true;
}

bool mgos_bq27421_data_read(struct mgos_bq27421 *ctx, uint8_t data_class,
                            uint8_t offset, uint8_t len, uint8_t *data) {
  if (!mgos_bq27421_unseal(ctx)) return false;
  uint8_t block_num = (offset >> 5);
  uint8_t block_offset = (offset & 0x1f);
  mgos_bq27421_sleep();
  if (!mgos_i2c_write_reg_b(ctx->bus, MGOS_BQ27421_ADDR,
                            MGOS_BQ27421_REG_DATA_CLASS, data_class) ||
      !mgos_bq27421_sleep() ||
      !mgos_i2c_write_reg_b(ctx->bus, MGOS_BQ27421_ADDR,
                            MGOS_BQ27421_REG_DATA_BLOCK, block_num)) {
    return false;
  }
  mgos_bq27421_sleep();
  if (!mgos_i2c_read_reg_n(ctx->bus, MGOS_BQ27421_ADDR,
                           MGOS_BQ27421_REG_BLOCK_DATA + block_offset, len,
                           data)) {
    return false;
  }
  return true;
}

bool mgos_bq27421_data_write(struct mgos_bq27421 *ctx, uint8_t data_class,
                             uint8_t offset, uint8_t len, const uint8_t *data) {
  if (!mgos_bq27421_enter_cfg_update(ctx)) {
    return false;
  }
  uint8_t old_data[len], old_cs = 0;
  if (!mgos_bq27421_data_read(ctx, data_class, offset, len, old_data) ||
      !mgos_bq27421_sleep() ||
      !mgos_i2c_read_reg_n(ctx->bus, MGOS_BQ27421_ADDR,
                           MGOS_BQ27421_REG_BLOCK_DATA_CHECKSUM, 1, &old_cs)) {
    return false;
  }
  uint8_t new_cs = 0xff - old_cs;
  for (uint8_t i = 0; i < len; i++) {
    new_cs -= old_data[i];
  }
  for (uint8_t i = 0; i < len; i++) {
    new_cs += data[i];
  }
  new_cs = 0xff - new_cs;
  // The read has already set class and block number, so we just need to write
  // data.
  uint8_t block_offset = (offset & 0x1f);
  if (!mgos_i2c_write_reg_n(ctx->bus, MGOS_BQ27421_ADDR,
                            MGOS_BQ27421_REG_BLOCK_DATA + block_offset, len,
                            data)) {
    return false;
  }
  mgos_bq27421_sleep();
  // Writing checksum commits the update.
  if (!mgos_i2c_write_reg_b(ctx->bus, MGOS_BQ27421_ADDR,
                            MGOS_BQ27421_REG_BLOCK_DATA_CHECKSUM, new_cs)) {
    return false;
  }
  mgos_bq27421_sleep();
  return true;
}

bool mgos_bq27421_data_read_i1(struct mgos_bq27421 *ctx, uint8_t data_class,
                               uint8_t offset, int8_t *val) {
  return mgos_bq27421_data_read_u1(ctx, data_class, offset, (uint8_t *) val);
}

bool mgos_bq27421_data_read_i2(struct mgos_bq27421 *ctx, uint8_t data_class,
                               uint8_t offset, int16_t *val) {
  return mgos_bq27421_data_read_u2(ctx, data_class, offset, (uint16_t *) val);
}

bool mgos_bq27421_data_read_i4(struct mgos_bq27421 *ctx, uint8_t data_class,
                               uint8_t offset, int32_t *val) {
  return mgos_bq27421_data_read_u4(ctx, data_class, offset, (uint32_t *) val);
}

bool mgos_bq27421_data_read_u1(struct mgos_bq27421 *ctx, uint8_t data_class,
                               uint8_t offset, uint8_t *val) {
  return mgos_bq27421_data_read(ctx, data_class, offset, 1, val);
}

bool mgos_bq27421_data_read_u2(struct mgos_bq27421 *ctx, uint8_t data_class,
                               uint8_t offset, uint16_t *val) {
  if (!mgos_bq27421_data_read(ctx, data_class, offset, 2, (uint8_t *) val))
    return false;
  *val = ntohs(*val);
  return true;
}

bool mgos_bq27421_data_read_u4(struct mgos_bq27421 *ctx, uint8_t data_class,
                               uint8_t offset, uint32_t *val) {
  if (!mgos_bq27421_data_read(ctx, data_class, offset, 4, (uint8_t *) val))
    return false;
  *val = ntohl(*val);
  return true;
}

bool mgos_bq27421_data_write_i1(struct mgos_bq27421 *ctx, uint8_t data_class,
                                uint8_t offset, int8_t val) {
  return mgos_bq27421_data_write_u1(ctx, data_class, offset, (uint8_t) val);
}

bool mgos_bq27421_data_write_i2(struct mgos_bq27421 *ctx, uint8_t data_class,
                                uint8_t offset, int16_t val) {
  return mgos_bq27421_data_write_u2(ctx, data_class, offset, (uint16_t) val);
}

bool mgos_bq27421_data_write_i4(struct mgos_bq27421 *ctx, uint8_t data_class,
                                uint8_t offset, int32_t val) {
  return mgos_bq27421_data_write_u4(ctx, data_class, offset, (uint32_t) val);
}

bool mgos_bq27421_data_write_u1(struct mgos_bq27421 *ctx, uint8_t data_class,
                                uint8_t offset, uint8_t val) {
  return mgos_bq27421_data_write(ctx, data_class, offset, 1, &val);
}

bool mgos_bq27421_data_write_u2(struct mgos_bq27421 *ctx, uint8_t data_class,
                                uint8_t offset, uint16_t val) {
  uint16_t data = htons((uint16_t) val);
  return mgos_bq27421_data_write(ctx, data_class, offset, 2, (uint8_t *) &data);
}

bool mgos_bq27421_data_write_u4(struct mgos_bq27421 *ctx, uint8_t data_class,
                                uint8_t offset, uint32_t val) {
  uint32_t data = htonl((uint32_t) val);
  return mgos_bq27421_data_write(ctx, data_class, offset, 4, (uint8_t *) &data);
}

bool mgos_bq27421_init(void) {
  return true;
}
