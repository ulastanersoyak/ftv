# ftv

a tool that hides files inside video files. it encrypts the given data and stores it as black and white blocks in video frames.

## prerequisites

- cmake 3.22+
- c++ compiler with c++23 support (gcc 14+ recommended)
- opencv 4.10+
- openssl 3.2+ (for argon2id)
- libcurl
- nlohmann_json

## build

```bash
git clone git@github.com:ulastanersoyak/ftv.git
cd ftv
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

run the tests:
```bash
ctest --test-dir build --output-on-failure
```

## usage

hide a file in a video:
```bash
ftv encrypt file.txt -o video.avi -k mypassword -w 640 -H 480 -f 30
```

get your file back (restored under its original name in the current directory):
```bash
ftv decrypt video.avi -k mypassword
```

## options

- `-o, --output <path>`    - where to write the result (required for encrypt, decrypt defaults to the original file name)
- `-k, --key <key>`        - password (required)
- `-w, --width <pixels>`   - video width (100-4096, default: 300)
- `-H, --height <pixels>`  - video height (100-4096, default: 300)
- `-f, --fps <number>`     - frames per second (1-60, default: 30)
- `-b, --block <pixels>`   - side of the square that stores one bit (1, 2, 4 or 8, default: 4)
- `-h, --help`             - show help

decrypt never overwrites an existing file, pass `-o` to pick another path.

## how it works

1. the file name (without directories) and contents are encrypted with aes-256-gcm. the key is derived from the password with argon2id and a random salt, so the same password never gives the same key twice.
2. the encrypted data is split into reed-solomon (255, 223) codewords, which are interleaved so damage to a region of a frame, or a whole frame, is spread thinly over every codeword. each codeword repairs up to 16 damaged bytes.
3. a small header (magic, format version, block size, data size), protected with its own reed-solomon parity, is written first. then every bit becomes a white (1) or black (0) square of `block` x `block` pixels, filling the frames row by row.
4. decrypting reads the squares back, detects the block size from the header, repairs errors, and decrypts. a wrong password or unrepairable damage is reported, never silently ignored.

the video plays like any other video, but it looks like black and white noise, so it hides the contents, not the fact that something is hidden.

bigger blocks hold less data per frame but survive lossy compression better. videos made before the current format version can't be read.
