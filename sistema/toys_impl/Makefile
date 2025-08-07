CFLAGS=`pkg-config fuse3 --cflags`
LIBS=`pkg-config fuse3 --libs`

all: toyfs

toyfs: toyfs.c
	gcc $(CFLAGS) toyfs.c -o toyfs $(LIBS)

clean:
	rm -f toyfs
