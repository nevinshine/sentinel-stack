# Contributing to the Sentinel Stack

Thank you for your interest in contributing to the Sentinel Stack! As a kernel-native defense quadrant operating from Ring -3 to Layer 7, contributions require rigorous engineering standards to maintain deterministic security.

## Code of Conduct

By participating in this project, you agree to abide by our Code of Conduct. We expect all contributors to maintain a professional, respectful, and inclusive environment.

## Development Workflow

1. **Fork & Branch:** Fork the repository and create a feature branch (`feat/your-feature` or `fix/your-fix`).
2. **Build Locally:** Ensure you can compile the entire monorepo using `make all` from the repository root.
3. **Run the Tests:** Run `make test` inside the respective component directory (e.g., `cd sentinel-cc && sudo make test`). Do not submit PRs that break existing integration or unit tests.
4. **Commit Formatting:** We use Conventional Commits (e.g., `feat: implement X`, `fix: resolve Y`).
5. **Pull Requests:** Open a Pull Request against the `main` branch. Provide a clear description of the problem solved, the architectural impact, and how it was tested.

## Submitting Issues

If you find a bug or have a feature request, please open an issue with:
- The component affected (e.g., `sentinel-smm`, `telos-runtime`).
- Steps to reproduce.
- Your OS and kernel version.

> [!WARNING]
> **Security Vulnerabilities:** If you discover a security vulnerability, please **DO NOT** open a public issue. See `SECURITY.md` for our coordinated disclosure process.

## Environment Bootstrap

To bootstrap a local development environment:
1. Ensure you have `clang`, `llvm`, `cmake`, `libbpf-dev`, `libelf-dev`, and `golang` installed.
2. Clone the repository and run `make all` to build the daemons.
3. Refer to the individual `README.md` and `docs/` folders inside each component for specific tooling requirements (e.g., Rust/Z3 for `telos-lang`, EDK II for `sentinel-smm`).
