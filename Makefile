c: c.c
	$(CC) $? -o $@ -lm

C-Win-Cygwin: c.c
	cc $? -o $@

C-Win-VS-10: c.c
	cl $? /Fe$@ wsock32.lib
