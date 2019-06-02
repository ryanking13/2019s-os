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

int main() {
    struct gps_location NAKSUNGDAE = {37, 476970, 126, 963720, 1000};
    struct gps_location TOKYO = {35, 680415, 139, 769057, 1000};
    struct gps_location B302_100 = {37, 448663, 126, 952551, 100};
    struct gps_location B302_10 = {37, 448663, 126, 952551, 10};
    struct gps_location B301_100 = {37, 449987, 126, 952497, 100};
    struct gps_location B301_10 = {37, 449987, 126, 952497, 10};
    struct gps_location COEX_1 = {37, 512456, 127, 58828, 1};
    struct gps_location COEX_2000 = {37, 512456, 127, 58828, 2000};
    struct gps_location GANGNAM_STATION_1 = {37, 497919, 127, 27656, 1};
    struct gps_location GANGNAM_STATION_2000 = {37, 497919, 127, 27656, 2000};
    int ret;

    ret = system("ls proj4 2> /dev/null 1> /dev/null");
    if (ret != 0) {
        printf("proj4 directory not exists, generating it\n");
        ret = system("./mount.sh");
        if (ret != 0) {
            printf("./mount.sh failed, terminating...\n");
            return 1;
        }
    }

    SET_GPS_LOCATION(&NAKSUNGDAE);
    printf("I'm now in NAKSUNGDAE\n");

    ret = system("echo 12341234 > proj4/dist1");
    if (ret != 0) { printf("FAIL: dist1 generation failed\n"); return 1; }

    SET_GPS_LOCATION(&TOKYO);
    printf("I'm now in TOKYO\n");

    ret = system("cat proj4/dist1 1>/dev/null 2>/dev/null");
    if (ret == 0) { printf("FAIL: you read NAKSUNGDAE file in TOKYO\n"); return 1; }

    SET_GPS_LOCATION(&COEX_1);
    printf("I'm now in COEX with accuracy 1 meter\n");

    ret = system("echo 12341234 > proj4/dist2");
    if (ret != 0) { printf("FAIL: dist2 generation failed\n"); return 1; }

    SET_GPS_LOCATION(&GANGNAM_STATION_1); 
    printf("I'm now in GANGNAM_STATION with accuracy 1 meter\n");

    ret = system("cat proj4/dist2 1>/dev/null 2>/dev/null");
    if (ret == 0) { printf("FAIL: you read COEX file in GANGNAM_STATION (acc: 1) \n"); return 1; }

    SET_GPS_LOCATION(&COEX_2000);
    printf("I'm now in COEX with accuracy 2000 meter\n");

    ret = system("echo 78907890 >> proj4/dist2");
    if (ret != 0) { printf("FAIL: dist2 modification failed\n"); return 1; }

    SET_GPS_LOCATION(&GANGNAM_STATION_2000);
    printf("I'm now in GANGNAM_STATION with accuracy 2000 meter\n");

    ret = system("cat proj4/dist2 1>/dev/null 2>/dev/null");
    if (ret != 0) { printf("FAIL: you failed to read COEX file in GANGNAM_STATION (acc: 2000) \n"); return 1; }

    printf("SUCCESS\n");
}