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

function min(a, b) {
    return a > b ? b : a;
}

function max(a, b) {
    return a > b ? a : b;
}

(function () {
    ui.makeColor = function (r, g, b) {
        return (((b >> 3) & 0x1F) << 11)
            | (((g >> 2) & 0x3F) << 5)
            | ((r >> 3) & 0x1F);
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
                        break;
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


    // View base class
    // --------------------------------------------------------------------------------
    ui.View = function () {
        this.position = { x: 0, y: 0 };
        this.size = { width: 0, height: 0 };
        this.foreground = 0xFFFF;
        this.background = 0;
        this.border = 0xFFFF;
        this.cornerRadius = 0;

        this.updateRequired = false;
        this.visible = true;

        this.onTouchDown = null;
        this.onTouchUp = null;
        this.onClick = null;
        this.onLongTouched = null;
    }

    ui.View.prototype.draw = function () {
        if (this.parent == undefined)
            throw "A view must in a view group.";
        this.startDraw();
        this.drawImpl();
        this.endDraw();
    }


    // Implementing draw function by overriding this method.
    ui.View.prototype.drawImpl = function () {
        this.drawBackground();
        this.drawBorder();
    }

    ui.View.prototype.drawBackground = function () {
        ui.setPenColor(this.background);
        ui.fillRectangle(
            this.position.x,
            this.position.y,
            this.position.x + this.size.width,
            this.position.y + this.size.height,
            this.cornerRadius);
    }

    ui.View.prototype.drawBorder = function () {
        ui.setPenColor(this.border);
        ui.drawRectangle(
            this.position.x,
            this.position.y,
            this.position.x + this.size.width - 1,
            this.position.y + this.size.height - 1,
            this.cornerRadius);
    }

    ui.View.prototype.setForeground = function (color) {
        this.updateRequired = true;
        this.foreground = color;
    }

    ui.View.prototype.setBackground = function (color) {
        this.updateRequired = true;
        this.background = color;
    }

    ui.View.prototype.setBorder = function (color) {
        this.updateRequired = true;
        this.border = color;
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

    ui.View.prototype.setCornerRadius = function (cornerRadius) {
        this.updateRequired = true;
        this.cornerRadius = cornerRadius;
    }

    ui.View.prototype.setVisible = function (visible) {
        this.visible = visible;
        this.updateRequired = true;
    }

    ui.View.prototype.setParent = function (viewGroup) {
        this.parent = viewGroup;
    }

    ui.View.prototype.getRenderBounding = function () {
        if (this.parent == undefined)
            return { left: 0, top: 0, right: 0, bottom: 0 };

        var l = max(this.parent.position.x, this.position.x);
        var t = max(this.parent.position.y, this.position.y);
        var r = min(this.parent.position.x + this.parent.size.width, this.position.x + this.size.width);
        var b = min(this.parent.position.y + this.parent.size.height, this.position.y + this.size.height);

        return { left: l, top: t, right: r, bottom: b };
    }

    ui.View.prototype.startDraw = function () {
    }

    ui.View.prototype.endDraw = function () {
        this.updateRequired = false;
        ui.RenderQueue.push(this);

        if (this.parent != undefined)
            ui.setRenderBounding(this.parent.position.x,
                this.parent.position.y,
                this.parent.position.x + this.parent.size.width,
                this.parent.position.x + this.parent.size.height)
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
    // --------------------------------------------------------------------------------


    // --------------------------------------------------------------------------------
    ui.ViewGroup = function () {
        ui.View.call(this);
        this.Views = new Array();
        this.position = { x: 0, y: 0 };
        this.size = { width: 0, height: 0 };
    }
    ui.ViewGroup.prototype = new ui.View();

    ui.ViewGroup.prototype.addView = function (view) {
        if (view.parent != undefined)
            throw "View already has a Parent.";

        var p = this.parent;
        while (p != undefined) {
            if (p === view)
                throw "The view to be added is a Parent of current view group.";
            p = p.parent;
        }

        view.setParent(this);
        this.Views.push(view);
    }

    ui.ViewGroup.prototype.addViewRelativly = function (view) {
        view.position.x += this.position.x;
        view.position.y += this.position.y;
        this.addView(view)
    }

    ui.ViewGroup.prototype.removeView = function (view) {
        var index = this.Views.findIndex(function (v) { return v === view });
        if (index == -1)
            throw "Cannot found view in view group.";
        this.Views.splice(index, 1);
        delete view.parent;
    }

    ui.ViewGroup.prototype.setPos = function (x, y) {
        var diffX = x - this.position.x;
        var diffY = y - this.position.y;
        ui.View.prototype.setPos.call(this);

        if (this.Views == undefined)
            return;

        for (var index = 0; index < this.Views.length; index++) {
            var view = this.Views[index];
            view.setPos(view.position.x + diffX, view.position.y + diffY);
        }
    }

    ui.ViewGroup.prototype.draw = function () {
        ui.View.prototype.drawImpl.call(this);

        if (this.Views == undefined)
            return;

        for (var index = 0; index < this.Views.length; index++) {
            var view = this.Views[index];
            if (view.visible) {
                view.draw();
            }
        }
    }
    // --------------------------------------------------------------------------------


    // --------------------------------------------------------------------------------
    ui.Screen = function () {

    }
    ui.Screen.prototype = new ui.ViewGroup();

    ui.Screen.prototype.draw = function () {
        ui.clear();
        ui.RenderQueue = new Array();
        ui.ViewGroup.prototype.draw.call(this);
        ui.update();
    }
    // --------------------------------------------------------------------------------


    // --------------------------------------------------------------------------------
    ui.ScrollLayout = function () {
        ui.ViewGroup.call(this);

        this.scrollX = 0;
        this.scrollY = 0;
        this.scrollHorizonal = false;
        this.scrollVertical = true;
    }
    ui.ScrollLayout.prototype = new ui.ViewGroup();

    ui.ScrollLayout.prototype.draw = function () {
        ui.RenderQueue.push(this);
        ui.ViewGroup.prototype.draw.call(this);
    }

    ui.ScrollLayout.prototype.touchDown = function (point) {

    }

    ui.ScrollLayout.prototype.touchUp = function (point) {

    }

    ui.ScrollLayout.prototype.touchMoved = function (oldPoint, newPoint) {
        if (this.Views == undefined)
            return;


        var diffX = 0;
        if (this.scrollHorizonal) {
            diffX = oldPoint.x - newPoint.x;
        }
        var diffY = 0;
        if (this.scrollVertical) {
            diffY = oldPoint.y - newPoint.y;
        }

        for (var index = 0; index < this.Views.length; index++) {
            var view = this.Views[index];

            if (view.setPos != undefined) {
                view.setPos(view.position.x - diffX, view.position.y - diffY);
            }
        }
    }

    ui.ScrollLayout.prototype.longTouched = function (point) {

    }
    // --------------------------------------------------------------------------------


    // TextView
    // --------------------------------------------------------------------------------
    ui.TextView = function () {
        ui.View.call(this);
        this.text = "";
        this.padding = 4;
        this.cornerRadius = 0;
        this.autoLineBreak = false;
        this.LineHeight = 16;
        this.textScrollX = 0;
        this.textScrollY = 0;
        this.scrollHorizonal = false;
        this.scrollVertical = true;
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

    ui.TextView.prototype.setPadding = function (padding) {
        this.updateRequired = true;
        this.padding = padding;
    }

    ui.TextView.prototype.drawImpl = function () {
        this.drawBackground();

        var bounding = this.getRenderBounding();
        ui.setPenColor(this.foreground);
        ui.setRenderBounding(
            bounding.left,
            bounding.top,
            bounding.right,
            bounding.bottom);
        if (!this.autoLineBreak) {
            ui.drawText(
                this.text,
                this.position.x + this.padding - this.textScrollX,
                this.position.y + this.padding - this.textScrollY);
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
                this.position.y + this.padding - this.textScrollY);
        }

        this.drawBorder();
    }

    ui.TextView.prototype.touchDown = function (point) {
        ui.View.prototype.touchDown.call(this, point);
    }

    ui.TextView.prototype.touchUp = function (point) {
        ui.View.prototype.touchUp.call(this, point);
    }

    ui.TextView.prototype.touchMoved = function (oldPoint, newPoint) {
        if (this.scrollHorizonal) {
            this.textScrollX += oldPoint.x - newPoint.x;
        }

        if (this.scrollVertical) {
            this.textScrollY += oldPoint.y - newPoint.y;
        }

        ui.View.prototype.touchMoved.call(this, oldPoint, newPoint);
    }
    // --------------------------------------------------------------------------------


    // Button
    // --------------------------------------------------------------------------------
    ui.Button = function () {
        ui.TextView.call(this);

        this.pressedColor = ui.makeColor(255, 255, 255);
        this.releasedColor = ui.makeColor(0, 0, 0);

        this.setBackground(this.releasedColor);
    }
    ui.Button.prototype = new ui.TextView();

    ui.Button.prototype.drawImpl = function () {
        var centerX = 0 - ((this.size.width - ui.measureTextWidth(this.text)) / 2);
        var centerY = 0 - ((this.size.height - 16) / 2);
        this.setTextScroll(centerX, centerY);
        ui.TextView.prototype.drawImpl.call(this);
    }

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
    // --------------------------------------------------------------------------------


    // --------------------------------------------------------------------------------
    ui.TextBox = function () {
        ui.TextView.call(this);

        this.cursor = 0;
    }
    ui.TextBox.prototype = new ui.TextView();

    ui.TextBox.prototype.insert = function (s) {
        this.updateRequired = true;
        var head = this.text.substring(0, this.cursor);
        var tail = this.text.substring(this.cursor, this.text.length);

        this.text = head + s + tail;
        this.cursor = this.cursor + s.length;
    }

    ui.TextBox.prototype.backspace = function () {
        var head = this.text.substring(0, this.cursor);
        var tail = this.text.substring(this.cursor, this.text.length);

        this.setText(head.substring(0, head.length - 1) + tail);
        this.setCursor(this.cursor - 1);
    }

    ui.TextBox.prototype.clear = function () {
        this.text = "";
        this.cursor = 0;
    }

    ui.TextBox.prototype.setText = function (text) {
        this.updateRequired = true;
        this.insert(text)
    }

    ui.TextBox.prototype.setCursor = function (index) {
        this.cursor = index;
    }

    ui.TextBox.prototype.touchDown = function (point) {
        // for (var i = 0; i < this.text.length; i++) {
        //     var bbox = this.getBboxAtIndex(i);
        //     if (point.x >= bbox.x
        //         && point.x <= bbox.x + bbox.width
        //         && point.y >= bbox.y
        //         && point.y <= bbox.y + bbox.height) {
        //         this.setCursor(i);
        //         break;
        //     }
        // }

        ui.View.prototype.touchDown.call(this, point);
    }

    ui.TextBox.prototype.getBboxAtIndex = function (i) {
        var bbox = {
            x: this.position.x + this.padding - this.textScrollX,
            y: this.position.y + this.padding - this.textScrollY,
            width: 0,
            height: 0
        };

        if (i > this.text.length)
            return bbox;

        var c = this.text[i];
        bbox.width = ui.measureTextWidth(c);
        bbox.height = 16;

        for (var j = 0; j < i; j++) {
            var c = this.text[j];

            if (c == '\n') {
                bbox.x = this.position.x + this.padding - this.textScrollX;
                bbox.y += 16;
                continue;
            }

            var cWidth = ui.measureTextWidth(c);
            bbox.x += cWidth;
            if ((this.autoLineBreak) && (bbox.x > (this.size.width - this.padding))) {
                bbox.x = this.position.x + this.padding - this.textScrollX;
                bbox.y += 16;
            }
        }


        return bbox;
    }

    ui.TextBox.prototype.drawImpl = function () {
        ui.TextView.prototype.drawImpl.call(this);

        var bbox = this.getBboxAtIndex(this.cursor);
        ui.setPenColor(0xFFFF);
        ui.drawLine(bbox.x, bbox.y,
            bbox.x, bbox.y + bbox.height);
    }
    // --------------------------------------------------------------------------------



    // --------------------------------------------------------------------------------
    ui.ProgressBar = function () {
        ui.View.call(this);

        this.min = 0;
        this.max = 100;
        this.value = 0;
        this.progressColor = ui.makeColor(51, 178, 255);
    }
    ui.ProgressBar.prototype = new ui.View();

    ui.ProgressBar.prototype.setMin = function (min) {
        this.min = min;
    }

    ui.ProgressBar.prototype.setMax = function (max) {
        this.max = max;
    }

    ui.ProgressBar.prototype.setValue = function (value) {
        this.value = value;
    }

    ui.ProgressBar.prototype.setProgressColor = function (color) {
        this.progressColor = color;
    }

    ui.ProgressBar.prototype.getPercentage = function () {
        return this.value / (this.max - this.min);
    }

    ui.ProgressBar.prototype.drawImpl = function () {
        this.drawBackground();
        this.drawProgress();
        this.drawBorder();
    }

    ui.ProgressBar.prototype.drawProgress = function () {
        ui.setPenColor(this.progressColor);
        ui.fillRectangle(
            this.position.x,
            this.position.y,
            this.position.x + (this.size.width * this.getPercentage()),
            this.position.y + this.size.height - 1,
            this.cornerRadius
        )
    }
    // --------------------------------------------------------------------------------



    // --------------------------------------------------------------------------------
    ui.ListView = function () {
        ui.ScrollLayout.call(this);

        this.itemHeight = 32;

        this.itemSource = null;
        this.buildItem = null;

        this.onItemClicked = null;
    }
    ui.ListView.prototype = new ui.ScrollLayout();

    ui.ListView.prototype.build = function () {
        var itemX = this.position.x;
        var itemY = this.position.y;
        for (var i = 0; i < this.itemSource.length; i++) {
            var item = this.buildItemInternal(this.itemSource[i]);

            item.position.x = itemX;
            item.position.y = itemY;

            this.addView(item);
            itemY += this.itemHeight;
        }
    }

    ui.ListView.prototype.buildItemInternal = function (data) {
        var item = this.buildItem(this.itemSource[i]);
        if (typeof item == 'ListViewItem') {
            item.size.width = this.size.width;
            item.size.height = this.itemHeight;
            return item;
        } else {
            throw 'The buildItem callback must returns a ListViewItem object.';
        }
    }

    ui.ListView.prototype.setItemSource = function (dataCollection) {
        this.itemSource = dataCollection;
        this.build();
    }

    ui.ListView.prototype.setItemHeight = function (height) {
        this.itemHeight = height;
    }


    ui.ListViewItem = function () {
        ui.ViewGroup.call(this);

        this.pressedColor = ui.makeColor(255, 255, 255);
        this.releasedColor = ui.makeColor(0, 0, 0);

        this.setBackground(this.releasedColor);
    }
    ui.ListViewItem.prototype = new ui.ViewGroup();

    ui.ListViewItem.prototype.touchDown = function (point) {
        ui.ViewGroup.prototype.touchDown.call(this, point);
        this.setBackground(this.pressedColor);
    }

    ui.ListViewItem.prototype.touchUp = function (point) {
        ui.ViewGroup.prototype.touchUp.call(this, point);
        this.setBackground(this.releasedColor);

        if (this.parent.onItemClicked != null && isFunction(this.parent.onItemClicked)) {
            this.parent.onItemClicked(this, point);
        }
    }

    ui.ListViewItem.prototype.setPressedColor = function (color) {
        this.pressedColor = color;
    }

    ui.ListViewItem.prototype.setReleasedColor = function (color) {
        this.releasedColor = color;
    }

    ui.ListViewItem.prototype.setParent = function (viewGroup) {
        if (typeof viewGroup == 'ListView') {
            ui.View.prototype.setParent.call(this, viewGroup);
        } else {
            throw 'A ListViewItem object can only being added to a ListView object.'
        }
    }
    // --------------------------------------------------------------------------------
}
)();