# libsawyer
Common code shared between [OpenRCT2](https://github.com/OpenRCT2/OpenRCT2) and [OpenLoco](https://github.com/OpenLoco/OpenLoco).

## Build
```
cmake -G Ninja -B bin
cmake --build bin
cmake --install bin
```

## Build and run tests
```
cmake --build bin --target tests
bin/tests
```

## Build fsaw
```
cd tools/fsaw
cmake -G Ninja -B bin
cmake --build bin
```

## Licence
**libsawyer** is licensed under the MIT License.
