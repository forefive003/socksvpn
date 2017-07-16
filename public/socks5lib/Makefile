
#include ../make.include

CPPFLAGS  =   -ggdb -DCORE_DUMP  -pthread -Wall
CPPFLAGS += -fno-strict-aliasing

CPPFLAGS += -I./ 
CPPFLAGS += -I../xutil/include
CPPFLAGS += -I../netpool/outclass

CPPFLAGS += -rdynamic -fPIC

SOURCES = $(wildcard ./*.cpp )
OBJECTS = $(patsubst %.cpp,%.o,$(SOURCES))
DEPENDS = $(patsubst %.cpp,%.d,$(SOURCES))
ASMFILE = $(patsubst %.cpp,%.s,$(SOURCES))

.PHONY: all clean

#target = libsocks5common.so
target = libsocks5common.a
all: $(target)

$(target): $(OBJECTS)	
#	g++ -shared -o $(target)  $(OBJECTS)  $(LDFLAGS)
#	mv $(target) $(LIB_LOCATION)
	ar -crs $(target)  $(OBJECTS) 
clean:
	@rm -fr $(OBJECTS) $(DEPENDS) $(ASMFILE) $(target)
	@rm -fr *.d *.o *.s 

	
install:
	@cp $(target)  /usr/local/lib/
uninstall:
	@rm  /usr/local/lib/$(target)
