uart.initPort(0,
    uart.UART_PIN_NO_CHANGE,
    uart.UART_PIN_NO_CHANGE,
    uart.UART_PIN_NO_CHANGE,
    uart.UART_PIN_NO_CHANGE);

function uartLog(s) { if (typeof (s) === 'string') s = s.replace('\n', '\r\n'); uart.writeString(0, s) }
function uartByte(c) { uart.writeByte(0, c) }

var lineBuf = '';
var shellMode = 1;

function tryEval(code, err) {
    try {
        return global.eval(code)
    } catch (e) {
        if (err) {
            if (err(e)) {
                return undefined;
            }
        }
        print('Unhandled error: ' + e) ;
        return undefined;
    }
}

function shellTask() {
    if (shellMode == 0) {
        while (uart.availableBytes(0) > 0) {
            var recvByte = uart.readByte(0);
            if ((recvByte === 0x0D) || (recvByte === 0x0A)) {
                if (lineBuf.length > 0) {
                    uartLog('\r\n');
                    uartLog('' + tryEval(lineBuf));
                    uartLog('\r\n>> ');
                    lineBuf = '';
                }
            } else {
                var s = String.fromCharCode(recvByte);
                uartByte(recvByte);
                lineBuf += s;
            }
        }
    } else {
        while (uart.availableBytes(0) > 0) {
            var recvByte = uart.readByte(0);
            if (recvByte === 0xF8) {
                if (lineBuf.length > 0) {
                    uart.writeString(0, '' + tryEval(lineBuf));
                    uartByte(0x0A);
                    lineBuf = '';
                }
            } else {
                var s = String.fromCharCode(recvByte);
                lineBuf += s;
            }
        }
    }
}

function shellSetMode(m) {
    shellMode = m;
}

uartLog('\r\n>> ');
setInterval(shellTask, 100);