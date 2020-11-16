#include "logger.h"
#include <time.h>
#include <sys/time.h>
#include <stdint.h>
#include <string.h>

void writelog(FILE* file, char* prompt)
{
    if(file)
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);

        time_t t = time((int64_t*)&tv);
        char* timestamp = ctime(&t);
        timestamp[strlen(timestamp) - 1] = (timestamp[strlen(timestamp) - 1] == '\n') ? '\0' : timestamp[strlen(timestamp) - 1];
        fprintf(file, "[%s] %s\n", timestamp, prompt);
    }
}
