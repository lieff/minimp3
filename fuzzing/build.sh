AFL_HARDEN=1 afl/afl-clang-fast -fsanitize=address,undefined -fno-sanitize-recover=address,undefined -O2 -o fuzz fuzz.c
