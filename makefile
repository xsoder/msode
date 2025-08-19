default:
	./nob
nob:
	cc -Isrc -o nob nob.c	
run:
	./.ld
clean:
	rm -rf build/
