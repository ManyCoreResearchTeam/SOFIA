

CROSS_COMPILE=arm-linux-gnueabihf-

CC=$(CROSS_COMPILE)gcc
AS=$(CROSS_COMPILE)as
LD=$(CROSS_COMPILE)ld
AR=$(CROSS_COMPILE)ar

JC=javac
JH=javah
JR=jar
### JDK_PATH must be set to build gem5OpJni
#JDK_PATH=/path/to/jdk/version_number

CFLAGS=-O2 -I $(JDK_PATH)/include/ -I $(JDK_PATH)/include/linux -march=armv7-a
LDFLAGS=-L. -lm5 -static

LIB_OBJS=m5op_arm.o
OBJS=m5.o
JNI_OBJS=m5op_arm.o jni_gem5Op.o

all: libm5.a m5

%.o: %.S
	$(CC) $(CFLAGS) -o $@ -c $<

%.o: %.c
	$(CC)  $(CFLAGS) -o $@ -c $<

m5: $(OBJS) libm5.a
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

libm5.a: $(LIB_OBJS)
	$(AR) rcs $@ $<

gem5OpJni: gem5OpJni.jar $(JNI_OBJS)
	$(CC) --shared -o lib$@.so $(JNI_OBJS)

gem5OpJni.jar:
	$(JC) jni/gem5Op.java; \
	$(JH) jni.gem5Op; \
	$(JR) cvf $@ jni/*.class

clean:
	rm -f *.o m5 libgem5OpJni.so gem5OpJni.jar jni/*.class libm5.a \
	jni_gem5Op.h
