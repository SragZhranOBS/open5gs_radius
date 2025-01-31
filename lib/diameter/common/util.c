/*
 * Copyright (C) 2019 by Sukchan Lee <acetcom@gmail.com>
 *
 * This file is part of Open5GS.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "ogs-diameter-common.h"

bool ogs_diam_app_connected(uint32_t app_id)
{
    struct fd_list *li = NULL;
    struct fd_app *found = NULL;
    bool relay = false;
    CHECK_POSIX( pthread_rwlock_rdlock(&fd_g_peers_rw) );
    for (li = fd_g_peers.next; li != &fd_g_peers; li = li->next) {
        struct peer_hdr *p = (struct peer_hdr *)li->o;
        int state = fd_peer_get_state(p);

        ogs_assert(p);
        ogs_assert(p->info.pi_diamid);
        ogs_assert(p->info.pi_diamidlen);

        if (state == STATE_OPEN) {
            ogs_debug("'%s' STATE is OPEN", p->info.pi_diamid);

            /* Check if the remote peer advertised the message's appli */
            fd_app_check(&p->info.runtime.pir_apps, app_id, &found);
            relay = 0 < p->info.runtime.pir_relay;

            if (found || relay) break;
        } else {
            ogs_debug("'%s' STATE[%d] is NOT open ", p->info.pi_diamid, state);
        }
    }
    CHECK_POSIX( pthread_rwlock_unlock(&fd_g_peers_rw) );

    if (found || relay)
        return true;
    else
        return false;
}

/* 
 * Sets the realm from IMSI
 *
 * The realm is in the following format:
 * EPC_DOMAIN="epc.mnc${MNC}.mcc${MCC}.3gppnetwork.org"
 * e.g. "epc.mnc0{01}.mcc{001}.3gppnetwork.org"
 * and IMSI is {001}{ 01}XXXXXXXX
 *              MCC  MNC
 */
DiamId_t ogs_set_realm_from_imsi_bcd(const char * imsi_bcd, uint16_t mnc_length) {
    DiamId_t realm = strdup(fd_g_config->cnf_diamrlm);
    /* Get the MCC part */
    char * mcc = strstr(realm, "mcc");
    if (mcc != NULL) {
        strncpy(mcc + 3, imsi_bcd, 3);
    }
    /* Get the MNC part */
    char * mnc = strstr(realm, "mnc");
    if (mnc != NULL) {
        if(mnc_length==3){
            strncpy(mnc + 3, imsi_bcd + 3, 3);
        }  
        else if(mnc_length==2){
            strncpy(mnc + 4, imsi_bcd + 3, 2);
        }
    }

    return realm;
}