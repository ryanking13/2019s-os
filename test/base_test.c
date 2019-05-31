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

int equal_location(struct gps_location *l1, struct gps_location *l2) {
    if (
        l1->lat_integer != l2->lat_integer ||
        l1->lng_integer != l2->lng_integer ||
        l1->lat_fractional != l2->lat_fractional ||
        l1->lng_fractional != l2->lng_fractional ||
        l1->accuracy != l2->accuracy
    ) {
        return 0;
    }

    return 1;
}

int main(int argc, char **argv) {
    struct gps_location loc;
    struct gps_location _loc;
    int ret;
    loc.lat_integer = 0;
    loc.lng_integer = 0;
    loc.lat_fractional = 0;
    loc.lng_fractional = 0;
    loc.accuracy = 0;

    printf("Step 1: Checking set_gps_location() with invalid parameter\n");

    ret = SET_GPS_LOCATION(NULL);
    if (ret >= 0) { printf("FAIL: NULL gps_location, return code %d\n", ret); return 1; }

    /////////////////////

    loc.lat_integer = -91;
    ret = SET_GPS_LOCATION(&loc);
    if (ret >= 0) { printf("FAIL: lat_integer = %d, return code %d\n", loc.lat_integer, ret); return 1; }

    loc.lat_integer = 91;
    ret = SET_GPS_LOCATION(&loc);
    if (ret >= 0) { printf("FAIL: lat_integer = %d, return code %d\n", loc.lat_integer, ret); return 1; }
    
    loc.lat_integer = 0;

    ////////////////////

    loc.lng_integer = -181;
    ret = SET_GPS_LOCATION(&loc);
    if (ret >= 0) { printf("FAIL: lng_integer = %d, return code %d\n", loc.lng_integer, ret); return 1; }

    loc.lng_integer = 181;
    ret = SET_GPS_LOCATION(&loc);
    if (ret >= 0) { printf("FAIL: lng_integer = %d, return code %d\n", loc.lng_integer, ret); return 1; }

    loc.lng_integer = 0;

    ////////////////////

    loc.lat_fractional = -1;
    ret = SET_GPS_LOCATION(&loc);
    if (ret >= 0) { printf("FAIL: lat_fractional = %d, return code %d\n", loc.lat_fractional, ret); return 1; }

    loc.lat_fractional = 1000000;
    ret = SET_GPS_LOCATION(&loc);
    if (ret >= 0) { printf("FAIL: lat_fractional = %d, return code %d\n", loc.lat_fractional, ret); return 1; }

    loc.lat_fractional = 0;

    /////////////////////

    loc.lng_fractional = -1;
    ret = SET_GPS_LOCATION(&loc);
    if (ret >= 0) { printf("FAIL: lng_fractional = %d, return code %d\n", loc.lng_fractional, ret); return 1; }

    loc.lng_fractional = 1000000;
    ret = SET_GPS_LOCATION(&loc);
    if (ret >= 0) { printf("FAIL: lng_fractional = %d, return code %d\n", loc.lng_fractional, ret); return 1; }

    loc.lng_fractional = 0;

    /////////////////////

    loc.accuracy = -1;
    ret = SET_GPS_LOCATION(&loc);
    if (ret >= 0) { printf("FAIL: accuracy = %d, return code %d\n", loc.accuracy, ret); return 1; }

    loc.accuracy = 10;

    /////////////////////

    printf("Step 2: Checking set_gps_location() with valid parameter\n");

    ret = SET_GPS_LOCATION(&loc);
    if (ret < 0) { printf("FAIL: valid gps_location, return code %d\n", ret); return 1; }

    /////////////////////

    /* preparing for get_gps_location test */

    loc.lat_integer = 12;
    loc.lat_fractional = 34;
    loc.lng_integer = 56;
    loc.lng_fractional = 78;
    loc.accuracy = 90;
    ret = SET_GPS_LOCATION(&loc);

    ret = system("ls proj4 2> /dev/null 1> /dev/null");
    if (ret != 0) {
        printf("proj4 directory not exists, generating it\n");
        ret = system("./mount.sh");
        if (ret != 0) {
            printf("./mount.sh failed, terminating...\n");
            return 1;
        }
    }

    ret = system("echo 1234 > proj4/test1");
    if (ret != 0) {
        printf("generating proj4/test1 failed, terminating...\n");
        return 1;
    }

    /////////////////////

    printf("Step 3: Checking get_gps_location() with invalid parameter\n");

    ret = GET_GPS_LOCATION(NULL, &_loc);
    if (ret >= 0) { printf("FAIL: NULL path, return code %d\n", ret); return 1; }

    ret = GET_GPS_LOCATION("proj4/test1", NULL);
    if (ret >= 0) { printf("FAIL: NULL loc, return code %d\n", ret); return 1; }

    ret = GET_GPS_LOCATION("proj4/NOTEXISTS", &_loc);
    if (ret >= 0) { printf("FAIL: not existing path, return code %d\n", ret); return 1; }

    ret = GET_GPS_LOCATION("mount.sh", &_loc);
    if (ret >= 0) { printf("FAIL: not ext2 file, return code %d\n", ret); return 1; }
    if (errno != ENODEV) { printf("FAIL: errno must be ENODEV(%d), but it is %d\n", -ENODEV, errno); return 1; }

    //////////////////////

    printf("Step 4: Checking get_gps_location() with valid parameter\n");
    
    ret = GET_GPS_LOCATION("proj4/test1", &_loc);
    if (ret < 0) { printf("FAIL: failed to get gps location, return code %d\n", ret); return 1; }

    if (!equal_location(&loc, &_loc)) {
        printf("FAIL: get_gps_location() did not set valid location\n");
        return 1;
    }

    ///////////////////////

    printf("Step 5: Checking file modification\n");

    loc.accuracy = 123;
    SET_GPS_LOCATION(&loc);
    system("echo 12341234 > proj4/test1");
    GET_GPS_LOCATION("proj4/test1", &_loc);

    if (!equal_location(&loc, &_loc)) {
        printf("FAIL: writing did not changed location\n");
        return 1;
    }

    loc.accuracy = 456;
    SET_GPS_LOCATION(&loc);
    system("echo 123123123 >> proj4/test1");
    GET_GPS_LOCATION("proj4/test1", &_loc);

    if (!equal_location(&loc, &_loc)) {
        printf("FAIL: appending did not changed location\n");
        return 1;
    }

    ////////////////////////

    printf("Done, all tests passed\n");
}