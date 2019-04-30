/**
 *
 * @file    command
 * @author  chosen0ne(louzhenlin86@126.com)
 * @date    2019-01-08 17:06:10
 */

#include "command.h"
#include "logging.h"

void command_incr(command_t *cmd) {
    cmd->refcnt++;
}

void command_release(command_t *cmd) {
    cmd->refcnt--;

    if (cmd->refcnt == 0) {
        cmd->destroy(cmd);
    } else if (cmd->refcnt < 0) {
        OCTOPUS_ERROR_LOG("fatal error: refcnt < 0");
    }
}
