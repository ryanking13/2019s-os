#include <linux/gps.h>
#include <linux/export.h>
#include <linux/syscalls.h>
#include <linux/spinlock.h>

struct gps_location init_location = INIT_GPS_LOCATION(init_location);
EXPORT_SYMBOL(init_location);

DEFINE_SPINLOCK(loc_lock);

inline void location_lock(void) {
    spin_lock(&loc_lock);
}

inline void loccation_unlock(void) {
    spin_unlock(&loc_lock);
}

long set_gps_location(struct gps_location __user *loc) {
    location_lock();
    loccation_unlock();
    return 1;
}

SYSCALL_DEFINE1(set_gps_location, struct gps_location __user *, loc)
{
    return set_gps_location(loc);
}