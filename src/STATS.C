/*

     / / / / / / / / / /
   / /  stats.c  / / /
 / / / / / / / / / /

         2026
        mpower

  player statistics
     for Wordell

*/

#include <stdlib.h> /* atol */
#include <stdio.h>  /* FILE, fopen, fclose, fgetc, fprintf, sprintf */
#include <string.h> /* strlen, strncmp */

#include "DOSCON.H"
#include "STATS.H"

/* DOS statistics filename - this is
** created in the current program directory
*/
#define STATS_FILE "WORDLE.INI"

/* number of possible winning guess levels
*/
#define STATS_WIN_LEVELS 6

/* fixed screen and work-buffer sizes
*/
#define STATS_SCREEN_WIDTH 80
#define STATS_LINE_SIZE    48

/* all persistent statistics are kept as counters
** percentages are calculated only for display
*/
struct PlayerStats {
    unsigned long gamesPlayed;
    unsigned long won[STATS_WIN_LEVELS];
    unsigned long gamesLost;
};

/* one shared statistics record owned by this module
*/
static struct PlayerStats playerStats;

/* ===================
** function prototypes
** ===================
*/
static void statsClear (void);
static int  statsSave   (void);
static int  statsReadLine (FILE *, char *, unsigned int);
static int  statsApplyLine (char *);
static unsigned long statsWins (void);
static unsigned long statsPercent (unsigned long, unsigned long);
static void  statsPrintCentered (int, char *);
static void  statsDrawPage (void);
static void  statsResetPage (void);

/* ==========
** statsClear
** ==========
** resets every counter in memory
*/
static void statsClear () {

    unsigned char i;

    playerStats.gamesPlayed = 0;
    playerStats.gamesLost = 0;

    i = 0;
    while (i < STATS_WIN_LEVELS) {
        playerStats.won[i] = 0;
        ++i;
    }
}

/* =========
** statsSave
** =========
** writes all counters to WORDLE.INI
*/
static int statsSave () {

    FILE *fp;
    unsigned char i;

    /* DOS text mode writes newline as CR/LF
    */
    fp = fopen(STATS_FILE, "w");
    if (fp == NULL) {
        return 0;
    }

    fprintf(fp, "GAMES=%lu\n", playerStats.gamesPlayed);

    i = 0;
    while (i < STATS_WIN_LEVELS) {
        fprintf(
            fp,
            "WIN%u=%lu\n",
            (unsigned int)(i + 1),
            playerStats.won[i]
        );
        ++i;
    }

    fprintf(fp, "LOST=%lu\n", playerStats.gamesLost);

    if (fclose(fp) == EOF) {
        return 0;
    }

    return 1;
}

/* =============
** statsReadLine
** =============
** reads one CR, LF, or CRLF terminated line
*/
static int statsReadLine (fp, buffer, max)
    FILE *fp;
    char *buffer;
    unsigned int max;
{

    int ch;
    unsigned int len;

    len = 0;

    while (1) {
        ch = fgetc(fp);

        if (ch == EOF) {
            break;
        }

        if (ch == '\r' || ch == '\n') {
            /* ignore empty separators caused by
            ** CRLF files copied from Windows
            */
            if (len == 0) {
                continue;
            }
            break;
        }

        if (len + 1 < max) {
            buffer[len] = (char)ch;
            ++len;
        }
    }

    buffer[len] = 0;

    if (ch == EOF && len == 0) {
        return 0;
    }

    return 1;
}

/* ==============
** statsApplyLine
** ==============
** applies one KEY=VALUE line to memory
*/
static int statsApplyLine (line)
    char *line;
{

    unsigned char i;
    unsigned long value;

    if (strncmp(line, "GAMES=", 6) == 0) {
        playerStats.gamesPlayed = (unsigned long)atol(line + 6);
        return 1;
    }

    if (strncmp(line, "LOST=", 5) == 0) {
        playerStats.gamesLost = (unsigned long)atol(line + 5);
        return 1;
    }

    i = 0;
    while (i < STATS_WIN_LEVELS) {
        if (
            line[0] == 'W' &&
            line[1] == 'I' &&
            line[2] == 'N' &&
            line[3] == (char)('1' + i) &&
            line[4] == '=') {

            value = (unsigned long)atol(line + 5);
            playerStats.won[i] = value;
            return 1;
        }
        ++i;
    }

    return 0;
}

/* =========
** statsInit
** =========
** loads WORDLE.INI or creates a new text file
** returns file save status when a write is needed
*/
int statsInit () {

    FILE *fp;
    char line[STATS_LINE_SIZE];
    unsigned char found;

    statsClear();
    found = 0;

    fp = fopen(STATS_FILE, "r");

    /* a missing file starts with all counters at zero
    ** and is immediately created as DOS text
    */
    if (fp == NULL) {
        return statsSave();
    }

    while (statsReadLine(fp, line, sizeof(line))) {
        if (statsApplyLine(line)) {
            ++found;
        }
    }

    fclose(fp);

    /* an empty or unrecognized file is repaired by
    ** replacing it with a clean zeroed statistics file
    */
    if (found == 0) {
        statsClear();
        return statsSave();
    }

    return 1;
}

/* ==============
** statsRecordWin
** ==============
** records one completed game at its guess level
** returns WORDLE.INI save status
*/
int statsRecordWin (guesses)
    int guesses;
{

    if (guesses < 1 || guesses > STATS_WIN_LEVELS) {
        return 1;
    }

    ++playerStats.gamesPlayed;
    ++playerStats.won[guesses - 1];
    return statsSave();
}

/* ===============
** statsRecordLoss
** ===============
** records one completed six-guess loss
** returns WORDLE.INI save status
*/
int statsRecordLoss () {

    ++playerStats.gamesPlayed;
    ++playerStats.gamesLost;
    return statsSave();
}

/* =========
** statsWins
** =========
** returns the combined number of won games
*/
static unsigned long statsWins () {

    unsigned char i;
    unsigned long total;

    i = 0;
    total = 0;

    while (i < STATS_WIN_LEVELS) {
        total += playerStats.won[i];
        ++i;
    }

    return total;
}

/* ============
** statsPercent
** ============
** calculates one whole-number percentage
*/
static unsigned long statsPercent (value, total)
    unsigned long value;
    unsigned long total;
{

    if (total == 0) {
        return 0;
    }

    return (value * 100L) / total;
}

/* ==================
** statsPrintCentered
** ==================
** prints one line centered in 80 columns
*/
static void statsPrintCentered (row, text)
    int row;
    char *text;
{

    unsigned int len;
    unsigned char column;

    if (text == NULL) {
        return;
    }

    len = strlen(text);

    if (len >= STATS_SCREEN_WIDTH) {
        column = 0;
    } else {
        column = (unsigned char)((STATS_SCREEN_WIDTH - len) / 2);
    }

    gotoxy(column, row);

    while (*text) {
        cputc(*text);
        ++text;
    }
}

/* =============
** statsDrawPage
** =============
** draws the complete single-screen statistics page
*/
static void statsDrawPage () {

    char line[STATS_LINE_SIZE];
    unsigned char i;
    unsigned long wins;

    wins = statsWins();

    clrscr();

    statsPrintCentered(1, "PLAYER STATS");
    statsPrintCentered(2, "------------");

    sprintf(line, "Games played: %lu", playerStats.gamesPlayed);
    statsPrintCentered(4, line);

    sprintf(
        line,
        "Games won: %lu (%lu%%)",
        wins,
        statsPercent(wins, playerStats.gamesPlayed)
    );
    statsPrintCentered(5, line);

    sprintf(
        line,
        "Games lost: %lu (%lu%%)",
        playerStats.gamesLost,
        statsPercent(playerStats.gamesLost, playerStats.gamesPlayed)
    );
    statsPrintCentered(6, line);

    statsPrintCentered(8, "Wins by guess level");

    i = 0;
    while (i < STATS_WIN_LEVELS) {
        /* reserve four positions for the win count
        ** and keep the percentage in a fixed second column
        */
        sprintf(
            line,
            "Won in %u: %4lu    %3lu%% of wins",
            (unsigned int)(i + 1),
            playerStats.won[i],
            statsPercent(playerStats.won[i], wins)
        );

        statsPrintCentered((unsigned char)(10 + i), line);
        ++i;
    }

    statsPrintCentered(18, "(R)eset statistics");
    statsPrintCentered(23, "Press a key");
}

/* ==============
** statsResetPage
** ==============
** confirms and performs a complete reset
*/
static void statsResetPage () {

    int ch;

    clrscr();

    statsPrintCentered(8, "Reset all player statistics?");
    statsPrintCentered(10, "Press Y to erase or any other key to cancel");

    ch = cgetc();

    if (ch >= 'a' && ch <= 'z') {
        ch &= ~0x20;
    }

    if (ch == 'Y') {
        statsClear();

        if (!statsSave()) {
            clrscr();
            statsPrintCentered(10, "Error saving WORDLE.INI");
            statsPrintCentered(12, "Press a key");
            cgetc();
        }
    }
}

/* =========
** statsPage
** =========
** shows statistics and handles the reset option
*/
void statsPage () {

    int ch;

    while (1) {
        statsDrawPage();
        ch = cgetc();

        if (ch >= 'a' && ch <= 'z') {
            ch &= ~0x20;
        }

        if (ch == 'R') {
            statsResetPage();
        } else {
            break;
        }
    }

    clrscr();
}
