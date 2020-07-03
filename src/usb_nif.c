#include <libusb-1.0/libusb.h>
#include "usb_nif.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

static void release_usb_ctx(struct usb_priv *priv, struct usb_rt *usb)
{
  // if(usb->handle) {
  //   debug("closing handle\r\n");
  //   libusb_close(usb->handle);
  // }

  if (priv->context)
  {
    debug("closing context\r\n");
    libusb_exit(priv->context);
  }
}

static void usb_rt_dtor(ErlNifEnv *env, void *obj)
{
  struct usb_priv *priv = enif_priv_data(env);
  struct usb_rt *usb = (struct usb_rt *)obj;

  debug("usb_rt_dtor called");

  release_usb_ctx(priv, usb);
}

static void usb_rt_stop(ErlNifEnv *env, void *obj, int fd, int is_direct_call)
{
  (void)env;
  (void)obj;
  (void)fd;
  (void)is_direct_call;
  //struct usb_priv *priv = enif_priv_data(env);
#ifdef DEBUG
  struct usb_rt *usb = (struct usb_rt *)obj;
  (void)usb;
  debug("usb_rt_stop called %s", (is_direct_call ? "DIRECT" : "LATER"));
#endif
}

static void usb_rt_down(ErlNifEnv *env, void *obj, ErlNifPid *pid, ErlNifMonitor *monitor)
{
  (void)env;
  (void)obj;
  (void)pid;
  (void)monitor;
#ifdef DEBUG
  struct usb_rt *usb = (struct usb_rt *)obj;
  (void)usb;
  debug("usb_rt_down called on");
#endif
}

static ErlNifResourceTypeInit usb_rt_init = {usb_rt_dtor, usb_rt_stop, usb_rt_down};

static int load(ErlNifEnv *env, void **priv_data, ERL_NIF_TERM info)
{
  (void)info;
#ifdef DEBUG
#ifdef LOG_PATH
  log_location = fopen(LOG_PATH, "w");
#endif
#endif
  debug("load");

  struct usb_priv *priv = enif_alloc(sizeof(struct usb_priv));
  if (!priv)
  {
    error("Can't allocate usb_priv");
    return 1;
  }

  priv->usbs_open = 0;
  priv->atom_ok = enif_make_atom(env, "ok");

  priv->usb_rt = enif_open_resource_type_x(env, "usb_rt", &usb_rt_init, ERL_NIF_RT_CREATE, NULL);

  *priv_data = (void *)priv;
  return 0;
}

static void unload(ErlNifEnv *env, void *priv_data)
{
  (void)env;

  struct usb_priv *priv = priv_data;
  (void)priv;
  debug("unload");
}

static ERL_NIF_TERM open_usb(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
  struct usb_priv *priv = enif_priv_data(env);
  int r, vendor_id, product_id;

  if (argc != 2 ||
      !enif_get_int(env, argv[0], &vendor_id) ||
      !enif_get_int(env, argv[1], &product_id))
    return enif_make_badarg(env);

  struct usb_rt *usb = enif_alloc_resource(priv->usb_rt, sizeof(struct usb_rt));
  r = libusb_init(&priv->context);
  if (r != LIBUSB_SUCCESS)
  {
    debug("libusb init failed %s\r\n", libusb_error_name(r));
    return make_error_tuple(env, "libusb_init_fail");
  }

  usb->handle = libusb_open_device_with_vid_pid(priv->context, vendor_id, product_id);
  if (!usb->handle)
  {
    libusb_exit(priv->context);
    return make_error_tuple(env, "libusb_dev_open_fail");
  }

  // Transfer ownership of the resource to Erlang so that it can be garbage collected.
  ERL_NIF_TERM usb_resource = enif_make_resource(env, usb);
  enif_release_resource(usb);

  priv->usbs_open++;

  return make_ok_tuple(env, usb_resource);
}

static ERL_NIF_TERM close_usb(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
  struct usb_priv *priv = enif_priv_data(env);
  struct usb_rt *usb;
  if (argc != 1 ||
      !enif_get_resource(env, argv[0], priv->usb_rt, (void **)&usb))
    return enif_make_badarg(env);

  release_usb_ctx(priv, usb);

  return priv->atom_ok;
}

static ERL_NIF_TERM control_transfer(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
  struct usb_priv *priv = enif_priv_data(env);
  struct usb_rt *usb;
  int r, timeout;
  int bmRequestType, bRequest, wValue, wIndex, wLength;
  unsigned char *raw_transfer;

  ErlNifBinary bin_transfer;
  ERL_NIF_TERM transfer_term;

  if (argc != 7 ||
      !enif_get_resource(env, argv[0], priv->usb_rt, (void **)&usb) ||
      !enif_get_int(env, argv[1], &bmRequestType) ||
      !enif_get_int(env, argv[2], &bRequest) ||
      !enif_get_int(env, argv[3], &wValue) ||
      !enif_get_int(env, argv[4], &wIndex) ||
      !enif_get_int(env, argv[6], &timeout))
    return enif_make_badarg(env);

  if (enif_is_binary(env, argv[5]))
  {
    if (!enif_inspect_binary(env, argv[5], &bin_transfer))
      return enif_make_badarg(env);

    r = libusb_control_transfer(usb->handle, bmRequestType, bRequest, wValue, wIndex, bin_transfer.data, bin_transfer.size, timeout);
    if (r < 0)
      return make_error_tuple(env, libusb_error_name(r));

    return priv->atom_ok;
  }
  else
  {
    if (!enif_get_int(env, argv[5], &wLength))
      return enif_make_badarg(env);
    raw_transfer = enif_make_new_binary(env, wLength, &transfer_term);

    r = libusb_control_transfer(usb->handle, bmRequestType, bRequest, wValue, wIndex, raw_transfer, wLength, timeout);
    if (r < 0)
      return make_error_tuple(env, libusb_error_name(r));
    return make_ok_tuple(env, transfer_term);
  }
}

static ERL_NIF_TERM bulk_transfer(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
  struct usb_priv *priv = enif_priv_data(env);
  struct usb_rt *usb;
  int r, endpoint, wLength, transfered, timeout;
  unsigned char *raw_transfer;
  ErlNifBinary bin_transfer;
  ERL_NIF_TERM transfer_term;

  if (argc != 4 ||
      !enif_get_resource(env, argv[0], priv->usb_rt, (void **)&usb) ||
      !enif_get_int(env, argv[1], &endpoint) ||
      !enif_get_int(env, argv[3], &timeout))
    return enif_make_badarg(env);

  if (enif_is_binary(env, argv[2]))
  {
    if (!enif_inspect_binary(env, argv[2], &bin_transfer))
      return enif_make_badarg(env);

    r = libusb_bulk_transfer(usb->handle, endpoint, bin_transfer.data, bin_transfer.size, &transfered, timeout);

    if (r < 0)
      return make_error_tuple(env, libusb_error_name(r));

    return make_ok_tuple(env, enif_make_int(env, transfered));
  }
  else
  {
    if (!enif_get_int(env, argv[2], &wLength))
      return enif_make_badarg(env);

    raw_transfer = enif_make_new_binary(env, wLength, &transfer_term);

    r = libusb_bulk_transfer(usb->handle, endpoint, raw_transfer, wLength, &transfered, timeout);
    if (r < 0)
      return make_error_tuple(env, libusb_error_name(r));
    return make_ok_tuple(env, transfer_term);
  }
}

static ERL_NIF_TERM get_device_list(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
  struct usb_priv *priv = enif_priv_data(env);
  (void)priv;

  libusb_device **devs;
  ssize_t cnt;
  int r, i, j;

  r = libusb_init(NULL);
  if (r < 0)
    return make_error_tuple(env, "libusb_error");

  cnt = libusb_get_device_list(NULL, &devs);
  if (cnt < 0)
    return make_error_tuple(env, "libusb_error");

  ERL_NIF_TERM devices_term[cnt];
  struct libusb_device_descriptor desc;
  libusb_device_handle *handle = NULL;

  for (i = 0; devs[i]; i++)
  {
    devices_term[i] = enif_make_new_map(env);
    libusb_device *dev = devs[i];

    r = libusb_get_device_descriptor(dev, &desc);
    if (r < 0)
      return make_error_tuple(env, "failed to get device descriptor");

    enif_make_map_put(env, devices_term[i], enif_make_atom(env, "bus_number"), enif_make_int(env, libusb_get_bus_number(dev)), &devices_term[i]);
    enif_make_map_put(env, devices_term[i], enif_make_atom(env, "device_address"), enif_make_int(env, libusb_get_device_address(dev)), &devices_term[i]);

    enif_make_map_put(env, devices_term[i], enif_make_atom(env, "bLength"), enif_make_int(env, desc.bLength), &devices_term[i]);
    enif_make_map_put(env, devices_term[i], enif_make_atom(env, "bDescriptorType"), enif_make_int(env, desc.bDescriptorType), &devices_term[i]);
    enif_make_map_put(env, devices_term[i], enif_make_atom(env, "bcdUSB"), enif_make_int(env, desc.bcdUSB), &devices_term[i]);
    enif_make_map_put(env, devices_term[i], enif_make_atom(env, "bDeviceClass"), enif_make_int(env, desc.bDeviceClass), &devices_term[i]);
    enif_make_map_put(env, devices_term[i], enif_make_atom(env, "bDeviceSubClass"), enif_make_int(env, desc.bDeviceSubClass), &devices_term[i]);
    enif_make_map_put(env, devices_term[i], enif_make_atom(env, "bDeviceProtocol"), enif_make_int(env, desc.bDeviceProtocol), &devices_term[i]);
    enif_make_map_put(env, devices_term[i], enif_make_atom(env, "bMaxPacketSize0"), enif_make_int(env, desc.bMaxPacketSize0), &devices_term[i]);
    enif_make_map_put(env, devices_term[i], enif_make_atom(env, "idVendor"), enif_make_int(env, desc.idVendor), &devices_term[i]);
    enif_make_map_put(env, devices_term[i], enif_make_atom(env, "idProduct"), enif_make_int(env, desc.idProduct), &devices_term[i]);
    enif_make_map_put(env, devices_term[i], enif_make_atom(env, "bcdDevice"), enif_make_int(env, desc.bcdDevice), &devices_term[i]);
    enif_make_map_put(env, devices_term[i], enif_make_atom(env, "iManufacturer"), enif_make_int(env, desc.iManufacturer), &devices_term[i]);
    enif_make_map_put(env, devices_term[i], enif_make_atom(env, "iProduct"), enif_make_int(env, desc.iProduct), &devices_term[i]);
    enif_make_map_put(env, devices_term[i], enif_make_atom(env, "iSerialNumber"), enif_make_int(env, desc.iSerialNumber), &devices_term[i]);
    enif_make_map_put(env, devices_term[i], enif_make_atom(env, "bNumConfigurations"), enif_make_int(env, desc.bNumConfigurations), &devices_term[i]);

    ERL_NIF_TERM configs_term[desc.bNumConfigurations];
    for (j = 0; j < desc.bNumConfigurations; j++)
    {
      struct libusb_config_descriptor *config;
      configs_term[j] = enif_make_new_map(env);

      r = libusb_get_config_descriptor(dev, i, &config);
      if (r != LIBUSB_SUCCESS)
      {
        debug("Couldn't retrieve descriptors %s\n", libusb_error_name(r));
        continue;
      }

      enif_make_map_put(env, configs_term[j],
                        enif_make_atom(env, "bLength"),
                        enif_make_int(env, config->bLength),
                        &configs_term[j]);

      enif_make_map_put(env, configs_term[j],
                        enif_make_atom(env, "bDescriptorType"),
                        enif_make_int(env, config->bDescriptorType),
                        &configs_term[j]);

      enif_make_map_put(env, configs_term[j],
                        enif_make_atom(env, "wTotalLength"),
                        enif_make_int(env, config->wTotalLength),
                        &configs_term[j]);

      enif_make_map_put(env, configs_term[j],
                        enif_make_atom(env, "bNumInterfaces"),
                        enif_make_int(env, config->bNumInterfaces),
                        &configs_term[j]);

      enif_make_map_put(env, configs_term[j],
                        enif_make_atom(env, "bConfigurationValue"),
                        enif_make_int(env, config->bConfigurationValue),
                        &configs_term[j]);

      enif_make_map_put(env, configs_term[j],
                        enif_make_atom(env, "iConfiguration"),
                        enif_make_int(env, config->iConfiguration),
                        &configs_term[j]);

      enif_make_map_put(env, configs_term[j],
                        enif_make_atom(env, "bmAttributes"),
                        enif_make_int(env, config->bmAttributes),
                        &configs_term[j]);

      enif_make_map_put(env, configs_term[j],
                        enif_make_atom(env, "MaxPower"),
                        enif_make_int(env, config->MaxPower),
                        &configs_term[j]);

      libusb_free_config_descriptor(config);
    }
    enif_make_map_put(env,
                      devices_term[i],
                      enif_make_atom(env, "configurations"),
                      enif_make_list_from_array(env, configs_term, desc.bNumConfigurations),
                      &devices_term[i]);

    if (handle)
      libusb_close(handle);
    handle = NULL;
  }
  libusb_free_device_list(devs, 1);

  libusb_exit(NULL);
  return enif_make_list_from_array(env, devices_term, cnt);
}

static ErlNifFunc nif_funcs[] = {
    {"open", 2, open_usb, ERL_NIF_DIRTY_JOB_IO_BOUND},
    {"close", 1, close_usb, 0},
    {"control_transfer", 7, control_transfer, ERL_NIF_DIRTY_JOB_IO_BOUND},
    {"bulk_transfer", 4, bulk_transfer, ERL_NIF_DIRTY_JOB_IO_BOUND},
    {"get_device_list", 0, get_device_list, 0}};

ERL_NIF_INIT(Elixir.LibUSB.Nif, nif_funcs, load, NULL, NULL, unload)
