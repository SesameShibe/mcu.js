var screen = new ui.Screen();
// setInterval(function () { screen.draw(); }, 33)
var tv = new ui.TextView();

tv.text = 'ESP32';
tv.autoLineBreak = true;
tv.setPos(0, 0);
tv.setSize(240, 240);
tv.setBackground(0x9090);
tv.setCornerRadius(10);
tv.setPadding(10);
screen.addView(tv);

var btn = new ui.Button();
btn.text = 'Touch Me!'
btn.setSize(80, 80);
btn.setPos(60, 60);
btn.setBackground(ui.makeColor(100, 100, 100));
btn.setReleasedColor(ui.makeColor(100, 100, 100));
btn.setCornerRadius(10);
//screen.addView(btn);

var r = 255, g = 0, b = 0;

function updateFrame() {
    screen.draw();
}

function playAnim() {
    setInterval(updateFrame, 1);
    setInterval(ui.dispatchTouchEvent, 1);
}