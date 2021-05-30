var manager = UIManager.Manager

var root = new View()
root.setSize(240, 240)
manager.currentPage = root

var clock = new Clock()
var clockUpdateInterval;
clock.onClicked = function (args) {
    clearInterval(clockUpdateInterval)
    manager.currentPage = root
}

var btn = new Button('Clock Demo')
btn.setSize(236, 64)
btn.setLocation(2, 240 - 64 - 2)
btn.onClicked = function (args) {
    manager.currentPage = clock
    clockUpdateInterval = setInterval(function () {
        var now = new Date()
        clock.hour = now.getHours()
        clock.minute = now.getMinutes()
        clock.second = now.getSeconds()
        manager.requestUpdate()
    }, 60 * 1000)
    manager.requestUpdate()
}

root.addView(btn)
manager.draw()

UIManager.pollScreen()