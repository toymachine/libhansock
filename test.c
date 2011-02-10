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

int test(Connection *connection, int init_index)
{
    Batch *batch = Batch_new();
    Executor *executor = Executor_new();

    //setup some commands
    char *cmd;
    if(init_index) {
        cmd = "P\t1\tconcurrence_test\ttbltest\tPRIMARY\ttest_id,test_string\n";
        Batch_write(batch, cmd, strlen(cmd), 1);
    }

    cmd = "1\t=\t1\t9\n";
    Batch_write(batch, cmd, strlen(cmd), 1);

    //associate batch with connections
    Executor_add(executor, connection, batch);

    //execute it
    if(Executor_execute(executor, 500) <= 0) {
        return -1;
    }
    else {
        ReplyIterator *replies = Batch_get_replies(batch);
        while(ReplyIterator_next(replies)) {
            ReplyType reply_type;
            char *reply_data;
            size_t reply_len;
            ReplyIterator_get_reply(replies, &reply_type, &reply_data, &reply_len);
            //printf("reply type: %d, data: '%.*s'\n", (int)reply_type, reply_len, reply_data);
            ReplyIterator *children = ReplyIterator_child_iterator(replies);
            while(ReplyIterator_next(children)) {
                ReplyType child_reply_type;
                char *child_reply_data;
                size_t child_reply_len;
                ReplyIterator_get_reply(children, &child_reply_type, &child_reply_data, &child_reply_len);
                //printf("\tchild reply type: %d, data: '%.*s'\n", (int)child_reply_type, child_reply_len, child_reply_data);
            }
            ReplyIterator_free(children);
        }
        ReplyIterator_free(replies);
    }

    //release all resources
    Executor_free(executor);
    Batch_free(batch);

    return 0;
}

int main(int argc, char *argv[])
{
    Module *module = Module_new();
    Module_init(module);

    //create our basic object
    Connection *connection = Connection_new("127.0.0.1:9998");

    for(int i = 0; i < 1000000; i++) {
        test(connection, i == 0);
    }

    Connection_free(connection);

    Module_free(module);

}
