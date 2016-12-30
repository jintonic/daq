CFLAGS = -Wall -O2

# Please define CAENSYS as an environmental variable
# if CAEN libs are not installed in standard place
ifneq (X$(CAENSYS), X)
  CAENLIB = -L$(CAENSYS)/lib
  CFLAGS += -I$(CAENSYS)/include
endif

# include sscanf.c if glibc < 2.7
SRCS = $(wildcard *.c)
GLIBC_MIN_VER = $(shell ldd --version | head -1 | awk -F. '{print $$NF}')
TOO_OLD = $(shell [ $(GLIBC_MIN_VER) -ge 7 ] && echo false)
ifeq ($(TOO_OLD), false)
  SRC=$(filter-out sscanf.c, $(SRCS))
endif
OBJ=$(SRC:.c=.o)
DEP=$(SRC:.c=.d)

#------------------------------------------------------------------------------

# create executable
daq.exe: $(OBJ)
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

