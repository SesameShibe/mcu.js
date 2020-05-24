var arr = new Int16Array(4)
setInterval(function() {
    var cnt = board.touch.readOne(arr)
    if (cnt > 0) {
        // the data in arr is invalid when cnt == 0
        print('X: ' + arr[0] + ' Y: ' + arr[1] + ' ID: ' + arr[2])
    }
}, 10)