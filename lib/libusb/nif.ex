defmodule LibUSB.Nif do
  @on_load {:load_nif, 0}
  @compile {:autoload, false}

  def load_nif() do
    nif_binary = Application.app_dir(:libusb, ["priv", "usb_nif"])

    :erlang.load_nif(to_charlist(nif_binary), 0)
  end

  def open(_vid, _pid) do
    :erlang.nif_error(:nif_not_loaded)
  end

  def close(_usb) do
    :erlang.nif_error(:nif_not_loaded)
  end

  def get_device_list() do
    :erlang.nif_error(:nif_not_loaded)
  end

  def control_transfer(_usb, _request_type, _request, _value, _index, _data, _timeout) do
    :erlang.nif_error(:nif_not_loaded)
  end

  def bulk_transfer(_usb, _endpoint, _data, _timeout) do
    :erlang.nif_error(:nif_not_loaded)
  end
end
