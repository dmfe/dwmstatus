/*
 * Copy me if you can.
 * by 20h
 */

#define _BSD_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <X11/Xlib.h>

char *tz_moscow = "Europe/Moscow";

static Display *dpy;

char *
smprintf(char *fmt, ...)
{
    va_list fmtargs;
    char *ret;
    int len;

    va_start(fmtargs, fmt);
    len = vsnprintf(NULL, 0, fmt, fmtargs);
    va_end(fmtargs);

    ret = malloc(++len);
    if (ret == NULL) {
        perror("malloc");
        exit(1);
    }

    va_start(fmtargs, fmt);
    vsnprintf(ret, len, fmt, fmtargs);
    va_end(fmtargs);

    return ret;
}

void
settz(char *tzname)
{
    setenv("TZ", tzname, 1);
}

char *
mktimes(char *fmt, char *tzname)
{
    char buf[129];
    time_t tim;
    struct tm *timtm;

    settz(tzname);
    tim = time(NULL);
    timtm = localtime(&tim);
    if (timtm == NULL)
        return smprintf("");

    if (!strftime(buf, sizeof(buf)-1, fmt, timtm)) {
        fprintf(stderr, "strftime == 0\n");
        return smprintf("");
    }

    char* icon = "";

    return smprintf("%s%s", icon, buf);
}

void
setstatus(char *str)
{
    XStoreName(dpy, DefaultRootWindow(dpy), str);
    XSync(dpy, False);
}

char *
readfile(char *base, char *file)
{
    char *path, line[513];
    FILE *fd;

    memset(line, 0, sizeof(line));

    path = smprintf("%s/%s", base, file);
    fd = fopen(path, "r");
    free(path);
    if (fd == NULL)
        return NULL;

    if (fgets(line, sizeof(line)-1, fd) == NULL)
        return NULL;
    fclose(fd);

    return smprintf("%s", line);
}

char *
getbattery(char *base)
{
    char *co, *status;
    int descap, remcap;

    descap = -1;
    remcap = -1;

    co = readfile(base, "present");
    if (co == NULL)
        return smprintf("");
    if (co[0] != '1') {
        free(co);
        return smprintf("not present");
    }
    free(co);

    co = readfile(base, "charge_full");
    if (co == NULL) {
        co = readfile(base, "energy_full");
        if (co == NULL)
            return smprintf("");
    }
    sscanf(co, "%d", &descap);
    free(co);

    co = readfile(base, "charge_now");
    if (co == NULL) {
        co = readfile(base, "energy_now");
        if (co == NULL)
            return smprintf("");
    }
    sscanf(co, "%d", &remcap);
    free(co);

    if (remcap < 0 || descap < 0)
        return smprintf("invalid");

    float capacity = ((float)remcap / (float)descap) * 100;
    char* bat_icon;
    if (capacity >= 90.0 && capacity <= 100.0) {
        bat_icon = "";
    } else if (capacity >= 60.0 && capacity < 90.0) {
        bat_icon = "";
    } else if (capacity >= 30.0 && capacity < 60.0) {
        bat_icon = "";
    } else if (capacity >= 25.0 && capacity < 30.0) {
        bat_icon = "";
    } else {
        bat_icon = "";
    }

    co = readfile(base, "status");
    if (!strncmp(co, "Discharging", 11)) {
        status = bat_icon;
    } else if (!strncmp(co, "Charging", 8)) {
        status = "";
    } else if (!strncmp(co, "Full", 4)) {
        status = "";
    } else {
        status = "";
    }

    return smprintf("%s%.0f%%",status, capacity);
}

int
get_sensor_temp(char *base, char *sensor)
{
    char *co;

    co = readfile(base, sensor);
    if (co == NULL)
        return -1;

    return atoi(co) / 1000;
}

char *
get_cpu_temp() {

    int cur_t, crit_t;
    char *icon;

    cur_t = get_sensor_temp("/sys/devices/virtual/thermal/thermal_zone0/hwmon1", "temp1_input");
    crit_t = get_sensor_temp("/sys/devices/virtual/thermal/thermal_zone0/hwmon1", "temp1_crit");

    float ratio = ((float)cur_t / (float)crit_t) * 100;

    if (ratio >= 90.0 && ratio <= 100.0) {
        icon = "";
    } else if (ratio >= 60.0 && ratio < 90.0) {
        icon = "";
    } else if (ratio >= 30.0 && ratio < 60.0) {
        icon = "";
    } else if (ratio >= 25.0 && ratio < 30.0) {
        icon = "";
    } else if (ratio >= 0.0 && ratio < 25.0 ) {
        icon = "";
    } else {
        icon = "";
    }

    return smprintf("%s%d°C", icon, cur_t);
}

int
main(void)
{
    char *status;
    char *bat;
    char *tm_msk;
    char *cpu_temp;

    if (!(dpy = XOpenDisplay(NULL))) {
        fprintf(stderr, "dwmstatus: cannot open display.\n");
        return 1;
    }

    for (;;sleep(60)) {
        bat = getbattery("/sys/class/power_supply/BAT0");
        tm_msk= mktimes("%a %d %b %Y %H:%M", tz_moscow);
        cpu_temp = get_cpu_temp();

        status = smprintf(" %s  %s  %s ", cpu_temp, bat, tm_msk);
        setstatus(status);

        free(cpu_temp);
        free(bat);
        free(tm_msk);
        free(status);
    }

    XCloseDisplay(dpy);

    return 0;
}

