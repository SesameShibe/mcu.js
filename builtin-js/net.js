'use strict';

(function () {
    var net = {}
    var activeConn = {};
    var activeServer = {};
    var workBuffer = socket.getWorkBuffer();

    function handleSend(conn) {
        var sendTarget = conn.sendTarget
        var ret = 0
        if (typeof sendTarget == 'string') {
            ret = socket.sendStr(conn.fd, sendTarget, 0, conn.sendOffset)
            if (ret == -2) {
                // finished
                conn.sendTarget = null;
                if (conn.onsent) {
                    conn.onsent(conn);
                }
            } else if (ret > 0) {
                conn.sendOffset += ret;
            }
        } else {
            ret = socket.send(conn.fd, sendTarget, 0, conn.sendOffset, conn.sendLength)
            if (ret == -2) {
                // finished
                conn.sendTarget = null;
                if (conn.onsent) {
                    conn.onsent(conn);
                }
            } else if (ret > 0) {
                conn.sendOffset += ret
            }
        }
    }

    function pollTask() {
        var fd;
        var ret;
        for (fd in activeConn) {
            var conn = activeConn[fd];
            ret = socket.recv(fd, 0);
            if (ret == 0) {
                conn.close();
            } else if (ret > 0) {
                if (conn.onrecv) {
                    conn.onrecv(conn, workBuffer.subarray(0, ret))
                }
            }
            if (conn.sendTarget) {
                handleSend(conn)
            }
        }
        for (fd in activeServer) {
            var server = activeServer[fd];
            var new_fd = socket.accept(fd);
            if (new_fd !== -1) {
                socket.setNonblocking(new_fd);
                var conn = new Conn(new_fd);
                server.onconn(conn);
            }
        }
    }

    function Conn(fd) {
        this.fd = fd;
        activeConn[this.fd] = this;
        this.sendTarget = null;
        this.sendOffset = 0;
        this.status = 1;
    }

    Conn.prototype.send = function (bufOrString, len) {
        len = len || bufOrString.length;
        if (this.sendTarget !== null) {
            throw 'Conn is already sending.'
        }
        this.sendTarget = bufOrString;
        this.sendOffset = 0;
        this.sendLength = len
        handleSend(this)
    }

    Conn.prototype.close = function () {
        if (this.status == 1) {
            this.status = 0;
            var ret = socket.close(this.fd);
            //print('close ' + this.fd + ' ' + ret);
            delete activeConn[this.fd];
            if (this.onclose) {
                this.onclose(this);
            }
        }
    }

    function Server(fd) {
        this.fd = fd;
        activeServer[this.fd] = this;
        this.status = 1;
    }

    Server.prototype.close = function () {
        if (this.status == 1) {
            this.status = 0;
            socket.close(this.fd);
            delete activeServer[this.fd];
        }
    }

    net.tcpConnect = function (addr, port) {
        var fd = socket.socket(socket.AF_INET, socket.SOCK_STREAM);
        socket.setNonblocking(fd);
        socket.connect(fd, addr, port);
        return new Conn(fd);
    }

    net.createTcpServer = function (port, conncb, backlog) {
        backlog = backlog || 5;
        var fd = socket.socket(socket.AF_INET, socket.SOCK_STREAM);
        socket.setNonblocking(fd);

        if (socket.bind(fd, "0.0.0.0", port) === -1) {
            throw 'bind failed';
        }
        if (socket.listen(fd, backlog) === -1) {
            throw 'listen failed';
        }
        var server = new Server(fd);
        server.onconn = conncb;
        return server;
    }

    setInterval(pollTask, 20);

    global.net = net
})();

