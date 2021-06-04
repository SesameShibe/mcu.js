const global = this;

(function () {
    var isCalling = false;
    nativePrint = print;

    global.interfacePrint = print;
    global.print = function (data) {
        if (isCalling) {
            return nativePrint(data);
        } else {
            isCalling = true;
            try {
                interfacePrint(data);
            } catch (e) {
                print('Unhandled error: ' + e);
            }
            isCalling = false;
            return;
        }
    };
})();

(function () {
    const MAX_TIMEOUT = 0x40000000;
    const LOOP_PERIOD_IN_MS = 20;
    const sleepMs = os.sleepMs;
    const getTickCountMs = os.getTickCountMs;
    var pendingTimeoutList = [];
    var intervalList = [];
    var periodCount = 0;
    global.setTimeout = function (handler, timeout, arg) {
        timeout = timeout || 0;
        if (timeout > MAX_TIMEOUT) {
            timeout = MAX_TIMEOUT;
        }
        var fireAtTick = getTickCountMs() + timeout;
        pendingTimeoutList.push([fireAtTick, handler, arg]);
    };
    global.setInterval = function (handler, timeout, arg) {
        timeout = timeout || 0;
        var loopCount = ~~(timeout / LOOP_PERIOD_IN_MS);
        if (loopCount <= 0) {
            loopCount = 1;
        }
        for (var id = 0; id < Number.MAX_VALUE; id++) {
            var idExists = false;
            for (var i = 0; i < intervalList.length; i++) {
                var t = intervalList[i];
                if (t.id == id) {
                    idExists = true;
                    break;
                }
            }

            if (!idExists) {
                intervalList.push({ id: id, task: [loopCount, handler, arg] });
                return id;
            }
        }
    };
    global.clearInterval = function (id) {
        for (var i = 0; i < intervalList.length; i++) {
            var t = intervalList[i];
            if (t.id == id) {
                intervalList.splice(i)
                break;
            }
        }
    }
    function handleTimeouts() {
        while (true) {
            var i;
            for (i = 0; i < pendingTimeoutList.length; i++) {
                var diff = pendingTimeoutList[i][0] - getTickCountMs();
                if ((diff < 0) || (diff > MAX_TIMEOUT)) {
                    break;
                }
            }
            if (i >= pendingTimeoutList.length) {
                break;
            }
            var t = pendingTimeoutList[i];
            pendingTimeoutList.splice(i, 1);
            t[1].call(global, t[2]);
        }
    }
    function handleIntervals() {
        for (var i = 0; i < intervalList.length; i++) {
            var t = intervalList[i].task;
            if ((periodCount % t[0]) == 0) {
                t[1].call(global, t[2]);
            }
        }
    }
    global.mainLoop = function () {
        while (true) {
            periodCount++;
            handleTimeouts();
            handleIntervals();
            sleepMs(LOOP_PERIOD_IN_MS);
        }
    };
})();

function boot() {
    fs.mountSpiffs();
    //eval /spi/boot.js
    if (fs.exists('/spi/boot.js')) {
        var fd = fs.open("/spi/boot.js", fs.O_RDONLY);
        var size = fs.lseek(fd, 0, fs.SEEK_END);
        fs.lseek(fd, 0, fs.SEEK_SET)
        var buf = new Buffer(size);
        fs.read(fd, buf, size);
        global.eval(buf.toString());
    }
    mainLoop();
    delete global.boot
}

