/*
 * Copyright (C) 2015 The CyanogenMod Project
 * Copyright (C) 2017 The LineageOS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#define LOG_TAG "PowerHal"
#include <utils/Log.h>

#include <hardware/hardware.h>
#include <hardware/power.h>

#define PATH_GPIO_KEYS "/sys/class/input/input14/enabled"
#define PATH_TOUCHKEY "/sys/class/input/input15/enabled"
#define PATH_TOUCHSCREEN "/sys/class/input/input1/enabled"

enum {
    PROFILE_POWER_SAVE,
    PROFILE_BALANCED,
    PROFILE_HIGH_PERFORMANCE,
    PROFILE_MAX,
};

static int current_profile = -1;

/* Usage: command > /dev/bL_operator
 *   command : 00 - switch disable
 *   	   01 - LITTLE only
 *   	   10 - big only
 *   	   11 - big.LITTLE
 *   echo 10 > /dev/bL_operator
 */
#define PATH_BL_OPERATOR "/dev/b.L_operator"
static char bL_operator[PROFILE_MAX][3] = {"01", "11", "10"};

static void sysfs_write(char *path, char *s)
{
    char buf[80];
    int len;
    int fd;

    if (path == NULL)
        return;

    if ((fd = open(path, O_WRONLY)) < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error opening %s: %s\n", path, buf);
        return;
    }

    len = write(fd, s, strlen(s));
    if (len < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error writing to %s: %s\n", path, buf);
    }

    close(fd);
}

static void power_init(struct power_module *module __unused)
{
}

static void power_set_interactive(struct power_module *module __unused, int on)
{
    ALOGD("%s: %sabling input devices", __func__, on ? "En" : "Dis");
    sysfs_write(PATH_TOUCHSCREEN, on ? "1" : "0");
    sysfs_write(PATH_TOUCHKEY, on ? "1" : "0");
    sysfs_write(PATH_GPIO_KEYS, on ? "1" : "0");
    if (current_profile != PROFILE_POWER_SAVE) {
        ALOGD("%s: %sabling big cluster", __func__, on ? "En" : "Dis");
        sysfs_write(PATH_BL_OPERATOR, bL_operator[on ? current_profile : PROFILE_POWER_SAVE]);
    }
}

static void set_power_profile(int profile)
{
    if (profile == current_profile)
        return;

    sysfs_write(PATH_BL_OPERATOR, bL_operator[profile])
    current_profile = profile;
}

static void power_hint(struct power_module *module __unused, power_hint_t hint,
        void *data)
{
    if (hint == POWER_HINT_SET_PROFILE)
        set_power_profile(*(int32_t *)data);
}

static void set_feature(struct power_module *module __unused,
                feature_t feature __unused, int state __unused)
{
}

static int get_feature(struct power_module *module __unused, feature_t feature)
{
    if (feature == POWER_FEATURE_SUPPORTED_PROFILES)
        return PROFILE_MAX;
    return -1;
}

static struct hw_module_methods_t power_module_methods = {
    .open = NULL,
};

struct power_module HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = POWER_MODULE_API_VERSION_0_3,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = POWER_HARDWARE_MODULE_ID,
        .name = "I9500 Power HAL",
        .author = "The LineageOS Project",
        .methods = &power_module_methods,
    },
    .init = power_init,
    .setInteractive = power_set_interactive,
    .powerHint = power_hint,
    .setFeature = set_feature,
    .getFeature = get_feature
};
