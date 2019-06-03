#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <sys/time.h>
#include <linux/gps.h>
#include <errno.h>

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
        printf("Getting GPS location failed, errno = %d\n", errno);
        printf("Please check arguments\n");
        return 1;
    }

    printf("GPS location information of: %s\n", argv[1]);

    char latitude[10];
    char longitude[10];

    if (loc.lat_integer < 0 && loc.lat_fractional > 0) {
        sprintf(latitude, "%d.%.6d", loc.lat_integer + 1, 1000000 - loc.lat_fractional);
    }
    else sprintf(latitude, "%d.%.6d", loc.lat_integer, loc.lat_fractional);

    if (loc.lng_integer < 0 && loc.lng_fractional > 0) {
        sprintf(longitude, "%d.%.6d", loc.lng_integer + 1, 1000000 - loc.lng_fractional);
    }
    else sprintf(longitude, "%d.%.6d", loc.lng_integer, loc.lng_fractional);

    printf("Latitude: %s\n", latitude);
    printf("Longitude: %s\n", longitude);
    printf("Accuracy: %d\n", loc.accuracy);
    printf("http://maps.google.com/maps?q=%s,%s\n", latitude, longitude);
    return 0;
}