
#include ../make.include

CPPFLAGS  =   -ggdb -DCORE_DUMP  -pthread -Wall
CPPFLAGS += -fno-strict-aliasing


CPPFLAGS += -I./ -I./src/ -I./include -I./outclass/
CPPFLAGS += -I../xutil/include

CPPFLAGS += -rdynamic -fPIC
LDFLAGS += -lpthread 

SOURCES = $(wildcard ./src/*.cpp ./outclass/*.cpp)
OBJECTS = $(patsubst %.cpp,%.o,$(SOURCES))
DEPENDS = $(patsubst %.cpp,%.d,$(SOURCES))
ASMFILE = $(patsubst %.cpp,%.s,$(SOURCES))

.PHONY: all clean

#target = libnetpool.so
target = libnetpool.a
all: $(target)

$(target): $(OBJECTS)	
#	g++ -shared -o $(target)  $(OBJECTS)  $(LDFLAGS)
	ar -crs $(target) $(OBJECTS)

clean:
	@rm -fr $(OBJECTS) $(DEPENDS) $(ASMFILE) $(target)
	@rm -fr *.d *.o *.s 


install:
	@cp $(target)  /usr/local/lib/
uninstall:
	@rm  /usr/local/lib/$(target)