all: clean httpd

httpd: main.o initvals.o sockdata.o dispatch.o filenames.o write.o mime.o
	gcc main.o initvals.o sockdata.o dispatch.o filenames.o write.o mime.o -o httpd -D_GNU_SOURCE -std=c99

main.o: main.c
	gcc -c main.c -D_GNU_SOURCE -std=c99

initvals.o: initvals.c initvals.h
	gcc -c initvals.c -D_GNU_SOURCE -std=c99

sockdata.o: sockdata.c sockdata.h
	gcc -c sockdata.c -D_GNU_SOURCE -std=c99

dispatch.o: dispatch.c dispatch.h
	gcc -c dispatch.c -D_GNU_SOURCE -std=c99

filenames.o: filenames.c filenames.h
	gcc -c filenames.c -D_GNU_SOURCE -std=c99

write.o: write.c write.h
	gcc -c write.c -D_GNU_SOURCE -std=c99

mime.o: mime.c mime.h
	gcc -c mime.c -D_GNU_SOURCE -std=c99

clean:
	rm -rf *.o *.h.gch httpd main
