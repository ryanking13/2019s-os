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

void print_map_link(struct gps_location* locp) {
    struct gps_location loc = *locp;
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
}

int main() {
    struct gps_location B302_100 = {37, 448663, 126, 952551, 100};
    struct gps_location B302_10 = {37, 448663, 126, 952551, 10};
    struct gps_location B301_100 = {37, 449987, 126, 952497, 100};
    struct gps_location B301_10 = {37, 449987, 126, 952497, 10};

    struct gps_location EIFFEL = {48, 858300, 2, 294500, 1000};
    struct gps_location LIBERTY = {40, 689200, -75, 955500, 1000};

    int ret;

    ret = system("ls proj4 2> /dev/null 1> /dev/null");
    if (ret != 0) {
        printf("proj4 directory does not exist, generating it\n");
        ret = system("./mount.sh");
        if (ret != 0) {
            printf("./mount.sh failed, terminating...\n");
            return 1;
        }
    }

    SET_GPS_LOCATION(&B301_10);
    printf("Creating 'proj4/f301' in B301 with accuracy 10 meters\n");

    ret = system("echo 12341234 > proj4/f301");
    if (ret != 0) { printf("FAIL: f301 generation failed\n"); return 1; }

    printf("\n");
    ret = system("./file_loc proj4/f301");
    if (ret != 0) { printf("FAIL: getting file location failed\n"); return 1; }

    printf("\nPress return to continue.\n");
    getchar();

    ////////////////

    SET_GPS_LOCATION(&EIFFEL);
    printf("Trying to read 'proj4/f301' in EIFFEL TOWER with accuracy 1000 meters\n");
    print_map_link(&EIFFEL);

    printf("\nPress return to continue.\n");
    getchar();

    ret = system("cat proj4/f301 1>/dev/null 2>/dev/null");
    if (ret == 0) { printf("FAIL: you read B301 file in EIFFEL TOWER\n"); return 1; }
    else printf("SUCCESS: you cannot read B301 file in EIFFEL TOWER\n");

    ////////////////

    printf("Press return to continue.\n");
    getchar();

    SET_GPS_LOCATION(&B302_10);
    printf("Trying to read 'proj4/f301' in B302 with accuracy 10 meters\n");
    print_map_link(&B302_10);

    printf("\nPress return to continue.\n");
    getchar();

    ret = system("cat proj4/f301 1>/dev/null 2>/dev/null");
    if (ret == 0) { printf("FAIL: you read B301 file in B302\n"); return 1; }
    else printf("SUCCESS: you cannot read B301 file in B302 (accuracy=10)\n");

    ////////////////

    printf("Press return to continue.\n");
    getchar();

    SET_GPS_LOCATION(&B301_100);
    printf("Modifying 'proj4/f301' in 301 with accuracy 100 meters\n");

    ret = system("echo 12341234 >> proj4/f301");
    if (ret != 0) { printf("FAIL: f301 modification failed\n"); return 1; }

    ret = system("./file_loc proj4/f301");
    if (ret != 0) { printf("FAIL: getting file location failed\n"); return 1; }

    ////////////////

    printf("\nPress return to continue.\n");
    getchar();

    SET_GPS_LOCATION(&B302_100);
    printf("Trying to read 'proj4/f301' in 302 with accuracy 100 meters\n");

    ret = system("cat proj4/f301 1>/dev/null 2>/dev/null");
    if (ret != 0) { printf("FAIL: you cannot read 301 file in 302 (acc: 100) \n"); return 1; }
    else printf("SUCCESS: you read B301 file in B302 (accuracy=100)\n");

    printf("\nAll tests complete.\n");
}