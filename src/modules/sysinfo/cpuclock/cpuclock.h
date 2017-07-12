#ifndef CPUCLOCK_H
#define CPUCLOCK_H

#include "../sysinfo.h"

typedef struct _Cpuclock_Config Cpuclock_Config;

struct _Cpuclock_Config
{
   Instance *inst;
   Evas_Object *max;
   Evas_Object *min;
   Evas_Object *general;
   Evas_Object *policy;
   Evas_Object *saving;
   Evas_Object *freq;
   Evas_Object *ps;
   Eina_List *powersaves;
   Eina_Bool frequencies;
   Eina_Bool pstate;
};


Evas_Object *cpuclock_configure(Instance *inst);
void _cpuclock_config_updated(Instance *inst);
void _cpuclock_set_governor(const char *governor);
void _cpuclock_set_frequency(int frequency);
void _cpuclock_set_pstate(int min, int max, int turbo);
#if defined __OpenBSD__ || defined __FreeBSD__
int _cpuclock_sysctl_frequency(int new_perf);
#endif

#endif
