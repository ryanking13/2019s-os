#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <sys/time.h>
#include <linux/gps.h>

#define SET_GPS_LOCATION(loc) syscall(398, loc)

int main(int argc, char **argv) {
    struct gps_location loc;

    loc.lat_integer = 0;
    loc.lng_integer = 0;
    loc.lat_fractional = 0;
    loc.lng_fractional = 0;
    loc.accuracy = 0;

    int ret = SET_GPS_LOCATION(&loc);
    printf("ret: %d\n", ret);
}