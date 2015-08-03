/*
 * Von Neumann whitener, 
 * If specified, reads from given file, else reads from  stdin data.
 * Outputs whitened data to file or stdout.
 *
 * Copyright (C) 2013 by Paul Warren <pwarren@pwarren.id.au>

 * Parts taken from:
 *  - rtl_test. Copyright (C) 2012 by Steve Markgraf <steve@steve-m.de>
 *  - http://openfortress.org/cryptodoc/random/noise-filter.c
 *      by Rick van Rein <rick@openfortress.nl>
 *  - snd-egd Copyright (C) 2008-2010 Nicholas J. Kain <nicholas aatt kain.us>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/sha.h>


#define BUFFER_SIZE 10000

/* Buffers */
unsigned char bitbuffer[BUFFER_SIZE] = {0};
unsigned char buffer[BUFFER_SIZE] = {0};

/* Counters */
unsigned int bitcounter = 0;
unsigned int buffercounter = 0;
unsigned int n_read = 0;

/* Crypto bits */
unsigned char hash_buffer[SHA512_DIGEST_LENGTH] = {0};
unsigned char hash_data_buffer[SHA512_DIGEST_LENGTH] = {0};
unsigned int hash_data_counter = 0;
unsigned int hash_data_bit_counter = 0;
unsigned int hash_loop = 0;
int aes_len;

EVP_CIPHER_CTX en;
AES_KEY wctx;

/* Functions (mostly from util.c */
int aes_init(unsigned char *key_data, int key_data_len, EVP_CIPHER_CTX *e_ctx) 
{
  int nrounds = 5;
  unsigned char key[32], iv[32];
  unsigned int salt[] = { 25016, 29592 };
  EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha1(), (unsigned char *)&salt, key_data, key_data_len, nrounds, key, iv);
  EVP_CIPHER_CTX_init(e_ctx);
  EVP_EncryptInit_ex(e_ctx, EVP_aes_256_cbc(), NULL, key, iv);
  return 0;
}

/*
 * Encrypt *len bytes of data
 * All data going in & out is considered binary (unsigned char[])
 */
unsigned char *aes_encrypt(EVP_CIPHER_CTX *e, unsigned char *plaintext, int *len)
{
  /* max ciphertext len for a n bytes of plaintext is n + AES_BLOCK_SIZE -1 bytes */
  int c_len = *len + AES_BLOCK_SIZE, f_len = 0;
  unsigned char *ciphertext = malloc(c_len);

  /* allows reusing of 'e' for multiple encryption cycles */
  EVP_EncryptInit_ex(e, NULL, NULL, NULL, NULL);
  
  /* update ciphertext, c_len is filled with the length of ciphertext generated,
   *len is the size of plaintext in bytes */
  EVP_EncryptUpdate(e, ciphertext, &c_len, plaintext, *len);
  /* update ciphertext with the final remaining bytes */
  EVP_EncryptFinal_ex(e, ciphertext+c_len, &f_len);

  *len = c_len + f_len;
  return ciphertext;
}
void store_hash_data(int bit) {
  /* store data in a sort of ring buffer */
  if (bit) {
    hash_data_buffer[hash_data_counter] |= 1 << hash_data_bit_counter;
  } else {
    hash_data_buffer[hash_data_counter] &= ~(1 << hash_data_bit_counter);
  }
  hash_data_bit_counter++;
  if (hash_data_bit_counter == sizeof(hash_data_buffer[0]) * 8) {
    hash_data_bit_counter = 0;
    hash_data_counter++;
  }
  
  if (hash_data_counter == SHA512_DIGEST_LENGTH) {
    hash_data_counter = 0;
    hash_loop=1;
  }
}

int main(int argc, char **argv) {
  unsigned int i, j;
  int ch, ch2;
  uint8_t *ciphertext;

  n_read = fread(&buffer, sizeof(buffer[0]), BUFFER_SIZE, stdin);
  while ( n_read > 0) {      
    /* for each byte in the rtl-sdr read buffer
       pick least significant 6 bits
       for now:
       debias, storing useful bits in write buffer, 
       and discarded bits in hash buffer
       until the write buffer is full.
       create a key by SHA512() hashing the hash buffer
       encrypt write buffer with key
       output encrypted buffer
       
    */
    
    /* debias(buffer, bitbuffer, n_read, sizeof(buffer[0])); */
    for (i=0; i < n_read * sizeof(buffer[0]); i++) {
      for (j=0; j < 6; j+= 2) {
	ch = (buffer[i] >> j) & 0x01;
	ch2 = (buffer[i] >> (j+1)) & 0x01;
	if (ch != ch2) {
	  if (ch) {
	    /* store a 1 in our bitbuffer */
	    bitbuffer[buffercounter] |= 1 << bitcounter;
	  } /* else leave it alone, we have a 0 alread */
	  bitcounter++;
	} else {
	  store_hash_data(ch);
	}
      }
      
      /* is byte full? */
      if (bitcounter >= sizeof(bitbuffer[0]) * 8) {
	buffercounter++;
	bitcounter = 0;
      }

      //fprintf(stderr,"bitcounter: %d \t buffercounter: %d\n",bitcounter,buffercounter);
      
      /* is buffer full? */
      if (buffercounter >= BUFFER_SIZE) {
	/* We have 2500 bytes of entropy 
	   Can now attempt encryption! */
	if (hash_loop) {
	  /* Get a key from disacarded bits */
	  SHA512(hash_data_buffer, sizeof(hash_data_buffer), hash_buffer);
	  /* use key to encrypt output */
	  /* AES_set_encrypt_key(hash_buffer, 128, &wctx); */
	  /* AES_encrypt(bitbuffer, bitbuffer_old, &wctx); */
	  aes_init(hash_buffer, sizeof(hash_buffer), &en);
	  aes_len = sizeof(bitbuffer);
	  ciphertext = aes_encrypt(&en, bitbuffer, &aes_len);
	  /* yay, send it to the output! */
	  fwrite(ciphertext,sizeof(ciphertext[0]),aes_len,stdout);
	  /* Clean up */
	  free(ciphertext);
	  EVP_CIPHER_CTX_cleanup(&en);
	}
	/* Reset the buffer */
	memset(bitbuffer,0,sizeof(bitbuffer));
	buffercounter = 0;
      }
    }
    n_read = fread(&buffer, sizeof(buffer[0]), BUFFER_SIZE, stdin);
  }
  
  return 0;
  
}
