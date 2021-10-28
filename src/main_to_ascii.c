/*
 * This prefix is part of dc_dump.
 *
 *  dc_dump is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Foobar is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with dc_dump.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dc_posix/dc_fcntl.h>
#include <dc_posix/dc_unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <bits/stdint-uintn.h>

#define BUF_SIZE 1024

int main(void) {

    struct dc_error err;
    struct dc_posix_env env;
    uint8_t byte0 = 0, byte1 = 0, byte2 = 0, byte3 = 0, byte4 = 0, byte5 = 0, byte6 = 0, byte7 = 0, byte8 = 0, byte9 = 0, byte10 = 0, byte11 = 0;
    int ret_val = EXIT_SUCCESS;

    dc_error_init(&err, NULL);
    dc_posix_env_init(&env, NULL);
    int fd0 = dc_open(&env, &err, "./program0.hamming", DC_O_RDONLY, 0);
    int fd1 = dc_open(&env, &err, "./program1.hamming", DC_O_RDONLY, 0);
    int fd2 = dc_open(&env, &err, "./program2.hamming", DC_O_RDONLY, 0);
    int fd3 = dc_open(&env, &err, "./program3.hamming", DC_O_RDONLY, 0);
    int fd4 = dc_open(&env, &err, "./program4.hamming", DC_O_RDONLY, 0);
    int fd5 = dc_open(&env, &err, "./program5.hamming", DC_O_RDONLY, 0);
    int fd6 = dc_open(&env, &err, "./program6.hamming", DC_O_RDONLY, 0);
    int fd7 = dc_open(&env, &err, "./program7.hamming", DC_O_RDONLY, 0);
    int fd8 = dc_open(&env, &err, "./program8.hamming", DC_O_RDONLY, 0);
    int fd9 = dc_open(&env, &err, "./program9.hamming", DC_O_RDONLY, 0);
    int fd10 = dc_open(&env, &err, "./program10.hamming", DC_O_RDONLY, 0);
    int fd11 = dc_open(&env, &err, "./program11.hamming", DC_O_RDONLY, 0);

    printf("num\n");


    if (dc_error_has_no_error(&err)) {
        ssize_t nread;

        while ((nread = dc_read(&env, &err, fd0, &byte0, 1)) > 0) {
            dc_read(&env, &err, fd1, &byte1, 1);
            dc_read(&env, &err, fd2, &byte2, 1);
            dc_read(&env, &err, fd3, &byte3, 1);
            dc_read(&env, &err, fd4, &byte4, 1);
            dc_read(&env, &err, fd5, &byte5, 1);
            dc_read(&env, &err, fd6, &byte6, 1);
            dc_read(&env, &err, fd7, &byte7, 1);
            dc_read(&env, &err, fd8, &byte8, 1);
            dc_read(&env, &err, fd9, &byte9, 1);
            dc_read(&env, &err, fd10, &byte10, 1);
            dc_read(&env, &err, fd11, &byte11, 1);
            if (dc_error_has_error(&err)) {
                ret_val = 1;
            }
            for (size_t j = 0; j < 7; j++) {
                uint8_t oneByte;
                printf("bit0: %d", (byte0 >> j) & 0x01);
                printf("  bit1: %d", (byte1 >> j) & 0x01);
                printf("  bit2: %d", (byte2 >> j) & 0x01);
                printf("  bit3: %d", (byte3 >> j) & 0x01);
                printf("  bit4: %d", (byte4 >> j) & 0x01);
                printf("  bit5: %d", (byte5 >> j) & 0x01);
                printf("  bit6: %d", (byte6 >> j) & 0x01);
                printf("  bit7: %d", (byte7 >> j) & 0x01);
                printf("  bit8: %d", (byte8 >> j) & 0x01);
                printf("  bit9: %d", (byte9 >> j) & 0x01);
                printf("  bit10: %d", (byte10 >> j) & 0x01);
                printf("  bit11: %d\n", (byte11 >> j) & 0x01);

                uint8_t temp0 = (byte0 >> j) & 0x01;
                uint8_t temp1 = (byte1 >> j) & 0x01;
                uint8_t temp2 = (byte2 >> j) & 0x01;
                uint8_t temp3 = (byte3 >> j) & 0x01;
                uint8_t temp4 = (byte4 >> j) & 0x01;
                uint8_t temp5 = (byte5 >> j) & 0x01;
                uint8_t temp6 = (byte6 >> j) & 0x01;
                uint8_t temp7 = (byte7 >> j) & 0x01;
                uint8_t temp8 = (byte8 >> j) & 0x01;
                uint8_t temp9 = (byte9 >> j) & 0x01;
                uint8_t temp10 = (byte10 >> j) & 0x01;
                uint8_t temp11 = (byte11 >> j) & 0x01;

                oneByte = (byte0 >> j) & 0x01;
            }


            printf("prefix:  %u\n", byte0);
            printf("prefix:  %u\n", byte1);
            printf("prefix:  %u\n", byte2);
            printf("\n");
            if (dc_error_has_error(&err)) {
                ret_val = 2;
            }
        }
        dc_dc_close(&env, &err, fd0);
        dc_dc_close(&env, &err, fd1);
        dc_dc_close(&env, &err, fd2);
        dc_dc_close(&env, &err, fd3);
        dc_dc_close(&env, &err, fd4);
        dc_dc_close(&env, &err, fd5);
        dc_dc_close(&env, &err, fd6);
        dc_dc_close(&env, &err, fd7);
        dc_dc_close(&env, &err, fd8);
        dc_dc_close(&env, &err, fd9);
        dc_dc_close(&env, &err, fd10);
        dc_dc_close(&env, &err, fd11);
    }
    return ret_val;
}