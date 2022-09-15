## Secure bootloader private key

The private key for the secure bootloader is stored here as an
encrypted file, and must be decrypted before building the nRF52810
application. The private key is encrypted using AES256 symmetric
encryption.

The public key half of the keypair is in [dfu_public_key.c](./dfu/dfu_public_key.c).

### Decrypting the private key


```
gpg --output private-key.pem --no-symkey-cache --decrypt private-key.pem.gpg
```

### Encrypting the private key

If a new keypair needs to be generated (this should happen rarely, if
ever), encrypt the private key using:

```
gpg --symmetric --no-symkey-cache --cipher-algo AES256 private-key.pem
```

Note that the `.gitignore` in this directory attempts to prevent you
from checking in the unencrypted `private-key.pem` file--the
unencrypted file should never be checked in.
