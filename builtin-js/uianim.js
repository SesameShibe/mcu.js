var x0 = 0, y0 = 0, x1 = 135, y1 = 135, x2 = 0, y2 = 135;
var x = 100, y = 200, r = 0;
var tx = 0, ty = 0;

function updateFrame() {
    ui.clear();
    ui.drawTriangle(x0, y0, x1, y1, x2, y2);

    x0++; y0++; x1--; y1--; x2++; y2--;
    if (x0 == 135) {
        x0 = 0, y0 = 0, x1 = 135, y1 = 135, x2 = 0, y2 = 135;
    }

    ui.drawCircle(x, y, r);
    r++;
    if (r > 200)
        r = 0;

    ui.drawText('ESP32 采用 40 nm 工艺制成，\n\
具有最佳的功耗性能、射频性能、\n\
稳定性、通用性和可靠性，适用于各种应用场景和不同功耗需求。\n\n\
乐鑫为用户提供完整的软、硬件资源，\n\
进行 ESP32 硬件设备的开发。\n\
其中，乐鑫的软件开发环境 ESP-IDF \n\
旨在协助用户快速开发物联网 (IoT) 应用，\n\
可满足用户对 Wi-Fi、蓝牙、\n\
低功耗等方面的要求。', 0, 0);

    ui.update();
}

function playAnim() {
    ui.init();
    ui.setPenColor(0xDA0);
    setInterval(updateFrame, 33);
}