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
 *  Check gpsd status.
 *  There is a project site for gpsd at <http://gpsd.berlios.de/>.
 *
 *  @file
 */
boolean_t check_gps(Socket_T socket) {
        char buf[STRLEN];
        const char *ok_gps_device = "GPSD,G=GPS";
        const char *ok_rtcm104_device = "GPSD,G=RTCM104";
        const char *ok_rtcm104v2_device = "GPSD,G=RTCM104v2";

        ASSERT(socket);

        if (socket_print(socket, "G\r\n") < 0) {
                socket_setError(socket, "GPS: error sending data -- %s", STRERROR);
                return false;
        }

        if (! socket_readln(socket, buf, sizeof(buf))) {
                socket_setError(socket, "GPS: error receiving data -- %s", STRERROR);
                return false;
        }

        Str_chomp(buf);
        if (strncasecmp(buf, ok_gps_device, strlen(ok_gps_device)) != 0) {
                if (strncasecmp(buf, ok_rtcm104v2_device, strlen(ok_rtcm104v2_device)) != 0) {
                        if (strncasecmp(buf, ok_rtcm104_device, strlen(ok_rtcm104_device)) != 0) {
                                socket_setError(socket, "GPS error (no device): %s", buf);
                                return false;
                        }
                }
        }
        
        return true;
}

