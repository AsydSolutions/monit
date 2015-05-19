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

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifdef HAVE_SYS_PROC_H
#include <sys/proc.h>
#endif

#ifdef HAVE_SYS_VMMETER_H
#include <sys/vmmeter.h>
#endif

#ifdef HAVE_KVM_H
#include <kvm.h>
#endif

#ifdef HAVE_UVM_UVM_H
#include <uvm/uvm.h>
#endif

#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif

#ifdef HAVE_SYS_SCHED_H
#include <sys/sched.h>
#endif

#include "monit.h"
#include "process.h"
#include "process_sysdep.h"


/**
 *  System dependent resource gathering code for OpenBSD.
 *
 *  @file
 */


/* ----------------------------------------------------------------- Private */


static int      hz;
static int      pagesize_kbyte;
static long     total_old    = 0;
static long     cpu_user_old = 0;
static long     cpu_syst_old = 0;
static unsigned maxslp;



/* ------------------------------------------------------------------ Public */


boolean_t init_process_info_sysdep(void) {
        int              mib[2];
        size_t           len;
        int64_t          physmem;
        struct clockinfo clock;

        mib[0] = CTL_KERN;
        mib[1] = KERN_CLOCKRATE;
        len    = sizeof(clock);
        if (sysctl(mib, 2, &clock, &len, NULL, 0) == -1) {
                DEBUG("system statistic error -- cannot get clock rate: %s\n", STRERROR);
                return false;
        }
        hz     = clock.hz;

        mib[0] = CTL_HW;
        mib[1] = HW_NCPU;
        len    = sizeof(systeminfo.cpus);
        if (sysctl(mib, 2, &systeminfo.cpus, &len, NULL, 0) == -1) {
                DEBUG("system statistic error -- cannot get cpu count: %s\n", STRERROR);
                return false;
        }

        mib[1] = HW_PHYSMEM64;
        len    = sizeof(physmem);
        if (sysctl(mib, 2, &physmem, &len, NULL, 0) == -1) {
                DEBUG("system statistic error -- cannot get real memory amount: %s\n", STRERROR);
                return false;
        }
        systeminfo.mem_kbyte_max = physmem / 1024;

        mib[1] = HW_PAGESIZE;
        len    = sizeof(pagesize_kbyte);
        if (sysctl(mib, 2, &pagesize_kbyte, &len, NULL, 0) == -1) {
                DEBUG("system statistic error -- cannot get memory page size: %s\n", STRERROR);
                return false;
        }
        pagesize_kbyte /= 1024;

        return true;
}


/**
 * Read all processes to initialize the information tree.
 * @param reference  reference of ProcessTree
 * @return treesize > 0 if succeeded otherwise = 0.
 */
int initprocesstree_sysdep(ProcessTree_T **reference) {
        int                       treesize;
        char                      buf[_POSIX2_LINE_MAX];
        size_t                    size = sizeof(maxslp);
#if (OpenBSD <= 201105)
        int                       mib_proc[6] = {CTL_KERN, KERN_PROC2, KERN_PROC_KTHREAD, 0, sizeof(struct kinfo_proc2), 0};
        static struct kinfo_proc2 *pinfo;
#else
        int                       mib_proc[6] = {CTL_KERN, KERN_PROC, KERN_PROC_KTHREAD, 0, sizeof(struct kinfo_proc), 0};
        static struct kinfo_proc *pinfo;
#endif
        static int                mib_maxslp[] = {CTL_VM, VM_MAXSLP};
        ProcessTree_T            *pt;
        kvm_t                    *kvm_handle;

        if (sysctl(mib_maxslp, 2, &maxslp, &size, NULL, 0) < 0) {
                LogError("system statistic error -- vm.maxslp failed\n");
                return 0;
        }

        if (sysctl(mib_proc, 6, NULL, &size, NULL, 0) == -1) {
                LogError("system statistic error -- kern.proc #1 failed\n");
                return 0;
        }

        size *= 2; // Add reserve for new processes which were created between calls of sysctl
        pinfo = CALLOC(1, size);
#if (OpenBSD <= 201105)
        mib_proc[5] = (int)(size / sizeof(struct kinfo_proc2));
#else
        mib_proc[5] = (int)(size / sizeof(struct kinfo_proc));
#endif
        if (sysctl(mib_proc, 6, pinfo, &size, NULL, 0) == -1) {
                FREE(pinfo);
                LogError("system statistic error -- kern.proc #2 failed\n");
                return 0;
        }

#if (OpenBSD <= 201105)
        treesize = (int)(size / sizeof(struct kinfo_proc2));
#else
        treesize = (int)(size / sizeof(struct kinfo_proc));
#endif

        pt = CALLOC(sizeof(ProcessTree_T), treesize);

        if (! (kvm_handle = kvm_openfiles(NULL, NULL, NULL, KVM_NO_FILES, buf))) {
                FREE(pinfo);
                FREE(pt);
                LogError("system statistic error -- kvm_openfiles failed: %s\n", buf);
                return 0;
        }

        for (int i = 0; i < treesize; i++) {
                pt[i].pid         = pinfo[i].p_pid;
                pt[i].ppid        = pinfo[i].p_ppid;
                pt[i].uid         = pinfo[i].p_ruid;
                pt[i].euid        = pinfo[i].p_uid;
                pt[i].gid         = pinfo[i].p_rgid;
                pt[i].starttime   = pinfo[i].p_ustart_sec;
                pt[i].cputime     = (long)((pinfo[i].p_rtime_sec * 10) + (pinfo[i].p_rtime_usec / 100000));
                pt[i].cpu_percent = 0;
                pt[i].mem_kbyte   = (unsigned long)(pinfo[i].p_vm_rssize * pagesize_kbyte);
                if (pinfo[i].p_stat == SZOMB)
                        pt[i].zombie = true;
                pt[i].time = get_float_time();
                char **args;
#if (OpenBSD <= 201105)
                if ((args = kvm_getargv2(kvm_handle, &pinfo[i], 0))) {
#else
                if ((args = kvm_getargv(kvm_handle, &pinfo[i], 0))) {
#endif
                        StringBuffer_T cmdline = StringBuffer_create(64);;
                        for (int j = 0; args[j]; j++)
                                StringBuffer_append(cmdline, args[j + 1] ? "%s " : "%s", args[j]);
                        pt[i].cmdline = Str_dup(StringBuffer_toString(StringBuffer_trim(cmdline)));
                        StringBuffer_free(&cmdline);
                }
                if (! pt[i].cmdline || ! *pt[i].cmdline) {
                        FREE(pt[i].cmdline);
                        pt[i].cmdline = Str_dup(pinfo[i].p_comm);
                }
        }
        FREE(pinfo);
        kvm_close(kvm_handle);

        *reference = pt;

        return treesize;
}


/**
 * This routine returns 'nelem' double precision floats containing
 * the load averages in 'loadv'; at most 3 values will be returned.
 * @param loadv destination of the load averages
 * @param nelem number of averages
 * @return: 0 if successful, -1 if failed (and all load averages are 0).
 */
int getloadavg_sysdep (double *loadv, int nelem) {
        return getloadavg(loadv, nelem);
}


/**
 * This routine returns kbyte of real memory in use.
 * @return: true if successful, false if failed (or not available)
 */
boolean_t used_system_memory_sysdep(SystemInfo_T *si) {
        struct uvmexp vm;
        int mib[2] = {CTL_VM, VM_UVMEXP};
        size_t len = sizeof(struct uvmexp);
        if (sysctl(mib, 2, &vm, &len, NULL, 0) == -1) {
                si->swap_kbyte_max = 0;
                LogError("system statistic error -- cannot get memory usage: %s\n", STRERROR);
                return false;
        }
        si->total_mem_kbyte = (vm.active + vm.wired) * pagesize_kbyte;
        si->swap_kbyte_max = vm.swpages * pagesize_kbyte;
        si->total_swap_kbyte = vm.swpginuse * pagesize_kbyte;
        return true;
}


/**
 * This routine returns system/user CPU time in use.
 * @return: true if successful, false if failed
 */
boolean_t used_system_cpu_sysdep(SystemInfo_T *si) {
        int    mib[] = {CTL_KERN, KERN_CPTIME};
        long   cp_time[CPUSTATES];
        long   total_new = 0;
        long   total;
        size_t len;

        len = sizeof(cp_time);
        if (sysctl(mib, 2, &cp_time, &len, NULL, 0) == -1) {
                LogError("system statistic error -- cannot get cpu time: %s\n", STRERROR);
                return false;
        }

        for (int i = 0; i < CPUSTATES; i++)
                total_new += cp_time[i];
        total     = total_new - total_old;
        total_old = total_new;

        si->total_cpu_user_percent = (total > 0) ? (int)(1000 * (double)(cp_time[CP_USER] - cpu_user_old) / total) : -10;
        si->total_cpu_syst_percent = (total > 0) ? (int)(1000 * (double)(cp_time[CP_SYS] - cpu_syst_old) / total) : -10;
        si->total_cpu_wait_percent = 0; /* there is no wait statistic available */

        cpu_user_old = cp_time[CP_USER];
        cpu_syst_old = cp_time[CP_SYS];

        return true;
}

