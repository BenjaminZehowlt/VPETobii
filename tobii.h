#ifndef LIBEYETRACK_TOBII_H
#define LIBEYETRACK_TOBII_H

struct tobii_device_t; // Contents Unknown
struct tobii_api_t; // Contents Unknown
struct tobii_custom_alloc_t; // Contents Unknown


typedef struct tobii_version_t {
    int major;
    int minor;
    int revision;
} tobii_version_t;

typedef enum tobii_log_level_t {
    TOBII_LOG_LEVEL_ERROR,
    TOBII_LOG_LEVEL_WARNING,
    TOBII_LOG_LEVEL_INFO,
    TOBII_LOG_LEVEL_DEBUG,
} tobii_log_level_t;

typedef enum tobii_error_t {

} tobii_error_t;

typedef enum tobii_field_of_use_t {
    TOBII_FIELD_OF_USE_INTERACTIVE = 1,
    TOBII_FIELD_OF_USE_ANALYTICAL = 2,
} tobii_field_of_use_t;

typedef void tobii_custom_log_t(void* log_context, tobii_log_level_t level, char const* text );
typedef tobii_error_t tobii_api_create_t(tobii_api_t** api, tobii_custom_alloc_t* alloc, tobii_custom_log_t* log);
typedef tobii_error_t tobii_get_api_version_t(tobii_version_t* version);

typedef void tobii_device_url_receiver_t(char const* url, void* user_data);
typedef tobii_error_t tobii_enumerate_local_device_urls_t(tobii_api_t* api, tobii_device_url_receiver_t* receiver, void* user_data);

typedef tobii_error_t tobii_device_create_t(tobii_api_t* api, char const* url, tobii_field_of_use_t field_of_use, tobii_device_t** device);

#endif //LIBEYETRACK_TOBII_H
