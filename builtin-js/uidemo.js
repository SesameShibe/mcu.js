var screen = new ui.Screen();
screen.size = { width: 240, height: 240 }

var tv = new ui.TextView();
tv.setSize(100, 200)
tv.setPos(140, 0)
screen.addView(tv);

var pb = new ui.ProgressBar();
pb.setCornerRadius(5);
pb.setSize(100, 40);
pb.setPos(140, 200);
screen.addView(pb);

var lv = new ui.ListView();
lv.setSize(140, 240);
lv.setPos(0, 0);
lv.buildItem = function (item, data) {
    var icon = new ui.Icon(data.icon);
    icon.setSize(16, 16);
    icon.setPos(1, 8);
    icon.setBackground(ui.Colors.transparent);
    item.addViewRelativly(icon);

    var title = new ui.TextView();
    title.setText(data.title);
    title.setSize(100, 28);
    title.setPos(17, 1);
    title.setBorderStyle(ui.BorderStyle.none);
    title.setBackground(ui.Colors.transparent);
    title.autoLineBreak = true;
    title.touchTransparent = true;
    item.addViewRelativly(title);
}
lv.onItemClicked = function (data, args) {
    print('Item clicked: ' + data.title);
    tv.setText('Item clicked:\n' + data.title);
}
lv.setItemSource([
    { title: 'Amazon', icon: Icons.amazon },
    { title: 'Apple', icon: Icons.apple },
    { title: 'Facebook', icon: Icons.facebook },
    { title: 'Google', icon: Icons.google },
    { title: 'Microsoft', icon: Icons.windows },
])
screen.addView(lv);

function updateFrame() {
    if (pb.value < 100) pb.setValue(pb.value + 10);
    else pb.setValue(0);
    screen.draw();
}

//function playAnim() {
setInterval(updateFrame, 1);
setInterval(ui.dispatchTouchEvents, 1);
//}