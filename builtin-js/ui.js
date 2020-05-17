Array.prototype.findIndex = function (f) {
    for (var index = 0; index < this.length; index++) {
        var element = this[index];
        if (f(element))
            return index;
    }
    return -1;
};

(function () {
    ui.makeColor = function (r, g, b) {
        return (((r >> 3) & 0x1F) << 11)
            | (((g >> 2) & 0x3F) << 5)
            | ((b >> 3) & 0x1F);
    }

    ui.ViewGroup = function () {
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


    // View base class
    ui.View = function () {
        this.position = { x: 0, y: 0 };
        this.size = { width: 0, height: 0 };
        this.foreground = 0xFFFF;
        this.background = 0;
    }

    ui.View.prototype.draw = function () {
        if (this.Parent == undefined)
            throw "View must in a view group.";
        this.startDraw();
    }

    ui.View.prototype.setForeground = function (color) {
        this.foreground = color;
    }

    ui.View.prototype.setBackground = function (color) {
        this.background = color;
    }

    ui.View.prototype.setSize = function (w, h) {
        this.size.width = w;
        this.size.height = h;
    }

    ui.View.prototype.setPos = function (x, y) {
        this.position.x = x;
        this.position.y = y;
    }

    ui.View.prototype.startDraw = function () {
        ui.setPenColor(this.foreground);
    }


    ui.TextView = function () {
        ui.View.call(this);
        this.text = "";
        this.padding = 4;
        this.cornerRadius = 0;
        this.autoLineBreak = false;
        this.LineHeight = 16;
        this.textScrollX = 0;
        this.textScrollY = 0;
    }
    ui.TextView.prototype = new ui.View();

    ui.TextView.prototype.setTextScroll = function (posX, posY) {
        this.textScrollX = posX;
        this.textScrollY = posY;
    }

    ui.TextView.prototype.setCornerRadius = function (cornerRadius) {
        this.cornerRadius = cornerRadius;
    }

    ui.TextView.prototype.setPadding = function (padding) {
        this.padding = padding;
    }

    ui.TextView.prototype.draw = function () {
        ui.View.prototype.draw.call(this);

        ui.setPenColor(this.background);
        ui.fillRectangle(
            this.position.x,
            this.position.y,
            this.position.x + this.size.width,
            this.position.y + this.size.height,
            this.cornerRadius);

        ui.setPenColor(this.foreground);
        if (!this.autoLineBreak) {
            ui.drawText(
                this.text,
                this.position.x + this.padding - this.textScrollX,
                this.position.y + this.padding - this.textScrollY,
                this.position.x + this.padding,
                this.position.y + this.padding,
                this.position.x + this.size.width - this.padding,
                this.position.y + this.size.height - this.padding);
        } else {
            var currentX = 0;
            var str = "";
            for (var index = 0; index < this.text.length; index++) {
                var c = this.text[index];
                var cWidth = ui.measureTextWidth(c);
                if (cWidth + currentX > (this.size.width - this.padding)) {
                    currentX = 0;
                    str += '\n';
                }
                str += c;
                currentX += cWidth;
            }
            ui.drawText(
                str,
                this.position.x + this.padding - this.textScrollX,
                this.position.y + this.padding - this.textScrollY,
                this.position.x + this.padding,
                this.position.y + this.padding,
                this.position.x + this.size.width - this.padding,
                this.position.y + this.size.height - this.padding);
        }
    }
}
)();