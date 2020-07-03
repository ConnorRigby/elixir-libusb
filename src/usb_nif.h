#ifndef USB_NIF_H
#define USB_NIF_H

#include <libusb-1.0/libusb.h>
#include "erl_nif.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#define DEBUG

#ifdef DEBUG
#define log_location stderr
//#define LOG_PATH "/tmp/circuits_gpio.log"
#define debug(...) do { enif_fprintf(log_location, __VA_ARGS__); enif_fprintf(log_location, "\r\n"); fflush(log_location); } while(0)
#define error(...) do { debug(__VA_ARGS__); } while (0)
#define start_timing() ErlNifTime __start = enif_monotonic_time(ERL_NIF_USEC)
#define elapsed_microseconds() (enif_monotonic_time(ERL_NIF_USEC) - __start)
#else
#define debug(...)
#define error(...) do { enif_fprintf(stderr, __VA_ARGS__); enif_fprintf(stderr, "\n"); } while(0)
#define start_timing()
#define elapsed_microseconds() 0
#endif

struct usb_priv {
    ERL_NIF_TERM atom_ok;
    ErlNifResourceType *usb_rt;
    libusb_context* context;
    int usbs_open;
};

struct usb_rt {
    ErlNifPid pid;
    libusb_device_handle* handle;
};

// nif_utils.c
ERL_NIF_TERM make_ok_tuple(ErlNifEnv *env, ERL_NIF_TERM value);
ERL_NIF_TERM make_error_tuple(ErlNifEnv *env, const char *reason);
int enif_get_boolean(ErlNifEnv *env, ERL_NIF_TERM term, bool *v);


#endif // USB_NIF_H
