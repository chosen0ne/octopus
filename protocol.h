/**
 *
 * @file    protocol
 * @author  chosen0ne(louzhenlin86@126.com)
 * @date    2018-12-27 11:46:19
 */

#ifndef OCTOPUS_PROTOCOL_H
#define OCTOPUS_PROTOCOL_H

#include "buffer.h"
#include "list.h"
#include "common.h"

#define protocol_t_implement    \
    decode_t    decode; \
    encode_t    encode; \
    protocol_destroy_t  destroy

typedef struct protocol_s protocol_t;

/**
 * A factory used to create a specified protocol which is used
 * to decode a command from byte stream and encode a command
 * to byte stream.
 */
typedef protocol_t* (*protocol_factory_t)();

/**
 * byte stream => command
 * Protocol decoder used to decode commands from the input byte stream.
 * Parameters for 'protocol_decoder_t' are:
 *  @param [in]state, state of the decoder. Different protocols have different
 *          state, so the type of state is void*.
 *  @param [in]input, byte stream read from the socket.
 *  @param [out]output_cmd_objs, a linked list used to store the objects of command
 *          decoded, and the type of the element is object_t*. In object_t*, a user
 *          customed command_t* is holded.
 *  @return int, OCTOPUS_OK if succeed, or OCTOPUS_ERR if failed.
 */
typedef int (*decode_t)(void *state, buffer_t *input, list_t *output_cmd_objs);

/**
 * command => byte stream
 * Protocol encoder used to encode a command to output byte stream.
 * Parameters for 'protocol_encoder_t' is similar to 'protocol_decoder_t'.
 *  @param [in]state, state of the decoder. Different protocols have different
 *          state, so the type of state is void*.
 *  @param [in]cmd_obj, a object holder of command need to be encoded.
 *  @param [out]output, a buffer to store the bytes of the command.
 *  @return int, OCTOPUS_OK if succeed, or OCTOPUS_ERR if failed.
 */
typedef int (*encode_t)(void *state, object_t *cmd_obj, buffer_t *output);

typedef void (*protocol_destroy_t)(protocol_t *protocol);

struct protocol_s {
    decode_t    decode;
    encode_t    encode;

    protocol_destroy_t  destroy;
};

#endif /* ifndef OCTOPUS_PROTOCOL_H */
