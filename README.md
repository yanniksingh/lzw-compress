# lzw-compress

`lzw-compress` is a fast, simple file compressor that uses the LZW algorithm.

More details, including a description of the algorithm, implementation details,
and performance data can be found
[here](https://github.com/yanniksingh/lzw-compress/blob/report/REPORT.md).

### Requirements

C compiler supporting C99

### Installation

```bash
git clone https://github.com/yanniksingh/lzw-compress
cd lzw-compress
make
```

### Usage

```bash
./compress [input-file-path] [compressed-file-path]
./uncompress [compressed-file-path] [output-file-path]
```
