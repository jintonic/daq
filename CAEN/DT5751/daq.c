#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <CAENDigitizer.h>
#include "cfg.h"

#define CAEN_USE_DIGITIZERS
#define IGNORE_DPP_DEPRECATED

volatile sig_atomic_t interrupted = 0;
void ctrl_c_pressed(int sig) { interrupted = 1; }


int main(int argc, char* argv[])
{
  if (argc<3) {
    printf("Usage: ./daq.exe daq.cfg run_num [num_of_evt_to_be_taken]\n");
    return 1;
  }
  signal(SIGINT, ctrl_c_pressed); // handle Ctrl-C

  // connect to digitizer
  int dt5751;
  if (CAEN_DGTZ_OpenDigitizer(CAEN_DGTZ_USB, 0, 0, 0, &dt5751)) {
    printf("Can't open DT5751! Quit.\n");
    return 1;
  }

  // get board info
  CAEN_DGTZ_BoardInfo_t board;
  if (CAEN_DGTZ_GetInfo(dt5751, &board)) { 
    printf("Can't get board info! Quit.\n");
    goto quit;
  }
  printf("Connected to %s\n", board.ModelName);
  printf("ROC FPGA Release is %s\n", board.ROC_FirmwareRel);
  printf("AMC FPGA Release is %s\n", board.AMC_FirmwareRel);

  // get channel temperatures
  uint32_t tC, ich;
  for (ich=0; ich<board.Channels; ich++) {
    printf("Temperature of ch %d: ", ich);
    if (CAEN_DGTZ_ReadTemperature(dt5751, ich, &tC)) printf("N.A.\n");
    else printf("%u degree C\n", tC);
  }

  // calibrate board
  if (CAEN_DGTZ_Calibrate(dt5751)) { 
    printf("Can't calibrate board! Quit.\n");
    goto quit;
  }

  // load configurations
  printf("\nParse %s\n", argv[1]);
  FILE *fcfg = fopen(argv[1],"r");
  if (!fcfg) { printf("cannot open %s, quit\n",argv[1]); goto quit; }
  RUN_CFG_t cfg;
  CAEN_DGTZ_ErrorCode err = CAEN_DGTZ_Success;
  err = ParseConfigFile(fcfg, &cfg);
  fclose(fcfg);
  if (err) goto quit;

  // global settings
  uint16_t nEvtBLT=256; // number of events for each block transfer
  err |= CAEN_DGTZ_Reset(dt5751);
  err |= CAEN_DGTZ_SetRecordLength(dt5751,cfg.ns);
  err |= CAEN_DGTZ_SetPostTriggerSize(dt5751,cfg.post);
  err |= CAEN_DGTZ_SetIOLevel(dt5751,cfg.exTrgSrc);
  err |= CAEN_DGTZ_SetMaxNumEventsBLT(dt5751,nEvtBLT);
  err |= CAEN_DGTZ_SetAcquisitionMode(dt5751,CAEN_DGTZ_SW_CONTROLLED);
  err |= CAEN_DGTZ_SetExtTriggerInputMode(dt5751,cfg.exTrgMod);
  err |= CAEN_DGTZ_SetSWTriggerMode(dt5751,cfg.swTrgMod);
  err |= CAEN_DGTZ_SetChannelEnableMask(dt5751,cfg.mask);
  err |= CAEN_DGTZ_SetChannelSelfTrigger(dt5751,cfg.chTrgMod,cfg.trgMask);
  printf("ch offset threshold polarity trigger record\n");
  for (ich=0; ich<Nch; ich++) { // configure individual channels
    if (cfg.trgMask & (1<<ich) || cfg.mask & (1<<ich)) {
      err |= CAEN_DGTZ_SetChannelDCOffset(dt5751,ich,cfg.offset[ich]);
      err |= CAEN_DGTZ_SetChannelTriggerThreshold(dt5751,ich,cfg.thr[ich]);
      err |= CAEN_DGTZ_SetTriggerPolarity(dt5751,ich,cfg.polarity>>ich&1);
      printf("%d %6d %8d %7d %7d %7d\n", ich, cfg.offset[ich], cfg.thr[ich],
	  cfg.polarity>>ich&1, cfg.trgMask>>ich&1, cfg.mask>>ich&1);
    }
  }
  if (err) { 
    printf("Can't configure board! Quit with error code %d.\n", err);
    goto quit;
  }
  sleep(1); // wait till baseline get stable

  // allocate memory for data taking
  char *buffer = NULL; uint32_t bytes;
  if (CAEN_DGTZ_MallocReadoutBuffer(dt5751,&buffer,&bytes)) {
    printf("Can't allocate memory! Quit.\n");
    goto quit;
  }
  printf("\nAllocated %d kB of memory\n",bytes/1024);

  // open output file
  cfg.run=atoi(argv[2]);
  cfg.sub=1;
  char fname[32];
  sprintf(fname, "run_%06d.%06d",cfg.run,cfg.sub);
  printf("Open output file %s\n", fname);
  FILE *output = fopen(fname,"wb");

  // start acquizition
  time_t t = time(NULL);
  struct tm lt = *localtime(&t);
  printf("Run %d starts at %d/%d/%d %d:%d:%d\n", cfg.run, lt.tm_year+1900,
      lt.tm_mon+1, lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec);
  if (CAEN_DGTZ_SWStartAcquisition(dt5751)) {
    printf("Can't start acquisition! Quit.\n");
    CAEN_DGTZ_FreeReadoutBuffer(&buffer);
    goto quit;
  }

  // write run cfg
  SaveCurrentTime(&cfg);
  EVT_HDR_t hdr;
  hdr.size=sizeof(EVT_HDR_t)+sizeof(RUN_CFG_t);
  hdr.cnt=0; hdr.ttt=0; hdr.type=0;
  fwrite(&hdr,1,sizeof(EVT_HDR_t),output);
  fwrite(&cfg,1,sizeof(RUN_CFG_t),output);

  // record start time
  int now, tpre = now = cfg.tsec*1000 + cfg.tus/1000, dt, runtime=0;

  // output loop
  int i, nEvtTot=0, nNeeded = 16777216, nEvtIn5sec=0; uint32_t nEvtOnBoard;
  if (argc==4) nNeeded=atoi(argv[3]);
  uint32_t bsize, fsize=0; char *evt = NULL;
  while (nEvtTot<=nNeeded && !interrupted) {
    // send 1 kHz software trigger to the board for "nEvtBLT" times
    if (cfg.swTrgMod!=CAEN_DGTZ_TRGMODE_DISABLED)
      for (i=0; i<nEvtBLT; i++) {
	CAEN_DGTZ_SendSWtrigger(dt5751);
	nanosleep((const struct timespec[]){{0, 1e6L}}, NULL);
      }

    // read data from board
    CAEN_DGTZ_ReadMode_t mode=CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT;
    CAEN_DGTZ_ReadData(dt5751,mode,buffer,&bsize);
    fsize += bsize;

    // write data to file
    CAEN_DGTZ_GetNumEvents(dt5751,buffer,bsize,&nEvtOnBoard);
    for (i=0; i<nEvtOnBoard; i++) {
      CAEN_DGTZ_EventInfo_t info;
      CAEN_DGTZ_GetEventInfo(dt5751,buffer,bsize,i,&info,&evt);
      EVT_HDR_t *header = (EVT_HDR_t*) evt; // update event header
      header->size=info.EventSize;
      header->cnt=info.EventCounter;
      header->ttt=info.TriggerTimeTag;
      header->type=1; // a real event
      fwrite(evt,1,info.EventSize,output);

      nEvtTot++; nEvtIn5sec++;
      if (nEvtTot>=nNeeded) break;
    }

    // display progress every 5 seconds
    struct timeval tv;
    gettimeofday(&tv,NULL);
    now = tv.tv_sec*1000 + tv.tv_usec/1000;
    runtime = now - (cfg.tsec*1000+cfg.tus/1000);
    dt = now - tpre;
    if (dt>5000) {
      printf("-------------------------\n");
      printf("run lasts: %6.2f min\n",(float)runtime/60000.);
      printf("now event: %d\n", nEvtTot);
      printf("trg rate:  %6.2f Hz\n", (float)nEvtIn5sec/(float)dt*1000.);
      printf("press Ctrl-C to stop run\n");
      tpre=now;
      nEvtIn5sec=0;
    }

    // start a new sub run when previous file is over 1GB
    if (fsize>1024*1024*1024) {
      fsize=0;
      printf("\nClose output file %s\n", fname);
      SaveCurrentTime(&cfg);
      hdr.size=sizeof(EVT_HDR_t)+sizeof(RUN_CFG_t);
      hdr.cnt=0; hdr.ttt=0; hdr.type=0;
      fwrite(&hdr,1,sizeof(EVT_HDR_t),output);
      fwrite(&cfg,1,sizeof(RUN_CFG_t),output);
      fclose(output);

      cfg.sub++;
      sprintf(fname, "run_%06d.%06d",cfg.run,cfg.sub);
      printf("Open output file %s\n", fname);
      output = fopen(fname,"wb");
      fwrite(&hdr,1,sizeof(EVT_HDR_t),output);
      fwrite(&cfg,1,sizeof(RUN_CFG_t),output);
    }
  }

  printf("\nClose output file %s\n", fname);
  SaveCurrentTime(&cfg);
  hdr.size=sizeof(EVT_HDR_t)+sizeof(RUN_CFG_t);
  hdr.cnt=0; hdr.ttt=0; hdr.type=0;
  fwrite(&hdr,1,sizeof(EVT_HDR_t),output);
  fwrite(&cfg,1,sizeof(RUN_CFG_t),output);
  fclose(output);

  t = time(NULL);
  lt = *localtime(&t);
  printf("Run %d stops at %d/%d/%d %d:%d:%d\n", cfg.run, lt.tm_year+1900,
      lt.tm_mon+1, lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec);
  CAEN_DGTZ_SWStopAcquisition(dt5751);
  CAEN_DGTZ_FreeReadoutBuffer(&buffer);
quit:
  CAEN_DGTZ_CloseDigitizer(dt5751);

  return 0;
}

