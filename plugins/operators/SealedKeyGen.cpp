/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SealedKeyGen.cpp - Generate Curve25519 keypair for SealedEncryptionOperator.
 *
 *  Created on: Feb 18, 2026
 *      Author: Berk Geveci
 */

#include <cstdlib>
#include <fstream>
#include <iostream>

#include <sodium.h>

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <public_key_file> <secret_key_file>" << std::endl;
        std::cerr << std::endl;
        std::cerr << "Generates a Curve25519 keypair for use with SealedEncryptionOperator."
                  << std::endl;
        std::cerr << "Copy ONLY the public key file to shared/HPC systems." << std::endl;
        std::cerr << "Keep the secret key file private on your local machine." << std::endl;
        return EXIT_FAILURE;
    }

    if (sodium_init() < 0)
    {
        std::cerr << "Error: libsodium could not be initialized" << std::endl;
        return EXIT_FAILURE;
    }

    unsigned char pk[crypto_box_PUBLICKEYBYTES];
    unsigned char sk[crypto_box_SECRETKEYBYTES];
    crypto_box_keypair(pk, sk);

    // Write public key
    std::ofstream pkFile(argv[1], std::ios::binary);
    if (!pkFile)
    {
        std::cerr << "Error: could not open public key file for writing: " << argv[1] << std::endl;
        sodium_memzero(sk, crypto_box_SECRETKEYBYTES);
        return EXIT_FAILURE;
    }
    pkFile.write(reinterpret_cast<const char *>(pk), crypto_box_PUBLICKEYBYTES);
    pkFile.close();

    // Write secret key
    std::ofstream skFile(argv[2], std::ios::binary);
    if (!skFile)
    {
        std::cerr << "Error: could not open secret key file for writing: " << argv[2] << std::endl;
        sodium_memzero(sk, crypto_box_SECRETKEYBYTES);
        return EXIT_FAILURE;
    }
    skFile.write(reinterpret_cast<const char *>(sk), crypto_box_SECRETKEYBYTES);
    skFile.close();

    // Print hex-encoded keys for convenient use with env vars / params
    char pkHex[crypto_box_PUBLICKEYBYTES * 2 + 1];
    char skHex[crypto_box_SECRETKEYBYTES * 2 + 1];
    sodium_bin2hex(pkHex, sizeof(pkHex), pk, crypto_box_PUBLICKEYBYTES);
    sodium_bin2hex(skHex, sizeof(skHex), sk, crypto_box_SECRETKEYBYTES);

    // Wipe secret key from memory
    sodium_memzero(sk, crypto_box_SECRETKEYBYTES);

    std::cout << "Keypair generated successfully:" << std::endl;
    std::cout << "  Public key file:  " << argv[1] << " (" << crypto_box_PUBLICKEYBYTES << " bytes)"
              << std::endl;
    std::cout << "  Secret key file:  " << argv[2] << " (" << crypto_box_SECRETKEYBYTES << " bytes)"
              << std::endl;
    std::cout << std::endl;
    std::cout << "Hex-encoded keys (for use with params or env vars):" << std::endl;
    std::cout << "  Public key:  " << pkHex << std::endl;
    std::cout << "  Secret key:  " << skHex << std::endl;
    std::cout << std::endl;
    std::cout << "Copy ONLY the public key to shared/HPC systems for writing." << std::endl;
    std::cout << "Keep the secret key private for reading." << std::endl;

    // Wipe hex secret key from memory
    sodium_memzero(skHex, sizeof(skHex));

    return EXIT_SUCCESS;
}
