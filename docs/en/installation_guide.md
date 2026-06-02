# Installation Guide

## Verified Environments

To use KUAF smoothly and securely, ensure that your environment is one of the verified environments.

|OS|CPU|Compiler|
|--|--|--|
|openEuler 22.03 LTS SP3|Kunpeng 920 series processors|GCC 10.3.1|

>**Notice**:
>Currently, KUAF supports only the Kunpeng 920 series processors.

## Installation KUAF

Install KAE first, and then install KUAF as follows. After the installation is complete, run the specified script to build the zlib library. During this process, the source code is downloaded from GitHub. Ensure that the network connection is normal.

**Installing KUAF<a name="section68113416198"></a>**

1. Install KAE. For details, see [Kunpeng Accelerator Engine User Guide](https://www.hikunpeng.com/document/detail/en/kunpengaccel/kae/usermanual/kunpengaccel_16_0002.html).

2. Download the KUAF source package, copy it to a custom directory, and decompress it.

    ```bash
    git clone https://gitcode.com/boostkit/KUAF.git
    ```

3. Install all modules using a script.

    ```bash
    cd KUAF
    sh build.sh all
    ```

4. Enable KUAF by configuring the OpenSSL configuration file **openssl.cnf**.

    Create **openssl.cnf** and add the following configuration information to the file:

    ```bash
    openssl_conf=openssl_def 
    [openssl_def]
    engines=engine_section 
    [engine_section] 
    kae=kae_section 
    [kae_section] 
    engine_id=kae
    dynamic_path=/usr/local/kuaf/lib/libkae.so 
    default_algorithms=ALL 
    init=1
    ```

    Set the **OPENSSL\_CONF** environment variable.

    ```bash
    export OPENSSL_CONF=/home/app/openssl.cnf  # Path for storing the **openssl.cnf** file
    ```

**Verifying the Installation<a name="section5820418197"></a>**

1. Set the environment variable.

    ```bash
    export LD_LIBRARY_PATH=/usr/local/kuaf/lib:/usr/local/lib:/usr/local/lib/engines-1.1$LD_LIBRARY_PATH
    ```

2. Check .so files in the **/usr/local/kuaf/lib** directory.

    ```bash
    ll /usr/local/kuaf/lib
    ```

    If **libkuaf.so**, **libz\_sw.so**, **libkaezip.so**, and **libkae.so** are displayed in the command output, the installation is successful.

    ```text
    total 1664
    -rwxr-xr-x. 1 root root   1250 Nov  6 19:24 libkae.la
    -rwxr-xr-x. 1 root root 364432 Nov  6 19:24 libkae.so
    lrwxrwxrwx. 1 root root     18 Nov  5 15:03 libkaezip.so -> libkaezip.so.2.0.4
    lrwxrwxrwx. 1 root root     18 Nov  5 15:03 libkaezip.so.0 -> libkaezip.so.2.0.4
    -rwxr-xr-x. 1 root root 215640 Nov  5 15:03 libkaezip.so.2.0.4
    -rw-r--r--. 1 root root 604300 Nov  5 15:32 libkuaf.a
    -rwxr-xr-x. 1 root root   1121 Nov  5 15:32 libkuaf.la
    lrwxrwxrwx. 1 root root     16 Nov  5 15:32 libkuaf.so -> libkuaf.so.1.0.0
    lrwxrwxrwx. 1 root root     16 Nov  5 15:32 libkuaf.so.1 -> libkuaf.so.1.0.0
    -rwxr-xr-x. 1 root root 346496 Nov  5 15:32 libkuaf.so.1.0.0
    lrwxrwxrwx. 1 root root     34 Nov  5 15:03 libz.so -> /usr/local/kuaf/lib/libz.so.1.2.11
    lrwxrwxrwx. 1 root root     34 Nov  5 15:03 libz.so.1 -> /usr/local/kuaf/lib/libz.so.1.2.11
    -rwxr-xr-x. 1 root root  67696 Nov  5 15:03 libz.so.1.2.11
    -rw-r--r--. 1 root root 141674 Nov  5 15:03 libz_sw.a
    lrwxrwxrwx. 1 root root     17 Nov  5 15:03 libz_sw.so -> libz_sw.so.1.2.11
    lrwxrwxrwx. 1 root root     17 Nov  5 15:03 libz_sw.so.1 -> libz_sw.so.1.2.11
    -rwxr-xr-x. 1 root root 143904 Nov  5 15:03 libz_sw.so.1.2.11
    ```

    Related files are generated in the installation path (**/usr/local/kuaf** by default). The **include** folder contains the header files of KUAF, and the **lib** folder contains the dynamic library files of KUAF.

## Uninstalling KUAF

If you do not require KUAF, uninstall it. During uninstallation, service flows that are being executed are affected. You are advised to stop the service flows before uninstalling KUAF.

1. Run the script to uninstall KUAF.

    ```bash
    sh build.sh cleanup
    ```

2. Verify that the **/usr/local/kuaf** directory is deleted.

    ```bash
    ll /usr/local/kuaf
    ```

    If the command output indicates that the directory cannot be found, the installation directory is deleted successfully.

3. Unset the **OPENSSL\_CONF** environment variable.

    ```bash
    unset OPENSSL_CONF
    ```
