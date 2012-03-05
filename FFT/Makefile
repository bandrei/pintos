CC      = c++
LD      = c++
CFLAGS  = -Wall -O2 -I. -c
LDFLAGS = -lm

INCS = Image.h FFT.h 

OBJS = Image.o FFT.o main.o

all: fft

clean:
	rm -rf $(OBJS) fft

main.o: $(INCS) main.cxx
	$(CC) $(CFLAGS) main.cxx -o main.o

Image.o: $(INCS) Image.cxx
	$(CC) $(CFLAGS) Image.cxx -o Image.o

FFT.o: $(INCS) FFT.cxx
	$(CC) $(CFLAGS) FFT.cxx -o FFT.o

fft: $(OBJS)
	$(LD) $(OBJS) -o fft $(LDFLAGS)
