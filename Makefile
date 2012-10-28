all :
	make -C src/


install :
	mv src/rdis /usr/bin/rdis

clean :
	make -C src clean