Array.prototype.findIndex = function (f) {
    for (var index = 0; index < this.length; index++) {
        var element = this[index];
        if (f(element))
            return index;
    }
    return -1;
};

function makeColor(r, g, b) {
    return (((b >> 3) & 0x1F) << 11)
        | (((r >> 2) & 0x3F) << 5)
        | ((g >> 3) & 0x1F);
}

function saveBitmap(bmp) {
    uart.writeByte(0, 0xFB);
    uart.writeBuf(0, bmp);
}

(function () {

    View = function () {
        this.background = makeColor(0x80, 0x80, 0x80)
        this.bitmap = null
        this.children = new Array();
        this.cornerRadius = 0;
        this.foreground = 0xFFFF;
        this.location = { x: 0, y: 0 };
        this.parent = null;
        this.size = { width: 0, height: 0 };
        this.transparentColor = 0

        this.init();
    }

    View.prototype.addView = function (view) {
        var p = this.parent;
        while (p != undefined) {
            if (p === view)
                throw "Cannot add parent to child.";
            p = p.parent;
        }

        view.parent = this;
        this.children.push(view);
    }

    View.prototype.clear = function () {
        gfx.fillRectangle(this.bitmap, this.transparentColor,
            0, 0, this.size.width, this.size.height,
            this.cornerRadius);
    }

    View.prototype.dispose = function () {
        gfx.releaseBitmap(this.bitmap);
    }

    View.prototype.drawBackground = function () {
        gfx.fillRectangle(this.bitmap, this.background,
            0, 0, this.size.width, this.size.height,
            this.cornerRadius);
    }

    View.prototype.drawBorder = function () {
        gfx.drawRectangle(this.bitmap, this.foreground,
            0, 0, this.size.width - 1, this.size.height - 1,
            this.cornerRadius);
    }

    View.prototype.init = function () {
        if (this.size.width == 0 || this.size.height == 0)
            return

        this.bitmap = gfx.newBitmap(this.size.width, this.size.height)
        this.render();
    }

    View.prototype.rect = function () {
        return {
            left: this.location.x,
            top: this.location.y,
            right: this.location.x + this.size.width,
            bottom: this.location.y + this.size.height
        }
    }

    View.prototype.render = function () {
        this.clear()
        this.drawBackground()
        this.drawBorder()
    }

    View.prototype.resetTransparentColor = function () {
        if (this.transparentColor == this.background) {
            this.transparentColor++

            if (this.transparentColor == this.foreground) {
                this.transparentColor++
            }
        }
    }

    View.prototype.removeView = function (view) {
        var index = this.children.findIndex(function (v) { return v === view });
        if (index != -1) {
            this.children.splice(index, 1);
        }
    }

    // Setters
    View.prototype.setBackground = function (bg) {
        this.background = bg;
        this.resetTransparentColor();
        this.render();
    }

    View.prototype.setCornerRadius = function (rd) {
        this.cornerRadius = rd;
        this.render();
    }

    View.prototype.setForeground = function (fg) {
        this.foreground = fg;
        this.resetTransparentColor();
        this.render();
    }

    View.prototype.setLocation = function (x, y) {
        this.location = { x: x, y: y };
    }

    View.prototype.setSize = function (width, height) {
        this.clear();
        this.size = { width: width, height: height };
        this.init()
    }
    // Setters end
})();

(function () {
    Label = function () {
        View.call(this)

        this.text = 'label'
    }
    Label.prototype = new View()

    Label.prototype.render = function () {
        this.clear()
        this.drawBackground()

        gfx.drawText(this.bitmap,
            this.foreground,
            this.text,
            0, 0)
    }
})();

(function () {
    Icon = function () {
        View.call(this)

        this.iconId = Icons.question
    }
    Icon.prototype = new View()

    Icon.prototype.render = function () {
        this.clear()
        this.drawBackground();

        gfx.drawIcon(this.bitmap,
            this.foreground,
            this.iconId,
            0, 0)
    }
})();

(function () {
    ProgressBar = function () {
        View.call(this)

        this.min = 0
        this.max = 100
        this.value = 0
        this.progressColor = 0xFD86
    }
    ProgressBar.prototype = new View()

    ProgressBar.prototype.drawProgress = function () {
        gfx.fillRectangle(
            this.bitmap,
            this.progressColor,
            0, 0,
            this.size.width * this.getPercentage(),
            this.size.height - 1,
            this.cornerRadius
        )
    }

    ProgressBar.prototype.render = function () {
        this.clear()
        this.drawBackground()
        this.drawProgress()
        this.drawBorder()
    }
})();

(function () {
    UIManager = function () {
        this.views = new Array()
        this.drawQueue = new Array()
        this.lastTouchedPoint = null;
        this.lastTouchedView = null;
    }

    UIManager.prototype.disposeView = function (view) {
        if (view.parent != null) {
            view.parent.removeView(view)
        }
        view.dispose()
        delete view
    }

    UIManager.prototype.drawView = function (view, x, y) {
        gfx.drawBitmapWithTransparent(
            lcd.getFB(),
            view.bitmap,
            x, y,
            view.transparentColor
        )
        this.drawQueue.push(view)

        for (var i = 0; i < view.children.length; i++) {
            var child = view.children[i];
            this.drawView(child,
                child.location.x + x,
                child.location.y + y)
        }
    }

    UIManager.prototype.draw = function () {
        this.drawQueue = new Array()

        gfx.fillRectangle(lcd.getFB(), 0, 0, 0, 240, 240)
        for (var i = 0; i < this.views.length; i++) {
            var view = this.views[i]
            this.drawView(view, view.location.x, view.location.y)
        }
        lcd.update();
    }

    UIManager.prototype.pollKey = function () {
        while (true) {
            // Should do something here.
        }
    }

    UIManager.prototype.pollTouch = function () {
        while (true) {
            if (this.readTouch()) {
                this.draw()
            }
        }
    }

    UIManager.prototype.readTouch = function () {
        if (!touch.isInited) touch.init();

        var touched = touch.isTouched();
        if (touched) {
            var point = touch.getPoint(0);

            if (this.lastTouchedPoint == null && this.lastTouchedView == null) {
                // Touch down
                for (var i = this.drawQueue.length - 1; i >= 0; i--) {
                    var v = this.drawQueue[i];
                    if ((point.x > v.position.x)
                        && (point.x < (v.position.x + v.size.width))
                        && (point.y > v.position.y)
                        && point.y < (v.position.y + v.size.height)
                        && (!v.touchTransparent)) {
                        //v.touchDown(point);

                        this.lastTouchedView = v;
                        this.lastTouchedPoint = point;
                        return true;
                    }
                }
            } else if (this.lastTouchedPoint.x == point.x
                && this.lastTouchedPoint.y == point.y) {
                // Long touch
                //this.lastTouchedView.longTouched(this.lastTouchedPoint);
                return true;
            } else {
                // Touch moved
                //this.lastTouchedView.touchMoved(this.lastTouchedPoint, point);
                this.lastTouchedPoint = point;
                return true;
            }
        } else {
            // Touch up
            if (this.lastTouchedPoint != null
                && this.lastTouchedView != null) {
                //this.lastTouchedView.touchUp(this.lastTouchedPoint);
                this.lastTouchedPoint = null;
                this.lastTouchedView = null;
                return true;
            }
        }

        return false;
    }
})();