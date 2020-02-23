/* Do not edit this file. It was automatically generated. */

#ifndef HEADER_NetBSDProcessList
#define HEADER_NetBSDProcessList
/*
htop - NetBSDProcessList.h
(C) 2014 Hisham H. Muhammad
(C) 2015 Michael McConville
Released under the GNU GPL, see the COPYING file
in the source distribution for its full text.
*/


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


ProcessList* ProcessList_new(UsersTable* usersTable, Hashtable* pidWhiteList, uid_t userId);

void ProcessList_delete(ProcessList* this);

char *NetBSDProcessList_readProcessName(kvm_t* kd, struct kinfo_proc2* kproc, int* basenameEnd);

/*
 * Taken from OpenBSD's ps(1).
 */
double getpcpu(const struct kinfo_proc2 *kp);

void ProcessList_goThroughEntries(ProcessList* this);

#endif
