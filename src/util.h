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
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
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
int debias(int16_t one, int16_t two, int bit_index);


