# webkit2-osr
### WebKit2 Offscreen Rendering experiment with GTK Backend and GLFW + OpenGL

# Building

Once you clone the project, update all the submodules by running

```console
git submodule update --init --recursive
```

Pyhton and jinja2 module is required for generating the OpenGL API bindings

```console
pip install jinja2
```

Building the executable can be done with CMake

```console
mkdir -p build && cd build

cmake ..
cmake --build . --config Debug
```

For a production build, replace `Debug` on the CMake build command with `Release`
