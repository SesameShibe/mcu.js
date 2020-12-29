(function () {
    ui.Clock = function () {
        ui.View.call(this);

        this.foreground = 0xFFFF
        this.size = { width: 240, height: 240 }
        this.radius = 120;
        this.hourScaleSize = 6
        this.minuteScaleSize = 2
        this.hourRadius = 20
        this.minuteRadius = 40
        this.secondRadius = 60
        this.initClockPanel();
    }
    ui.Clock.prototype = new ui.View();

    ui.Clock.prototype.initClockPanel = function () {
        var fb = ui.copyFB(0, 0, 240, 240)

        var minuteStepRad = 2 * Math.PI / 60;
        ui.clear();
        ui.setPenColor(this.foreground);
        ui.setRenderRect(0, 0, 240, 240);
        ui.fillCircle(this.radius, this.radius, 2, 0xF);
        var rad = 0;
        while (rad < 2 * Math.PI) {
            px0 = this.radius + (this.radius * Math.sin(rad));
            py0 = this.radius - (this.radius * Math.cos(rad));

            if (Math.round(rad / minuteStepRad) % 5 == 0) {
                px1 = px0 - (this.hourScaleSize * Math.sin(rad));
                py1 = py0 + (this.hourScaleSize * Math.cos(rad));
            } else {
                px1 = px0 - (this.minuteScaleSize * Math.sin(rad));
                py1 = py0 + (this.minuteScaleSize * Math.cos(rad));
            }
            ui.drawLine(px0, py0, px1, py1);

            rad += minuteStepRad;
        }
        this.panel = ui.copyFB(0, 0, this.radius * 2, this.radius * 2);

        ui.clear()
        ui.setFB(fb)
        ui.freeCopy(fb)
    }

    ui.Clock.prototype.drawImpl = function () {
        var rect = this.getRenderRect();
        ui.setRenderRect(
            rect.left,
            rect.top,
            rect.right,
            rect.bottom);

        ui.putPixels(
            this.position.x,
            this.position.y,
            this.radius * 2,
            this.radius * 2,
            this.panel
        );

        var now = new Date();
        hour = now.getHours();
        minute = now.getMinutes();
        second = now.getSeconds();

        hourRad = hour * 2 * Math.PI / 12
        minRad = minute * 2 * Math.PI / 60
        secRad = second * 2 * Math.PI / 60

        centX = this.position.x + this.radius
        centY = this.position.y + this.radius

        hourX = centX + this.hourRadius * Math.sin(hourRad)
        hourY = centY - this.hourRadius * Math.cos(hourRad)

        minX = centX + this.minuteRadius * Math.sin(minRad)
        minY = centY - this.minuteRadius * Math.cos(minRad)

        secX = centX + this.secondRadius * Math.sin(secRad)
        secY = centY - this.secondRadius * Math.cos(secRad)

        ui.drawLine(centX, centY, hourX, hourY)
        ui.drawLine(centX, centY, minX, minY)
        ui.drawLine(centX, centY, secX, secY)
    }
}
)();

