const global = this;

//global.print = console.log;

// Minimal console object.
global.console = (function(){
    var useProxyWrapper = true;
    var c = {};
    function console_log(args, errName) {
        var msg = Array.prototype.map.call(args, function (v) {
            if (typeof v === "object" && v !== null) { return console.format(v); };
            return v;
        }).join(" ");
        if (errName) {
            var err = new Error(msg);
            err.name = "Trace";
            print(err.stack || err);
        } else {
            print(msg);
        }
    };
    c.format = function format(v) { try { return Duktape.enc("jx", v); } catch (e) { return String(v); } };
    c.assert = function assert(v) {
        if (arguments[0]) { return; }
        console_log(arguments.length > 1 ? Array.prototype.slice.call(arguments, 1) : [ "false == true" ], "AssertionError");
    };
    c.log = function log() { console_log(arguments, null); };
    c.debug = function debug() { console_log(arguments, null); };
    c.trace = function trace() { console_log(arguments, "Trace"); };
    c.info = function info() { console_log(arguments, null); };
    c.warn = function warn() { console_log(arguments, null); };
    c.error = function error() { console_log(arguments, "Error"); };
    c.exception = function exception() { console_log(arguments, "Error"); };
    c.dir = function dir() { console_log(arguments, null); };
    if (typeof Proxy === "function" && useProxyWrapper) {
        var orig = c;
        var dummy = function () {};
        c = new Proxy(orig, {
            get: function (targ, key, recv) {
                var v = targ[key];
                return typeof v === "function" ? v : dummy;
            }
        });
    }
    return c;
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
        intervalList.push([loopCount, handler, arg]);
    };
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
            var t = intervalList[i];
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
    mainLoop();
    delete global.boot
}

