/* OS Project 4 */
#ifndef _LINUX_GPS_H
#define _LINUX_GPS_H

#define INIT_GPS_LOCATION(_name) {\
    .lat_integer=-1,\
    .lat_fractional=-1,\
    .lng_integer=-1,\
    .lng_fractional=-1,\
    .accuracy=-1\
}

#define LAT_TO_U32(x) (x + 90)
#define LNG_TO_U32(x) (x + 180)
#define U32_TO_LAT(x) (x - 90)
#define U32_TO_LNG(x) (x - 180)

// (meter) THIS VALUE IS FROM NASA :)
#define EARTH_CIRCUMFERENCE 40030000 
// (meter) distance per 1 lat/lng difference
#define UNIT_DISTANCE 111195
// (micrometer) didstance per 1 * 1e-6 lat/lng difference
#define UNIT_DISTANCE_MM (UNIT_DISTANCE * 1e6)

struct gps_location {
  int lat_integer;
  int lat_fractional;
  int lng_integer;
  int lng_fractional;
  int accuracy;
};

/* global gps_location struct */
extern struct gps_location init_location;
void location_lock(void);
void location_unlock(void);
int can_access_here(struct gps_location *);

#endif