# Glo_v1
firmware for Glo the Polyphonic Whale, ESP32 based pocket synth ( http://phonicbloom.com/ )

## Build from sources

### Build with Docker

1. Build image
``` sh
docker build -t idf-glo-v1 .
```

2. To build in container you'd have to change python binary to `python2.7` in
   sdkconfig file

```
-CONFIG_PYTHON="python"
+CONFIG_PYTHON="python2.7"

```

3. Run container interactive
``` sh
docker run -v $(pwd):/usr/glo --tty --interactive idf-glo-v1
```

3. compile inside container
``` sh
cd usr/glo
make all
```


