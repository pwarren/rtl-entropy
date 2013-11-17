/*
 * Copyright (C) 2013 Nicholas J. Kain <nicholas aatt kain.us>
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

#include <math.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <syslog.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <grp.h>
#include <pwd.h>
#include <openssl/evp.h>
#include <openssl/aes.h>

#include "log.h"
#include "util.h"
#include "defines.h"

char *pidfile_path = DEFAULT_PID_FILE;

int parse_user(char *username, int *gid)
{
  int t;
  char *p;
  struct passwd *pws;
  
  t = (unsigned int) strtol(username, &p, 10);
  if (*p != '\0') {
    pws = getpwnam(username);
    if (pws) {
      t = (int)pws->pw_uid;
      if (*gid < 1)
	*gid = (int)pws->pw_gid;
    } else suicide("FATAL - Invalid uid specified.\n");
  }
  return t;
}

int parse_group(char *groupname)
{
  int t;
  char *p;
  struct group *grp;
  
  t = (unsigned int) strtol(groupname, &p, 10);
  if (*p != '\0') {
    grp = getgrnam(groupname);
    if (grp) {
      t = (int)grp->gr_gid;
    } else suicide("FATAL - Invalid gid specified.\n");
  }
  return t;
}

void write_pidfile(void)
{
  FILE *fh = fopen(pidfile_path, "w");
  if (!fh)
    suicide("failed creating pid file %s", pidfile_path);
  
  fprintf(fh, "%i", getpid());
  fclose(fh);
}

void daemonize(void)
{
  if (daemon(0, 0) == -1)
    suicide("fork failed");
  
  write_pidfile();
}


double atofs(char* f)
/* standard suffixes */
{
  char* chop;
  double suff = 1.0;
  chop = malloc((strlen(f)+1)*sizeof(char));
  strncpy(chop, f, strlen(f)-1);
  switch (f[strlen(f)-1]) {
  case 'G':
    suff *= 1e3;
  case 'M':
    suff *= 1e3;
  case 'k':
    suff *= 1e3;
    suff *= atof(chop);}
  free(chop);
  if (suff != 1.0) {
    return suff;}
  return atof(f);
}



/**
 * Create an 256 bit key and IV using the supplied key_data. salt can be added for taste.
 * Fills in the encryption and decryption ctx objects and returns 0 on success
 **/
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
