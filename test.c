/**
* Copyright (C) 2010, Hyves (Startphone Ltd.)
*
* This module is part of Libredis (http://github.com/toymachine/libredis) and is released under
* the New BSD License: http://www.opensource.org/licenses/bsd-license.php
*
*/

#include <stdio.h>
#include <assert.h>

#include "libhansock/hansock.h"
#include "libhansock/parser.h"

int main(int argc, char *argv[])
{
    int error = 0;

    Module *module = Module_new();
    Module_init(module);

    /*
    ReplyParser *parser = ReplyParser_new();
    Buffer *buffer = Buffer_new(128);

    //char *s = "ab\tcd\tef\n";
    //char *s = "aa\t\x0001\x0040\txx\n";
    //char *s = "\x0000\t\x0000\t\x0000\n";
    char *s = "a\t\n";

    Buffer_write(buffer, s, 20);
    //Buffer_flip(buffer);
    Buffer_dump(buffer, 128);

    Reply *reply = NULL;

    ReplyParserResult result = ReplyParser_execute(parser, Buffer_data(buffer), Buffer_position(buffer), &reply);
    assert(RPR_REPLY == result);
    printf("result: %d\n", result);

    Buffer_free(buffer);
    ReplyParser_free(parser);

    */

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
        ReplyIterator *replies = Batch_get_replies(batch);
        while(ReplyIterator_next(replies)) {
            ReplyType reply_type;
            char *reply_data;
            size_t reply_len;
            ReplyIterator_get_reply(replies, &reply_type, &reply_data, &reply_len);
            printf("reply type: %d, data: '%.*s'\n", (int)reply_type, reply_len, reply_data);
            ReplyIterator *children = ReplyIterator_child_iterator(replies);
            while(ReplyIterator_next(children)) {
                ReplyType child_reply_type;
                char *child_reply_data;
                size_t child_reply_len;
                ReplyIterator_get_reply(children, &child_reply_type, &child_reply_data, &child_reply_len);
                printf("\tchild reply type: %d, data: '%.*s'\n", (int)child_reply_type, child_reply_len, child_reply_data);
            }
            ReplyIterator_free(children);
        }
        ReplyIterator_free(replies);
    }

    //release all resources
    Executor_free(executor);
    Batch_free(batch);
    Connection_free(connection);

    Module_free(module);

    return error;
}
