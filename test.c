/**
* Copyright (C) 2010, Hyves (Startphone Ltd.)
*
* This module is part of Libredis (http://github.com/toymachine/libredis) and is released under
* the New BSD License: http://www.opensource.org/licenses/bsd-license.php
*
*/

#include <stdio.h>

#include "libhansock/hansock.h"
#include "libhansock/parser.h"

int main(int argc, char *argv[])
{
    int error = 0;

    Module *module = Module_new();
    Module_init(module);

    ReplyParser *parser = ReplyParser_new();

    Buffer *buffer = Buffer_new(128);

    //char *s = "ab\tcd\tef\n";

    char *s = "aa\t\x0001\x0040\txx\n";

    Buffer_write(buffer, s, 20);
    //Buffer_flip(buffer);
    Buffer_dump(buffer, 128);

    Reply *reply = NULL;
    ReplyParser_execute(parser, Buffer_data(buffer), Buffer_position(buffer), &reply);

    /*

    //create our basic object
    Batch *batch = Batch_new();
    Connection *connection = Connection_new("127.0.0.1:9998");
    Executor *executor = Executor_new();

    //setup some commands
    char *cmd;
    cmd = "P\t1\tconcurrence_test\ttbltest\tPRIMARY\ttest_id,test_string\n";
    Batch_write(batch, cmd, strlen(cmd), 1);
    cmd = "1\t=\t1\t9\n";
    Batch_write(batch, cmd, strlen(cmd), 1);

    //associate batch with connections
    Executor_add(executor, connection, batch);

    //execute it
    if(Executor_execute(executor, 500) <= 0) {
        printf("executor error: %s", Module_last_error(module));
        error = 1;
    }
    else {
        //read out replies
        ReplyType reply_type;
        char *reply_data;
        size_t reply_len;
        int level;
        while((level = Batch_next_reply(batch, &reply_type, &reply_data, &reply_len))) {
            printf("level: %d, reply type: %d, data: '%.*s'\n", level, (int)reply_type, reply_len, reply_data);
        }
    }

    //release all resources
    Executor_free(executor);
    Batch_free(batch);
    Connection_free(connection);
    */
    Buffer_free(buffer);
    ReplyParser_free(parser);
    Module_free(module);

    return error;
}
