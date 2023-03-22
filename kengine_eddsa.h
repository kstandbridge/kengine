#ifndef KENGINE_EDDSA_H

// NOTE(kstandbridge): This is based on http://ed25519.cr.yp.to/

// Heavily inspired by https://github.com/orlp/ed25519 however the code has been
// modified extensively and does not represent the work of the origional author

typedef u8 ed25519_private_key[64];
typedef u8 ed25519_public_key[32];
typedef u8 ed25519_signature[64];

typedef struct ed25519_key_pair
{
    ed25519_public_key Public;
    ed25519_private_key Private;
} ed25519_key_pair;

b32
Ed25519Verify(ed25519_signature Signature, string Message, ed25519_public_key Public);

ed25519_key_pair
Ed25519CreateKeyPair(ed25519_private_key Persistent);

void
Ed25519Sign(ed25519_signature Signature, string Message, ed25519_key_pair KeyPair);

#define KENGINE_EDDSA_H
#endif //KENGINE_EDDSA_H
