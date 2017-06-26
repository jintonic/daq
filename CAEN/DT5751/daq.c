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
  int dt5751; CAEN_DGTZ_ErrorCode err = CAEN_DGTZ_Success;
  err = CAEN_DGTZ_OpenDigitizer(CAEN_DGTZ_USB, 0, 0, 0, &dt5751);
  if (err) { printf("Can't open DT5751!"); return 1; }

  // get board info
  CAEN_DGTZ_BoardInfo_t board;
  err = CAEN_DGTZ_GetInfo(dt5751, &board);
  if (err) { printf("Can't get board info!\n"); goto close; }
  printf("Connected to %s\n", board.ModelName);
  printf("ROC FPGA Release: %s\n", board.ROC_FirmwareRel);
  printf("AMC FPGA Release: %s\n", board.AMC_FirmwareRel);

  // get channel temperatures
  uint32_t tC, ich;
  printf("Temperature: ");
  for (ich=0; ich<board.Channels; ich++) {
    err = CAEN_DGTZ_ReadTemperature(dt5751, ich, &tC);
    if (err) printf("N.A.(ch %d) ", ich);
    else printf("%uC(ch %d) ", tC, ich);
  }

  // calibrate board
  err = CAEN_DGTZ_Calibrate(dt5751);
  if (err) { printf("Can't calibrate board!\n"); goto close; }

  // load configurations
  printf("\n\nParsing %s...\n", argv[1]);
  FILE *fcfg = fopen(argv[1],"r");
  if (!fcfg) { perror("Error"); goto close; }
  RUN_CFG_t cfg;
  err = ParseConfigFile(fcfg, &cfg);
  fclose(fcfg);
  if (err) goto close;
  else printf("Done.\n");

  // global settings
  uint16_t nEvtBLT=1024; // number of events for each block transfer
  err |= CAEN_DGTZ_Reset(dt5751);
  err |= CAEN_DGTZ_SetRecordLength(dt5751,cfg.ns);
  err |= CAEN_DGTZ_SetPostTriggerSize(dt5751,cfg.post);
  err |= CAEN_DGTZ_SetMaxNumEventsBLT(dt5751,nEvtBLT);
  err |= CAEN_DGTZ_SetAcquisitionMode(dt5751,CAEN_DGTZ_SW_CONTROLLED);
  err |= CAEN_DGTZ_SetChannelEnableMask(dt5751,cfg.mask);
  err |= CAEN_DGTZ_SetIOLevel(dt5751,cfg.exTrgSrc);
  err |= CAEN_DGTZ_SetExtTriggerInputMode(dt5751,cfg.exTrgMod);
  err |= CAEN_DGTZ_SetSWTriggerMode(dt5751,cfg.swTrgMod);
  err |= CAEN_DGTZ_SetChannelSelfTrigger(dt5751,cfg.chTrgMod,cfg.trgMask);
  for (ich=0; ich<Nch; ich++) { // configure individual channels
    err |= CAEN_DGTZ_SetChannelDCOffset(dt5751,ich,cfg.offset[ich]);
    err |= CAEN_DGTZ_SetChannelTriggerThreshold(dt5751,ich,cfg.thr[ich]);
    err |= CAEN_DGTZ_SetTriggerPolarity(dt5751,ich,cfg.polarity>>ich&1);
  }
  if (err) { printf("Board configure error: %d\n", err); goto close; }
  sleep(1); // wait till baseline get stable

  // allocate memory for data taking
  char *buffer = NULL; uint32_t bytes;
  err = CAEN_DGTZ_MallocReadoutBuffer(dt5751,&buffer,&bytes);
  if (err) { printf("Can't allocate memory! Quit.\n"); goto close; }
  else printf("\nAllocated %d kB of memory\n",bytes/1024);

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
  err = CAEN_DGTZ_SWStartAcquisition(dt5751);
  if (err) { printf("Can't start acquisition! Quit.\n"); goto free; }

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
  uint32_t bsize, fsize=0;
  while (nEvtTot<nNeeded && !interrupted) {
    // send 1 kHz software trigger to the board for "nEvtBLT" times
    if (cfg.swTrgMod!=CAEN_DGTZ_TRGMODE_DISABLED)
      for (i=0; i<nEvtBLT; i++) {
	CAEN_DGTZ_SendSWtrigger(dt5751);
	nanosleep((const struct timespec[]){{0, 1e6L}}, NULL);
      }

    // read data from board
    CAEN_DGTZ_ReadMode_t mode=CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT;
    err = CAEN_DGTZ_ReadData(dt5751,mode,buffer,&bsize);
    if (err) { printf("Can't read data from board! Abort.\n"); break; }
    fsize += bsize;

    // write data to file
    err = CAEN_DGTZ_GetNumEvents(dt5751,buffer,bsize,&nEvtOnBoard);
    if (err) { printf("nEvtOnBoard: %d, err: %d\n", nEvtOnBoard, err); break; }
    for (i=0; i<nEvtOnBoard; i++) {
      CAEN_DGTZ_EventInfo_t info; char *rawEvt = NULL;
      err = CAEN_DGTZ_GetEventInfo(dt5751,buffer,bsize,i,&info,&rawEvt);
      if (err) { printf("i: %d, err: %d\n", i, err); break; }

      EVT_HDR_t *header = (EVT_HDR_t*) rawEvt; // update event header
      uint32_t evt_size_hdr_excluded = header->size; // save it
      header->size=info.EventSize;
      header->cnt=info.EventCounter;
      header->ttt=info.TriggerTimeTag;
      header->type=1; // a real event
      fwrite(rawEvt,1,info.EventSize,output);
      header->size=evt_size_hdr_excluded; // reset it, otherwise, error occur

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
free:
  CAEN_DGTZ_FreeReadoutBuffer(&buffer);
close:
  CAEN_DGTZ_CloseDigitizer(dt5751);

  return 0;
}

