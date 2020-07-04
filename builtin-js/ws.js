'use strict';

(function () {
    var ws = {};
    ws.MAX_DATA_SIZE = 1024
    ws.OPCODE_CLOSE = 0x8
    ws.OPCODE_PING = 0x9
    ws.OPCODE_PONG = 0xA
    ws.OPCODE_CONT_FRAME = 0x0
    ws.OPCODE_TEXT_FRAME = 0x1
    ws.OPCODE_BIN_FRAME = 0x2

    var textEnc = new TextEncoder()
    var textDec = new TextDecoder()
    var dbgPrint = print
    var errPrint = print


    function calcSecAccept(clientKey) {
        var buf = textEnc.encode(clientKey + '258EAFA5-E914-47DA-95CA-C5AB0DC85B11')
        var hash = new Uint8Array(20)
        crypto.hashBuf('sha1', buf, hash)
        return Duktape.enc('base64', hash)
    }

    function handleMask(buf,len,mask) {
        for (var i = 0; i < len; i++) {
            buf[i] ^= mask[i % 4]
        }
        return buf
    }

    ws.Conn = function (conn,type) {
        this.conn = conn
        this.type = type || 0 //0: Server, 1: Client

        this.state = 0 // 0: not upgraded, 1: upgraded, receiving header 2: header received, receiving payload
        this.recvHeaderBuf = new Uint8Array(14)
        this.recvHeaderLen = 0
        this.recvPayloadBuf = null
        this.recvMaskEnabled = 0
        this.recvMask = new Uint8Array(4)
        this.recvOpCode = 0
        this.recvFin = 0
        this.recvPayloadLen = 0
        this.recvPayloadPtr = 0

        this.pendingPayload = null
        this.sendHeaderBuf = new Uint8Array(14)
        this.sendQueue = []
        this.sendBusy = false

        this.server = null
    }

    ws.Server = function () {
        this.clients = {}
    }

    ws.Conn.prototype.checkAndParseRecvHeader = function () {
        var ptr = 2
        if (this.recvHeaderLen < ptr) {
            return ptr
        }
        var buf = this.recvHeaderBuf

        this.recvOpCode = buf[0] & 0x0F
        this.recvFin = (buf[0] >>> 7) & 1
        this.recvMaskEnabled = (buf[1] >>> 7) & 1

        var payloadLen = buf[1] & 0x7F
        if (payloadLen == 126) {
            ptr += 2
            if (this.recvHeaderLen < ptr) {
                return ptr
            }
            payloadLen = (buf[ptr - 2] << 8) | buf[ptr - 1]
        } else if (payloadLen == 127) {
            errPrint('ws error: payloadLen too large to be decoded.')
            this.close()
        }
        if (payloadLen > ws.MAX_DATA_SIZE) {
            errPrint('ws error: payloadLen too large: ' + payloadLen)
            this.close()
        }

        if (this.recvMaskEnabled) {
            ptr += 4
            if (this.recvHeaderLen < ptr) {
                return ptr
            }
            for (var i = 0; i < 4; i++) {
                this.recvMask[i] = buf[ptr - 4 + i]
            }
        }

        this.recvPayloadLen = payloadLen
        this.recvPayloadPtr = 0
        return 0
    }



    ws.Conn.prototype.close = function () {
        this.conn.close()
    }

    ws.Conn.prototype.send = function (data) {
        print('send')
        this.sendQueue.push(data)
        if (this.sendBusy == false) {
            if (this.state > 0) {
                onTcpConnSent(this.conn)
            }
        }
    }

    function onTcpConnSent(conn) {
        var wsConn = conn.wsConn
        wsConn.sendBusy = true

        if (wsConn.pendingPayload === null) {
            // no payload or already sent 
            if (wsConn.sendQueue.length == 0) {
                // no data in queue
                wsConn.sendBusy = false
                return
            }
            var data = wsConn.sendQueue.shift()
            var buf = wsConn.sendHeaderBuf
            var headerLen = 2
            var opcode = 2
            var payload = null

            if (typeof data === 'number') {
                // send an opcode with empty data
                opcode = data
            } else if (typeof data === 'string') {
                opcode = ws.OPCODE_TEXT_FRAME
                payload = textEnc.encode(data)
            } else {
                opcode = ws.OPCODE_BIN_FRAME
                payload = data
            }

            var payloadLen = 0
            if (payload !== null) {
                payloadLen = payload.length
            }
            // set FIN bit
            buf[0] = opcode | (1 << 7)
            if (payloadLen < 126) {
                buf[1] = payloadLen
            } else {
                headerLen += 2
                buf[1] = 126
                buf[2] = payloadLen >> 8
                buf[3] = payloadLen & 0xFF
            }

            //this conn is a client
            if (wsConn.type === 1) {

                // set MASK bit
                buf[1] = buf[1] | 0x80

                //gen masking-key
                var mask = new Uint8Array(4)
                for (var i = 0; i < 4; i++) {
                    mask[i] = Math.random() * 1000
                    buf[headerLen + i] = mask[i]
                }
                headerLen += 4

                if (payloadLen > 0) {
                    handleMask(payload,payloadLen,mask)
                }
            }

            wsConn.pendingPayload = payload
            wsConn.conn.send(wsConn.sendHeaderBuf.subarray(0, headerLen))
        } else {
            var tmp = wsConn.pendingPayload

            wsConn.pendingPayload = null
            wsConn.conn.send(tmp)
        }
    }

    function onTcpConnClose(conn) {
        if (conn.wsConn.onclose) {
            conn.wsConn.onclose(conn.wsConn)
        }
        delete conn.wsConn
    }

    function onTcpConnRecv(conn, buf) {
        var wsConn = conn.wsConn

        if (wsConn.state == 0) {
            var headerStr = textDec.decode(buf)
            dbgPrint(headerStr)
            if (headerStr.endsWith('\r\n\r\n')) {

                return
            }
        }
        var desiredHeaderLen = 2
        for (var p = 0; p < buf.length; p++) {
            if (wsConn.state == 1) {
                wsConn.recvHeaderBuf[wsConn.recvHeaderLen] = buf[p]
                wsConn.recvHeaderLen++
                if (wsConn.recvHeaderLen == desiredHeaderLen) {
                    desiredHeaderLen = wsConn.checkAndParseRecvHeader()
                }
                if (desiredHeaderLen == 0) {
                    // header received
                    desiredHeaderLen = 2
                    if (wsConn.recvPayloadLen == 0) {
                        // just call the callback with null data
                        if (wsConn.onframe) {
                            wsConn.onframe(wsConn, wsConn.recvOpCode, null, wsConn.recvFin)
                        }
                    } else {
                        wsConn.state = 2
                    }
                    wsConn.recvHeaderLen = 0
                }
            } else if (wsConn.state == 2) {
                wsConn.recvPayloadBuf[wsConn.recvPayloadPtr] = buf[p]
                wsConn.recvPayloadPtr++
                if (wsConn.recvPayloadPtr >= wsConn.recvPayloadLen) {
                    // payload received, unmask if needed
                    if (wsConn.recvMaskEnabled) {
                        handleMask(wsConn.recvPayloadBuf,wsConn.recvPayloadLen,wsConn.recvMask)
                    }
                    if (wsConn.onframe) {
                        wsConn.onframe(wsConn, wsConn.recvOpCode, wsConn.recvPayloadBuf.subarray(0, wsConn.recvPayloadLen), wsConn.recvFin)
                    }
                    wsConn.state = 1
                }
            }
        }
    }

    function createWsConn(tcpConn,type){
        var wsConn = new ws.Conn(tcpConn,type)
        tcpConn.wsConn = wsConn
        tcpConn.onrecv = onTcpConnRecv
        tcpConn.onsent = onTcpConnSent
        tcpConn.onclose = onTcpConnClose
        wsConn.state = 1
        wsConn.recvPayloadBuf = new Uint8Array(ws.MAX_DATA_SIZE)
        return wsConn
    }


    ws.createServer = function (port, connListener) {
        var wsServer = new ws.Server()
        wsServer.onconn = connListener
        var tcpServer = net.createTcpServer(port, function (tcpConn) {
            http.waitForHTTPHeader(tcpConn, function (tcpConn, header, additionalDataAfterHeader) {
                dbgPrint(header)
                if (header) {
                    var key = http.findValueByKey(header,'Sec-WebSocket-Key')
                    if (key) {
                        dbgPrint('ws upgrade header received, key: ' + key)
                        var accept = calcSecAccept(key)
                        tcpConn.send('HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: ' + accept + '\r\n\r\n')
                        var wsConn = createWsConn(tcpConn,0)
                        wsConn.server = wsServer
                        wsConn.server.onconn(wsConn)
                        return
                    }
                }
                errPrint('ws error: invalid upgrade header: ' + header)
                conn.close()
            })
        })
        wsServer.tcpServer = tcpServer
    }
    
    ws.connect = function (url,onconn) {
        var opt = {
            'url': http.parseUrl(url),
            'version':'HTTP/1.0',
        };
        opt.header = {
            'Host': opt.url.hostname + ':' + opt.url.port,
            'User-Agent': 'mcujs',
            'Upgrade': 'websocket',
            'Connection': 'Upgrade',
            'Sec-WebSocket-Key': 'dGhlIHNhbXBsZSBub25jZQ==',
            'Sec-WebSocket-Version': '13'
        };
        var tcpConn = http.sendRequest(opt);
        http.waitForHTTPHeader(tcpConn, function (tcpConn, header, additionalDataAfterHeader) {
            dbgPrint(header)
            if (header) {
                var value = http.findValueByKey(header,'Sec-WebSocket-Accept')
                if(value === 's3pPLMBiTxaQ9kYGzzhZRbK+xOo='){
                    dbgPrint('ws Accept!')
                    var wsConn = createWsConn(tcpConn,1)
                    onconn(wsConn)
                }else{
                    errPrint('ws error: invalid Sec-WebSocket-Accept: ' + key)
                    conn.close()
                }
                return
            }
            errPrint('ws error: invalid upgrade header: ' + header)
            conn.close()
        })
    }

    global.ws = ws
})();
