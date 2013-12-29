/*
 * Copyright (C) 2013 Paul Warren <pwarren@pwarren.id.au>
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

#include <openssl/evp.h>
#include <openssl/sha.h>

extern char *pidfile_path;
extern unsigned char hash_buffer[SHA512_DIGEST_LENGTH];
extern unsigned char hash_data_buffer[SHA512_DIGEST_LENGTH];
extern unsigned int hash_loop;

int parse_user(char *username, int *gid);
int parse_group(char *groupname);
void write_pidfile(void);
void daemonize(void);
double atofs(char* f);
int aes_init(unsigned char *key_data, int key_data_len, EVP_CIPHER_CTX *e_ctx);
unsigned char *aes_encrypt(EVP_CIPHER_CTX *e, unsigned char *plaintext, int *len);
void store_hash_data(int bit);
int debias(void *in_buffer, void *out_buffer, void *discard_buffer, size_t num_samples);


