#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "errExit.h"
#include <openssl/sha.h>
#include <openssl/evp.h>


void digest_file(const char *filename, uint8_t * hash) {

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (mdctx == NULL) {
        fprintf(stderr, "EVP_MD_CTX_new failed\n");
        exit(1);
    }
    if (EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL) != 1) {
        fprintf(stderr, "EVP_DigestInit_ex failed\n");
        EVP_MD_CTX_free(mdctx);
        exit(1);
    }

    char buffer[32];
    int file = open(filename, O_RDONLY, 0);
    if (file == -1) {
        printf("File %s does not exist\n", filename);
        EVP_MD_CTX_free(mdctx);
        exit(1);
    }

    ssize_t bR;
    do {
        bR = read(file, buffer, 32);
        if (bR > 0) {
            if (EVP_DigestUpdate(mdctx, buffer, bR) != 1) {
                fprintf(stderr, "EVP_DigestUpdate failed\n");
                close(file);
                EVP_MD_CTX_free(mdctx);
                exit(1);
            }
        } else if (bR < 0) {
            printf("Read failed\n");
            close(file);
            EVP_MD_CTX_free(mdctx);
            exit(1);
        }
    } while (bR > 0);

    unsigned int hash_len = 0;
    if (EVP_DigestFinal_ex(mdctx, hash, &hash_len) != 1) {
        fprintf(stderr, "EVP_DigestFinal_ex failed\n");
        close(file);
        EVP_MD_CTX_free(mdctx);
        exit(1);
    }

    close(file);
    EVP_MD_CTX_free(mdctx);
}