GCC ?= gcc
CCMODE = PROGRAM
INCLUDES =  -I include
CFLAGS =  -Wall $(MACRO)
TARGET = server.out
SRCS := $(wildcard *.c)  
# LIBS = -lpthread -lncurses # 不启动UI界面
# LIBS = -lpthread -lncurses -D UI # 启动UI界面
LIBS = -lpthread -lncurses -D _D # 启动服务端调试模式

ifeq ($(CCMODE),PROGRAM)
$(TARGET): $(LINKS) $(SRCS) 
	$(GCC) $(CFLAGS) $(INCLUDES) -o $(TARGET)  $(SRCS) $(LIBS)
	@chmod +x $(TARGET)
	@echo make $(TARGET) ok.
clean:
	rm -rf $(TARGET)
endif

 
 


