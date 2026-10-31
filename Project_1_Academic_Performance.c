#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>

#define STUDENT_ID "003"
#define PASS 60.0

typedef struct {
    int id;
    char name[60], course[60];
    double grade;
} Student;

typedef struct {
    Student *list;
    size_t count;
} Database;

/* Read bounded strings and reject excess characters. */
int input(const char *message, char *s, size_t size) {
    printf("%s", message);
    if (!fgets(s, (int)size, stdin)) return 0;
    size_t n = strlen(s);
    if (n && s[n - 1] == '\n') s[n - 1] = '\0';
    else if (!feof(stdin)) {
        int c;
        while ((c = getchar()) != '\n' && c != EOF) {}
        puts("Input too long.");
        return 0;
    }
    return 1;
}
int number(const char *message, int *value) {
    char s[80], *end;
    if (!input(message, s, sizeof s)) return 0;
    errno = 0;
    long n = strtol(s, &end, 10);
    while (isspace((unsigned char)*end)) end++;
    if (errno || s == end || *end || n < INT_MIN || n > INT_MAX) {
        puts("Invalid whole number."); return 0;
    }
    *value = (int)n;
    return 1;
}
int position(Database *db, int id) {
    for (size_t i = 0; i < db->count; i++)
        if (db->list[i].id == id) return (int)i;
    return -1;
}
void heading(void) { puts("ID     Name                    Course                 Grade"); }
void display(Database *db) {
    if (!db->count) { puts("No students."); return; }
    heading();
    for (Student *p = db->list; p < db->list + db->count; p++)
        printf("%-6d %-23.23s %-22.22s %5.1f\n", p->id, p->name, p->course, p->grade);
}
void add(Database *db) {
    Student s = {0}; char grade[80], *end;
    if (!number("New ID: ", &s.id)) return;
    if (s.id <= 0 || position(db, s.id) != -1) { puts("ID must be positive and unique."); return; }
    if (!input("Full name: ", s.name, sizeof s.name) || !s.name[0]) return;
    if (!input("Course: ", s.course, sizeof s.course) || !s.course[0]) return;
    if (!input("Grade (0-100): ", grade, sizeof grade)) return;
    errno = 0;
    s.grade = strtod(grade, &end);
    while (isspace((unsigned char)*end)) end++;
    if (errno || grade == end || *end || !(s.grade >= 0 && s.grade <= 100)) {
        puts("Invalid grade. Enter 0 to 100."); return;
    }
    Student *temp = realloc(db->list, (db->count + 1) * sizeof *temp);
    if (!temp) { puts("Memory allocation failed."); return; }
    db->list = temp;
    db->list[db->count++] = s;
    printf("Student added. Total: %zu\n", db->count);
}
int grade_order(const void *a, const void *b) {
    double x = ((const Student *)a)->grade, y = ((const Student *)b)->grade;
    return (x < y) - (x > y); /* highest first */
}
int name_order(const void *a, const void *b) {
    return strcmp(((const Student *)a)->name, ((const Student *)b)->name);
}
int id_order(const void *a, const void *b) {
    int x = ((const Student *)a)->id, y = ((const Student *)b)->id;
    return (x > y) - (x < y);
}
void sort(Database *db) {
    int option;
    if (!db->count) { puts("No students."); return; }
    if (!number("Sort: 1=grade, 2=name, 3=ID: ", &option)) return;
    int (*compare)(const void *, const void *) = NULL;
    if (option == 1) compare = grade_order;
    if (option == 2) compare = name_order;
    if (option == 3) compare = id_order;
    if (!compare) { puts("Invalid choice."); return; }
    qsort(db->list, db->count, sizeof(Student), compare);
    display(db);
}
double average(Database *db) {
    if (!db->count) return 0;
    double sum = 0;
    for (Student *p = db->list; p < db->list + db->count; p++) sum += p->grade;
    return sum / db->count;
}
void show_average(Database *db) {
    if (db->count) printf("Average: %.2f\n", average(db));
    else puts("No students.");
}
void top(Database *db) {
    if (!db->count) { puts("No students."); return; }
    Student *best = db->list;
    for (Student *p = db->list + 1; p < db->list + db->count; p++)
        if (p->grade > best->grade) best = p;
    printf("Top student: %s (ID %d), %.1f\n", best->name, best->id, best->grade);
}
void search(Database *db) {
    int option;
    if (!number("Search 1=ID, 2=exact name: ", &option)) return;
    if (option == 1) {
        int id;
        if (!number("ID: ", &id)) return;
        int i = position(db, id);
        if (i >= 0) printf("Found: %d | %s | %s | %.1f\n", db->list[i].id,
                           db->list[i].name, db->list[i].course, db->list[i].grade);
        else puts("Not found.");
    } else if (option == 2) {
        char name[60]; int found = 0;
        if (!input("Full name: ", name, sizeof name)) return;
        for (size_t i = 0; i < db->count; i++)
            if (!strcmp(db->list[i].name, name)) {
                printf("Found: %d | %s | %.1f\n", db->list[i].id, name, db->list[i].grade);
                found = 1;
            }
        if (!found) puts("Not found.");
    } else puts("Invalid choice.");
}
void remove_student(Database *db) {
    int id;
    if (!number("ID to remove: ", &id)) return;
    int i = position(db, id);
    if (i < 0) { puts("Not found."); return; }
    for (size_t j = (size_t)i; j + 1 < db->count; j++) db->list[j] = db->list[j + 1];
    db->count--;
    if (!db->count) { free(db->list); db->list = NULL; }
    else {
        Student *temp = realloc(db->list, db->count * sizeof *temp);
        if (temp) db->list = temp; /* failed shrinking is harmless */
    }
    printf("Removed. Total: %zu\n", db->count);
}
/* Original analysis: risk = max(0, (60 - grade) / 60 * 100).
   Low: 0; Moderate: >0 and <25; High: >=25. */
double risk(double grade) { return grade >= PASS ? 0 : (PASS - grade) / PASS * 100; }
const char *level(double r) { return r == 0 ? "Low" : r < 25 ? "Moderate" : "High"; }
void analyze(Database *db) {
    if (!db->count) { puts("No students."); return; }
    puts("Academic Risk Index (pass mark 60):");
    for (Student *p = db->list; p < db->list + db->count; p++)
        printf("%d %-23.23s Grade:%5.1f Risk:%5.1f%% %s\n",
               p->id, p->name, p->grade, risk(p->grade), level(risk(p->grade)));
}
void summary(Database *db) {
    printf("\nSUMMARY | Students: %zu\n", db->count);
    if (!db->count) return;
    double low = db->list[0].grade, high = low;
    size_t passed = 0, moderate = 0, high_risk = 0;
    for (Student *p = db->list; p < db->list + db->count; p++) {
        if (p->grade < low) low = p->grade;
        if (p->grade > high) high = p->grade;
        if (p->grade >= PASS) passed++;
        double r = risk(p->grade);
        if (r >= 25) high_risk++;
        else if (r > 0) moderate++;
    }
    printf("Average: %.2f | Highest: %.1f | Lowest: %.1f\n", average(db), high, low);
    printf("Passed (>=60): %zu | Below pass: %zu\n", passed, db->count - passed);
    printf("Risk groups: Low=%zu Moderate=%zu High=%zu\n",
           db->count - moderate - high_risk, moderate, high_risk);
}
int main(void) {
    Student initial[] = {
        {2101,"Ada Okoro","Algorithms",91}, {2102,"Ben Musa","Databases",68},
        {2103,"Cora Ibe","Networks",54}, {2104,"Dayo Ali","Security",78},
        {2105,"Eno Obi","Algorithms",45}, {2106,"Fola James","Networks",84},
        {2107,"Gani Bell","Databases",59}, {2108,"Hauwa Amin","Security",72},
        {2109,"Ife Uche","Algorithms",95}, {2110,"Jide Bello","Networks",33},
        {2111,"Kemi Lawal","Databases",63}, {2112,"Lami Eze","Security",88}
    };
    Database db = {0};
    db.count = sizeof initial / sizeof initial[0];
    db.list = malloc(db.count * sizeof(Student));
    if (!db.list) { puts("Memory allocation failed."); return 1; }
    memcpy(db.list, initial, sizeof initial);
    void (*operations[])(Database *) = {
        add, display, sort, show_average, top, search, remove_student, analyze, summary
    };
    printf("Student ID: %s\nProject: Academic Performance and Risk Analysis\n", STUDENT_ID);
    puts("Custom analysis: Academic Risk Index");
    for (;;) {
        int choice;
        puts("\n1 Add  2 Display  3 Sort  4 Average  5 Top  6 Search");
        puts("7 Remove  8 Risk analysis  9 Summary  0 Exit");
        if (!number("Choice: ", &choice)) { if (feof(stdin)) break; continue; }
        if (choice == 0) break;
        if (choice >= 1 && choice <= 9) operations[choice - 1](&db);
        else puts("Invalid choice.");
    }
    free(db.list);
    return 0;
}
