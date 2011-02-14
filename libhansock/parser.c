/**
* Copyright (C) 2010 - 2011, Hyves (Startphone Ltd.)
*
* This module is part of Libhansock (http://github.com/toymachine/libhansock) and is released under
* the New BSD License: http://www.opensource.org/licenses/bsd-license.php
*
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdlib.h>

#include "common.h"
#include "alloc.h"
#include "parser.h"

#define MARK rp->mark = rp->p

struct _ReplyParser
{
    size_t p; //position
    int cs; //state
    Reply *reply;
    size_t mark; //helper to mark start of interesting data
    int encoded; //whether we are currently parsing an encoded string
};

void ReplyParser_reset(ReplyParser *rp, int p)
{
    rp->p = p;
    rp->cs = 0;
    rp->mark = 0;
    rp->reply = NULL;
    rp->encoded = 0;
}

ReplyParser *ReplyParser_new()
{
    DEBUG(("alloc ReplyParser\n"));
    ReplyParser *rp = Alloc_alloc_T(ReplyParser);
    if(rp == NULL) {
        Module_set_error(GET_MODULE(), "Out of memory while allocating ReplyParser");
        return NULL;
    }
    ReplyParser_reset(rp, 0);
    return rp;
}

void ReplyParser_free(ReplyParser *rp)
{
    if(rp == NULL) {
        return;
    }
    DEBUG(("dealloc ReplyParser\n"));
    Alloc_free_T(rp, ReplyParser);
}


/**
 * A State machine for parsing replies.
 */
ReplyParserResult ReplyParser_execute(ReplyParser *rp, const char *data, size_t len, Reply **reply)
{
    DEBUG(("enter rp exec, rp->p: %d, len: %d, cs: %d\n", rp->p, len, rp->cs));
    assert(rp->p <= len);
    *reply = NULL;
    while((rp->p) < len) {
        Byte c = data[rp->p];
        //printf("cs: %d, char: %d\n", rp->cs, c);
        switch(rp->cs) {
            case 0: { //initial state
                assert(rp->reply == NULL);
                rp->reply = Reply_new(RT_LINE, NULL, 0, 0);
                //fall trough to state 1
                rp->cs = 1;
                MARK;
            }
            case 1: { //normal state
                if(c >= 16 && c <= 255) {
                    //NORMAL unencoded char, most common
                    rp->p++;
                    rp->cs = 1;
                    continue;
                }
                else if(c == 0x09) { //TAB
                    //end of string
                    assert(rp->reply != NULL);
                    Reply_add_child(rp->reply, Reply_new(rp->encoded ? RT_ENCODED_STRING : RT_STRING, data, rp->mark, rp->p - rp->mark));
                    rp->p++;
                    rp->cs = 1;
                    rp->encoded = 0;
                    MARK;
                    continue;
                }
                else if(c == 0x0A) { //EOL
                    assert(rp->reply != NULL);
                    Reply_add_child(rp->reply, Reply_new(rp->encoded ? RT_ENCODED_STRING : RT_STRING, data, rp->mark, rp->p - rp->mark));
                    *reply = rp->reply;
                    ReplyParser_reset(rp, ++rp->p);
                    return RPR_REPLY;
                }
                else if(c == 0x00) { //NULL
                    rp->p++;
                    rp->cs = 2;
                    continue;
                }
                else if(c == 0x01) { //start of encoded char
                    rp->p++;
                    rp->cs = 3;
                    rp->encoded = 1;
                    continue;
                }
                break;
            }
            case 2: {
                // after reading a NULL
                // here we expect either a TAB or EOL
                if(c == 0x09) { //TAB
                    assert(rp->reply != NULL);
                    Reply_add_child(rp->reply, Reply_new(RT_NULL, NULL, 0, 0));
                    rp->p++;
                    rp->cs = 1;
                    MARK;
                    continue;
                }
                else if(c == 0x0A) { //EOL
                    assert(rp->reply != NULL);
                    Reply_add_child(rp->reply, Reply_new(RT_NULL, NULL, 0, 0));
                    *reply = rp->reply;
                    ReplyParser_reset(rp, ++rp->p);
                    return RPR_REPLY;
                }
                break;
            }
            case 3: {
                // next char should be encoded char
                if((c >= 0x40) && (c <= 0x4f)) {
                    //OK
                    rp->p++;
                    rp->cs = 1;
                    continue;
                }
                break;
            }
            default: {
                //will return error
            }
        }
        return RPR_ERROR;
    }
    DEBUG(("exit rp pos: %d len: %d cs: %d\n", rp->p, len, rp->cs));
    assert(rp->p == len);
    return RPR_MORE;
}

