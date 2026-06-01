# Quick Start

This document describes the functions and configuration procedures of the Kunpeng Unified Acceleration Framework (KUAF), helping you quickly get started with this framework.

## Procedure

Please refer to the [Installation Guide](./installation_guide.md) to complete the installation of the KUAF before use.

## Configuration

### Configure the configuration file of KUAF

The default settings for the configuration file **/etc/kuaf/kuaf.cnf** are as follows. **log_level** indicates the log level, which can be set to **none**, **debug**, **error**, **warning**, or **info**. **expired_interval** indicates the data expiration time of the hardware bandwidth-based scheduling, in milliseconds. **update_interval** indicates the interval for the daemon thread of the hardware bandwidth-based scheduling to update data, in milliseconds.

```bash
[KUAF_Section]
log_level=none
expired_interval=500
update_interval=50
```

### Configure the configuration file of KAE

The default settings for the configuration file **/etc/kuaf/kae.cnf** are as follows. **ZLIB-DEFLATE** is the scheduling configuration item for zlib compression, and **ZLIB-INFLATE** is the scheduling configuration item for zlib decompression. **scheduler** indicates the scheduling policy, which can be set to **bandwidth** or **ratio**. The value of the bandwidth-based policy indicates the bandwidth allocated to hardware computing. When the hardware bandwidth usage reaches the upper limit, software computing is used. The value of the ratio-based policy indicates the proportion of hardware computing.

```bash
[ZLIB-DEFLATE]
scheduler=bandwidth
bandwidth=3072

[ZLIB-INFLATE]
scheduler=ratio
ratio=60%
```

## Testing

KUAF provides a performance test tool to preliminarily evaluate the zlib/crypto performance.

### Decompression

1. Enter the test directory.

   ```bash
   cd test/perftest/zlib_perf
   make
   ```

2. Run the following test command.

   - Compression: Start 63 child processes, each performing 1000 compression tasks. Each task initializes a new stream, and the input data is from itemadata.

      ```bash
      taskset -c 0-63 ./zip_perf -m 63 -n 1000 -i -f itemdata
      ```

   - Decompression: Start 63 child processes, each running for 10 seconds to perform decompression tasks. Each task initializes a new stream, and the input data is from itemadata.gz.

     ```bash
     taskset -c 0-63 ./zip_perf -m 63 -t 10 -i -f itemdata.gz -d
     ```

   Parameter description:
    - -m: multi process
    - -l: stream length(KB)
    - -w: windowBits
    - -v: compress level(1~9)
    - -f: input  filename(-l useless if this work)
    - -o: output filename
    - -n: loop times
    - -t: run seconds
    - -i: initialize a new compression stream for each task
    - -d: compress or decompress

### Encryption and decryption

1. Enter the test directory.

   ```bash
   cd test/perftest/crypto_perf
   make
   ```

2. Run the following test command.

   * Test the RSA algorithm

     ```bash
     ./rsa_perf
     ```

   * Test the SM2 algorithm

     ```bash
     ./sm2_perf   
     ```
