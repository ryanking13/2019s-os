#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <sys/time.h>
#include <linux/gps.h>

#define SET_GPS_LOCATION(loc) syscall(398, loc)

int main(int argc, char **argv) {

    if (argc < 6) {
        printf("usage: %s <lat_integer> <lat_fractional> <lng_integer> <lng_fractional> <accuracy>\n", argv[0]);
        return 1;
    }

    struct gps_location loc;

    loc.lat_integer = atoi(argv[1]);
    loc.lng_integer = atoi(argv[3]);
    loc.lat_fractional = atoi(argv[2]);
    loc.lng_fractional = atoi(argv[4]);
    loc.accuracy = atoi(argv[5]);

    int ret = SET_GPS_LOCATION(&loc);

    if (ret < 0) {
        printf("Setting GPS location failed\n");
        printf("Please check arguments\n");
        return 1;
    }

    printf("Successfully set new GPS location\n");
    return 0;
}