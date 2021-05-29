manager = new UIManager()

root = new View()
root.setSize(240, 240)
manager.views.push(root)

var btn = new Button('button1')
btn.setSize(236, 64)
btn.setLocation(2, 240 - 64 - 2)
btn.onTouchDown = function (args) {
    print('touch x:'+args.point.x+', y:'+args.point.y)
}
btn.onLongTouched = function (args) {
    print('long touch x:'+args.point.x+', y:'+args.point.y)
}

root.addView(btn)
manager.draw()

manager.pollTouch()