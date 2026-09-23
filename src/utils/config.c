#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Trim leading and trailing whitespace in-place
static char* trim(char* s) {
    while (*s == ' ' || *s == '\t') s++;
    char* end = s + strlen(s) - 1;
    while (end > s && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        *end = '\0';
        end--;
    }
    return s;
}

int Config_Load(AppConfig* cfg, const char* ini_path) {
    // Set defaults
    strncpy(cfg->video_path, "assets\\garage_loop.mp4", MAX_PATH_LEN - 1);
    cfg->video_path[MAX_PATH_LEN - 1] = '\0';
    cfg->target_fps = 60;
    cfg->idle_timeout_sec = 300;
    cfg->pause_on_battery = 1;

    FILE* f = fopen(ini_path, "r");
    if (!f) {
        return -1;
    }

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        char* trimmed = trim(line);

        // Skip empty lines, comments, and section headers
        if (trimmed[0] == '\0' || trimmed[0] == ';' || trimmed[0] == '#' || trimmed[0] == '[') {
            continue;
        }

        char* eq = strchr(trimmed, '=');
        if (!eq) continue;

        *eq = '\0';
        char* key = trim(trimmed);
        char* val = trim(eq + 1);

        if (strcmp(key, "VideoPath") == 0) {
            strncpy(cfg->video_path, val, MAX_PATH_LEN - 1);
            cfg->video_path[MAX_PATH_LEN - 1] = '\0';
        } else if (strcmp(key, "TargetFPS") == 0) {
            cfg->target_fps = atoi(val);
        } else if (strcmp(key, "IdleTimeoutSeconds") == 0) {
            cfg->idle_timeout_sec = atoi(val);
        } else if (strcmp(key, "PauseOnBattery") == 0) {
            cfg->pause_on_battery = atoi(val);
        }
    }

    fclose(f);
    return 0;
}
