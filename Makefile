all: C-Win-Cygwin C-Win-VS-10

C-Win-Cygwin: c.c
	cc $? -o $@

C-Win-VS-10: c.c
	cl $? /Fe$@
