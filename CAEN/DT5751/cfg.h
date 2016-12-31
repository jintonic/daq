#ifndef CFG_H
#define CFG_H

#include <stdio.h>
#include <stdint.h>

#ifndef Nch
#define Nch (4) ///< total number of channels
#endif

#ifndef VERSION
#define VERSION (0) ///< versin of binary data structure
#endif

typedef enum { TTL, NIM, } TRG_SRC_t; ///< Source of trigger signal

typedef struct {
   uint32_t size;///< size of event header + size of data
   int32_t cnt; ///< trigger counter
   int32_t ttt; ///< trigger time tag, up to (8 ns)*(2^31-1)
   uint8_t type; ///< 0: run config, 1: real event
   unsigned reserved:24;
} EVT_HDR_t; ///< Event header

/**
 * Run configurations.
 * It is 10 words long and is saved as the first and last events of a run.
 * RUN_CFG_t::trgMask is explained in DT5751 User's Manual:
 * http://www.caen.it/servlet/checkCaenManualFile?Id=11299
 */
typedef struct {
   uint16_t run;        ///< run number, up to 2^16 = 65536
   uint8_t  sub;        ///< sub run number, up to 2^8 = 256
   uint8_t  ver;        ///< binary format version number, up to 2^8 = 256
   uint32_t tsec;       ///< time from OS in second
   uint32_t tus;        ///< time from OS in micro second
   uint32_t ns;         ///< number of samples in each wf
   unsigned mask:    4; ///< channel enable/disable mask
   unsigned swTrgMod:2; ///< software trigger mode: CAEN_DGTZ_TriggerMode_t
   unsigned chTrgMod:2; ///< internal trigger mode: CAEN_DGTZ_TriggerMode_t
   unsigned exTrgMod:2; ///< external trigger mode: CAEN_DGTZ_TriggerMode_t
   unsigned exTrgSrc:1; ///< external trigger source: 0 - TTL, 1 - NIM
   unsigned post:    7; ///< percentage of wf after trg, 0 ~ 100
   unsigned reserved:14;
   uint32_t trgMask;    ///< trigger mask and coincidence window
   uint16_t thr[Nch];   ///< 0 ~ 2^10-1
   uint16_t offset[Nch];///< 16-bit DC offset
} RUN_CFG_t;

void SaveCurrentTime(RUN_CFG_t *cfg); ///< Save OS time to \param cfg.

int ParseConfigFile(FILE *fcfg, RUN_CFG_t *cfg);

#endif

