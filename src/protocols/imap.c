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

#include "config.h"

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "protocol.h"

/**
 *  Check the server for greeting code '* OK' and then send LOGOUT and
 *  check for code '* BYE'. If alive return true, else, return false.
 *
 *  @file
 */
boolean_t check_imap(Socket_T socket) {
        char buf[STRLEN];
        const char *ok = "* OK";
        const char *bye = "* BYE";

        ASSERT(socket);

        // Read and check IMAP greeting
        if (! socket_readln(socket, buf, sizeof(buf))) {
                socket_setError(socket, "IMAP: greeting read error -- %s", STRERROR);
                return false;
        }
        Str_chomp(buf);
        if (strncasecmp(buf, ok, strlen(ok)) != 0) {
                socket_setError(socket, "IMAP: invalid greeting -- %s", buf);
                return false;
        }

        // Logout and check response
        if (socket_print(socket, "001 LOGOUT\r\n") < 0) {
                socket_setError(socket, "IMAP: logout command error -- %s", STRERROR);
                return false;
        }
        if (! socket_readln(socket, buf, sizeof(buf))) {
                socket_setError(socket, "IMAP: logout response read error -- %s", STRERROR);
                return false;
        }
        Str_chomp(buf);
        if (strncasecmp(buf, bye, strlen(bye)) != 0) {
                socket_setError(socket, "IMAP: invalid logout response: %s", buf);
                return false;
        }

        return true;
}

