/**
 *
 * @file    command
 * @author  chosen0ne(louzhenlin86@126.com)
 * @date    2019-01-07 18:54:31
 */

#ifndef OCTOPUS_COMMAND_H
#define OCTOPUS_COMMAND_H

#define command_t_implement     \
    void (*destroy)(command_t *cmd);

typedef struct command_s command_t;

struct command_s {
    /**
     * use to destroy the command_t, which will release the resourses belongs to cmd.
     */
    void (*destroy)(command_t *cmd);
};

#endif /* ifndef OCTOPUS_COMMAND_H */
