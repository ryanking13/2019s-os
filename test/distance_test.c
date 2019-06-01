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
    struct gps_location NAKSUNGDAE = {37, 476970, 126, 963720, 100};
    struct gps_location SULIP = {37, 481245, 126, 952753, 100};
    struct gps_location B302_100 = {37, 448663, 126, 952551, 100};
    struct gps_location B302_10 = {37, 448663, 126, 952551, 10};
    struct gps_location B301_100 = {37, 449987, 126, 952497, 100};
    struct gps_location B301_10 = {37, 449987, 126, 952497, 10};
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

    SET_GPS_LOCATION(&NAKSUNGDAE);
    printf("I'm now in NAKSUNGDAE\n");

    ret = system("echo 12341234 > proj4/dist1");
    if (ret != 0) { printf("FAIL: dist1 generation failed\n"); return 1; }

    SET_GPS_LOCATION(&SULIP);
    printf("I'm now in SULIP\n");

    ret = system("cat proj4/dist1 1>/dev/null 2>/dev/null");
    if (ret == 0) { printf("FAIL: you read NAKSUNGDAE file in SULIP\n"); return 1; }

    SET_GPS_LOCATION(&B301_10);
    printf("I'm now in B301 with accuracy 10 meter\n");

    ret = system("echo 12341234 > proj4/dist2");
    if (ret != 0) { printf("FAIL: dist2 generation failed\n"); return 1; }

    SET_GPS_LOCATION(&B302_10); 
    printf("I'm now in B302 with accuracy 10 meter\n");

    ret = system("cat proj4/dist2 1>/dev/null 2>/dev/null");
    if (ret == 0) { printf("FAIL: you read B301 file in B302 (acc: 10) \n"); return 1; }

    SET_GPS_LOCATION(&B301_100);
    printf("I'm now in B301 with accuracy 100 meter\n");

    ret = system("echo 78907890 >> proj4/dist2");
    if (ret != 0) { printf("FAIL: dist2 modification failed\n"); return 1; }

    SET_GPS_LOCATION(&B302_100);
    printf("I'm now in B302 with accuracy 100 meter\n");

    ret = system("cat proj4/dist2 1>/dev/null 2>/dev/null");
    if (ret != 0) { printf("FAIL: you cannot read B301 file in B302 (acc: 100) \n"); return 1; }

    printf("SUCCESS\n");
}
