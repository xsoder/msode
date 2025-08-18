default:
	./nob
nob:
	cc -o -Isrc nob nob.c	
run:
	./.ld

clean:
	rm -rf build/

