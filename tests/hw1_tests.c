#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <string.h>  // You may use this here in the test cases, but not elsewhere.
#include <math.h>
#include "const.h"

#include "misc.h"

Test(basecode_tests_suite, validargs_help_test) {
    int argc = 2;
    char *argv[] = {"bin/dtmf", "-h", NULL};
    int ret = validargs(argc, argv);
    int exp_ret = 0;
    int opt = global_options;
    int flag = 0x1;
    cr_assert_eq(ret, exp_ret, "Invalid return for valid args.  Got: %d | Expected: %d",
                 ret, exp_ret);
    cr_assert_eq(opt & flag, flag, "Correct bit (0x1) not set for -h. Got: %x", opt);
}

Test(basecode_tests_suite, validargs_generate_test) {
    char *nfile = "noise.au";
    int argc = 4;
    char *argv[] = {"bin/dtmf", "-g", "-n", nfile, NULL};
    int ret = validargs(argc, argv);
    int exp_ret = 0;
    int opt = global_options;
    int flag = 0x2;
    cr_assert_eq(ret, exp_ret, "Invalid return for valid args.  Got: %d | Expected: %d",
                 ret, exp_ret);
    cr_assert(opt & flag, "Generate mode bit wasn't set. Got: %x", opt);
    cr_assert_eq(strcmp(noise_file, nfile), 0,
                 "Variable 'noise_file' was not properly set.  Got: %s | Expected: %s",
                 noise_file, nfile);
}

Test(basecode_tests_suite, validargs_detect_test) {
    int argc = 4;
    char *argv[] = {"bin/dtmf", "-d", "-b", "10", NULL};
    int exp_ret = 0;
    int exp_size = 10;
    int ret = validargs(argc, argv);
    cr_assert_eq(ret, exp_ret, "Invalid return for valid args.  Got: %d | Expected: %d",
                 ret, exp_ret);
    cr_assert_eq(block_size, exp_size, "Block size not properly set. Got: %d | Expected: %d",
                 block_size, exp_size);
}

Test(basecode_tests_suite, validargs_error_test) {
    int argc = 4;
    char *argv[] = {"bin/dtmf", "-g", "-b", "10", NULL};
    int exp_ret = -1;
    int ret = validargs(argc, argv);
    cr_assert_eq(ret, exp_ret, "Invalid return for valid args.  Got: %d | Expected: %d",
                 ret, exp_ret);
}

Test(basecode_tests_suite, help_system_test) {
    char *cmd = "bin/dtmf -h";

    // system is a syscall defined in stdlib.h
    // it takes a shell command as a string and runs it
    // we use WEXITSTATUS to get the return code from the run
    // use 'man 3 system' to find out more
    int return_code = WEXITSTATUS(system(cmd));

    cr_assert_eq(return_code, EXIT_SUCCESS,
                 "Program exited with %d instead of EXIT_SUCCESS",
                 return_code);
}

Test(basecode_tests_suite, audio_read_header_test) {
    FILE *fp;
    AUDIO_HEADER header;

    int return_code = -1;

    fp = fopen("rsrc/white_noise_10s.au", "r");
    if (fp) {
        return_code = audio_read_header(fp, &header);
        fclose(fp);
    }
    cr_assert_eq(header.magic_number, AUDIO_MAGIC);
    //printf("data_offset: %u\n", header.data_offset );
    //printf("data_size: %u\n", header.data_size );
    cr_assert_eq(header.encoding, PCM16_ENCODING);
    cr_assert_eq(header.sample_rate, AUDIO_FRAME_RATE);
    cr_assert_eq(header.channels, AUDIO_CHANNELS);
    cr_assert_eq(return_code, EXIT_SUCCESS);

}

Test(basecode_tests_suite, goertzel_basic_test) {
    int N = 100;
    int k = 1;
    GOERTZEL_STATE g0, g1, g2;
    goertzel_init(&g0, N, 0);
    goertzel_init(&g1, N, 1);
    goertzel_init(&g2, N, 2);
    double x;
    for (int i = 0; i < N - 1; i++) {
        x = cos(2 * M_PI * i / N);
        goertzel_step(&g0, x);
        goertzel_step(&g1, x);
        goertzel_step(&g2, x);
    }
    x = cos(2 * M_PI * (N - 1) / N);
    double r0 = goertzel_strength(&g0, x);
    double r1 = goertzel_strength(&g1, x);
    double r2 = goertzel_strength(&g2, x);
    double eps = 1e-6;
    cr_assert((fabs(r0) < eps), "r0 was %f, should be 0.0", r0);
    cr_assert((fabs(r1 - 0.5) < eps), "r1 was %f, should be 0.5", r1);
    cr_assert((fabs(r2) < eps), "r2 was %f, should be 0.0", r2);
}

Test(basecode_tests_suite, goertzel_advanced_test) {
    FILE *fp;
    int N = 1000;
    double k = 0;
    k = (double) 697 / 8000 * N;
    GOERTZEL_STATE g0, g1, g2, g3, g4, g5, g6, g7;
    goertzel_init(&g0, N, k);
    k = (double) 770 / 8000 * N;
    goertzel_init(&g1, N, k);
    k = (double) 852 / 8000 * N;
    goertzel_init(&g2, N, k);
    k = (double) 941 / 8000 * N;
    goertzel_init(&g3, N, k);
    k = (double) 1209 / 8000 * N;
    goertzel_init(&g4, N, k);
    k = (double) 1336 / 8000 * N;
    goertzel_init(&g5, N, k);
    k = (double) 1477 / 8000 * N;
    goertzel_init(&g6, N, k);
    k = (double) 1633 / 8000 * N;
    goertzel_init(&g7, N, k);
    int16_t x;
    fp = fopen("rsrc/dtmf_0_500ms.au", "r");
    cr_assert((fp != 0));
    fseek(fp, 24, SEEK_SET);
    for (int i = 0; i < N - 1; i++) {
        cr_assert_eq(audio_read_sample(fp, &x), 0);
        goertzel_step(&g0, (double) x / INT16_MAX);
        goertzel_step(&g1, (double) x / INT16_MAX);
        goertzel_step(&g2, (double) x / INT16_MAX);
        goertzel_step(&g3, (double) x / INT16_MAX);
        goertzel_step(&g4, (double) x / INT16_MAX);
        goertzel_step(&g5, (double) x / INT16_MAX);
        goertzel_step(&g6, (double) x / INT16_MAX);
        goertzel_step(&g7, (double) x / INT16_MAX);
    }
    cr_assert_eq(audio_read_sample(fp, &x), 0);
    fclose(fp);
    double r0 = goertzel_strength(&g0, (double) x / INT16_MAX);
    double r1 = goertzel_strength(&g1, (double) x / INT16_MAX);
    double r2 = goertzel_strength(&g2, (double) x / INT16_MAX);
    double r3 = goertzel_strength(&g3, (double) x / INT16_MAX);
    double r4 = goertzel_strength(&g4, (double) x / INT16_MAX);
    double r5 = goertzel_strength(&g5, (double) x / INT16_MAX);
    double r6 = goertzel_strength(&g6, (double) x / INT16_MAX);
    double r7 = goertzel_strength(&g7, (double) x / INT16_MAX);
    double eps = 1e-6;
    cr_assert((fabs(r0 - 0.000003) < eps), "r0 was %f, should be 0.000003", r0);
    cr_assert((fabs(r1 - 0.000007) < eps), "r1 was %f, should be 0.000007", r1);
    cr_assert((fabs(r2 - 0.000007) < eps), "r2 was %f, should be 0.000007", r2);
    cr_assert((fabs(r3 - 0.031544) < eps), "r3 was %f, should be 0.031544", r3);
    cr_assert((fabs(r4 - 0.000002) < eps), "r4 was %f, should be 0.000002", r4);
    cr_assert((fabs(r5 - 0.031479) < eps), "r5 was %f, should be 0.031479", r5);
    cr_assert((fabs(r6 - 0.000009) < eps), "r6 was %f, should be 0.000009", r6);
    cr_assert((fabs(r7 - 0.000001) < eps), "r7 was %f, should be 0.000001", r7);
}

Test(basecode_tests_suite, getNextDTMFEvent) {
    FILE *fp;
    uint32_t start = 0, end = 0;
    int symbol;
    fp = fopen("rsrc/dtmf_all.txt", "r");
    cr_assert(getNextDTMFEvent(fp, &start, &end, &symbol) == 0);
    cr_assert(getNextDTMFEvent(fp, &start, &end, &symbol) == 0);
    /*printf("%u\n", start);
    printf("%u\n", end);
    printf("%d\n", symbol);*/
    cr_assert(getNextDTMFEvent(fp, &start, &end, &symbol) == 0);

    fclose(fp);
}