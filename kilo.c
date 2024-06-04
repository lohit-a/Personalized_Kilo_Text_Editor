/*** includes ***/

#include <unistd.h> // contains misc constants and functions
#include <termios.h> // used to modify the attributes of terminal (ex. turn off ECHO)
#include <stdlib.h> // used to import atexit() to do stuff after program termination
#include <stdio.h>
#include <ctype.h>
#include <errno.h> // for error handling

/*** data ***/

struct termios orig_attr; // use global variable to get original attributes of terminal (inside enableRawMode())

/*** terminal ***/

void die(const char* s){ // for error handling
    // most C library functions set the global errno variable (in errno.h) to indicate failure and what happened
    perror(s); // from stdio.h and prints s then a descriptive message of the error by looking at the global errno variable
    exit(1); // from stdlib.h and terminates program with return of 1 which indicates failure (because it is non zero)

    // tscsetattr, tcgetattr, and read() all return -1 on failure and set errno to indicate what happened
}

void disableRawMode(void){ 
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_attr) == -1){ die("tcsetattr() failed"); } 
}

// how to change terminal's attributes:
    // 1. use tcgerattr() to get current attributes as a struct
    // 2. change contents of struct
    // 3. use tcsetattr() to write changes back to terminal
    // turn off ECHO this way
void enableRawMode(void){
    if (tcgetattr(STDIN_FILENO, &orig_attr) == -1){ die("tcgetattr() failed"); } // step 1
    // we can fail tcgetattr by passing in a text file or pipe as standard input instead of terminal - for text file, run something like ./kilo <kilo.c
    
    struct termios raw_attr = orig_attr; // makes a COPY of orig_attr into raw_attr so that orig traits are preserved (orig used in disable())
    raw_attr.c_iflag &= ~(IXON | ICRNL | BRKINT | ISTRIP | INPCK);
    // iflags are input flags ('I' for input such as in IXON)
    // IXON is for ctrl-s and ctrl-q used for flow control == not needed (https://en.wikipedia.org/wiki/Software_flow_control#)
        // so now they just read the ASCII values and do nothing
    // ICRNL fixes ctrl-m (https://viewsourcecode.org/snaptoken/kilo/02.enteringRawMode.html#fix-ctrl-m)
    // the last 3 basically not needed but was part of the orignal "raw mode" so we continue the tradition
    raw_attr.c_oflag &= ~(OPOST); // oflag for output flag (affects output related things on terminal)
    // terminal converts all return (newline ('\n') command) inputs into '\r\n' == carriage return AND newline
    // so "return" requires BOTH in practice - \r pushes cursor back to left side of screen and \n pushes cursor to next line
    // OPOST turns off this automatic conversion (but this means we must account for it when using printf() to output)
        // https://viewsourcecode.org/snaptoken/kilo/02.enteringRawMode.html#turn-off-all-output-processing
    raw_attr.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN); // step 2 - equivalent to: = raw.c_lflag & ~(ECHO | etc)
    // lflags are local/misc flags - the "I" in ICANON and ISIG is NOT for "input" bc they are not input flags
    // ICANON turns off canonical mode (allows input to be read BYTE BY BYTE)
    // IEXTEN turns off ctrl-v
    // ISIG turns off SIGINT (interrupt signal) and SIGTSTP which are triggered by ctrl-c and ctrl-z respectively (terminate program)
        // so now instead of termination, inputting them just get read as their ASCII values and do nothing
        // also disables ctrl-y on macOS
    // ECHO, ICANON, etc are 32-bit flags defined in termios (https://en.wikipedia.org/wiki/Bit_field#)
    // they are turned on by flipping exactly one unique bit to 1; the fourth bit for ECHO for example
    // we take the bitwise OR of all the bits we want to change (https://stackoverflow.com/questions/48477989/termios-syntax-configuration-with-bitwise-operators)
    // then bitwise NOT the result. then bitwise AND to original attributes - this makes the bits for just the flags we want (ECHO etc) to turn off
    raw_attr.c_cflag |= CS8; // sets character size (CS) to 8 BITS PER BYTE (this is already true on most modern systems though so probably not needed)
    // CS8 is not a flag - it is a bit mask and is set using | instead of &, unlike flags

    // we want read() to return 0 automatically if user does not input anything for certain amount of time:
    raw_attr.c_cc[VMIN] = 0; // c_cc stands for control chars field - an array of bytes that control terminal stuff
    raw_attr.c_cc[VTIME] = 1;
    // vmin sets the MIN number of bytes of input needed before read() can return. 0 so that it can return the moment there is input
    // vtime sets the MAX amount of time that read() waits for input before automatic termination - it is in tenths of a second
        // = 1 means "if user does not input within 1/10 of a second, then terminate"

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw_attr) == -1){ die("tcsetattr() failed in enable()"); } 
    // tcsetattr() applies the change made in step 2 to the terminal
    // tcsaflush says WHEN to apply the change - waits for all pending output
    // to be written to terminal and discards unread input then applies changes
}

/*** init ***/

int main(void){ // in C, use void in function definitions to distingush from a prototype vs a no parameter function
    // goal is to read individual keypresses from user
    enableRawMode();

    while (1){ // infinite loop, termination condition (== 'q') is inside 
        char c = '\0'; // for every iteration, set c to null (0 in ascii) for consistency - if read() timesout bc of no input then 0 is displayed
        
        // read() and STDIN_FILENO come from unistd.h and the latter is 0 (constant for standard input file stream)
        // read is defined as follows: read(int file_des, void* buffer, size_t n);
            // read will pull n bytes from the open file descriptor file_des and store into buffer
        // so while is true as long as standard input (terminal for us) has chars to process
        // read() returns 0 when at end-of-file (EOF) which is done in terminal with ctrl-D (or just ctrl-C to force terminate)
        if (read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN){ die("read() failed"); }
        // the second condtion is strictly there to work with Cygwin
        // on timeout (not failure) Cygwin causes read() to read() return -1 AND set errno to EAGAIN
        // everyone else returns 0 for timeout - so we account for EAGAIN to NOT be an error

        // below will print every char inputted into terminal in ASCII codes
        if (iscntrl(c)){ printf("%d\r\n", c); }
        else{ printf("%d ('%c')\r\n", c, c); }
        // iscntrl() from ctype.h and tests if a character is a control character (up-arrow, enter, etc) - NOT to be printed
        // so if control char, just print the ASCII code for it
        // if not, print out the code AND the symbol (a, b, 6, etc)
        // we need \r\n instead of just \n because we turned off OPOST in enableRawMode

        if (c == 'q'){ break; } // termination condition - bc ICANON is off, program quits the instant 'q' is entered to terminal
    }
    // terminal defaults to canonical/cooked mode so input is sent to program only after enter is pressed
    // we want to process EACH key press so we need RAW mode which requires turning off many flags
    // the second condtion shows how cooked mode works - typing "hello" and enter into terminal
    // will process each char without terminating (no 'q')
    // then typing "bye, quincy" and enter will process each char and terminate at 'q', not processing "uincy"

    atexit(disableRawMode); // from stdlib.h and called at termination as long as it is in main() or used in a function called by main()
    return 0;
}

// The ECHO feature causes each key you type to be printed to the terminal, so you can see what you’re typing. 
// useful in canonical mode, but gets in the way when trying to render a user interface in raw mode. 
// So we turn ECHO off - so terminal doesn’t print what you are typing. 
// same as when typing a password into terminal, using sudo for example.
