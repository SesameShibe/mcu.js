
#include "lwip/sockets.h"
#include "lwip/netdb.h"

uint8_t EXT_RAM_ATTR workBuffer[4500];
char host[16];

i32 halSocketSocket(i32 a0, i32 a1, i32 a2)
{
    return socket(a0, a1, a2);
}

i32 halSocketListen(i32 fd, i32 backlog)
{
    return listen(fd, backlog);
}

i32 halSocketClose(i32 fd)
{
    return close(fd);
}

int32_t halSocketConnect(int32_t s, const char *addr, int32_t port)
{
    struct sockaddr_in ra;

    memset(&ra, 0, sizeof(struct sockaddr_in));

    ra.sin_family = AF_INET;
    ra.sin_addr.s_addr = inet_addr(addr);
    ra.sin_port = htons(port);

    int32_t ret = connect(s, (struct sockaddr *)&ra, sizeof(struct sockaddr_in));
    return ret;
}

int32_t halSocketRecv(int32_t s, int32_t flags)
{
    int32_t recv_len = recv(s, workBuffer, sizeof(workBuffer), flags);
    return recv_len;
}

int32_t halSocketBind(int32_t s, const char *addr, int32_t port)
{
    struct sockaddr_in sa;

    memset(&sa, 0, sizeof(struct sockaddr_in));

    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = inet_addr(addr);
    sa.sin_port = htons(port);

    int32_t ret = bind(s, (struct sockaddr *)&sa, sizeof(struct sockaddr_in));
    return ret;
}

int32_t halSocketAccept(int32_t s)
{
    struct sockaddr_in isa;
    socklen_t addr_size = sizeof(isa);

    int32_t ret = accept(s, (struct sockaddr *)&isa, &addr_size);
    return ret;
}

int32_t halSocketSelect(int32_t maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset, uint32_t t)
{
    struct timeval timeout = {0, (suseconds_t) t};
    int32_t ret = select(maxfdp1, readset, writeset, exceptset, &timeout);
    return ret;
}

// set a fd nonblocking
int halSocketSetNonblocking(int fd)
{
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}

int halSocketErrno()
{
    return errno;
}

int halSocketShutdown(int fd, int flags)
{
    return shutdown(fd, flags);
}

int halSocketSendStr(int fd, const char *str, int flags, u32 offset)
{
    u32 strLength = strlen(str);
    if (offset >= strLength)
    {
        return -3;
    }
    int ret = send(fd, str + offset, (size_t)strLength - offset, flags);
    if (ret > 0)
    {
        if (offset + ret >= strLength)
        {
            // we are finished!
            ret = -2;
        }
    }
    return ret;
}

int halSocketSend(int fd, JS_BUFFER buf, int flags, u32 offset, u32 totalLen) {
    if ((totalLen > buf.size) || (offset >= totalLen)) {
        return -3;
    }
    int ret = send(fd, buf.buf + offset, totalLen - offset, flags);
    if (ret > 0) {
        if (offset + ret >= totalLen) {
            ret = -2;
        }
    }
    return ret;
}

JS_BUFFER halSocketGetWorkBuffer() {
    JS_BUFFER ret = {
        .buf = workBuffer,
        .size = sizeof(workBuffer)
    };
    return ret;
}

const char * halSocketGetHostByName(const char * name) {
    struct hostent *hptr;
    if ((hptr = gethostbyname(name)) == NULL) {
        return NULL;
    }
    if (hptr->h_addrtype == AF_INET) {
        return inet_ntop(hptr->h_addrtype,hptr->h_addr,host,sizeof(host));
    }
    return NULL;
}

#if 0
static duk_ret_t glueSocketFcntl(duk_context *ctx)
{
    int32_t ret;
    int32_t arg0;
    int32_t arg1;
    int32_t arg2;

    arg0 = duk_to_int32(ctx, 0);
    arg1 = duk_to_int32(ctx, 1);
    arg2 = duk_to_int32(ctx, 2);

    ret = halSocketFcntl(arg0, arg1, arg2);
    duk_push_int(ctx, ret);
    return 1;
}

static duk_ret_t glueSocketSetNonblocking(duk_context *ctx)
{
    int32_t ret;
    int32_t fd;

    fd = duk_to_int32(ctx, 0);

    ret = halSocketSetNonblocking(fd);
    duk_push_int(ctx, ret);
    return 1;
}


static duk_ret_t glueSocketClose(duk_context *ctx)
{
    int32_t ret;
    int32_t arg0;

    arg0 = duk_to_int32(ctx, 0);

    ret = halSocketClose(arg0);
    duk_push_int(ctx, ret);
    return 1;
}

static duk_ret_t glueSocketSocket(duk_context *ctx)
{
    int32_t ret;
    int32_t arg0;
    int32_t arg1;
    int32_t arg2;

    arg0 = duk_to_int32(ctx, 0);

    arg1 = duk_to_int32(ctx, 1);

    arg2 = duk_to_int32(ctx, 2);

    ret = halSocketSocket(arg0, arg1, arg2);
    duk_push_int(ctx, ret);
    return 1;
}

static duk_ret_t glueSocketConnect(duk_context *ctx)
{
    int32_t ret;
    int32_t socket;
    const char* host;
    uint32_t port;

    socket = duk_to_int32(ctx, 0);

    host = duk_to_string(ctx, 1);

    port = duk_to_uint32(ctx, 2);

    ret = halSocketConnect(socket, host, port);
    duk_push_int(ctx, ret);
    return 1;
}

static duk_ret_t glueSocketGetWorkBuffer(duk_context *ctx)
{


    /* Create an external buffer pointing to the work buffer. */
    duk_push_external_buffer(ctx);
    duk_config_buffer(ctx, -1, workBuffer, sizeof(workBuffer));
    return 1;
}

static duk_ret_t glueSocketRecv(duk_context *ctx)
{
    int32_t ret;
    int32_t socket;
    int32_t flag;

    socket = duk_to_int32(ctx, 0);
    flag = duk_to_int32(ctx, 1);

    ret = halSocketRecv(socket, flag);
    duk_push_int(ctx, ret);
    return 1;
}

/* socket, string, flags. */
static duk_ret_t glueSocketSendStr(duk_context *ctx)
{
    int32_t ret;
    duk_size_t sz;
    int32_t socket;
    const char* str;
    int32_t flag;

    socket = duk_to_int32(ctx, 0);
    str = duk_to_lstring(ctx, 1, &sz);
    flag = duk_to_int32(ctx, 2);
    size_t offset = duk_to_int32(ctx, 3);
    if (offset >= sz) {
        duk_push_int(ctx, -3);
        return 1;
    }

    ret = halSocketSend(socket,str + offset,(size_t)sz - offset,flag);
    if (ret > 0) {
        if (offset + ret >= sz) {
            // we are finished!
            ret = -2;
        }
    }
    duk_push_int(ctx, ret);
    return 1;
}

static duk_ret_t glueSocketShutdown(duk_context *ctx)
{
    int32_t ret;
    int32_t arg0;
    int32_t arg1;

    arg0 = duk_to_int32(ctx, 0);
    arg1 = duk_to_int32(ctx, 1);

    ret = halSocketShutdown(arg0, arg1);
    duk_push_int(ctx, ret);
    return 1;
}

static duk_ret_t glueSocketBind(duk_context *ctx)
{
    int32_t ret;
    int32_t arg0;
    const char* arg1;
    uint32_t arg2;

    arg0 = duk_to_int32(ctx, 0);

    arg1 = duk_to_string(ctx, 1);

    arg2 = duk_to_uint32(ctx, 2);

    ret = halSocketBind(arg0, arg1, arg2);
    duk_push_int(ctx, ret);
    return 1;
}

static duk_ret_t glueSocketListen(duk_context *ctx)
{
    int32_t ret;
    int32_t arg0;
    int32_t arg1;

    arg0 = duk_to_int32(ctx, 0);
    arg1 = duk_to_int32(ctx, 1);

    ret = halSocketListen(arg0, arg1);
    duk_push_int(ctx, ret);
    return 1;
}

static duk_ret_t glueSocketAccept(duk_context *ctx)
{
    int32_t ret;
    int32_t arg0;

    arg0 = duk_to_int32(ctx, 0);

    ret = halSocketAccept(arg0);
    duk_push_int(ctx, ret);
    return 1;
}

static duk_ret_t glueSocketErrno(duk_context *ctx)
{
    int32_t ret;

    ret = errno;
    duk_push_int(ctx, ret);
    return 1;
}
#endif
