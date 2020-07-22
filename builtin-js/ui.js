Array.prototype.findIndex = function (f) {
    for (var index = 0; index < this.length; index++) {
        var element = this[index];
        if (f(element))
            return index;
    }
    return -1;
};

function isFunction(functionToCheck) {
    return functionToCheck && {}.toString.call(functionToCheck) === '[object Function]';
}

(function () {
    ui.makeColor = function (r, g, b) {
        return (((r >> 3) & 0x1F) << 11)
            | (((g >> 2) & 0x3F) << 5)
            | ((b >> 3) & 0x1F);
    }

    ui.RenderQueue = new Array();
    ui.LastTouchedView = null;
    ui.LastTouchedPoint = null;

    ui.dispatchTouchEvents = function () {
        if (!touch.isInited) touch.init();

        var touched = touch.isTouched();
        if (touched) {
            var point = touch.getPoint(0);

            if (ui.LastTouchedPoint == null && ui.LastTouchedView == null) {
                // Touch down
                for (var i = ui.RenderQueue.length - 1; i >= 0; i--) {
                    var v = ui.RenderQueue[i];
                    if ((point.x > v.position.x)
                        && (point.x < (v.position.x + v.size.width))
                        && (point.y > v.position.y)
                        && point.y < (v.position.y + v.size.height)) {
                        v.touchDown(point);

                        ui.LastTouchedView = v;
                        ui.LastTouchedPoint = point;
                    }
                }
            } else if (ui.LastTouchedPoint.x == point.x
                && ui.LastTouchedPoint.y == point.y) {
                // Long touch
                ui.LastTouchedView.longTouched(ui.LastTouchedPoint);
            } else {
                // Touch moved
                ui.LastTouchedView.touchMoved(ui.LastTouchedPoint, point);
                ui.LastTouchedPoint = point;
            }
        } else {
            // Touch up
            if (ui.LastTouchedPoint != null
                && ui.LastTouchedView != null) {
                ui.LastTouchedView.touchUp(ui.LastTouchedPoint);
                ui.LastTouchedPoint = null;
                ui.LastTouchedView = null;
            }
        }
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

    }
    ui.Screen.prototype = new ui.ViewGroup();

    ui.Screen.prototype.draw = function () {
        ui.clear();
        ui.RenderQueue = new Array();
        ui.ViewGroup.prototype.draw.call(this);
        ui.update();
    }


    ui.ScrollLayout = function () {
        ui.ViewGroup.call(this);

        this.position = { x: 0, y: 0 };
        this.size = { width: 240, height: 240 };
        this.scrollX = 0;
        this.scrollY = 0;
    }
    ui.ScrollLayout.prototype = new ui.ViewGroup();

    ui.ScrollLayout.prototype.draw = function () {
        ui.ViewGroup.prototype.draw.call(this);
        ui.RenderQueue.push(this);
    }

    ui.ScrollLayout.prototype.touchDown = function (point) {

    }

    ui.ScrollLayout.prototype.touchUp = function (point) {

    }

    ui.ScrollLayout.prototype.touchMoved = function (oldPoint, newPoint) {
        if (this.Views == undefined)
            return;

        var diffX = oldPoint.x - newPoint.x;
        var diffY = oldPoint.y - newPoint.y;

        for (var index = 0; index < this.Views.length; index++) {
            var view = this.Views[index];

            if (view.setPos != undefined) {
                view.setPos(view.position.x - diffX, view.position.y - diffY);
            }
        }
    }

    ui.ScrollLayout.prototype.longTouched = function (point) {

    }


    // View base class
    ui.View = function () {
        this.position = { x: 0, y: 0 };
        this.size = { width: 0, height: 0 };
        this.foreground = 0xFFFF;
        this.background = 0;
        this.updateRequired = false;

        this.onTouchDown = null;
        this.onTouchUp = null;
        this.onClick = null;
        this.onLongTouched = null;
    }

    ui.View.prototype.draw = function () {
        if (this.Parent == undefined)
            throw "A view must in a view group.";
        this.startDraw();
    }

    ui.View.prototype.setForeground = function (color) {
        this.updateRequired = true;
        this.foreground = color;
    }

    ui.View.prototype.setBackground = function (color) {
        this.updateRequired = true;
        this.background = color;
    }

    ui.View.prototype.setSize = function (w, h) {
        this.updateRequired = true;
        this.size.width = w;
        this.size.height = h;
    }

    ui.View.prototype.setPos = function (x, y) {
        this.updateRequired = true;
        this.position.x = x;
        this.position.y = y;
    }

    ui.View.prototype.startDraw = function () {
    }

    ui.View.prototype.endDraw = function () {
        this.updateRequired = false;
        ui.RenderQueue.push(this);
    }

    ui.View.prototype.touchDown = function (point) {
        this.updateRequired = true;
        if (this.onTouchDown != null && isFunction(this.onTouchDown))
            this.onTouchDown(point)
    }

    ui.View.prototype.touchUp = function (point) {
        this.updateRequired = true;
        if (this.onTouchUp != null && isFunction(this.onTouchUp))
            this.onTouchUp(point);
    }

    ui.View.prototype.touchMoved = function (oldPoint, newPoint) {
        this.updateRequired = true;
        if (this.onTouchMoved != null && isFunction(this.onTouchMoved))
            this.onTouchMoved(oldPoint, newPoint);
    }

    ui.View.prototype.longTouched = function (point) {
        this.updateRequired = true;
        if (this.onLongTouched != null && isFunction(this.onLongTouched))
            this.onLongTouched(point);
    }


    // TextView
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

    ui.TextView.prototype.setText = function (text) {
        this.text = text;
        this.updateRequired = true;
    }

    ui.TextView.prototype.setTextScroll = function (posX, posY) {
        this.updateRequired = true;
        this.textScrollX = posX;
        this.textScrollY = posY;
    }

    ui.TextView.prototype.setCornerRadius = function (cornerRadius) {
        this.updateRequired = true;
        this.cornerRadius = cornerRadius;
    }

    ui.TextView.prototype.setPadding = function (padding) {
        this.updateRequired = true;
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

        this.endDraw();
    }

    ui.TextView.prototype.touchDown = function (point) {
        ui.View.prototype.touchDown.call(this, point);
    }

    ui.TextView.prototype.touchUp = function (point) {
        ui.View.prototype.touchUp.call(this, point);
    }

    ui.TextView.prototype.touchMoved = function (oldPoint, newPoint) {
        this.textScrollX += oldPoint.x - newPoint.x;
        this.textScrollY += oldPoint.y - newPoint.y;

        ui.View.prototype.touchMoved.call(this, oldPoint, newPoint);
    }


    // Button
    ui.Button = function () {
        ui.TextView.call(this);

        this.pressedColor = ui.makeColor(255, 255, 255);
        this.releasedColor = ui.makeColor(0, 0, 0);

        this.setBackground(this.releasedColor);
    }
    ui.Button.prototype = new ui.TextView();

    ui.Button.prototype.touchDown = function (point) {
        this.setBackground(this.pressedColor);
        ui.TextView.prototype.touchDown.call(this, point);
    }

    ui.Button.prototype.touchUp = function (point) {
        this.setBackground(this.releasedColor);
        ui.TextView.prototype.touchUp.call(this, point);

        if (this.onClick != null && isFunction(this.onClick)) {
            this.onClick();
        }
    }

    ui.Button.prototype.touchMoved = function (oldPoint, newPoint) {
        ui.View.prototype.touchMoved.call(this, oldPoint, newPoint);
    }

    ui.Button.prototype.setPressedColor = function (color) {
        this.pressedColor = color;
    }

    ui.Button.prototype.setReleasedColor = function (color) {
        this.releasedColor = color;
    }

    ui.TextBox = function () {
        ui.TextView.call(this);

        this.cursor = 0;
    }
    ui.TextBox.prototype = new ui.TextView();

    ui.TextBox.prototype.insertChar = function (c) {
        var head = this.text.substring(0, this.cursor);
        var tail = this.text.substring(this.cursor, this.text.length);

        this.setText(head + c + tail);
        this.setCursor(this.cursor + 1);
    }

    ui.TextBox.prototype.backspace = function () {
        var head = this.text.substring(0, this.cursor);
        var tail = this.text.substring(this.cursor, this.text.length);

        this.setText(head.substring(0, head.length - 1) + tail);
        this.setCursor(this.cursor - 1);
    }

    ui.TextBox.prototype.setCursor = function (index) {
        this.cursor = index;
    }

    ui.TextBox.prototype.touchDown = function (point) {
        ui.View.prototype.touchDown.call(this, point);
    }
}
)();