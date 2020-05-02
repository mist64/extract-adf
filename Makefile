all:
	cc -o extract-adf extract-adf.c -lz

clean:
	rm extract-adf
