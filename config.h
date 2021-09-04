#ifndef CONFIG_LOADER_DEF
#define CONFIG_LOADER_DEF

void Config_Load(const char *path);
void Config_Close();
char *Config_GetBrowserCmd();
#endif