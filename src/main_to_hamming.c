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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
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


static void trace_reporter(const struct dc_posix_env *env,
                           const char *file_name,
                           const char *function_name,
                           size_t line_number);

static void usage(int exit_code);

//static void write_message(const char *str, const char *file_name);


static void convert_to_binary(uint8_t *binary, ssize_t nread, const char *chars, uint8_t *k) {
    uint8_t i, j, num;
    for (i = 0; i < (uint8_t) nread; i++) {
        j = (*k) - 1;
        num = (uint8_t) chars[i];
        while (num != 0) {
            binary[j--] = num % 2;
            num /= 2;
        }
        (*k) += 8; // because the next position of second character is at index of (7+8)=15
    }
}


static void addingHam(uint8_t *binary, int parity) {
    int c1 = binary[10] + binary[8] + binary[6] + binary[4] + binary[2];
    if ((((c1 % 2) != 0) && (parity == 0)) || (((c1 % 2) == 0) && (parity == 1))) {
        binary[0] = 1;
    }
    int c2 = binary[10] + binary[9] + binary[6] + binary[5] + binary[2];
    if ((((c2 % 2) != 0) && (parity == 0)) || (((c2 % 2) == 0) && (parity == 1))) {
        binary[1] = 1;
    }
    int c3 = binary[11] + binary[6] + binary[5] + binary[4];
    if ((((c3 % 2) != 0) && (parity == 0)) || (((c3 % 2) == 0) && (parity == 1))) {
        binary[3] = 1;
    }
    int c4 = binary[11] + binary[10] + binary[9] + binary[8];
    if ((((c4 % 2) != 0) && (parity == 0)) || (((c4 % 2) == 0) && (parity == 1))) {
        binary[7] = 1;
    }
}

static void convert_eight_to_twelve(uint8_t *testbinary, const uint8_t *binary, int *bin) {

    for (int i = 1; i <= 12; i++) {
        double rem = remainder(log(i), log(2));
        double newRem;
        if (rem < 0) { newRem = -(rem); }
        else { newRem = rem; }

        if (newRem < 0.0000001) {
            testbinary[i - 1] = 0;
        } else {
            testbinary[i - 1] = binary[(*bin)++];
        }
    }
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
    int bin = 0;
    char chars[BUF_SIZE];
    uint8_t i, k = 8;
    uint8_t testbinary[12];
    uint8_t byte0 = 0, byte1 = 0, byte2 = 0, byte3 = 0, byte4 = 0, byte5 = 0, byte6 = 0, byte7 = 0, byte8 = 0, byte9 = 0, byte10 = 0, byte11 = 0;
    int looop = 0;


    app_settings = (struct application_settings *) settings;
    prefix = dc_setting_path_get(env, app_settings->prefix);

    if (prefix == NULL) {
        usage(EXIT_FAILURE);
    }
    parity = dc_setting_string_get(env, app_settings->parity);
    if (strcmp(parity, "even") != 0 && strcmp(parity, "odd") != 0) {
        printf("The parity bit should be odd or even\n");
        usage(EXIT_FAILURE);
    }

    int hammingParity;
    if (strcmp(parity, "even") == 0) {
        hammingParity = 0;
    } else {
        hammingParity = 1;
    }

    printf("parity: %s\n", parity);
    printf("prefix: %s\n", prefix);
    // write_message(parity, prefix);

    nread = (dc_read(env, err, STDIN_FILENO, chars, BUF_SIZE)) - 1;

    uint8_t binary[8 * nread];

    //filling binary array with zeros
    for (i = 0; i < (uint8_t) (8 * nread); i++) {
        binary[i] = 0;
    }

    //converting each char and filling the binary array
    convert_to_binary(binary, nread, chars, &k);

    // Filling array of 12 with zeros
    for (i = 0; i < 12; i++) {
        testbinary[i] = 0;
    }


    size_t len = dc_strlen(env, prefix);
    char pathname[12][len + 11];
    for (i = 0; i < 12; i++) {
        snprintf(pathname[i], len + 11, "%s%d.hamming", prefix, i);
    }

    // open files
    int f0 = dc_open(env, err, pathname[0], DC_O_CREAT | DC_O_WRONLY, S_IRUSR | S_IWUSR);
    int f1 = dc_open(env, err, pathname[1], DC_O_CREAT | DC_O_WRONLY, S_IRUSR | S_IWUSR);
    int f2 = dc_open(env, err, pathname[2], DC_O_CREAT | DC_O_WRONLY, S_IRUSR | S_IWUSR);
    int f3 = dc_open(env, err, pathname[3], DC_O_CREAT | DC_O_WRONLY, S_IRUSR | S_IWUSR);
    int f4 = dc_open(env, err, pathname[4], DC_O_CREAT | DC_O_WRONLY, S_IRUSR | S_IWUSR);
    int f5 = dc_open(env, err, pathname[5], DC_O_CREAT | DC_O_WRONLY, S_IRUSR | S_IWUSR);
    int f6 = dc_open(env, err, pathname[6], DC_O_CREAT | DC_O_WRONLY, S_IRUSR | S_IWUSR);
    int f7 = dc_open(env, err, pathname[7], DC_O_CREAT | DC_O_WRONLY, S_IRUSR | S_IWUSR);
    int f8 = dc_open(env, err, pathname[8], DC_O_CREAT | DC_O_WRONLY, S_IRUSR | S_IWUSR);
    int f9 = dc_open(env, err, pathname[9], DC_O_CREAT | DC_O_WRONLY, S_IRUSR | S_IWUSR);
    int f10 = dc_open(env, err, pathname[10], DC_O_CREAT | DC_O_WRONLY, S_IRUSR | S_IWUSR);
    int f11 = dc_open(env, err, pathname[11], DC_O_CREAT | DC_O_WRONLY, S_IRUSR | S_IWUSR);

    if (dc_error_has_error(err)) {
        ret_val = 1;
    }


    for (size_t ou = 1; ou <= (size_t) (8 * nread); ou += 8) {

        //convert 8 to 12
        convert_eight_to_twelve(testbinary, binary, &bin);

        //adding hamming code
        addingHam(testbinary, hammingParity);


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

        //shifting by one and adding the first bit
        byte0 = (uint8_t) ((byte0 << 1) | (testbinary[0] >> 0));
        byte1 = (uint8_t) ((byte1 << 1) | (testbinary[1] >> 0));
        byte2 = (uint8_t) ((byte2 << 1) | (testbinary[2] >> 0));
        byte3 = (uint8_t) ((byte3 << 1) | (testbinary[3] >> 0));
        byte4 = (uint8_t) ((byte4 << 1) | (testbinary[4] >> 0));
        byte5 = (uint8_t) ((byte5 << 1) | (testbinary[5] >> 0));
        byte6 = (uint8_t) ((byte6 << 1) | (testbinary[6] >> 0));
        byte7 = (uint8_t) ((byte7 << 1) | (testbinary[7] >> 0));
        byte8 = (uint8_t) ((byte8 << 1) | (testbinary[8] >> 0));
        byte9 = (uint8_t) ((byte9 << 1) | (testbinary[9] >> 0));
        byte10 = (uint8_t) ((byte10 << 1) | (testbinary[10] >> 0));
        byte11 = (uint8_t) ((byte11 << 1) | (testbinary[11] >> 0));



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

//static void write_message(const char *str, const char *file_name) {
//    //printf("writng  %s to %s\n", str, file_name);
//    printf("heloooooooooooooooooooooooooooooo %s\n", str);
//}
//
static void usage(int exit_code) {
    fprintf(stderr, "Usage: --parity <string> --prefix <path-to-output-prefix>");
    exit(1);
}


static void trace_reporter(__attribute__((unused)) const struct dc_posix_env *env,
                           const char *file_name,
                           const char *function_name,
                           size_t line_number) {
    fprintf(stdout, "TRACE: %s : %s : @ %zu\n", file_name, function_name, line_number);
}