#include <time.h>
#include <string.h>
#include <ctype.h>
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
  int i, ch=-1, parameter, post;

  // default cfg
  cfg->ver=VERSION;
  cfg->ns=1024;
  cfg->mask=0x0;
  cfg->swTrgMod=CAEN_DGTZ_TRGMODE_DISABLED;
  cfg->exTrgMod=CAEN_DGTZ_TRGMODE_DISABLED;
  cfg->chTrgMod=CAEN_DGTZ_TRGMODE_ACQ_ONLY;
  cfg->exTrgSrc=TTL;
  cfg->post=75;
  cfg->trgMask=0x0; // trigger on none of the channels
  for (i=0; i<Nch; i++) {
    cfg->thr[i]=500;
    cfg->offset[i]=0;
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

    // upper case to lower case
    for(i=0; setting[i]; i++) setting[i] = tolower(setting[i]);

    // Section (Common or individual channel)
    if (setting[0] == '[') {
      if (strstr(setting, "common")) {
	ch = -1;
	continue; 
      }
      sscanf(setting+1, "%d", &parameter);
      if (parameter < 0 || parameter >= Nch)
	printf("%d: invalid channel number\n", parameter);
      else
	ch = parameter;
      continue;
    }

    // number of waveform samples
    if (strstr(setting, "number_of_samples")!=NULL) {
      read = fscanf(fcfg, "%u", &cfg->ns);
      continue;
    }

    // post trigger (percentage after trigger)
    if (strstr(setting, "post_trigger_percentage")!=NULL) {
      read = fscanf(fcfg, "%d", &post);
      cfg->post=post;
      continue;
    }

    // software trigger mode
    if (strstr(setting, "software_trigger_mode")!=NULL) {
      read = fscanf(fcfg, "%s", option);
      for(i=0; setting[i]; i++) setting[i] = tolower(setting[i]);
      if (strcmp(option, "disabled")==0)
	cfg->swTrgMod = CAEN_DGTZ_TRGMODE_DISABLED;
      else if (strcmp(option, "extout_only")==0)
	cfg->swTrgMod = CAEN_DGTZ_TRGMODE_EXTOUT_ONLY;
      else if (strcmp(option, "acq_only")==0)
	cfg->swTrgMod = CAEN_DGTZ_TRGMODE_ACQ_ONLY;
      else if (strcmp(option, "acq_and_extout")==0)
	cfg->swTrgMod = CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT;
      else
	printf("%s: invalid trigger mode\n", option);
      continue;
    }

    // internal trigger mode 
    if (strstr(setting, "internal_trigger_mode")!=NULL) {
      read = fscanf(fcfg, "%s", option);
      for(i=0; setting[i]; i++) setting[i] = tolower(setting[i]);
      if (strcmp(option, "disabled")==0)
	cfg->chTrgMod = CAEN_DGTZ_TRGMODE_DISABLED;
      else if (strcmp(option, "extout_only")==0)
	cfg->chTrgMod = CAEN_DGTZ_TRGMODE_EXTOUT_ONLY;
      else if (strcmp(option, "acq_only")==0)
	cfg->chTrgMod = CAEN_DGTZ_TRGMODE_ACQ_ONLY;
      else if (strcmp(option, "acq_and_extout")==0)
	cfg->chTrgMod = CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT;
      else 
	printf("%s: invalid internal trigger mode\n", option);

      continue;
    }

    // external trigger mode
    if (strstr(setting, "external_trigger_mode")!=NULL) {
      read = fscanf(fcfg, "%s", option);
      for(i=0; setting[i]; i++) setting[i] = tolower(setting[i]);
      if (strcmp(option, "disabled")==0)
	cfg->exTrgMod = CAEN_DGTZ_TRGMODE_DISABLED;
      else if (strcmp(option, "extout_only")==0)
	cfg->exTrgMod = CAEN_DGTZ_TRGMODE_EXTOUT_ONLY;
      else if (strcmp(option, "acq_only")==0)
	cfg->exTrgMod = CAEN_DGTZ_TRGMODE_ACQ_ONLY;
      else if (strcmp(option, "acq_and_extout")==0)
	cfg->exTrgMod = CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT;
      else
	printf("%s: invalid trigger mode\n", option);
      continue;
    }

    // external trigger source
    if (strstr(setting, "external_trigger_source")!=NULL) {
      read = fscanf(fcfg, "%s", option);
      for(i=0; setting[i]; i++) setting[i] = toupper(setting[i]);
      if (strcmp(option, "TTL")==0)
	cfg->exTrgSrc=TTL;
      else if (strcmp(option, "NIM")==0)
	cfg->exTrgSrc=NIM;
      else
	printf("%s: invalid trigger source\n", option);
      continue;
    }

    // channel DC offset
    if (!strcmp(setting, "channel_DC_offset")) {
      read = fscanf(fcfg, "%u", &parameter);
      if (ch == -1) for(i=0; i<Nch; i++) cfg->offset[i] = parameter;
      else cfg->offset[ch] = parameter;
      continue;
    }

    // channel threshold
    if (strstr(setting, "channel_trigger_threshold")!=NULL) {
      read = fscanf(fcfg, "%u", &parameter);
      if (ch == -1) for(i=0; i<Nch; i++) cfg->thr[i] = parameter;
      else cfg->thr[ch] = parameter;
      continue;
    }

    // channel trigger mask (yes/no)
    if (strstr(setting, "channel_enable_trigger")!=NULL) {
      read = fscanf(fcfg, "%s", option);
      for(i=0; setting[i]; i++) setting[i] = tolower(setting[i]);
      if (strcmp(option, "yes")==0) {
	if (ch == -1) cfg->trgMask = 0xF;
	else cfg->trgMask |= (1 << ch);
	continue;
      } else if (strcmp(option, "no")==0) {
	if (ch == -1) cfg->trgMask = 0x0;
	else cfg->trgMask &= ~(1 << ch);
	continue;
      } else {
	printf("%s: invalid option to enable channel trigger\n", option);
      }
      continue;
    }

    // channel recording mask (yes/no)
    if (strstr(setting, "channel_enable_recording")!=NULL) {
      read = fscanf(fcfg, "%s", option);
      for(i=0; setting[i]; i++) setting[i] = tolower(setting[i]);
      if (strcmp(option, "yes")==0) {
	if (ch == -1) cfg->mask = 0xf;
	else cfg->mask |= (1 << ch);
	continue;
      } else if (strcmp(option, "no")==0) {
	if (ch == -1) cfg->mask = 0x0;
	else cfg->mask &= ~(1 << ch);
	continue;
      } else {
	printf("%s: invalid option to enable channel recording\n", option);
      }
      continue;
    }

    // coincidence window
    if (strstr(setting, "coincidence_window")!=NULL) {
      read = fscanf(fcfg, "%u", &parameter);
      if (parameter < 16) cfg->trgMask |= (parameter << 20);
      else printf("%d: invalid coincidence window\n",parameter);
      continue;
    }

    // coincidence level
    if (strstr(setting, "coincidence_level")!=NULL) {
      read = fscanf(fcfg, "%u", &parameter);
      if (parameter < 16) cfg->trgMask |= (parameter << 24);
      else printf("%d: invalid coincidence level\n",parameter);
      continue;
    }

    printf("%s: invalid setting\n", setting);
  }
  return 0;
}

