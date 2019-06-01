#include <linux/kernel.h>
#include <linux/gps.h>
#include <linux/export.h>
#include <linux/syscalls.h>
#include <linux/spinlock.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/namei.h>
#include "dec6.c"

struct gps_location init_location = INIT_GPS_LOCATION(init_location);
EXPORT_SYMBOL(init_location);

DEFINE_SPINLOCK(loc_lock);

void location_lock(void) {
    spin_lock(&loc_lock);
}

void location_unlock(void) {
    spin_unlock(&loc_lock);
}

int can_access_here(struct gps_location *file_loc) {
    int ret = 1, distance_int;
    struct gps_location *curr_loc = &init_location;
    struct decimal_6 curr_lat, curr_lng, file_lat, file_lng, distance;
    location_lock();
    curr_lat.integer = curr_loc->lat_integer;
    curr_lat.fractional = curr_loc->lat_fractional;
    curr_lng.integer = curr_loc->lng_integer;
    curr_lng.fractional = curr_loc->lng_fractional;
    file_lat.integer = file_loc->lat_integer;
    file_lat.fractional = file_loc->lat_fractional;
    file_lng.integer = file_loc->lng_integer;
    file_lng.fractional = file_loc->lng_fractional;
    distance = get_distance_dec6(curr_lng,curr_lat,file_lng,file_lat);
    distance_int = distance.integer * 1000 + distance.fractional/1000 * (1-2*(distance.integer<0));
    if(distance_int < 0)
        distance_int *=(-1);
    ret = distance_int <=file_loc->accuracy + curr_loc->accuracy;
    location_unlock();
    return ret;
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
        _loc->lng_integer < -180 || _loc->lng_integer > 180 ||
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

long get_gps_location(const char __user *pathname, struct gps_location __user * loc) {
    struct filename *tmp;
    struct gps_location *_loc;
    struct path path;
    struct inode *inode;
    int error = 0;

    if (pathname == NULL || loc == NULL) return -EFAULT;

    // from open() syscall
    tmp = getname(pathname);
	if (IS_ERR(tmp)) {
        // printk(KERN_INFO "getname failed\n");
		return PTR_ERR(tmp);
    }

    error = kern_path(tmp->name, 0, &path);
    if (error) {
        // printk(KERN_INFO "kern_path failed\n");
        return -EFAULT;
    }

    inode = path.dentry->d_inode;

    // check fs/namei.c and fs/open.c
    if (inode_permission(inode, MAY_READ | MAY_GET_LOCATION)) {
        // printk(KERN_INFO "inode_permission failed\n");
        return -EACCES;
    }

    if (!inode->i_op->get_gps_location) {
        // printk(KERN_INFO "get_gps_location not exists\n");
        return -ENODEV;
    }

    _loc = (struct gps_location *)kmalloc(sizeof(struct gps_location), GFP_KERNEL);
    inode->i_op->get_gps_location(inode, _loc);
    
    error = copy_to_user(loc, _loc, sizeof(struct gps_location));
    kfree(_loc);

    if (error) {
        // printk(KERN_INFO "copy_to_user failed\n");
        return -EFAULT;
    }

    return 0;
}

SYSCALL_DEFINE1(set_gps_location, struct gps_location __user *, loc)
{
    return set_gps_location(loc);
}

SYSCALL_DEFINE2(get_gps_location, const char __user *, pathname, struct gps_location __user *, loc)
{
    return get_gps_location(pathname, loc);
}
