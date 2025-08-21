default:
	cc -Isrc -o nob nob.c		
	./nob
run:default
	./.ld
clean:
	rm -rf build/
