# CodeGuard Security Rules Catalog

This document details the security rules implemented in CodeGuard v1.0.

---

## Rule Summary

| Rule ID | Rule Name | Severity | Target Operation | Description |
|---|---|---|---|---|
| **STR001** | Unsafe `strcpy()` | `HIGH` | `strcpy()` | Buffer overflow caused by unbounded string copy or length exceeding destination |
| **STR002** | Unsafe `gets()` | `HIGH` | `gets()` | Inherently unsafe input function without buffer boundary constraints |
| **STR003** | Unsafe `sprintf()` | `HIGH` | `sprintf()` | Buffer overflow caused by unbounded formatted string output |
| **STR004** | Unterminated `strncpy()` | `HIGH` | `strncpy()` | Missing null terminator when copy length reaches destination buffer size |
| **MEM001** | Unsafe `memcpy()` | `HIGH` | `memcpy()` | Buffer overflow caused by byte copy count exceeding destination buffer capacity |
| **MEM002** | Unsafe `memmove()` | `MEDIUM` | `memmove()` | Suspicious memory operation where byte count exceeds source or destination bounds |
| **MEM003** | Potential Use-After-Free | `HIGH` | `free()`, `delete` | Pointer accessed or dereferenced after memory has been deallocated |
| **INT001** | Integer Overflow | `MEDIUM` | `malloc()`, `new[]` | Dynamic integer multiplication or arithmetic in memory allocation size |
| **CMD001** | Command Injection | `HIGH` | `system()`, `popen()` | Dynamic or untrusted user input passed directly to command interpreter |
| **PTR001** | Null Pointer Dereference | `HIGH` | `*ptr`, `ptr->field` | Dereferencing a pointer initialized to null without reassignment or null-check |

---

## Detailed Rule Specifications

### 1. STR001 – Unsafe `strcpy()` Buffer Overflow

- **Severity:** HIGH
- **CWE:** CWE-120 (Buffer Copy without Checking Size of Input)
- **Description:** `strcpy()` does not verify whether the destination array can hold the source string, leading to stack or heap memory corruption. CodeGuard detects direct size mismatches, tainted data flow from input sources, and cross-function argument overflows.

#### Vulnerable Example:
```cpp
char buffer[4];
strcpy(buffer, "Hello"); // Source is 6 bytes (including null); buffer is only 4
```

#### Safe Example:
```cpp
char buffer[10];
strcpy(buffer, "Hello"); // Safe: 6 bytes fits in 10 bytes
```
*Remediation:* Use bounded operations like `strncpy()`, `strlcpy()`, or C++ `std::string`.

---

### 2. STR002 – Unsafe `gets()` Function

- **Severity:** HIGH
- **CWE:** CWE-242 (Use of Inherently Dangerous Function)
- **Description:** `gets()` reads stdin until a newline is found without checking destination buffer size. It is impossible to use `gets()` safely.

#### Vulnerable Example:
```cpp
char buffer[64];
gets(buffer); // Unbounded read from stdin
```

#### Safe Example:
```cpp
char buffer[64];
fgets(buffer, sizeof(buffer), stdin); // Bounded read
```
*Remediation:* Replace `gets()` with `fgets()` or `std::getline()`.

---

### 3. STR003 – Unsafe `sprintf()` Buffer Overflow

- **Severity:** HIGH
- **CWE:** CWE-134 / CWE-120
- **Description:** `sprintf()` writes formatted output to a memory buffer without specifying a maximum length. If formatted arguments exceed buffer capacity, memory corruption occurs.

#### Vulnerable Example:
```cpp
char buffer[5];
sprintf(buffer, "%s", "Hello"); // Output length 6 exceeds capacity 5
```

#### Safe Example:
```cpp
char buffer[64];
snprintf(buffer, sizeof(buffer), "%s", "Hello");
```
*Remediation:* Replace `sprintf()` with `snprintf()`, supplying the destination buffer size.

---

### 4. STR004 – Unterminated `strncpy()`

- **Severity:** HIGH
- **CWE:** CWE-170 (Improper Null-Termination)
- **Description:** If the source string length is greater than or equal to the count parameter, `strncpy()` copies characters without appending a null terminator byte. Subsequent string reads will read past the buffer.

#### Vulnerable Example:
```cpp
char buffer[10];
strncpy(buffer, "HelloWorld", 10); // No null terminator added!
```

#### Safe Example:
```cpp
char buffer[10];
strncpy(buffer, "Hello", sizeof(buffer) - 1);
buffer[sizeof(buffer) - 1] = '\0';
```
*Remediation:* Always explicitly set the last byte of the buffer to `'\0'` or use `std::string`.

---

### 5. MEM001 – Unsafe `memcpy()`

- **Severity:** HIGH
- **CWE:** CWE-120 (Buffer Overflow via memcpy)
- **Description:** `memcpy()` copies $N$ bytes from source to destination without boundary checks. If $N$ exceeds the destination capacity, memory past the destination buffer is overwritten.

#### Vulnerable Example:
```cpp
char dest[5];
char src[20] = "Sample data";
memcpy(dest, src, 10); // 10 bytes copied into 5-byte buffer
```

#### Safe Example:
```cpp
char dest[20];
memcpy(dest, src, 10); // Safe: 10 <= 20
```
*Remediation:* Ensure copy count does not exceed destination capacity, or use bounds-checked functions.

---

### 6. MEM002 – Unsafe `memmove()` Bounds

- **Severity:** MEDIUM
- **CWE:** CWE-125 / CWE-787
- **Description:** Detects cases where the byte count supplied to `memmove()` exceeds the allocated capacity of the destination or source buffer.

#### Vulnerable Example:
```cpp
char dest[8];
memmove(dest, src, 16); // Count 16 exceeds destination capacity 8
```

#### Safe Example:
```cpp
char dest[20];
memmove(dest, src, 10); // Safe
```
*Remediation:* Validate that the byte count parameter is less than or equal to both source and destination bounds.

---

### 7. MEM003 – Potential Use-After-Free

- **Severity:** HIGH
- **CWE:** CWE-416 (Use After Free)
- **Description:** Accessing memory after deallocation via `free()` or `delete` can result in undefined behavior, heap corruption, and arbitrary code execution.

#### Vulnerable Example:
```cpp
char* ptr = (char*)malloc(32);
free(ptr);
*ptr = 'A'; // Dereferencing deallocated memory!
```

#### Safe Example:
```cpp
char* ptr = (char*)malloc(32);
free(ptr);
ptr = nullptr; // Pointer nulled out immediately
```
*Remediation:* Set pointers to `nullptr` immediately after freeing, or use RAII smart pointers (`std::unique_ptr`).

---

### 8. INT001 – Risky Integer Arithmetic in Allocation

- **Severity:** MEDIUM
- **CWE:** CWE-190 (Integer Overflow or Wraparound)
- **Description:** Arithmetic expressions (multiplication/addition) in memory allocation parameters (e.g. `malloc(n * sizeof(int))`) can wrap around 32-bit/64-bit integer limits, resulting in a tiny allocation followed by an out-of-bounds heap write.

#### Vulnerable Example:
```cpp
int count;
cin >> count;
char* buffer = (char*)malloc(count * sizeof(int)); // Overflow possible if count is large
```

#### Safe Example:
```cpp
size_t count = 10;
char* buffer = (char*)malloc(count * sizeof(int)); // Small constant bound
```
*Remediation:* Check for integer overflow (e.g. `__builtin_mul_overflow` or `std::numeric_limits`) before calculating allocation sizes.

---

### 9. CMD001 – Potential Command Injection

- **Severity:** HIGH
- **CWE:** CWE-78 (OS Command Injection)
- **Description:** Passing dynamic strings, unvalidated user input, or non-literal expressions to `system()` or `popen()` allows attackers to execute arbitrary shell commands.

#### Vulnerable Example:
```cpp
char* cmd = argv[1];
system(cmd); // Command injection vector
```

#### Safe Example:
```cpp
system("ls -la"); // Safe constant literal
```
*Remediation:* Avoid invoking shell interpreters. Use `execve()` with an explicit argument array, or validate against an exact whitelist.

---

### 10. PTR001 – Potential Null Pointer Dereference

- **Severity:** HIGH
- **CWE:** CWE-476 (NULL Pointer Dereference)
- **Description:** Dereferencing a pointer that was initialized to `nullptr` / `NULL` without verifying that it points to valid memory or reassigning it leads to program crashes and denial of service.

#### Vulnerable Example:
```cpp
int* ptr = nullptr;
*ptr = 42; // Crash!
```

#### Safe Example:
```cpp
int* ptr = nullptr;
int value = 42;
ptr = &value;
*ptr = 42; // Valid address assigned
```
*Remediation:* Verify that pointers are not `nullptr` before dereferencing, or initialize pointers directly with valid memory.

