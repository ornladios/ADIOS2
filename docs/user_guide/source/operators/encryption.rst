**********
Encryption
**********

The Encryption Operator uses the :ref:`Plugins` interface.
This operator uses `libsodium <https://doc.libsodium.org/>`_ for encrypting and decrypting data.
If ADIOS can find libsodium at configure time, this plugin will be built.

The encryption mode is selected automatically based on the parameters supplied.

Symmetric Mode
==============

In symmetric mode, the operator generates a secret key and encrypts the data with the key and a
nonce as described in the libsodium `secret key cryptography docs
<https://doc.libsodium.org/secret-key_cryptography/secretbox>`_.
The key is saved to the specified ``SecretKeyFile`` and will be used for decryption. The key should
be kept confidential since it is used to both encrypt and decrypt the data.

Symmetric mode is activated when ``SecretKeyFile`` is provided without any public-key parameters.

============================== ===================== ===========================================================
 **Key**                       **Value Format**       **Explanation**
============================== ===================== ===========================================================
 PluginName                     string                Required. Name to refer to plugin, e.g., ``MyOperator``
 PluginLibrary                  string                Required. Name of shared library, ``EncryptionOperator``
 SecretKeyFile                  string                Required. Path to secret key file
============================== ===================== ===========================================================

Asymmetric Mode
===============

In asymmetric mode, a random per-write session key is sealed with the recipient's Curve25519 public
key using ``crypto_box_seal``, and the bulk data is encrypted with that session key via
``crypto_secretbox``. Only the public key is needed for writing; both the public and secret keys
are needed for reading.

Asymmetric mode is activated when ``PublicKeyFile`` or ``PublicKey`` is provided. Key pairs can
be generated with the ``adios2_seal_keygen`` utility.

The secret key lookup order for reading is: ``SecretKeyFile`` parameter, ``SecretKey`` parameter,
``ADIOS2_SECRET_KEY_FILE`` environment variable, ``ADIOS2_SECRET_KEY`` environment variable.
If only the secret key is supplied, the public key is derived automatically via Curve25519.

============================== ===================== ===========================================================
 **Key**                       **Value Format**       **Explanation**
============================== ===================== ===========================================================
 PluginName                     string                Required. Name to refer to plugin, e.g., ``MyOperator``
 PluginLibrary                  string                Required. Name of shared library, ``EncryptionOperator``
 PublicKeyFile                  string                Path to a 32-byte Curve25519 public key file
 PublicKey                      string                Hex-encoded public key (64 hex characters)
 SecretKeyFile                  string                Path to a 32-byte Curve25519 secret key file (reading)
 SecretKey                      string                Hex-encoded secret key (64 hex characters, reading only)
============================== ===================== ===========================================================
