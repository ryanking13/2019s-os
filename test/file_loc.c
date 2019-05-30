#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <sys/time.h>
#include <linux/gps.h>

#define SET_GPS_LOCATION(loc) syscall(398, loc)
#define GET_GPS_LOCATION(path, loc) syscall(399, path, loc)

int main(int argc, char **argv) {

    if (argc < 2) {
        printf("usage: %s <path>\n", argv[0]);
        return 1;
    }

    struct gps_location loc;

    int ret = GET_GPS_LOCATION(argv[1], &loc);

    if (ret < 0) {
        printf("Getting GPS location failed\n");
        printf("Please check arguments\n");
        return 1;
    }

    printf("GPS location information of: %s\n", argv[1]);
    printf("Latitude: %d.%.6d\n", loc.lat_integer, loc.lat_fractional);
    printf("Longitude: %d.%.6d\n", loc.lng_integer, loc.lng_fractional);
    printf("Accuracy: %d\n", loc.accuracy);
    printf("http://maps.google.com/maps?q=%d.%.6d,%d.%.6d\n", loc.lat_integer, loc.lat_fractional, loc.lng_integer, loc.lng_fractional);
    return 0;
}