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

struct gps_location {
  int lat_integer;
  int lat_fractional;
  int lng_integer;
  int lng_fractional;
  int accuracy;
};

#endif