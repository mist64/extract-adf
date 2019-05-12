# extract-adf

*extract-adf* is a tool that extracts files from (broken) Amiga _OFS_ ADF/ADZ/DMS disk images.

It will create all files with the correct directory hierarchy and the correct timestamps on the local filesystem.

It is specially optimized for broken filesystems, and can recover as much as possible even on those. One use case is to extract the [leftover files on the Kickstart 1.0 disk](http://www.pagetable.com/?p=34).

## Usage

	Extract-ADF 4.0 Originally (C)2008 Michael Steil with many further additions by Sigurbjorn B. Larusson
	DMS extraction code (C) 1998 David Tritscher
	
	Usage: ./extract-adf [-D] [-a] [-z] [-d] [-s <startsector>] [-e <endsector>] [-o <outputfilename>] <adf/adz/dmsfilename>
	
		-a will force ADF extraction (if the filename ends in adf ADF will be assumed
		-z will force ADZ extraction (if the filename ends in adz or adf.gz ADZ will be assumed
		-d will force DMS extraction (if the filename ends in dms DMS format will be assumed
		-D will activate debugging output which will print very detailed information about everything that is going on
		-s along with an integer argument from 0 to 1760 (DD) or 3520 (HD), will set the starting sector of the extraction process
		-e along with an integer argument from 0 to 1760 (DD) or 3520 (HD), will set the end sector of the extraction process
		-o along with an outputfilename will redirect output (including debugging output) to a file instead of to the screen
		Finally the last argument is the ADF/ADZ or DMS filename to process
	
	The defaults for start and end sector are 0 and 1760 respectively, this tool was originally
	created to salvage lost data from kickstart disks (which contain the kickstart on sectors 0..512)
	in order to skip the sectors on kickstart disks which might contain non OFS data, set the start sector to 513
	
	To use this tool on a HD floppy, the end sector needs to be 3520
	
	If you get a Bus error it means that you specificed a non-existing end sector
	
	This program does not support FFS floppies(!), it only supports OFS style Amiga Floppies
	
	Happy hunting!


## Authors

* Michael Steil
* Sigurbjorn B. Larusson

## History

### Version 1.0, 2008, Michael Steil

Original version for extracting the leftover files on the Kickstart 1.0 disk.

### Version 2.0, 2011, Sigurbjorn B. Larusson

* Hack to restore the file path and to pass the adf file and start sector/end sector used as an argument. This makes it possible to use on any OFS adf file and on HD OFS floppies and to tune where to start and end the process for any other purpose

* Orphaned files still end up where ever you launched the binary, you'll have to manually move them into the structure if you know where the files should be located
 
* Killed all output unless DEBUG is defined as 1 or higher, you can easily enable it (along with even more debugging output from all the stuff I added) by defining DEBUG as 1...

### Version 3.0, 2017, Sigurbjorn B. Larusson

* Changed the default start sector to 0
* Now uses getopt to retrieve command line options
	* Added a commandline option flag (-s) to specifiy a start sector, you can now place this in any order on the command line
	* Added a commandline option flag (-e) to specify an end sector, you can now place this in any order on the commandline
	* Added a commandline option flag (-d) to turn on debugging from the command line instead of having to recompile with the debug flag on
	* Added a commandline option flag (-o) to specify writing the output to a file instead of to stdout, this is usefull due to the amount of debugging that has been built into the program
* Added quite a bit of additional debugging output, it now goes through significant detail for every sector it works on, this can help puzzle together the filesystem, I highly recommend writing to a file!
* Significantly altered the progress of creating directories, it now creates orphaned directories at the CWD so that the directory tree present on disk is always restored as close to the original as possible
* Significantly altered the progress of writing orphaned files
	* It now tries to extract the original filename as well as the parent filename before making up a filename
	* It also tries to place them in their respective directories
	* It now creates files when parsing the headers, so even files that have no recoverable data get created and you know where they would have been placed in the hierarchy
* Added a much better ascii/hexdumper function which now dumps both ascii and hex, 20 characters per line (20 ascii characters + FF format for the hex codes)
* Added many additional segmentation checks and cleaned up the memory allocation, it should be fairly rare for this utility to crash now, even with badly mangled filesystems

### Version 4.0, 2019 Sigurbjorn B. Larusson

* Added support for compressed ADF files (gzip compressed)
* Added support for DMS files
	*    This uses a lot of code borrowed from undmz.c, (C) David Tritscher and available on Aminet http://aminet.net/package/misc/unix/undms-1.3.c
	*    It also uses header information for DMS files from http://lclevy.free.fr/amiga/DMS.txt
* Added support for ZIP files containing an ADF
* Added commandline option to force treating file as a ADF file
* Added commandline option to force treating file as a ADZ file (gzip compressed ADF)
* Added commandline option to force treating file as a DMS file
* Added function to automagically determine format from file extension
* Added support for re-creating original file timestamps
* Added support to detect endianness and checks to not convert msb to lsb if we're running on a big endian system, it should therefore now be possible to compile and use this on a big endian machine, if you try it, tell me about it
* Fixed some minor bugs

## TODO
* The source code could do with a cleanup and even a rewrite, I'll leave that for the next time I have time to work on it
