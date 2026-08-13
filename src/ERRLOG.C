/*
    / / / / / / / / /
  / / ERRLOG.C  / /
/ / / / / / / / /

DOS build-output log


before building the main project (BUILD.bat)
first build:
MAKEERR.BAT

once you have the ERRLOG.exe you can easily capture
build errors in a text file when building the main
project:

ERRLOG BUILD.LOG CL /AS /Os WORDELL.C DOSCON.C STATS.C

so just run
MAKEERR.BAT once

note: i've included ERRLOG.exe
here for ease of use

then
BUILD.bat as needed

*/

#include <stdio.h>
#include <io.h>
#include <process.h>

/* =======
** showLog
** =======
** displays completed log file on stdout
*/
static void showLog (fileName)
char *fileName;
{
    FILE *fp;
    int ch;

    fp = fopen(fileName, "r");

    if (fp == NULL) {
        return;
    }

    while ((ch = fgetc(fp)) != EOF) {
        fputc(ch, stdout);
    }

    fclose(fp);
}

/* =============
** program entry
** =============
*/
int main (argc, argv)
int argc;
char **argv;
{
    FILE *logFile;
    int saveOut;
    int saveErr;
    int result;

    if (argc < 3) {
        printf("Usage: ERRLOG logfile program [arguments]\n");
        return 1;
    }

    /* save normal console handles
    */
    saveOut = dup(1);
    saveErr = dup(2);

    if (saveOut < 0 || saveErr < 0) {
        printf("ERRLOG: cannot save DOS output handles\n");
        return 1;
    }

    /* create or replace the log
    */
    logFile = fopen(argv[1], "w");

    if (logFile == NULL) {
        close(saveOut);
        close(saveErr);
        printf("ERRLOG: cannot create %s\n", argv[1]);
        return 1;
    }

    fflush(stdout);
    fflush(stderr);

    /* redirect stdout and stderr to log
    */
    if (dup2(fileno(logFile), 1) < 0) {
        fclose(logFile);
        dup2(saveOut, 1);
        dup2(saveErr, 2);
        close(saveOut);
        close(saveErr);
        printf("ERRLOG: cannot redirect stdout\n");
        return 1;
    }

    if (dup2(fileno(logFile), 2) < 0) {
        dup2(saveOut, 1);
        fclose(logFile);
        dup2(saveErr, 2);
        close(saveOut);
        close(saveErr);
        printf("ERRLOG: cannot redirect stderr\n");
        return 1;
    }

    /* run requested program and wait
    */
    result = spawnvp(P_WAIT, argv[2], &argv[2]);

    if (result < 0) {
        fprintf(stderr, "ERRLOG: cannot run %s\n", argv[2]);
    }

    fflush(stdout);
    fflush(stderr);

    /* restore console handles
    */
    dup2(saveOut, 1);
    dup2(saveErr, 2);

    close(saveOut);
    close(saveErr);
    fclose(logFile);

    /* show captured output
    */
    showLog(argv[1]);

    if (result < 0) {
        return 1;
    }

    return result;
}
