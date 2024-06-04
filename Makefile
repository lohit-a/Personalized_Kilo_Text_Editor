# this is a comment in makefiles

kilo: kilo.c # "kilo" says what to build and "kilo.c" is what is required to build "kilo" 
	$(CC) kilo.c -o kilo -Wall -Wextra -pedantic -std=c99

# ^ the indent MUST be tab (not spaces)

# $(CC) allows make to extend to the C Compiler (CC)
# -Wall == “all Warnings” - compiler warns you when code might not technically be wrong \
but is considered bad practice, like using variables before initializing them
# -Wextra and -pedantic turn on even more warnings
# -std=c99 specifies the standard of C that we are usingh (C99)
    # C99 allows us to declare variables anywhere within a function, \
	whereas ANSI C requires all variables to be declared at the top of a function or block

# ^ observe the use of SPACES (not tab) for the indented comment here 

# the benefit of make is it can tell that the current version of kilo.c \
has already been compiled by looking at each file’s last-modified timestamp. \
If kilo was last modified after kilo.c was last modified, then make assumes \
that kilo.c has already been compiled, \
and so it doesn’t bother running the compilation command