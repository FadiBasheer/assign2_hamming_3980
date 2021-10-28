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


void addingHam(int location, int *a, uint8_t *binary, int size) {
    int count = 0;
    for (int i = 0; i < size; i++) {
        if (binary[a[i] - 1] == 1) {
            count++;
        }
        // printf("---- %d", binary[a[i] - 1]);
    }
    if ((count % 2) != 0) {
        binary[location - 1] = 1;
    }
}


static void error_reporter(const struct dc_error *err) {
    printf("\n");
    fprintf(stderr, "ERROR: %s : %s : @ %zu : %d\n", err->file_name, err->function_name, err->line_number,
            err->err_code);
    fprintf(stderr, "ERROR: %s\n", err->message);
    printf("\n");
}


//static void error_reporter(const struct dc_error *err) {
//    fprintf(stderr, "ERROR: %s : %s : @ %zu : %d\n", err->file_name, err->function_name, err->line_number, 0);
//    fprintf(stderr, "ERROR: %s\n", err->parity);
//}




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
    ret_val = dc_application_run(&env, &err, info, create_settings, destroy_settings, run, dc_default_create_lifecycle,
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
    int ret_val;

    app_settings = (struct application_settings *) settings;
    prefix = dc_setting_path_get(env, app_settings->prefix);


    if (prefix == NULL) {
        usage(EXIT_FAILURE);
    }
    parity = dc_setting_string_get(env, app_settings->parity);
    printf("parity: %s\n", parity);
    printf("prefix: %s\n", prefix);
    write_message(parity, prefix);


    uint8_t test;
    test = 'A';
    printf("num: %d\n", test);
    printf("num: %d\n", 'A');

    char chars[BUF_SIZE];


//    dc_error_init(err, NULL);
//    dc_posix_env_init((struct dc_posix_env *) &env, NULL);
//    ret_val = EXIT_SUCCESS;

    /*
    while ((nread = dc_read(&env, &err, STDIN_FILENO, chars, BUF_SIZE)) > 0) {
        if (dc_error_has_error(&err)) {
            ret_val = 1;
        }


        for (int i = 0; i < strlen(chars); i++) {
            chars[i] = toupper(chars[i]);
        }
        dc_write(&env, &err, STDOUT_FILENO, chars, (size_t) nread);

        if (dc_error_has_error(&err)) {
            ret_val = 2;
        }
    }
*/


    nread = (dc_read(env, err, STDIN_FILENO, chars, BUF_SIZE)) - 1;
    printf("str_len: %zu\n", nread);


    // size_t str_len = strlen(chars) - 1;


    uint8_t i, j, k = 8, num;
    uint8_t binary[8 * nread];

    //filling binary array with zeros
    for (i = 0; i < (8 * nread); i++) {
        binary[i] = 0;
    }

    //converting each char and filling the binary array
    for (i = 0; i < nread; i++) {
        j = k - 1;
        num = chars[i];
        while (num != 0) {
            binary[j--] = num % 2;
            num /= 2;
        }
        k += 8; // because the next position of second character is at index of (7+8)=15
    }


// Convert from 8 to 12
    uint8_t testbinary[12];
    for (i = 0; i < 12; i++) {
        testbinary[i] = 0;
    }
    int bin = 0;
    int one[5] = {3, 5, 7, 9, 11};
    int tow[5] = {3, 6, 7, 10, 11};
    int four[4] = {5, 6, 7, 12};
    int eight[4] = {9, 10, 11, 12};

    // open the files
    int f0 = dc_open(env, err, "./program0.hamming", DC_O_CREAT | DC_O_WRONLY, S_IRUSR | S_IWUSR);
    int f1 = dc_open(env, err, "./program1.hamming", DC_O_CREAT | DC_O_WRONLY, S_IRUSR | S_IWUSR);
    int f2 = dc_open(env, err, "./program2.hamming", DC_O_CREAT | DC_O_WRONLY, S_IRUSR | S_IWUSR);
    int f3 = dc_open(env, err, "./program3.hamming", DC_O_CREAT | DC_O_WRONLY, S_IRUSR | S_IWUSR);
    int f4 = dc_open(env, err, "./program4.hamming", DC_O_CREAT | DC_O_WRONLY, S_IRUSR | S_IWUSR);
    int f5 = dc_open(env, err, "./program5.hamming", DC_O_CREAT | DC_O_WRONLY, S_IRUSR | S_IWUSR);
    int f6 = dc_open(env, err, "./program6.hamming", DC_O_CREAT | DC_O_WRONLY, S_IRUSR | S_IWUSR);
    int f7 = dc_open(env, err, "./program7.hamming", DC_O_CREAT | DC_O_WRONLY, S_IRUSR | S_IWUSR);
    int f8 = dc_open(env, err, "./program8.hamming", DC_O_CREAT | DC_O_WRONLY, S_IRUSR | S_IWUSR);
    int f9 = dc_open(env, err, "./program9.hamming", DC_O_CREAT | DC_O_WRONLY, S_IRUSR | S_IWUSR);
    int f10 = dc_open(env, err, "./program10.hamming", DC_O_CREAT | DC_O_WRONLY, S_IRUSR | S_IWUSR);
    int f11 = dc_open(env, err, "./program11.hamming", DC_O_CREAT | DC_O_WRONLY, S_IRUSR | S_IWUSR);

    if (dc_error_has_error(err)) {
        ret_val = 1;
    }

    uint8_t byte0 = 0, byte1 = 0, byte2 = 0, byte3 = 0, byte4 = 0, byte5 = 0, byte6 = 0, byte7 = 0, byte8 = 0, byte9 = 0, byte10 = 0, byte11 = 0;
    int looop = 0;


    for (size_t ou = 1; ou <= (size_t) (8 * nread); ou += 8) {
        for (i = 1; i <= 12; i++) {
            double rem = remainder(log(i), log(2));
            double newRem;
            if (rem < 0) { newRem = -(rem); }
            else { newRem = rem; }

            if (newRem < 0.0000001) {
                testbinary[i - 1] = 0;
            } else {
                testbinary[i - 1] = binary[bin++];
            }
        }
        addingHam(1, one, testbinary, 5);
        addingHam(2, tow, testbinary, 5);
        addingHam(4, four, testbinary, 4);
        addingHam(8, eight, testbinary, 4);

        printf("\n");

        for (int p = 0; p < 12; p++) {
            printf("%d", testbinary[p]);
        }
        printf("\n");

        if (looop == 8) {
            dc_write(env, err, f0, &byte0, 1);
            dc_write(env, err, f1, &byte1, 1);
            dc_write(env, err, f2, &byte2, 1);
            dc_write(env, err, f3, &byte3, 1);
            dc_write(env, err, f4, &byte4, 1);
            dc_write(env, err, f5, &byte5, 1);
            dc_write(env, err, f6, &byte6, 1);
            dc_write(env, err, f7, &byte7, 1);
            dc_write(env, err, f8, &byte8, 1);
            dc_write(env, err, f9, &byte9, 1);
            dc_write(env, err, f10, &byte10, 1);
            dc_write(env, err, f11, &byte11, 1);
            if (dc_error_has_error(err)) {
                ret_val = 2;
            }
            byte0 = 0, byte1 = 0, byte2 = 0, byte3 = 0, byte4 = 0, byte5 = 0, byte6 = 0, byte7 = 0, byte8 = 0, byte9 = 0, byte10 = 0, byte11 = 0;
            looop = 0;
        }
        looop++;

        //shifting by one
        byte0 = (uint8_t) (byte0 << 1);
        byte1 = (uint8_t) (byte1 << 1);
        byte2 = (uint8_t) (byte2 << 1);
        byte3 = (uint8_t) (byte3 << 1);
        byte4 = (uint8_t) (byte4 << 1);
        byte5 = (uint8_t) (byte5 << 1);
        byte6 = (uint8_t) (byte6 << 1);
        byte7 = (uint8_t) (byte7 << 1);
        byte8 = (uint8_t) (byte8 << 1);
        byte9 = (uint8_t) (byte9 << 1);
        byte10 = (uint8_t) (byte10 << 1);
        byte11 = (uint8_t) (byte11 << 1);

        printf("bit0: %d", (testbinary[0] >> 0) & 0x01);
        printf("  bit1: %d", (testbinary[1] >> 0) & 0x01);
        printf("  bit2: %d", (testbinary[2] >> 0) & 0x01);
        printf("  bit3: %d", (testbinary[3] >> 0) & 0x01);
        printf("  bit4: %d", (testbinary[4] >> 0) & 0x01);
        printf("  bit5: %d", (testbinary[5] >> 0) & 0x01);
        printf("  bit6: %d", (testbinary[6] >> 0) & 0x01);
        printf("  bit7: %d", (testbinary[7] >> 0) & 0x01);
        printf("  bit8: %d", (testbinary[8] >> 0) & 0x01);
        printf("  bit9: %d", (testbinary[9] >> 0) & 0x01);
        printf("  bit10: %d", (testbinary[10] >> 0) & 0x01);
        printf("  bit11: %d\n", (testbinary[11] >> 0) & 0x01);

        //adding the first bit
        byte0 = (uint8_t) (byte0 | (testbinary[0] >> 0));
        byte1 = (uint8_t) (byte1 | (testbinary[1] >> 0));
        byte2 = (uint8_t) (byte2 | (testbinary[2] >> 0));
        byte3 = (uint8_t) (byte3 | (testbinary[3] >> 0));
        byte4 = (uint8_t) (byte4 | (testbinary[4] >> 0));
        byte5 = (uint8_t) (byte5 | (testbinary[5] >> 0));
        byte6 = (uint8_t) (byte6 | (testbinary[6] >> 0));
        byte7 = (uint8_t) (byte7 | (testbinary[7] >> 0));
        byte8 = (uint8_t) (byte8 | (testbinary[8] >> 0));
        byte9 = (uint8_t) (byte9 | (testbinary[9] >> 0));
        byte10 = (uint8_t) (byte10 | (testbinary[10] >> 0));
        byte11 = (uint8_t) (byte11 | (testbinary[11] >> 0));



        //printing the byte
        for (int l = 7; l >= 0; --l) { printf("%d", (byte0 >> l) & 0x01); }
        printf(" ");
        for (int l = 7; l >= 0; --l) { printf("%d", (byte1 >> l) & 0x01); }
        printf(" ");
        for (int l = 7; l >= 0; --l) { printf("%d", (byte2 >> l) & 0x01); }
        printf(" ");
        for (int l = 7; l >= 0; --l) { printf("%d", (byte3 >> l) & 0x01); }
        printf(" ");
        for (int l = 7; l >= 0; --l) { printf("%d", (byte4 >> l) & 0x01); }
        printf(" ");
        for (int l = 7; l >= 0; --l) { printf("%d", (byte5 >> l) & 0x01); }
        printf(" ");
        for (int l = 7; l >= 0; --l) { printf("%d", (byte6 >> l) & 0x01); }
        printf(" ");
        for (int l = 7; l >= 0; --l) { printf("%d", (byte7 >> l) & 0x01); }
        printf(" ");
        for (int l = 7; l >= 0; --l) { printf("%d", (byte8 >> l) & 0x01); }
        printf(" ");
        for (int l = 7; l >= 0; --l) { printf("%d", (byte9 >> l) & 0x01); }
        printf(" ");
        for (int l = 7; l >= 0; --l) { printf("%d", (byte10 >> l) & 0x01); }
        printf(" ");
        for (int l = 7; l >= 0; --l) { printf("%d", (byte11 >> l) & 0x01); }


    }

    dc_write(env, err, f0, &byte0, 1);
    dc_write(env, err, f1, &byte1, 1);
    dc_write(env, err, f2, &byte2, 1);
    dc_write(env, err, f3, &byte3, 1);
    dc_write(env, err, f4, &byte4, 1);
    dc_write(env, err, f5, &byte5, 1);
    dc_write(env, err, f6, &byte6, 1);
    dc_write(env, err, f7, &byte7, 1);
    dc_write(env, err, f8, &byte8, 1);
    dc_write(env, err, f9, &byte9, 1);
    dc_write(env, err, f10, &byte10, 1);
    dc_write(env, err, f11, &byte11, 1);
    if (dc_error_has_error(err)) {
        ret_val = 2;
    }

    dc_dc_close(env, err, f0);
    dc_dc_close(env, err, f1);
    dc_dc_close(env, err, f2);
    dc_dc_close(env, err, f3);
    dc_dc_close(env, err, f4);
    dc_dc_close(env, err, f5);
    dc_dc_close(env, err, f6);
    dc_dc_close(env, err, f7);
    dc_dc_close(env, err, f8);
    dc_dc_close(env, err, f9);
    dc_dc_close(env, err, f10);
    dc_dc_close(env, err, f11);

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