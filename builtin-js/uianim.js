var screen = new ui.Screen();
// setInterval(function () { screen.draw(); }, 33)
var tv = new ui.TextView();

tv.text = 'ESP32 采用 40 nm 工艺制成，\n\
具有最佳的功耗性能、射频性能、\n\
稳定性、通用性和可靠性，适用于各种应用场景和不同功耗需求。\n\n\
乐鑫为用户提供完整的软、硬件资源，\n\
进行 ESP32 硬件设备的开发。\n\
其中，乐鑫的软件开发环境 ESP-IDF \n\
旨在协助用户快速开发物联网 (IoT) 应用，\n\
可满足用户对 Wi-Fi、蓝牙、\n\
低功耗等方面的要求。';
tv.autoLineBreak = true;
tv.setPos(0, 0);
tv.setSize(240, 240);
tv.setBackground(0x9090);
tv.setCornerRadius(10);
tv.setPadding(10);
screen.addView(tv);

var r = 255, g = 0, b = 0;

function updateFrame() {
    tv.setBackground(ui.makeColor(r, g, b));

    var l = ((r * 0.299) + (g * 0.587) + (b * 0.144));
    tv.text = '红(R):' + r + '\n绿(G):' + g + '\n蓝(B):' + b + '\n亮度(L): ' + l;
    if (l > 140) {
        tv.setForeground(ui.makeColor(0, 0, 0));
    } else {
        tv.setForeground(ui.makeColor(255, 255, 255));
    }

    if (r == 255 && g < 255 && b == 0) {
        g++;
    } else if (r > 0 && g == 255 && b == 0) {
        r--;
    } else if (r == 0 && g == 255 && b < 255) {
        b++;
    } else if (r == 0 && g > 0 && b == 255) {
        g--;
    } else if (r < 255 && g == 0 && b == 255) {
        r++;
    } else if (r == 255 && g == 0 && b > 0) {
        b--;
    }

    screen.draw();
}

function playAnim() {
    setInterval(updateFrame, 1);
}