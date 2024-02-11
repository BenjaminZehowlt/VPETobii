#if defined(_WIN32)
    #define EXPORT __declspec(dllexport)
    #include <windows.h>
#include <cstdio>

#else
    #define EXPORT __attribute__((visibility("default")))
#endif

#include "crc32.c"
#include "tobii.h"
#include "vive_control_t.h"
#include <libusb.h>
#include <thread>

std::thread* _thread = nullptr;

bool _has_loaded_stream_engine = false;

tobii_api_create_t* tobii_api_create;
tobii_get_api_version_t* tobii_get_api_version;
tobii_enumerate_local_device_urls_t* tobii_enumerate_local_device_urls;
tobii_device_create_t* tobii_device_create;

libusb_device* find_device(libusb_device** devices, uint16_t vendor_id, uint16_t product_id) {
    libusb_device* device;
    int i = 0;

    while ((device = devices[i++]) != nullptr) {
        struct libusb_device_descriptor desc;
        int r = libusb_get_device_descriptor(device, &desc);
        if (r < 0) {
            fprintf(stderr, "failed to get device descriptor");
            return nullptr;
        }

        if (desc.idVendor == vendor_id && desc.idProduct == product_id)
            return device;
    }

    return nullptr;
}

bool send_power_on(libusb_device* device) {
    libusb_device_handle* handle;
    int r = libusb_open(device, &handle);
    if (r < 0)
    {
        fprintf(stderr, "failed to open device");
        return false;
    }

    r = libusb_claim_interface(handle, 0);
    if (r < 0)
    {
        fprintf(stderr, "failed to claim interface");
        return false;
    }

    // Get serial number
    unsigned char serial[256];
    r = libusb_get_string_descriptor_ascii(handle, 0x03, serial, sizeof(serial));

    if (r < 0)
    {
        fprintf(stderr, "failed to get serial number");
        return false;
    }

    uint32_t crc = ~0U;
    uint32_t snLen = strlen((char*)serial);
    uint8_t* sn = serial;

    while (snLen--)
    {
        crc = crc32_tab[(crc ^ *sn++) & 0xFF] ^ (crc >> 8);
    }

    unsigned char buffer[64];
    int actual_length;

    for (int k = 0; k < 10; ++k)
    {
        // Listen to events from the device
        r = libusb_interrupt_transfer(handle, 0x81, buffer, sizeof(buffer), &actual_length, 1000);

        if (r < 0) {
            return false;
        }

        if (actual_length <= 0)
            continue;

        if (buffer[0] != 0x03 || buffer[1] != 0xD0 || buffer[2] != 0x2C) {
            k--;
            continue;
        }
    }

    int crcLength = 2;
    uint32_t bufferOffset = sizeof(buffer) - crcLength;
    // CRC last two bytes of the buffer

    while (crcLength--)
    {
        crc = crc32_tab[(crc ^ buffer[bufferOffset++]) & 0xFF] ^ (crc >> 8);
    }

    crc = ~crc;

    memset(buffer, 0, sizeof(buffer));

    vive_control_t* control = (vive_control_t*)buffer;
    control->report_id = 0x04;
    control->command = 0x2978;
    control->length = 0x38;
    control->unknown2 = 0x08;
    control->crc[0] = (crc >> 25) | 0x80;
    control->crc[1] = crc >> 17;
    control->crc[2] = crc >> 9;

    // Request set feature
    r = libusb_control_transfer(
            handle, LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_ENDPOINT_OUT | LIBUSB_RECIPIENT_DEVICE,
            0x09, 0x04 | 0x03 << 8, 0, buffer, sizeof(buffer), 1000);
    if (r < 0)
    {
        printf("failed to send control transfer %s\n", libusb_error_name(r));
        return false;
    }

    //libusb_close(handle);
    return true;
}

void url_receiver(char const* url, void* user_data) {
    char* buffer = (char*)user_data;
    if( *buffer != '\0' ) return; // only keep first value

    if( strlen( url ) < 256 )
        strcpy( buffer, url );
}

void log(void* log_context, tobii_log_level_t level, char const* text) {
    printf("%s\n", text);
}

extern "C" EXPORT tobii_device_t* enable_eye_chip() {
    libusb_device** devices;
    int r;
    ssize_t cnt;

    r = libusb_init_context(nullptr, nullptr, 0);
    libusb_set_debug(nullptr, LIBUSB_LOG_LEVEL_WARNING);

    if (r < 0)
    {
        return nullptr;
    }

    cnt = libusb_get_device_list(nullptr, &devices);

    if (cnt < 0)
    {
        return nullptr;
    }

    libusb_device* device = find_device(devices, 0x0bb4, 0x0309);

    if (device == nullptr)
    {
        return nullptr;
    }

    if (!send_power_on(device))
    {
        return nullptr;
    }
    else
    {
        // Schedule keep alive thread that runs every 500ms
        _thread = new std::thread([&devices]() {
            libusb_device* device = find_device(devices, 0x0bb4, 0x0309);

            while (true)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                send_power_on(device);
            }
        });

        _thread->detach();
    }

    libusb_device* wait_device = nullptr;

    // Wait for the device to boot 0x2104:0x020f
    while (true)
    {
        cnt = libusb_get_device_list(nullptr, &devices);

        if (cnt < 0)
        {
            return nullptr;
        }

        wait_device = find_device(devices, 0x2104, 0x020f);

        if (wait_device != nullptr)
        {
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    if (!_has_loaded_stream_engine)
    {
#if defined(_WIN32)
        HMODULE module = LoadLibraryA("tobii_stream_engine.dll");
        if (module == nullptr)
        {
            printf("Failed to load tobii_stream_engine.dll\n");
            return nullptr;
        }

        tobii_api_create = (tobii_api_create_t*)GetProcAddress(module, "tobii_api_create");
        tobii_get_api_version = (tobii_get_api_version_t*)GetProcAddress(module, "tobii_get_api_version");
        tobii_enumerate_local_device_urls = (tobii_enumerate_local_device_urls_t*)GetProcAddress(module, "tobii_enumerate_local_device_urls");
        tobii_device_create = (tobii_device_create_t*)GetProcAddress(module, "tobii_device_create");
#else
        void* module = dlopen("libtobii_stream_engine.so", RTLD_LAZY);
        if (module == nullptr)
        {
            return nullptr;
        }

        tobii_api_create = (tobii_api_create_t*)dlsym(module, "tobii_api_create");
        tobii_get_api_version = (tobii_get_api_version_t*)dlsym(module, "tobii_get_api_version");
        tobii_enumerate_local_device_urls = (tobii_enumerate_local_device_urls_t*)dlsym(module, "tobii_enumerate_local_device_urls");
        tobii_device_create = (tobii_device_create_t*)dlsym(module, "tobii_device_create");
#endif

        _has_loaded_stream_engine = true;
    }

    tobii_api_t* api;

    if (tobii_api_create(&api, nullptr, nullptr) != 0)
    {
        printf("tobii_api_create failed\n");
        return nullptr;
    }

    char buffer[256] = { 0 };
    int retries = 10;

    while( retries-- > 0 && buffer[0] == '\0' )
    {
        if (tobii_enumerate_local_device_urls(api, url_receiver, buffer) != 0)
        {
            printf("tobii_enumerate_local_device_urls failed\n");
            return nullptr;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    if (strlen(buffer) == 0)
    {
        printf("No device found\n");
        return nullptr;
    }

    printf("Device URL: %s\n", buffer);

    tobii_device_t* tobii_device;
    int error = tobii_device_create(api, buffer, TOBII_FIELD_OF_USE_INTERACTIVE, &tobii_device);
    if (error != 0)
    {
        printf("tobii_device_create failed %d\n", error);
        return nullptr;
    }

    return tobii_device;
}