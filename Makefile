build:
	make build-shared

build-shared:
	mkdir -p build
	gcc -c -fpic src/zrfs.c -o build/zrfs.o
	gcc -c -fpic src/acl.c -o build/acl.o
	gcc -shared -o build/libzrfs.so build/*.o -lzprotocol
	rm -f build/*.o

	mkdir -p build/include build/lib
	cp build/libzrfs.so build/lib/libzrfs.so
	cp src/zrfs.h build/include/zrfs.h

build-static:
	mkdir -p build
	gcc -c src/zrfs.c -o build/zrfs.o
	gcc -c src/acl.c -o build/acl.o
	ar rs build/libzrfs.a build/*.o
	rm -f build/*.o

	mkdir -p build/include build/lib
	cp build/libzrfs.a build/lib/
	cp src/zrfs.h build/include/

install-static:
	cp build/libzrfs.a /usr/lib/
	cp build/include/zrfs.h /usr/include/

build-cli:
	mkdir -p build
	gcc src/* -o build/zrfs -lzprotocol -lsodium -lpthread `pkg-config fuse3 --cflags --libs`

install:
	cp build/libzrfs.so /usr/lib/libzrfs.so
	cp build/include/zrfs.h /usr/include/zrfs.h

install-cli:
	cp build/zrfs /usr/local/bin/zrfs

remove:
	rm -f /usr/lib/libzrfs.so
	rm -f /usr/lib/libzrfs.a
	rm -f /usr/include/zrfs.h

clean:
	rm -rf build

backup:
	mkdir -p ../zrfs-c_backup && \
	cd ../zrfs-c_backup && \
	cp -R ../zrfs-c "zrfs-c_backup_$$(date "+%Y-%m-%d_%H%M%S")"
