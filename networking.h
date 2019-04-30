/**
 *
 * @file    networking
 * @author  chosen0ne(louzhenlin86@126.com)
 * @date    2018-12-27 11:07:58
 */

#ifndef OCTOPUS_NETWORKING_H
#define OCTOPUS_NETWORKING_H

#include <netdb.h>
#include <sys/socket.h>

#include <libae/ae.h>

#include "common.h"

int tcp_nonblk_srv(int sockfd, struct addrinfo *addr, int backlog);
int addr_parse(struct sockaddr *addr, char *ipbuf, int ipbuf_size, uint16_t *port);
void client_connected(struct aeEventLoop *event_loop, int fd, void *cli_data, int mask);
void process_input_bytestream(struct aeEventLoop *event_loop, int fd, void *cli_data, int mask);
void output_response(struct aeEventLoop *event_loop, int fd, void *cli_data, int mask);

#endif /* ifndef OCTOPUS_NETWORKING_H */
