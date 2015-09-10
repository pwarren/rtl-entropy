/* AMLS - bit extraction strategy - version 0.1

   http://www.ciphergoth.org/software/unbiasing

   Paul Crowley <paul@ciphergoth.org>, corners@sbcglobal.net
   December 2001

   Convert a biased uncorrelated bitstream into an unbiased
   uncorrelated bitstream, according to the strategy described by
   Yuval Peres, "Iterating von Neumann's Procedure for Extracting
   Random Bits", The Annals of Statistics, 1992, pp 590-597.  
   
   Public domain.  No warranty, express or implied. */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define GET_BIT(a,x) (((a)>>(x))&1)
#define ASSIGN_BIT(a,x,v) ((v)?((a)|=(1<<(x))):((a)&=~(1<<(x))))

#define WORD_WIDTH (5) /* Width is 2^5 == 32 */
#define WORD_MASK ((1<<WORD_WIDTH)-1) /* 31 */

/* x is multiply read here */
#define GET_ABIT(a,x) GET_BIT((a)[(x)>>WORD_WIDTH], (x) & WORD_MASK)
#define ASSIGN_ABIT(a,x,v) \
    ASSIGN_BIT((a)[(x)>>WORD_WIDTH], (x) & WORD_MASK, v)

#define MAXLEVELS (14) /* requires 4k of storage */
#define NUM_STREAMS (1<<MAXLEVELS) /* but stream 0 is unused */

/* Each stream needs 2 bits of storage */
#define STREAM_STORAGE_WORDS ((NUM_STREAMS*2)>>WORD_WIDTH)

uint32_t stream_storage[STREAM_STORAGE_WORDS];

/* Each input bit must be 0 or 1.  Other inputs will cause invalid
   output.

   The output is ASCII zeroes and ones. The number of bits in the
   output is strictly less than that of the input. */

void amls_accept_bit(
    int stream_id,
    int bit,
    char **output /* Moving output pointer */
)
{
    int old_bit;
    
    if (stream_id >= NUM_STREAMS)
        return;
    if (!GET_ABIT(stream_storage, 2*stream_id)) {
            /* We don't have an old bit - store one */
        ASSIGN_ABIT(stream_storage, 2*stream_id, 1);
        ASSIGN_ABIT(stream_storage, 2*stream_id +1, bit);
    } else {
            /* Use the old bit up */
        old_bit = GET_ABIT(stream_storage, 2*stream_id +1);
        ASSIGN_ABIT(stream_storage, 2*stream_id, 0);
        if (bit == old_bit) {
            amls_accept_bit(2*stream_id +1, bit, output);
            amls_accept_bit(2*stream_id, 0, output);
        } else {
            *(*output)++ = bit ? '1' : '0';
            amls_accept_bit(2*stream_id, 1, output);
        }
    }
}

/* Example and test */

#define INPUT_SIZE (100000)     /* Number of bits in the input */
#define ITERATIONS (10)       /* Number of iterations in the test */

int main(
    int argc,
    char* argv[]
) 
{
    char test_output[INPUT_SIZE +1];
    char *output;
    int bias;
    int i, j;
    int total_out = 0;

    if (argc != 2 || (bias = atoi(argv[1])) <= 0 || bias >= 100) {
        fprintf(stderr, "Usage: %s bias-percentage < /dev/urandom\n", argv[0]);
        exit(-1);
    }
    printf("Bias: %d / 100\n", bias);
    printf("Input bits: %d \n", INPUT_SIZE);
    for (j = 0 ; j < ITERATIONS ; j++) {
            /* Assume integer 0 is stored as chars 0000 */
        memset(stream_storage, 0,
               sizeof(uint32_t) * STREAM_STORAGE_WORDS);
            /* Read random bytes from stdin.  Run this program as
               amls 90 < /dev/urandom */
        output = test_output;
        for (i = 0; i < INPUT_SIZE; ) {
            int c = getchar();            
                /* Should handle EOF here... */
            if (c < 200) {
                amls_accept_bit(1, (c < (bias * 2)), &output);
                i++;
            }
        }
        printf("Output size: %d\n", output - test_output);
        total_out += output - test_output;
    }
#if 0
    *output = '\0';
    printf("Output: %s\n", test_output);
#endif
    printf("Total inputs: %d\n", INPUT_SIZE * j);
    printf("Total output: %d\n", total_out);
    return 0;
}

