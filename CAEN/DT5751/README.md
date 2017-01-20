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

# Get DC offset calibration curve

Simply run `./offset.sh`. It calls `offset.C`, which is a CERN ROOT macro.
[CERN ROOT](https://root.cern.ch/) must be installed for `offse.C` to run. The
data will be saved in a file called `offset.dat`. Four png files will be
generated from the data file, corresponding to 4 channels. Each contains a plot
of the 16-bit DAC count versus the 10-bit ADC count, which is fitted by the
calibration curve _p0+p1*x_. The best fitted values of the free parameters,
_p0_ and _p1_, will be shown on screen.

