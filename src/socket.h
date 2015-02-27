/*
 * Copyright (C) Tildeslash Ltd. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 *
 * You must obey the GNU Affero General Public License in all respects
 * for all of the code used other than OpenSSL.
 */


#ifndef MONIT_SOCKET_H
#define MONIT_SOCKET_H

#define SOCKET_TCP  1
#define SOCKET_UDP  2


/**
 * This class implements a <b>Socket</b>. A socket is an endpoint for
 * communication between two machines.
 *
 * @file
 */


typedef struct Socket_T *Socket_T;


/**
 * Create a new Socket opened against host:port. The returned Socket
 * is a connected socket. This method can be used to create either TCP
 * or UDP sockets and the type parameter is used to select the socket
 * type. If the use_ssl parameter is true the socket is created using
 * SSL. Only TCP sockets may use SSL.
 * @param host The remote host to open the Socket against. The host
 * may be a hostname found in the DNS or an IP address string.
 * @param port The port number to connect to
 * @param type The socket type to use (SOCKET_TCP or SOCKET_UPD)
 * @param family The socket family to use (see Socket_Family type)
 * @param use_ssl if true the socket is created supporting SSL
 * @param timeout The timeout value in milliseconds
 * @return The connected Socket or NULL if an error occurred
 */
Socket_T socket_new(const char *host, int port, int type, Socket_Family family, boolean_t use_ssl, int timeout);


/**
 * Factory method for creating a new Socket from a monit Port object
 * @param port The port object to create a socket from
 * @return The connected Socket or NULL if an error occurred
 */
Socket_T socket_create(void *port);


/**
 * Create a new Socket opened against host:port with an explicit
 * ssl value for connect and read. Otherwise, same as socket_new()
 * @param host The remote host to open the Socket against. The host
 * may be a hostname found in the DNS or an IP address string.
 * @param port The port number to connect to
 * @param type The socket type to use (SOCKET_TCP or SOCKET_UPD)
 * @param family The socket family to use (see Socket_Family type)
 * @param ssl Options for SSL
 * @param timeout The timeout value in milliseconds
 * @return The connected Socket or NULL if an error occurred
 */
Socket_T socket_create_t(const char *host, int port, int type, Socket_Family family, SslOptions_T ssl, int timeout);


/**
 * Create a new unix Socket for given path for connect and read.
 * Otherwise, same as socket_new().
 * @param path The path to unix socket
 * @param type The socket type to use (SOCKET_TCP or SOCKET_UPD)
 * @param timeout The timeout value in milliseconds
 * @return The connected Socket or NULL if an error occurred
 */
Socket_T socket_create_u(const char *path, int type, int timeout);


/**
 * Factory method for creating a Socket object from an accepted
 * socket. The given socket must be a socket created from accept(2).
 * If the sslserver context is non-null the socket will support
 * ssl. This method does only support TCP sockets.
 * @param socket The accepted socket
 * @param addr The socket address
 * @param sslserver A ssl server connection context, may be NULL
 * @return A Socket or NULL if an error occurred
 */
Socket_T socket_create_a(int socket, struct sockaddr *addr, void *sslserver);


/**
 * Destroy a Socket object. Close the socket and release allocated
 * resources.
 * @param S A Socket object reference
 */
void socket_free(Socket_T *S);


/**
 * Set a read/write <code>timeout</code> in milliseconds. During a read
 * operation the socket will wait up to <code>timeout</code>
 * milliseconds for data to become available if not already present.
 * @param S A Socket object
 * @param timeout The timeout value in milliseconds
 */
void socket_setTimeout(Socket_T S, int timeout);


/**
 * Returns the socket's read/write <code>timeout</code> in milliseconds.
 * @param S A Socket object
 * @return The timeout value in milliseconds
 */
int socket_getTimeout(Socket_T S);


/**
 * Returns true if the socket is ready for i|o
 * @param S A Socket object
 * @return true if the socket is ready otherwise false
 */
boolean_t socket_is_ready(Socket_T S);


/**
 * Return true if the connection is encrypted with SSL
 * @param S A Socket object
 * @return true if SSL is used otherwise false
 */
boolean_t socket_is_secure(Socket_T S);


/**
 * Return true if the connection is over UDP
 * @param S A Socket object
 * @return true if UDP is used otherwise false
 */
boolean_t socket_is_udp(Socket_T S);


/**
 * Get the underlying socket descriptor
 * @param S A Socket object
 * @return The socket descriptor
 */
int socket_get_socket(Socket_T S);


/**
 * Get the type of this socket.
 * @param S A Socket object
 * @return The socket type
 */
int socket_get_type(Socket_T S);


/**
 * Get the Port object used to create this socket. If no Port object
 * was used this method returns NULL.
 * @param S A Socket object
 * @return The Port object or NULL
 */
void *socket_get_Port(Socket_T S);


/**
 * Get the remote port number the socket is connected to
 * @param S A Socket object
 * @return The remote host's port number
 */
int socket_get_remote_port(Socket_T S);


/**
 * Get the remote host this socket is connected to. The host is either
 * a host name in DNS or an IP address string.
 * @param S A Socket object
 * @return The remote host
 */
const char *socket_get_remote_host(Socket_T S);


/**
 * Get the local (ephemeral) port number this socket is using.
 * @param S A Socket object
 * @return The port number on success otherwise -1
 */
int socket_get_local_port(Socket_T S);


/**
 * Get the local interface IP address
 * @param S A Socket object
 * @param host A buffer for the hostname
 * @param hostlen A buffer length
 * @return The local host interface address or NULL if an error occurred
 */
const char *socket_get_local_host(Socket_T S, char *host, int hostlen);


/**
 * Writes the given error message into the Socket's internal input buffer.
 * This method is used to report errors and when no more reads will be
 * attempted. Clients should use socket_getError() to retrieve the error message.
 * @param S A Socket_T object
 * @param error An error string to write to the socket's internal
 * buffer
 */
void socket_setError(Socket_T S, const char *error, ...) __attribute__((format (printf, 2, 3)));


/**
 * Returns an error message set via socket_setError().
 * @param S A Socket_T object
 * @return An error message set on the socket
 */
const char *socket_getError(Socket_T S);


/**
 * Switches a connected socket to ssl.
 * @param S The already connected socket
 * @param ssl Options for ssl
 * @return true if ssl is ready otherwise false
 */
boolean_t socket_switch2ssl(Socket_T S, SslOptions_T ssl);


/**
 * Writes a character string. Use this function to send text based
 * messages to a client.
 * @param S A Socket_T object
 * @param m A String to send to the client
 * @return The bytes sent or -1 if an error occured
 */
int socket_print(Socket_T S, const char *m, ...) __attribute__((format (printf, 2, 3)));


/**
 * Write size bytes from the buffer b.
 * @param S A Socket_T object
 * @param b The data to be written
 * @param size The size of the data in b
 * @return The bytes sent or -1 if an error occured
 */
int socket_write(Socket_T S, void *b, size_t size);


/**
 * Read a single byte. The byte is returned as an int in the range 0
 * to 255.
 * @param S A Socket_T object
 * @return The byte read, or -1 if the end of the stream has been reached
 */
int socket_read_byte(Socket_T S);


/**
 * Reads size bytes and stores them into the byte buffer pointed to by b.
 * @param S A Socket_T object
 * @param b A Byte buffer
 * @param size The size of the buffer b
 * @return The bytes read or -1 if an error occured
 */
int socket_read(Socket_T S, void *b, int size);


/**
 * Reads in at most one less than size <code>characters</code> and
 * stores them into the buffer pointed to by s. Reading stops after
 * an EOF or a newline.  If a newline is read, it is stored into the
 * buffer.  A '\0' is stored after the last character in the buffer.
 * @param S A Socket_T object
 * @param s A character buffer to store the string in
 * @param size The size of the string buffer, s
 * @return s on success, and NULL on error or when end of file occurs
 * while no characters have been read.
 */
char *socket_readln(Socket_T S, char *s, int size);


/**
 * Clears any data that exists in the input buffer
 * @param S A Socket_T object
 */
void socket_reset(Socket_T S);


/**
 * Shut down the writing side of the socket
 * @param S A Socket object
 * @return true if the write side was shutdown otherwise false
 */
boolean_t socket_shutdown_write(Socket_T S);


/**
 * Set TCP_NODELAY option on socket
 * @param S A Socket object
 * @return true upon successful completion, otherwise false
 */
boolean_t socket_set_tcp_nodelay(Socket_T S);

#endif
