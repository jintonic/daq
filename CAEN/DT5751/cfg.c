#include <time.h>
#include <string.h>
#include <CAENDigitizerType.h>
#include "cfg.h"

void SaveCurrentTime(RUN_CFG_t *cfg)
{
  struct timeval tv;
  gettimeofday(&tv,NULL);
  cfg->tsec = tv.tv_sec;
  cfg->tus = tv.tv_usec;
}

int ParseConfigFile(FILE *fcfg, RUN_CFG_t *cfg) 
{
  char setting[256], option[256];
  int i, ch=-1, parameter, swTrgMod, exTrgMod, exTrgSrc, post;

  // default cfg
  cfg->ver=VERSION;
  cfg->ns=1024;
  cfg->mask=0x0;
  cfg->swTrgMod=CAEN_DGTZ_TRGMODE_DISABLED;
  cfg->exTrgMod=CAEN_DGTZ_TRGMODE_DISABLED;
  cfg->exTrgSrc=TTL;
  cfg->post=75;
  cfg->trgMask=0xf; // trigger on any channel
  for (i=0; i<Nch; i++) {
    cfg->thr[i]=500;
    cfg->offset[i]=0;
    cfg->trgMod[i]=CAEN_DGTZ_TRGMODE_ACQ_ONLY;
  }

  // parse cfg file
  while (!feof(fcfg)) {
    // read a word from the file
    int read = fscanf(fcfg, "%s", setting);

    // skip empty lines
    if(!read || !strlen(setting)) continue;

    // skip comments
    if(setting[0] == '#') {
      fgets(setting, 256, fcfg);
      continue;
    }

    // Section (COMMON or individual channel)
    if (setting[0] == '[') {
      if (strstr(setting, "COMMON")) {
	ch = -1;
	continue; 
      }
      sscanf(setting+1, "%d", &parameter);
      if (parameter < 0 || parameter >= Nch) {
	printf("%s: Invalid channel number\n", setting);
      } else {
	ch = parameter;
      }
      continue;
    }

    // number of waveform samples
    if (strstr(setting, "N_WF_SMPL")!=NULL) {
      read = fscanf(fcfg, "%hu", &cfg->ns);
      continue;
    }

    // channel recording enable (YES/NO)
    if (strstr(setting, "CH_REC_ENABLE")!=NULL) {
      read = fscanf(fcfg, "%s", option);
      if (strcmp(option, "YES")==0) {
	if (ch == -1) cfg->mask = 0xF;
	else cfg->mask |= (1 << ch);
	continue;
      } else if (strcmp(option, "NO")==0) {
	if (ch == -1) cfg->mask = 0x0;
	else cfg->mask &= ~(1 << ch);
	continue;
      } else {
	printf("%s: invalid option\n", option);
      }
      continue;
    }

    // software trigger mode
    if (strstr(setting, "SW_TRG_MOD")!=NULL) {
      read = fscanf(fcfg, "%s", option);
      if (strcmp(option, "DISABLED")==0)
	cfg->swTrgMod = CAEN_DGTZ_TRGMODE_DISABLED;
      else if (strcmp(option, "EXTOUT_ONLY")==0)
	cfg->swTrgMod = CAEN_DGTZ_TRGMODE_EXTOUT_ONLY;
      else if (strcmp(option, "ACQ_ONLY")==0)
	cfg->swTrgMod = CAEN_DGTZ_TRGMODE_ACQ_ONLY;
      else if (strcmp(option, "ACQ_AND_EXTOUT")==0)
	cfg->swTrgMod = CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT;
      else
	printf("%s: invalid option\n", option);
      continue;
    }

    // external trigger mode
    if (strstr(setting, "EX_TRG_MOD")!=NULL) {
      read = fscanf(fcfg, "%s", option);
      if (strcmp(option, "DISABLED")==0)
	cfg->exTrgMod = CAEN_DGTZ_TRGMODE_DISABLED;
      else if (strcmp(option, "EXTOUT_ONLY")==0)
	cfg->exTrgMod = CAEN_DGTZ_TRGMODE_EXTOUT_ONLY;
      else if (strcmp(option, "ACQ_ONLY")==0)
	cfg->exTrgMod = CAEN_DGTZ_TRGMODE_ACQ_ONLY;
      else if (strcmp(option, "ACQ_AND_EXTOUT")==0)
	cfg->exTrgMod = CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT;
      else
	printf("%s: invalid option\n", option);
      continue;
    }

    // external trigger source
    if (strstr(setting, "EX_TRG_SRC")!=NULL) {
      read = fscanf(fcfg, "%s", option);
      if (strcmp(option, "TTL")==0)
	cfg->exTrgSrc=TTL;
      else if (strcmp(option, "NIM")==0)
	cfg->exTrgSrc=NIM;
      else
	printf("%s: invalid option\n", option);
      continue;
    }

    // post trigger (percentage after trigger)
    if (strstr(setting, "POST_TRG_%")!=NULL) {
      read = fscanf(fcfg, "%hhu", &post);
      cfg->post=post;
      continue;
    }

    // channel trigger mask (YES/NO)
    if (strstr(setting, "CH_TRG_ENABLE")!=NULL) {
      read = fscanf(fcfg, "%s", option);
      if (strcmp(option, "YES")==0) {
	if (ch == -1) cfg->trgMask = 0xF;
	else cfg->trgMask |= (1 << ch);
	continue;
      } else if (strcmp(option, "NO")==0) {
	if (ch == -1) cfg->trgMask = 0x0;
	else cfg->trgMask &= ~(1 << ch);
	continue;
      } else {
	printf("%s: invalid option\n", option);
      }
      continue;
    }

    // channel threshold
    if (strstr(setting, "CH_TRG_THRESHOLD")!=NULL) {
      read = fscanf(fcfg, "%hu", &parameter);
      if (ch == -1)
	for(i=0; i<Nch; i++) cfg->thr[i] = parameter;
      else
	cfg->thr[ch] = parameter;
      continue;
    }

    // channel DC offset
    if (!strcmp(setting, "CH_DC_OFFSET")) {
      read = fscanf(fcfg, "%hu", &parameter);
      if (ch == -1)
	for(i=0; i<Nch; i++) cfg->offset[i] = parameter;
      else
	cfg->offset[ch] = parameter;
      continue;
    }

    // channel trigger mode 
    if (strstr(setting, "CH_TRG_MOD")!=NULL) {
      read = fscanf(fcfg, "%s", option);
      CAEN_DGTZ_TriggerMode_t tm = CAEN_DGTZ_TRGMODE_DISABLED;
      if (strcmp(option, "DISABLED")==0)
	tm = CAEN_DGTZ_TRGMODE_DISABLED;
      else if (strcmp(option, "EXTOUT_ONLY")==0)
	tm = CAEN_DGTZ_TRGMODE_EXTOUT_ONLY;
      else if (strcmp(option, "ACQ_ONLY")==0)
	tm = CAEN_DGTZ_TRGMODE_ACQ_ONLY;
      else if (strcmp(option, "ACQ_AND_EXTOUT")==0)
	tm = CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT;
      else 
	printf("%s: invalid option\n", option);

      if (ch == -1)
	for(i=0; i<Nch; i++) cfg->trgMod[i] = tm;
      else
	cfg->trgMod[ch] = tm;

      continue;
    }

    printf("%s: invalid setting\n", setting);
  }
  return 0;
}

