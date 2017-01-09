# Getting started

1. type `make` to compile `daq.c`
2. edit `daq.cfg` to configure your run
3. run `./daq.exe daq.cfg 123`, where 123 is a run number

# Data structure

The data structure is defined in `cfg.h`. The first and last events in a run
are not real events. They have a normal event header defined as EVT_HDR_t,
followed by RUN_CFG_t, which records run configurations. Other events are
physical events with the same event header and raw event data structure as
described in the DT5751 user's manual.

