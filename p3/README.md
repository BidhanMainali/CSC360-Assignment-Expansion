# CSC 360 P3 - Simple File System

Bidhan Mainali
V01060563

## How to build

Run `make` in this folder. It builds all four programs (`diskinfo`, `disklist`,
`diskget`, `diskput`) in the same directory. Run `make clean` to remove them.

I tested everything on linux.csc.uvic.ca.

## Files

- `sfs_common.h` / `sfs_common.c` - shared code used by all four programs
  (reading the superblock, reading the FAT, following the block chain, reading
  and writing directory entries, path handling). I put it here so I didn't have
  to copy the same code into every program.
- `diskinfo.c`, `disklist.c`, `diskget.c`, `diskput.c` - one file per part.

## What I implemented and whether it works

### Part I - diskinfo (works)
Prints the super block info and the FAT free/reserved/allocated counts. I checked
it against test.img and the numbers match the expected output in the spec
(Free 6341, Reserved 49, Allocated 10).

### Part II - disklist (works)
Lists the contents of a directory in the required format (F/D, size, name, date).
Works for the root directory and for sub-directories, including nested paths like
`/dir1/dir2/dir3/dir4/dir5` on large.img. I tested it on test.img, non-empty.img
and large.img and the output matches.

### Part III - diskget (works)
Copies a file out of the image to the current directory. I verified the copied
files are identical to the originals using sha1sum, and I tested with binary
files (cat.jpg, 201.jpg, video.mp4) as well as text files. If the file or the
directory isn't found it prints `Requested file <filename> not found in <path>.`
as required.

### Part IV - diskput (works)
Copies a file from the host into the image. Works for the root directory and for
sub-directories. If the target sub-directory doesn't exist yet it creates it and
then copies the file in. If the source file doesn't exist on the host it prints
`Source file <filename> not found.` I tested it by putting files in (text and
binary), listing them with disklist, and copying them back out with diskget and
checking the sha1sum matches the original.

## Notes

- The file system is big endian on disk, so I use ntohl/ntohs when reading and
  htonl/htons when writing.
- All programs return 0 on success and non-zero on failure.
- diskput changes the image, so when I tested it I always worked on a copy of
  test.img.
