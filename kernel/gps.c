#include <linux/kernel.h>
#include <linux/gps.h>
#include <linux/export.h>
#include <linux/syscalls.h>
#include <linux/spinlock.h>
#include <linux/errno.h>
#include <linux/slab.h>

struct gps_location init_location = INIT_GPS_LOCATION(init_location);
EXPORT_SYMBOL(init_location);

DEFINE_SPINLOCK(loc_lock);

inline void location_lock(void) {
    spin_lock(&loc_lock);
}

inline void location_unlock(void) {
    spin_unlock(&loc_lock);
}

long set_gps_location(struct gps_location __user *loc) {
    struct gps_location *_loc;

    if (loc == NULL) return -EFAULT;

    _loc = (struct gps_location *)kmalloc(sizeof(struct gps_location), GFP_KERNEL);

    if (!_loc) return -EFAULT;

    if(copy_from_user(_loc, loc, sizeof(struct gps_location)) > 0) {
        kfree(_loc);
        return -EFAULT;
    }

    if (_loc->lat_integer < -90 || _loc->lat_integer > 90 ||
        _loc->lng_integer < -90 || _loc->lng_integer > 90 ||
        _loc->lat_fractional < 0 || _loc->lat_fractional > 999999 ||
        _loc->lng_fractional < 0 || _loc->lng_fractional > 999999 ||
        _loc->accuracy < 0) {
        kfree(_loc);
        return -EINVAL;
    }

    location_lock();
    init_location.lat_integer = _loc->lat_integer;
    init_location.lat_fractional = _loc->lat_fractional;
    init_location.lng_integer = _loc->lng_integer;
    init_location.lng_fractional = _loc->lng_fractional;
    init_location.accuracy = _loc->accuracy;
    location_unlock();

    kfree(_loc);
    return 0;
}

SYSCALL_DEFINE1(set_gps_location, struct gps_location __user *, loc)
{
    return set_gps_location(loc);
}