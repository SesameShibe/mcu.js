'use strict';


global.board = {
    probe: [],
    name: 'generic'
}

global.board.probe.push(function () {
    // board support for ttgo t-watch 
    const I2C_ADDR_AXP202 = 0x35
    const I2C_ADDR_FT5206 = 0x38

    const CHIP_ID_AXP202 = 0x41
    const CHIP_ID_FT5206_FT6236 = 0x64

    const AXP_REG_CHIP_ID = 0x03
    const AXP_REG_OFF_CTL = 0x32
    const AXP_REG_LDO234_DC23_CTL = 0x12

    const PIN_BL = 12

    var axpChipID = 0


    // I2C bus 0: AXP202(PMIC) + PCF8563(RTC) + BMA423(Sensor)
    i2c.init(0, 21, 22)
    for (var retry = 0; retry < 3; retry++) {
        axpChipID = i2c.simpleRead8(0, I2C_ADDR_AXP202, AXP_REG_CHIP_ID)
        if (axpChipID == CHIP_ID_AXP202) {
            break
        }
        os.sleepMs(10)
    }
    if (axpChipID == CHIP_ID_AXP202) {
        print('AXP202 detected, assuming the board is TTGO t-watch.')
    } else {
        return false
    }

    var board = global.board
    board.pmic = {
        shutdown: function () {
            board.pmic.setBit(AXP_REG_OFF_CTL, 7)
        },
        setBit: function (reg, bit) {
            i2c.simpleWrite8(0, I2C_ADDR_AXP202, reg,
                i2c.simpleRead8(0, I2C_ADDR_AXP202, reg) | (1 << bit))
        },
        clearBit: function (reg, bit) {
            i2c.simpleWrite8(0, I2C_ADDR_AXP202, reg,
                (i2c.simpleRead8(0, I2C_ADDR_AXP202, reg) | (1 << bit)) - (1 << bit))
        }
    }

    gpio.config(PIN_BL, gpio.OUTPUT, 0)
    board.bl = {
        set: function (brightness) {
            if (brightness > 0) {
                board.pmic.setBit(AXP_REG_LDO234_DC23_CTL, 2)
                gpio.write(PIN_BL, 1)
            } else {
                board.pmic.clearBit(AXP_REG_LDO234_DC23_CTL, 2)
                gpio.write(PIN_BL, 0)
            }
        }
    }

    var _buf = new Uint8Array(16)
    // I2C bus 1: FT5206/FT6236(Touch) 
    i2c.init(1, 23, 32)
    global.board.touch = {
        readOne: function (arr) {
            i2c.simpleReadBuf(1, I2C_ADDR_FT5206, 0, _buf)
            if (_buf[2] < 1) {
                return 0
            }
            arr[0] = _buf[0x03] & 0x0F;
            arr[0] <<= 8;
            arr[0] |= _buf[0x04];
            arr[1] = _buf[0x05] & 0x0F;
            arr[1] <<= 8;
            arr[1] |= _buf[0x06];
            arr[2] = _buf[0x05] >> 4;
            return 1;
        }
    }
    ui.init()
    board.bl.set(100)
    global.board.name = 't-watch'
    return true
});

(function () {
    for (var i = 0; i < global.board.probe.length; i++) {
        if ((global.board.probe[i])()) {
            break
        }
    }
    delete global.board.probe;
})();