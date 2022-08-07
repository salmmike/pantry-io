/* Functions related to non-voltatile storage */

#define MAX_USERNAME_LENGTH 128
#define MAX_PASSWD_LENGTH 128
#define STR_LENGTH 128

void init_nvs(void);
char* get_nvs_data(const char *val_name);
char* get_wifi_passwd(void);
char* get_wifi_user(void);
char* get_auth_token(void);
char* get_unit_id(void);

int save_wifi_credientials(const char* username, const char* passwd);
int save_auth_token(const char* token);
int save_unit_id(const char* id);
