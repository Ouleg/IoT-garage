#include <string.h>
#include "registry.h"

#define MAX_DEV 64
static device_t devs[MAX_DEV];
static int used = 0;

static int find_index(const char* id){
  for(int i=0;i<used;i++) if(strcmp(devs[i].id,id)==0) return i;
  return -1;
}

bool registry_init(void){ used=0; memset(devs,0,sizeof devs); return true; }

bool registry_upsert(const device_t* d){
  if(!d || !d->id[0]) return false;
  int i=find_index(d->id);
  if(i<0){
    if(used>=MAX_DEV) return false;
    i=used++;
    memset(&devs[i],0,sizeof(devs[i]));
  }
  devs[i]=*d;
  return true;
}

bool registry_set_info(const char* id, const char* json){
  int i=find_index(id); if(i<0) return false;
  strncpy(devs[i].info_json, json, sizeof(devs[i].info_json)-1);
  return true;
}
bool registry_set_state(const char* id, const char* json){
  int i=find_index(id); if(i<0) return false;
  strncpy(devs[i].state_json, json, sizeof(devs[i].state_json)-1);
  return true;
}
bool registry_get_info(const char* id, char* out, int outsz){
  int i=find_index(id); if(i<0||!out||outsz<=0) return false;
  strncpy(out, devs[i].info_json, outsz-1); out[outsz-1]='\0'; return true;
}
bool registry_get_state(const char* id, char* out, int outsz){
  int i=find_index(id); if(i<0||!out||outsz<=0) return false;
  strncpy(out, devs[i].state_json, outsz-1); out[outsz-1]='\0'; return true;
}
bool registry_mark_seen(const char* id, long now){
  int i=find_index(id); if(i<0) return false;
  devs[i].last_seen_ssdp = now; (void)now; return true;
}
int registry_prune_expired(long now, long max_age){
  (void)now; (void)max_age; return 0; // stub
}
