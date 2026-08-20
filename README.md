# Clang for c Dev Flake

A nix flake for setting up nix devshels and package with clang standard derivation

## Requirements

- Nix flake enabled on any nix system or system with nix.

- direnv (needed if you want to setup auto startup for flake on entering by the shell).

## How to setup

### Vanilla

- Init flake template.

```bash
nix flake init -t github:PeDro0210/c-clang-base-dev-flake#default
```

- You are all set!

### With direnv

- Install direnv

- Init flake template.

```bash
nix flake init -t github:PeDro0210/c-clang-base-dev-flake#default
```

- Allow direnv.

```bash
direnv allow
```
