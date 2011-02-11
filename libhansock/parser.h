/**
* Copyright (C) 2010 - 2011, Hyves (Startphone Ltd.)
*
* This module is part of Libhansock (http://github.com/toymachine/libhansock) and is released under
* the New BSD License: http://www.opensource.org/licenses/bsd-license.php
*
*/

#ifndef __PARSER_H
#define __PARSER_H

#include "reply.h"

typedef struct _ReplyParser ReplyParser;

typedef enum _ReplyParserResult
{
    RPR_ERROR = 0,
    RPR_MORE = 1,
    RPR_REPLY = 2
} ReplyParserResult;

ReplyParser *ReplyParser_new();
void ReplyParser_reset(ReplyParser *rp, int p);
void ReplyParser_free(ReplyParser *rp);
ReplyParserResult ReplyParser_execute(ReplyParser *rp, const char *data, size_t len, Reply **reply);

#endif
