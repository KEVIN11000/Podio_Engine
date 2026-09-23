#ifndef CONFIG_H
#define CONFIG_H

#define MAX_PATH_LEN 260

typedef struct {
    char video_path[MAX_PATH_LEN];
    int  target_fps;
    int  idle_timeout_sec;
    int  pause_on_battery;
} AppConfig;

// Loads configuration from the given INI file path.
// Returns 0 on success, -1 on failure.
int Config_Load(AppConfig* cfg, const char* ini_path);

#endif // CONFIG_H
