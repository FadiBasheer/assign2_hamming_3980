/*Fadi Basheer*/

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

static void usage(int exit_code);

static void write_message(const char *parity, const char *file_name);

/**
 * Testing and correcting hamming code
 * @param binary
 * @param hammingParity
 */
static void testingHamming(uint8_t *binary, int hammingParity) {
    int c1 = binary[10] ^ binary[8] ^ binary[6] ^ binary[4] ^ binary[2] ^ binary[0];
    int c2 = binary[10] ^ binary[9] ^ binary[6] ^ binary[5] ^ binary[2] ^ binary[1];
    int c3 = binary[11] ^ binary[6] ^ binary[5] ^ binary[4] ^ binary[3];
    int c4 = binary[11] ^ binary[10] ^ binary[9] ^ binary[8] ^ binary[7];
    int c = (c4 * 8) + (c3 * 4) + (c2 * 2) + c1;

    if (c != 0 && hammingParity == 0) {
        binary[c - 1] ^= 1;
    }
    if (c != 15 && hammingParity == 1) {
        binary[14 - c] ^= 1;
    }
}

/**
 * report errors
 * @param err
 */
static void error_reporter(const struct dc_error *err) {
    printf("\n");
    fprintf(stderr, "ERROR: %s : %s : @ %zu : %d\n", err->file_name, err->function_name, err->line_number,
            err->err_code);
    fprintf(stderr, "ERROR: %s\n", err->message);
    printf("\n");
}

/**
 * main function
 * @param argc
 * @param argv
 * @return
 */
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
    ret_val = dc_application_run(&env, &err, info, create_settings, destroy_settings, run,
                                 dc_default_create_lifecycle,
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

    settings->opts.opts_count = (sizeof(opts) / sizeof(struct options)) + 1;
    settings->opts.opts_size = sizeof(struct options);
    settings->opts.opts = dc_calloc(env, err, settings->opts.opts_count, settings->opts.opts_size);
    dc_memcpy(env, settings->opts.opts, opts, sizeof(opts));
    settings->opts.flags = "c:m:f:";
    settings->opts.env_prefix = "Hamming";

    return (struct dc_application_settings *) settings;
}

/**
 * destroy settings
 * @param env
 * @param err
 * @param psettings
 * @return
 */
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
    int ret_val = 0;
    uint8_t binary_with_hamming[12];
    uint8_t byte0 = 0, byte1 = 0, byte2 = 0, byte3 = 0, byte4 = 0, byte5 = 0, byte6 = 0, byte7 = 0, byte8 = 0, byte9 = 0, byte10 = 0, byte11 = 0;

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

    write_message(parity, prefix);

    //Generating files names
    size_t len = dc_strlen(env, prefix);
    char pathname[12][len + 11];
    for (int i = 0; i < 12; i++) {
        snprintf(pathname[i], len + 11, "%s%d.hamming", prefix, i);
    }

    int fd0 = dc_open(env, err, pathname[0], DC_O_RDONLY, 0);
    int fd1 = dc_open(env, err, pathname[1], DC_O_RDONLY, 0);
    int fd2 = dc_open(env, err, pathname[2], DC_O_RDONLY, 0);
    int fd3 = dc_open(env, err, pathname[3], DC_O_RDONLY, 0);
    int fd4 = dc_open(env, err, pathname[4], DC_O_RDONLY, 0);
    int fd5 = dc_open(env, err, pathname[5], DC_O_RDONLY, 0);
    int fd6 = dc_open(env, err, pathname[6], DC_O_RDONLY, 0);
    int fd7 = dc_open(env, err, pathname[7], DC_O_RDONLY, 0);
    int fd8 = dc_open(env, err, pathname[8], DC_O_RDONLY, 0);
    int fd9 = dc_open(env, err, pathname[9], DC_O_RDONLY, 0);
    int fd10 = dc_open(env, err, pathname[10], DC_O_RDONLY, 0);
    int fd11 = dc_open(env, err, pathname[11], DC_O_RDONLY, 0);


    if (dc_error_has_no_error(err)) {
        while ((dc_read(env, err, fd0, &byte0, 1)) > 0) {
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

            for (size_t whichBit = 8; whichBit >= 1; whichBit--) {
                binary_with_hamming[0] = (byte0 & (1 << (whichBit - 1))) >> (whichBit - 1);
                binary_with_hamming[1] = (byte1 & (1 << (whichBit - 1))) >> (whichBit - 1);
                binary_with_hamming[2] = (byte2 & (1 << (whichBit - 1))) >> (whichBit - 1);
                binary_with_hamming[3] = (byte3 & (1 << (whichBit - 1))) >> (whichBit - 1);
                binary_with_hamming[4] = (byte4 & (1 << (whichBit - 1))) >> (whichBit - 1);
                binary_with_hamming[5] = (byte5 & (1 << (whichBit - 1))) >> (whichBit - 1);
                binary_with_hamming[6] = (byte6 & (1 << (whichBit - 1))) >> (whichBit - 1);
                binary_with_hamming[7] = (byte7 & (1 << (whichBit - 1))) >> (whichBit - 1);
                binary_with_hamming[8] = (byte8 & (1 << (whichBit - 1))) >> (whichBit - 1);
                binary_with_hamming[9] = (byte9 & (1 << (whichBit - 1))) >> (whichBit - 1);
                binary_with_hamming[10] = (byte10 & (1 << (whichBit - 1))) >> (whichBit - 1);
                binary_with_hamming[11] = (byte11 & (1 << (whichBit - 1))) >> (whichBit - 1);

                //check if the byte is not empty
                int flag = 0;
                for (int l = 0; l < 12; ++l) {
                    if (binary_with_hamming[l] == 1) {
                        flag = 1;
                        break;
                    }
                }

                if (flag == 1) {

                    testingHamming(binary_with_hamming, hammingParity);

                    char ch;
                    //create the char
                    for (int l = 2; l < 12; ++l) {
                        if (l != 3 && l != 7) {
                            ch = (char) (ch << 1);
                            ch = ((char) (ch | (binary_with_hamming[l] >> 0)));
                        }
                    }
                    printf("%c", ch);
                }
            }
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
    return ret_val;
}

/**
 * writing a message
 * @param parity
 * @param file_name
 */
static void write_message(const char *parity, const char *file_name) {
    printf("\n");
    printf("You chose (%s) parity bit, and the prefix for files is: %s\n", parity, file_name);
    printf("Your message is: ");
}

static void usage(int exit_code) {
    fprintf(stderr, "Usage: --parity <string> --prefix <path-to-output-prefix>");
    exit(exit_code);
}







