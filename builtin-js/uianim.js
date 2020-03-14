var x0 = 0, y0 = 0, x1 = 135, y1 = 135, x2 = 0, y2 = 135;
var x = 100, y = 200, r = 0;
var tx = 0, ty = 0;

function updateFrame() {
    print('Free Mem: '+os.getFreeMem()+'\r');
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

    ui.drawText('你好,mcu.js', tx, ty);
    tx++; ty++;
    if (tx == 135)
        tx = 0;
    if (ty == 240)
        ty = 0;

    ui.update();
}

function playAnim() {
    ui.init();
    ui.setPenColor(0xDA0);
    setInterval(updateFrame, 33);
}