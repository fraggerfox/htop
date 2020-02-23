/*
htop - NetBSDProcessList.c
(C) 2014 Hisham H. Muhammad
(C) 2015 Michael McConville
Released under the GNU GPL, see the COPYING file
in the source distribution for its full text.
*/

#include "ProcessList.h"
#include "NetBSDProcessList.h"
#include "NetBSDProcess.h"

#include <uvm/uvm_extern.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mount.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/resource.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*{

#include <kvm.h>

typedef struct CPUData_ {
   unsigned long long int totalTime;
   unsigned long long int totalPeriod;
} CPUData;

typedef struct NetBSDProcessList_ {
   ProcessList super;
   kvm_t* kd;

   CPUData* cpus;

} NetBSDProcessList;

}*/

/*
 * avoid relying on or conflicting with MIN() and MAX() in sys/param.h
 */
#ifndef MINIMUM
#define MINIMUM(x, y)		((x) > (y) ? (y) : (x))
#endif

#ifndef MAXIMUM
#define MAXIMUM(x, y)		((x) > (y) ? (x) : (y))
#endif

#ifndef CLAMP
#define CLAMP(x, low, high)	(((x) > (high)) ? (high) : MAXIMUM(x, low))
#endif

#define BOUNDS(x) isnan(x) ? 0.0 : (x > 100) ? 100.0 : x;

static int MIB_hw_physmem[2];
static int MIB_vm_stats_vm_v_page_count[4];

static long fscale;

ProcessList* ProcessList_new(UsersTable* usersTable, Hashtable* pidWhiteList, uid_t userId) {
   int mib[] = { CTL_HW, HW_NCPU };
   int fmib[] = { CTL_KERN, KERN_FSCALE };
   int i, e;
   NetBSDProcessList* opl;
   ProcessList* pl;
   size_t size;
   char errbuf[_POSIX2_LINE_MAX];

   opl = xCalloc(1, sizeof(NetBSDProcessList));
   pl = (ProcessList*) opl;
   size = sizeof(pl->cpuCount);
   ProcessList_init(pl, Class(NetBSDProcess), usersTable, pidWhiteList, userId);

   e = sysctl(mib, 2, &pl->cpuCount, &size, NULL, 0);
   if (e == -1 || pl->cpuCount < 1) {
      pl->cpuCount = 1;
   }
   opl->cpus = xRealloc(opl->cpus, pl->cpuCount * sizeof(CPUData));

   size = sizeof(fscale);
   if (sysctl(fmib, 2, &fscale, &size, NULL, 0) < 0) {
      err(1, "fscale sysctl call failed");
   }

   for (i = 0; i < pl->cpuCount; i++) {
      opl->cpus[i].totalTime = 1;
      opl->cpus[i].totalPeriod = 1;
   }

   opl->kd = kvm_openfiles(NULL, NULL, NULL, KVM_NO_FILES, errbuf);
   if (opl->kd == NULL) {
      errx(1, "kvm_open: %s", errbuf);
   }

   return pl;
}

void ProcessList_delete(ProcessList* this) {
   const NetBSDProcessList* opl = (NetBSDProcessList*) this;

   if (opl->kd) {
      kvm_close(opl->kd);
   }

   free(opl->cpus);

   ProcessList_done(this);
   free(this);
}

static inline void NetBSDProcessList_scanMemoryInfo(ProcessList* pl) {
   static int uvmexp_mib[] = {CTL_VM, VM_UVMEXP2};
   struct uvmexp_sysctl uvmexp;
   size_t size_uvmexp = sizeof(uvmexp);

   if (sysctl(uvmexp_mib, 2, &uvmexp, &size_uvmexp, NULL, 0) < 0) {
      err(1, "uvmexp sysctl call failed");
   }

   pl->totalMem = uvmexp.npages * PAGE_SIZE_KB;

//   FILE *fp;
//   fp = fopen("debug.log", "w");
//   fprintf(fp,"npages %lu\n", uvmexp.npages);
//   fprintf(fp,"free %lu\n", uvmexp.free);
//   fprintf(fp,"wired %lu\n", uvmexp.wired);
//   fprintf(fp,"active %lu\n", uvmexp.active);
//   fprintf(fp,"inactive %lu\n", uvmexp.inactive);
//   fprintf(fp,"paging %lu\n", uvmexp.paging);
//   fprintf(fp,"pagesize %lu\n", uvmexp.pagesize);
//   fprintf(fp, "%lu\n",PAGE_SIZE_KB);

//   // Taken from NetBSD systat/iostat.c, top/machine.c and uvm_sysctl(9)
//   static int bcache_mib[] = {CTL_VFS, VFS_GENERIC, VFS_BCACHESTAT};
//   struct bcachestats bcstats;
//   size_t size_bcstats = sizeof(bcstats);
//
//   if (sysctl(bcache_mib, 3, &bcstats, &size_bcstats, NULL, 0) < 0) {
//      err(1, "cannot get vfs.bcachestat");
//   }

//   pl->cachedMem = bcstats.numbufpages * PAGE_SIZE_KB;
   pl->freeMem = uvmexp.free * PAGE_SIZE_KB;
   pl->usedMem = (uvmexp.npages - uvmexp.free - uvmexp.paging) * PAGE_SIZE_KB;
   //pl->buffersMem = uvmexp.free * PAGE_SIZE_KB;
   pl->cachedMem = (uvmexp.anonpages + uvmexp.filepages + uvmexp.execpages) * PAGE_SIZE_KB;
//   const NetBSDProcessList* opl = (NetBSDProcessList*) pl;

//   size_t len = sizeof(pl->totalMem);
//   sysctl(MIB_hw_physmem, 2, &(pl->totalMem), &len, NULL, 0);
//   pl->totalMem /= 1024;
//   sysctl(MIB_vm_stats_vm_v_wire_count, 4, &(pl->usedMem), &len, NULL, 0);
//   pl->usedMem *= PAGE_SIZE_KB;
//   pl->freeMem = pl->totalMem - pl->usedMem;
////   sysctl(MIB_vm_stats_vm_v_cache_count, 4, &(pl->cachedMem), &len, NULL, 0);
////   pl->cachedMem *= PAGE_SIZE_KB;
//
////   struct kvm_swap swap[16];
////   int nswap = kvm_getswapinfo(opl->kd, swap, sizeof(swap)/sizeof(swap[0]), 0);
//   pl->totalSwap = 0;
//   pl->usedSwap = 0;
////   for (int i = 0; i < nswap; i++) {
////      pl->totalSwap += swap[i].ksw_total;
////      pl->usedSwap += swap[i].ksw_used;
////   }
   pl->totalSwap = uvmexp.swpages * PAGE_SIZE_KB;
   pl->usedSwap = uvmexp.swpginuse * PAGE_SIZE_KB;
//
//   pl->sharedMem = 0;  // currently unused
//   pl->buffersMem = 0; // not exposed to userspace

}

char *NetBSDProcessList_readProcessName(kvm_t* kd, struct kinfo_proc2* kproc, int* basenameEnd) {
   char *s, **arg;
   size_t len = 0, n;
   int i;

   /*
    * Like NetBSD's top(1), we try to fall back to the command name
    * (argv[0]) if we fail to construct the full command.
    */
   arg = kvm_getargv2(kd, kproc, 500);
   if (arg == NULL || *arg == NULL) {
      *basenameEnd = strlen(kproc->p_comm);
      return xStrdup(kproc->p_comm);
   }
   for (i = 0; arg[i] != NULL; i++) {
      len += strlen(arg[i]) + 1;   /* room for arg and trailing space or NUL */
   }
   /* don't use xMalloc here - we want to handle huge argv's gracefully */
   if ((s = malloc(len)) == NULL) {
      *basenameEnd = strlen(kproc->p_comm);
      return xStrdup(kproc->p_comm);
   }

   *s = '\0';

   for (i = 0; arg[i] != NULL; i++) {
      n = strlcat(s, arg[i], len);
      if (i == 0) {
         /* TODO: rename all basenameEnd to basenameLen, make size_t */
         *basenameEnd = MINIMUM(n, len-1);
      }
      /* the trailing space should get truncated anyway */
      strlcat(s, " ", len);
   }

   return s;
}

/*
 * Taken from OpenBSD's ps(1).
 */
double getpcpu(const struct kinfo_proc2 *kp) {
   if (fscale == 0)
      return (0.0);

#define   fxtofl(fixpt)   ((double)(fixpt) / fscale)

   return (100.0 * fxtofl(kp->p_pctcpu));
}

void ProcessList_goThroughEntries(ProcessList* this) {
   NetBSDProcessList* opl = (NetBSDProcessList*) this;
   Settings* settings = this->settings;
   bool hideKernelThreads = settings->hideKernelThreads;
   bool hideUserlandThreads = settings->hideUserlandThreads;
   struct kinfo_proc2* kproc;
   bool preExisting;
   Process* proc;
   NetBSDProcess* fp;
   struct tm date;
   struct timeval tv;
   int count = 0;
   int i;

   NetBSDProcessList_scanMemoryInfo(this);

   struct kinfo_proc2* kprocs = kvm_getproc2(opl->kd, KERN_PROC_ALL, 0, sizeof(struct kinfo_proc2), &count);
   //struct kinfo_proc* kprocs = getprocs(KERN_PROC_ALL, 0, &count);

   gettimeofday(&tv, NULL);

   for (i = 0; i < count; i++) {
      kproc = &kprocs[i];

      preExisting = false;
      proc = ProcessList_getProcess(this, kproc->p_pid, &preExisting, (Process_New) NetBSDProcess_new);
      fp = (NetBSDProcess*) proc;

      proc->show = ! ((hideKernelThreads && Process_isKernelThread(proc))
	          || (hideUserlandThreads && Process_isUserlandThread(proc)));

      if (!preExisting) {
         proc->ppid = kproc->p_ppid;
	 proc->tpgid = kproc->p_tpgid;
	 proc->tgid = kproc->p_pid;
	 proc->session = kproc->p_sid;
	 proc->tty_nr = kproc->p_tdev;
	 proc->pgrp = kproc->p__pgid;
	 proc->st_uid = kproc->p_uid;
	 proc->starttime_ctime = kproc->p_ustart_sec;
	 proc->user = UsersTable_getRef(this->usersTable, proc->st_uid);
	 ProcessList_add((ProcessList*)this, proc);
	 proc->comm = NetBSDProcessList_readProcessName(opl->kd, kproc, &proc->basenameOffset);
	 (void) localtime_r((time_t*) &kproc->p_ustart_sec, &date);
	 strftime(proc->starttime_show, 7, ((proc->starttime_ctime > tv.tv_sec - 86400) ? "%R " : "%b%d "), &date);
      } else {
	 if (settings->updateProcessNames) {
	   free(proc->comm);
	   proc->comm = NetBSDProcessList_readProcessName(opl->kd, kproc, &proc->basenameOffset);
	 }
      }

      proc->m_size = kproc->p_vm_dsize;
      proc->m_resident = kproc->p_vm_rssize;
      proc->percent_mem = (proc->m_resident * PAGE_SIZE_KB) / (double)(this->totalMem) * 100.0;
      proc->percent_cpu = CLAMP(getpcpu(kproc), 0.0, this->cpuCount*100.0);
      //proc->nlwp = kproc->p_numthreads;
      //proc->time = kproc->p_rtime_sec + ((kproc->p_rtime_usec + 500000) / 10);
      proc->nice = kproc->p_nice - 20;
      proc->time = kproc->p_rtime_sec + ((kproc->p_rtime_usec + 500000) / 1000000);
      proc->time *= 100;
      proc->priority = kproc->p_priority - PZERO;

      switch (kproc->p_stat) {
         case SIDL:     proc->state = 'I'; break;
         case LSRUN:    proc->state = 'R'; break;
         case LSSLEEP:  proc->state = 'S'; break;
         case SSTOP:    proc->state = 'T'; break;
         case SZOMB:    proc->state = 'Z'; break;
         case SDEAD:    proc->state = 'D'; break;
         case LSONPROC: proc->state = 'P'; break;
         default:       proc->state = '?';
      }

      if (Process_isKernelThread(proc)) {
	this->kernelThreads++;
      }

      this->totalTasks++;
      // LSRUN ('R') means runnable, not running
      if (proc->state == 'P') {
        this->runningTasks++;
      }
      proc->updated = true;
   }
}
