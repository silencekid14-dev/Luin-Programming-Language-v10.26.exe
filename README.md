# Luin-Programming-Language-v10.26.exe
Team up and build! Making a Programming Language!. Come help at Xcort Team , link: https://xcortpage.netlify.app/
# Luin Language Compiler 🚀

Welcome to **Luin**, a custom, statically-typed serious programming language with a lightweight compiler written from scratch in pure C. Luin is designed to explore language syntax, automatic type inference, scope management, and automated C code generation and building games and apps.
To run the demo.sx file write: ".\Luin.exe(or whatever the name) demo.sx
Works for Windows only!
I want friends to help me finish it together.
From: Xcort-Team: https://xcortpage.netlify.app/
Version: Luin_v10.26.exe

The compiler reads `.sx` source files, tokenizes the language grammar, infers variable and function types across multiple passes, and transpiles the code into optimization-ready C code compiled on-the-fly via GCC.

## ✨ Features Implemented So Far
* **Custom Lexer/Tokenizer:** Built entirely from scratch handling identifiers, strings, f-strings, numerals, operators, and boolean tokens.
* **Multi-Pass Type Inference:** Resolves types of variables, parameter types from global function calls, and function return types natively.
* **Scope Tracking Stack:** Supports proper isolation between global operations and local function scopes.
* **Clean Syntax:** Supports clean assignments, mathematical expressions, formatted standard output (`show`), interactive reading (`ask`), and clean conditional blocks.

## 🛠️ Code Architecture

The implementation spans nearly 1,000 lines of highly optimized C code, structured across key compilation phases:
1. **Tokenization:** Splits source text into a strong typing system of distinct tokens.
2. **Definition Collection:** Scans files to build global reference points for all defined functions.
3. **Global Type Identification:** Registers variables to prevent type mismatches.
4. **Parameter Inference:** Evaluates parameters dynamically based on calls.
5. **C Codegen Pipeline:** Formats expressions cleanly into an output C file for GCC compilation.

## 🚀 Getting Started

### Prerequisites
* A standard C compiler (like `gcc`) added to your system path.

### Compilation & RUN
Compile the Luin compiler itself using your terminal:
```bash
gcc main.c -o Luin.exe
./Luin demo.sx

