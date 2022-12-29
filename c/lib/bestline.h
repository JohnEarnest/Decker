#pragma once

typedef struct bestlineCompletions {
  unsigned long len;
  char **cvec;
} bestlineCompletions;

typedef void(bestlineCompletionCallback)(const char *, bestlineCompletions *);
typedef char *(bestlineHintsCallback)(const char *, const char **,
                                       const char **);
typedef void(bestlineFreeHintsCallback)(void *);
typedef unsigned(bestlineXlatCallback)(unsigned);

void bestlineSetCompletionCallback(bestlineCompletionCallback *);
void bestlineSetHintsCallback(bestlineHintsCallback *);
void bestlineSetFreeHintsCallback(bestlineFreeHintsCallback *);
void bestlineAddCompletion(bestlineCompletions *, const char *);
void bestlineSetXlatCallback(bestlineXlatCallback *);

char *bestline(const char *);
char *bestlineRaw(const char *, int, int);
char *bestlineWithHistory(const char *, const char *);
int bestlineHistoryAdd(const char *);
int bestlineHistorySave(const char *);
int bestlineHistoryLoad(const char *);
void bestlineFreeCompletions(bestlineCompletions *);
void bestlineHistoryFree(void);
void bestlineClearScreen(int);
void bestlineMaskModeEnable(void);
void bestlineMaskModeDisable(void);
void bestlineDisableRawMode(void);
void bestlineFree(void *);

char bestlineIsSeparator(unsigned);
char bestlineNotSeparator(unsigned);
char bestlineIsXeparator(unsigned);
unsigned bestlineUppercase(unsigned);
unsigned bestlineLowercase(unsigned);
long bestlineReadCharacter(int, char *, unsigned long);
