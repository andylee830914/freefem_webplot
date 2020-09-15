# FreeFEM webplot
A Web-based Interactive Visualization Module for FreeFEM

Demo : [https://freefem.andylee.tw](https://freefem.andylee.tw)

## Usage
### Compile
```
ff-c++ webplot.cpp
clang++ -bundle -undefined dynamic_lookup -g webplot.o -o ./webplot.dylib
```
### Use in FreeFEM
```
load "webplot"
webplot(u,Th);
server();
```
### Function Usage
#### `webplot()`
Drawing `mesh Th` only.
```
webplot(mesh Th [, options]);
```

Drawing both `mesh Th` and `FE function u`.
```
webplot(Vh u, mesh Th [, options]);
```

Options

| type| option | function |
|---|---|---|
|string| cmm | comment shown on the graph |

#### `server()`
Start server
```
server([, options]);
```

Options

| type| option | function | default value |
|---|---|---|---|
|string| host | run server on the IP address | "localhost" |
| int | port | run server on the port | 1234 |
