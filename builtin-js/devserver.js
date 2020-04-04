(function (){

    var lineBuf = '';
    var textDec = new TextDecoder();

    global.devServer = function(port){
        port = port || 80;

        ws.createServer(port,function(conn){
            var oldInterface = interfacePrint;

            print("ws connect success!");

            interfacePrint = function(data){
                conn.send(''+data);
            }

            conn.onclose = function(){
                interfacePrint = oldInterface;
            }

            conn.onframe = function(conn,opCode,data,fin){
                if (opCode === ws.OPCODE_CLOSE){
                    conn.close();
                    print("ws close success!");
                }
                else if (opCode === ws.OPCODE_TEXT_FRAME && data !== null) {
                    lineBuf += textDec.decode(data);
                    if(fin === 1){
                        print(''+tryEval(lineBuf));
                        lineBuf = '';
                    }
                }
            }
        });
    }
})();