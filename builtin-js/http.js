'use strict';

(function () {
    var http = {};
    var textDec = new TextDecoder();
    var error = print
    var debug = print

    function generateHeaderFromDict(header) {
        var str = '';
        for (var key in header) {
            var tmp = key.substring(0, 1).toUpperCase() + key.substring(1)
            str += tmp + ': ' + header[key] + "\r\n";
        }
        return str;
    }

    function Request(method, path) {
        this.method = method;
        this.path = path;
        this.header = null
    }

    function Response(conn) {
        this.conn = conn;
        this.header = null;
    }

    Response.prototype.setHeader = function (statusCode, headerDict) {
        statusCode = statusCode || 200;
        headerDict = headerDict || { "connection": "close" };
        this.header = "HTTP/1.0 " + statusCode + " OK\r\n" + generateHeaderFromDict(headerDict) + "\r\n";
    }

    Response.prototype.sendStringAndClose = function (body) {
        if (!this.header) {
            this.setHeader()
        }
        //this.header = this.header + "content-length: " + Buffer.byteLength(body, 'utf8') + "\r\n\r\n";
        this.conn.send(this.header + body);
        this.conn.onsent = function (conn) {
            conn.close();
        }
        
    }

    Response.prototype.serveFile = function (path) {
        if (!fs.exists(path)) {
            error("http error: no such file to serve");
            this.setHeader(404);
            this.send('404');
            return;
        }
        var filebuf = new Uint8Array(1024);
        var file = fs.open(path, fs.O_RDONLY);
        this.setHeader(200);
        this.conn.onsent = function (conn) {
            var count = fs.read(file, filebuf, filebuf.length);
            if (count === 0) {
                fs.close(file);
                conn.close();
            }
            conn.send(filebuf.subarray(0, count));
        }
        this.conn.send(this.header);
    }

    // find \n
    function findBoundary(workBuffer, ret) {
        for (var i = 1; i < ret; i++) {
            if (workBuffer[i] === 10) {
                return i;
            }
        }
        return -1;
    }

    // requestListener(req,res)
    http.createServer = function (port, requestListener) {
        return net.createTcpServer(port, function (conn) {
            http.waitForHTTPHeader(conn, function(conn, header, additionalDataAfterHeader) {
                debug(header)
                if (header) {
                    var firstLine = header.substring(0, header.indexOf('\r\n'))
                    var arr = firstLine.split(' ', 3)
                    var req = new Request(arr[0].toUpperCase(), arr[1])
                    req.header = header
                    var res = new Response(conn)
                    requestListener(req, res)
                    if (req.ondata) {
                        if (additionalDataAfterHeader) {
                            if (additionalDataAfterHeader.length > 0) {
                                req.ondata(req, additionalDataAfterHeader)
                            }
                        }
                        conn.onrecv = function(conn, buf) {
                            req.ondata(req, buf)
                        }
                    }
                    
                    return
                }
                error("http error: cannot parse header");
                conn.send("HTTP/1.1 400 Bad Request\r\nconnection: close\r\n\r\n");
                conn.close()
            })
        });
    }

    http.waitForHTTPHeader = function (tcpConn, callback, maxHeaderLen, timeout) {
        var headerCmpState = 0
        var header = ''
        maxHeaderLen = maxHeaderLen || 8192
        // TODO: support timeout
        timeout = timeout || 120

        tcpConn.onrecv = function(tcpConn, buf) {
            var bufLen = buf.length
            var i = 0
            for (i = 0; i < bufLen; i++) {
                if ((headerCmpState % 2 == 0) && (buf[i] == 0x0D)) {
                    headerCmpState ++
                } else if ((headerCmpState % 2 == 1) && (buf[i] == 0x0A)) {
                    headerCmpState ++
                } else {
                    headerCmpState = 0
                }
                if (headerCmpState == 4) {
                    break
                }
            }
            if (headerCmpState == 4) {
                // '\r\n\r\n' sequence received
                header += textDec.decode(buf.subarray(0, i + 1))
                tcpConn.onrecv = null
                callback(tcpConn, header, buf.subarray(i + 1))
                header = null
            } else {
                header += textDec.decode(buf)
                if (header.length > maxHeaderLen) {
                    tcpConn.onrecv = null
                    header = null
                    callback(tcpConn, null, new Uint8Array(0))
                }
            }
        }
    }


    /*
        const options = {
            'url': {
                hostname: 'mcujs.org',
                port: 80,
                path: '/dl',
                args: 'a=2'
            },
            'method': 'GET',
            'header': {
              'Connection': 'close'
            }
        }; 
    */
    http.parseUrl = function (url) {
        var reg = /^(http):\/\/([\w\.]+):?([\d]*)([\w\.\-\/]*)\??(.*)/;
        var parsed = url.match(reg);
        return {
            'hostname': parsed[2],
            'port': parsed[3] || 80,
            'path': parsed[4] || '/',
            'args': parsed[5]
        }
    }

    function genRequestHeader(opt) {
        var str = opt.method + ' ' + opt.url.path + ' HTTP/1.0\r\n';
        return str + generateHeaderFromDict(opt.header) + '\r\n';
    }

    http.sendRequest = function (opt,ondata) {
        opt.method = opt.method || 'GET';
        opt.header = opt.header || { 'Host': opt.url.hostname + ':' + opt.url.port, 'User-Agent': 'mcujs', 'Connection': 'close' };

        var conn = net.tcpConnect(socket.getHostByName(opt.url.hostname), opt.url.port);

        conn.send(genRequestHeader(opt));
        conn.onrecv = function(conn, buf){
            ondata(buf);
        }
        
        return conn;
    }

    http.get = function (url, ondata) {
        var opt = { 'url': http.parseUrl(url) };
        http.sendRequest(opt, ondata);
    }

    global.http = http
})();