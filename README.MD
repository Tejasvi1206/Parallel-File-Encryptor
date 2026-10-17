![Build Status](https://github.com/Tejasvi1206/Parallel-File-Encryptor/actions/workflows/build.yml/badge.svg)

# File Encryption/Decryption Tool

This project is a simple command-line based file encryption and decryption tool built in C++.  
It processes all regular files inside a given directory (recursively) and performs the selected
action — encrypt or decrypt — on each file.

---

## 🔧 Features

- Works with a directory and all its sub-directories
- Encrypts or decrypts files based on user input
- Uses filesystem library to iterate through files
- Uses a task-based design with process management

---

## 📌 How to Use

1. Run the compiled program
   make clean
   make
   ./encrypt_decrypt
2. Enter the directory path: test
3. Enter the action (encrypt/decrypt): encrypt

And you are good to go...