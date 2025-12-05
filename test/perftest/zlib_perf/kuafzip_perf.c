#include <zlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <wait.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdint.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <sys/file.h>

static uLong read_inputFile(const char *fileName, void **input)
{
    FILE *sourceFile = fopen(fileName, "r");
    if (sourceFile == NULL) {
        fprintf(stderr, "%s not exist!\n", fileName);
        return 0;
    }
    int fd = fileno(sourceFile);
    struct stat fs;
    (void)fstat(fd, &fs);

    uLong input_size = fs.st_size;
    *input = malloc(input_size * sizeof(Bytef));
    if (*input == NULL) {
        return 0;
    }
    (void)fread(*input, 1, input_size, sourceFile);
    fclose(sourceFile);

    return input_size;
}

static size_t write_outputFile(const char *outFileName, void *output, uLong output_size)
{
    FILE *outputFile = fopen(outFileName, "w");
    if (!outputFile) {
        fprintf(stderr, "%s create failed!\n", outFileName);
        return 0;
    }
    int fd = fileno(outputFile);
    if (flock(fd, LOCK_EX) != 0) {
        fclose(outputFile);
        return 0;
    }
    size_t count = fwrite(output, sizeof(Bytef), output_size, outputFile);
    flock(fd, LOCK_UN);
    fclose(outputFile);
    return count;
}

uint8_t *get_compress_input(size_t input_sz)
{
    uint8_t *inbuf = (uint8_t *)malloc(input_sz * sizeof(uint8_t));
    if (inbuf == NULL) {
        return NULL;
    }

    memset(inbuf, 0, input_sz);
    srand((unsigned int)time(NULL));
    int i = 0;
    for (i = 0; i < input_sz; i++) {
        inbuf[i] = (uint8_t)rand() % 254 + 1;
    }

    return inbuf;
}

uint8_t *get_decompress_input(size_t input_sz, uLong *pblen)
{
    uint8_t *inbuf = (uint8_t *)malloc(input_sz * sizeof(uint8_t));
    if (inbuf == NULL) {
        return NULL;
    }

    memset(inbuf, 0, input_sz);
    srand((unsigned int)time(NULL));
    int i = 0;
    for (i = 0; i < input_sz; i++) {
        inbuf[i] = (uint8_t)rand() % 254 + 1;
    }

    uLong blen = compressBound(input_sz);
    uint8_t *outbuf = (uint8_t *)malloc(blen * sizeof(uint8_t));
    memset(outbuf, 0, blen);
    int cret = compress2((Bytef *)outbuf, (uLongf *)&blen, (Bytef *)inbuf, (uLong)input_sz, 1);
    if (cret != Z_OK && cret != Z_BUF_ERROR) {
        free(outbuf);
        outbuf = NULL;
    }

    free(inbuf);
    *pblen = blen;
    return outbuf;
}

int do_multi_perf_loop(int multi, uLong stream_len, int loop_times, int windowBits, int level, int compress,
    void *output, uLong output_sz, void *inbuf, uLong blen, const char *out_filename, int need_init)
{
    int i, j, err;
    int ret = 0;
    pid_t pid_child = 0;
    fflush(stdout);
    fflush(stderr);

    int pipefd[2] = {0};
    pipe(pipefd);

    struct timeval start, stop;
    gettimeofday(&start, NULL);
    for (i = 0; i < multi; i++) {
        pid_child = fork();
        if (pid_child == 0 || pid_child == -1) {
            break;
        }
    }

    if (pid_child == 0) {
        int do_reset = 0;
        z_stream strm;
        strm.zalloc = (alloc_func)0;
        strm.zfree = (free_func)0;
        strm.opaque = (voidpf)0;
        for (j = 0; j < loop_times; j++) {
            strm.next_in = (z_const Bytef *)inbuf;
            strm.next_out = output;
            if (!do_reset && compress) {
                deflateInit2_(&strm, level, Z_DEFLATED, windowBits, 8, Z_DEFAULT_STRATEGY, ZLIB_VERSION, sizeof(z_stream));
            }
            if (!do_reset && !compress) {
                inflateInit2_(&strm, windowBits, ZLIB_VERSION, sizeof(z_stream));
            }
            if (compress) {
                blen = compressBound(stream_len);
                // ret = compress2((Bytef *)output, (uLongf *)&blen, (Bytef *)inbuf, (uLong)stream_len, 1);
                /***********************************************/
                strm.avail_in = stream_len;
                strm.avail_out = blen;
                err = deflate(&strm, Z_FINISH);
                ret = (err == Z_STREAM_END ? Z_OK : err);
                if (j == loop_times - 1) {
                    output_sz = strm.total_out;
                    double compress_rate = 100.0 * output_sz / stream_len;
                    fprintf(stdout,
                        "compress_size is %luB = %.3lfMB, compress_rate is %.3lf%%\n",
                        output_sz,
                        1.0 * output_sz / (1 << 20),
                        compress_rate);
                    if (out_filename) {
                        write_outputFile(out_filename, output, output_sz);
                    }
                }
                if (need_init) {
                    (void)deflateEnd(&strm);
                } else {
                    deflateReset(&strm);
                    do_reset = 1;
                }

                /***********************************************/
                if (ret != Z_OK && ret != Z_BUF_ERROR) {
                    fprintf(stderr, "compres error, ret = %d\n", ret);
                }
            } else {
                // ret = uncompress((Bytef *)output, &output_sz, (const Bytef *)inbuf, blen);
                /***********************************************/
                strm.avail_in = blen;
                strm.avail_out = output_sz;
                err = inflate(&strm, Z_FINISH);
                ret = (err == Z_STREAM_END ? Z_OK : err);
                if (j == loop_times - 1) {
                    output_sz = strm.total_out;
                    fprintf(stdout, "uncompress_size is %luB = %.3lfMB\n", output_sz, 1.0 * output_sz / (1 << 20));

                    close(pipefd[0]);
                    char buffer[32] = {0};
                    sprintf(buffer, "%lu", output_sz);
                    write(pipefd[1], buffer, strlen(buffer) + 1);

                    if (out_filename) {
                        write_outputFile(out_filename, output, output_sz);
                    }
                }
                if (need_init) {
                    (void)inflateEnd(&strm);
                } else {
                    inflateReset(&strm);
                    do_reset = 1;
                }

                /***********************************************/
                if (ret < 0) {
                    printf("uncompres error, ret = %d\n", ret);
                }
            }
        }
    }

    if (pid_child > 0) {
        ret = -1;
        while (1) {
            ret = wait(NULL);
            if (ret == -1) {
                if (errno == EINTR) {
                    continue;
                }
                break;
            }
        }
    }

    if (pid_child > 0 || multi == 0) {
        if (multi == 0) {
            multi = 1;
        }
        gettimeofday(&stop, NULL);
        uLong time1 = (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec;

        close(pipefd[1]);
        char buffer[32] = {0};
        read(pipefd[0], buffer, 32);
        char *endptr;
        output_sz = strtoul(buffer, &endptr, 10);

        stream_len = compress ? stream_len : output_sz;
        float speed1 = 1000000.0 / time1 * loop_times * multi * stream_len / (1 << 30);
        printf("kaezip %s perf result:\n", compress ? "compress" : "decompress");
        printf("     time used: %lu us, speed = %.3f GB/s\n", time1, speed1);
        // ensuring all buffered data is immediately written
        fflush(stdout);
    }

    return ret;
}

int do_multi_perf_second(int multi, uLong stream_len, int duration, int windowBits, int level, int compress,
    void *output, uLong output_sz, void *inbuf, uLong blen, const char *out_filename, int need_init)
{
    int i, err;
    int ret = 0;
    pid_t pid_child = 0;

    struct timeval start_time;
    gettimeofday(&start_time, NULL);

    // 涓烘瘡涓瓙杩涚▼鍒涘缓绠￠亾
    int **pipes = malloc(multi * sizeof(int *));
    for (i = 0; i < multi; i++) {
        pipes[i] = malloc(2 * sizeof(int));
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    for (i = 0; i < multi; i++) {
        pid_child = fork();
        if (pid_child < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid_child == 0) {
            // 瀛愯繘绋嬶細鍏抽棴璇荤锛屾墽琛屽帇缂?瑙ｅ帇
            close(pipes[i][0]);

            struct timeval child_start;
            gettimeofday(&child_start, NULL);

            uLong total_output = 0;
            int do_reset = 0;
            z_stream strm;
            strm.zalloc = (alloc_func)0;
            strm.zfree = (free_func)0;
            strm.opaque = (voidpf)0;
            while (1) {
                if (!do_reset && compress) {
                    deflateInit2_(&strm, level, Z_DEFLATED, windowBits, 8, Z_DEFAULT_STRATEGY, ZLIB_VERSION, sizeof(z_stream));
                }
                if (!do_reset && !compress) {
                    inflateInit2_(&strm, windowBits, ZLIB_VERSION, sizeof(z_stream));
                }

                struct timeval current;
                gettimeofday(&current, NULL);
                long elapsed = (current.tv_sec - child_start.tv_sec) * 1000000 + current.tv_usec - child_start.tv_usec;
                if (elapsed >= duration * 1000000) {
                    if (compress) {
                        deflateEnd(&strm);
                    } else {
                        inflateEnd(&strm);
                    }
                    break;
                }
                strm.next_in = (z_const Bytef *)inbuf;
                strm.next_out = output;

                if (compress) {
                    strm.avail_in = stream_len;
                    strm.avail_out = blen;
                    err = deflate(&strm, Z_FINISH);
                    if (err == Z_STREAM_END) {
                        total_output += stream_len;
                        if (need_init) {
                            (void)deflateEnd(&strm);
                        } else {
                            deflateReset(&strm);
                            do_reset = 1;
                        }
                    }
                } else {
                    strm.avail_in = blen;
                    strm.avail_out = output_sz;
                    err = inflate(&strm, Z_FINISH);
                    if (err == Z_STREAM_END) {
                        total_output += strm.total_out;
                        // printf("total output: %lu \n",total_output);
                        if (need_init) {
                            (void)inflateEnd(&strm);
                        } else {
                            inflateReset(&strm);
                            do_reset = 1;
                        }
                    }
                }
            }
            write(pipes[i][1], &total_output, sizeof(total_output));
            close(pipes[i][1]);  // 鍏抽棴鍐欑
            exit(EXIT_SUCCESS);
        } else {
            // 鐖惰繘绋嬶細鍏抽棴鍐欑
            close(pipes[i][1]);
        }
    }

    if (pid_child > 0) {
        while (1) {
            ret = wait(NULL);
            if (ret == -1) {
                if (errno == EINTR)
                    continue;
                break;
            }
        }

        uLong total_compressed = 0;
        for (i = 0; i < multi; i++) {
            uLong child_output;
            read(pipes[i][0], &child_output, sizeof(child_output));
            total_compressed += child_output;
            close(pipes[i][0]);  // 鍏抽棴璇荤
            free(pipes[i]);      // 閲婃斁鍐呭瓨
        }
        free(pipes);

        struct timeval stop;
        gettimeofday(&stop, NULL);
        // long total_usec = (stop.tv_sec - start_time.tv_sec) * 1000000L + (stop.tv_usec - start_time.tv_usec);

        // float speed = 1000000.0 / total_usec * total_compressed / (1 << 30);

        float speed = (float)total_compressed / duration / (1 << 30);

        printf("kaezip %s perf result:\n", compress ? "compress" : "decompress");
        printf("     time used: %d s, total compressed: %lu B, speed = %.3f GB/s\n", duration, total_compressed, speed);
    }

    return ret;
}

int do_compress_perf(const char *in_filename, const char *out_filename, int multi, uLong stream_len, int loop_times,
    int time_seconds, int use_loop, int need_init, int windowBits, int level)
{
    void *inbuf = NULL;
    if (in_filename) {
        fprintf(stdout, "compress filename : %s\n", in_filename);
        stream_len = read_inputFile(in_filename, &inbuf);
    } else {
        inbuf = get_compress_input(stream_len);
    }
    if (!inbuf) {
        fprintf(stderr, "inbuf is NULL!\n");
        return -1;
    }
    fprintf(stdout, "input_size is %luB\n", stream_len);

    uLong blen = compressBound(stream_len);
    uLong output_sz = blen;
    void *outbuf = malloc(output_sz * sizeof(uint8_t));
    if (outbuf == NULL) {
        return -1;
    }
    memset(outbuf, 0, output_sz);

    int ret;
    if (use_loop) {
        ret = do_multi_perf_loop(multi,
            stream_len,
            loop_times,
            windowBits,
            level,
            1,
            outbuf,
            output_sz,
            inbuf,
            blen,
            out_filename,
            need_init);
    } else {
        ret = do_multi_perf_second(multi,
            stream_len,
            time_seconds,
            windowBits,
            level,
            1,
            outbuf,
            output_sz,
            inbuf,
            blen,
            out_filename,
            need_init);
    }

    free(inbuf);
    free(outbuf);
    return ret;
}

int do_decompress_perf(const char *in_filename, const char *out_filename, int multi, int stream_len, int loop_times,
    int time_seconds, int use_loop, int need_init, int windowBits, int level)
{
    uLong blen = 0;
    void *inbuf = NULL;
    if (in_filename) {
        fprintf(stdout, "uncompress filename : %s\n", in_filename);
        stream_len = read_inputFile(in_filename, &inbuf);
        blen = stream_len;
    } else {
        inbuf = get_decompress_input(stream_len, &blen);
    }
    if (inbuf == NULL) {
        fprintf(stderr, "inbuf is NULL!\n");
        return -1;
    }
    fprintf(stdout, "input_size is %luB\n", blen);

    uLong output_sz = in_filename ? blen * 8 : stream_len;
    void *output = malloc(output_sz * sizeof(uint8_t));
    if (output == NULL) {
        return -1;
    }
    memset(output, 0, output_sz);

    int ret;
    if (use_loop) {
        ret = do_multi_perf_loop(multi,
            stream_len,
            loop_times,
            windowBits,
            level,
            0,
            output,
            output_sz,
            inbuf,
            blen,
            out_filename,
            need_init);
    } else {
        ret = do_multi_perf_second(multi,
            stream_len,
            time_seconds,
            windowBits,
            level,
            0,
            output,
            output_sz,
            inbuf,
            blen,
            out_filename,
            need_init);
    }

    free(inbuf);
    free(output);
    return ret;
}

void usage(void)
{
    printf("usage: \n");
    printf("  -m: multi process \n");
    printf("  -l: stream length(KB)\n");
    printf("  -w: windowBits\n");
    printf("  -v: compress level(1~9)\n");
    printf("  -f: input  filename(-l useless if this work)\n");
    printf("  -o: output filename\n");
    printf("  -n: loop times\n");
    printf("  -t: run seconds\n");
    printf("  -i: initialize a new compression stream for each task\n");
    printf("  -d: compress or decompress\n");
    printf("  example: ./kaezip_perf -m 2 -l 1024 -n 1000\n");
    printf("           ./kaezip_perf -d -m 2 -l 1024 -n 1000\n");
}

// [root@localhost test]# ./kaezip_perf
// usage:
//   -m: multi process
//   -l: stream length(KB)
//   -n: loop times
//   -d: compress or decompress
//   example: ./kaezip_perf -m 2 -l 1024 -n 1000
//            ./kaezip_perf -d -m 2 -l 1024 -n 1000

// default input parameter used
// kaezip perf input parameter: multi process 2, stream length: 1024(KB), loop times: 1000
// kaezip compress perf result:
//      time used: 509004 us, speed = 4.024 GB/s
//
// [root@localhost test]# ./kaezip_perf -d
// kaezip perf parameter: multi process 2, stream length: 1024(KB), loop times: 1000
// kaezip decompress perf result:
//      time used: 810318 us, speed = 2.527 GB/s

int main(int argc, char **argv)
{
    const char *optstring = "dim:l:n:w:f:o:v:t:h";
    int o = 0;
    int multi = 1;
    int level = 6;
    uLong stream_len = 1024;
    int loop_times = 1000;
    int compress = 1;
    int windowBits = 15;
    char input_filename[128] = {0};
    char output_filename[128] = {0};
    int use_loop = 0;
    int time_seconds = 5;
    int need_init = 0;
    while ((o = getopt(argc, argv, optstring)) != -1) {
        if (optstring == NULL)
            continue;
        switch (o) {
            case 'm':
                multi = atoi(optarg);
                break;
            case 'l':
                stream_len = atoi(optarg);
                break;
            case 'v':
                level = atoi(optarg);
                break;
            case 'n':
                loop_times = atoi(optarg);
                use_loop = 1;
                break;
            case 't':
                time_seconds = atoi(optarg);
                break;
            case 'i':
                need_init = 1;
                break;
            case 'w':
                windowBits = atoi(optarg);
                break;
            case 'd':
                compress = 0;
                break;
            case 'f':
                strcpy(input_filename, optarg);
                break;
            case 'o':
                strcpy(output_filename, optarg);
                break;
            case 'h':
                usage();
                return 0;
        }
    }

    if (argc <= 1) {
        usage();
        printf("\ndefault input parameter used\n");
    }

    printf("kaezip perf parameter: multi process %d, stream length: %lu(KB), loop times: %d, windowBits : %d, level : "
           "%d\n",
        multi,
        stream_len,
        loop_times,
        windowBits,
        level);

    const char *in_filename = input_filename[0] == 0 ? NULL : input_filename;
    const char *out_filename = output_filename[0] == 0 ? NULL : output_filename;
    stream_len *= 1024;
    if (compress) {
        return do_compress_perf(in_filename,
            out_filename,
            multi,
            stream_len,
            loop_times,
            time_seconds,
            use_loop,
            need_init,
            windowBits,
            level);
    } else {
        return do_decompress_perf(in_filename,
            out_filename,
            multi,
            stream_len,
            loop_times,
            time_seconds,
            use_loop,
            need_init,
            windowBits,
            level);
    }
}