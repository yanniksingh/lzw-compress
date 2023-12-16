# LZW Implementation and Performance Tuning

### Introduction

LZW is a variation of the LZ77 and LZ78 compressors, developed by Welch in 1984 [1]. Like LZ77 and LZ78, it is a lossless compressor, and Welch notes in the original paper that it is a comparatively simple algorithm, making efficient implementations possible [1]. This project implements LZW two different ways: one implementation is an educational one written in Python optimized for clarity, while the other is written in C and optimized for performance. The project benchmarks both implementations against a Python implementation of the original LZ77 algorithm as well as a standard highly-optimized LZW implementation [2]. We find that while LZW generally does not compress data as well as LZ77, its simplicity makes simple yet high-throughput implementations possible.

### Existing Work on LZW

Welch [1] proposes and describes the algorithm, in addition to discussing results (compression ratio) on some sample data, and describing some implementation concerns. Like LZ77, LZW has a notion of matchfinding, where after a substring of the input is parsed the first time, future occurrences can be encoded cheaply as a reference to the original occurrence. Unlike LZ77, LZW uses extremely simple data structures and algorithms for matchfinding: the state maintained by both encoder and decoder is a prefix tree that associates substrings seen so far in the input with integer codes.

To give a brief example, say we have an LZW compressor over the alphabet `{A, B}`, and we want to encode the string `ABAB`. The encoder initializes its prefix tree to contain all possible length-1 strings over the alphabet, i.e. all possible symbols, mapped to integer codes beginning at 1. We can visualize the prefix tree as a table matching strings and codes:

| String | Code |
|--------|------|
| `A`    | 1    |
| `B`    | 2    |

At each step, the encoder essentially finds the longest prefix of the remaining input that exists in the prefix tree, outputs the corresponding code, and adds the prefix plus the next character of input to the prefix tree (mapped to the next available integer code). Thus, in the first step, we see that the longest prefix of `ABAB` that exists in the tree is `A`, so the encoder outputs code 1 and adds `AB` (the prefix, plus the next letter `B`) to the prefix tree:

| String | Code |
|--------|------|
| `A`    | 1    |
| `B`    | 2    |
| `AB`   | 3    |

With remaining input `BAB`, the encoder now parses longest prefix `B`, outputs code 2, and adds `BA` to the prefix tree:

| String | Code |
|--------|------|
| `A`    | 1    |
| `B`    | 2    |
| `AB`   | 3    |
| `BA`   | 4    |

The remaining input is now `AB`, which we see exists in the string table, so the encoder outputs code 3. Since there is no more input, no string is added to the prefix tree at this step.

Amazingly, given just the sequence 1, 2, 3 and knowledge of the alphabet `{A, B}`, the decoder can reconstruct the input with no additional information. Like the encoder, the decoder begins with all possible symbols mapped to integer codes beginning at 1. There is one key difference in how the encoder and decoder access the prefix tree: while the encoder queries for a code given a string, the decoder does the reverse, i.e. given a code it needs to know the corresponding string. For this reason, the order of table columns is reversed below.

| Code | String |
|------|--------|
| 1    | `A`    |
| 2    | `B`    |

At each step, the decoder essentially parses the next code (one code at a time) and "walks backward" through the prefix tree to determine the corresponding symbols. For instance, at first, it parses the first code (1) and outputs the symbol `A`. After this, it parses the next code (2) and outputs the symbol `B`. One key observation is that going through this process is also enough for the decoder to reconstruct the prefix tree built up by the encoder: for instance, having parsed codes 1 and 2 to `AB`, the decoder now knows that the encoder must have parsed `A`, output code 1, added `AB` to the prefix tree, then parsed `B` and output code 2. Thus, at this point, the decoder can map the next available code (3) to `AB`:

| Code | String |
|------|--------|
| 1    | `A`    |
| 2    | `B`    |
| 3    | `AB`   |

In general, after parsing every code after the first, the decoder can map the string corresponding to the *previously* output code (in this case `A`), plus the first character of the string corresponding to the *current* code (in this case `B`), to the next available code. Thus, the decoder next parses code 3, outputs `AB`, and adds `BA` to the prefix tree:

| Code | String |
|------|--------|
| 1    | `A`    |
| 2    | `B`    |
| 3    | `AB`   |
| 4    | `BA`   |

At this point, there are no codes left to parse, and the decoder has reconstructed the original input `ABAB`. As Welch mentions, this decoding algorithm is a slight oversimplification; there are special cases where the decoder might see a code for which it doesn't yet have a string in the prefix tree. For the full details of how the decoder works, the interested reader should refer to [1] or to the Python implementation [here](https://github.com/yanniksingh/stanford_compression_library/blob/main/scl/compressors/lzw.py). Using an implementation of this algorithm, Welch generally finds compression ratios between 1.5 and 2.5 for a variety of data.

Since Welch's orginal paper, many more implementations of the algorithm have been developed. For example, the BSD `compress` utility uses LZW [2], and so does GIF [3]. Of particular interest is `ncompress` [2], which implements an improved version of BSD `compress` that to an extent influenced the C implementation in this project. `ncompress` makes several notable design decisions to improve throughput and compression ratio:
* One of the key design decisions in implementing the LZW encoder is how to represent the prefix tree. A good implementation should minimize lookup time and memory usage. `ncompress` represents the prefix tree as a hash table using open addressing (no chaining), mapping code/symbol pairs to codes. This takes advantage of the prefix property of the tree to avoid having to store strings in duplicate: for example, in the example above the entry `{AB: 3}` can be represented as a mapping from the key `(1, B)` to the value `3`, since `AB` extends the existing string `A` (code 1) with the character `B`. This way, each prefix tree entry takes O(1) space. 
* Another key design decision is how to manage growth of the prefix tree, which could in principle become arbitrarily large as more input is read. `ncompress` clears the prefix tree on the encoder side when the compression ratio starts to drop (i.e. statistics of the input change), outputting a special code that causes the decoder to reset at the same point. This allows adapting to changing statistics as well as limiting memory usage.
* Somewhat orthogonal to the algorithm itself is the issue of frequent disk access (and the associated latency) when reading from/writing to a file in a loop. `ncompress` buffers input and output in user memory, reading/writing to disk only occasionally and in large chuncks.

### Methods

#### Python Implementation

This project's Python implementation of LZW just implements the algorithm as simply and understandably as possible; it is for the most part a straightforward translation of the pseudocode described by Welch in [1]. On the encoder side, the prefix tree is represented as a hash table as described above for `ncompress`, but using the default Python dictionary implementation. On the decoder side, the prefix tree is represented as a list, where the item at index `i` represents the entry for code `i` and contains the parent code and symbol (thus allowing the decoder to reconstruct a string in the tree by iteratively walking through the list until the root is reached). No attempt is made to limit memory usage, and the prefix tree could in principle become arbitrarily large.

One design decision I faced was how to store the output codes in compressed files. This may seem trivial for the example above where the output is 1,2,3, but in practice some codes could be much more common than others, so further entropy coding (e.g. Huffman coding) could allow further reductions in compressed file size. I tried this but found Huffman coding to be significantly slower (by over an order of magnitude) than simply storing the codes as their binary representations, with bit width equal to the number of bits needed to reperesent the largest code in the list.

The Python implementation can be found [here](https://github.com/yanniksingh/stanford_compression_library/blob/main/scl/compressors/lzw.py).

#### C Implementation

Unlike the Python implementation, the C implementation was optimized for maximum encoder and decoder throughput, even at the expense of compression ratio. I broadly considered the same design decisions mentioned in the previous section for `ncompress`.

On the decision of how to represent the prefix tree, on the encoder side the implementation uses a somewhat similar (but simpler) approach to `ncompress`. Like `ncompress`, the tree is represented as a map from code/symbol pairs to codes. Unlike `ncompress`, instead of using hashing, I simply allocate a large array with enough space for each code/symbol pair key to deterministically map to a dedicated slot that has its corresponding value. I also tested the C++ STL `unordered_map` container, but found that it performed worse on throughput. On the decoder side, like the Python implementation, I use an array with the same structure, but instead of growing it dynamically as codes are added, I simply allocate enough space upfront for all codes, to reduce the number of memory allocations. The sizes of both encoder and decoder arrays can be known in advance because the implementation declares a maximum allowed code value.

On the decision of how to manage growth of the prefix tree, like `ncompress`, this implementation periodically clears the prefix tree and uses a special code to signal this to the decoder. Unlike `ncompress`, the C implementation simply clears whenever the maximum allowed code value has been reached.

Like `ncompress`, this implementation also reads and writes to disk in chunks to reduce the number of accesses to disk.

#### Benchmarking

The project's two implementations were benchmarked against `ncompress` and a [Python implementation of LZ77](https://github.com/kedartatwawadi/stanford_compression_library/blob/main/scl/compressors/lz77.py). Data was collected on compressed file size, encoder throughput, and decoder throughput. All compressors were built and run with default parameters/options.

Throughput was measured based on the uncompressed file size and time taken by the encoder and decoder, as measured by the real time returned by the Unix `time` utility. The median of five runs was used, running on a machine with an Intel i5 processor.

### Results

Compressed file size (bytes):

| File                         | raw      |LZ77Encoder|LZWEncoder|ncompress|compress|
|------------------------------|----------|-----------|----------|---------|--------|
| bootstrap-3.3.6.min.css      |121260    |20126      |41567     |37687    |44292   |
| eff.html                     |41684     |10220      |19462     |17620    |22218   |
| zlib.wasm                    |86408     |41156      |58770     |54913    |62666   |
| jquery-2.1.4.min.js          |84345     |31322      |44366     |40509    |47302   |
| kennedy.xls                  |1029744   |210182     |327222    |310451   |332882  |
| alice29.txt                  |152089    |54106      |67096     |62247    |70148   |
| vmlinux-4.2.6-300.fc23.x86_64|19667216  |5969015    |7714999   |8315571  |8461662 |

Encoder throughput (KB/s):

| File                         |LZ77Encoder|LZWEncoder|ncompress|compress|
|------------------------------|-----------|----------|---------|--------|
| bootstrap-3.3.6.min.css      |101        |302       |20000    |4300    |
| eff.html                     |91         |138       |7000     |1500    |
| zlib.wasm                    |125        |214       |10000    |3100    |
| jquery-2.1.4.min.js          |111        |219       |10000    |2900    |
| kennedy.xls                  |37         |624       |49000    |23000   |
| alice29.txt                  |109        |328       |15000    |4900    |
| vmlinux-4.2.6-300.fc23.x86_64|64         |676       |55700    |34600   |

Decoder throughput (KB/s):

| File                         |LZ77Decoder|LZWDecoder|ncompress|compress|
|------------------------------|-----------|----------|---------|--------|
| bootstrap-3.3.6.min.css      |234        |335       |20000    |20000   |
| eff.html                     |114        |146       |8000     |7000    |
| zlib.wasm                    |60         |242       |10000    |20000   |
| jquery-2.1.4.min.js          |100        |247       |10000    |10000   |
| kennedy.xls                  |301        |807       |64000    |69000   |
| alice29.txt                  |141        |375       |20000    |30000   |
| vmlinux-4.2.6-300.fc23.x86_64|129        |968       |68300    |89000   |

### Discussion

TODO

### Conclusion

TODO

### References

[1] https://ieeexplore.ieee.org/document/1659158

[2] https://github.com/vapier/ncompress

[3] https://www.w3.org/Graphics/GIF/spec-gif89a.txt
