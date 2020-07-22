var screen = new ui.Screen();
var scrollLayout = new ui.ScrollLayout();
var tv = new ui.TextView();

tv.text = 'ESP32';
tv.autoLineBreak = true;
tv.setPos(0, 0);
tv.setSize(40, 40);
tv.setBackground(0x9090);
tv.setCornerRadius(10);
tv.setPadding(10);
scrollLayout.addView(tv);

var btn = new ui.Button();
btn.text = 'Touch Me!'
btn.setSize(80, 80);
btn.setPos(60, 60);
btn.setBackground(ui.makeColor(100, 100, 100));
btn.setReleasedColor(ui.makeColor(100, 100, 100));
btn.setCornerRadius(10);
scrollLayout.addView(btn);

screen.addView(scrollLayout)

function updateFrame() {
    screen.draw();
}

function playAnim() {
    setInterval(updateFrame, 1);
    setInterval(ui.dispatchTouchEvents, 1);
}