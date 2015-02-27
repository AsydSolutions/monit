/*
 * Copyright (C) Tildeslash Ltd. All rights reserved.
 * Copyright (C) 2009 Alan DeKok <aland@freeradius.org> All rights reserved.
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

#include "config.h"

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include "md5.h"
#include "protocol.h"


/**
 *  Simple RADIUS test.
 *
 *  We send a Status-Server packet, and expect an Access-Accept or Accounting-Response packet.
 *
 *
 */
boolean_t check_radius(Socket_T socket) {
        int i, length, left;
        int secret_len;
        Port_T P;
        md5_context_t ctx;
        char *secret;
        unsigned char *attr;
        unsigned char  digest[16];
        unsigned char  response[STRLEN];
        unsigned char  request[38] = {
                /* Status-Server */
                0x0c,

                /* Code, we always use zero */
                0x00,

                /* Packet length */
                0x00,
                0x26,

                /* Request Authenticator */
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,

                /* Message-Authenticator */
                0x50,

                /* Length */
                0x12,

                /* Contents of Message-Authenticator */
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                0x00
        };

        ASSERT(socket);

        if (socket_get_type(socket) != SOCK_DGRAM) {
                socket_setError(socket, "RADIUS: unsupported socket type -- protocol test skipped");
                return true;
        }

        P = socket_get_Port(socket);
        ASSERT(P);

        secret = P->request ? P->request : "testing123";
        secret_len = (int)strlen(secret);

        /* get 16 bytes of random data */
        for (i = 0; i < 16; i++)
                request[i + 4] = ((unsigned int)random()) & 0xff;

        /* sign the packet */
        Util_hmacMD5(request, sizeof(request), (unsigned char *)secret, secret_len, request + 22);

        if (socket_write(socket, (unsigned char *)request, sizeof(request)) < 0) {
                socket_setError(socket, "RADIUS: error sending query -- %s", STRERROR);
                return false;
        }

        /* the response should have at least 20 bytes */
        if ((length = socket_read(socket, (unsigned char *)response, sizeof(response))) < 20) {
                socket_setError(socket, "RADIUS: error receiving response -- %s", STRERROR);
                return false;
        }

        /* compare the response code (should be Access-Accept or Accounting-Response) */
        if ((response[0] != 2) && (response[0] != 5)) {
                socket_setError(socket, "RADIUS: Invalid reply code -- error occured");
                return false;
        }

        /* compare the packet ID (it should be the same as in our request) */
        if (response[1] != 0x00) {
                socket_setError(socket, "RADIUS: ID mismatch");
                return false;
        }

        /* check the length */
        if (response[2] != 0) {
                socket_setError(socket, "RADIUS: message is too long");
                return false;
        }

        /* check length against packet data */
        if (response[3] != length) {
                socket_setError(socket, "RADIUS: message has invalid length");
                return false;
        }

        /* validate that it is a well-formed packet */
        attr = response + 20;
        left = length - 20;
        while (left > 0) {
                if (left < 2) {
                        socket_setError(socket, "RADIUS: message is malformed");
                        return false;
                }

                if (attr[1] < 2) {
                        socket_setError(socket, "RADIUS: message has invalid attribute length");
                        return false;
                }

                if (attr[1] > left) {
                        socket_setError(socket, "RADIUS: message has attribute that is too long");
                        return false;
                }

                /* validate Message-Authenticator, if found */
                if (attr[0] == 0x50) {
                        /* FIXME: validate it */
                }
                left -= attr[1];
        }

        /* save the reply authenticator, and copy the request authenticator over */
        memcpy(digest, response + 4, 16);
        memcpy(response + 4, request + 4, 16);

        md5_init(&ctx);
        md5_append(&ctx, (const md5_byte_t *)response, length);
        md5_append(&ctx, (const md5_byte_t *)secret, secret_len);
        md5_finish(&ctx, response + 4);

        if (memcmp(digest, response + 4, 16) != 0)
                socket_setError(socket, "RADIUS: message fails authentication");

        return true;
}

