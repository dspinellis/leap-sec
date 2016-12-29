c: c.c
	$(CC) $? -o $@

C-Win-Cygwin: c.c
	cc $? -o $@

C-Win-VS-10: c.c
	cl $? /Fe$@ wsock32.lib
