**********
Encryption
**********

The Encryption Operator uses the :ref:`Plugins` interface and requires
`libsodium <https://doc.libsodium.org/>`_.  If ADIOS2 finds libsodium at
configure time the plugin is built automatically.

The operator supports two modes.  Mode is selected by which key sources are
present (parameters or environment variables); asymmetric mode takes priority.

~~~~~~~~~~~~~~
Symmetric Mode
~~~~~~~~~~~~~~

Uses **XSalsa20-Poly1305** (``crypto_secretbox``).  A single 32-byte secret
key is shared between writer and reader.  For each variable chunk a fresh
random 24-byte nonce is generated; the ciphertext carries a 16-byte Poly1305
MAC.  On-disk layout::

    [ uint64 original_size | 24 B nonce | ciphertext + 16 B MAC ]

Key lookup order (first match wins):

1. ``ADIOS2_SECRET_KEY`` environment variable (hex string)
2. ``SecretKeyFile`` parameter — path to a 32-byte binary key file.
   On the writer side the file is created if it does not exist.
3. ``SecretKey`` parameter (hex string)

============================== ===================== ===========================================================
 **Parameter**                  **Value**              **Notes**
============================== ===================== ===========================================================
 PluginName                     string                Required
 PluginLibrary                  ``EncryptionOperator`` Required
 SecretKeyFile                  file path             Key file (generated if absent on write side)
 SecretKey                      64-char hex           Alternative to file
============================== ===================== ===========================================================

~~~~~~~~~~~~~~~
Asymmetric Mode
~~~~~~~~~~~~~~~

Uses a **hybrid** scheme: **Curve25519** key agreement + **XSalsa20-Poly1305**
bulk cipher.

For each variable chunk:

1. A random 32-byte **session key** is generated.
2. The session key is encrypted with ``crypto_box_seal`` — an anonymous ECDH
   construction using an ephemeral sender keypair.  Only the recipient's public
   key is required to seal; both keys are required to unseal.
3. The chunk data is encrypted with the session key via ``crypto_secretbox``.
4. The session key is wiped from memory.

On-disk layout::

    [ uint64 original_size | 48 B sealed_session_key | 24 B nonce | ciphertext + 16 B MAC ]

The 48-byte sealed session key is ``crypto_box_SEALBYTES (32) + 32``.

**Writer** only needs the **public key**.
**Reader** needs the **secret key** (public key can be derived automatically).

Key pairs can be generated with the ``adios2_seal_keygen`` utility.

Public key lookup order (first match wins):

1. ``ADIOS2_PUBLIC_KEY_FILE`` environment variable (file path)
2. ``ADIOS2_PUBLIC_KEY`` environment variable (64-char hex)
3. ``PublicKeyFile`` parameter
4. ``PublicKey`` parameter (hex)

Secret key lookup order (first match wins):

1. ``ADIOS2_ASYM_SECRET_KEY_FILE`` environment variable (file path)
2. ``ADIOS2_ASYM_SECRET_KEY`` environment variable (64-char hex)
3. ``SecretKeyFile`` parameter
4. ``SecretKey`` parameter (hex)

If a secret key is provided without a public key, the public key is derived
automatically via ``crypto_scalarmult_base`` (Curve25519 base-point multiply).
Setting any of the four asymmetric environment variables activates asymmetric
mode even when no parameters are passed.

============================== ===================== ===========================================================
 **Parameter**                  **Value**              **Notes**
============================== ===================== ===========================================================
 PluginName                     string                Required
 PluginLibrary                  ``EncryptionOperator`` Required
 PublicKeyFile                  file path             32-byte Curve25519 public key (writer / reader)
 PublicKey                      64-char hex           Alternative to file
 SecretKeyFile                  file path             32-byte Curve25519 secret key (reader)
 SecretKey                      64-char hex           Alternative to file
============================== ===================== ===========================================================
