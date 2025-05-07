# ftv

a tool that hides files inside video files. it encrypts the given data and embeds it into video frames, making it look like a  video file.

## prerequisites

- cmake 3.27+
- c++ compiler with c++23 support
- opencv 4.10+
- openssl 3.4.0+

## build

```bash
git clone git@github.com:ulastanersoyak/ftv.git
cd ftv
mkdir build
cd build
cmake ..
make
```

## usage

hide a file in a video:
```bash
ftv encrypt file.txt -o video.avi -k mypassword -w 640 -h 480 -f 30
```

get your file back:
```bash
ftv decrypt video.avi -k mypassword
```

## options

- `-o, --output <file>`   - where to save the video (needed for encrypt)
- `-k, --key <key>`       -  password (max 32 chars)
- `-w, --width <pixels>`  - video width (100-4096, default: 300)
- `-h, --height <pixels>` - video height (100-4096, default: 300)
- `-f, --fps <number>`    - frames per second (1-60, default: 30)

## how it works

ftv uses aes-256-gcm encryption to secure your data before hiding it in the video. the video looks normal and can be played like any other video file, but hidden data is stored within it.