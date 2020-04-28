declare module fs {function lseek():number;function exists():string;function read():number;function write():number;function mountSpiffs():void;function close():number;function open():number;const SEEK_END:number;const O_CREAT:number;const SEEK_SET:number;const O_APPEND:number;const SEEK_CUR:number;const O_RDONLY:number;const O_WRONLY:number;const O_RDWR:number;const O_TRUNC:number;}declare module socket {function socket():number;function bind():number;function setNonblocking():number;function sendStr():number;function send():number;function getHostByName():string;function accept():number;function connect():number;function shutdown():number;function errno():number;function getWorkBuffer():Uint8Array;function close():number;function recv():number;function listen():number;const SOCK_RAW:number;const SOCK_DGRAM:number;const SOCK_STREAM:number;const AF_INET:number;const F_GETFL:number;const O_NONBLOCK:number;const F_SETFL:number;}declare module uart {function readByte():number;function availableBytes():number;function writeString():void;function writeByte():void;function initPort():boolean;const UART_PIN_NO_CHANGE:number;}declare module wifiSTA {function begin():void;function isConnected():boolean;}declare module crypto {function hashBuf():number;}declare module spi {function write8():void;function begin():void;function end():void;function writeBuf():void;function write32():void;function write16():void;}declare module ui {function drawTriangle():void;function measureTextWidth():number;function drawLine():void;function getPenColor():number;function clear():void;function drawText():void;function update():void;function setPenColor():void;function fillTriangle():void;function init():void;function drawCircle():void;function measureTextHeight():number;function fillRectangle():void;function drawRectangle():void;const SCREEN_WIDTH:number;const SCREEN_HEIGHT:number;}declare module tft {function print():void;function fillScreen():void;function setTextColor():void;function setCursor():void;const WHITE:number;const BLACK:number;}declare module gpio {function write():void;function read():number;function pinMode():void;const INPUT:number;const OUTPUT:number;}declare module os {function getTickCountMs():number;function getMinFreeMem():number;function reboot():number;function sleepMs():void;function getFreeMem():number;}