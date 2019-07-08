# Mongoose OS Driver for TI BQ27421 Fuel Gauge

## Overview

This library contains driver for the [TI BQ27421](http://www.ti.com/product/BQ27421-G1) li-ion battery fuel gauge.

## Example usage

Initialize and read some registers:

```c

#include "mgos.h"
#include "mgos_bq27421.h"

struct mgos_bq27421 *g = mgos_bq27421_create(mgos_i2c_get_global());

uint16_t dt = 0, fwv = 0, dcap = 0;

if (!mgos_bq27421_ctl_read(g, MGOS_BQ27421_CTL_DEVICE_TYPE, &dt) ||
    !mgos_bq27421_ctl_read(g, MGOS_BQ27421_CTL_FW_VERSION, &fwv) ||
    !mgos_bq27421_reg_read(g, MGOS_BQ27421_REG_CAP_DESIGN, &dcap)) {
  // Error
}

LOG(LL_INFO, ("Gauge type 0x%x, fw version 0x%x, battery design capacity %u mAh", dt, fwv, dcap));
```

Update battery design capacity (signed 16-bit integer in data class 82 at offset 10):

```
if (!mgos_bq27421_data_write_i2(g, 82, 10, 1234 /* mAh */)) {
  // Error
}
```

The rest of the API is documented in the [include/mgos_bq27421.h](header file).

## Acknowledgements

This driver is contributed by [Watchdog System](https://www.watchdog-system.com/).

## License

Released under Apache License 2.0, see [LICENSE](LICENSE) file.
