This is the firmware source code for following devices:

Glo the Polyphonic Whale v1 (year 2019) 
Gecho Loopsynth v2 and Glo the Polyphonic Whale v2 (year 2024)

To select for which model to compile, change compiler conditions in the file board.h
(as this update is primarily for the Loopsynth, the build for Glo Whale 
was not tested and it might not compile without small modifications)

The newer code (from 2024) should reliably compile against ESP-IDF version 4.4,
we used commit a49e0180ee638e41876a8f7bc6428a983fc69d66 from 18/08/2023 08:33:19

Find more information at:
http://phonicbloom.com/
http://gechologic.com/

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


