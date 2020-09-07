var screen = new ui.Screen();
screen.size = { width: 240, height: 240 }

var tv = new ui.TextView();
tv.setSize(100, 240)
tv.setPos(140, 0)
screen.addView(tv);

var lv = new ui.ListView();
lv.setSize(140, 240);
lv.setPos(0, 0);
lv.buildItem = function (data) {
    var item = new ui.ListViewItem();

    var a = new ui.TextView();
    a.setText(data.title);
    a.setSize(-1, -1)
    a.autoLineBreak = true;
    a.clickTransparent = true;
    item.addViewRelativly(a);

    return item;
}
lv.onItemClicked = function (data, args) {
    print('Item clicked: ' + data.title);
    tv.setText('Item clicked:\n' + data.title);
}
lv.setItemSource([
    { title: 'Item 1' },
    { title: 'Item 2' },
    { title: 'Item 3' },
    { title: 'Item 4' },
])
screen.addView(lv);

function updateFrame() {
    screen.draw();
}

//function playAnim() {
setInterval(updateFrame, 1);
setInterval(ui.dispatchTouchEvents, 1);
//}