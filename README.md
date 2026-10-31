# Project 2: C Programming Formative

This repository contains three C programs created for Project 2. It covers structures, dynamic memory allocation, pointers, function pointers, string manipulation, unions, and data analysis.

## Project Files

### 1. Academic Performance and Risk Analysis (`Project_1_Academic_Performance.c`)
A student record management system that stores and analyzes academic performance. It allows users to add, remove, search, and sort student records, calculate average grades, identify top-performing students, and generate performance summaries.

* **Key Concepts:** Structures, dynamic memory allocation, pointers, string comparison, and function pointers.
* **Custom Analysis:** Academic Risk Index, which identifies students who may be at academic risk based on their grades.

### 2. Text Mining and Linguistic Pattern Analyzer (`Project_2_Text_Analyzer.c`)
A text analysis program that processes a paragraph and extracts useful word statistics. It counts words, identifies the longest and most frequently used words, searches and sorts words, and generates a text summary.

* **Key Concepts:** Dynamic memory allocation, pointer traversal, string manipulation, and function pointers.
* **Custom Analysis:** Lexical Diversity Score, which measures vocabulary variety by comparing the number of unique words to the total word count.

### 3. IoT Event Monitoring and Callback Engine (`Project_3_IoT_Monitor.c`)
A simulated IoT monitoring system that manages devices and processes sensor readings. It supports temperature, pressure, and battery voltage sensors, detects abnormal readings, and generates alerts and monitoring summaries.

* **Key Concepts:** Structures, unions, dynamic memory allocation, pointers, and callback functions.
* **Custom Analysis:** Sensor Drift Detection, which identifies significant changes between consecutive sensor readings.

## How to Run

### C Programs

You will need a C compiler (like GCC) installed. Run these commands in your terminal:

1. Compile the code: `gcc filename.c -o program`
2. Run the executable (Mac/Linux): `./program`
3. Run the executable (Windows): `.\program.exe`

**Note:** For the IoT monitoring program, use `gcc Project_3_IoT_Monitor.c -o program -lm` to link the math library.

## Project Features

All three programs demonstrate:
* Dynamic memory management using `malloc()`, `calloc()`, or `realloc()`.
* Pointer-based data processing.
* Function pointers for executing different operations.
* Menu-driven interfaces.
* Custom analysis functions.
* Input validation and memory cleanup.
