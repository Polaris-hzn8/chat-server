GCC ?= gcc
CCMODE = PROGRAM
INCLUDES =  -I ../common
CFLAGS =  -Wall $(MACRO)
TARGET = server.out
SRCS := $(wildcard *.c)  
LIBS = -lpthread -D _D # 需要引用的库

ifeq ($(CCMODE),PROGRAM)
$(TARGET): $(LINKS) $(SRCS) 
	$(GCC) $(CFLAGS) $(INCLUDES) -o $(TARGET)  $(SRCS) $(LIBS)
	@chmod +x $(TARGET)
	@echo make $(TARGET) ok.
clean:
	rm -rf $(TARGET)
endif

 
 


