#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>

#define STUDENT_ID "001"
enum { TEMP = 1, PRESSURE = 2, BATTERY = 3 };
enum { NORMAL = 0, ALERT = 1 };

typedef union {
    float temperature;
    int pressure;
    float voltage;
} Reading;

typedef struct Device {
    int id, type, status;
    char name[40];
    Reading reading;
    double previous, last_alert_value;
    int has_previous;
    size_t samples, alerts, drifts;
    char last_alert[170];
} Device;

typedef struct {
    Device *devices;
    size_t count, total, normal, abnormal, by_type[3], drift_alerts;
} Monitor;

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
int number(const char *message, int *value) {
    char s[80], *end;
    if (!input(message, s, sizeof s)) return 0;
    errno = 0;
    long n = strtol(s, &end, 10);
    while (isspace((unsigned char)*end)) end++;
    if (errno || s == end || *end || n < INT_MIN || n > INT_MAX) {
        puts("Invalid whole number."); return 0;
    }
    *value = (int)n; return 1;
}
const char *type_name(int type) {
    return type == TEMP ? "Temperature" : type == PRESSURE ? "Pressure" : "Battery";
}
const char *unit(int type) {
    return type == TEMP ? "C" : type == PRESSURE ? "kPa" : "V";
}
double value(Device *d) { /* Read only the active union member. */
    if (d->type == TEMP) return d->reading.temperature;
    if (d->type == PRESSURE) return d->reading.pressure;
    return d->reading.voltage;
}
void alert(Device *d, const char *callback, const char *range, const char *reason) {
    d->status = ALERT;
    d->last_alert_value = value(d);
    snprintf(d->last_alert, sizeof d->last_alert, "Range: %s | Reason: %s | Callback: %s",
             range, reason, callback);
    printf("ALERT | ID:%d | %s | %.2f %s | %s\n",
           d->id, d->name, value(d), unit(d->type), d->last_alert);
}
void temperature_monitor(Device *d) {
    if (d->reading.temperature < 15 || d->reading.temperature > 35)
        alert(d, "temperature_monitor", "15 to 35 C", "temperature outside range");
}
void pressure_monitor(Device *d) {
    if (d->reading.pressure < 90 || d->reading.pressure > 110)
        alert(d, "pressure_monitor", "90 to 110 kPa", "pressure outside range");
}
void battery_monitor(Device *d) {
    if (d->reading.voltage < 3.3 || d->reading.voltage > 4.2)
        alert(d, "battery_monitor", "3.30 to 4.20 V", "battery voltage outside range");
}
/* Extra callback: detect a large change since this device's previous reading.
   Thresholds: 12 C, 16 kPa, or 0.65 V. */
void drift_detector(Device *d) {
    double limit = d->type == TEMP ? 12 : d->type == PRESSURE ? 16 : 0.65;
    if (d->has_previous && fabs(value(d) - d->previous) > limit) {
        const char *range = d->type == TEMP ? "change <= 12 C" :
                            d->type == PRESSURE ? "change <= 16 kPa" : "change <= 0.65 V";
        alert(d, "drift_detector", range, "reading changed too quickly");
        d->drifts++;
    }
}
void process_device(Device *d, void (*callback)(Device *)) { callback(d); }
int position(Monitor *m, int id) {
    for (size_t i = 0; i < m->count; i++) if (m->devices[i].id == id) return (int)i;
    return -1;
}
void register_device(Monitor *m) {
    Device d = {0};
    if (!number("Device ID: ", &d.id)) return;
    if (d.id <= 0 || position(m, d.id) >= 0) { puts("ID must be positive and unique."); return; }
    if (!input("Name: ", d.name, sizeof d.name) || !d.name[0]) return;
    if (!number("Type 1=temperature 2=pressure 3=battery: ", &d.type)) return;
    if (d.type < 1 || d.type > 3) { puts("Invalid device type."); return; }
    Device *p = realloc(m->devices, (m->count + 1) * sizeof *p);
    if (!p) { puts("Memory allocation failed."); return; }
    m->devices = p; m->devices[m->count++] = d;
    printf("Registered. Devices: %zu\n", m->count);
}
void display(Monitor *m) {
    if (!m->count) { puts("No devices."); return; }
    for (Device *d = m->devices; d < m->devices + m->count; d++)
        printf("%d | %-18.18s | %-11s | %s | Samples: %zu\n",
               d->id, d->name, type_name(d->type),
               !d->samples ? "NEW" : d->status ? "ALERT" : "NORMAL", d->samples);
}
void remove_device(Monitor *m) {
    int id;
    if (!number("ID to remove: ", &id)) return;
    int i = position(m, id);
    if (i < 0) { puts("Device not found."); return; }
    for (size_t j = (size_t)i; j + 1 < m->count; j++) m->devices[j] = m->devices[j + 1];
    m->count--;
    if (!m->count) { free(m->devices); m->devices = NULL; }
    else {
        Device *p = realloc(m->devices, m->count * sizeof *p);
        if (p) m->devices = p; /* failed shrinking retains valid records */
    }
    printf("Removed. Devices: %zu\n", m->count);
}
int random_between(int low, int high) { return low + rand() % (high - low + 1); }
void generate(Device *d) {
    /* Documented generation ranges:
       temperature -5..50 C, pressure 75..125 kPa, battery 2.80..4.60 V */
    if (d->type == TEMP) d->reading.temperature = random_between(-50, 500) / 10.0f;
    else if (d->type == PRESSURE) d->reading.pressure = random_between(75, 125);
    else d->reading.voltage = random_between(280, 460) / 100.0f;
}
void simulate(Monitor *m) {
    int n;
    if (!m->count) { puts("Register a device first."); return; }
    if (!number("Number of readings (10-1000): ", &n)) return;
    if (n < 10 || n > 1000) { puts("Choose 10 to 1000 readings."); return; }
    void (*callbacks[])(Device *) = {temperature_monitor, pressure_monitor, battery_monitor};
    puts("Ranges: Temp 15..35 C; Pressure 90..110 kPa; Battery 3.30..4.20 V");
    for (int i = 0; i < n; i++) {
        Device *d = m->devices + i % m->count;
        generate(d);
        d->status = NORMAL;
        printf("Reading %d | ID:%d | %s | %s | %.2f %s\n",
               i + 1, d->id, d->name, type_name(d->type), value(d), unit(d->type));
        const char *callback = d->type == TEMP ? "temperature_monitor" :
                               d->type == PRESSURE ? "pressure_monitor" : "battery_monitor";
        printf("  Callback: %s\n", callback);
        process_device(d, callbacks[d->type - 1]);
        size_t old_drifts = d->drifts;
        process_device(d, drift_detector);
        m->drift_alerts += d->drifts - old_drifts;
        d->previous = value(d);
        d->has_previous = 1;
        d->samples++; m->total++;
        if (d->status == ALERT) {
            m->abnormal++; m->by_type[d->type - 1]++; d->alerts++;
            puts("  Result: ABNORMAL");
        } else { m->normal++; puts("  Result: NORMAL"); }
    }
}
void show_alerts(Monitor *m) {
    int found = 0;
    for (Device *d = m->devices; d < m->devices + m->count; d++) {
        if (!d->alerts) continue;
        found = 1;
        printf("ID:%d | %s | Alerts:%zu | Drift alerts:%zu\n",
               d->id, d->name, d->alerts, d->drifts);
        printf("  Most recent alert: %.2f %s | %s\n",
               d->last_alert_value, unit(d->type), d->last_alert);
    }
    if (!found) puts("No alerts. Full alert details are printed during simulation.");
}
void summary(Monitor *m) {
    printf("\nSUMMARY | Registered devices: %zu | Readings: %zu\n", m->count, m->total);
    printf("Normal: %zu | Abnormal: %zu\n", m->normal, m->abnormal);
    printf("Alerts by type: Temp=%zu Pressure=%zu Battery=%zu\n",
           m->by_type[0], m->by_type[1], m->by_type[2]);
    printf("Custom callback - Sensor Drift Detector: %zu detections\n", m->drift_alerts);
    puts("One reading counts once as abnormal even if both callbacks trigger.");
}
int main(void) {
    Device initial[] = {
        {.id=301, .name="Room Temperature", .type=TEMP},
        {.id=302, .name="Water Pressure", .type=PRESSURE},
        {.id=303, .name="Battery Station", .type=BATTERY}
    };
    Monitor m = {0};
    m.count = sizeof initial / sizeof initial[0];
    m.devices = malloc(sizeof initial);
    if (!m.devices) { puts("Memory allocation failed."); return 1; }
    memcpy(m.devices, initial, sizeof initial);
    srand((unsigned)time(NULL));
    void (*operations[])(Monitor *) = {register_device, display, remove_device,
                                       simulate, show_alerts, summary};
    printf("Student ID: %s\nProject: Dynamic IoT Event Monitoring and Callback Engine\n", STUDENT_ID);
    puts("Custom callback: Sensor Drift Detector");
    for (;;) {
        int choice;
        puts("\n1 Register  2 Display  3 Remove  4 Simulate  5 Alerts  6 Summary  0 Exit");
        if (!number("Choice: ", &choice)) { if (feof(stdin)) break; continue; }
        if (choice == 0) break;
        if (choice >= 1 && choice <= 6) operations[choice - 1](&m);
        else puts("Invalid choice.");
    }
    free(m.devices);
    return 0;
}
