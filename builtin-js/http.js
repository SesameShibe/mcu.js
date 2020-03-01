var http = {};
(function () {

    var decoder = new TextDecoder();

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
        this.headers = "HTTP/1.1 " + stateCode + "\r\n";
        for (var key in headersDict) {
            this.headers = this.headers + key + ': ' + headersDict[key] + "\r\n";
        }
        this.headers += "\r\n";
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
            this.send();
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

})();