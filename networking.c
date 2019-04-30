/**
 *
 * @file    networking
 * @author  chosen0ne(louzhenlin86@126.com)
 * @date    2018-12-27 11:11:44
 */

#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <assert.h>

#include "networking.h"
#include "logging.h"
#include "common.h"
#include "client.h"
#include "buffer.h"
#include "processor.h"
#include "protocol.h"
#include "ioworker_pool.h"
#include "octopus.h"

#define READ_SOCK_UNIT_BYTES    1024
#define WRITE_SOCK_UNIT_BYTES   1024

// Implementation of multi-threaded IO:
// 1) Each thread(worker) has a event loop. Main thread accecpts new connected socket, and
//      pass it to a thread. The socket will be processed by the thread.
// 2) Main thread process all io event, and other thread will pass all io events to Main
//      thread by a pipe.

typedef struct {
    struct aeEventLoop      *event_loop;

    int     fd;
    void    *cli_data;
    int     mask;
} event_ctx_t;

static inline int socket_set_nonblock(int sockfd) {
    int     flags;

    if ((flags = fcntl(sockfd, F_GETFL)) == -1) {
        OCTOPUS_ERROR_LOG_BY_ERRNO("failed to get flag of fd, socket: %d", sockfd);
        return OCTOPUS_ERR;
    }

    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        OCTOPUS_ERROR_LOG_BY_ERRNO("failed to set flag of fd, socket: %d", sockfd);
        return OCTOPUS_ERR;
    }

    return OCTOPUS_OK;
}

int addr_parse(struct sockaddr *addr, char *ipbuf, int ipbuf_size, uint16_t *port) {
    struct sockaddr_in      *addr_in;
    struct sockaddr_in6     *addr_in6;
    in_port_t               p;
    void                    *addr_src;

    THREE_PTRS_NULL_CHECK(addr, ipbuf, port);

    if (addr->sa_family == AF_INET) {
        addr_in = (struct sockaddr_in *)addr;
        p = addr_in->sin_port;
        addr_src = &addr_in->sin_addr;
    } else if (addr->sa_family == AF_INET6) {
        addr_in6 = (struct sockaddr_in6 *)addr;
        p = addr_in6->sin6_port;
        addr_src = &addr_in6->sin6_addr;
    } else {
        OCTOPUS_ERROR_LOG("A protocol not supported");
        return OCTOPUS_ERR;
    }

    if (inet_ntop(addr->sa_family, addr_src, ipbuf, ipbuf_size) == NULL) {
        OCTOPUS_ERROR_LOG_BY_ERRNO("Failed to parse a binary format of address");
        return OCTOPUS_ERR;
    }

    *port = ntohs(p);

    return OCTOPUS_OK;
}

int tcp_nonblk_srv(int sockfd, struct addrinfo *addr, int backlog) {
    int     optval;

    optval = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
        OCTOPUS_ERROR_LOG_BY_ERRNO("failed to set SO_REUSEADDR of socket, socket: %d", sockfd);
        return OCTOPUS_ERR;
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) == -1) {
        OCTOPUS_ERROR_LOG_BY_ERRNO("failed to set SO_KEEPALIVE of socket, socket: %d", sockfd);
        return OCTOPUS_ERR;
    }
    if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval)) == -1) {
        OCTOPUS_ERROR_LOG_BY_ERRNO("failed to set TCP_NODELAY of socket, socket: %d", sockfd);
        return OCTOPUS_ERR;
    }

    if (bind(sockfd, addr->ai_addr, addr->ai_addrlen) == -1) {
        OCTOPUS_ERROR_LOG_BY_ERRNO("failed to bind, socket: %d", sockfd);
        return OCTOPUS_ERR;
    }

    if (listen(sockfd, backlog) == -1) {
        OCTOPUS_ERROR_LOG("failed to listen, socket: %d", sockfd);
        return OCTOPUS_ERR;
    }

    if (socket_set_nonblock(sockfd) == OCTOPUS_ERR) {
        OCTOPUS_ERROR_LOG("failed to set non-block for socket");
        return OCTOPUS_ERR;
    }

    return OCTOPUS_OK;
}

void client_connected(struct aeEventLoop *event_loop, int fd, void *cli_data, int mask) {
    triple_t    *srv_ctx;
    int         cli_fd;
    socklen_t   cli_addrlen, sock_addrlen;
    client_t    *cli;
    char        sock_host[16];
    uint16_t    sock_port;
    protocol_t  *protocol;
    processor_t *processor;

    ioworker_pool_t     *pool;
    struct sockaddr     cli_addr, sock_addr;

    protocol_factory_t      protocol_factory;
    processor_factory_t     processor_factory;

    OCTOPUS_NOT_USED(mask);

    cli_addrlen = sizeof(cli_addr);
    if ((cli_fd = accept(fd, &cli_addr, &cli_addrlen)) == -1) {
        OCTOPUS_ERROR_LOG_BY_ERRNO("failed to accept connection");
        return;
    }

    if ((cli = client_create()) == NULL) {
        OCTOPUS_ERROR_LOG("failed to create client");
        return;
    }

    if (addr_parse(&cli_addr, cli->host, 16, &cli->port) == OCTOPUS_ERR) {
        OCTOPUS_ERROR_LOG("failed to parse address for client");
        goto failed;
    }

    sock_addrlen = sizeof(sock_addr);
    if (getsockname(fd, &sock_addr, &sock_addrlen) == -1) {
        OCTOPUS_ERROR_LOG_BY_ERRNO("failed to get socket name");
        goto failed;
    }

    if (addr_parse(&sock_addr, sock_host, 16, &sock_port) == OCTOPUS_ERR) {
        OCTOPUS_ERROR_LOG("failed to parse address for listen socket");
        goto failed;
    }

    OCTOPUS_TRACE_LOG("receive a new connection at %s:%u, cli addr: %s:%u", sock_host, sock_port,
            cli->host, cli->port);

    srv_ctx = (triple_t *)cli_data;
    protocol_factory = (protocol_factory_t)(srv_ctx->first);
    processor_factory = (processor_factory_t)(srv_ctx->second);

    cli->fd = cli_fd;
    cli->oct = srv_ctx->third;
    protocol = protocol_factory();
    if (protocol == NULL) {
        OCTOPUS_ERROR_LOG("failed to create protocol for client, cli: %s:%u",
                cli->host, cli->port);
        goto failed;
    }

    cli->protocol_obj = object_create(OBJECT_TYPE_PROTOCOL, protocol);
    if (cli->protocol_obj == NULL) {
        OCTOPUS_ERROR_LOG("failed to create object of protocol for client, cli: %s:%u",
                cli->host, cli->port);
        goto failed;
    }

    processor = processor_factory();
    if (processor == NULL) {
        OCTOPUS_ERROR_LOG("faield to create processor for client, cli: %s:%u",
                cli->host, cli->port);
        goto failed;
    }

    cli->processor_obj = object_create(OBJECT_TYPE_PROCESSOR, processor);
    if (cli->processor_obj == NULL) {
        OCTOPUS_ERROR_LOG("failed to create object of processor for client, cli: %s:%u",
                cli->host, cli->port);
        goto failed;
    }

    if (socket_set_nonblock(cli_fd) == OCTOPUS_ERR) {
        OCTOPUS_ERROR_LOG("failed to set non-block for socket");
        goto failed;
    }

    pool = octopus_ioworker_pool(cli->oct);
    if (pool == NULL) {
        if (aeCreateFileEvent(event_loop, cli_fd, AE_READABLE, process_input_bytestream, cli)
                == AE_ERR) {
            OCTOPUS_ERROR_LOG("failed to add file event for new client");
            goto failed;
        }
    } else {
        if (ioworker_pool_add_client(pool, cli) == OCTOPUS_ERR) {
            OCTOPUS_ERROR_LOG("failed to add client to ioworker pool");
            goto failed;
        }
    }
    OCTOPUS_TRACE_LOG("add file event, fd: %d", cli_fd);

    OCTOPUS_TRACE_LOG("successful to init client(%s:%u), ready to process request",
            cli->host, cli->port);

    return;

failed:
    client_destroy(cli);
}

void process_input_bytestream(struct aeEventLoop *event_loop, int fd, void *cli_data, int mask) {
    client_t    *cli;
    int         data_read, read_size;
    object_t    *result_cmd_obj, *input_cmd_obj;
    iterator_t  *cmd_obj_iter;
    protocol_t  *protocol;
    processor_t *processor;

    cli = (client_t *)cli_data;

    assert(cli != NULL);
    assert(cli->processor_obj != NULL);
    assert(cli->protocol_obj != NULL);

    protocol = cli->protocol_obj->obj.protocol;
    processor = cli->processor_obj->obj.processor;

    do {
        read_size = buffer_space_remaining(cli->inbuf) > READ_SOCK_UNIT_BYTES ?
                READ_SOCK_UNIT_BYTES : buffer_space_remaining(cli->inbuf);

        if (read_size == 0) {
            OCTOPUS_DEBUG_LOG("client buffer is full, cli: %s:%d", cli->host, cli->port);
            return;
        }

        // 1. read data from socket to buffer
        if ((data_read = buffer_write_from_fd(cli->inbuf, fd, read_size)) == OCTOPUS_ERR) {
            OCTOPUS_ERROR_LOG("failed to read data from socket, endpoint: %s:%d", cli->host, cli->port);
            return;
        } else if (data_read == OCTOPUS_EOF) {
            // client has closed
            OCTOPUS_TRACE_LOG("client has closed, cli: %s:%d", cli->host, cli->port);
            client_destroy(cli);
            OCTOPUS_TRACE_LOG("rm file event, fd: %d", fd);
            aeDeleteFileEvent(event_loop, fd, mask);
            return;
        } else if (data_read == 0) {
            // EAGAIN, no data to read
            OCTOPUS_TRACE_LOG("EAGAIN, no data read, fd: %d", fd);
            break;
        }

        // data_read > 0 means there is data need to read

        // 2. call protocol decoder to decode the buffer, and generate commands
        if (protocol->decode(protocol, cli->inbuf, cli->input_cmd_objs) == OCTOPUS_ERR) {
            OCTOPUS_ERROR_LOG("failed to decode, client will be closed, endpoint: %s:%d",
                    cli->host, cli->port);
            client_destroy(cli);
            return;
        }

        // 3. process commands
        OCTOPUS_DEBUG_LOG("decode %d commands", list_size(cli->input_cmd_objs));

        cmd_obj_iter = cli->input_cmd_objs_iter;
        for (list_iter_init(cli->input_cmd_objs, cmd_obj_iter); cmd_obj_iter->has_next(cmd_obj_iter);) {
            input_cmd_obj = cmd_obj_iter->next(cmd_obj_iter);
            // increase refcnt for iterator
            input_cmd_obj->incr(input_cmd_obj);
            result_cmd_obj = processor->process(processor, input_cmd_obj);
            if (result_cmd_obj == NULL) {
                OCTOPUS_ERROR_LOG("a null command for response, cli: %s:%d", cli->host, cli->port);
                continue;
            }

            // 4. encode response
            if (protocol->encode(protocol, result_cmd_obj, cli->outbuf) == OCTOPUS_ERR) {
                OCTOPUS_ERROR_LOG("failed to encode command, endpoint: %s:%d", cli->host, cli->port);
                continue;
            }

            result_cmd_obj->decr(result_cmd_obj);

            // remote the output command that has been processed
            cmd_obj_iter->remove(cmd_obj_iter);
            // decrease refcnt for iterator
            input_cmd_obj->decr(input_cmd_obj);
        }

        // 5. add write event to event loop
        if (buffer_content_len(cli->outbuf) > 0) {
            if (aeCreateFileEvent(event_loop, fd, AE_WRITABLE, output_response, cli) == AE_ERR) {
                OCTOPUS_ERROR_LOG("failed to add write event to event loop");
                return;
            }
        }
    } while (1);
}

void output_response(struct aeEventLoop *event_loop, int fd, void *cli_data, int mask) {
    client_t    *cli;
    int         write_size, data_written;

    OCTOPUS_NOT_USED(mask);

    cli = (client_t *)cli_data;
    while (buffer_content_len(cli->outbuf) > 0) {
        write_size = WRITE_SOCK_UNIT_BYTES;
        if (buffer_content_len(cli->outbuf) < WRITE_SOCK_UNIT_BYTES) {
            write_size = buffer_content_len(cli->outbuf);
        }

        if ((data_written = buffer_read_to_fd(cli->outbuf, fd, write_size)) == OCTOPUS_ERR) {
            OCTOPUS_ERROR_LOG("failed to read buffer to socket, endpoint: %s:%d",
                    cli->host, cli->port);
            return;
        } else if (data_written < write_size) {
            // send buffer is full, need to wait
            break;
        } else if (data_written == OCTOPUS_RESET) {
            OCTOPUS_ERROR_LOG("connection has been reset, close client, client: %s:%d",
                    cli->host, cli->port);
            aeDeleteFileEvent(event_loop, fd, AE_WRITABLE | AE_READABLE);
            client_destroy(cli);

            return;
        }
    }

    // all output buffer has been send, need remove write event handler
    if (buffer_content_len(cli->outbuf) == 0) {
        OCTOPUS_TRACE_LOG("rm file event, fd: %d", fd);
        aeDeleteFileEvent(event_loop, fd, AE_WRITABLE);
    }

    return;
}
