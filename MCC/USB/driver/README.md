This is a better Makefile that can be used with the MCC Linux drivers available from https://github.com/wjasper/Linux_Drivers/tree/master/USB/mcc-libusb. Simply replace the original Makefile with this one and type

```
make
```

A shared library `libmccusb.so` should be created in the same directory. The compilation of some of the test files may fail. But that is not a problem of the Makefile and it does not affect the generation of the shared libray. Simply delete the problematic test file if you don't use that particular device and type `make` again, the make process should be able to go through.

To use the library, one can either execute the following commands as root

```
sudo make install
sudo ldconfig
```

or put the following line to your `~/.profile` as a normal user

```
export LD_LIBRARY_PATH=/path/to/libmccusb.so:$LD_LIBARY_PATH
```

