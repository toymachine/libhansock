/**
* Copyright (C) 2010 - 2011, Hyves (Startphone Ltd.)
*
* This module is part of Libhansock (http://github.com/toymachine/libhansock) and is released under
* the New BSD License: http://www.opensource.org/licenses/bsd-license.php
*
*/

/*
 * This is the public C API of the libhansock library.
 *
 * It provides a low-level and fast client driver for the handlersocket mysql extension.
 *
 * This client supports proper timeouts, and uses asynchronous socket IO to be able to talk to many servers in parallel.
 *
 * The library should be initialized first using the 'Module_new' function to get reference to the library.
 * Then you can set some optional properties (using 'Module_set_XXX' functions) to configure the library.
 * Finally you should call Module_init to initialize the library proper.
 * At the end of your program, call 'Module_free' to release all resources in use by the library.
 *
 * The API is written in a 'pseudo' object oriented style, using only opaque references to the Batch, Connection and Executor 'Classes'.
 * Each of these will be created and destroyed using their respective XXX_new() and XXX_free methods.
 *
 * In order to communicate with a handlersocket server you will need at least to create an instance of Batch, Connection and Executor. e.g.:
 *
 * TODO describe primary API!
 */
#ifndef HANSOCK_H
#define HANSOCK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>

typedef struct _Module Module;
typedef struct _Batch Batch;
typedef struct _Connection Connection;
typedef struct _Ketama Ketama;
typedef struct _Executor Executor;
typedef struct _ReplyIterator ReplyIterator;

#define LIBHANSOCKAPI __attribute__((visibility("default")))


/*
 * Create a new instance of the library. (Currently this just returns the same global instance. This might change in the future)
 */
LIBHANSOCKAPI Module *Module_new();

/**
 * Override various memory-management functons to be used by libhansock. You don't have to set these,
 * libhansock will use normal alloc/realloc/free in that case.
 */
LIBHANSOCKAPI void Module_set_alloc_alloc(Module *module, void * (*alloc_malloc)());
LIBHANSOCKAPI void Module_set_alloc_realloc(Module *module, void * (*alloc_realloc)(void *, size_t));
LIBHANSOCKAPI void Module_set_alloc_free(Module *module, void (*alloc_free)(void *));

/**
 * Initialise the libhansock module once all properties have been set. The library is now ready to be used.
 * Returns -1 if there is an error, 0 if all is ok.
 */
LIBHANSOCKAPI int Module_init(Module *module);

/**
 * Gets the amount of heap memory currently allocated by the libhansock module. This should return 0 after the module has been freed.
 */
LIBHANSOCKAPI size_t Module_get_allocated(Module *module);

/**
 * Gets a textual description of the last error that occurred.
 */
LIBHANSOCKAPI char *Module_last_error(Module *module);

/**
 * Release all resources still held by libhansock. The library cannot be used anymore after this call.
 */
LIBHANSOCKAPI void Module_free(Module *module);

/**
 * Create a new connection to a Handlersocket endpoint. addr should be a string <hostname:port> or <ip-address:port>.
 * If the port (and colon) part is omitted the default HS port of 9998 will be used.
 * Note that the actual connection will not be made at this point. It will open the connection as soon as the first command
 * will be written to HS.
 */
LIBHANSOCKAPI Connection *Connection_new(const char *addr);

/**
 * Release all resources held by the connection.
 */
LIBHANSOCKAPI void Connection_free(Connection *connection);

/**
 * Enumerates the type of replies that can be read from a Batch.
 */
typedef enum _ReplyType
{
    RT_ERROR = -1,
    RT_LINE = 1,
    RT_STRING = 2,
    RT_NULL = 3,
    RT_ENCODED_STRING = 4
} ReplyType;

/**
 * Create a new Batch of commands
 */
LIBHANSOCKAPI Batch *Batch_new();

/**
 * Release all resources of the given batch
 */
LIBHANSOCKAPI void Batch_free(Batch *batch);

/**
 * Writes a command or part of a command into the batch. The batch will keep an internal pointer to the last written
 * position, so that the next write will be appended from there. The batch will automatically grow in size.
 * The num_commands argument specifies how many commands are added by this write. It might be 0 if you are writing a command in parts
 * It is possible to call the write method without a string, just to set the number of commands in the batch.
 * In that case pass NULL for str and 0 for str_len.
 */
LIBHANSOCKAPI void Batch_write(Batch *batch, const char *str, size_t str_len, int num_commands);

/**
 * Write a decimal into the batch (as a string, like using %d in a printf call).
 */
LIBHANSOCKAPI void Batch_write_decimal(Batch *batch, long decimal);

/**
 * If a batch was aborted (maybe because a connection went down or timed-out), there will be an error message
 * associated with the batch. Use this function to retrieve it.
 * Returns NULL if there was no error in the batch.
 */
LIBHANSOCKAPI char *Batch_error(Batch *batch);

/**
 * Gets the replies from this batch as an iterator. YOU are responsible for freeing the iterator when you are done iterating.
 * Some replies have child replies. You can call ReplyIterator_child_iterator to get an iterator over these children.
 */
LIBHANSOCKAPI ReplyIterator *Batch_get_replies(Batch *batch);

/**
 * Advances the iterator to the next reply. returns 0 when there are no more replies to read.
 */
LIBHANSOCKAPI int ReplyIterator_next(ReplyIterator *iterator);

/**
 * Fetch the data associated with the reply currently pointed to by the iterator.
 */
LIBHANSOCKAPI int ReplyIterator_get_reply(ReplyIterator *iterator, ReplyType *reply_type, char **data, size_t *len);

/*
 * Get an iterator over the children of the reply pointed to by the given iterator.
 * Returns NULL if there are no children.
 */
LIBHANSOCKAPI ReplyIterator *ReplyIterator_child_iterator(ReplyIterator *iterator);

/**
 * Releases any resources held by the reply iterator
 */
LIBHANSOCKAPI void ReplyIterator_free(ReplyIterator *iterator);

/**
 * Creates a new empty Executor
 */
LIBHANSOCKAPI Executor *Executor_new();

/**
 * Frees any resources held by the Executor
 */
LIBHANSOCKAPI void Executor_free(Executor *executor);

/**
 * Associate a batch with a connection. When execute is called the commands from the batch
 * will be executed on the given connection.
 * Returns 0 if all ok, -1 if there was an error making the association.
 */
LIBHANSOCKAPI int Executor_add(Executor *executor, Connection *connection, Batch *batch);

/**
 * Execute all associated (connection, batch) pairs within the given timeout. The commands
 * contained by the batches will be send to their respective connections, and the replies to these commands
 * will be gathered in their respective batches.
 * When all batches complete within the timeout, the result of this function is 1.
 * If a timeout occurs before completion. the result of this function is 0, and all commands in all batches that
 * were not completed at the time of timeout will contain an timeout error reply.
 * If 1 or more batches had an error, the function will return -1. The respective batches will have their errors set.
 * If there is an error with this method itself (e.g. the polling or select method), it will return -2 (And you consult the global module error).
 */
LIBHANSOCKAPI int Executor_execute(Executor *executor, int timeout_ms);


/**
* Create a new ketama consistent hashing object.
* The implementation was taken more or less straight from the original libketama,
* for info on this see: http://www.audioscrobbler.net/development/ketama/
* The API was changed a bit to fit the coding style of libhansock. plus all
* the shared memory stuff was removed as I thought that to be too specific for a general library.
* A good explanation of consistent hashing can be found here:
* http://www.tomkleinpeter.com/2008/03/17/programmers-toolbox-part-3-consistent-hashing/
* The ketama algorithm is widely used in many clients (for instance memcached clients).
*
* Basic usage:
*
* Ketama *ketama = Ketama_new(); //create a new ketama object
* //add your server list:
* Ketama_add_server(ketama, '127.0.0.1', 6379, 100);
* Ketama_add_server(ketama, '127.0.0.1', 6390, 125);
* Ketama_add_server(ketama, '10.0.0.18', 6379, 90);
* // ... etc etc
* // Then create the hash-ring by calling
* Ketama_create_continuum(ketama);
* // Now your ketama object is ready for use.
* // You can map a key to some server like this
* char *my_key = "foobar42";
* int ordinal = Ketama_get_server_ordinal(ketama, my_key, strlen(my_key));
* char *server_address = Ketama_get_server_address(ketama, ordinal);
*/
LIBHANSOCKAPI Ketama *Ketama_new();

/**
 * Frees any resources held by the ketama object.
 */
LIBHANSOCKAPI void Ketama_free(Ketama *ketama);

/**
 * Add a server to the hash-ring. This must be called (repeatedly) BEFORE calling Ketama_create_continuum.
 * Address must be an ip-address or hostname of a server. port is the servers port number.
 * The weight is the relative weight of this server in the ring.
 */
LIBHANSOCKAPI int Ketama_add_server(Ketama *ketama, const char *addr, int port, unsigned long weight);

/**
 * After all servers have been added call this method to finalize the hash-ring before use.
 */
LIBHANSOCKAPI void Ketama_create_continuum(Ketama *ketama);

/**
 * Hash the given key to some server (denoted by ordinal). key_len is the length of the key in bytes.
 */
LIBHANSOCKAPI int Ketama_get_server_ordinal(Ketama *ketama, const char* key, size_t key_len);

/**
 * Return the address of the server as a string "address:port" as passed to the original call to Ketama_add_server
 */
LIBHANSOCKAPI char *Ketama_get_server_address(Ketama *ketama, int ordinal);

#ifdef __cplusplus
}
#endif

#endif
