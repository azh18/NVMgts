######################################
#
######################################
#source file
#源文件，自动找所有.c和.cpp文件，并将目标定义为同名.o文件
SOURCE  := $(wildcard *.cpp)
OBJS    := $(patsubst %.cpp,%.o,$(patsubst %.cpp,%.o,$(SOURCE)))
  
#target you can change test to what you want
#目标文件名，输入任意你想要的执行文件名
TARGET  := test
  
#compile and lib parameter
#编译参数
CC      := g++
LIBS    :=
LDFLAGS :=
DEFINES :=
INCLUDE := -Iheaders
CFLAGS  := -g -Wall $(DEFINES) $(INCLUDE)
CXXFLAGS:= $(CFLAGS) -DHAVE_CONFIG_H -std=c++11 -m64

#i think you should do anything here
#下面的基本上不需要做任何改动了
.PHONY : everything objs clean veryclean rebuild

everything : $(TARGET)

all : $(TARGET)

objs : $(OBJS)

rebuild: veryclean everything
              
clean :
	rm -fr *.so
	rm -fr *.o
    
veryclean : clean
	rm -fr $(TARGET)

$(TARGET) : $(OBJS)
	gcc -m64 -g -c p_mmap.c -o p_mmap.o
	$(CC) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS) $(LIBS) p_mmap.o
