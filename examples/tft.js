const TFT_MOSI = 19;
const TFT_SCLK = 18;
const TFT_CS = 5; 
const TFT_DC = 16;
const TFT_BL = 4;
const TFT_RST = 23;

const TFT_WIDTH = 135;
const TFT_HEIGHT = 240;

gpio.pinMode(TFT_CS, gpio.OUTPUT)
gpio.pinMode(TFT_BL, gpio.OUTPUT)
gpio.pinMode(TFT_DC, gpio.OUTPUT)
gpio.pinMode(TFT_RST, gpio.OUTPUT)
gpio.write(TFT_BL, 1)
spi.begin(TFT_SCLK, -1, TFT_MOSI, -1)

function tftWrite8(u8, isData) {
    print(u8 +' ' + isData)
    gpio.write(TFT_DC, isData? 1 : 0);
    gpio.write(TFT_CS, 0)
    spi.write8(u8)
    gpio.write(TFT_CS, 1)
}

function tftInit(initCode) {
    for (var i = 0; i < initCode.length; i++) {
        var cmd = initCode[i]
        if (cmd[0] == -1) {
            os.sleepMs(cmd[1])
            continue
        }
        tftWrite8(cmd[0], 0)
        for (var j = 1; j < cmd.length; j++) {
            tftWrite8(cmd[j], 1)
        }
    }
}
gpio.write(TFT_RST, 1)
os.sleepMs(5)
gpio.write(TFT_RST, 0)
os.sleepMs(100)
gpio.write(TFT_RST, 1)

tftInit([[-1,120],[17],[-1,120],[19],[54,8],[58,85],[-1,10],[178,12,12,0,51,51],[183,53],[187,40],[192,12],[194,1,255],[195,16],[196,32],[198,15],[208,164,161],[33],[42,0,0,0,229],[43,0,0,1,63],[41]])

