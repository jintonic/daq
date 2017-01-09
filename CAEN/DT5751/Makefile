CFLAGS = -Wall -O2

# Please define CAENSYS as an environmental variable
# if CAEN libs are not installed in standard place
ifneq (X$(CAENSYS), X)
  CAENLIB = -L$(CAENSYS)/lib
  CFLAGS += -I$(CAENSYS)/include
endif

# include sscanf.c if glibc < 2.7
SRC = cfg.c daq.c
GLIBC_MIN_VER = $(shell ldd --version | head -1 | awk -F. '{print $$NF}')
TOO_OLD = $(shell [ $(GLIBC_MIN_VER) -lt 7 ] && echo true)
ifeq ($(TOO_OLD), true)
  SRC+=sscanf.c
endif
OBJ=$(SRC:.c=.o)
DEP=$(SRC:.c=.d)

#------------------------------------------------------------------------------

# create executables
daq.exe: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(CAENLIB) -lCAENDigitizer

calibrate.exe: calibrate.c
	$(CC) $(CFLAGS) -o $@ $^ $(CAENLIB) -lCAENDigitizer

# include *.d files, which are makefiles defining dependencies between files
ifeq ($(filter clean,$(MAKECMDGOALS)),)
  -include $(DEP)
endif

# rules to create *.d files
%.d:%.c
	@echo create $@
	@set -e; rm -f $@; \
	  $(CXX) -MM $(CPPFLAGS) $< > $@.$$$$; \
	  sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	  rm -f $@.$$$$ 

clean:
	@for file in `cat .gitignore`; do rm -f $$file; done

.PHONY: clean

