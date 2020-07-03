# LibUSB

## Usage

This project is still very much a work in progress.

### get_device_list

```elixir
iex(1)> LibUSB.Nif.get_device_list()
[
  %{
    bDescriptorType: 1,
    bDeviceClass: 0,
    bDeviceProtocol: 0,
    bDeviceSubClass: 0,
    bLength: 18,
    bMaxPacketSize0: 64,
    bNumConfigurations: 1,
    bcdDevice: 257,
    bcdUSB: 272,
    bus_number: 2,
    configurations: [
      %{
        MaxPower: 50,
        bConfigurationValue: 0,
        bDescriptorType: 2,
        bLength: 9,
        bNumInterfaces: 4,
        bmAttributes: 224,
        iConfiguration: 0,
        wTotalLength: 269
      }
    ],
    device_address: 5,
    iManufacturer: 1,
    iProduct: 2,
    iSerialNumber: 3,
    idProduct: 22532,
    idVendor: 2652
  }
]
```

### control_transfer read/write

```elixir
iex(1)> {:ok, usb} = LibUSB.Nif.open(0x1915, 0x7777)
{:ok, #Reference<0.2476641389.3169189905.136487>}
iex(2)>
```

### bulk_transfer read/write

```elixir
iex(1)> {:ok, usb} = LibUSB.Nif.open(0x1915, 0x7777)
{:ok, #Reference<0.2476641389.3169189905.136487>}
iex(2)> LibUSB.Nif.bulk_transfer(usb, 1, <<0x00>>, 1000)
{:ok, 1}
iex(3)> LibUSB.Nif.bulk_transfer(usb, 0x81, 64, 1000)
{:ok,
 <<0, 5, 0, 0, 0, 0, 0, 0, 177, 134, 126, 152, 243, 127, 0, 0, 47, 4, 0, 0, 0,
   0, 0, 0, 193, 134, 126, 152, 243, 127, 0, 0, 255, 5, 0, 0, 0, 0, 0, 0, 209,
   134, 126, 152, 243, 127, 0, 0, ...>>}
iex(4)>
```

## Installation

If [available in Hex](https://hex.pm/docs/publish), the package can be installed
by adding `libusb` to your list of dependencies in `mix.exs`:

```elixir
def deps do
  [
    {:libusb, "~> 0.1.0"}
  ]
end
```

Documentation can be generated with [ExDoc](https://github.com/elixir-lang/ex_doc)
and published on [HexDocs](https://hexdocs.pm). Once published, the docs can
be found at [https://hexdocs.pm/libusb](https://hexdocs.pm/libusb).

