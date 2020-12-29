Array.prototype.findIndex = function (f) {
    for (var index = 0; index < this.length; index++) {
        var element = this[index];
        if (f(element))
            return index;
    }
    return -1;
};

Number.prototype.clamp = function (min, max) {
    return Math.min(Math.max(this, min), max);
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
            | (((r >> 2) & 0x3F) << 5)
            | ((g >> 3) & 0x1F);
    }

    ui.Colors = {
        white: 0xFFFF,
        black: 0,
        red: 0x1F,
        greed: 0x7E0,
        blue: 0xF7D8,
        lightBlue: 0xFD86,
        transparent: ui.COLOR_TRANSPARENT
    }

    ui.BorderStyle = {
        none: 0,
        thin: 1
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
                        && point.y < (v.position.y + v.size.height)
                        && (!v.touchTransparent)) {
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
        this.foreground = ui.Colors.white;
        this.background = 0;
        this.border = ui.Colors.white;
        this.borderStyle = ui.BorderStyle.thin;
        this.cornerRadius = 0;

        this.visible = true;
        this.touchTransparent = false;

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


    ui.View.prototype.drawImpl = function () {
        this.drawBackground();
        this.drawBorder();
    }

    ui.View.prototype.startDraw = function () {
    }

    ui.View.prototype.endDraw = function () {
        ui.RenderQueue.push(this);
        this.restoreRenderRect();
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
        if (this.borderStyle == ui.BorderStyle.none) {
            return;
        } else if (this.borderStyle == ui.BorderStyle.thin) {
            ui.setPenColor(this.border);
            ui.drawRectangle(
                this.position.x,
                this.position.y,
                this.position.x + this.size.width - 1,
                this.position.y + this.size.height - 1,
                this.cornerRadius);
        }
    }

    ui.View.prototype.requestUpdate = function () {
        if (this.parent != undefined) {
            this.parent.requestUpdate();
        }
    }

    ui.View.prototype.setForeground = function (color) {
        this.foreground = color;
        this.requestUpdate();
    }

    ui.View.prototype.setBackground = function (color) {
        this.background = color;
        this.requestUpdate();
    }

    ui.View.prototype.setBorder = function (color) {
        this.border = color;
        this.requestUpdate();
    }

    ui.View.prototype.setBorderStyle = function (style) {
        this.borderStyle = style;
        this.requestUpdate();
    }

    ui.View.prototype.setSize = function (w, h) {
        this.size.width = w;
        this.size.height = h;
        this.requestUpdate();
    }

    ui.View.prototype.setPos = function (x, y) {
        this.position.x = x;
        this.position.y = y;
        this.requestUpdate();
    }

    ui.View.prototype.setCornerRadius = function (cornerRadius) {
        this.cornerRadius = cornerRadius;
        this.requestUpdate();
    }

    ui.View.prototype.setVisible = function (visible) {
        this.visible = visible;
        this.requestUpdate();
    }

    ui.View.prototype.setParent = function (viewGroup) {
        this.parent = viewGroup;
        if (this.size.width == -1) {
            this.size.width = this.parent.width;
        }
        if (this.size.height == -1) {
            this.size.height = this.parent.height;
        }
    }

    ui.View.prototype.getRenderRect = function () {
        if (this.parent == undefined)
            return { left: 0, top: 0, right: 0, bottom: 0 };

        var parentRenderRect = this.parent.getRenderRect();

        var l = max(parentRenderRect.left, this.position.x);
        var t = max(parentRenderRect.top, this.position.y);
        var r = min(parentRenderRect.right, this.position.x + this.size.width);
        var b = min(parentRenderRect.bottom, this.position.y + this.size.height);

        return { left: l, top: t, right: r, bottom: b };
    }

    ui.View.prototype.restoreRenderRect = function () {
        if (this.parent != undefined) {
            var parentRenderRect = this.parent.getRenderRect();
            ui.setRenderRect(parentRenderRect.left,
                parentRenderRect.top,
                parentRenderRect.right,
                parentRenderRect.bottom);
        }
    }

    ui.View.prototype.touchDown = function (point) {
        if (this.onTouchDown != null && isFunction(this.onTouchDown))
            this.onTouchDown(point)
        this.requestUpdate();
    }

    ui.View.prototype.touchUp = function (point) {
        if (this.onTouchUp != null && isFunction(this.onTouchUp))
            this.onTouchUp(point);
        this.requestUpdate();
    }

    ui.View.prototype.touchMoved = function (oldPoint, newPoint) {
        if (this.onTouchMoved != null && isFunction(this.onTouchMoved))
            this.onTouchMoved(oldPoint, newPoint);
        this.requestUpdate();
    }

    ui.View.prototype.longTouched = function (point) {
        if (this.onLongTouched != null && isFunction(this.onLongTouched))
            this.onLongTouched(point);
        this.requestUpdate();
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
        view.setPos(view.position.x + this.position.x, view.position.y + this.position.y);
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
        ui.View.prototype.setPos.call(this, x, y);

        if (this.Views == undefined)
            return;

        for (var index = 0; index < this.Views.length; index++) {
            var view = this.Views[index];
            view.setPos(view.position.x + diffX, view.position.y + diffY);
        }
    }

    ui.ViewGroup.prototype.draw = function () {
        this.startDraw();
        this.drawImpl();
        ui.RenderQueue.push(this);

        if (this.Views == undefined)
            return;

        for (var index = 0; index < this.Views.length; index++) {
            var view = this.Views[index];
            if (view.visible) {
                view.draw();
            }
        }

        this.endDraw();
    }

    ui.ViewGroup.prototype.endDraw = function () {
        this.restoreRenderRect();
    }
    // --------------------------------------------------------------------------------


    // --------------------------------------------------------------------------------
    ui.Screen = function () {
        this.updateRequired = true;
    }
    ui.Screen.prototype = new ui.ViewGroup();

    ui.Screen.prototype.draw = function () {
        if (this.updateRequired) {
            ui.clear();
            ui.RenderQueue = new Array();
            ui.ViewGroup.prototype.draw.call(this);
            ui.update();
            this.updateRequired = false;
        }
    }

    ui.Screen.prototype.getRenderRect = function () {
        return { left: 0, top: 0, right: 240, bottom: 240 };
    }

    ui.Screen.prototype.requestUpdate = function () {
        this.updateRequired = true;
    }
    // --------------------------------------------------------------------------------


    // --------------------------------------------------------------------------------
    ui.ScrollLayout = function () {
        ui.ViewGroup.call(this);

        this.scrollX = 0;
        this.scrollY = 0;
        this.scrollHorizonal = false;
        this.scrollVertical = true;

        this.contentWidth = 0;
        this.contentHeight = 0;
    }
    ui.ScrollLayout.prototype = new ui.ViewGroup();

    ui.ScrollLayout.prototype.addView = function (view) {
        ui.ViewGroup.prototype.addView.call(this, view);

        // Cache content size
        this.refreshContentSize();
    }

    ui.ScrollLayout.prototype.removeView = function (view) {
        ui.ViewGroup.prototype.removeView.call(this, view);

        // Cache content size
        this.refreshContentSize();
    }

    ui.ScrollLayout.prototype.refreshContentSize = function () {
        this.contentWidth = this.calcContentWidth();
        this.contentHeight = this.calcContentHeight();
    }

    ui.ScrollLayout.prototype.calcContentWidth = function () {
        var leftEdge = this.Views[0].position.x;
        var rightEdge = this.Views[0].position.x + this.Views[0].size.width;

        for (var i = 0; i < this.Views.length; i++) {
            var view = this.Views[i];
            if (view.position.x < leftEdge) {
                leftEdge = view.position.x;
            }

            var r = view.position.x + view.size.width;
            if (r > rightEdge) {
                rightEdge = r;
            }
        }

        var contentWidth = rightEdge - leftEdge;
        return contentWidth;
    }

    ui.ScrollLayout.prototype.calcContentHeight = function () {
        var topEdge = this.Views[0].position.y;
        var bottomEdge = this.Views[0].position.y + this.Views[0].size.height;

        for (var i = 0; i < this.Views.length; i++) {
            var view = this.Views[i];
            if (view.position.y < topEdge) {
                topEdge = view.position.y;
            }

            var b = view.position.y + view.size.height;
            if (b > bottomEdge) {
                bottomEdge = b;
            }
        }

        var contentHeight = bottomEdge - topEdge;
        return contentHeight;
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

        this.setScroll(this.scrollX + diffX, this.scrollY + diffY);
    }

    ui.ScrollLayout.prototype.longTouched = function (point) {

    }

    ui.ScrollLayout.prototype.setScroll = function (x, y) {
        var scrollChanged = false
        var diffX = 0
        var diffY = 0
        x = x.clamp(0, this.contentWidth - this.size.width)
        y = y.clamp(0, this.contentHeight - this.size.height)

        if (this.scrollX != x) {
            diffX = x - this.scrollX
            this.scrollX = x
            scrollChanged = true
        }
        if (this.scrollY != y) {
            diffY = y - this.scrollY
            this.scrollY = y
            scrollChanged = true
        }

        if (scrollChanged) {
            for (var index = 0; index < this.Views.length; index++) {
                var view = this.Views[index];

                if (view.setPos != undefined) {
                    view.setPos(view.position.x - diffX, view.position.y - diffY)
                }
            }
        }
    }

    ui.ScrollLayout.prototype.endDraw = function () {
        if (this.scrollVertical)
            this.drawVerticalScrollBar();

        if (this.scrollHorizonal)
            this.drawHorizonalScrollBar()

        ui.ViewGroup.prototype.endDraw.call(this);
    }

    ui.ScrollLayout.prototype.drawVerticalScrollBar = function () {
        var barWidth = 5;
        var barHeight = this.size.height - 1;
        var barLeft = this.position.x + this.size.width - barWidth - 1;
        var barTop = this.position.y;

        ui.setPenColor(this.background)
        ui.fillRectangle(barLeft, barTop, barLeft + barWidth, barTop + barHeight, 1)
        ui.setPenColor(this.foreground)
        ui.drawRectangle(barLeft, barTop, barLeft + barWidth, barTop + barHeight, 1)

        var scrollerHeight = barHeight;
        if (this.size.height < this.contentHeight) {
            scrollerHeight = this.size.height / this.contentHeight * scrollerHeight;
        }
        var scrollerWidth = 3
        var scrollProgress = this.scrollY / this.contentHeight;
        var scrollerTop = scrollProgress * barHeight;
        var scrollerLeft = barLeft + ((barWidth - scrollerWidth) / 2)
        ui.fillRectangle(scrollerLeft, scrollerTop, scrollerLeft + scrollerWidth, scrollerTop + scrollerHeight, 0)
    }

    ui.ScrollLayout.prototype.drawHorizonalScrollBar = function () {

    }
    // --------------------------------------------------------------------------------



    // --------------------------------------------------------------------------------
    ui.ListView = function () {
        ui.ScrollLayout.call(this);

        this.itemHeight = 32;
        this.itemSpacing = 1;

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

            item.setPos(itemX, itemY);

            this.addView(item);
            itemY += this.itemHeight + this.itemSpacing;
        }
    }

    ui.ListView.prototype.addView = function (view) {
        view.index = this.Views.length;
        ui.ScrollLayout.prototype.addView.call(this, view);
    }

    ui.ListView.prototype.buildItemInternal = function (data) {
        var item = new ui.ListViewItem();
        this.buildItem(item, data);
        item.data = data;
        item.setSize(this.size.width, this.itemHeight);
        return item;
    }

    ui.ListView.prototype.setItemSource = function (dataCollection) {
        this.itemSource = dataCollection;
        this.build();
    }

    ui.ListView.prototype.setItemHeight = function (height) {
        this.itemHeight = height;
    }

    ui.ListView.prototype.setItemSpacing = function (spacing) {
        this.itemSpacing = spacing;
    }

    ui.ListViewItem = function () {
        ui.ViewGroup.call(this);

        this.data = null;
        this.index = 0;

        this.pressedColor = ui.Colors.white;
        this.releasedColor = ui.Colors.black;

        this.moved = false;

        this.setBackground(this.releasedColor);
    }
    ui.ListViewItem.prototype = new ui.ViewGroup();

    ui.ListViewItem.prototype.touchDown = function (point) {
        this.setBackground(this.pressedColor);
        ui.ViewGroup.prototype.touchDown.call(this, point);
    }

    ui.ListViewItem.prototype.touchUp = function (point) {
        this.setBackground(this.releasedColor);
        ui.ViewGroup.prototype.touchUp.call(this, point);

        if (this.parent.onItemClicked != null && isFunction(this.parent.onItemClicked) && !this.moved) {
            this.parent.onItemClicked(this.data, { position: point, index: this.index });
        }

        this.moved = false;
    }

    ui.ListViewItem.prototype.touchMoved = function (oldPoint, newPoint) {
        this.parent.touchMoved(oldPoint, newPoint);
        this.moved = true;
    }

    ui.ListViewItem.prototype.setPressedColor = function (color) {
        this.pressedColor = color;
    }

    ui.ListViewItem.prototype.setReleasedColor = function (color) {
        this.releasedColor = color;
    }

    ui.ListViewItem.prototype.setParent = function (viewGroup) {
        if (viewGroup instanceof ui.ListView) {
            ui.View.prototype.setParent.call(this, viewGroup);
        } else {
            throw 'A ListViewItem object can only being added to a ListView object.'
        }
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
        this.scrollVertical = false;
    }
    ui.TextView.prototype = new ui.View();

    ui.TextView.prototype.setText = function (text) {
        this.text = text;
        this.requestUpdate();
    }

    ui.TextView.prototype.setTextScroll = function (posX, posY) {
        this.textScrollX = posX;
        this.textScrollY = posY;
        this.requestUpdate();
    }

    ui.TextView.prototype.setPadding = function (padding) {
        this.padding = padding;
        this.requestUpdate();
    }

    ui.TextView.prototype.drawImpl = function () {
        this.drawBackground();

        var rect = this.getRenderRect();
        ui.setRenderRect(
            rect.left,
            rect.top,
            rect.right,
            rect.bottom);
        ui.setPenColor(this.foreground);
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

        this.pressedColor = ui.Colors.white;
        this.releasedColor = ui.Colors.black;

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
        var head = this.text.substring(0, this.cursor);
        var tail = this.text.substring(this.cursor, this.text.length);

        this.text = head + s + tail;
        this.cursor = this.cursor + s.length;
        this.requestUpdate();
    }

    ui.TextBox.prototype.backspace = function () {
        var head = this.text.substring(0, this.cursor);
        var tail = this.text.substring(this.cursor, this.text.length);

        this.setText(head.substring(0, head.length - 1) + tail);
        this.setCursor(this.cursor - 1);
        this.requestUpdate();
    }

    ui.TextBox.prototype.clear = function () {
        this.text = "";
        this.cursor = 0;
        this.requestUpdate();
    }

    ui.TextBox.prototype.setText = function (text) {
        this.insert(text)
        this.requestUpdate();
    }

    ui.TextBox.prototype.setCursor = function (index) {
        this.cursor = index;
        this.requestUpdate();
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
        ui.setPenColor(ui.Colors.white);
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
        this.progressColor = ui.Colors.lightBlue;
    }
    ui.ProgressBar.prototype = new ui.View();

    ui.ProgressBar.prototype.setMin = function (min) {
        this.min = min;
        this.requestUpdate();
    }

    ui.ProgressBar.prototype.setMax = function (max) {
        this.max = max;
        this.requestUpdate();
    }

    ui.ProgressBar.prototype.setValue = function (value) {
        this.value = value;
        this.requestUpdate();
    }

    ui.ProgressBar.prototype.setProgressColor = function (color) {
        this.progressColor = color;
        this.requestUpdate();
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
    ui.Icon = function (iconId) {
        ui.View.call(this);
        this.setIcon(iconId);
        this.setBorderStyle(ui.BorderStyle.none);
    }
    ui.Icon.prototype = new ui.View();

    ui.Icon.prototype.setIcon = function (iconId) {
        this.iconId = iconId;
    }

    ui.Icon.prototype.drawImpl = function () {
        this.drawBackground();

        ui.setPenColor(this.foreground);
        ui.drawIcon(this.iconId, this.position.x, this.position.y);

        this.drawBorder();
    }
    // --------------------------------------------------------------------------------
}
)();