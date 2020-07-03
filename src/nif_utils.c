#include "usb_nif.h"

#include <string.h>

ERL_NIF_TERM make_ok_tuple(ErlNifEnv *env, ERL_NIF_TERM value)
{
    struct usb_priv *priv = enif_priv_data(env);

    return enif_make_tuple2(env, priv->atom_ok, value);
}

ERL_NIF_TERM make_error_tuple(ErlNifEnv *env, const char *reason)
{
    ERL_NIF_TERM error_atom = enif_make_atom(env, "error");
    ERL_NIF_TERM reason_atom = enif_make_atom(env, reason);

    return enif_make_tuple2(env, error_atom, reason_atom);
}
