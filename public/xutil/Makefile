

CPPFLAGS  =   -ggdb -DCORE_DUMP  -O2 -pthread -Wall
CPPFLAGS += -fno-strict-aliasing

CPPFLAGS += -I./ -I./src/ -I./include

CPPFLAGS += -rdynamic -fPIC

LDFLAGS += -lpthread 


SOURCES = $(wildcard ./src/*.cpp)
OBJECTS = $(patsubst %.cpp,%.o,$(SOURCES))
DEPENDS = $(patsubst %.cpp,%.d,$(SOURCES))
ASMFILE = $(patsubst %.cpp,%.s,$(SOURCES))

.PHONY: all clean

#target = libxutil.so
target = libxutil.a
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
