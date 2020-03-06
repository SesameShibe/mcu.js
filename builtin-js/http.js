var http = {};
(function () {

    var decoder = new TextDecoder();

    function headersToStr (headers) {
        var str = '';
        for (var key in headers) {
            str += key + ': ' + headers[key] + "\r\n";
        }
        return str;
    }

    function Request (method, url) {
        this.method = method;
        this.url = url;
    }

    function Response (conn) {
        this.conn = conn;
        this.headers = '';
    }
    Response.prototype.writeHead = function (stateCode,headersDict) {
        stateCode = stateCode || 200;
        headersDict = headersDict || {"connection":"close"};
        this.headers = "HTTP/1.1 " + stateCode + "\r\n" + headersToStr(headersDict) + "\r\n";
    }
    Response.prototype.send = function (body) {
        //this.headers = this.headers + "content-length: " + Buffer.byteLength(body, 'utf8') + "\r\n\r\n";
        this.conn.send(this.headers + body);
        this.conn.onsent = function (conn) {
            conn.close();
        }
    }
    Response.prototype.serveFile = function (path) {
        if (!fs.exists(path)) {
            print("No such file!");
            this.writeHead(404);
            this.send('404');
            return;
        }
        var filebuf = new Uint8Array(1024);
        var file = fs.open(path,fs.O_RDONLY);
        this.writeHead(200);
        this.conn.onsent = function (conn) {
            var count = fs.read(file,filebuf,filebuf.length);
            if (count === 0) {
                fs.close(file);
                conn.close();
            }
            conn.send(filebuf.subarray(0,count));
        }
        this.conn.send(this.headers);
    }

    // find \n
    function findBoundary (workBuffer, ret) {
        for (var i = 1; i < ret; i++) {
            if (workBuffer[i] === 10) {
                return i;
            }
        }
        return -1;
    }

    // requestListener(req,res)
    http.createServer = function (port,requestListener) {
        return net.createTcpServer(port,function (conn){
            var isReqline = true; //Assuming that the first packet contains the full request line
            var req, res; 
            conn.onrecv = function (conn, workBuffer, ret) {
                var boundary = 0;
                if (isReqline) {
                    boundary = findBoundary(workBuffer,ret)+1;
                    if (!boundary) {
                        print("PARSE ERROR!");
                        conn.send("HTTP/1.1 400 Bad Request\r\nconnection: close\r\n\r\n");
                        conn.close();
                        return;
                    }
                    var reqline = decoder.decode(workBuffer.subarray(0,boundary-2)).split(' ');
                    req = new Request(reqline[0].toUpperCase(), reqline[1]);
                    res = new Response(conn);
                    requestListener(req,res);
                    isReqline = false;
                }
                if (req.ondata && boundary < ret) {
                    req.ondata(workBuffer.subarray(boundary,ret));
                }
            }
        });
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
        'headers': {
          'Connection': 'close'
        }
    }; 
*/      
    http.parseUrl = function (url) {
        var reg = /^(http):\/\/([\w.]+):?([\d]*)([\w\/-]*)\??([\w=&.]*)/;
        var parsed = url.match(reg);
        return {
            'hostname': parsed[2],
            'port': parsed[3] || 80,
            'path': parsed[4] || '/',
            'args': parsed[5]
        }
    }

    function genRequestHeader (opt) {
        var str = opt.method+' '+opt.url.path+' HTTP/1.1\r\n';
        return str + headersToStr(opt.headers) + '\r\n';
    }

    http.request = function (opt,callback) {
        opt.method = opt.method || 'GET';
        opt.headers = opt.headers || {'Host': opt.url.hostname+':'+opt.url.port,'User-Agent': 'mcujs','Connection': 'close'};

        var conn = net.tcpConnect(socket.getHostByName(opt.url.hostname), opt.url.port);
        
        if (opt.method == 'GET') {
            conn.send(genRequestHeader(opt));
            conn.onrecv = function(conn, workBuffer, ret){
                callback(workBuffer.subarray(0,ret));
            }
        }
        return conn;
    }

    http.get = function (url,callback) {
        var opt = {'url': http.parseUrl(url)};
        http.request(opt,callback);
    }
})();