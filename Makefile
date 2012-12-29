all :
	make -C src/

install :
	cp src/rdis /usr/bin/rdis

clean :
	make -C src clean
