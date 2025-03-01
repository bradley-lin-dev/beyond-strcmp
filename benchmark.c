// Benchmarking functions for parsers.
// Copyright 2025 Bradley Lin
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files(the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "benchmark.h"
#include "stopwatch.h"
#include <stdio.h>


static char boolStrings[32][32];
static int stringCount = 0;

// Get inputs that will be sequentially used to test parsing functions.
void get_inputs() {
    printf("Enter bool: ");
    
    while (fgets(boolStrings[stringCount], 20, stdin) != NULL) {
        if (boolStrings[stringCount][0] == '\0')
            continue;

        // Escape if empty input is returned.
        if (boolStrings[stringCount][0] == '\n') {
            break;
        }
        boolStrings[stringCount][strlen(boolStrings[stringCount]) - 1] = '\0';
        printf("\rString %d: %s\n", stringCount, boolStrings[stringCount]);
        stringCount++;
        printf("Enter bool: ");
    }
    printf("Got %d inputs!\n", stringCount);
}

// Run test of parser for BOOL_RUNS number of times and display the elapsed time.
void run_test(int (*parser)(const char*), const char * name) {
    volatile char* newString;
    volatile int resultBool;
    int trueCounter = 0;
    int errorCounter = 0;

    double time_before = time_now();
    for (int i = 0; i < BOOL_RUNS; i++) {
        newString = boolStrings[i % stringCount];
        resultBool = parser(newString);
        trueCounter += (resultBool == 1) ? 1u : 0u;
        errorCounter += (resultBool == -1) ? 1u : 0u;
    }
    double time_after = time_now();
    printf("%s: %f, True: %d, Error: %d\n", name, time_after - time_before, trueCounter, errorCounter);
}
