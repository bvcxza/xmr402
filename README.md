# XMR402

A HTTP [402](https://datatracker.ietf.org/doc/html/rfc7231#section-6.5.2) reverse proxy authentication server that utilizes [Monero](https://www.getmonero.org) Transaction Proof Authentication Token for paid APIs.

## Protocol

TODO

## Build

```
cd xmr402
git submodule update --init --force --recursive
mamba env update -n $(basename $PWD) -f conda.yml
. env.sh
bld_xmr && cnf && bld
```

## Run

### Create STAGENET Wallets

```
mkdir ~/.local/share/xmr-stagenet-wallets
wallet --stagenet --daemon-address 45.63.8.26:38081 --generate-new-wallet ~/.local/share/xmr-stagenet-wallets/client
```

```
wallet --stagenet --daemon-address 45.63.8.26:38081 --generate-new-wallet ~/.local/share/xmr-stagenet-wallets/server
Password: 1234
```

### Run Main Web Service (simulation)

```
python3 -m http.server
```

### Run XMR402 Proxy

```
run 0.0.0.0 8080 127.0.0.1 8000 ~/.local/share/xmr-stagenet-wallets/server 1234 0.01 3 150 STAGENET http://45.63.8.26:38081
```

### Test No Payment Request

```
curl -v http://127.0.0.1:8080
```

### Fund Client Wallet

https://community.rino.io/faucet/stagenet/
https://stagenet-faucet.xmr-tw.org/

### Pay the Server

```
wallet --stagenet --daemon-address 45.63.8.26:38081 --wallet ~/.local/share/xmr-stagenet-wallets/client
transfer 59gqHxPdgjC3vWKxubBCDNZuCiMB3UjXNYHbT7ojjjd6KiLyCjiqtcp8T3js5d4h58Xf815FUZoaRDrqpBiejKz3JcZczbV 0.01
get_tx_proof 59632c9de3453907ada348fe47233d6c6648a2d21b08d716fad54c40ee0c1cac 59gqHxPdgjC3vWKxubBCDNZuCiMB3UjXNYHbT7ojjjd6KiLyCjiqtcp8T3js5d4h58Xf815FUZoaRDrqpBiejKz3JcZczbV
```

```
cat monero_tx_proof ; echo
```

### Redo the Request with Token

```
curl -v -H "Authorization: Bearer 59632c9de3453907ada348fe47233d6c6648a2d21b08d716fad54c40ee0c1cac:OutProofV2jHQwEHcvw8uYSJcCCFzdMZh3odi1uqJXaYUv7aTPg2i8M1XnwSWcuDw5Jz1CzWjBM5QhvnxwkFZT8dhoGZoG3GG6Gx2ki9u2isuTAkncMLGFukMNgYgrzKEZUd9Wyf5e9MCh" http://127.0.0.1:8080
```

## Donations

Please consider donating to support the development of this project.

[Monero (XMR)](https://www.getmonero.org): 8AADjm5nz4GXXn7Tf6FNfwCaAjAdkvdUs5KgRwBGUj2NHwWqkxbfLzYPom3mL6a1cN1aypyfvyzaxHAM8aARbafFKkABT6Z

