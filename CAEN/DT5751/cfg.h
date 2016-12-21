#ifndef CFG_H
#define CFG_H

#include <stdio.h>
#include <stdint.h>

#ifndef Nch
#define Nch (4) ///< total number of channels
#endif

typedef enum {
   SOFTWARE_TRG,
   INTERNAL_TRG,
   EXTERNAL_TTL,
   EXTERNAL_NIM,
} TRG_MODE_t; ///< trigger mode

/**
 * Event header defined by CAEN.
 * It is 4 words long, and is explained in DT5751 User's Manual:
 * http://www.caen.it/servlet/checkCaenManualFile?Id=11299
 */
typedef struct {
   unsigned size   :28;///< size of event header + size of data
   unsigned begin  :4; ///< always 1010
   unsigned mask   :4; ///< channel enable/disable mask
   unsigned pattern:16;///< not used
   unsigned zle    :1; ///< 0 if zero-length encoding firmware is not used
   unsigned res    :2; ///< not used
   unsigned board  :5; ///< not used
   unsigned cnt    :24;///< trigger counter
   unsigned type   :8; ///< 0: run config, 1: real event
   unsigned ttt    :32;///< trigger time tag, up to (8 ns)*(2^31-1)
} EVT_HDR_t;

/**
 * Run configurations.
 * RUN_CFG::mask is explained in DT5751 User's Manual:
 * http://www.caen.it/servlet/checkCaenManualFile?Id=11299
 */
typedef struct {
   uint16_t run;        ///< run number, up to 2^16 = 65536
   uint8_t  subrun;     ///< sub run number, up to 2^8 = 256
   uint8_t  version;    ///< binary format version number, up to 2^8 = 256
   uint32_t tsec;       ///< time from OS in second
   uint32_t tus;        ///< time from OS in micro second
   uint32_t ns;         ///< number of samples in each wf
   uint8_t  post;       ///< percentage of wf after trg, 0 ~ 100
   uint8_t  mode;       ///< trigger mode
   uint32_t mask;       ///< trigger mask and coincidence window
   uint16_t thr[Nch];   ///< 0 ~ 2^10-1
   uint16_t offset[Nch];///< 16-bit DC offset
   uint16_t reserved;
} RUN_CFG_t;

void SaveCurrentTime(RUN_CFG_t *cfg); ///< Save OS time to \param cfg.

int ParseConfigFile(FILE *fcfg, RUN_CFG_t *cfg);

#endif

