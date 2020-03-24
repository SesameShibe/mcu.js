(function (){

    var lineBuf = '';
    var textDec = new TextDecoder();

    global.webshell = function(port){
        port = port || 80;

        ws.createServer(port,function(conn){
            print("ws connect success!");

            conn.onframe = function(conn,opCode,data,fin){
                if (opCode === ws.OPCODE_CLOSE){
                    conn.close();
                    print("ws close success!");
                }
                else if (opCode === ws.OPCODE_TEXT_FRAME && data !== null) {
                    lineBuf += textDec.decode(data);
                    if(fin === 1){
                        conn.send(''+tryEval(lineBuf));
                        lineBuf = '';
                    }
                }
            }
        });
    }
})();