#include "logger.h"
#include <time.h>
#include <sys/time.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

char* fname;

void configure(char* logfile)
{
    fname = logfile;
}

void writelog(char* prompt)
{
    FILE* file = fopen(fname, "a");
    file = file ? file : stdout;
    if(file)
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);

        time_t t = time((int64_t*)&tv);
        char* timestamp = ctime(&t);
        timestamp[strlen(timestamp) - 1] = (timestamp[strlen(timestamp) - 1] == '\n') ? '\0' : timestamp[strlen(timestamp) - 1];
        fprintf(file, "[%s] %s\n", timestamp, prompt);
    }
    fclose(file);
}
