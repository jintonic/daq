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

uint32_t GetDCOffset(int ch, int adc)
{
  if (adc<0 || adc>65535) {
    printf("DC offset %d is not in [0,65535], set to 900\n",adc);
    adc=900;
  }
  // in case of 16-bit DAC input
  if (adc>1023) return adc;
  // in case of ADC input
  float value = 10000;
  if (ch==0) value = 63736.3-56.6*adc;
  if (ch==1) value = 67168.0-57.8*adc;
  if (ch==2) value = 67295.0-58.1*adc;
  if (ch==3) value = 67574.0-58.1*adc;
  if (value<0 || value>65535) {
    printf("DC offset value %.0f is out of range. Set it to 10000.\n", value);
    value=10000;
  }
  return (uint32_t) value;
}

int ParseConfigFile(FILE *fcfg, RUN_CFG_t *cfg) 
{
  char setting[256], option[256];
  int i, ch=-1, parameter, post;

  // default cfg
  cfg->ver=VERSION;
  cfg->ns=1001; // 7 x 143
  cfg->mask=0x0; // disable all channels
  cfg->swTrgMod=CAEN_DGTZ_TRGMODE_DISABLED;
  cfg->exTrgMod=CAEN_DGTZ_TRGMODE_DISABLED;
  cfg->chTrgMod=CAEN_DGTZ_TRGMODE_DISABLED;
  cfg->exTrgSrc=CAEN_DGTZ_IOLevel_TTL;
  cfg->post=75;
  cfg->polarity=0xf; // trigger on falling edge
  cfg->trgMask=0x0; // trigger on none of the channels
  for (i=0; i<Nch; i++) { cfg->thr[i]=770; cfg->offset[i]=1000; }

  // parse cfg file
  while (!feof(fcfg)) {
    // read a word from the file
    int read = fscanf(fcfg, "%s", setting);

    // skip empty lines
    if(read<=0 || !strlen(setting)) continue;

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
      if (parameter < 0 || parameter >= Nch) {
	printf("%d: invalid channel number\n", parameter);
	return 1;
      } else
	ch = parameter;
      continue;
    }
    printf(" %s: ",setting);

    // number of waveform samples
    if (strstr(setting, "number_of_samples")!=NULL) {
      read = fscanf(fcfg, "%u", &cfg->ns);
      if (cfg->ns%7==0) { printf("%d\n",cfg->ns); continue; }
      printf("Number of samples %d is not divisible by 7,",cfg->ns);
      if (cfg->ns%7<4) cfg->ns=cfg->ns/7*7;
      else cfg->ns=(cfg->ns/7+1)*7;
      printf("rounded to %d\n", cfg->ns);
      continue;
    }

    // post trigger percentage after trigger
    if (strstr(setting, "post_trigger_percentage")!=NULL) {
      read = fscanf(fcfg, "%d", &post);
      cfg->post=post;
      printf("%d\n", cfg->post);
      continue;
    }

    // software trigger mode
    if (strstr(setting, "software_trigger_mode")!=NULL) {
      read = fscanf(fcfg, "%s", option);
      for(i=0; option[i]; i++) option[i] = tolower(option[i]);
      if (strcmp(option, "disabled")==0)
	cfg->swTrgMod = CAEN_DGTZ_TRGMODE_DISABLED;
      else if (strcmp(option, "extout_only")==0)
	cfg->swTrgMod = CAEN_DGTZ_TRGMODE_EXTOUT_ONLY;
      else if (strcmp(option, "acq_only")==0) {
	cfg->swTrgMod = CAEN_DGTZ_TRGMODE_ACQ_ONLY;
        cfg->trgMask |= (1 << 31);
      } else if (strcmp(option, "acq_and_extout")==0) {
	cfg->swTrgMod = CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT;
        cfg->trgMask |= (1 << 31);
      } else {
	printf("%s: invalid trigger mode\n", option);
	return 1;
      }
      printf("%d\n", cfg->swTrgMod);
      continue;
    }

    // internal trigger mode 
    if (strstr(setting, "internal_trigger_mode")!=NULL) {
      read = fscanf(fcfg, "%s", option);
      for(i=0; option[i]; i++) option[i] = tolower(option[i]);
      if (strcmp(option, "disabled")==0)
	cfg->chTrgMod = CAEN_DGTZ_TRGMODE_DISABLED;
      else if (strcmp(option, "extout_only")==0)
	cfg->chTrgMod = CAEN_DGTZ_TRGMODE_EXTOUT_ONLY;
      else if (strcmp(option, "acq_only")==0)
	cfg->chTrgMod = CAEN_DGTZ_TRGMODE_ACQ_ONLY;
      else if (strcmp(option, "acq_and_extout")==0)
	cfg->chTrgMod = CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT;
      else {
	printf("%s: invalid internal trigger mode\n", option);
	return 1;
      }
      printf("%d\n", cfg->chTrgMod);
      continue;
    }

    // external trigger mode
    if (strstr(setting, "external_trigger_mode")!=NULL) {
      read = fscanf(fcfg, "%s", option);
      for(i=0; option[i]; i++) option[i] = tolower(option[i]);
      if (strcmp(option, "disabled")==0)
	cfg->exTrgMod = CAEN_DGTZ_TRGMODE_DISABLED;
      else if (strcmp(option, "extout_only")==0)
	cfg->exTrgMod = CAEN_DGTZ_TRGMODE_EXTOUT_ONLY;
      else if (strcmp(option, "acq_only")==0) {
	cfg->exTrgMod = CAEN_DGTZ_TRGMODE_ACQ_ONLY;
        cfg->trgMask |= (1 << 30);
      } else if (strcmp(option, "acq_and_extout")==0) {
	cfg->exTrgMod = CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT;
        cfg->trgMask |= (1 << 30);
      } else {
	printf("%s: invalid trigger mode\n", option);
	return 1;
      }
      printf("%d\n", cfg->exTrgMod);
      continue;
    }

    // external trigger source
    if (strstr(setting, "external_trigger_source")!=NULL) {
      read = fscanf(fcfg, "%s", option);
      for(i=0; option[i]; i++) option[i] = toupper(option[i]);
      if (strcmp(option, "TTL")==0)
	cfg->exTrgSrc=CAEN_DGTZ_IOLevel_TTL;
      else if (strcmp(option, "NIM")==0)
	cfg->exTrgSrc=CAEN_DGTZ_IOLevel_NIM;
      else {
	printf("%s: invalid trigger source\n", option);
	return 1;
      }
      printf("%d\n", cfg->exTrgSrc);
      continue;
    }

    // channel DC offset
    if (strstr(setting, "channel_dc_offset")!=NULL) {
      read = fscanf(fcfg, "%d", &parameter);
      if (ch==-1) {
	for (i=0;i<Nch;i++) {
	  cfg->offset[i]=GetDCOffset(i,parameter);
	  printf("%d(ch %d) ", cfg->offset[i], i);
	}
	printf("\n");
      } else {
	cfg->offset[ch] = GetDCOffset(ch,parameter);
	printf("%d(ch %d)\n", cfg->offset[ch], ch);
      }
      continue;
    }

    // channel threshold
    if (strstr(setting, "channel_trigger_threshold")!=NULL) {
      read = fscanf(fcfg, "%d", &parameter);
      if (ch == -1) {
	for(i=0; i<Nch; i++) {
	  cfg->thr[i] = parameter;
	  printf("%d(ch %d) ", cfg->thr[i], i);
	}
	printf("\n");
      } else {
	cfg->thr[ch] = parameter;
	printf("%d(ch %d)\n", cfg->thr[ch], ch);
      }
      continue;
    }

    // channel trigger mask (yes/no)
    if (strstr(setting, "channel_enable_trigger")!=NULL) {
      read = fscanf(fcfg, "%s", option);
      for(i=0; i<strlen(option); i++) option[i] = tolower(option[i]);
      if (strcmp(option, "yes")==0) {
	if (ch == -1) for (i=0; i<4; i++) cfg->trgMask |= (1 << i);
	else cfg->trgMask |= (1 << ch);
      } else if (strcmp(option, "no")==0) {
	if (ch == -1) for (i=0; i<4; i++) cfg->trgMask &= ~(1 << i);
	else cfg->trgMask &= ~(1 << ch);
      } else {
	printf("%s: invalid option to enable channel trigger\n", option);
      }
      for (i=32; i-->0;) {
	if ((i+1)%4==0) printf(" ");
	printf("%d",cfg->trgMask>>i&1);
      }
      printf("\n");
      continue;
    }

    // channel trigger polarity
    if (strstr(setting, "channel_trigger_polarity")!=NULL) {
      read = fscanf(fcfg, "%s", option);
      for(i=0; i<strlen(option); i++) option[i] = tolower(option[i]);
      if (strcmp(option, "negative")==0) {
	if (ch==-1) cfg->polarity = 0xf;
	else cfg->polarity |= (1<<ch);
      } else if (strcmp(option, "positive")==0) {
	if (ch==-1) cfg->polarity = 0x0;
	else cfg->polarity &= ~(1<<ch);
      } else {
	printf("%s: invalid trigger polarity\n", option);
	return 1;
      }
      for (i=Nch; i-->0;) printf("%d",cfg->polarity>>i&1); printf("\n");
      continue;
    }

    // channel recording mask (yes/no)
    if (strstr(setting, "channel_enable_recording")!=NULL) {
      read = fscanf(fcfg, "%s", option);
      for(i=0; i<strlen(option); i++) option[i] = tolower(option[i]);
      if (strcmp(option, "yes")==0) {
	if (ch == -1) cfg->mask = 0xf;
	else cfg->mask |= (1 << ch);
      } else if (strcmp(option, "no")==0) {
	if (ch == -1) cfg->mask = 0x0;
	else cfg->mask &= ~(1 << ch);
      } else {
	printf("%s: invalid option to enable channel recording\n", option);
	return 1;
      }
      for (i=Nch; i-->0;) printf("%d",cfg->mask>>i&1); printf("\n");
      continue;
    }

    // coincidence window
    if (strstr(setting, "coincidence_window")!=NULL) {
      read = fscanf(fcfg, "%d", &parameter);
      if (parameter < 16) cfg->trgMask |= (parameter << 20);
      else printf("%d: invalid coincidence window\n",parameter);
      if (parameter==0) printf("1 ns\n");
      else printf("%d ns\n",parameter*16);
      continue;
    }

    // coincidence level
    if (strstr(setting, "coincidence_level")!=NULL) {
      read = fscanf(fcfg, "%d", &parameter);
      if (parameter < 16) cfg->trgMask |= (parameter << 24);
      else printf("%d: invalid coincidence level\n",parameter);
      printf("more than %d channels\n",parameter);
      continue;
    }

    printf("%s: invalid setting\n", setting);
    return 1;
  }
  return 0;
}

