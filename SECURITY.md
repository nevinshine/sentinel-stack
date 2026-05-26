# Security Policy

The Sentinel Stack team takes security seriously. This document outlines our security reporting guidelines, supported versions, and vulnerability disclosure timeline.

## Supported Versions

Only the latest active release branches receive security updates. Security patches are backported to the `stable` branch for a limited time.

| Version | Supported          | Notes                                      |
| ------- | ------------------ | ------------------------------------------ |
| v1.0.x  | :white_check_mark: | Current Release Candidate                  |
| v0.9.x  | :x:                | Deprecated. Upgrade to v1.0.x recommended. |
| < v0.9  | :x:                | Unsupported                                |

## Reporting a Vulnerability

If you have discovered a security vulnerability in any of the Sentinel Stack components (including `telos-runtime`, `hyperion-xdp`, `telos-lang`, `sentinel-vmi`, or `sentinel-kv`), **do not open a public issue.**

Please report the vulnerability by emailing our security team at **security@sentinel-stack.local**. 

### PGP Key

For sensitive disclosures, please encrypt your email using our PGP key:
```
Key ID: 0x9A4F3B2C1E7D8055
Fingerprint: 1234 5678 9ABC DEF0 1234 5678 9A4F 3B2C 1E7D 8055
```

### What to Include

When reporting an issue, please provide:
* The affected component(s) and version(s).
* A detailed description of the vulnerability and its potential impact.
* Steps to reproduce the vulnerability (proof-of-concept scripts or network captures are highly appreciated).
* Any potential mitigations or suggested fixes.

## Vulnerability Classification (CVSS)

We classify all reported vulnerabilities using the Common Vulnerability Scoring System (CVSS v3.1):

* **Critical (9.0 - 10.0):** Remote Code Execution, Authentication Bypass, or full Sandbox Escape.
* **High (7.0 - 8.9):** Privilege Escalation, Denial of Service (wire-speed), or significant Information Disclosure.
* **Medium (4.0 - 6.9):** Partial evasion of eBPF enforcement or limited Information Disclosure.
* **Low (0.1 - 3.9):** Theoretical vulnerabilities or issues requiring complex, improbable prerequisites.

## Coordinated Disclosure Timeline

We follow a strict **90-day coordinated disclosure policy**:

1. **Acknowledgment:** We will acknowledge receipt of your report within 48 hours.
2. **Triage:** We will triage the issue, assess its CVSS severity, and notify you of our findings within 7 days.
3. **Patching:** We will work to develop and test a patch within 90 days.
4. **Disclosure:** Once the patch is released, we will publish a security advisory and acknowledge your contribution (with your permission). If we require more than 90 days due to architectural complexities (e.g., Ring -1 NPT fixes), we will negotiate an extension with the reporter.

We ask that you maintain strict confidentiality until the patch is publicly available and the coordinated disclosure date is reached.
