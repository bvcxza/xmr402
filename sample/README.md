# Sample of static content access to a web service api protected by xmr402 reverse proxy

## Build xmr402

[Here](../README.md/#build)

## Run web service api

```
(cd sample/api && python3 service.py)
```

## Run xmr402 reverse proxy

```
. env.sh
run 0.0.0.0 8080 127.0.0.1 8888 Bearer ~/.local/share/xmr-stagenet-wallets/server 1234 0.01 3 150 STAGENET http://45.63.8.26:38081
```

## Run static content service

```
(cd sample/static && python3 -m http.server)
```

