
#include "txt.h"

#include <math.h>

#ifdef STANDALONE
#include <stdio.h>
#endif

int is_text(const u8 * data, s32 len) {
#ifdef STANDALONE
    printf("Data of length %d.\n");
#endif

    s32 histogram[256] = { 0 };
    for(s32 i = 0; i < len; i++)
        histogram[data[i]]++;
    
    // Text criterions:
    // 1. Shannon entropy is between 4.5 and 5.2.
    // 2. Majority of the document must be uppercase/lowercase numbers.
    // 3. The file has a proper amount of whitespace
    // -----

    // Step 1
    double entropy = 0;
    for(s32 i = 0; i < 256; i++) {
        double p = (double)histogram[i] / len;
        if(p == 0) continue;
        entropy += p * log2(p);
    }
    entropy = -entropy;

#ifdef STANDALONE
    printf("Shannon entropy: %lf\n", entropy);
#endif

    if(entropy > 5.4 || entropy < 4.5)
        return 0;
    
    // Step 2
    s32 letters = 0;
    s32 whitespace = 0;
    for(s32 i = 0; i < 256; i++) {
        if(i >= 'A' && i <= 'Z')
            letters += histogram[i];
        else if(i >= 'a' && i <= 'z')
            letters += histogram[i];
        else if(i >= '0' && i <= '9')
            letters += histogram[i];
        else if(i == ' ' || i == '\t' || i == '\n' || i == '\r' || i == '\v')
            whitespace += histogram[i];
    }

#ifdef STANDALONE
    printf("Letters: %d, whitespace: %d, text to other ratio: %lf\n", letters, whitespace, (double)(letters+whitespace) / len);
#endif

    if((double)(letters+whitespace) / len < 0.6)
        return 0;

    // Step 3
    double letters_ratio = (double)letters / whitespace;
    if(letters_ratio < 2 || letters_ratio > 9)
        return 0;

#ifdef STANDALONE
    printf("Letter to whitespace ratio: %lf\n", letters_ratio);
#endif

    return 1;
}

#ifdef STANDALONE
int main(int argc, char * argv[]) {
    FILE * f = fopen(argv[1], "rb");
    fseek(f, 0, SEEK_END);
    s32 len = ftell(f);
    fseek(f, 0, SEEK_SET);
    u8 * data = malloc(len);
    fread(data, 1, len, f);
    fclose(f);
    printf("%d\n", is_text(data, len));
}
#endif
