## prerequisites

- cmake (version 3.27 or higher)
- c++ compiler with C++23 support
- opencv (version 4.10 or higher)
- openssl (version 3.4.0 or higher)

## building from source

1. clone the repository:
```bash
git clone https://github.com/yourusername/ftv.git
cd ftv
```

2. create a build directory and compile:
```bash
mkdir build
cd build
cmake ..
make
```

3. (optional) install systemwide:
```bash
sudo make install
```

## usage

### hiding a file in video

```bash
ftv encrypt <input_file> -o <output_file> -k <key> [options]
```

### retrieving a file from Video

```bash
ftv decrypt <input_file> -k <key>
```

### options

| option | description | default |
|--------|-------------|---------|
| `-o, --output <file>` | video save location (required for encrypt) | - |
| `-k, --key <key>` | password (max 32 characters) | - |
| `-w, --width <pixels>` | video width (100-4096) | 300 |
| `-h, --height <pixels>` | video height (100-4096) | 300 |
| `-f, --fps <number>` | frames per second (1-60) | 30 |
| `--help` | show help message | - |

### Example Usage

encoding a file:
```bash
ftv encrypt file.txt -o video.avi -k mypassword -w 640 -h 480 -f 30
```

decoding a file:
```bash
ftv decrypt video.avi -k mypassword
```
