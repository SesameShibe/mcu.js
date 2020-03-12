
var ws = {};

(function() {
    /*
        States:
        0: Waiting for upgrading header
        1: Upgraded
    */

    var textEnc = new TextEncoder()
    var textDec = new TextDecoder()

    function calcSecAccept(clientKey) {
        var buf = textEnc.encode(clientKey + '258EAFA5-E914-47DA-95CA-C5AB0DC85B11')
        var hash = new Uint8Array(20)
        crypto.hashBuf('sha1', buf, hash)
        return Duktape.enc('base64', hash)
    }

    ws.Conn = function(conn) {
        this.conn = conn
        this.state = 0
        this.pendingData = []
    }

    ws.Server = function(conn) {
        this.conn = conn
        this.clients = {}
    }
    
    ws.debug = print
    ws.error = print

    function onTcpConnSent(conn) {
        var wsConn = conn.wsConn

    }

    function onTcpConnRecv(conn, buf) {
        var wsConn = conn.wsConn
        if (wsConn.state == 0) {
            var headerStr = textDec.decode(buf)
            ws.debug(headerStr)
            if (headerStr.endsWith('\r\n')) {
                var pos = headerStr.indexOf('\r\nSec-WebSocket-Key:')
                var pos2 = header.indexOf('\r\n', pos + 20)
                if ((pos > 0) && (pos2 > 0)) {
                    var key = headerStr.substring(pos, pos2).trim()
                    ws.debug('ws upgrade header received, key:' + key)
                    var accept = calcSecAccept(key)
                }
            }
        }
    }

    ws.createServer = function(port, connListener) {
        var conn = net.createTcpServer(port, function(tcpConn) {
            tcpConn.wsConn = new ws.Conn(tcpConn)
            tcpConn.onrecv = onTcpConnRecv
            tcpConn.onsent = onTcpConnSent
        })
        return new ws.Server(conn)
    }
})();
