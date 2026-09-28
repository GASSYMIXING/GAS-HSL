#include <obs-module.h>
#include <graphics/vec4.h>
#include <util/bmem.h>

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("gas-hsl", "en-US")

enum { REGION_COUNT = 8 };

static const char *const region_names[REGION_COUNT] = {
    "red", "orange", "yellow", "green", "cyan", "blue", "purple", "magenta"
};
static const char *const region_labels[REGION_COUNT] = {
    "Red", "Orange", "Yellow", "Green", "Cyan", "Blue", "Purple", "Magenta"
};
static const char *const controls[3] = {"hue", "saturation", "lightness"};
static const char *const control_labels[3] = {"Hue", "Saturation", "Lightness"};

struct gas_hsl {
    obs_source_t *source;
    gs_effect_t *effect;
    gs_eparam_t *master_param;
    gs_eparam_t *region_params[REGION_COUNT];
    gs_eparam_t *custom_param;
    gs_eparam_t *custom_color_param;
    struct vec4 master;
    struct vec4 regions[REGION_COUNT];
    struct vec4 custom;
    struct vec4 custom_color;
    bool enabled;
};

#ifdef _WIN32
static SRWLOCK picker_lock = SRWLOCK_INIT;
static HHOOK picker_hook_handle;
static obs_weak_source_t *picker_target;
static obs_source_t *picker_identity;

static void stop_picker(obs_source_t *only_source)
{
    AcquireSRWLockExclusive(&picker_lock);
    HHOOK hook = NULL;
    obs_weak_source_t *weak = NULL;
    if (!only_source || picker_identity == only_source) {
        hook = picker_hook_handle;
        weak = picker_target;
        picker_hook_handle = NULL;
        picker_target = NULL;
        picker_identity = NULL;
    }
    ReleaseSRWLockExclusive(&picker_lock);
    if (hook)
        UnhookWindowsHookEx(hook);
    if (weak)
        obs_weak_source_release(weak);
}

static LRESULT CALLBACK pick_mouse(int code, WPARAM message, LPARAM value)
{
    if (code != HC_ACTION || message != WM_LBUTTONDOWN)
        return CallNextHookEx(NULL, code, message, value);

    const MSLLHOOKSTRUCT *click = (const MSLLHOOKSTRUCT *)value;
    HWND hit = WindowFromPoint(click->pt);
    DWORD process_id = 0;
    if (!hit || !GetParent(hit) || !GetWindowThreadProcessId(hit, &process_id) ||
        process_id != GetCurrentProcessId())
        return CallNextHookEx(NULL, code, message, value);

    HDC screen = GetDC(NULL);
    COLORREF color = screen ? GetPixel(screen, click->pt.x, click->pt.y) : CLR_INVALID;
    if (screen)
        ReleaseDC(NULL, screen);
    if (color == CLR_INVALID)
        return CallNextHookEx(NULL, code, message, value);

    AcquireSRWLockExclusive(&picker_lock);
    obs_weak_source_t *weak = picker_target;
    HHOOK hook = picker_hook_handle;
    picker_target = NULL;
    picker_hook_handle = NULL;
    picker_identity = NULL;
    obs_source_t *source = weak ? obs_weak_source_get_source(weak) : NULL;
    ReleaseSRWLockExclusive(&picker_lock);
    if (hook)
        UnhookWindowsHookEx(hook);
    if (weak)
        obs_weak_source_release(weak);
    if (source) {
        obs_data_t *settings = obs_source_get_settings(source);
        obs_data_set_int(settings, "custom_color", (long long)color);
        obs_source_update(source, settings);
        obs_source_update_properties(source);
        obs_data_release(settings);
        obs_source_release(source);
    }
    return CallNextHookEx(NULL, code, message, value);
}

static bool pick_preview(obs_properties_t *props, obs_property_t *button, void *data)
{
    struct gas_hsl *filter = data;
    (void)props;
    (void)button;
    if (!filter)
        return false;
    AcquireSRWLockShared(&picker_lock);
    bool already_armed = picker_identity == filter->source;
    ReleaseSRWLockShared(&picker_lock);
    stop_picker(NULL);
    if (already_armed)
        return false;

    HMODULE module = NULL;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                               GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCWSTR)pick_mouse, &module))
        return false;
    obs_weak_source_t *weak = obs_source_get_weak_source(filter->source);
    HHOOK hook = SetWindowsHookExW(WH_MOUSE_LL, pick_mouse, module, 0);
    if (!hook) {
        obs_weak_source_release(weak);
        blog(LOG_WARNING, "[GAS-HSL] Could not start preview color picker");
        return false;
    }
    AcquireSRWLockExclusive(&picker_lock);
    picker_target = weak;
    picker_hook_handle = hook;
    picker_identity = filter->source;
    ReleaseSRWLockExclusive(&picker_lock);
    return false;
}
#endif

static void setting_name(char *buffer, size_t size, const char *group, const char *control)
{
    snprintf(buffer, size, "%s_%s", group, control);
}

static float setting_value(obs_data_t *settings, const char *group, const char *control)
{
    char name[64];
    setting_name(name, sizeof(name), group, control);
    return (float)obs_data_get_int(settings, name);
}

static void read_group(obs_data_t *settings, const char *name, struct vec4 *value)
{
    value->x = setting_value(settings, name, "hue") / 360.0f;
    value->y = setting_value(settings, name, "saturation") / 100.0f;
    value->z = setting_value(settings, name, "lightness") / 100.0f;
    value->w = 0.0f;
}

static const char *gas_hsl_name(void *unused)
{
    (void)unused;
    return obs_module_text("Filter.Name");
}

static void gas_hsl_update(void *data, obs_data_t *settings)
{
    struct gas_hsl *filter = data;
    filter->enabled = obs_data_get_bool(settings, "enable_processing");
    read_group(settings, "master", &filter->master);
    for (int i = 0; i < REGION_COUNT; ++i)
        read_group(settings, region_names[i], &filter->regions[i]);
    read_group(settings, "custom", &filter->custom);
    vec4_from_rgba(&filter->custom_color,
                   (uint32_t)obs_data_get_int(settings, "custom_color") | 0xFF000000);
}

static void *gas_hsl_create(obs_data_t *settings, obs_source_t *source)
{
    struct gas_hsl *filter = bzalloc(sizeof(*filter));
    filter->source = source;
    char *path = obs_module_file("effects/hsl.effect");
    char *error = NULL;
    if (path) {
        obs_enter_graphics();
        filter->effect = gs_effect_create_from_file(path, &error);
        obs_leave_graphics();
        bfree(path);
    }
    if (!filter->effect) {
        blog(LOG_ERROR, "[GAS-HSL] Failed to load hsl.effect: %s", error ? error : "file missing");
        bfree(error);
        bfree(filter);
        return NULL;
    }
    bfree(error);
    filter->master_param = gs_effect_get_param_by_name(filter->effect, "master_hsl");
    for (int i = 0; i < REGION_COUNT; ++i) {
        char name[64];
        setting_name(name, sizeof(name), region_names[i], "hsl");
        filter->region_params[i] = gs_effect_get_param_by_name(filter->effect, name);
    }
    filter->custom_param = gs_effect_get_param_by_name(filter->effect, "custom_hsl");
    filter->custom_color_param = gs_effect_get_param_by_name(filter->effect, "custom_color");
    gas_hsl_update(filter, settings);
    return filter;
}

static void gas_hsl_destroy(void *data)
{
    struct gas_hsl *filter = data;
    if (!filter)
        return;
#ifdef _WIN32
    stop_picker(filter->source);
#endif
    obs_enter_graphics();
    gs_effect_destroy(filter->effect);
    obs_leave_graphics();
    bfree(filter);
}

static void gas_hsl_render(void *data, gs_effect_t *unused)
{
    struct gas_hsl *filter = data;
    (void)unused;
    if (!filter->enabled) {
        obs_source_skip_video_filter(filter->source);
        return;
    }
    if (!obs_source_process_filter_begin(filter->source, GS_RGBA, OBS_ALLOW_DIRECT_RENDERING))
        return;
    gs_effect_set_vec4(filter->master_param, &filter->master);
    for (int i = 0; i < REGION_COUNT; ++i)
        gs_effect_set_vec4(filter->region_params[i], &filter->regions[i]);
    gs_effect_set_vec4(filter->custom_param, &filter->custom);
    gs_effect_set_vec4(filter->custom_color_param, &filter->custom_color);
    obs_source_process_filter_end(filter->source, filter->effect, 0, 0);
}

static void gas_hsl_defaults(obs_data_t *settings)
{
    obs_data_set_default_bool(settings, "enable_processing", true);
    obs_data_set_default_int(settings, "custom_color", 0x000080FF);
    char name[64];
    for (int group = -1; group <= REGION_COUNT; ++group) {
        const char *group_name = group < 0 ? "master" :
                                 group == REGION_COUNT ? "custom" : region_names[group];
        for (int control = 0; control < 3; ++control) {
            setting_name(name, sizeof(name), group_name, controls[control]);
            obs_data_set_default_int(settings, name, 0);
        }
    }
}

static bool reset_all(obs_properties_t *props, obs_property_t *button, void *data)
{
    struct gas_hsl *filter = data;
    (void)props;
    (void)button;
    if (!filter)
        return false;
    obs_data_t *settings = obs_source_get_settings(filter->source);
    char name[64];
    obs_data_set_int(settings, "custom_color", 0x000080FF);
    for (int group = -1; group <= REGION_COUNT; ++group) {
        const char *group_name = group < 0 ? "master" :
                                 group == REGION_COUNT ? "custom" : region_names[group];
        for (int control = 0; control < 3; ++control) {
            setting_name(name, sizeof(name), group_name, controls[control]);
            obs_data_set_int(settings, name, 0);
        }
    }
    obs_source_update(filter->source, settings);
    obs_data_release(settings);
    return true;
}

static void add_group(obs_properties_t *root, const char *name, const char *label, bool master)
{
    obs_properties_t *group = obs_properties_create();
    if (strcmp(name, "custom") == 0)
        obs_properties_add_color(group, "custom_color", obs_module_text("CustomColor"));
    char setting[64];
    for (int i = 0; i < 3; ++i) {
        setting_name(setting, sizeof(setting), name, controls[i]);
        const int range = i == 0 ? (master ? 180 : 60) : 100;
        obs_property_t *property = obs_properties_add_int_slider(
            group, setting, obs_module_text(control_labels[i]), -range, range, 1);
        obs_property_int_set_suffix(property, i == 0 ? "°" : "%");
    }
    obs_properties_add_group(root, name, obs_module_text(label), OBS_GROUP_NORMAL, group);
}

static obs_properties_t *gas_hsl_properties(void *data)
{
    obs_properties_t *props = obs_properties_create();
    obs_properties_add_bool(props, "enable_processing", obs_module_text("EnableProcessing"));
    add_group(props, "master", "Master", true);
    for (int i = 0; i < REGION_COUNT; ++i)
        add_group(props, region_names[i], region_labels[i], false);
    add_group(props, "custom", "Custom", false);
#ifdef _WIN32
    obs_properties_add_button2(props, "pick_preview", obs_module_text("PickPreview"), pick_preview, data);
#endif
    obs_properties_add_button2(props, "reset_all", obs_module_text("ResetAll"), reset_all, data);
    obs_properties_add_text(props, "creator", obs_module_text("Creator"), OBS_TEXT_INFO);
    obs_properties_add_text(props, "website", obs_module_text("WebsiteLink"), OBS_TEXT_INFO);
    return props;
}

static struct obs_source_info gas_hsl_info = {
    .id = "gas_hsl_filter",
    .type = OBS_SOURCE_TYPE_FILTER,
    .output_flags = OBS_SOURCE_VIDEO,
    .get_name = gas_hsl_name,
    .create = gas_hsl_create,
    .destroy = gas_hsl_destroy,
    .update = gas_hsl_update,
    .video_render = gas_hsl_render,
    .get_defaults = gas_hsl_defaults,
    .get_properties = gas_hsl_properties,
};

bool obs_module_load(void)
{
    obs_register_source(&gas_hsl_info);
    blog(LOG_INFO, "[GAS-HSL] Plugin loaded");
    return true;
}

void obs_module_unload(void)
{
#ifdef _WIN32
    stop_picker(NULL);
#endif
}
