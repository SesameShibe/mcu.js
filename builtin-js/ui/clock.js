(function () {
    Clock = function () {
        View.call(this);

        this.foreground = 0xFFFF
        this.zeroScaleColor = makeColor(255, 0, 0)
        this.size = { width: 240, height: 240 }
        this.radius = 120;
        this.hourScaleSize = 6
        this.minuteScaleSize = 2
        this.hourRadius = 40
        this.minuteRadius = 60
        this.secondRadius = 80

        this.hour = 0
        this.minute = 0
        this.second = 0

        this.init()
    }
    Clock.prototype = new View();

    Clock.prototype.drawClockPanel = function () {
        var minuteStepRad = 2 * Math.PI / 60;

        gfx.fillCircle(this.bitmap, this.background, this.radius, this.radius, this.radius, 0xF);
        var rad = 0;
        while (rad < ((2 * Math.PI) - (minuteStepRad / 2))) {
            px0 = this.radius + (this.radius * Math.sin(rad));
            py0 = this.radius - (this.radius * Math.cos(rad));

            if (Math.round(rad / minuteStepRad) % 5 == 0) {
                px1 = px0 - (this.hourScaleSize * Math.sin(rad));
                py1 = py0 + (this.hourScaleSize * Math.cos(rad));
            } else {
                px1 = px0 - (this.minuteScaleSize * Math.sin(rad));
                py1 = py0 + (this.minuteScaleSize * Math.cos(rad));
            }

            var scaleColor = this.foreground;
            if (rad == 0) {
                scaleColor = this.zeroScaleColor
                py1 += this.hourScaleSize;
            }
            gfx.drawLine(this.bitmap, scaleColor, px0, py0, px1, py1);

            rad += minuteStepRad;
        }
    }

    Clock.prototype.render = function () {
        this.clear()
        this.drawClockPanel()

        hour = this.hour + (this.minute / 60)
        minute = this.minute
        second = this.second

        hourRad = hour * 2 * Math.PI / 12
        minRad = minute * 2 * Math.PI / 60
        // secRad = second * 2 * Math.PI / 60

        centX = this.location.x + this.radius
        centY = this.location.y + this.radius

        hourX = centX + this.hourRadius * Math.sin(hourRad)
        hourY = centY - this.hourRadius * Math.cos(hourRad)

        minX = centX + this.minuteRadius * Math.sin(minRad)
        minY = centY - this.minuteRadius * Math.cos(minRad)

        // secX = centX + this.secondRadius * Math.sin(secRad)
        // secY = centY - this.secondRadius * Math.cos(secRad)

        gfx.drawLine(this.bitmap, this.foreground, centX, centY, hourX, hourY)
        gfx.drawLine(this.bitmap, this.foreground, centX, centY, minX, minY)
        // drawLine(this.bitmap, this.foreground, centX, centY, secX, secY)
    }

    // Setters
    Clock.prototype.setHour = function (h) {
        this.hour = h
        this.render()
    }

    Clock.prototype.setMinute = function (m) {
        this.minute = m
        this.render()
    }

    Clock.prototype.setSecond = function (s) {
        this.second = s
        this.render()
    }
    // Setters end
}
)();

