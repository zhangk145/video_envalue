CC=g++
SRC=$(wildcard *.cpp)
OBJ=$(SRC:.cpp=.o)
PRF=-pg
CCFLAGS=-I/usr/local/include -Wall -O -g
LD_PATH=-L.
LD_NAME=-lavcodec -lavformat -lavutil
# LD_NAME=libavcodec.a libavformat.a libavutil.a
# COP=-W -Wall -Wno-sign-compare -Wno-multichar -Wno-pointer-sign -Wno-parentheses -Wno-missing-field-initializers -Wno-missing-braces -O3
DST=VideoEnvaluer
INSTDIR=/usr/local/bin

all: $(DST)

VideoEnvaluer: Decoder.o Envaluer.o Main.o
	@echo L $@ ...
	@$(CC) $^ -o $@ -lm $(CCFLAGS) $(LD_PATH) $(LD_NAME)

# %.o: %.cpp
# 	@echo C $< ...
# 	@$(CC) $(COP) $(INCLUDE_PATH) $(LIB_PATH) $(LIB_NAME) -c $<

clean:
	@rm -f $(OBJ) $(DST) *.o-*
