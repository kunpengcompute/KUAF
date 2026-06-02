# Kunpeng Unified Acceleration Framework

## Latest Updates

- \[2025.12.30\]: This is the second official release. Supports RSA and SM2 algorithms.
- \[2025.06.30\]: This is the first official release.

## Project Introduction

### Overview

Developed by Kunpeng, Kunpeng Unified Acceleration Framework (KUAF) enhances the collaboration between the Kunpeng Accelerator Engine (KAE) hardware and software libraries for faster compression, encryption, and decryption.

KUAF can schedule KAE hardware accelerators to accelerate zlib compression/decompression and OpenSSL encryption/decryption. [**Table 1** KUAF scheduling policies](#KUAF-scheduling-policies) lists the scheduling policies supported by KUAF.

**Table 1** KUAF scheduling policies<a id="KUAF-scheduling-policies"></a>

|No.|Policy|Description|
|--|--|--|
|1|Hardware bandwidth-based scheduling|Collects hardware bandwidth usage and switches to software libraries when the hardware bandwidth usage reaches the upper limit.|
|2|Software-hardware resource ratio-based scheduling|Configures a software-hardware resource ratio and randomly implements software computing or hardware computing based on the ratio.|

### Supported Algorithms

- Encryption and decryption
    - Asymmetric algorithm RSA, operating in synchronous mode
    - Asymmetric algorithm SM2, operating in synchronous mode

- zlib
    - Deflate/inflate interfaces in the open source zlib library

## Directory Structure

The full project directory structure is as follows:

```text
├── docs                                                     # Project document directory
│   └── en                                                   # English document directory
│       ├── figures                                          # Directory of figures in documents
│       ├── quick_start.md                                   # Quick Start
│       ├── release_notes.md                                 # KUAF Release Notes
│       ├── installation_guide.md                            # KUAF Installation Guide
│       ├── user_guide.md                                    # KUAF User Guide
│       ├── api_reference.md                                 # KUAF API Reference
├── adapter                                                  # KUAF adaptation layer
│   ├── kuaf_comp                                            # decompression adaptation layer
│   ├── kuaf_crypto                                          # encryption and decryption adaptation layer
└── config                                                   # Configuration file
├── open_source/patch                                        # Patch file
├── package                                                  # Packaging script
├── src                                                      # KUAF source code
│   ├── common                                               # Source code for the configuration file and log printing
│   ├── core                                                 # Scheduling source code
├── test                                                     # Test code
│   ├── functest                                             # Function test
│   ├── fuzztest                                             # Fuzz test
│   ├── perftest                                             # Performance test
│   ├── gtest-download                                       # Script for downloading the test framework
├── LICENSE                                                  # Project license
└── README_EN.md                                             # Project description document
└── build.sh                                                 # Build script
└── env.check.sh                                             # Environment check script
```

## Release Notes

For details about feature changes in each released version, see [Release Notes](docs/en/release_notes.md).

## Quick Start

The quick start guide of KUAF provides instructions for quick installation and API usage reference. For details, see *Quick Start*.

## Documents

|Name|Description|
|--|--|
|[Release Notes](docs/en/release_notes.md)|Provides version mapping information about KUAF.|
|[Installation Guide](docs/en/installation_guide.md)|Provides detailed instructions for installing KUAF by compiling the source code and installing the RPM package.|
|[API Reference](docs/en/api_reference.md)|Provides KUAF API descriptions and API calling examples.|
|[Quick Start](docs/en/quick_start.md)|Provides KUAF configuration examples.|

## Disclaimer

**To KUAF users**

- This software is intended solely for debugging and development. You are responsible for any risks and should carefully review the following information:
    - This code repository contributes to the OpenSSL and zlib open-source projects solely for performance optimization. It strictly adheres to the coding style and methods, as well as security design of the native open-source software. Any vulnerability and security issues of the software shall be resolved by the corresponding upstream communities according to their response mechanisms. Please pay attention to the notifications and version updates released by the upstream communities. The Kunpeng computing community does not assume any responsibility for software vulnerabilities and security issues.
    - Data processing and deletion: Users are responsible for managing and deleting any data generated while using this software. Users are advised to delete such data promptly after use to prevent information leakage.
    - Data confidentiality and transmission: Users understand and agree not to share or transmit any data generated by this software. Neither the software nor its developers are responsible for any information leakage, data breaches, or other negative consequences.
    - User input security: Users are responsible for the security of any commands they enter and for any risks or losses resulting from improper input. The software and its developers are not liable for issues caused by incorrect command usage.

- Disclaimer scope: This disclaimer applies to all individuals and entities using this software. By using the software, you acknowledge and accept this statement and assume all risks and responsibilities arising from its use. If you do not agree, please stop using the software immediately.
- Before using this software, please **read and understand the preceding disclaimer**. If you have any questions, contact the developer.

**To data owners**

If you do not want your dataset to be mentioned in KUAF, or if you wish to update its description, please submit an issue on GitCode. We will delete or update your description according to your request. Thank you for your understanding and contribution to KUAF.

## License

For details about the license for using KUAF, see [LICENSE](https://gitcode.com/boostkit/KUAF/blob/master/LICENSE).

The documents in the KUAF **docs** directory are licensed under CC-BY 4.0. For details, see [LICENSE](docs/LICENSE).

## Contribution Statement

If you have any questions or want to provide feedback on feature requirements and bug reports, you can submit issues. For details, see the [contribution guideline](https://gitcode.com/boostkit/community/blob/master/docs/contributor/contributing.md).

1. Submitting an error report: If you find a non-security vulnerability in KUAF, first search the **Issues** in the KUAF repository to avoid submitting duplicates. If the vulnerability is not listed, create a new issue. If you discover a security-related issue, do not disclose it publicly. Please refer to the security handling guidelines for details. All error reports must include complete information about the issue.
2. Handling security issues: For guidance on handling security issues in this project, please contact the core team via email for instructions.
3. Resolving existing issues: Review the issue list of the repository to identify issues that need attention, and attempt to resolve them.
4. Proposing new features: Use the **Feature** label when creating an issue for a new feature. We will review and confirm proposals periodically.
5. How to contribute:
    1. Fork the repository of the project.
    2. Clone it to your local machine.
    3. Create a development branch.
    4. Conduct local testing. All unit tests, including any new test cases, must pass before submission.
    5. Submit your code.
    6. Create a pull request (PR).
    7. Code review: Modify the code according to review comments and resubmit your changes. This process may involve multiple rounds of iterations.
    8. After your PR is approved by the required number of reviewers, the committer will conduct the final review.
    9. After your PR is approved and all tests pass, the CI system will merge it into the project's main branch.

## Suggestions and Feedback

You are welcome to contribute to the community. If you have any questions or suggestions, submit [issues](https://gitcode.com/boostkit/community/blob/master/docs/contributor/issue-submit.md). We will reply to you as soon as possible. Thank you for your support.

## Acknowledgments

KUAF is jointly developed by the following Huawei department:

- Kunpeng Computing BoostKit Development Dept

Thank you to everyone in the community for your PRs. We warmly welcome contributions to KUAF!
