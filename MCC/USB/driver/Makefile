HDRS = $(wildcard *.h)
TEST = $(wildcard test*.c)
SRCS = $(filter-out $(TEST), $(wildcard *.c))
EXES = $(TEST:.c=.exe)
OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)
NAME = mccusb
CFLAGS = -Wall -fPIC -O2

.PHONY: all clean install uninstall

all: $(EXES)

$(EXES):%.exe:%.c lib$(NAME).so
	$(CC) $(CFLAGS) -I. $< -o $@ -L. -l$(NAME) -lm -lhidapi-libusb -lusb-1.0
	@echo $@ created

lib$(NAME).so: $(OBJS)
	$(CC) $(CFLAGS) -shared -o $@ $^ -lc -lm
	@echo $@ created

ifeq ($(filter install uninstall clean, $(MAKECMDGOALS)),)
  -include $(DEPS)
endif

%.d:%.c
	@set -e; rm -f $@; \
	  $(CC) -MM $(CFLAGS) $< > $@.$$$$; \
	  sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	  rm -f $@.$$$$
	@echo $@ created

clean:
	rm -rf *.o *.d *~ *.exe *.so

install:
	-install -d /usr/local/lib
	-install -c --mode=0755 lib$(NAME).so /usr/local/lib
	-/bin/ln -s /usr/local/lib/lib$(NAME).so /usr/lib/lib$(NAME).so
	-install -d /usr/local/include/libusb
	-install -c --mode=0644 ${HDRS} /usr/local/include/libusb/

uninstall:
	-rm -f /usr/local/lib/lib$(NAME)*
	-rm -f /usr/lib/lib$(NAME)*
	-rm -rf /usr/local/include/libusb
