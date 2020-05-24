const PIN_SDA = 21
const PIN_SCL = 22
i2c.init(0, PIN_SDA, PIN_SCL)
for (var addr = 0; addr < 128; addr++) {
    i2c.start(0);
    var ack = i2c.addr(0, addr, 0);
    i2c.stop(0);
    if (ack) {
        print('Found Device Addr: 0x' + addr.toString(16))
    }
}
print('Scan done.')