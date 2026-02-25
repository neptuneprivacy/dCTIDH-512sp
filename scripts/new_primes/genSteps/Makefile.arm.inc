# ARM/mobile build for ctidh-512: use C implementations instead of x86 .S
# Usage: make -f Makefile -f Makefile.arm.inc ctidh-512.main
# Or from project root: make -C scripts/new_primes/genSteps -f Makefile -f Makefile.arm.inc ctidh-512.main

# Build C objects for uintbig and fp (replacing .S)
uintbig512_c.o: uintbig512_c.c uintbig.h uintbig_namespace.h Makefile
	$(CC) -DBITS=512 -D'NAMESPACEBITS(x)=highctidh_512_##x' -D'NAMESPACEGENERIC(x)=highctidh_##x' \
		-c uintbig512_c.c -o uintbig512_c.o

fp512_c.o: fp512_c.c fp.h fp_namespace.h uintbig.h uintbig_namespace.h Makefile
	$(CC) -DBITS=512 -D'NAMESPACEBITS(x)=highctidh_512_##x' -D'NAMESPACEGENERIC(x)=highctidh_##x' \
		-c fp512_c.c -o fp512_c.o

int32_sort_portable.o: int32_sort_portable.c int32_sort.h Makefile
	$(CC) -D'NAMESPACEGENERIC(x)=highctidh_##x' -c int32_sort_portable.c -o int32_sort_portable.o

# Override: build uintbig512.o from C source (so libhighctidh_512.a gets the right object)
uintbig512.o: uintbig512_c.o
	cp uintbig512_c.o uintbig512.o

fp512.o: fp512_c.o
	cp fp512_c.o fp512.o

# Override: build int32_sort.o from portable C (so libtest.a gets the right object)
int32_sort.o: int32_sort_portable.o
	cp int32_sort_portable.o int32_sort.o

# Ensure dependencies are built when building the overridden .o files
uintbig512.o: uintbig512_c.c
fp512.o: fp512_c.c
int32_sort.o: int32_sort_portable.c
