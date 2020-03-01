

const ST7789_NOP = '\x00';
const ST7789_SWRESET = '\x01';
const ST7789_RDDID = '\x04';
const ST7789_RDDST = '\x09';

const ST7789_RDDPM = '\x0A';      // Read display power mode
const ST7789_RDD_MADCTL = '\x0B';      // Read display MADCTL
const ST7789_RDD_COLMOD = '\x0C';      // Read display pixel format
const ST7789_RDDIM = '\x0D';      // Read display image mode
const ST7789_RDDSM = '\x0E';      // Read display signal mode
const ST7789_RDDSR = '\x0F';      // Read display self-diagnostic result (ST7789V)

const ST7789_SLPIN = '\x10';
const ST7789_SLPOUT = '\x11';
const ST7789_PTLON = '\x12';
const ST7789_NORON = '\x13';

const ST7789_INVOFF = '\x20';
const ST7789_INVON = '\x21';
const ST7789_GAMSET = '\x26';      // Gamma set
const ST7789_DISPOFF = '\x28';
const ST7789_DISPON = '\x29';
const ST7789_CASET = '\x2A';
const ST7789_RASET = '\x2B';
const ST7789_RAMWR = '\x2C';
const ST7789_RGBSET = '\x2D';      // Color setting for 4096, 64K and 262K colors
const ST7789_RAMRD = '\x2E';

const ST7789_PTLAR = '\x30';
const ST7789_VSCRDEF = '\x33';      // Vertical scrolling definition (ST7789V)
const ST7789_TEOFF = '\x34';      // Tearing effect line off
const ST7789_TEON = '\x35';      // Tearing effect line on
const ST7789_MADCTL = '\x36';      // Memory data access control
const ST7789_IDMOFF = '\x38';      // Idle mode off
const ST7789_IDMON = '\x39';      // Idle mode on
const ST7789_RAMWRC = '\x3C';      // Memory write continue (ST7789V)
const ST7789_RAMRDC = '\x3E';      // Memory read continue (ST7789V)
const ST7789_COLMOD = '\x3A';

const ST7789_RAMCTRL = '\xB0';      // RAM control
const ST7789_RGBCTRL = '\xB1';      // RGB control
const ST7789_PORCTRL = '\xB2';      // Porch control
const ST7789_FRCTRL1 = '\xB3';      // Frame rate control
const ST7789_PARCTRL = '\xB5';      // Partial mode control
const ST7789_GCTRL = '\xB7';      // Gate control
const ST7789_GTADJ = '\xB8';      // Gate on timing adjustment
const ST7789_DGMEN = '\xBA';      // Digital gamma enable
const ST7789_VCOMS = '\xBB';      // VCOMS setting
const ST7789_LCMCTRL = '\xC0';      // LCM control
const ST7789_IDSET = '\xC1';      // ID setting
const ST7789_VDVVRHEN = '\xC2';      // VDV and VRH command enable
const ST7789_VRHS = '\xC3';      // VRH set
const ST7789_VDVSET = '\xC4';      // VDV setting
const ST7789_VCMOFSET = '\xC5';      // VCOMS offset set
const ST7789_FRCTR2 = '\xC6';      // FR Control 2
const ST7789_CABCCTRL = '\xC7';      // CABC control
const ST7789_REGSEL1 = '\xC8';      // Register value section 1
const ST7789_REGSEL2 = '\xCA';      // Register value section 2
const ST7789_PWMFRSEL = '\xCC';      // PWM frequency selection
const ST7789_PWCTRL1 = '\xD0';      // Power control 1
const ST7789_VAPVANEN = '\xD2';      // Enable VAP/VAN signal output
const ST7789_CMD2EN = '\xDF';      // Command 2 enable
const ST7789_PVGAMCTRL = '\xE0';      // Positive voltage gamma control
const ST7789_NVGAMCTRL = '\xE1';      // Negative voltage gamma control
const ST7789_DGMLUTR = '\xE2';      // Digital gamma look-up table for red
const ST7789_DGMLUTB = '\xE3';      // Digital gamma look-up table for blue
const ST7789_GATECTRL = '\xE4';      // Gate control
const ST7789_SPI2EN = '\xE7';      // SPI2 enable
const ST7789_PWCTRL2 = '\xE8';      // Power control 2
const ST7789_EQCTRL = '\xE9';      // Equalize time control
const ST7789_PROMCTRL = '\xEC';      // Program control
const ST7789_PROMEN = '\xFA';      // Program mode enable
const ST7789_NVMSET = '\xFC';      // NVM setting
const ST7789_PROMACT = '\xFE';      // Program action


var cmdList = []
function tftWriteCommand(cmd) {
    cmdList.push([cmd.charCodeAt(0)])
}

function tftWriteData(dat) {
    var cmd = cmdList[cmdList.length - 1]
    for (var i = 0; i < dat.length; i++) {
        cmd.push(dat.charCodeAt(i))
    }
}

var os = {}
os.sleepMs = function(ms) {
    cmdList.push([-1, ms])
}

function tftInit() {
    
    tftWriteCommand(ST7789_SWRESET);
    tftWriteCommand(ST7789_SWRESET);
    os.sleepMs(120);
    // Sleep out
    tftWriteCommand(ST7789_SLPOUT);
    os.sleepMs(120);

    // Normal display mode on
    tftWriteCommand(ST7789_NORON);

    // Display and color format setting
    tftWriteCommand(ST7789_MADCTL);
    tftWriteData('\x08');

    // JLX240 display datasheet
    //tftWriteCommand('\xB6\x0A\x82');

    tftWriteCommand(ST7789_COLMOD);
    tftWriteData('\x55');
    os.sleepMs(10);

    //ST7789V Frame rate setting
    tftWriteCommand(ST7789_PORCTRL);
    tftWriteData('\x0c\x0c\x00\x33\x33');

    tftWriteCommand(ST7789_GCTRL);      // Voltages: VGH / VGL
    tftWriteData('\x35');

    //ST7789V Power setting
    tftWriteCommand(ST7789_VCOMS);
    tftWriteData('\x28');		// JLX240 display datasheet

    tftWriteCommand(ST7789_LCMCTRL);
    tftWriteData('\x0C');

    tftWriteCommand(ST7789_VDVVRHEN);
    tftWriteData('\x01\xFF');

    tftWriteCommand(ST7789_VRHS);       // voltage VRHS
    tftWriteData('\x10');

    tftWriteCommand(ST7789_VDVSET);
    tftWriteData('\x20');

    tftWriteCommand(ST7789_FRCTR2);
    tftWriteData('\x0f');

    tftWriteCommand(ST7789_PWCTRL1);
    tftWriteData('\xa4\xa1');
/*
    //ST7789V gamma setting
    tftWriteCommand(ST7789_PVGAMCTRL);
    tftWriteData('\xd0\x00\x02\x07\x0a\x28\x32\x44\x42\x06\x0e\x12\x14\x17');

    tftWriteCommand(ST7789_NVGAMCTRL);
    tftWriteData('\xd0\x00\x02\x07\x0a\x28\x31\x54\x47\x0e\x1c\x17\x1b\x1e');
*/
    tftWriteCommand(ST7789_INVON);

    tftWriteCommand(ST7789_CASET);    // Column address set
    tftWriteData('\x00\x00\x00\xE5');    // 239

    tftWriteCommand(ST7789_RASET);    // Row address set
    tftWriteData('\x00\x00\x01\x3F');    // 319

    tftWriteCommand(ST7789_DISPON);    //Display on
}

tftInit()
console.log(JSON.stringify(cmdList))
