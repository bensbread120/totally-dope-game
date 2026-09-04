# totally-dope-game

# It is totally dope
# Indeed the dopeness here is immaculate

buidling on commandline:
```bash
mkdir build
```
The below command is using the place that my SDL3, SDL3_image, and glm libraries are installed. You will need to change the path to where you have them installed.
```bash
cmake -S . -D CMAKE_PREFIX_PATH="C:\\dev\\SDL3-3.4.14;C:\\dev\\SDL3_image-3.4.4;C:\\dev\\glm-1.0.3-VC" -B build/

cmake --build build/
```