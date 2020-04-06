Array.prototype.findIndex = function (f) {
    for (var index = 0; index < this.length; index++) {
        var element = this[index];
        if (f(element))
            return index;
    }
    return -1;
};

(function () {
    ui.ViewGroup = function (name) {
        this.Name = name;
        this.Views = new Array();
    }

    ui.ViewGroup.prototype.addView = function (view) {
        if (view.Parent != undefined)
            throw "View already has a parent.";

        var p = this.Parent;
        while (p != undefined) {
            if (p === view)
                throw "The view to be added is a parent of current view group.";
            p = p.Parent;
        }

        view.Parent = this;
        this.Views.push(view);
    }

    ui.ViewGroup.prototype.removeView = function (view) {
        print("Remove " + view.Name + " from " + this.Name);
        var index = this.Views.findIndex(function (v) { return v === view });
        if (index == -1)
            throw "Cannot found view in view group.";
        this.Views.splice(index, 1);
        delete view.Parent;
    }

    ui.ViewGroup.prototype.draw = function () {
        if (this.Views == undefined)
            return;

        for (var index = 0; index < this.Views.length; index++) {
            var view = this.Views[index];
            view.draw();
        }
    }


    ui.Screen = function () {
        ui.init();
    }
    ui.Screen.prototype = new ui.ViewGroup();

    ui.Screen.prototype.draw = function () {
        ui.clear();
        ui.ViewGroup.prototype.draw.call(this);
        ui.update();
    }


    ui.View = function () {
        this.position = { x: 0, y: 0 };
        this.size = { width: 0, height: 0 };
    }

    ui.View.prototype.draw = function () {
        if (this.Parent == undefined)
            throw "View must in a view group.";
    }


    ui.TextView = function () {
        ui.View.call(this);
        this.text = "";
        this.Padding = 0;
        this.autoLineBreak = false;
        this.LineHeight = 16;
    }
    ui.TextView.prototype = new ui.View();

    ui.TextView.prototype.draw = function () {
        ui.View.prototype.draw.call(this);

        ui.drawRectangle(
            this.position.x,
            this.position.y,
            this.position.x + this.size.width,
            this.position.y + this.size.height);

        if (!this.autoLineBreak) {
            ui.drawText(
                this.text,
                this.position.x + this.Padding,
                this.y + this.Padding);
        } else {
            var currentX = 0;
            var str = "";
            for (var index = 0; index < this.text.length; index++) {
                var c = this.text[index];
                var cWidth = ui.measureTextWidth(c);
                if (cWidth + currentX > this.size.width) {
                    currentX = 0;
                    str += '\n';
                }
                str += c;
                currentX += cWidth;
            }
            ui.drawText(
                str,
                this.position.x + this.Padding,
                this.y + this.Padding);
        }
    }
}
)();

var t = 'ESP32 采用 40 nm 工艺制成，\n\
具有最佳的功耗性能、射频性能、\n\
稳定性、通用性和可靠性，适用于各种应用场景和不同功耗需求。\n\n\
乐鑫为用户提供完整的软、硬件资源，\n\
进行 ESP32 硬件设备的开发。\n\
其中，乐鑫的软件开发环境 ESP-IDF \n\
旨在协助用户快速开发物联网 (IoT) 应用，\n\
可满足用户对 Wi-Fi、蓝牙、\n\
低功耗等方面的要求。';

function initUi() {
    var s = new ui.Screen();
    var tv = new ui.TextView();
    tv.text = t;
    tv.autoLineBreak = true;
    tv.size.width = 50;
    tv.size.height = 50;
    s.addView(tv);
    setInterval(function () { s.draw(); }, 100)
}