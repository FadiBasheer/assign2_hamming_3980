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
#include <bits/stdint-uintn.h>
#include <dc_application/command_line.h>
#include <dc_application/config.h>
#include <dc_application/options.h>
#include <dc_posix/dc_stdlib.h>
#include <dc_posix/dc_string.h>
#include <dc_posix/dc_posix_env.h>
#include <getopt.h>
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <dc_posix/dc_fcntl.h>
#include <dc_posix/dc_unistd.h>
#include <bits/stdint-uintn.h>
#include <dc_application/command_line.h>
#include <dc_application/config.h>
#include <dc_application/defaults.h>
#include <dc_application/environment.h>
#include <dc_application/options.h>
#include <dc_posix/dc_stdlib.h>
#include <dc_posix/dc_string.h>
#include <dc_posix/dc_posix_env.h>
#include <getopt.h>

#define BUF_SIZE 1024

struct application_settings {
    struct dc_opt_settings opts;
    struct dc_setting_string *parity;
    struct dc_setting_path *prefix;
};


static struct dc_application_settings *create_settings(const struct dc_posix_env *env, struct dc_error *err);

static int destroy_settings(const struct dc_posix_env *env,
                            struct dc_error *err,
                            struct dc_application_settings **psettings);

static int run(const struct dc_posix_env *env, struct dc_error *err, struct dc_application_settings *settings);

static void error_reporter(const struct dc_error *err);

static int open_out(const struct dc_posix_env *env, struct dc_error *err, struct dc_setting_path *setting);

static void trace_reporter(const struct dc_posix_env *env,
                           const char *file_name,
                           const char *function_name,
                           size_t line_number);

static void usage(int exit_code);

static void write_message(const char *str, const char *file_name);


//void print_binary(int *binary, int str_len) {
//    printf("\n\nBinary Converter: \n");
//    for (int i = 1; i <= 8 * str_len; i++) {
//        printf("%d", binary[i - 1]);
//        if (i % 8 == 0) {
//            printf(" ");
//        }
//    }
//}


void testingHamming(uint8_t *binary, int hammingParity) {
    int c1 = binary[10] ^ binary[8] ^ binary[6] ^ binary[4] ^ binary[2] ^ binary[0];
    int c2 = binary[10] ^ binary[9] ^ binary[6] ^ binary[5] ^ binary[2] ^ binary[1];
    int c3 = binary[11] ^ binary[6] ^ binary[5] ^ binary[4] ^ binary[3];
    int c4 = binary[11] ^ binary[10] ^ binary[9] ^ binary[8] ^ binary[7];
    int c = (c4 * 8) + (c3 * 4) + (c2 * 2) + c1;

    if (c == 0 && hammingParity == 0) {
        printf("\nNo error while transmission of data\n");
    } else if (c != 0 && hammingParity == 0) {
        printf("\nError on position %d", c);
    }
    if (c == 15 && hammingParity == 1) {
        printf("\nNo error while transmission of data\n");
    } else if (c != 15 && hammingParity == 1) {
        printf("\nError on position %d", (15 - c));
    }
    printf("\n");
}

static void error_reporter(const struct dc_error *err) {
    printf("\n");
    fprintf(stderr, "ERROR: %s : %s : @ %zu : %d\n", err->file_name, err->function_name, err->line_number,
            err->err_code);
    fprintf(stderr, "ERROR: %s\n", err->message);
    printf("\n");
}

int main(int argc, char *argv[]) {
    dc_posix_tracer tracer;
    dc_error_reporter reporter;
    struct dc_posix_env env;
    struct dc_error err;
    struct dc_application_info *info;
    int ret_val;

    reporter = error_reporter;
    tracer = trace_reporter;
    tracer = NULL;
    dc_error_init(&err, reporter);
    dc_posix_env_init(&env, tracer);
    info = dc_application_info_create(&env, &err, "Settings Application");
    ret_val = dc_application_run(&env, &err, info, create_settings, destroy_settings, run,
                                 dc_default_create_lifecycle,
                                 dc_default_destroy_lifecycle, NULL, argc, argv);
    dc_application_info_destroy(&env, &info);
    dc_error_reset(&err);

    return ret_val;
}

static struct dc_application_settings *create_settings(const struct dc_posix_env *env, struct dc_error *err) {
    static bool default_verbose = false;
    struct application_settings *settings;

    DC_TRACE(env);
    settings = dc_malloc(env, err, sizeof(struct application_settings));

    if (settings == NULL) {
        return NULL;
    }

    settings->opts.parent.config_path = dc_setting_path_create(env, err);
    settings->parity = dc_setting_string_create(env, err);
    settings->prefix = dc_setting_path_create(env, err);


    struct options opts[] = {
            {(struct dc_setting *) settings->opts.parent.config_path,
                    dc_options_set_path,
                    "config",
                    required_argument,
                    'c',
                    "CONFIG",
                    dc_string_from_string,
                    NULL,
                    dc_string_from_config,
                    NULL},
            {(struct dc_setting *) settings->parity,
                    dc_options_set_string,
                    "parity",
                    required_argument,
                    'm', //short name
                    "PARITY", //environment variable
                    dc_string_from_string,
                    "parity", //our config
                    dc_string_from_config,
                    "Hello, Default World!"},//defult value
            {(struct dc_setting *) settings->prefix,
                    dc_options_set_path,
                    "prefix",
                    required_argument,
                    'f',//short name
                    "PREFIX", //environment variable
                    dc_string_from_string,
                    "prefix", //config
                    dc_string_from_config,
                    NULL},
    };

    // note the trick here - we use calloc and add 1 to ensure the last line is all 0/NULL
    settings->opts.opts_count = (sizeof(opts) / sizeof(struct options)) + 1;
    settings->opts.opts_size = sizeof(struct options);
    settings->opts.opts = dc_calloc(env, err, settings->opts.opts_count, settings->opts.opts_size);
    dc_memcpy(env, settings->opts.opts, opts, sizeof(opts));
    settings->opts.flags = "c:m:f:";
    settings->opts.env_prefix = "Hamming";

    return (struct dc_application_settings *) settings;
}

static int destroy_settings(const struct dc_posix_env *env,
                            __attribute__((unused)) struct dc_error *err,
                            struct dc_application_settings **psettings) {
    struct application_settings *app_settings;

    DC_TRACE(env);
    app_settings = (struct application_settings *) *psettings;
    dc_setting_string_destroy(env, &app_settings->parity);
    dc_setting_path_destroy(env, &app_settings->prefix);
    dc_free(env, app_settings->opts.opts, app_settings->opts.opts_count);
    dc_free(env, *psettings, sizeof(struct application_settings));

    if (env->null_free) {
        *psettings = NULL;
    }
    return 0;
}

static int run(const struct dc_posix_env *env, struct dc_error *err, struct dc_application_settings *settings) {
    struct application_settings *app_settings;

    const char *prefix;
    const char *parity;
    DC_TRACE(env);
    ssize_t nread;
    int ret_val = 0;
    char chars[BUF_SIZE];
    uint8_t i, j, k = 8, num;

    uint8_t testbinary[12];
    int bin = 0;
    int one[5] = {3, 5, 7, 9, 11};
    int tow[5] = {3, 6, 7, 10, 11};
    int four[4] = {5, 6, 7, 12};
    int eight[4] = {9, 10, 11, 12};
    uint8_t byte0 = 0, byte1 = 0, byte2 = 0, byte3 = 0, byte4 = 0, byte5 = 0, byte6 = 0, byte7 = 0, byte8 = 0, byte9 = 0, byte10 = 0, byte11 = 0;
    int looop = 0;

    app_settings = (struct application_settings *) settings;
    prefix = dc_setting_path_get(env, app_settings->prefix);


    if (prefix == NULL) {
        usage(EXIT_FAILURE);
    }
    parity = dc_setting_string_get(env, app_settings->parity);
    printf("parity: %s\n", parity);
    printf("prefix: %s\n", prefix);
    int hammingParity;
    if (strcmp(parity, "even") == 0) {
        hammingParity = 0;
    } else {
        hammingParity = 1;
    }

    write_message(parity, prefix);

    int fd0 = dc_open(env, err, "./program0.hamming", DC_O_RDONLY, 0);
    int fd1 = dc_open(env, err, "./program1.hamming", DC_O_RDONLY, 0);
    int fd2 = dc_open(env, err, "./program2.hamming", DC_O_RDONLY, 0);
    int fd3 = dc_open(env, err, "./program3.hamming", DC_O_RDONLY, 0);
    int fd4 = dc_open(env, err, "./program4.hamming", DC_O_RDONLY, 0);
    int fd5 = dc_open(env, err, "./program5.hamming", DC_O_RDONLY, 0);
    int fd6 = dc_open(env, err, "./program6.hamming", DC_O_RDONLY, 0);
    int fd7 = dc_open(env, err, "./program7.hamming", DC_O_RDONLY, 0);
    int fd8 = dc_open(env, err, "./program8.hamming", DC_O_RDONLY, 0);
    int fd9 = dc_open(env, err, "./program9.hamming", DC_O_RDONLY, 0);
    int fd10 = dc_open(env, err, "./program10.hamming", DC_O_RDONLY, 0);
    int fd11 = dc_open(env, err, "./program11.hamming", DC_O_RDONLY, 0);


    //if (dc_error_has_no_error(err)) {
    while ((nread = dc_read(env, err, fd0, &byte0, 1)) > 0) {
        dc_read(env, err, fd1, &byte1, 1);
        dc_read(env, err, fd2, &byte2, 1);
        dc_read(env, err, fd3, &byte3, 1);
        dc_read(env, err, fd4, &byte4, 1);
        dc_read(env, err, fd5, &byte5, 1);
        dc_read(env, err, fd6, &byte6, 1);
        dc_read(env, err, fd7, &byte7, 1);
        dc_read(env, err, fd8, &byte8, 1);
        dc_read(env, err, fd9, &byte9, 1);
        dc_read(env, err, fd10, &byte10, 1);
        dc_read(env, err, fd11, &byte11, 1);

        if (dc_error_has_error(err)) {
            ret_val = 1;
        }
        //for test
//        printf("bit0: %d\n", (byte0));
//        printf("  bit1: %d\n", (byte1));
//        printf("  bit2: %d\n", (byte2));
//        printf("  bit3: %d\n", (byte3));
//        printf("  bit4: %d\n", (byte4));
//        printf("  bit5: %d\n", (byte5));
//        printf("  bit6: %d\n", (byte6));
//        printf("  bit7: %d\n", (byte7));
//        printf("  bit8: %d\n", (byte8));
//        printf("  bit9: %d\n", (byte9));
//        printf("  bit10: %d\n", (byte10));
//        printf("  bit11: %d\n", (byte11));
        uint8_t temp0, temp1, temp2, temp3, temp4, temp5, temp6, temp7, temp8, temp9, temp10, temp11 = 0;
        for (size_t whichBit = 8; whichBit >= 1; whichBit--) {
            printf("bit0: %d", (byte0 & (1 << (whichBit - 1))));
            printf("  bit1: %d", (byte1 & (1 << (whichBit - 1))));
            printf("  bit2: %d", (byte2 & (1 << (whichBit - 1))));
            printf("  bit3: %d", (byte3 & (1 << (whichBit - 1))));
            printf("  bit4: %d", (byte4 & (1 << (whichBit - 1))));
            printf("  bit5: %d", (byte5 & (1 << (whichBit - 1))));
            printf("  bit6: %d", (byte6 & (1 << (whichBit - 1))));
            printf("  bit7: %d", (byte7 & (1 << (whichBit - 1))));
            printf("  bit8: %d", (byte8 & (1 << (whichBit - 1))));
            printf("  bit9: %d", (byte9 & (1 << (whichBit - 1))));
            printf("  bit10: %d", (byte10 & (1 << (whichBit - 1))));
            printf("  bit11: %d\n", (byte11 & (1 << (whichBit - 1))));


            testbinary[0] = (byte0 & (1 << (whichBit - 1))) >> (whichBit - 1);
            testbinary[1] = (byte1 & (1 << (whichBit - 1))) >> (whichBit - 1);
            testbinary[2] = (byte2 & (1 << (whichBit - 1))) >> (whichBit - 1);
            testbinary[3] = (byte3 & (1 << (whichBit - 1))) >> (whichBit - 1);
            testbinary[4] = (byte4 & (1 << (whichBit - 1))) >> (whichBit - 1);
            testbinary[5] = (byte5 & (1 << (whichBit - 1))) >> (whichBit - 1);
            testbinary[6] = (byte6 & (1 << (whichBit - 1))) >> (whichBit - 1);
            testbinary[7] = (byte7 & (1 << (whichBit - 1))) >> (whichBit - 1);
            testbinary[8] = (byte8 & (1 << (whichBit - 1))) >> (whichBit - 1);
            testbinary[9] = (byte9 & (1 << (whichBit - 1))) >> (whichBit - 1);
            testbinary[10] = (byte10 & (1 << (whichBit - 1))) >> (whichBit - 1);
            testbinary[11] = (byte11 & (1 << (whichBit - 1))) >> (whichBit - 1);


            int flag = 0;
            for (int l = 0; l < 12; ++l) {
                if (testbinary[l] == 1) {
                    flag = 1;
                    printf("break\n");
                    break;
                }
            }
            if (flag == 1) {
                testbinary[5] = 0;
                testingHamming(testbinary, hammingParity);
            }
        }

        if (dc_error_has_error(err)) {
            ret_val = 2;
        }
    }


    dc_dc_close(env, err, fd0);
    dc_dc_close(env, err, fd1);
    dc_dc_close(env, err, fd2);
    dc_dc_close(env, err, fd3);
    dc_dc_close(env, err, fd4);
    dc_dc_close(env, err, fd5);
    dc_dc_close(env, err, fd6);
    dc_dc_close(env, err, fd7);
    dc_dc_close(env, err, fd8);
    dc_dc_close(env, err, fd9);
    dc_dc_close(env, err, fd10);
    dc_dc_close(env, err, fd11);

    error_reporter(err);

    printf("\n");

    return ret_val;
}

static void write_message(const char *str, const char *file_name) {
    //printf("writng  %s to %s\n", str, file_name);
    printf("heloooooooooooooooooooooooooooooo %s\n", str);
}

static void usage(int exit_code) {
    fprintf(stderr, "Usage: --parity <string> --prefix <path-to-output-prefix>");
    exit(exit_code);
}


static void trace_reporter(__attribute__((unused)) const struct dc_posix_env *env,
                           const char *file_name,
                           const char *function_name,
                           size_t line_number) {
    fprintf(stdout, "TRACE: %s : %s : @ %zu\n", file_name, function_name, line_number);
}







