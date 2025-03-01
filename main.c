// Comparison of boolean parsing functions.
// Copyright 2025 Bradley Lin
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files(the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "stopwatch.h"
#include "benchmark.h"

#include <immintrin.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#pragma region Functions

// A kind of inefficient way to parse boolean values.
// Shouldn't appear as bad code otherwise.
int String_parseBoolLoop(const char* string) {
    if (string == NULL) {
        return -1;
    }

    // Stored in order of predicted usage probability.
    static const char const* match[16] = {"true", "yes", "no", "false", "on", "off", "True", "False", "Yes", "No", "enable", "disable", "On", "Off", "Enable", "Disable"};
    static const int const* value[16] = {     1,     1,    0,       0,    1,     0,      1,       0,     1,    0,        1,         0,    1,     0,        1,         0};
    for (int j = 0; j < 16; j++) {
        const char* strMatch = match[j];
        const char* strCheck = string;
        int result = 1;

        // Traverse through entire match string.
        // Potentially going off beyond the end
        // of check string should be ok... I hope.
        while (*strMatch != '\0') {
            if (*(strCheck++) != *(strMatch++)) {
                result = 0;
                break; // Break early so we don't process the rest of the string.
            }
        }
        if (result)
            return value[j];
    }

    return -1;
}

// An example of one of the very few excuses to
// use the "goto" keyword. "If" statement checking
// "result" is not needed anymore saving instructions
// and hence avoiding branch prediction misses.
// Shouldn't appear as bad code otherwise.
int String_parseBoolGoto(const char* string) {
    if (string == NULL) {
        return -1;
    }

    // Stored in order of predicted usage probability.
    static const char const* match[16] = {"true", "yes", "no", "false", "on", "off", "True", "False", "Yes", "No", "enable", "disable", "On", "Off", "Enable", "Disable"};
    static const int const* value[16] = {1,     1,    0,       0,    1,     0,      1,       0,     1,    0,        1,         0,    1,     0,        1,         0};
    for (int j = 0; j < 16; j++) {
        const char* strMatch = match[j];
        const char* strCheck = string;

        // Traverse through entire match string.
        // Potentially going off beyond the end
        // of check string should be ok... I hope.
        while (*strMatch != '\0') {
            if (*(strCheck++) != *(strMatch++)) {
                goto no_match; // Exit early to avoid return and further string processing.
            }
        }
        return value[j];
    no_match: // Placed at end of loop so the iterator j is incremented.
        continue; // Label requires code following it.
    }

    return -1;
}

// Intuitive approach to string comparison. More common keywords
// are closer to the top as earlier matches skip the rest
// of the code, so computation time can vary drastically.
// Most of the time this is good enough.
int String_parseBoolStrcmp(const char* string) {
    if (string == NULL) {
        return -1;
    }

    // Due to the nature of how logical or works, if an earlier
    // expression is true, the later ones do not get evaluated.
    // So a "true" will be much faster than a "Disable".
    if (strcmp(string, "true") == 0 ||
        strcmp(string, "yes") == 0 ||
        strcmp(string, "on") == 0 ||
        strcmp(string, "enable") == 0 ||
        strcmp(string, "True") == 0 ||
        strcmp(string, "Yes") == 0 ||
        strcmp(string, "On") == 0 ||
        strcmp(string, "Enable") == 0)
        return 1;

    else if (strcmp(string, "false") == 0 ||
        strcmp(string, "no") == 0 ||
        strcmp(string, "off") == 0 ||
        strcmp(string, "disable") == 0 ||
        strcmp(string, "False") == 0 ||
        strcmp(string, "No") == 0 ||
        strcmp(string, "Off") == 0 ||
        strcmp(string, "Disable") == 0)
        return 0;


    return -1;
}

// Parses a string that represents a boolean value and returns the according int.
// Uses bitwise hashing math to determine which word to compare based on first letter.
// Hashing expression is currently inefficient enough that its slower than above.
int String_parseBoolHashing(const char* string) {
    // Check if string even points to something.
    if (string == NULL) {
        return -1;
    }
    
    // Check if word matches any true keywords.
    int firstChar = (int)*string;
    static const char* match[8] = {"Enable", "On", "True", "Yes", "enable", "on", "true", "yes"};
    // Use bit-hacking math expression (hash) to determine which keyword to match against.
    if (strcmp(string, match[((firstChar - 'A') >> 3u) & 7u]) == 0)
        return 1;

    // Check if word matches any false keywords.
    static const char* notMatch[8] = {"Disable", "False", "No", "Off", "disable", "false", "no", "off"};
    // Precalculate value that's used 2 times in following hash.
    int capsOffset = (firstChar - 65) >> 5u;
    // Use bit-hacking math expression (hash) to determine which keyword to match against.
    if (strcmp(string, notMatch[((((firstChar - 57 - (capsOffset << 4u)) << 3u) - firstChar + (capsOffset << 5u)) >> 5u) & 7u]) == 0)
        return 0;

    return -1;
}

// Parses a string that represents a boolean value and returns the according int.
// Uses bitwise hashing math to determine which word to compare based on first letter.
// Then uses cursed reinterpret casting to a 64-bit unsigned integer to process 8
// characters at once. A mask is used to clear garbage read from the end of the source
// string. It's ok to read a bit after the memory region of the string as segfaults
// are caused after the end of the page and we assume the source string is memory
// aligned.
int String_parseBoolPacking(const char* string) {
    // Check if string even points to something.
    if (string == NULL) {
        return -1;
    }

    // Check if word matches any true keywords.
    // NEED TO CLEAN END OF STRING BEFORE MATCHING.
    // Get value of first character as an integer.
    int firstChar = (int)*string;
    // Constant expression array of baked "true" strings.
    // Need to be 0 padded so every item is 8 characters long
    // which is the size of uint64_t.
    static const char* match = "Enable\0\0" "On\0\0\0\0\0\0" "True\0\0\0\0" "Yes\0\0\0\0\0" "enable\0\0" "on\0\0\0\0\0\0" "true\0\0\0\0" "yes\0\0\0\0";
    // Hashing algorithm with no collision if match.
    int offset = (firstChar - 'A') & 56u; // 0b00111000
    // Use hash to select string to compare. Perform mask and
    // reinterpret cast 8 character strings as 64-bit unsigned
    // integers to compare 8 characters in one operation.
    if (strcmp(string, match + offset) == 0)
        return 1;

    // Check if word matches any "false" keywords.
    static const char* notMatch = "Disable\0" "False\0\0\0" "No\0\0\0\0\0\0" "Off\0\0\0\0\0" "\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC" "\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC" "\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC" "disable\0" "false\0\0\0" "no\0\0\0\0\0\0" "off\0\0\0\0\0" "\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC" "\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC" "\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC" "\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC" "\xCC\xCC\xCC\xCC\xCC\xCC\xCC";
    // Different hashing algorithm with no collision if match.
    // This hashing algorithm's simplicity relies on a larger
    // padded array of empty items.
    offset = ((((firstChar - 57) << 3u) - firstChar) >> 2u) & 120u;
    // Same as before.
    if (strcmp(string, notMatch + offset) == 0)
        return 0;

    return -1;
}

// Parses a string that represents a boolean value and returns the according int.
// Uses bitwise hashing math to determine which word to compare based on first letter.
// Then uses cursed reinterpret casting to a 64-bit unsigned integer to process 8
// characters at once. A mask is used to clear garbage read from the end of the source
// string. It's ok to read a bit after the memory region of the string as segfaults
// are caused after the end of the page and we assume the source string is memory
// aligned. The hashing math is simplified by packing keywords into a single string and
// for returning 0, offsetting the positions to suit the hashing result.
int String_parseBoolReinterpret(const char* string) {
    // Check if string even points to something.
    if (string == NULL) {
        return -1;
    }

    // Check if word matches any true keywords.
    // NEED TO CLEAN END OF STRING BEFORE MATCHING.
    // Get value of first character as an integer.
    int firstChar = (int)*string;
    // Constant expression array of baked "true" strings.
    // Need to be 0 padded so every item is 8 characters long
    // which is the size of uint64_t.
    static const char* match = "Enable\0\0" "On\0\0\0\0\0\0" "True\0\0\0\0" "Yes\0\0\0\0\0" "enable\0\0" "on\0\0\0\0\0\0" "true\0\0\0\0" "yes\0\0\0\0";
    // Hashing algorithm with no collision if match.
    int offset = (firstChar - 'A') & 56u; // 0b00111000
    // Lookup table of lengths of each match for masking.
    static const int lengths[64] = {8, 0, 0, 0, 0, 0, 0, 0, 40, 0, 0, 0, 0, 0, 0, 0, 24, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 40, 0, 0, 0, 0, 0, 0, 0, 24, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0, 0, 0, 0, 0};
    // Use hash to select string to compare. Perform mask and
    // reinterpret cast 8 character strings as 64-bit unsigned
    // integers to compare 8 characters in one operation.
    if (((*((uint64_t*)string)) & (0xFFFFFFFFFFFFFFFF >> lengths[offset])) == *((uint64_t*)(match + offset)))
        return 1;

    // Check if word matches any "false" keywords.
    static const char* notMatch = "Disable\0" "False\0\0\0" "No\0\0\0\0\0\0" "Off\0\0\0\0\0" "\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC" "\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC" "\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC" "disable\0" "false\0\0\0" "no\0\0\0\0\0\0" "off\0\0\0\0\0" "\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC" "\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC" "\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC" "\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC" "\xCC\xCC\xCC\xCC\xCC\xCC\xCC";
    // Different hashing algorithm with no collision if match.
    // This hashing algorithm's simplicity relies on a larger
    // padded array of empty items.
    offset = ((((firstChar - 57) << 3u) - firstChar) >> 2u) & 120u;
    // Another lookup table of lengths of each match for masking.
    static const int lengths2[128] = {0, 0, 0, 0, 0, 0, 0, 0,    16, 0, 0, 0, 0, 0, 0, 0,    40, 0, 0, 0, 0, 0, 0, 0,    32, 0, 0, 0, 0, 0, 0, 0,    64, 0, 0, 0, 0, 0, 0, 0,    64, 0, 0, 0, 0, 0, 0, 0,    64, 0, 0, 0, 0, 0, 0, 0,    0, 0, 0, 0, 0, 0, 0, 0,    16, 0, 0, 0, 0, 0, 0, 0,    40, 0, 0, 0, 0, 0, 0, 0,    32, 0, 0, 0, 0, 0, 0, 0,    64, 0, 0, 0, 0, 0, 0, 0,    64, 0, 0, 0, 0, 0, 0, 0,    64, 0, 0, 0, 0, 0, 0, 0,    64, 0, 0, 0, 0, 0, 0, 0,    64, 0, 0, 0, 0, 0, 0, 0};
    // Same as before.
    if ((*((uint64_t*)string) & (0xFFFFFFFFFFFFFFFF >> lengths2[offset])) == *((uint64_t*)(notMatch + offset)))
        return 0;

    return -1;
}

// Parses a string that represents a boolean value and returns the according int.
// Uses bitwise hashing math to determine which word to compare based on first letter.
// Then uses cursed reinterpret casting to a 64-bit unsigned integer to process 8
// characters at once. A mask is used to clear garbage read from the end of the source
// string. It's ok to read a bit after the memory region of the string as segfaults
// are caused after the end of the page and we assume the source string is memory
// aligned. The hashing math is simplified by packing keywords into a single string and
// for returning 0, offsetting the positions to suit the hashing result. x86 instrinsic
// bzhi is used for masking whichs saves instructions over manually doing it.
int String_parseBoolReinterpretIntrinsic(const char* string) {
    // Check if string even points to something.
    if (string == NULL) {
        return -1;
    }

    

    // Check if word matches any true keywords.
    // NEED TO CLEAN END OF STRING BEFORE MATCHING.
    // Get value of first character as an integer.
    int firstChar = (int)*string;
    // Constant expression array of baked "true" strings.
    // Need to be 0 padded so every item is 8 characters long
    // which is the size of uint64_t.
    static const char* match = "Enable\0\0" "On\0\0\0\0\0\0" "True\0\0\0\0" "Yes\0\0\0\0\0" "enable\0\0" "on\0\0\0\0\0\0" "true\0\0\0\0" "yes\0\0\0\0";
    // Hashing algorithm with no collision if match.
    int offset = (firstChar - 'A') & 56u; // 0b00111000
    // Lookup table of lengths of each match for masking.
    static const uint32_t lengths[64] = {56, 0, 0, 0, 0, 0, 0, 0, 24, 0, 0, 0, 0, 0, 0, 0, 40, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0, 0, 0, 0, 0, 56, 0, 0, 0, 0, 0, 0, 0, 24, 0, 0, 0, 0, 0, 0, 0, 40, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0, 0, 0, 0, 0};
    
    // Use hash to select string to compare. Perform mask via
    // bzhi instruction and reinterpret cast 8 character strings
    // as 64-bit unsigned integers to compare 8 characters in one
    // operation.
    if (_bzhi_u64(*((uint64_t*)string), lengths[offset]) == *((uint64_t*)(match + offset)))
        return 1;

    // Check if word matches any "false" keywords.
    static const char* notMatch = "Disable\0" "False\0\0\0" "No\0\0\0\0\0\0" "Off\0\0\0\0\0" "\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC" "\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC" "\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC" "disable\0" "false\0\0\0" "no\0\0\0\0\0\0" "off\0\0\0\0\0" "\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC" "\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC" "\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC" "\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC" "\xCC\xCC\xCC\xCC\xCC\xCC\xCC";
    // Different hashing algorithm with no collision if match.
    // This hashing algorithm's simplicity relies on a larger
    // padded array of empty items.
    offset = ((((firstChar - 57) << 3u) - firstChar) >> 2u) & 120u;
    // Another lookup table of lengths of each match for masking.
    static const int lengths2[128] = {64, 0, 0, 0, 0, 0, 0, 0,    48, 0, 0, 0, 0, 0, 0, 0,    24, 0, 0, 0, 0, 0, 0, 0,    32, 0, 0, 0, 0, 0, 0, 0,    0, 0, 0, 0, 0, 0, 0, 0,    0, 0, 0, 0, 0, 0, 0, 0,    0, 0, 0, 0, 0, 0, 0, 0,    64, 0, 0, 0, 0, 0, 0, 0,    48, 0, 0, 0, 0, 0, 0, 0,    24, 0, 0, 0, 0, 0, 0, 0,    32, 0, 0, 0, 0, 0, 0, 0,    0, 0, 0, 0, 0, 0, 0, 0,    0, 0, 0, 0, 0, 0, 0, 0,    0, 0, 0, 0, 0, 0, 0, 0,    0, 0, 0, 0, 0, 0, 0, 0,    0, 0, 0, 0, 0, 0, 0, 0};
    // Same as before.
    if (_bzhi_u64(*((uint64_t*)string), lengths2[offset]) == *((uint64_t*)(notMatch + offset)))
        return 0;

    return -1;
}

#pragma endregion

int main() {
    printf("Parsing Bool Test %d Runs\n\n", BOOL_RUNS);
    get_inputs();
    
    printf("\n");

    run_test(String_parseBoolLoop, "Character Looping Bool Parser");
    run_test(String_parseBoolGoto, "Better Character Looping Bool Parser");
    run_test(String_parseBoolStrcmp, "strcmp Bool Parser");
    run_test(String_parseBoolHashing, "Hash-Based Bool Parser");
    run_test(String_parseBoolPacking, "Packing Hash-Based Bool Parser");
    run_test(String_parseBoolReinterpret, "Reinterpret Hash-Based Bool Parser");
    run_test(String_parseBoolReinterpretIntrinsic, "Intrinsic Reinterpret Hash-Based Bool Parser");

    printf("\nDone!\n");
    return 0;
}
