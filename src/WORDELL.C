/*

     / / / / / / / / / /
   / /  wordell.c  / /
 / / / / / / / / / /

         2026
        mpower

     MS-DOS port
   Microsoft C 4.0

 build with BUILD.BAT

*/


#include <stdlib.h> /* abort, srand, rand */
#include <stdio.h>  /* fprintf, stderr, printf, puts, putchar, SEEK_SET */
#include <string.h> /* strcmp, strlen, strcpy */
#include <fcntl.h>  /* O_RDONLY, O_BINARY */
#include <io.h>     /* open, close, lseek, read */
#include <conio.h>  /* kbhit */

#include "DOSCON.H"
#include "STATS.H"

/* comment this out for normal game play
** it's used for testing positioning
*/
/* #define DEBUG */

/* length of a word - you cannot change this
** without  changing the word list too . . .
** you also must rebuild the packed word files
** for the new length
*/
#define WORD_LENGTH 5

/* max tries allowed to find the word
*/
#define MAX_TRIES 6

/* keyboard values used by the input routines
*/
#define KEY_ESCAPE   27
#define INPUT_ESCAPE -2

/* fixed gameplay screen positions
** each accepted guess uses two rows
*/
#define GUESS_X           18
#define HINT_X            27
#define FIRST_GUESS_Y      9
#define GUESS_ROW_STEP     2
#define GUESS_CLEAR_WIDTH 60

/* Number of characters in the alphabet + 1
** if set, the word and a few stats are
** printed before the game starts
*/
#define ALPHA_SIZE 27

/* packed word filenames - put these files
** in the same directory as WORDELL.EXE
*/
#define ALL_FILE "ALL5.BIN"
#define SOL_FILE "SOL5.BIN"

/* generated fixed-record word data
*/
#include "WORDDATA.H"

/* cheap error termination script
*/
#define err(x) fprintf \
(stderr, "\n[%s:%i] Fatal error: %s\n", __FILE__, __LINE__, x);\
abort();

/* cheap boolean
*/
#define bool int
#define false (0)
#define true (!false)

/* low-level file handles for the packed word files
*/
int fdAll;
int fdSol;

/* number of words in the solution list
*/
unsigned int wordCount = SOLUTION_COUNT;

/* selected word from solution list
*/
char word[WORD_LENGTH + 1] = {0};

/* possible characters (used to show unused characters)
** the size specifier is necessary or its value will be
** read only
*/
char alpha[ALPHA_SIZE] = "abcdefghijklmnopqrstuvwxyz";

/* validation buffer -
** only one starting-letter
** group is loaded at a time
*/
char groupBuf   [GROUP_BUF_SIZE];
unsigned int    groupWords = 0;
int loadedGroup = -1;

/* ===================
** function prototypes
** ===================
*/
void initScreen (void);                 /* sets 80-column text mode */
void showTitle  (void);                 /* shows centered title */
void showTitle2 (void);                 /* shows game title at top */
void showTitle3 (void);                 /* shows help title at top */
void waitKey    (void);                 /* waits for a keypress */

int  readLine (char *, int);            /* reads one keyboard line */

bool readExact (                        /* fixed-position read */
    int,
    unsigned long,
    char *,
    unsigned int
);

int  randomGame (unsigned int);         /* keyboard-timed random game */
int  pickWord (char *, int);            /* pick the given word */

bool hasWord    (char *);               /* is the word in the list? */
bool findWord   (char *);               /* C search of loaded group */
int  toLower    (char *);               /* convert to lowercase */
int  toUpper    (char *);               /* convert to uppercase */
bool chkWord    (char *);               /* checks input against solution */
bool isWord     (char *);               /* is user input a valid word? */

int  strpos (char *, int);              /* position of char in string */
void remAlpha   (char *);               /* removes used letters */
bool gameLoop   (void);                 /* runs the main game loop */
int  menu       (void);                 /* runs the menu */
void help       (void);                 /* shows the help text */

void printAt (int, int, char *);
void printAtCurrent (int, char *);
void charAt (int, int, int);
void clearSpan (int, int, int);

/* ====================================
** printAt prints a  C string   x: 0-79
** at a fixed screen location   y: 0-23
** ====================================
*/
void printAt (x, y, text)
    int x;
    int y;
    char *text;
{

    /* protect against a bad string pointer
    */
    if (text == NULL) {
        return;
    }

    /* move the cursor to the
    ** requested screen pos
    */
    gotoxy(x, y);

    /* print the string one char at a time to
    ** hopefully avoid screen memory problems
    */
    while (*text) {
        cputc(*text);
        ++text;
    }
}

/* ==========================================
** printAtCurrent prints a C string  x: 0-79
** on the cursor's current screen row
** ==========================================
*/
void printAtCurrent (x, text)
    int x;
    char *text;
{

    /* protect against a bad string pointer
    */
    if (text == NULL) {
        return;
    }

    /* move horizontally but leave the
    ** current screen row unchanged
    */
    gotox(x);

    /* print the string one char at a time
    ** in the same manner as printAt
    */
    while (*text) {
        cputc(*text);
        ++text;
    }
}

/* ====================================
** charAt prints one character  x: 0-79
** at a fixed screen location   y: 0-23
** ====================================
*/
void charAt (x, y, ch)
    int x;
    int y;
    int ch;
{

    /* move the cursor to the
    ** requested screen pos
    */
    gotoxy(x, y);

    /* print one char via conio
    */
    cputc(ch);
}

/* =======================================
** clearSpan erases part of one screen row
** without printing a newline or scrolling
** =======================================
*/
void clearSpan (x, y, width)
    int x;
    int y;
    int width;
{

    gotoxy(x, y);

    while (width > 0) {
        cputc(' ');
        --width;
    }
}

/* =============
** program entry
** =============
*/
int main() {
    int gameId;

    initScreen();

    /* search for the data files
    ** in the current directory
    */
    fdAll = open(ALL_FILE, O_RDONLY | O_BINARY);
    if (fdAll < 0) {
        err("error opening ALL5.BIN");
    }

    fdSol = open(SOL_FILE, O_RDONLY | O_BINARY);
    if (fdSol < 0) {
        close(fdAll);
        err("error opening SOL5.BIN");
    }

    /* load player statistics or create
    ** WORDLE.INI when it does not exist
    */
    if (!statsInit()) {
        clrscr();
        printAt(28, 11, "Error saving WORDLE.INI");
        waitKey();
    }

    /*#ifdef DEBUG
    printf("Word count: %u\n", wordCount);
    waitKey();
    #endif*/

    while (true) {
        gameId = menu();
        if (gameId < 0) {
            break;
        }

        if (pickWord(word, gameId) < 0) {
            close(fdAll);
            close(fdSol);
            err("error reading solution word");
        }

        /* shows the answer for testing
        */
        #ifdef DEBUG
            printf("Word: %s\n", word);
            waitKey();
        #endif

        /* printf("Running game #%i\n", gameId + 1); */

        /* ESC during a guess returns to the menu.
        ** A completed game also returns to the menu
        ** after the final keypress.
        */
        if (!gameLoop()) {
            break;
        }
    }

    /* cleanup
    */
    close(fdAll);
    close(fdSol);
    return 0;
}

/* ==========
** initScreen
** ==========
*/
void initScreen() {

    /* keep an existing 80-column text mode
    ** otherwise switch to BIOS mode 3
    */
    dosVideoInit();
    clrscr();
}

/* =========
** showTitle
** =========
*/
void showTitle() {
    printAt(27, 6, "\\    / _  ._  _|  _  | |");
    printAt(28, 7, "\\/\\/ (_) |  (_| (/_ | |");
}

/* ==========
** showTitle2
** ==========
*/
void showTitle2() {
    printAt(31, 0,  "|  | _  _ _| _ | |");
    printAt(31, 1, "|/\\|(_)| (_|(/_| |");
}

/* ==========
** showTitle3
** ==========
*/
void showTitle3() {
    printAt(31, 1,  "|  | _  _ _| _ | |");
    printAt(31, 2, "|/\\|(_)| (_|(/_| |");
}

/* =======
** waitKey
** =======
*/
void waitKey() {
    puts("");

    printAtCurrent (
        34,
        "Press any key"
    );
    cgetc();
}

/* ========
** readLine
** ========
*/
int readLine(buffer, max)
    char *buffer;
    int max;
{

    /* for current length of typed text
    */
    int len;

    /* for key read from keyboard
    */
    int ch;

    /* true if user typed past buffer size
    */
    int overflow;

    /* start with an empty line
    */
    len = 0;
    overflow = false;

    /* keep reading keys until RETURN
    */
    while (true) {

    /* get one key from DOS keyboard
    */
    ch = cgetc();

    /* ESC cancels the current word entry
    ** and reports it to the game loop
    */
    if (ch == KEY_ESCAPE) {
        buffer[0] = 0;
        return INPUT_ESCAPE;
    }

    /* RETURN ends the input line
    ** accepts either CR or LF
    */
    if (ch == '\r' || ch == '\n') {

    /* do not print a newline here
    ** gameLoop owns all cursor movement
    */

    /* terminate C string
    */
    buffer[len] = 0;

    /* report overflow to caller
    */
    if (overflow) {
        return -1;
    }

    /* return number of chars typed
    */
        return len;
    }

    /* handle backspace/delete
    */
    if (ch == 8 || ch == 127) {

        if (len > 0) {
            --len;

            /* erase character on screen
            */
            printf("\b \b");
        }
        } else if (ch >= ' ' && ch <= '~') {

            /* accept normal printable ASCII
            */
            if (len < max - 1) {
                buffer[len] = (char)ch;
                ++len;

                /* echo typed character
                */
                putchar(ch);
            } else {

            /* remember that input was too long
            */
                overflow = true;
            }
        } /* end else if */
    } /* end while */
} /* end readLine() */

/* =========
** readExact
** =========
*/
bool readExact(fd, offset, buffer, len)
    int fd;
    unsigned long offset;
    char *buffer;
    unsigned int len;
{

    /* number of bytes read by one call
    */
    int got;

    /* move to exact byte position in file
    */
    if (lseek(fd, offset, SEEK_SET) < 0) {
        return false;
    }

    /* keep reading until requested
    ** number of bytes is loaded
    */
    while (len > 0) {

    /* read next block into buffer
    */
    got = read(fd, buffer, len);
    if (got <= 0) {
        return false;
    }

    /* advance buffer pointer by the
    ** number of bytes just read
    */
    buffer += got;

    /* reduce number of bytes left
    */
        len -= got;
    }
    return true;
}

/* ==========
** randomGame
** ==========
*/
int randomGame(seed)
    unsigned int seed;
{

    /* protect against an empty solution list
    */
    if (wordCount == 0) {
        return -1;
    }

    /* initialize the C library random generator
    ** from the timed S keypress
    */
    srand(seed);

    /* return a valid solution index
    */
    return (int)(rand() % wordCount);
}

/* ====
** menu
** ====
*/
int menu() {
    int ch;

    /* timed seed while waiting for a menu key
    */
    unsigned int seed;

    while (true) {
        clrscr();
        showTitle();

        printAt(32, 10, "(S)tart new game");
        printAt(32, 11, "(H)ow to play");
        printAt(32, 12, "(P)layer stats");
        printAt(32, 14, "(Q)uit Wordell");

        /* time how long the player waits
        ** before pressing a menu key
        */
        seed = 1;
        while (!kbhit()) {
            ++seed;
        }
        ch = cgetc();

        /* convert lowercase to uppercase
        ** without needing a string buffer
        */
        if (ch >= 'a' && ch <= 'z') {
            ch &= ~0x20;
        }

        /* handle the menu choice
        */
        if (ch == 'S') {
            return randomGame(seed);
        } else if (ch == 'H') {
            help();
        } else if (ch == 'P') {
            statsPage();
        } else if (ch == 'Q' || ch == KEY_ESCAPE) {
            return -1;
        }
        /* any other key simply redraws
        ** the menu and waits again
        ** this also helps if the screen
        ** gets corrupted, just refresh
        */
    }
}

void help() {
    clrscr();
    showTitle3();
    printf (
        "\n"
    );
    printf (
        "\n                     "
    );
    printf (
        "Guess the %i letter word within %i tries\n",
        WORD_LENGTH,
        MAX_TRIES
    );
    printAt (
        13,
        6,
        "After every guess, hints are shown for each character"
    );
    printAt (
        20,
        8,  "# = Character found and position correct"
    );
    printAt (
        20,
        9,  "o = Character found but position wrong"
    );
    printAt (
        20,
        10,  "_ = Character not found at all"
    );
    printAt (
        12,
        12, "Unused letters of the alphabet are shown next to the hint"
    );
    printAt (
        16,
        14, "Guessing RATES when the word is TESTS shows __oo#"
    );
    printAt (
        20,
        16, "[ESC] during game play quits that round"
    );
    printAt (
        20,
        18, "Player stats are recorded at the end of"
    );
    printAt (
        23,
        19, "each round. Quits are not recorded.\n"
    );


/*
    puts("");
    printf("Guess the %i letter word within %i tries\n",
           WORD_LENGTH, MAX_TRIES);
    puts("");
    puts("After every guess, hints are shown for each character.");
    puts("They look like this:");
    puts("  _ = Character not found at all");
    puts("  # = Character found and position correct");
    puts("  o = Character found but position wrong");
    puts("");
    puts("Unused letters of the alphabet are shown next to the hint.");
    puts("");
    puts("Guessing RATES when the word is TESTS shows __oo#");*/
    waitKey();
}

bool gameLoop() {
    char          guess[WORD_LENGTH + 1];
    char          statusText[48];
    int           guesses;
    int           inputLen;
    int           statsOK;
    unsigned char guessRow;
    unsigned char hintRow;

    guess[0] = 0;
    guesses = 0;
    inputLen = 0;
    statsOK = true;

    /* a game abandoned with ESC may already have
    ** removed letters from the shared alphabet
    */
    strcpy(alpha, "abcdefghijklmnopqrstuvwxyz");

    clrscr();
    showTitle2();

    /* fixed-position instructions leave the lower
    ** twelve rows available for all six guesses
    */
    printAt(27, 3, "# correct place");
    printAt(27, 4, "o correct letter/wrong place");
    printAt(27, 5, "_ not in word");
    printAt(27, 7, "Hint     Unused alphabet <ESC> quits");
    printAt(27, 8, "----     ---------------------------");

    while (guesses < MAX_TRIES && strcmp(guess, word)) {
        guessRow = FIRST_GUESS_Y + (guesses * GUESS_ROW_STEP);
        hintRow = guessRow + 1;

        /* always repaint the current attempt in its
        ** fixed two-row slot - invalid guesses reuse it
        */
        clearSpan(GUESS_X, guessRow, GUESS_CLEAR_WIDTH);
        clearSpan(GUESS_X, hintRow, GUESS_CLEAR_WIDTH);

        sprintf(statusText, "Guess %i: ", guesses + 1);
        printAt(GUESS_X, guessRow, statusText);
        gotoxy(
            (unsigned char)(GUESS_X + strlen(statusText)),
            guessRow
        );

        inputLen = readLine(guess, sizeof(guess));

        /* first ESC clears the game screen
        ** and returns to the main menu
        */
        if (inputLen == INPUT_ESCAPE) {
            clrscr();
            return true;
        }

        toLower(guess);

        if (inputLen == WORD_LENGTH) {

            /* skip the test logic if the word was found
            */
            if (strcmp(guess, word)) {
                if (isWord(guess) && hasWord(guess)) {
                    ++guesses;

                    /* the hint and alphabet occupy the
                    ** second row of this accepted guess
                    */
                    gotoxy(HINT_X, hintRow);

                    if (chkWord(guess)) {
                        remAlpha(guess);
                        printf("     %s", alpha);
                    }
                } else {
                    printAt(
                        20,
                        hintRow,
                        "Word not in list - press a key"
                    );
                    cgetc();

                    /* erase only the rejected attempt
                    ** then let the same guess number retry
                    */
                    clearSpan(
                        GUESS_X,
                        guessRow,
                        GUESS_CLEAR_WIDTH
                    );
                    clearSpan(
                        GUESS_X,
                        hintRow,
                        GUESS_CLEAR_WIDTH
                    );
                    guess[0] = 0;
                }
            }
        } else {
            sprintf(
                statusText,
                "Must be %i characters - press a key",
                WORD_LENGTH
            );

            printAt(20, hintRow, statusText);
            cgetc();

            /* wrong-length input also reuses
            ** the current fixed guess slot
            */
            clearSpan(GUESS_X, guessRow, GUESS_CLEAR_WIDTH);
            clearSpan(GUESS_X, hintRow, GUESS_CLEAR_WIDTH);
            guess[0] = 0;
        }
    }

    /* final messages use reserved rows so neither a
    ** win nor a loss can scroll the DOS screen
    */
    /* broken: clearSpan(18, 10, GUESS_CLEAR_WIDTH); */

    if (strcmp(guess, word)) {
        statsOK = statsRecordLoss();
        toUpper(word);
        sprintf(statusText, "Better luck next time! The word was %s", word);
        printAt(22, 22, statusText);
    } else {
        statsOK = statsRecordWin((unsigned char)(guesses + 1));
        toUpper(word);

        sprintf(
            statusText,
            "You win! The word is: %s",
            word
        );

        printAt(
            (unsigned char)((80 - strlen(statusText)) / 2),
            21,
            statusText
        );
    }

    waitKey();

    /* report a failed statistics write only after
    ** the completed game result has been shown
    */
    if (!statsOK) {
        clrscr();
        printAt(28, 11, "Error saving WORDLE.INI");
        waitKey();
    }

    /* printAt(35, 23, "Press any key"); */
    /* cgetc(); */
    clrscr();
    return true;
}

/* ========
** remAlpha
** ========
** removes guessed letters from the visible list
** any character found in guess is replaced with '_'
** lets the player see which letters have been used
*/
void remAlpha(guess)
    char *guess;
{

    /* index into guess
    */
    int i;

    /* position of guessed character inside alpha
    */
    int pos;

    /* start at first guessed character
    */
    i = 0;
    pos = 0;

    /* only process a valid string
    */
    if (guess != NULL) {

    /* walk through every character in the guessed word
    */
        while (guess[i]) {

        /* find this guessed character
        ** in the remaining alphabet
        */
            pos = strpos(alpha, guess[i]);

        /* if found, mark it as used
        */
            if (pos >= 0) {
                alpha[pos] = '_';
            }

        /* move to next guessed character
        */
            ++i;
        }
    }
}

int strpos (str, search)
    char *str;
    int search;
{
    int i;
    i = 0;
    if (str != NULL) {
        while (str[i]) {
        if (str[i] == search) {
            return i;
        }
        ++i;
        }
    }

    return -1;
}

bool chkWord(guess)
    char *guess;
{
    int i;
    int pos;
    char copy[WORD_LENGTH + 1];
    char result[WORD_LENGTH + 1];

    if (strlen(guess) == strlen(word)) {
        i = 0;
        pos = -1;
        result[WORD_LENGTH] = 0;
        strcpy(copy, word);

        /* do all correct positions first
        */
        while (copy[i]) {
            if (copy[i] == guess[i]) {

                /* character found and position correct
                */
                result[i] = '#';
                copy[i] = '_';

            } else {

                /* fll remaining slots with blanks
                */
                result[i] = '_';
            }
            ++i;
        }

        i = 0;
        while (copy[i]) {
            pos = strpos(copy, guess[i]);

            /*Char must exist but do not overwrite a good guess
            */
            if (pos >= 0 && result[i] != '#') {
                /*Character found but position wrong
                */
                result[i] = 'o';
                copy[pos] = '_';
            }
            ++i;
        }

        printf("%s", result);
        return true;
    }

    return false;
}

int toUpper(str)
    char *str;
{
    int i;

    /* handle null pointer for safety
    */
    if (str == NULL) {
        return 0;
    }

    i = 0;

    while (str[i]) {
        if (str[i] >= 'a' && str[i] <= 'z') {
            str[i] &= ~0x20; /* Make uppercase */
        }
        ++i;
    }
    /* Return the number of
    ** processed characters
    */
    return i;
}

int toLower(str)
    char *str;
{
    int i;

    i = 0;

    while (str[i]) {
        if (str[i] >= 'A' && str[i] <= 'Z') {
            str[i] |= 0x20; /* Make lowercase */
        }
        ++i;
    }

    return i;
}

/* =======
** hasWord
** =======
** looks for a guess in the packed master word list only
** one first-letter group is kept in memory at a time
*/
bool hasWord(word)
    char *word;
{
    /* zero-based first-letter group
    ** a = 0 through z = 25
    */
    int group;
    /* number of packed word bytes
    ** needed for the selected group
    */
    unsigned int bytes;
    /* don't bother if the argument is invalid
    */
    if (
        word == NULL ||
        strlen(word) != WORD_LENGTH ||
        !isWord(word)) {

        return false;
    }

    /* convert the first letter into its
    ** matching packed-file group number
    */
    group = word[0] - 'a';
    /* reject anything outside the 26
    ** lowercase alphabet groups
    */
    if (group < 0 || group >= 26) {
        return false;
    }

    /* load a new first-letter group only
    ** when it differs from the cached group
    */
    if (loadedGroup != group) {
        /* each packed word occupies exactly
        ** WORD_LENGTH bytes
        */
        bytes = allCount[group] * WORD_LENGTH;
        /* make sure this group fits inside
        ** the fixed validation buffer
        */
        if (bytes > GROUP_BUF_SIZE) {
            err("letter group is too large");
        }

        /* read the complete group from its
        ** recorded position in ALL5.BIN
        */
        if (!readExact(fdAll, allOffset[group], groupBuf, bytes)) {
            return false;
        }

        /* remember how many words are loaded
        ** and which group it is
        */
        groupWords = allCount[group];
        loadedGroup = group;
    }

    /* search the loaded group in C
    */
    return findWord(word);
}

/* ========
** findWord
** ========
** searches the loaded packed group in C
** each record is exactly WORD_LENGTH bytes
*/
bool findWord(query)
    char *query;
{
    unsigned int i;
    char *record;

    record = groupBuf;
    i = 0;

    while (i < groupWords) {
        if (
            record[0] == query[0] &&
            record[1] == query[1] &&
            record[2] == query[2] &&
            record[3] == query[3] &&
            record[4] == query[4]) {

            return true;
        }

        record += WORD_LENGTH;
        ++i;
    }

    return false;
}

/* ======
** isWord
** ======
** verifies that a guess contains exactly WORD_LENGTH
** lowercase alphabetic characters - typical wordle
** usage is 5
*/
bool isWord(word)
    char *word;
{
    /* character position being checked
    */
    int i;

    /* start at -1 because the loop increments
    ** before checking the first character
    */
    i = -1;

    /* continue only when the string has
    ** exactly the required word length
    */
    if (strlen(word) == WORD_LENGTH) {
        /* examine every character in the word
        */
        while (word[++i]) {
            /* only lowercase a through
            ** z are accepted
            */
            if (word[i] < 'a' || word[i] > 'z') {
                return false;
            }
        }
        /* every character passed validation
        */
        return true;
    }

    /* the word had the wrong length
    */
    return false;
}

/* ========
** pickWord
** ========
** reads one fixed-length solution record
*/
int pickWord (word, index)
    char *word;
    int index;
{

    /* byte position of the selected word
    ** inside SOL5.BIN
    */
    unsigned long offset;

    /* word records have no separators
    ** so do it by word length
    */
    offset = ((unsigned long)index) * WORD_LENGTH;

    /* read exactly one packed solution word
    */
    if (!readExact (
        fdSol,
        offset,
        word,
        WORD_LENGTH)) {
        /* leave the caller with an empty
        ** string if the file read fails
        */
        word[0] = 0;
        return -1;
    }

    /* terminate the word as a C string
    */
    word[WORD_LENGTH] = 0;
    /* normalize the selected solution to lowercase
    */
    toLower(word);
    /* return the selected index
    */
    return index;
}
