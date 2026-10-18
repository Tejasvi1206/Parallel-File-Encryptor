![Build Status](https://github.com/Tejasvi1206/Parallel-File-Encryptor/actions/workflows/build.yml/badge.svg)

# Parallel File Encryptor

A command-line tool written in C++17 that encrypts or decrypts every file in a directory (including sub-directories) **in parallel**.
You can choose between two concurrency models — **multiprocessing** or **multithreading** — and compare how they work.

---

## 🔧 Features

- Recursively processes every file in a directory
- Two parallel execution modes:
  - **`process`** — forks a child process per file, coordinated through a POSIX shared-memory queue and named semaphores
  - **`thread`** — a thread pool (one worker per CPU core) pulling tasks from a mutex/condition-variable queue
- Strict input validation (only `encrypt` / `decrypt` and `process` / `thread` are accepted)
- Never touches the `.env` key file, so files can always be decrypted
- Prints start and finish timestamps for each file

---

## 📋 Requirements

- Linux or **WSL** on Windows (uses POSIX `fork`, `shm_open`, and semaphores)
- `g++` with C++17 support
- `make`

---

## 🚀 Getting Started

### 1. Clone the repository

```bash
git clone https://github.com/Tejasvi1206/Parallel-File-Encryptor.git
cd Parallel-File-Encryptor
```

### 2. Set your key

The key is read from a `.env` file in the folder you run the program from. Copy the example, then replace the number inside with any whole number of your choice:

```bash
cp .env.example .env
```

> ⚠️ Use the **same key** to decrypt that you used to encrypt. `.env` is git-ignored so your key is never committed.

### 3. Build

```bash
make
```

This produces two executables:

| Executable | Purpose |
|---|---|
| `encrypt_decrypt` | Main program — processes a whole directory |
| `cryption` | Standalone helper that processes a single task (`"path,ENCRYPT"`) |

### 4. (Optional) Create test files

```bash
python3 makeDirs.py
```

This creates a `test/` folder with 10 files of random text.

---

## 📌 Usage

```bash
./encrypt_decrypt
```

Then answer the three prompts:

```
Enter the directory path: test
Enter the action (encrypt/decrypt): encrypt
Enter the mode (process/thread): thread
```

To restore the files, run it again with the **same directory** and `decrypt`.

> ⚠️ Files are modified **in place**. Running `encrypt` twice encrypts twice, so you would need to run `decrypt` twice as well. Try it on the `test/` folder first.

---

## ⚙️ How It Works

1. `main.cpp` walks the directory and creates a task (`"path,ACTION"`) for each file.
2. Each task is handed to the chosen mode (process or thread).
3. A worker runs `executeCryption`, which shifts every byte of the file by the key.

**Process mode** — the parent writes each task into a ring buffer in POSIX shared memory, then `fork()`s a child that takes one task off the queue. Three named semaphores keep it safe: one counts queued items, one counts free slots, and one acts as a lock between processes. The parent waits for all children before cleaning up.

**Thread mode** — a pool of worker threads (one per CPU core) waits on a condition variable. Each worker takes the next task from the queue and processes it; the main thread waits until every task is finished.

---

## 📊 Benchmark

Encrypting **200 MB** (200 files × 1 MB) on a 12-core machine (Linux on WSL2):

| Mode | Workers | Time | Speedup |
|---|---|---|---|
| Sequential | 1 | 1291 s (21.5 min) | 1× |
| `thread` (default) | 12 threads | 199 s | **6.5×** |
| `process` (default) | 200 processes | 144–173 s | **7.4–9.0×** |
| `thread` | 200 threads | 148 s | **8.7×** |

All runs were verified with an encrypt → decrypt round trip: every file matched the original exactly.

**Takeaways**

- Both parallel modes are roughly **7–9× faster** than sequential processing.
- With the **same number of workers**, threads and processes perform about the same. Each task takes seconds, so the extra cost of `fork()` over creating a thread is negligible.
- **More workers than cores helps.** The cipher makes a system call for almost every byte, so workers spend a lot of time waiting; extra workers keep the CPU busy in the meantime.
- It is not 12×, because the work is limited by system calls and disk I/O, not by raw CPU.

> Each configuration was timed once, so expect some run-to-run variation (process mode measured 144 s and 173 s in two runs).

---

## 📁 Project Structure

```
.
├── main.cpp                          # Entry point: prompts, directory walk, mode selection
├── Makefile
├── makeDirs.py                       # Generates sample test files
├── .env.example                      # Template for the key file
├── src/app/
│   ├── encryptDecrypt/
│   │   ├── Cryption.cpp / .hpp       # The encrypt/decrypt routine
│   │   └── CryptionMain.cpp          # Standalone `cryption` executable
│   ├── fileHandling/
│   │   ├── IO.cpp / .hpp             # Opens files for read/write
│   │   └── ReadEnv.cpp               # Reads the key from .env
│   └── processes/
│       ├── Task.hpp                  # Task struct + string (de)serialisation
│       ├── ProcessManagement.cpp/.hpp  # Multiprocessing: shared memory + semaphores
│       └── ThreadPool.cpp / .hpp     # Multithreading: worker thread pool
└── .github/workflows/build.yml       # CI: builds the project on every push
```

---

## ⚠️ Disclaimer

This is a learning project focused on **parallel programming** (processes, threads, shared memory, synchronisation). The cipher is a simple byte shift (Caesar-style) and is **not secure** — do not use it to protect real data.

---

## 🛣️ Roadmap

- [ ] Process files in blocks instead of byte by byte (faster on large files)
- [ ] Mark encrypted files so they are not encrypted twice
- [ ] Limit the number of child processes running at once
- [x] Benchmark process mode vs thread mode
- [ ] Use more threads than CPU cores by default, since the work is I/O-bound
- [ ] Replace the byte shift with real encryption (e.g. AES-256-GCM)
