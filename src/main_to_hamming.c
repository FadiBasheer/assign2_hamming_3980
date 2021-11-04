/*Fadi Basheer*/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <dc_application/command_line.h>
#include <dc_application/config.h>
#include <dc_application/options.h>
#include <dc_posix/dc_fcntl.h>
#include <dc_posix/dc_string.h>
#include <dc_posix/dc_unistd.h>
#include <dc_posix/sys/dc_stat.h>
#include <dc_util/dump.h>
#include <dc_util/streams.h>
#include <getopt.h>
#include <sys/fcntl.h>
#include <unistd.h>

#define BUF_SIZE 6024

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
 * Convert char to binary
 * @param binary
 * @param nread
 * @param chars
 * @param k
 */
static void convert_to_binary(uint8_t *binary, size_t nread, char *chars) {
    uint8_t num;
    size_t j;
    size_t k = 8;
    printf("nrea: %zd\n", nread);
    for (size_t i = 0; i < nread; i++) {
        j = k - 1;
        num = (uint8_t) chars[i];
        printf("binary %zu= %c  %d \n", i, chars[i], num);
        //printf("%d ", num);
        while (num != 0) {
            binary[j--] = num % 2;
            num /= 2;
        }
        k += 8; // because the next position of second character is at index of (7+8)=15
    }
}

/**
 * Adding hamming code
 * @param binary
 * @param parity
 */
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

/**
 * Convert from array 0f 8 to array of 12
 * @param binary_with_hamming
 * @param binary
 * @param bin
 */
static void convert_eight_to_twelve(uint8_t *binary_with_hamming, const uint8_t *binary, size_t ou) {
//    for (int i = 1; i <= 12; i++) {
//        double rem = remainder(log(i), log(2));
//        double newRem;
//        if (rem < 0) { newRem = -(rem); }
//        else { newRem = rem; }
//
//        if (newRem < 0.0000001) {
//            binary_with_hamming[i - 1] = 0;
//        } else {
//            binary_with_hamming[i - 1] = binary[(*bin)++];
//        }
//    }

    binary_with_hamming[0] = 0;
    binary_with_hamming[1] = 0;
    binary_with_hamming[2] = binary[ou++];
    binary_with_hamming[3] = 0;
    binary_with_hamming[4] = binary[ou++];
    binary_with_hamming[5] = binary[ou++];
    binary_with_hamming[6] = binary[ou++];
    binary_with_hamming[7] = 0;
    binary_with_hamming[8] = binary[ou++];
    binary_with_hamming[9] = binary[ou++];
    binary_with_hamming[10] = binary[ou++];
    binary_with_hamming[11] = binary[ou++];

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
    size_t nread;
    int ret_val;
    uint8_t bin = 0;
    char chars[BUF_SIZE];
    //uint8_t i, k = 8;
    size_t i;
    uint8_t binary_with_hamming[12];
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

    write_message(parity, prefix);

    nread = ((size_t) ((dc_read(env, err, STDIN_FILENO, chars, BUF_SIZE)) - 1));
    printf("nread: %zu\n", nread);
    printf("chars: %s\n", chars);
    uint8_t binary[8 * nread];

    //filling binary array with zeros
    for (i = 0; i < (8 * nread); i++) {
        binary[i] = 0;
    }

    //converting each char and filling the binary array
    convert_to_binary(binary, nread, chars);
    printf("(8 * nread): %lu\n\n", (8 * nread));
    int fadi = 0;
    int gggg = 0;
//    for (i = 0; i < (8 * nread); i++) {
//        if (fadi == 8) {
//            //printf("\n");
//            fadi = 0;
//        }
//        fadi++;
//        if (gggg == 80) {
//            //printf("\n");
//            gggg = 0;
//        }
//        gggg++;
//        //printf("%d ", binary[i]);
//    }
//    for (size_t kk = 0; kk < 8; kk++) {
//        printf("%d  {%zu} ", binary[(size_t) (kk + bin)], (size_t) (kk + bin));
//    }

    // Filling array of 12 with zeros
    for (i = 0; i < 12; i++) {
        binary_with_hamming[i] = 0;
    }

    //Generating files names
    size_t len = dc_strlen(env, prefix);
    char pathname[12][len + 11];
    for (i = 0; i < 12; i++) {
        snprintf(pathname[i], len + 11, "%s%zu.hamming", prefix, i);
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
    int looooooop2 = 0;
    printf("\n");
    for (size_t ou = 1; ou <= (8 * nread); ou += 8) {


        printf("bin: %zu\n", ou - 1);
        //convert 8 to 12
        for (i = 0; i < 8; i++) {
            printf("%d", binary[i + ou - 1]);
        }
        convert_eight_to_twelve(binary_with_hamming, binary, ou - 1);

        printf("\n");
        for (size_t kk = 0; kk < 12; kk++) {
            printf("%d", binary_with_hamming[kk]);
        }

        //adding hamming code
        addingHam(binary_with_hamming, hammingParity);

        printf("\n");
        if (looooooop2 == 10) {
            printf("\n");
            looooooop2 = 0;
        }
        looooooop2++;
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
        byte0 = (uint8_t) ((byte0 << 1) | (binary_with_hamming[0] >> 0));
        byte1 = (uint8_t) ((byte1 << 1) | (binary_with_hamming[1] >> 0));
        byte2 = (uint8_t) ((byte2 << 1) | (binary_with_hamming[2] >> 0));
        byte3 = (uint8_t) ((byte3 << 1) | (binary_with_hamming[3] >> 0));
        byte4 = (uint8_t) ((byte4 << 1) | (binary_with_hamming[4] >> 0));
        byte5 = (uint8_t) ((byte5 << 1) | (binary_with_hamming[5] >> 0));
        byte6 = (uint8_t) ((byte6 << 1) | (binary_with_hamming[6] >> 0));
        byte7 = (uint8_t) ((byte7 << 1) | (binary_with_hamming[7] >> 0));
        byte8 = (uint8_t) ((byte8 << 1) | (binary_with_hamming[8] >> 0));
        byte9 = (uint8_t) ((byte9 << 1) | (binary_with_hamming[9] >> 0));
        byte10 = (uint8_t) ((byte10 << 1) | (binary_with_hamming[10] >> 0));
        byte11 = (uint8_t) ((byte11 << 1) | (binary_with_hamming[11] >> 0));
//        printf("%d %d %d %d %d %d %d %d %d %d %d %d\n", byte0, byte1, byte2, byte3, byte4, byte5, byte6, byte7, byte8,
//               byte9, byte10, byte11);
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
    return ret_val;
}

/**
 * writing a message
 * @param parity
 * @param file_name
 */
static void write_message(const char *parity, const char *file_name) {
    printf("You chose (%s) parity bit, and the prefix for files is: %s\n", parity, file_name);
}

/**
 *
 * @param exit_code
 */
static void usage(int exit_code) {
    fprintf(stderr, "Usage: --parity <string> --prefix <path-to-output-prefix>");
    exit(1);
}
