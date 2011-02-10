/**
* Copyright (C) 2010, Hyves (Startphone Ltd.)
*
* This module is part of Libredis (http://github.com/toymachine/libredis) and is released under
* the New BSD License: http://www.opensource.org/licenses/bsd-license.php
*
*/

#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "common.h"
#include "alloc.h"
#include "batch.h"
#include "list.h"
#include "reply.h"

struct _Batch
{
#ifdef SINGLETHREADED
    struct list_head list; //for creating lists of batches
#endif

    int num_commands;
    struct list_head reply_queue; //finished commands that have replies set

    Buffer *write_buffer;
    Buffer *read_buffer;

    //error for aborted batch
    Buffer *error;
};

struct _Reply
{
    struct list_head list; //for creating lists of replies

    ReplyType type;

    const char *data;
    size_t offset;
    size_t len;

    struct list_head children;
};

ALLOC_LIST_T(Reply, list)

Reply *Reply_new(ReplyType type, const char *data, size_t offset, size_t len)
{
    Reply *reply;
    Reply_list_alloc(&reply);
    DEBUG(("Reply_new, type: %d, offset: %d, len: %d\n", type, offset, len));
    reply->type = type;
    reply->data = data;
    reply->offset = offset;
    reply->len = len;
    INIT_LIST_HEAD(&reply->children);
    return reply;
}

void _Reply_free(Reply *reply, int final)
{
    if(!final) {
        while(!list_empty(&reply->children)) {
            Reply *child = list_pop_T(Reply, list, &reply->children);
            Reply_free(child);
        }
    }
    Reply_list_free(reply, final);
}

void Reply_add_child(Reply *reply, Reply *child)
{
    list_add_tail(&child->list, &reply->children);
}

int Reply_has_child(Reply *reply)
{
    return !list_empty(&reply->children);
}

Reply *Reply_pop_child(Reply *reply)
{
    return list_pop_T(Reply, list, &reply->children);
}

size_t Reply_length(Reply *reply)
{
    return reply->len;
}

Byte *Reply_data(Reply *reply)
{
    assert(reply->data != NULL);
    return (Byte *)reply->data + reply->offset;
}

ReplyType Reply_type(Reply *reply)
{
    return reply->type;
}


struct _ReplyIterator
{
#ifdef SINGLETHREADED
    struct list_head list; //for creating lists of reply iterators
#endif

    struct list_head *head;
    struct list_head *current;
};

ALLOC_LIST_T(ReplyIterator, list)

ReplyIterator *ReplyIterator_new(struct list_head *replies)
{
    ReplyIterator *iterator;
    ReplyIterator_list_alloc(&iterator);
    iterator->head = replies;
    iterator->current = iterator->head;
    return iterator;
}

void _ReplyIterator_free(ReplyIterator *iterator, int final)
{
    ReplyIterator_list_free(iterator, final);
}

int ReplyIterator_next(ReplyIterator *iterator)
{
    iterator->current = iterator->current->next;
    return iterator->current != iterator->head;
}

int ReplyIterator_get_reply(ReplyIterator *iterator, ReplyType *reply_type, char **data, size_t *len)
{
    if(iterator->current != iterator->head) {
        Reply *current_reply = list_entry(iterator->current, Reply, list);
        *reply_type = current_reply->type;
        *len = current_reply->len;
        if(current_reply->type == RT_ERROR ||
           current_reply->type == RT_STRING ||
           current_reply->type == RT_ENCODED_STRING) {
                *data = Reply_data(current_reply);
        }
        else {
            *data = NULL;
        }
        return 0;
    }
    else {
        return -1; //TODO set module error
    }
}

ReplyIterator *ReplyIterator_child_iterator(ReplyIterator *iterator)
{
    if(iterator->current != iterator->head) {
        Reply *current_reply = list_entry(iterator->current, Reply, list);
        if(list_empty(&current_reply->children)) {
            return NULL;
        }
        else {
            return ReplyIterator_new(&current_reply->children);
        }
    }
    else {
        return NULL;
    }
}

ALLOC_LIST_T(Batch, list)

Batch *Batch_new()
{
    Batch *batch;
    if(Batch_list_alloc(&batch)) {
        batch->read_buffer = Buffer_new(DEFAULT_READ_BUFF_SIZE);
        batch->write_buffer = Buffer_new(DEFAULT_WRITE_BUFF_SIZE);
    }
    batch->num_commands = 0;
    INIT_LIST_HEAD(&batch->reply_queue);

    batch->error = NULL;

    return batch;
}

void _Batch_reply_list_free(struct list_head *list_to_free)
{
    while(!list_empty(list_to_free)) {
        DEBUG(("_Batch_reply_list_free\n"));
        Reply *reply = list_pop_T(Reply, list, list_to_free);
        _Batch_reply_list_free(&reply->children);
        Reply_free(reply);
    }
}

void _Batch_free(Batch *batch, int final)
{
    DEBUG(("_Batch_free\n"));
    /*
    while(!list_empty(&batch->reply_queue)) {
        Reply *reply = list_pop_T(Reply, list, &batch->reply_queue);
        Reply_free(reply);
    }
    */
    _Batch_reply_list_free(&batch->reply_queue);
    if(final) {
        DEBUG(("_Batch_free final\n"));
        Buffer_free(batch->read_buffer);
        Buffer_free(batch->write_buffer);
    }
    else {
        DEBUG(("_Batch_free re-use\n"));
        Buffer_clear(batch->read_buffer);
        Buffer_clear(batch->write_buffer);
#ifndef NDEBUG
        Buffer_fill(batch->read_buffer, (Byte)0xEA);
        Buffer_fill(batch->write_buffer, (Byte)0xEA);
#else
        Buffer_fill(batch->read_buffer, 0);
        Buffer_fill(batch->write_buffer, 0);
#endif
    }
    if (batch->error) {
        Buffer_free(batch->error);
        batch->error = NULL;
    }
    Batch_list_free(batch, final);
}

void Batch_write(Batch *batch, const char *str, size_t str_len, int num_commands)
{
    if(str != NULL && str_len > 0) {
        Buffer_write(batch->write_buffer, str, str_len);
    }
    batch->num_commands += num_commands;
}

void Batch_write_decimal(Batch *batch, long decimal)
{
    char buff[32];
    int buff_len = snprintf(buff, 32, "%ld", decimal);
    Batch_write(batch, buff, buff_len, 0);
}

int Batch_has_command(Batch *batch)
{
    return batch->num_commands > 0;
}


void Batch_add_reply(Batch *batch, Reply *reply)
{
    DEBUG(("pop cmd from command queue\n"));
    DEBUG(("add reply/cmd back to reply queue\n"));
    batch->num_commands -= 1;
    list_add_tail(&reply->list, &batch->reply_queue);
}

ReplyIterator *Batch_get_replies(Batch *batch)
{
    return ReplyIterator_new(&batch->reply_queue);
}

char *Batch_error(Batch *batch)
{
    if(batch->error) {
        return Buffer_data(batch->error);
    }
    else {
        return NULL;
    }
}

void Batch_abort(Batch *batch, const char *error)
{
    DEBUG(("Batch abort\n"));
    assert(batch->error == NULL);
    int length = strlen(error) + 1; //include terminating null character
    batch->error = Buffer_new(length);
    Buffer_write(batch->error, error, length);
    while(Batch_has_command(batch)) {
        DEBUG(("Batch abort, adding error reply\n"));
        Batch_add_reply(batch, Reply_new(RT_ERROR, Buffer_data(batch->error), 0, length));
    }
}

Buffer *Batch_read_buffer(Batch *batch)
{
    return batch->read_buffer;
}

Buffer *Batch_write_buffer(Batch *batch)
{
    return batch->write_buffer;
}

