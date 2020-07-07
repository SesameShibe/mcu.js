function mapRange(num, fromMin, fromMax, toMin, toMax) {
    return (num - fromMin) * (toMax - toMin) / (fromMax - fromMin) + toMin;
}

(function () {
    touch = {};
    touch.BUSID = 1;
    touch.SDA = 23;
    touch.SCL = 32;
    touch.SLAVE_ADDRESS = 0x38;
    touch.TOUCHES_REG = 0x02;
    touch.THRESHHOLD_REG = 0x80;
    touch.DEVIDE_MODE = 0x00;
    touch.TD_STATUS = 2;
    touch.TOUCH1_XH = 3;
    touch.TOUCH1_XL = 4;
    touch.TOUCH1_YH = 5;
    touch.TOUCH1_YL = 6;
    touch.MONITOR = 1;
    touch.SLEEP = 3;
    touch.POWER_REG = 0x87;
    touch.isInited = false;

    touch.writeByte = function (reg, b) {
        i2c.simpleWrite8(touch.BUSID, touch.SLAVE_ADDRESS, reg, b);
    }
    touch.readByte = function (reg) {
        return i2c.simpleRead8(touch.BUSID, touch.SLAVE_ADDRESS, reg);
    }
    touch.init = function () {
        i2c.init(touch.BUSID, touch.SDA, touch.SCL, 0);
        touch.isInited = true;
    }
    touch.setThreshold = function (threshold) {
        if (!touch.isInited) return;
        touch.writeByte(touch.THRESHHOLD_REG, threshold);
    }
    touch.isTouched = function () {
        if (!touch.isInited) return false;

        var touches = touch.readByte(touch.TOUCHES_REG);
        if (touches == 1 || touches == 2)
            return true;
        else
            return false;
    }
    touch.getPoint = function (n) {
        if (!touch.isInited) return { x: 0, y: 0 };

        buf = new Buffer(16);
        i2c.simpleReadBuf(touch.BUSID, touch.SLAVE_ADDRESS, touch.DEVIDE_MODE, buf);
        var touches = buf[touch.TD_STATUS];
        if (touches == 0 || touches > 2) return { x: 0, y: 0 };

        px = ((buf[touch.TOUCH1_XH + (n * 6)] & 0x0F) << 8) | buf[touch.TOUCH1_XL + (n * 6)];
        py = ((buf[touch.TOUCH1_YH + (n * 6)] & 0x0F) << 8) | buf[touch.TOUCH1_YL + (n * 6)];
        id = buf[touch.TOUCH1_YH + (n * 6)] >> 4;

        // Touch pad size is 320*320,
        // we need to map the point to lcd size 240*240.
        px = mapRange(px, 0, 320, 0, 240);
        py = mapRange(py, 0, 320, 0, 240);
        return { x: px, y: py };
    }
    touch.monitor = function () {
        if (!touch.isInited) return;
        touch.writeByte(touch.POWER_REG, touch.MONITOR);
    }
    touch.sleep = function () {
        if (!touch.isInited) return;
        touch.writeByte(touch.POWER_REG, touch.SLEEP);
    }
})();