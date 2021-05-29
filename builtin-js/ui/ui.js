Array.prototype.findIndex = function (f) {
    for (var index = 0; index < this.length; index++) {
        var element = this[index];
        if (f(element))
            return index;
    }
    return -1;
};

function makeColor(r, g, b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
}

function parseSize(val) {
    return { width: val & 0xFFFF, height: (val >> 16) & 0xFFFF }
}

function saveBitmap(bmp) {
    uart.writeByte(0, 0xFB);
    uart.writeBuf(0, bmp);
}

(function () {

    View = function () {
        this.background = makeColor(0x80, 0x80, 0x80)
        this.bitmap = null
        this.children = new Array()
        this.cornerRadius = 0
        this.foreground = 0xFFFF
        this.location = { x: 0, y: 0 }
        this.parent = null
        this.size = { width: 0, height: 0 }
        this.transparentColor = 0
    }

    View.prototype.addView = function (view) {
        var p = this.parent
        while (p != undefined) {
            if (p === view)
                throw "Cannot add parent to child."
            p = p.parent
        }

        view.parent = this
        this.children.push(view)
        view.init()
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
        if (typeof (this.background) == 'string' && this.background.toLowerCase() == 'transparent') {
            gfx.fillRectangle(this.bitmap, this.transparentColor,
                0, 0, this.size.width, this.size.height,
                this.cornerRadius)
        } else {
            gfx.fillRectangle(this.bitmap, this.background,
                0, 0, this.size.width, this.size.height,
                this.cornerRadius)
        }
    }

    View.prototype.drawBorder = function () {
        gfx.drawRectangle(this.bitmap, this.foreground,
            0, 0, this.size.width - 1, this.size.height - 1,
            this.cornerRadius);
    }

    View.prototype.getBottom = function () {
        return this.getTop() + this.size.height
    }

    View.prototype.getLeft = function () {
        if (this.parent == null) {
            return this.location.x
        } else {
            return this.parent.getLeft() + this.location.x
        }
    }

    View.prototype.getRight = function () {
        return this.getLeft() + this.size.width
    }

    View.prototype.getTop = function () {
        if (this.parent == null) {
            return this.location.y
        } else {
            return this.parent.getTop() + this.location.y
        }
    }

    View.prototype.init = function () {
        if (this.size.width == 0 || this.size.height == 0)
            return

        this.bitmap = gfx.newBitmap(this.size.width, this.size.height)
        this.render();
    }

    View.prototype.rect = function () {
        return {
            left: this.getLeft(),
            top: this.getTop(),
            right: this.getRight(),
            bottom: this.getBottom()
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

    // Touch Events
    View.prototype.longTouched = function (point) {
        if (this.onLongTouched != null) {
            var args = {
                point: point,
                relativePoint: { x: point.x - this.getLeft(), y: point.y - this.getTop() }
            }
            this.onLongTouched(args)
        }
    }

    View.prototype.touchDown = function (point) {
        if (this.onTouchDown != null) {
            var args = {
                point: point,
                relativePoint: { x: point.x - this.getLeft(), y: point.y - this.getTop() }
            }
            this.onTouchDown(args)
        }
    }

    View.prototype.touchMoved = function (oldPoint, newPoint) {
        if (this.onTouchMoved != null) {
            var args = {
                oldPoint: oldPoint,
                relativeOldPoint: { x: oldPoint.x - this.getLeft(), y: oldPoint.y - this.getTop() },
                newPoint: newPoint,
                relativeNewPoint: { x: newPoint.x - this.getLeft(), y: newPoint.y - this.getTop() }
            }
            this.onTouchMoved(args)
        }
    }

    View.prototype.touchUp = function (point) {
        if (this.onTouchUp != null) {
            var args = {
                point: point,
                relativePoint: { x: point.x - this.getLeft(), y: point.y - this.getTop() }
            }
            this.onTouchUp(args)
        }
    }
    // Touch Events End

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
        if (this.size.width != width || this.size.height != height) {
            this.clear();
            this.size = { width: width, height: height };

            if (this.bitmap != null) {
                gfx.releaseBitmap(this.bitmap)
            }

            this.init()
        }
    }
    // Setters End
})();

(function () {
    Button = function (text) {
        View.call(this)

        this.pressed = false
        this.pressedColor = 0xFE52
        this.releasedColor = 0x3436
        this.onClicked = null

        this.setSize(96, 32)
        this.setText(text)
    }
    Button.prototype = new View()

    Button.prototype.render = function () {
        this.clear()
        if (this.pressed) {
            gfx.fillRectangle(this.bitmap, this.pressedColor,
                0, 0, this.size.width, this.size.height,
                this.cornerRadius);
        } else {
            gfx.fillRectangle(this.bitmap, this.releasedColor,
                0, 0, this.size.width, this.size.height,
                this.cornerRadius);
        }
    }

    Button.prototype.setSize = function (width, height) {
        View.prototype.setSize.call(this, width, height)
        if (this.label != null) {
            this.label.setLocation(
                (this.size.width - this.label.size.width) / 2,
                (this.size.height - this.label.size.height) / 2
            )
        }
    }

    Button.prototype.setText = function (text) {
        if (this.label == undefined || this.label == null) {
            this.label = new Label(text)
            this.label.touchTransparent = true
            this.label.setBackground('transparent')
            this.addView(this.label)
        }

        this.label.setText(text)
        this.label.setLocation(
            (this.size.width - this.label.size.width) / 2,
            (this.size.height - this.label.size.height) / 2
        )
    }

    Button.prototype.touchDown = function (point) {
        this.pressed = true
        View.prototype.touchDown.call(this, point)
    }

    Button.prototype.touchUp = function (point) {
        this.pressed = false
        View.prototype.touchUp.call(this, point)

        if (this.onClicked != null) {
            this.onClicked(point)
        }
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
    Label = function (text) {
        View.call(this)

        this.setText(text)
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

    Label.prototype.setText = function (text) {
        this.text = text
        var s = parseSize(gfx.measureText(this.text))
        this.setSize(s.width, s.height)
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
        this.lastTouchedPoint = null
        this.lastTouchedView = null
        this.pressCounter = 0
    }

    UIManager.prototype.disposeView = function (view) {
        if (view.parent != null) {
            view.parent.removeView(view)
        }
        view.dispose()
        delete view
    }

    UIManager.prototype.drawView = function (view, x, y) {
        view.render()
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
        var mngr = this
        setInterval(function () {
            if (mngr.readTouch()) {
                mngr.draw()
            }
        }, 10)
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
                    var rect = v.rect();
                    if ((point.x > rect.left)
                        && (point.x < rect.right)
                        && (point.y > rect.top)
                        && (point.y < rect.bottom)
                        && (!v.touchTransparent)) {
                        v.touchDown(point);

                        this.lastTouchedView = v;
                        this.lastTouchedPoint = point;
                        return true;
                    }
                }
            } else if (Math.abs(this.lastTouchedPoint.x - point.x) < 5
                && Math.abs(this.lastTouchedPoint.y - point.y) < 5) {
                // Long touch
                this.pressCounter++
                if (this.pressCounter > 5) {
                    this.lastTouchedView.longTouched(this.lastTouchedPoint);
                    this.pressCounter = 0
                }
                return true;
            } else {
                // Touch moved
                this.lastTouchedView.touchMoved(this.lastTouchedPoint, point);
                this.lastTouchedPoint = point;
                return true;
            }
        } else {
            // Touch up
            if (this.lastTouchedPoint != null
                && this.lastTouchedView != null) {
                this.lastTouchedView.touchUp(this.lastTouchedPoint);
                this.lastTouchedPoint = null;
                this.lastTouchedView = null;
                return true;
            }
        }

        return false;
    }
})();