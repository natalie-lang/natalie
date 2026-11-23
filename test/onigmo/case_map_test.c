#include "onigmo.h"
#include <assert.h>
#include <string.h>

/* GHSA-34m4-m9fh-gqp5: sharp-s expands to two bytes. */
int main(void) {
    OnigEncoding encodings[] = {
        ONIG_ENCODING_ISO_8859_1,
        ONIG_ENCODING_ISO_8859_2,
        ONIG_ENCODING_ISO_8859_3,
        ONIG_ENCODING_ISO_8859_4,
        ONIG_ENCODING_ISO_8859_9,
        ONIG_ENCODING_ISO_8859_10,
        ONIG_ENCODING_ISO_8859_13,
        ONIG_ENCODING_ISO_8859_14,
        ONIG_ENCODING_ISO_8859_15,
        ONIG_ENCODING_ISO_8859_16,
        ONIG_ENCODING_WINDOWS_1250,
        ONIG_ENCODING_WINDOWS_1252,
        ONIG_ENCODING_WINDOWS_1254,
        ONIG_ENCODING_WINDOWS_1257,
    };
    OnigCaseFoldType modes[] = { ONIGENC_CASE_UPCASE, ONIGENC_CASE_FOLD };
    const OnigUChar source[] = { 0xdf };
    for (unsigned int i = 0; i < sizeof(encodings) / sizeof(encodings[0]); ++i) {
        for (unsigned int j = 0; j < sizeof(modes) / sizeof(modes[0]); ++j) {
            OnigCaseFoldType flags = modes[j];
            const OnigUChar *current = source;
            OnigUChar output[] = { 0xa5, 0xa5, 0xa5 };
            OnigEncoding enc = encodings[i];
            int written = enc->case_map(&flags, &current, source + 1, output, output + 1, enc);
            assert(written == 0 && current == source);
            assert(output[0] == 0xa5 && output[1] == 0xa5 && output[2] == 0xa5);
            written = enc->case_map(&flags, &current, source + 1, output, output + 2, enc);
            assert(written == 2 && current == source + 1);
            assert(memcmp(output, j == 0 ? "SS" : "ss", 2) == 0 && output[2] == 0xa5);
        }
    }
    return 0;
}
