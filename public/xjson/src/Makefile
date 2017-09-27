
CFLAGS  =   -ggdb -DCORE_DUMP  -O2 -pthread -Wall
CFLAGS += -fno-strict-aliasing
CFLAGS += -I./
CFLAGS += -rdynamic -fPIC

LDFLAGS += -lpthread 


SOURCES = $(wildcard ./*.c)
OBJECTS = $(patsubst %.c,%.o,$(SOURCES))
DEPENDS = $(patsubst %.c,%.d,$(SOURCES))
ASMFILE = $(patsubst %.c,%.s,$(SOURCES))

.PHONY: all clean

#target = libxutil.so
target = libxjson.a
all: $(target)

$(target): $(OBJECTS)	
#	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJECTS) -o $(target)
#	g++ -shared -o $(target)  $(OBJECTS)  $(LDFLAGS)
#	mv $(target) $(LIB_LOCATION)
	ar -crv $(target)  $(OBJECTS)
	
clean:
	@rm -fr $(OBJECTS) $(DEPENDS) $(ASMFILE) $(target)
	@rm -fr *.d *.o *.s 
