/* AMLS version 0.5

   Convert a biased uncorrelated bitstream into an unbiased
   uncorrelated bitstream.
   
   http://www.ciphergoth.org/software/unbiasing

   Paul Crowley <paul@ciphergoth.org>, corners@sbcglobal.net
   December 2001

   You can do anything you want with it. It comes with a "garbage
   man's guarantee". Satisfaction guaranteed, or double your garbage
   back.  (In other words, this code has NO WARRANTY, express or
   implied, as with points 11 and 12 of the GPL.)
   
   This source is based on an algorithm called "Advanced Multilevel
   Strategy" described in a paper titled "Tossing a Biased Coin" by
   Michael Mitzenmacher

   http://www.fas.harvard.edu/~libcs124/CS/coinflip3.pdf

   To facilitate storage this recursive implementation does not
   process the bits in the same order as described in the paper.  It
   does not produce exactly the same output, but the output will be
   unbiased if the input is truly uncorrelated.  */

#include <stdio.h>
#include <stdlib.h>

/* Each input byte must be '0' or '1' (ASCII).  Other inputs will
   cause invalid output.

   The output is ASCII zeroes and ones. The length of the output is
   strictly less than that of the input. */

void amls_round(
    char *input_start,
    char *input_end,
    char **output /* Moving output pointer */
)
{
    char *low = input_start;
    char *high = input_end -1;
    char *doubles = input_start;
    
    if (high <= low)
        return;
    do {
        if (*low == *high) {
            *doubles++ = *low;
            *high = '0';
        } else {
            *(*output)++ = *low;
            *high = '1';
        }
        low++;
        high--;
    } while (high > low);
    amls_round(input_start, doubles, output);
    amls_round(high + 1, input_end, output);
}

/* Example and test */

#define INPUT_SIZE (10000)     /* Number of bits in the input */
#define ITERATIONS (100)       /* Number of iterations in the test */

int main(
    int argc,
    char* argv[]
) 
{
    char test_input[INPUT_SIZE +1];
    char test_output[INPUT_SIZE +1];
    char *output;
    int bias;
    int i, j;
    int total_out;

    if (argc != 2 || (bias = atoi(argv[1])) <= 0 || bias >= 100) {
        fprintf(stderr, "Usage: %s bias-percentage < /dev/urandom\n", argv[0]);
        exit(-1);
    }
    printf("Bias: %d / 100\n", bias);
    printf("Input bits: %d \n", INPUT_SIZE);
    for (j = 0 ; j < ITERATIONS ; j++) {
            /* Read random bytes from stdin.  Run this program as
               amls 90 < /dev/urandom */
        for (i = 0; i < INPUT_SIZE; ) {
            int c = getchar();            
                /* Should handle EOF here... */
            if (c < 200) {
                test_input[i++] =
                    (c < (bias * 2)) ? '1' : '0';
            }
        }
        output = test_output;
        amls_round(test_input, test_input + INPUT_SIZE, &output);
        printf("Output size: %d\n", output - test_output);
        total_out += output - test_output;
    }
#if 0
    test_input[INPUT_SIZE] = '\0';
    *output = '\0';
    printf("Input: %s\n", test_input);
    printf("Output: %s\n", test_output);
#endif
    printf("Total inputs: %d\n", INPUT_SIZE * j);
    printf("Total output: %d\n", total_out);
    return 0;
}

