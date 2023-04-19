GCC ?= gcc
CCMODE = PROGRAM
INCLUDES =  -I include
CFLAGS =  -Wall $(MACRO)
TARGET = client.out
SRCS := $(wildcard *.c)  
LIBS = -lpthread -lncurses -D _D # 不启动UI界面
# LIBS = -lpthread -lncurses -D UI # 启动UI界面

ifeq ($(CCMODE),PROGRAM)
$(TARGET): $(LINKS) $(SRCS) 
	$(GCC) $(CFLAGS) $(INCLUDES) -o $(TARGET)  $(SRCS) $(LIBS)
	@chmod +x $(TARGET)
	@echo make $(TARGET) ok.
clean:
	rm -rf $(TARGET)
endif
