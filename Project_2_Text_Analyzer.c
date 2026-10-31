#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdint.h>

#define STUDENT_ID "002"

typedef struct wordStat {
    char name[40];
    int count;
} WordStat;

typedef struct {
    char *text;
    size_t length, capacity;
    WordStat *words;
    size_t distinct, room, total, rejected;
} Analyzer;

/* L. Frank Baum, The Wonderful Wizard of Oz (1900), Chapter 1. */
const char *sample =
"Dorothy lived in the midst of the great Kansas prairies, with Uncle Henry, "
"who was a farmer, and Aunt Em, who was the farmer's wife. Their house was "
"small, for the lumber to build it had to be carried by wagon many miles. "
"There were four walls, a floor and a roof, which made one room; and this "
"room contained a rusty looking cookstove, a cupboard for the dishes, a "
"table, three or four chairs, and the beds. Uncle Henry and Aunt Em had a "
"big bed in one corner, and Dorothy a little bed in another corner. There "
"was no garret at all, and no cellar-except a small hole dug in the ground, "
"called a cyclone cellar, where the family could go in case one of those "
"great whirlwinds arose, mighty enough to crush any building in its path. "
"It was reached by a trap door in the middle of the floor, from which a "
"ladder led down into the small, dark hole.";

int input(const char *message, char *s, size_t cap) {
    printf("%s", message);
    if (!fgets(s, (int)cap, stdin)) return 0;
    size_t n = strlen(s);
    if (n && s[n - 1] == '\n') s[n - 1] = '\0';
    else if (!feof(stdin)) {
        int c; while ((c = getchar()) != '\n' && c != EOF) {}
        puts("Input too long."); return 0;
    }
    return 1;
}
int get_choice(int *choice) {
    char line[40], *end;
    if (!input("Choice: ", line, sizeof line)) return 0;
    errno = 0;
    long n = strtol(line, &end, 10);
    while (isspace((unsigned char)*end)) end++;
    if (errno || end == line || *end || n < 0 || n > 11) { puts("Invalid choice."); return 0; }
    *choice = (int)n; return 1;
}
/* Allocate a growing text buffer instead of imposing a fixed text length. */
int append(Analyzer *a, const char *chunk) {
    size_t n = strlen(chunk);
    if (n > SIZE_MAX - a->length - 1) return 0;
    size_t needed = a->length + n + 1;
    if (needed > a->capacity) {
        size_t size = a->capacity ? a->capacity : 256;
        while (size < needed) {
            if (size > SIZE_MAX / 2) { size = needed; break; }
            size *= 2;
        }
        char *p = realloc(a->text, size);
        if (!p) { puts("Text memory allocation failed."); return 0; }
        a->text = p; a->capacity = size;
    }
    memcpy(a->text + a->length, chunk, n + 1);
    a->length += n;
    return 1;
}
int count_word(Analyzer *a, const char *word) {
    for (size_t i = 0; i < a->distinct; i++) {
        if (!strcmp(a->words[i].name, word)) {
            if (a->words[i].count == 2147483647) return 0;
            a->words[i].count++; a->total++; return 1;
        }
    }
    if (a->distinct == a->room) {
        size_t next = a->room ? a->room * 2 : 16;
        if (next < a->room || next > SIZE_MAX / sizeof(WordStat)) return 0;
        WordStat *p = realloc(a->words, next * sizeof *p);
        if (!p) { puts("Word memory allocation failed."); return 0; }
        a->words = p; a->room = next;
    }
    strcpy(a->words[a->distinct].name, word);
    a->words[a->distinct].count = 1;
    a->distinct++; a->total++; return 1;
}
int letter(unsigned char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
/* Walk through the text using pointer arithmetic. */
int analyze_text(Analyzer *a) {
    if (!a->text) return 1;
    const unsigned char *p = (const unsigned char *)a->text;
    while (*p) {
        while (*p && !letter(*p)) p++;
        if (!*p) break;
        char word[40]; size_t n = 0;
        while (letter(*p)) {
            if (n < 39) word[n] = (char)tolower(*p);
            n++; p++;
        }
        if (n > 39) { a->rejected++; continue; }
        word[n] = '\0';
        if (!count_word(a, word)) return 0;
    }
    return 1;
}
void replace_text(Analyzer *current, Analyzer *next) {
    if (!analyze_text(next)) {
        puts("Analysis failed; previous text kept.");
        free(next->text); free(next->words); return;
    }
    free(current->text); free(current->words);
    *current = *next;
    printf("Text ready: %zu words, %zu distinct, %zu overlong rejected.\n",
           current->total, current->distinct, current->rejected);
}
void load_example(Analyzer *a) {
    Analyzer next = {0};
    if (append(&next, sample)) replace_text(a, &next);
    else free(next.text);
    puts("Source: The Wonderful Wizard of Oz, Chapter 1, L. Frank Baum (1900)");
}
void paste_text(Analyzer *a) {
    Analyzer next = {0}; char line[256];
    puts("Paste text; finish with END on its own line:");
    while (fgets(line, sizeof line, stdin)) {
        if (!strcmp(line, "END\n") || !strcmp(line, "END")) break;
        if (!append(&next, line)) { free(next.text); return; }
    }
    replace_text(a, &next);
}
void word_count(Analyzer *a) {
    printf("Words: %zu | Distinct: %zu | Rejected: %zu\n",
           a->total, a->distinct, a->rejected);
}
WordStat *longest(Analyzer *a) {
    WordStat *best = NULL;
    for (WordStat *p = a->words; p && p < a->words + a->distinct; p++)
        if (!best || strlen(p->name) > strlen(best->name)) best = p;
    return best;
}
WordStat *frequent(Analyzer *a) {
    WordStat *best = NULL;
    for (WordStat *p = a->words; p && p < a->words + a->distinct; p++)
        if (!best || p->count > best->count) best = p;
    return best;
}
void show_longest(Analyzer *a) {
    WordStat *p = longest(a);
    if (p) printf("Longest: %s (%zu letters)\n", p->name, strlen(p->name));
    else puts("No words.");
}
void show_frequent(Analyzer *a) {
    WordStat *p = frequent(a);
    if (p) printf("Most frequent: %s (%d times)\n", p->name, p->count);
    else puts("No words.");
}
void statistics(Analyzer *a) {
    if (!a->distinct) { puts("No words."); return; }
    for (size_t i = 0; i < a->distinct; i++)
        printf("%-39s %d\n", a->words[i].name, a->words[i].count);
}
int alphabetic(const void *x, const void *y) {
    return strcmp(((const WordStat *)x)->name, ((const WordStat *)y)->name);
}
int by_frequency(const void *x, const void *y) {
    const WordStat *a = x, *b = y;
    return a->count == b->count ? strcmp(a->name, b->name) : (a->count < b->count) - (a->count > b->count);
}
void sort_words(Analyzer *a) {
    if (!a->distinct) { puts("No words."); return; }
    char s[10];
    if (!input("Sort 1=alphabet, 2=frequency: ", s, sizeof s)) return;
    if (strcmp(s, "1") && strcmp(s, "2")) { puts("Invalid choice."); return; }
    qsort(a->words, a->distinct, sizeof(WordStat), !strcmp(s, "1") ? alphabetic : by_frequency);
    statistics(a);
}
void search_word(Analyzer *a) {
    char s[80];
    if (!input("Search word: ", s, sizeof s)) return;
    size_t n = strlen(s);
    if (!n || n > 39) { puts("Use 1-39 letters."); return; }
    for (char *p = s; *p; p++) {
        if (!letter((unsigned char)*p)) { puts("Letters only."); return; }
        *p = (char)tolower((unsigned char)*p);
    }
    for (size_t i = 0; i < a->distinct; i++)
        if (!strcmp(a->words[i].name, s)) {
            printf("%s: %d occurrence(s)\n", s, a->words[i].count); return;
        }
    printf("%s: 0 occurrences\n", s);
}
double diversity(Analyzer *a) { return a->total ? 100.0 * a->distinct / a->total : 0; }
void custom(Analyzer *a) {
    printf("Lexical Diversity = distinct / total x 100 = %.2f%%\n", diversity(a));
    puts("Higher values mean a larger share of different words.");
}
void addresses(Analyzer *a) {
    printf("Text buffer (dynamic text): %p\n", (void *)a->text);
    printf("Word array (dynamic statistics): %p\n", (void *)a->words);
    printf("First word entry: %p\n", a->distinct ? (void *)&a->words[0] : NULL);
}
void summary(Analyzer *a) {
    size_t letters = 0;
    for (size_t i = 0; i < a->distinct; i++)
        letters += strlen(a->words[i].name) * (size_t)a->words[i].count;
    printf("\nSUMMARY | Total: %zu | Distinct: %zu\n", a->total, a->distinct);
    printf("Average word length: %.2f\n", a->total ? (double)letters / a->total : 0);
    show_longest(a); show_frequent(a); custom(a);
    printf("Rejected overlong tokens: %zu\n", a->rejected);
}
int main(void) {
    Analyzer a = {0};
    void (*operations[])(Analyzer *) = {
        word_count, show_longest, show_frequent, statistics, sort_words,
        search_word, summary, addresses, custom
    };
    printf("Student ID: %s\nProject: Dynamic Text Mining and Linguistic Pattern Analyzer\n", STUDENT_ID);
    puts("Custom analysis: Lexical Diversity Score");
    load_example(&a);
    for (;;) {
        int choice;
        puts("\n1 Count  2 Longest  3 Frequent  4 Statistics  5 Sort  6 Search");
        puts("7 Summary  8 Addresses  9 Lexical diversity  10 Paste text  11 Reload sample  0 Exit");
        if (!get_choice(&choice)) { if (feof(stdin)) break; continue; }
        if (choice == 0) break;
        if (choice >= 1 && choice <= 9) operations[choice - 1](&a);
        else if (choice == 10) paste_text(&a);
        else if (choice == 11) load_example(&a);
    }
    free(a.text); free(a.words);
    return 0;
}
