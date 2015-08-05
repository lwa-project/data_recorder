/*
  MCS-DROS: Monitor and Control System - Data Recorder Operating Software
  Copyright (C) 2009-2010  Virginia Tech, Christopher Wolfe <chwolfe2@vt.edu>

  This file is part of MCS-DROS.
 
  MCS-DROS is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  MCS-DROS is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with MCS-DROS.  If not, see <http://www.gnu.org/licenses/>.
*/
/*
 * fuse-lwafs.c
 *
 *  Created on: Dec 11,  2009
 *      Author: chwolfe2
 */

#include <stdlib.h>
#include <stddef.h>
#define FUSE_USE_VERSION 27
#include <fuse.h>
#include <fuse_opt.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <assert.h>

#include "Disk.h"
#include "FileSystem.h"
#include "HostInterface.h"


// code legibility macros
#define _CHUNKSIZE_ fs->fileSystemHeaderData->filesystemInfo.chunkSize

int lwafs_getattr(const char *path,  struct stat *stbuf);
int lwafs_readlink(const char *path,  char *buffer,  size_t buffer_length);
int lwafs_mknod(const char * path,  mode_t mode,  dev_t device);
int lwafs_mkdir(const char * path,  mode_t mode);
int lwafs_unlink(const char *path);
int lwafs_rmdir(const char * path);
int lwafs_symlink(const char * path,  const char * target);
int lwafs_rename(const char * path,  const char * newpath);
int lwafs_link(const char * path,  const char * target);
int lwafs_chmod(const char * path,  mode_t mode);
int lwafs_chown(const char * path,  uid_t UserId,  gid_t GroupId);
int lwafs_truncate(const char * path,  off_t length);
int lwafs_open(const char *path,  struct fuse_file_info *fi);
int lwafs_read(const char * path,  char *buf,  size_t size,  off_t offset,  struct fuse_file_info *fi);
int lwafs_write(const char * path,  const char * buf,  size_t size,  off_t offset,  struct fuse_file_info * fi);
int lwafs_statfs(const char * path,  struct statvfs * stats);
int lwafs_flush(const char * path,  struct fuse_file_info * fi);
int lwafs_release(const char * path,  struct fuse_file_info * fi);
int lwafs_fsync(const char * path,  int datasync,  struct fuse_file_info * fi);
int lwafs_setxattr (const char * path,  const char * attributeName,  const char * attributeValue,  size_t attributeSize,  int flags);
int lwafs_getxattr(const char * path,  const char * attributeName,  char * attributeValue,  size_t attributeSizeMax);
int lwafs_listxattr(const char * path,  char * attributeList,  size_t attributeListSizeMax);
int lwafs_removexattr(const char * path,  const char * attributeName);
int lwafs_opendir(const char * path,  struct fuse_file_info * fi);
int lwafs_readdir(const char * path,  void * buf,  fuse_fill_dir_t filler,  off_t offset,  struct fuse_file_info *fi);
int lwafs_releasedir(const char * path,  struct fuse_file_info * fi);
int lwafs_fsyncdir(const char * path,  int datasync,  struct fuse_file_info * fi);
void * lwafs_init(struct fuse_conn_info * conn);
void lwafs_destroy(void * data);
int lwafs_access (const char * path,  int mode);
int lwafs_create(const char * path,  mode_t mode,  struct fuse_file_info * fi);
int lwafs_ftruncate(const char * path,  off_t offset,  struct fuse_file_info * fi);
int lwafs_fgetattr(const char * path,  struct stat * stats,  struct fuse_file_info * fi);
int lwafs_lock(const char * path,  struct fuse_file_info * fi,  int cmd,  struct flock * lock);
int lwafs_utimens(const char * path,  const struct timespec tv[2]);
int lwafs_utime(const char *, struct utimbuf *);
int lwafs_bmap(const char * path,  size_t blocksize,  uint64_t *idx);
#define lwafs_flag_nullpath_ok 0
//int lwafs_ioctl(const char * path,  int cmd,  void *arg,  struct fuse_file_info * fi,  unsigned int flags,  void *data);
//int lwafs_poll(const char * path,  struct fuse_file_info * fi,  struct fuse_pollhandle *ph,  unsigned *reventsp);


void flushWriteBuffer(FileSystem* fs,  MyFile* myfile);
int resize(FileSystem* fs,  MyFile* myfile,  size_t newSize);

static struct fuse_operations lwafs_operations = {
		.access			= lwafs_access,
		.bmap			= lwafs_bmap,
		.chmod			= lwafs_chmod,
		.chown			= lwafs_chown,
		.create			= lwafs_create,
		.destroy		= lwafs_destroy,
		.fgetattr		= lwafs_fgetattr,
		.flush			= lwafs_flush,
		.fsync			= lwafs_fsync,
		.fsyncdir		= lwafs_fsyncdir,
		.ftruncate		= lwafs_ftruncate,
		.getattr		= lwafs_getattr,
		.getxattr		= lwafs_getxattr,
		.init			= lwafs_init,
		.link			= lwafs_link,
		.listxattr		= lwafs_listxattr,
		.lock			= lwafs_lock,
		.mkdir			= lwafs_mkdir,
		.mknod			= lwafs_mknod,
		.open			= lwafs_open,
		.opendir		= lwafs_opendir,
		.read			= lwafs_read,
		.readdir		= lwafs_readdir,
		.readlink		= lwafs_readlink,
		.release		= lwafs_release,
		.releasedir		= lwafs_releasedir,
		.removexattr	= lwafs_removexattr,
		.rename			= lwafs_rename,
		.rmdir			= lwafs_rmdir,
		.setxattr		= lwafs_setxattr,
		.statfs			= lwafs_statfs,
		.symlink		= lwafs_symlink,
		.truncate		= lwafs_truncate,
		.unlink			= lwafs_unlink,
		.utimens		= lwafs_utimens,
		.utime		    = lwafs_utime,
		.write			= lwafs_write,
};

/**
 * Define option parsing.
 */

typedef struct __LWAFS_CONFIG {
	 char *     testClone;
     char *		device;
     int 		create;
     int        trace;
}LWAFS_CONFIG;



enum {
     KEY_HELP,
     KEY_VERSION,
};

#define MYFS_OPT(t,  p,  v) { t,  (offsetof(struct __LWAFS_CONFIG ,  p)) ,  v }

static struct fuse_opt lwafs_opts[] = {
	 MYFS_OPT("TestByClone=%s",      testClone,  0),
     MYFS_OPT("device=%s",           device,  0),
     MYFS_OPT("create",              create,  1),
     MYFS_OPT("trace",               trace,  1),
     FUSE_OPT_KEY("-V",              KEY_VERSION),
     FUSE_OPT_KEY("--version",       KEY_VERSION),
     FUSE_OPT_KEY("-h",              KEY_HELP),
     FUSE_OPT_KEY("--help",          KEY_HELP),
     FUSE_OPT_END
};

static int lwafs_option_parser(void *data,  const char *arg,  int key,  struct fuse_args *outargs){
     switch (key) {
     case KEY_HELP:
             fprintf(stderr,
                     "usage: %s mountpoint [options]\n"
                     "\n"
                     "general options:\n"
                     "    -o opt, [opt...]  mount options\n"
                     "    -h   --help      print help\n"
                     "    -V   --version   print version\n"
                     "\n"
                     "lwafs options:\n"
                     "    --device=/dev/xxx specify the block device to use.\n"
                     "    --create          create filesystem if one does not exist\n"
                     ,  outargs->argv[0]);
             fuse_opt_add_arg(outargs,  "-ho");
             fuse_main(outargs->argc,  outargs->argv,  &lwafs_operations,  NULL);
             exit(1);
     case KEY_VERSION:
             fprintf(stderr,  "Myfs version %s\n",  "2.8.1");
             fuse_opt_add_arg(outargs,  "--version");
             fuse_main(outargs->argc,  outargs->argv,  &lwafs_operations,  NULL);
             exit(0);
     }
     return 1;
}

LWAFS_CONFIG conf;
#define TRACE (conf.trace!=0)

int main(int argc,  char *argv[]){
	int rv;

	struct fuse_args args = FUSE_ARGS_INIT(argc,  argv);
	memset(&conf,  0,  sizeof(conf));
	fuse_opt_parse(&args,  &conf,  lwafs_opts,  lwafs_option_parser);
	rv = fuse_main(args.argc,  args.argv,  &lwafs_operations,  NULL);
	return rv;

}



















void testCloneFiles(FileSystem * fs,  const char* dir){
	DIR *dp;
    struct dirent *ep;
	struct stat fileStats;
	char fullpath[4096];
	int findex=-1;

    dp = opendir (dir);
	if (dp == NULL){
    	perror ("Couldn't open the supplied clone directory");
    	FileSystem_Close(fs);
    	return;
    }
	void * buffer = mmap(NULL, _CHUNKSIZE_,  PROT_READ | PROT_WRITE,  MAP_PRIVATE | MAP_ANONYMOUS | MAP_LOCKED,  -1,  0);
	if ((!buffer)||(buffer==MAP_FAILED)){
		FileSystem_Close(fs);
		printf("COULD NOT ALLOCATE CLONE MEM!!!\n");
		return;
	}

	while ((ep = readdir (dp))){
		if (ep->d_type==DT_REG){
			bzero(fullpath, 4096);
			sprintf(fullpath, "%s/%s", dir, ep->d_name);
			if (stat(fullpath, &fileStats)>=0){
				printf("CLONING FILE: Size = %16ld,  Name = '%s'\n", fileStats.st_size, fullpath);
				switch(FileSystem_CreateFile(fs, ep->d_name, (size_t)fileStats.st_size, 0, &findex)){
					case SUCCESS:{
						FILE* file=fopen(fullpath, "rb");
						if (!file){
							//printf("COULD NOT CREATE CLONE FILE (OPEN PHASE)!!!\n");
							FileSystem_DeleteFile(fs, findex);
						} else {
							MyFile myfile;
							bzero(&myfile, sizeof(MyFile));
							size_t base=
									fs->fileSystemHeaderData->fileInfo[findex].startPosition +
									_CHUNKSIZE_;
							myfile.index=-1;
							size_t numWholeChunks = ((size_t)fileStats.st_size)/_CHUNKSIZE_;
							size_t remainingBytes = ((size_t)fileStats.st_size) & (_CHUNKSIZE_-1);
							size_t offset=0;
							ssize_t bytesWritten;
							int error=0;
							while (numWholeChunks && !error){
								if (fseek(file, offset, SEEK_SET)){
									perror("COULD NOT CREATE CLONE FILE (COPY PHASE)!!!\nfseek");
									error=1;
								} else {
									ssize_t bytesRead=fread(buffer, 1, _CHUNKSIZE_, file);
									if (bytesRead!=_CHUNKSIZE_){
										error=1;
									} else {
										if (_FileSystem_SynchronousWrite(fs, buffer, base+offset, _CHUNKSIZE_, &bytesWritten)!=SUCCESS){
											printf("COULD NOT CREATE CLONE FILE (COPY PHASE)!!!\n");
											error=1;
										}
									}
								}
								offset+=_CHUNKSIZE_;
								numWholeChunks--;
							}
							while (remainingBytes && !error){
								if (fseek(file, offset, SEEK_SET)){
									perror("COULD NOT CREATE CLONE FILE (COPY PHASE)!!!\nfseek");
									error=1;
								} else {
									bzero(buffer, _CHUNKSIZE_);
									ssize_t bytesRead=fread(buffer, 1, remainingBytes, file);
									if (bytesRead!=remainingBytes){
										error=1;
									} else {
										if (_FileSystem_SynchronousWrite(fs, buffer, base+offset, _CHUNKSIZE_, &bytesWritten)!=SUCCESS){
											printf("COULD NOT CREATE CLONE FILE (COPY PHASE)!!!\n");
											error=1;
										}
									}
								}
								offset+=remainingBytes;
								remainingBytes-=remainingBytes;
							}
							if (offset!=fileStats.st_size){
								printf("clone bytes not equal\n");
							}
							fclose(file);

						}
					}break;
					default:{
						printf("cloning error\n");

					}break;
				}

			}
		}
	}
	closedir (dp);

}




//------------------------------------------------------------------
//--	Filesystem initialization
//------------------------------------------------------------------

/**
 * Initialize filesystem
 * The return value will passed in the private_data field of
 * fuse_context to all file operations and as a parameter to the
 * destroy() method.
 */
Disk * disk = NULL;
void * lwafs_init(struct fuse_conn_info * conn){
	if(TRACE)fprintf(stderr, HLG("LWAFS: TRACE: lwafs_init(struct fuse_conn_info * conn)\n"));fflush(stderr);

	StatusCode sc;
	FileSystem* fs = NULL;

	if (!conf.device){
			printf("LWAFS: Fatal Error: block device not specified.\nExiting...\n");
			exit(FAILURE); // is this the right thing to do?
		} else {
			fprintf(stderr,  "LWAFS: Allocate the device info memory.\n");
			disk=(Disk*) malloc(sizeof(Disk));
			if (disk==NULL){
				fprintf(stderr,  "LWAFS: Fatal Error: Cannot allocate space for disk info.\nExiting...\n");
				exit(FAILURE); // is this the right thing to do?
			}
			memset((void*)disk,  0,  sizeof(Disk));


			//fprintf(stderr,  "LWAFS: Allocate the filesystem info memory.\n");
			fs=(FileSystem*) malloc(sizeof(FileSystem));
			if (fs==NULL){
				fprintf(stderr,  "LWAFS: Fatal Error: Cannot allocate space for filesystem info.\nExiting...\n");
				exit(FAILURE); // is this the right thing to do?
			}
			memset((void*)fs,  0,  sizeof(FileSystem));

			// enumerate drives
			//fprintf(stderr,  "LWAFS: Enumerate the system's storage devices.\n");fflush(stderr);
			sc = Disk_IdentifyAll();
			if (sc!=SUCCESS){
				fprintf(stderr,  "LWAFS: Fatal Error: Could not enumerate system storage devices.\nExiting...\n");
				exit(FAILURE); // is this the right thing to do?

			}
			//fprintf(stderr,  "LWAFS: Get disk info for device '%s'\n", conf.device);fflush(stderr);
			// get info of command-line specified drive
			sc=Disk_GetDiskInfo(conf.device, disk);
			if (sc != SUCCESS  || disk==NULL){
				fprintf(stderr,  "LWAFS: Fatal Error: Could not get information for device '%s'.\nExiting...\n", conf.device);fflush(stderr);
				exit(FAILURE); // is this the right thing to do?
			}

			// if filesystem creation was specified,  do it...
			if (conf.create){
				fprintf(stderr,  "LWAFS: Creating filesystem on device '%s'\n", conf.device);fflush(stderr);
				sc=FileSystem_Create(disk);
				if (sc != SUCCESS){
					fprintf(stderr,  "LWAFS: Fatal Error: Could not get create filesystem on device '%s'.\nExiting...\n", conf.device);fflush(stderr);
					exit(FAILURE); // is this the right thing to do?
				}
			} else {
				//fprintf(stderr,  "LWAFS: Will NOT create filesystem on device '%s'\n", conf.device);fflush(stderr);
			}

			// try to open the filesystem
			fprintf(stderr,  "LWAFS: Opening filesystem on device '%s'\n", conf.device);fflush(stderr);
			sc=FileSystem_Open(fs, disk);
			if (sc!=SUCCESS){
				fprintf(stderr,  "LWAFS: Fatal Error: Could not get open filesystem on device '%s'.\nExiting...\n", conf.device);fflush(stderr);
				exit(FAILURE); // is this the right thing to do?
			}
			/*
			if (conf.create){
				int findex;
				sc = FileSystem_CreateFile(fs,  "Apple",  10485760,  0,  &findex);
				if (sc!=SUCCESS){
					fprintf(stderr,  "LWAFS: Fatal Error: Could not get create test file on device '%s'.\nExiting...\n", conf.device);fflush(stderr);
					exit(FAILURE); // is this the right thing to do?
				}
				sc = FileSystem_CreateFile(fs,  "Dapple",  10485760,  0,  &findex);
				if (sc!=SUCCESS){
					fprintf(stderr,  "LWAFS: Fatal Error: Could not get create test file on device '%s'.\nExiting...\n", conf.device);fflush(stderr);
					exit(FAILURE); // is this the right thing to do?
				}
				sc = FileSystem_CreateFile(fs,  "Do",  10485760,  0,  &findex);
				if (sc!=SUCCESS){
					fprintf(stderr,  "LWAFS: Fatal Error: Could not get create test file on device '%s'.\nExiting...\n", conf.device);fflush(stderr);
					exit(FAILURE); // is this the right thing to do?
				}

			}
			*/

			// return fs to be used in future calls 'fuse_get_context()->private_data' returns fs
			if (conf.testClone){
				testCloneFiles(fs, conf.testClone);
			}

			return (void*) fs;
		}
	return NULL;
}

/**
 * Clean up filesystem
 * Called on filesystem exit.
 */
void lwafs_destroy(void * data){
	StatusCode sc=FAILURE;
	FileSystem* fs=NULL;
	if(TRACE)fprintf(stderr, HLG("LWAFS: TRACE: lwafs_destroy(void * data)\n"));fflush(stderr);
	if (data!=NULL){
		fs = (FileSystem*) data;
		fprintf(stderr,  "LWAFS: Closing filesystem on device '%s'.\n", fs->diskInfo->deviceName);fflush(stderr);
		sc = FileSystem_Close(fs);

		if (sc!=SUCCESS){
			fprintf(stderr,  "LWAFS: Fatal Error: Could not get close filesystem on device '%s'.\n", fs->diskInfo->deviceName);fflush(stderr);
		}
		if (disk) free(disk);
		if (fs) free(fs);
		disk=NULL;


	}
}


/** Get file attributes.
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.	 The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
int lwafs_getattr(const char *path,  struct stat *stbuf){
	if(TRACE)fprintf(stderr, HLG("LWAFS: TRACE: lwafs_getattr(\"%s\")\n"), path);fflush(stderr);
	struct fuse_context* fc;
	FileSystem* fs;
	fc=fuse_get_context();
	fs = (FileSystem*) fc->private_data;
	if (fs==NULL){
		fprintf(stderr, "LWAFS: Error: filesystem user data missing in fuse context\n");fflush(stderr);
		return -ENODEV;
	}
	if (strcmp(path, "/")==0){
		//fprintf(stderr, HLR("LWAFS: TRACE: strcmp(path, \"%s\")==0\n"), path);fflush(stderr);

		stbuf->st_atim    = fs->fileSystemHeaderData->filesystemInfo.atime;
		stbuf->st_ctim    = fs->fileSystemHeaderData->filesystemInfo.ctime;
		stbuf->st_mtim    = fs->fileSystemHeaderData->filesystemInfo.mtime;
		stbuf->st_size    = fs->fileSystemHeaderData->filesystemInfo.diskSize;
		stbuf->st_blksize = 512l; //igTesnored
		stbuf->st_blocks  = stbuf->st_size / 512l;
		stbuf->st_dev     = fs->diskInfo->devNumber;
		stbuf->st_rdev    = fs->diskInfo->devNumber;
		stbuf->st_mode    = fs->fileSystemHeaderData->filesystemInfo.amode;
		stbuf->st_gid     = fs->fileSystemHeaderData->filesystemInfo.gid;
		stbuf->st_uid     = fs->fileSystemHeaderData->filesystemInfo.uid;
		//stbuf->st_ino = ;
		stbuf->st_nlink   = 2;
	} else {
		if ( (path[0]=='/') && (FileSystem_FileExists(fs, &path[1])) ){
			//fprintf(stderr, HLR("LWAFS: TRACE: (path[0]=='/') && (FileSystem_FileExists(fs, %s))\n"), &path[1]);fflush(stderr);
			int findex = _FileSystem_GetFileIndex(fs, &path[1]);
			FileInfo* fi=&(fs->fileSystemHeaderData->fileInfo[findex]);
			stbuf->st_atim    = fi->atime;
			stbuf->st_ctim    = fi->ctime;
			stbuf->st_mtim    = fi->mtime;
			stbuf->st_size    = fi->size;
			stbuf->st_blksize = 512l; //ignored
			stbuf->st_blocks  = (fi->stopPosition-fi->startPosition) / 512l;
			stbuf->st_dev     = fs->diskInfo->devNumber;
			stbuf->st_rdev    = fs->diskInfo->devNumber;
			stbuf->st_mode    = fi->amode;
			stbuf->st_gid     = fi->gid;
			stbuf->st_uid     = fi->uid;

			//stbuf->st_ino = ;
			stbuf->st_nlink   = 0;
		} else {
			//if (TRACE)fprintf(stderr, HLR("LWAFS: TRACE: !!!((path[0]=='/') && (FileSystem_FileExists(fs, &path[1])))\n"));fflush(stderr);

			return -ENOENT;
		}
	}
	/*
	fprintf(stderr, HLB("LWAFS:  --- getattr('%s') --- \n"), path);fflush(stderr);
	fprintf(stderr, HLB("      st_ctim    is %012lu:%012lu\n"), stbuf->st_ctim.tv_sec, stbuf->st_ctim.tv_nsec);fflush(stderr);
	fprintf(stderr, HLB("      st_mtim    is %012lu:%012lu\n"), stbuf->st_mtim.tv_sec, stbuf->st_mtim.tv_nsec);fflush(stderr);
	fprintf(stderr, HLB("      st_atim    is %012lu:%012lu\n"), stbuf->st_atim.tv_sec, stbuf->st_atim.tv_nsec);fflush(stderr);
	fprintf(stderr, HLB("      st_size    is %18lu\n"), stbuf->st_size);fflush(stderr);
	fprintf(stderr, HLB("      st_blksize is %18lu\n"), stbuf->st_blksize);fflush(stderr);
	fprintf(stderr, HLB("      st_blocks  is %18lu\n"), stbuf->st_blocks);fflush(stderr);
	fprintf(stderr, HLB("      st_dev     is %18lu\n"), stbuf->st_dev);fflush(stderr);
	fprintf(stderr, HLB("      st_rdev    is %18lu\n"), stbuf->st_rdev);fflush(stderr);
	fprintf(stderr, HLB("      st_mode    is %18o\n"), stbuf->st_mode);fflush(stderr);
	fprintf(stderr, HLB("      st_gid     is %18u\n"), stbuf->st_gid);fflush(stderr);
	fprintf(stderr, HLB("      st_uid     is %18u\n"), stbuf->st_uid);fflush(stderr);

	fprintf(stderr, HLB("      \n"));fflush(stderr);

    */


	return 0;
}

/** Read the target of a symbolic link
 * The buffer should be filled with a null terminated string.  The
 * buffer size argument includes the space for the terminating
 * null character.	If the linkname is too long to fit in the
 * buffer,  it should be truncated.	The return value should be 0
 * for success.
 */
int lwafs_readlink(const char *path,  char *buffer,  size_t buffer_length){
	if(TRACE)fprintf(stderr, HLG("LWAFS: TRACE: lwafs_readlink(\"%s\",  ... )\n"), path);fflush(stderr);
	return -ENOENT;
}

/** Remove a file */
int lwafs_unlink(const char *path){
	if(TRACE)fprintf(stderr, HLG("LWAFS: TRACE: lwafs_unlink(\"%s\")\n"), path);fflush(stderr);
	struct fuse_context* fc;
	FileSystem* fs;
	fc=fuse_get_context();
	fs = (FileSystem*) fc->private_data;
	if (fs==NULL){
		fprintf(stderr, "LWAFS: Error: filesystem user data missing in fuse context\n");fflush(stderr);
		return -ENODEV;
	}
	if (path[0]!='/') return -ENOENT;
	int findex=_FileSystem_GetFileIndex(fs, &path[1]);
	if (findex<0) return -ENOENT;
	StatusCode sc = FileSystem_DeleteFile(fs,  findex);
	if (sc!=SUCCESS) return -EINVAL;
	return 0;
}


/** File open operation
	 * # No creation (O_CREAT,  O_EXCL) and by default also no
	 * truncation (O_TRUNC) flags will be passed to open(). If an
	 * application specifies O_TRUNC,  fuse first calls truncate()
	 * and then open(). Only if 'atomic_o_trunc' has been
	 * specified and kernel version is 2.6.24 or later,  O_TRUNC is
	 * passed on to open.
	 * # Unless the 'default_permissions' mount option is given,
	 * open should check if the operation is permitted for the
	 * given flags. Optionally open may also return an arbitrary
	 * filehandle in the fuse_file_info structure,  which will be
	 * passed to all file operations.
	 */
#define printOpt(o) {if ( (TRACE) && ((fi->flags & o) != 0)) fprintf(stderr,  HLR("%s "), #o); }

int lwafs_open(const char *path,  struct fuse_file_info *fi){
	if(TRACE)fprintf(stderr, HLG("LWAFS: TRACE: lwafs_open(\"%s\", "), path);
	printOpt(O_RDONLY);	printOpt(O_WRONLY); printOpt(O_RDWR);
	printOpt(O_NOFOLLOW); printOpt(O_EXCL); printOpt(O_LARGEFILE);
	printOpt(O_DIRECT); printOpt(O_SYNC); printOpt(O_NONBLOCK);
	printOpt(O_APPEND); printOpt(O_CREAT); printOpt(O_DIRECTORY);
	printOpt(O_NOATIME); printOpt(O_ASYNC); printOpt(O_NOCTTY);
	printOpt(O_TRUNC);
	if(TRACE)fprintf(stderr, HLG(" )\n"));fflush(stderr);

	struct fuse_context* fc;
	FileSystem* fs;
	fc=fuse_get_context();
	if (fc==NULL){
		fprintf(stderr, "Error: Can't get FUSE context.\n");
		return -EIO;
	}
	fs = (FileSystem*) fc->private_data;
	if (fs==NULL){
		fprintf(stderr, "LWAFS: Error: filesystem user data missing in fuse context\n");fflush(stderr);
		return -ENODEV;
	}
	if (path[0]!='/') return -ENOENT;
	if (strchr(&path[1], '/')) return -ENOENT;
	if (!strlen(&path[1])) return -ENOENT;
	int findex=_FileSystem_GetFileIndex(fs, &path[1]);
	if (findex<0) return -ENOENT;

	boolean fRead        = ((fi->flags & O_WRONLY) == 0)|((fi->flags & O_RDWR) != 0);
	boolean fWrite       = ((fi->flags & O_WRONLY) != 0)|((fi->flags & O_RDWR) != 0);

	// options that don't matter
	//boolean fNoFollow    = ((fi->flags & O_NOFOLLOW) != 0);
	//boolean fExclude     = ((fi->flags & O_EXCL) != 0);
	//boolean fLargefile   = ((fi->flags & O_LARGEFILE) != 0);

	// options that affect operation
	boolean fDirect      = ((fi->flags & O_DIRECT) != 0)  ;
	boolean fSynchronous = ((fi->flags & O_SYNC) != 0) || fDirect;
	boolean fNoBlock     = ((fi->flags & O_NONBLOCK) != 0);

	// unsupported operations
	boolean fAppend      = ((fi->flags & O_APPEND) != 0);
	boolean fCreate      = ((fi->flags & O_CREAT) != 0);
	boolean fDirectory   = ((fi->flags & O_DIRECTORY) != 0);
	boolean fAccessTime  = ((fi->flags & O_NOATIME) != 0);
	boolean fAsync       = ((fi->flags & O_ASYNC) != 0);
	boolean fNoTerm      = ((fi->flags & O_NOCTTY) != 0);
	boolean fTruncate    = ((fi->flags & O_TRUNC) != 0);

	if (fCreate || fDirectory || fAsync || fAccessTime || fNoTerm || fTruncate )
		return -ENOTSUP;
	if (fSynchronous)
		fi->direct_io=1;
	if (!fDirect)
		fi->keep_cache=1;

	MyFile * myfile;
	myfile = (MyFile *) malloc(sizeof(MyFile));
	if (!myfile)
		return -ENOMEM;
	myfile->index=-1;
	if (_FileSystem_GetFileIndex(fs, &path[1])==-1){
		free(myfile);
		return -ENOENT;
	}


	StatusCode sc= FileSystem_OpenFile(fs, myfile, &path[1], ((fWrite) ? WRITE : READ));



	if (sc!=SUCCESS){
		free(myfile);
		return -EACCES;
	}
	myfile->buffer = mmap(NULL,  _CHUNKSIZE_,  PROT_READ | PROT_WRITE,  MAP_PRIVATE | MAP_ANONYMOUS | MAP_LOCKED,  -1,  0);
	if (myfile->buffer == MAP_FAILED){
		free(myfile);
		fprintf(stderr, "LWAFS: Error: Cannot map memory for file\n");fflush(stderr);
		return -EIO;
	}
	myfile->buffer1stByte			= (size_t)-1l;
	myfile->contentSize			 	= 0;
	fi->fh=(uint64_t) myfile;
	if(TRACE)printf(HLR("\n\nFILE PTR at open: %lu\n\n"),fi->fh);
	myfile->newFile           = 0;
	myfile->sizeAtOpen        = fs->fileSystemHeaderData->fileInfo[myfile->index].size;
	myfile->maxWrittenAddress = 0;
	return 0;
}




/** Read data from an open file
 * Read should return exactly the number of bytes requested except
 * on EOF or error,  otherwise the rest of the data will be
 * substituted with zeroes.	 An exception to this is when the
 * 'direct_io' mount option is specified,  in which case the return
 * value of the read system call will reflect the return value of
 * this operation.
 */
size_t min (size_t a,  size_t b){
	return ((a<b) ? a : b );
}
int lwafs_read(const char * path,  char *buf,  size_t size,  off_t offset,  struct fuse_file_info *fi){
	if(TRACE)fprintf(stderr, HLG("LWAFS: TRACE: lwafs_read(\"%s\",  %lu)    %lu bytes at offset 0x%lx \n"), path, size,  size,  offset);fflush(stderr);

	int readComplete = 0;
	int error		  = 0;
	off_t 	current = 0;
	ssize_t result;
	StatusCode sc;
	MyFile * file = (MyFile *) fi->fh;
	FileSystem * fs=file->fs;
	size_t base=
		file->fs->fileSystemHeaderData->fileInfo[file->index].startPosition +
		_CHUNKSIZE_;
	size_t requisite1stByte;
	size_t copystart;
	size_t copylength;
	size_t fileLength=file->fs->fileSystemHeaderData->fileInfo[file->index].size;
	if ((offset+size) > fileLength)
		size=fileLength-offset;
	while (!readComplete){
		requisite1stByte =  ((offset+current)/_CHUNKSIZE_)*_CHUNKSIZE_;
		if ((!file->contentSize) || (file->contentSize && ((file->buffer1stByte != requisite1stByte) || (file->contentSize + file->buffer1stByte < offset+current)))){ // does the buffer hold data? // is it not the right data?
			sc = _FileSystem_SynchronousRead(file->fs, file->buffer, base+requisite1stByte, _CHUNKSIZE_, &result);
			if(TRACE)printf("READ CHUNK @ %lx\n",base+requisite1stByte);
			if ((sc!=SUCCESS) || result != _CHUNKSIZE_){
				return -EIO;
			}
			file->contentSize = result;
			file->buffer1stByte = requisite1stByte;
		}
		copystart  = (offset+current)%_CHUNKSIZE_;
		copylength = min((size-current) ,  (file->contentSize-copystart));
		memcpy(&buf[current], &file->buffer[copystart], copylength);
		current+=copylength;
		readComplete = (current==size);
	}
	if (error)
		return -1;
	else
		return current;
}

/** Get file system statistics
 * The 'f_frsize',  'f_favail',  'f_fsid' and 'f_flag' fields are ignored
 */
int lwafs_statfs(const char * path,  struct statvfs * stats){
	if(TRACE)fprintf(stderr, HLG("LWAFS: TRACE: lwafs_statfs(\"%s\")\n"), path);fflush(stderr);
	struct fuse_context* fc;
	FileSystem * fs;
	fc=fuse_get_context();
	fs = (FileSystem*) fc->private_data;
	if (fs==NULL){
		fprintf(stderr, "LWAFS: Error: filesystem user data missing in fuse context\n");fflush(stderr);
		return -ENODEV;
	}
	if (path[0]!='/') return -ENOENT;
	if (strchr(&path[1], '/')) return -ENOENT;
	int findex=_FileSystem_GetFileIndex(fs, &path[1]);
	if (findex<0) return -ENOENT;

	stats->f_bfree  = FileSystem_GetFreeSpace(fs) / 1024l;
	stats->f_bavail = stats->f_bfree;
	stats->f_blocks = fs->fileSystemHeaderData->filesystemInfo.diskSize / 1024l;
	stats->f_bsize  = 1024l;
	stats->f_ffree  = MAX_NUMBER_FILES-fs->fileSystemHeaderData->filesystemInfo.number_files_present;
	stats->f_files  = fs->fileSystemHeaderData->filesystemInfo.number_files_present;
	stats->f_namemax=1023l;
	return 0;

}


/** Release an open file
 * # Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.
 * # For every open() call there will be exactly one release() call
 * with the same flags and file descriptor.	 It is possible to
 * have a file opened more than once,  in which case only the last
 * release will mean,  that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 */
int lwafs_release(const char * path,  struct fuse_file_info * fi){
	if(TRACE)fprintf(stderr, HLG("LWAFS: TRACE: lwafs_release(\"%s\")\n"), path);fflush(stderr);
	if (fi->fh){
		//fprintf(stderr, HLR("LWAFS: closing(\"%s\")\n"), path);fflush(stderr);
		MyFile* myFile=(MyFile*) fi->fh;
		if(TRACE)printf(HLR("\n\nFILE PTR at close: %lu\n\n"),fi->fh);
		FileSystem * fs= myFile->fs;

		if (myFile->mode==WRITE) {
			FileInfo* fii=&fs->fileSystemHeaderData->fileInfo[myFile->index];

			if(TRACE)fprintf(stderr, HLB("LWAFS: TRACE: release writable file\n"));fflush(stderr);
			flushWriteBuffer(myFile->fs,myFile);

			// now do bookkeeping with filesize
			// LWAFS files are pre-allocated, so we have to do some trickery to comply with the conventional on-the-fly allocation mechanism
			size_t finalSize=0;
			if (myFile->newFile){
				if(TRACE)fprintf(stderr, HLB("LWAFS: TRACE: release writable file: was new file\n"));fflush(stderr);

				finalSize=myFile->maxWrittenAddress;
			} else {
				if(TRACE)fprintf(stderr, HLB("LWAFS: TRACE: release writable file: was not new file\n"));fflush(stderr);
				finalSize = ((myFile->maxWrittenAddress) > myFile->sizeAtOpen) ? (myFile->maxWrittenAddress) : (myFile->sizeAtOpen);
			}
			if(TRACE)fprintf(stderr, HLB("LWAFS: TRACE: release writable file: final size is %lu\n"),finalSize);fflush(stderr);
			//sanity check. write function should handle this, but check to make sure
			size_t currentAllocatedSize = myFile->fs->fileSystemHeaderData->fileInfo[myFile->index].size;
			size_t finalChunks=finalSize / _CHUNKSIZE_;
			size_t currentChunks=currentAllocatedSize / _CHUNKSIZE_;
			if (finalChunks > currentChunks){
				fprintf(stderr, HLR("LWAFS: ERROR: file size manipulation exceeded bounds or was corrupted.\n"));fflush(stderr);
			}

			resize(fs,myFile,finalSize);
			fii->currentPosition=finalSize;
			//FileSystem_CloseFile(myFile); // only need to "close" the file if it was open for writing
		}




		if(munmap(myFile->buffer, myFile->_CHUNKSIZE_)!=0)
			perror("munmap");
		myFile->buffer					= NULL;
		myFile->buffer1stByte			= (size_t)-1l;
		myFile->contentSize			 	= 0;
		FileSystem_CloseFile(myFile);
		fi->fh=(uint64_t) NULL;
		free(myFile);
	}
	return 0;
}

/** Open directory
 * Unless the 'default_permissions' mount option is given,
 * this method should check if opendir is permitted for this
 * directory. Optionally opendir may also return an arbitrary
 * filehandle in the fuse_file_info structure,  which will be
 * passed to readdir,  closedir and fsyncdir.
 */
int lwafs_opendir(const char * path,  struct fuse_file_info * fi){
	if(TRACE)fprintf(stderr, HLG("LWAFS: TRACE: lwafs_opendir(\"%s\")\n"), path);fflush(stderr);
	if (strcmp(path, "/")!=0) return -ENOENT;
	return 0;
}

/** Read directory
 * This supersedes the old getdir() interface.  New applications
 * should use this.
 * The filesystem may choose between two modes of operation:
 * 1) The readdir implementation ignores the offset parameter,  and
 * passes zero to the filler function's offset.  The filler
 * function will not return '1' (unless an error happens),  so the
 * whole directory is read in a single readdir operation.  This
 * works just like the old getdir() method.
 * 2) The readdir implementation keeps track of the offsets of the
 * directory entries.  It uses the offset parameter and always
 * passes non-zero offset to the filler function.  When the buffer
 * is full (or an error happens) the filler function will return
 * '1'.
 * Introduced in version 2.3
 */
int lwafs_readdir(const char * path,  void * buf,  fuse_fill_dir_t filler,  off_t offset,  struct fuse_file_info *fi){
	if(TRACE)fprintf(stderr, HLG("LWAFS: TRACE: lwafs_readdir(\"%s\")\n"), path);fflush(stderr);
	struct fuse_context* fc;
	FileSystem * fs;
	fc=fuse_get_context();
	fs = (FileSystem*) fc->private_data;
	if (fs==NULL){
		fprintf(stderr, "LWAFS: Error: filesystem user data missing in fuse context\n");fflush(stderr);
		return -ENODEV;
	}
	if (strcmp(path, "/")!=0) return -ENOENT;

	size_t nfiles=fs->fileSystemHeaderData->filesystemInfo.number_files_present;
	size_t i=0;
	int done=0;
	struct stat stbuf;
	char fullpath[4096];
	while ((i<nfiles) && ( !done)) {
		sprintf(fullpath, "/%s", fs->fileSystemHeaderData->fileInfo[i+1].name);
		if (!lwafs_getattr(fullpath,  &stbuf))
			done = filler( buf,  fs->fileSystemHeaderData->fileInfo[i+1].name, &stbuf ,  0);
		else
			done = filler( buf,  fs->fileSystemHeaderData->fileInfo[i+1].name, NULL ,  0);
		i++;
	}
	return 0;

}

/** Release directory */
int lwafs_releasedir(const char * path,  struct fuse_file_info * fi){
	if(TRACE)fprintf(stderr, HLG("LWAFS: TRACE: lwafs_releasedir(\"%s\")\n"), path);fflush(stderr);
	if (strcmp(path, "/")!=0) return -EBADF;
	return 0;
}

/**
 * Check file access permissions
 * This will be called for the access() system call.  If the
 * 'default_permissions' mount option is given,  this method is not
 * called.
 * mode is a mask consisting of one or more of R_OK,  W_OK,  X_OK and  F_OK
 */
int lwafs_access (const char * path,  int mode){
	if(TRACE)fprintf(stderr, HLG("LWAFS: TRACE: lwafs_access(\"%s\" %o)\n"), path, mode);fflush(stderr);
	struct fuse_context* fc;
	FileSystem * fs;
	fc=fuse_get_context();
	fs = (FileSystem*) fc->private_data;
	if (fs==NULL){
		fprintf(stderr, "LWAFS: Error: filesystem user data missing in fuse context\n");fflush(stderr);
		return -ENODEV;
	}


	if (strcmp(path, "/")==0){
		return 0;
	} else {
		if (!FileSystem_FileExists(fs,&path[1])) return -ENOENT;
		int idx=_FileSystem_GetFileIndex(fs,&path[1]);
		if (idx<0) return -ENOENT;
		FileInfo* fi=&fs->fileSystemHeaderData->fileInfo[idx];
		uid_t uid=fi->uid;
		gid_t gid=fi->gid;
		mode_t amode = fi->mode;
		if (fc->uid == uid){ // owner
			if ((mode & R_OK) && (!(S_IRUSR & amode))) return -EPERM;
			if ((mode & W_OK) && (!(S_IWUSR & amode))) return -EPERM;
			if ((mode & X_OK) && (!(S_IXUSR & amode))) return -EPERM;
			return 0;
		} else if (fc->gid == gid){ // group
			if ((mode & R_OK) && (!(S_IRGRP & amode))) return -EPERM;
			if ((mode & W_OK) && (!(S_IWGRP & amode))) return -EPERM;
			if ((mode & X_OK) && (!(S_IXGRP & amode))) return -EPERM;
			return 0;
		} else { //others
			if ((mode & R_OK) && (!(S_IROTH & amode))) return -EPERM;
			if ((mode & W_OK) && (!(S_IWOTH & amode))) return -EPERM;
			if ((mode & X_OK) && (!(S_IXOTH & amode))) return -EPERM;
			return 0;
		}
	}
}

int lwafs_mkdir(const char * path,  mode_t mode){
	return -EPERM;
}
int lwafs_rmdir(const char * path){
	return -EPERM;
}
int lwafs_symlink(const char * path,  const char * target){
	return -EPERM;
}
int lwafs_flush(const char * path,  struct fuse_file_info * fi){
	return 0;
}
int lwafs_link(const char * path,  const char * target){
	return -ENOTSUP;
}
int lwafs_fsync(const char * path,  int datasync,  struct fuse_file_info * fi){
	return 0;
}
int lwafs_setxattr (const char * path,  const char * attributeName,  const char * attributeValue,  size_t attributeSize,  int flags){
	return -ENOTSUP;
}
int lwafs_getxattr(const char * path,  const char * attributeName,  char * attributeValue,  size_t attributeSizeMax){
	return -ENOTSUP;
}
int lwafs_listxattr(const char * path,  char * attributeList,  size_t attributeListSizeMax){
	return -ENOTSUP;
}
int lwafs_removexattr(const char * path,  const char * attributeName){
	return -ENOTSUP;
}
int lwafs_fsyncdir(const char * path,  int datasync,  struct fuse_file_info * fi){
	return 0;
}
int lwafs_fgetattr(const char * path,  struct stat * stats,  struct fuse_file_info * fi){
	return lwafs_getattr(path, stats);
}
int lwafs_lock(const char * path,  struct fuse_file_info * fi,  int cmd,  struct flock * lock){
	return -ENOTSUP;
}



int lwafs_utimens(const char * path,  const struct timespec tv[2]){
	if(TRACE)fprintf(stderr, HLG("LWAFS: TRACE: lwafs_utimens(\"%s\")\n"), path);fflush(stderr);
	struct fuse_context* fc;
	FileSystem * fs;
	fc=fuse_get_context();
	fs = (FileSystem*) fc->private_data;
	if (fs==NULL){
		fprintf(stderr, "LWAFS: Error: filesystem user data missing in fuse context\n");fflush(stderr);
		return -ENODEV;
	}
	if (strcmp(path, "/")==0){
		//fprintf(stderr, HLR("LWAFS: TRACE: strcmp(path, \"/\")==0\n"));fflush(stderr);
		fs->fileSystemHeaderData->filesystemInfo.atime=tv[0];
		fs->fileSystemHeaderData->filesystemInfo.mtime=tv[1];
		return 0;
	} else {
		//fprintf(stderr, HLR("LWAFS: TRACE: strcmp(path, \"/\")!=0\n"));fflush(stderr);
		if ( (path[0]=='/') && (FileSystem_FileExists(fs, &path[1])) ){
			//fprintf(stderr, HLR("LWAFS: TRACE: (path[0]=='/') && (FileSystem_FileExists(fs, %s))\n"), &path[1]);fflush(stderr);
			int findex = _FileSystem_GetFileIndex(fs, &path[1]);
			FileInfo* fi=&(fs->fileSystemHeaderData->fileInfo[findex]);
			fi->atime = tv[0];
			fi->mtime = tv[1];
			return 0;
		} else {
			//fprintf(stderr, HLR("LWAFS: TRACE: !!!((path[0]=='/') && (FileSystem_FileExists(fs, &path[1])))\n"));fflush(stderr);
			return -ENOENT;
		}
	}

}
int lwafs_utime(const char * path, struct utimbuf * timebuf){
	if(TRACE)fprintf(stderr, HLG("LWAFS: TRACE: lwafs_utimens(\"%s\")\n"), path);fflush(stderr);
	struct timespec tv[2];
	tv[0].tv_sec  = timebuf->actime / 1000000;
	tv[0].tv_nsec = (timebuf->actime % 1000000)*1000;
	tv[1].tv_sec  = timebuf->modtime / 1000000;
	tv[1].tv_nsec = (timebuf->modtime % 1000000)*1000;
	return lwafs_utimens(path,tv);
}

int lwafs_rename(const char * path,  const char * newpath){
	if(TRACE)fprintf(stderr, HLG("LWAFS: TRACE: lwafs_rename(\"%s\", \"%s\")\n"), path, newpath);fflush(stderr);
	struct fuse_context* fc;
	FileSystem * fs;
	fc=fuse_get_context();
	fs = (FileSystem*) fc->private_data;
	if (fs==NULL){
		fprintf(stderr, "LWAFS: Error: filesystem user data missing in fuse context\n");fflush(stderr);
		return -ENODEV;
	}
	if (strcmp(path, "/")==0)		return -EPERM;
	if (path[0]!='/')				return -ENOENT;
	if (strchr(&path[1], '/'))		return -ENOENT;
	if (newpath[0]!='/')			return -EPERM;
	if (strchr(&newpath[1], '/'))	return -EPERM;
	if ( (path[0]=='/') && (FileSystem_FileExists(fs, &path[1])) ){
		//fprintf(stderr, HLR("LWAFS: TRACE: (path[0]=='/') && (FileSystem_FileExists(fs, %s))\n"), &path[1]);fflush(stderr);
		int findex = _FileSystem_GetFileIndex(fs, &path[1]);
		FileInfo* fi=&(fs->fileSystemHeaderData->fileInfo[findex]);
		strncpy(fi->name,  &newpath[1],  FILE_MAX_NAME_LENGTH);
		fi->name[FILE_MAX_NAME_LENGTH]='\0';
		return 0;
	} else {
		//fprintf(stderr, HLR("LWAFS: TRACE: !!!((path[0]=='/') && (FileSystem_FileExists(fs, &path[1])))\n"));fflush(stderr);
		return -ENOENT;
	}
}

int lwafs_chmod(const char * path,  mode_t mode){
	if(TRACE)fprintf(stderr, HLG("LWAFS: TRACE: lwafs_chmod(\"%s\" %o)\n"), path, mode);fflush(stderr);
	struct fuse_context* fc;
	FileSystem * fs;
	fc=fuse_get_context();
	fs = (FileSystem*) fc->private_data;
	if (fs==NULL){
		fprintf(stderr, "LWAFS: Error: filesystem user data missing in fuse context\n");fflush(stderr);
		return -ENODEV;
	}
	if (strcmp(path, "/")==0){
		return -EPERM;
	} else {
		fprintf(stderr, HLR("LWAFS: TRACE: strcmp(path, \"/\")!=0\n"));fflush(stderr);
		if ( (path[0]=='/') && (FileSystem_FileExists(fs, &path[1])) ){
			//fprintf(stderr, HLR("LWAFS: TRACE: (path[0]=='/') && (FileSystem_FileExists(fs, %s))\n"), &path[1]);fflush(stderr);
			int findex = _FileSystem_GetFileIndex(fs, &path[1]);
			FileInfo* fi=&(fs->fileSystemHeaderData->fileInfo[findex]);
			fi->amode = mode;
			return 0;
		} else {
			//fprintf(stderr, HLR("LWAFS: TRACE: !!!((path[0]=='/') && (FileSystem_FileExists(fs, &path[1])))\n"));fflush(stderr);
			return -ENOENT;
		}
	}
}
int lwafs_chown(const char * path,  uid_t UserId,  gid_t GroupId){
	if(TRACE)fprintf(stderr, HLG("LWAFS: TRACE: lwafs_chown(\"%s\" %d %d)\n"),  path,  UserId,  GroupId);fflush(stderr);
	struct fuse_context* fc;
	FileSystem * fs;
	fc=fuse_get_context();
	fs = (FileSystem*) fc->private_data;
	if (fs==NULL){
		fprintf(stderr, "LWAFS: Error: filesystem user data missing in fuse context\n");fflush(stderr);
		return -ENODEV;
	}
	if (strcmp(path, "/")==0){
		return -EPERM;
	} else {
		//fprintf(stderr, HLR("LWAFS: TRACE: strcmp(path, \"/\")!=0\n"));fflush(stderr);
		if ( (path[0]=='/') && (FileSystem_FileExists(fs, &path[1])) ){
			//fprintf(stderr, HLR("LWAFS: TRACE: (path[0]=='/') && (FileSystem_FileExists(fs, %s))\n"), &path[1]);fflush(stderr);
			int findex = _FileSystem_GetFileIndex(fs, &path[1]);
			FileInfo* fi=&(fs->fileSystemHeaderData->fileInfo[findex]);
			fi->uid = UserId;
			fi->gid = GroupId;
			return 0;
		} else {
			//fprintf(stderr, HLR("LWAFS: TRACE: !!!((path[0]=='/') && (FileSystem_FileExists(fs, &path[1])))\n"));fflush(stderr);
			return -ENOENT;
		}
	}
}
int lwafs_bmap(const char * path,  size_t blocksize,  uint64_t *idx){
	return -EPERM;
}




#define DEFAULTFILESIZE 1024l * 1024l * 256l

int lwafs_create(const char * path,  mode_t mode,  struct fuse_file_info * fi){
	if(TRACE)fprintf(stderr, HLG("LWAFS: TRACE: lwafs_create(\"%s\" %o)\n"),  path,  mode);fflush(stderr);

	// options that affect operation
	boolean fRead        = ((fi->flags & O_WRONLY) == 0)|((fi->flags & O_RDWR) != 0);
	boolean fWrite       = ((fi->flags & O_WRONLY) != 0)|((fi->flags & O_RDWR) != 0);
	boolean fDirect      = ((fi->flags & O_DIRECT) != 0)  ;
	boolean fSynchronous = ((fi->flags & O_SYNC) != 0) || fDirect;
	boolean fNoBlock     = ((fi->flags & O_NONBLOCK) != 0);

	// unsupported operations
	boolean fAppend      = ((fi->flags & O_APPEND) != 0);
	boolean fCreate      = ((fi->flags & O_CREAT) != 0);
	boolean fDirectory   = ((fi->flags & O_DIRECTORY) != 0);
	boolean fAccessTime  = ((fi->flags & O_NOATIME) != 0);
	boolean fAsync       = ((fi->flags & O_ASYNC) != 0);
	boolean fNoTerm      = ((fi->flags & O_NOCTTY) != 0);
	boolean fTruncate    = ((fi->flags & O_TRUNC) != 0);

	if(TRACE)fprintf(stderr, HLG("fRead         %d\n"),  fRead);fflush(stderr);
	if(TRACE)fprintf(stderr, HLG("fWrite        %d\n"),  fWrite);fflush(stderr);
	if(TRACE)fprintf(stderr, HLG("fDirect       %d\n"),  fDirect);fflush(stderr);
	if(TRACE)fprintf(stderr, HLG("fSynchronous  %d\n"),  fSynchronous);fflush(stderr);
	if(TRACE)fprintf(stderr, HLG("fNoBlock      %d\n"),  fNoBlock);fflush(stderr);
	if(TRACE)fprintf(stderr, HLG("fAppend       %d\n"),  fAppend);fflush(stderr);
	if(TRACE)fprintf(stderr, HLG("fCreate       %d\n"),  fCreate);fflush(stderr);
	if(TRACE)fprintf(stderr, HLG("fDirectory    %d\n"),  fDirectory);fflush(stderr);
	if(TRACE)fprintf(stderr, HLG("fAccessTime   %d\n"),  fAccessTime);fflush(stderr);
	if(TRACE)fprintf(stderr, HLG("fAsync        %d\n"),  fAsync);fflush(stderr);
	if(TRACE)fprintf(stderr, HLG("fNoTerm       %d\n"),  fNoTerm);fflush(stderr);
	if(TRACE)fprintf(stderr, HLG("fTruncate     %d\n"),  fTruncate);fflush(stderr);

	if (fDirectory || fAsync || fAccessTime || fNoTerm)
		return -ENOTSUP;
	if (!fWrite) {
		return -EPERM; //since can't create a file that you're not writing to
	}
	if (fSynchronous)		fi->direct_io=1;
	if (!fDirect)		    fi->keep_cache=1;

	struct fuse_context* fc;
	FileSystem * fs;
	fc=fuse_get_context();
	fs = (FileSystem*) fc->private_data;
	if (fs==NULL){
		fprintf(stderr, "LWAFS: Error: filesystem user data missing in fuse context\n");fflush(stderr);
		return -ENODEV;
	}

	if (!S_ISREG(mode)) return -EPERM;
	if (strchr(&path[1], '/')) return -ENOTDIR;
	if (FileSystem_FileExists(fs, &path[1])){
		if (fTruncate){
			if (lwafs_unlink(path)) return -EIO;
		} else {
			return -EEXIST;
		}
	}
	if(!FileSystem_IsFileCreatable(fs, (char*)&path[1], DEFAULTFILESIZE, 0))
		return -ENOSPC;
	int findex;
	if(FileSystem_CreateFile(fs, (char*)&path[1], DEFAULTFILESIZE, 0, &findex) != SUCCESS)
		return -EIO;


	MyFile * myfile;
	myfile = (MyFile *) malloc(sizeof(MyFile));
	if (!myfile)		return -ENOMEM;
	myfile->index=-1;
	StatusCode sc= FileSystem_OpenFile(fs, myfile, &path[1], WRITE);
	if (sc!=SUCCESS){
		free(myfile);
		return -EIO;
	}
	myfile->buffer = mmap(NULL,  myfile->_CHUNKSIZE_,  PROT_READ | PROT_WRITE,  MAP_PRIVATE | MAP_ANONYMOUS | MAP_LOCKED,  -1,  0);
	if (myfile->buffer == MAP_FAILED){
		free(myfile);
		//TODO close file
		fprintf(stderr, "LWAFS: Error: Cannot map memory for file\n");fflush(stderr);
		return -EIO;
	}
	fs->fileSystemHeaderData->fileInfo[myfile->index].size=0;
	myfile->buffer1stByte			= (size_t)-1l;
	myfile->contentSize			 	= 0;
	fi->fh=(uint64_t) myfile;
	if(TRACE)printf(HLR("\n\nFILE PTR at create: %lu\n\n"),fi->fh);
	myfile->newFile    = 1;
	myfile->sizeAtOpen = 0;
	myfile->maxWrittenAddress = 0;
	return 0;

}

int lwafs_mknod(const char * path,  mode_t mode,  dev_t device){
	return -ENOTSUP;
}

int resize(FileSystem* fs,  MyFile* myfile,  size_t newSize){

	if(TRACE)fprintf(stderr, HLB("LWAFS: TRACE: resize: new size is %lu\n"),newSize);fflush(stderr);

	// possibilities are :
	// 1) file is last file,  and we can expand arbitrarily assuming enough free space
	// 2) file is somewhere in the middle,  but space exists to expand the region
	// 3) file is in the middle,  and no space exists to expand there,  but space exists at the end of the current
	// 4) no free space anywhere...
	FileInfo * fi = &(fs->fileSystemHeaderData->fileInfo[myfile->index]);
	if (newSize == fi->size) {
		//nothing to do
		return 0;
	}
	size_t numChunks=((newSize/_CHUNKSIZE_)+1);
	size_t sizeOnDisk=(numChunks+2) * _CHUNKSIZE_;

	int conflictIndex=_FileSystem_RangeConflictsIgnore(fs, fi->startPosition, fi->startPosition+sizeOnDisk, myfile->index);
	if (!conflictIndex){ // good,  the file can be expanded in situ
		if(TRACE)fprintf(stderr,HLY("resize in situ\n"));
		//this covers case 1, 2
		// just need to move the stop tag to the right place...
		size_t curStopLocation = fi->stopPosition;
		size_t newStopLocation = (fi->startPosition + sizeOnDisk );
		void * patternBuff=mmap(
				NULL,
				_CHUNKSIZE_,
				PROT_READ | PROT_WRITE,  MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE | MAP_LOCKED,
				-1,
				0);
		if ((patternBuff==NULL)||(patternBuff==MAP_FAILED)){
			fprintf(stderr,"[FILE SYSTEM] Error: can't allocate memory for file resize.\n");
			perror("[FILE SYSTEM] mmap(...)");
			return -EIO;
		}
		_FileSystem_CreatePattern(patternBuff,  _CHUNKSIZE_);
		ssize_t bytesWritten0=0;
		int result=_FileSystem_SynchronousWrite(fs, patternBuff, newStopLocation - _CHUNKSIZE_,  _CHUNKSIZE_, &bytesWritten0);
		munmap(patternBuff, _CHUNKSIZE_); // do this regardless of outcome...
		if ((result == SUCCESS) && bytesWritten0 == _CHUNKSIZE_){
			fi->stopPosition = newStopLocation;
			//fi->size         = newSize;
			return 0;
		} else {
			fprintf(stderr,HLR("[FILE SYSTEM] Error: relocating stop tag write failed. result=%d written=%lu\n"),result,bytesWritten0);
			return -EIO;
		}
	} else { // no room to expand the file in it's current location,  so we need to move the file and change the start/stop tags,  etc.
		if(TRACE)fprintf(stderr,HLY("resize ex situ\n"));
		//this covers case 3, 4
		size_t tempIndex=fs->fileSystemHeaderData->filesystemInfo.number_files_present+1; // +1 is because the 0th file is actually the filesystem header
		size_t i=0;
		for( i=1; i<tempIndex; i++){
			// there are files,  so the candidate positions all start at the end of a file :)
			size_t newstartPosition = fs->fileSystemHeaderData->fileInfo[i].stopPosition; //start looking at the 1st position after this file
			size_t newStopPosition  = newstartPosition + sizeOnDisk;
			if ( ! _FileSystem_RangeConflicts(fs,  newstartPosition,  newStopPosition)){
				// found a place to put it...
				void * readBuff=mmap(
						NULL,
						_CHUNKSIZE_,
						PROT_READ | PROT_WRITE,  MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE | MAP_LOCKED,
						-1,
						0);
				if ((readBuff==NULL)||(readBuff==MAP_FAILED)){
					printf("[FILE SYSTEM] Error: can't allocate memory for file resize.\n");
					perror("[FILE SYSTEM] mmap(...)");
					return -EIO;
				}
				size_t curPos=0;
				size_t bytesLeft = newStopPosition - newstartPosition;
				ssize_t bytesRead=0;
				ssize_t bytesWritten=0;
				int error=0;
				while (bytesLeft && !error){
					int result = _FileSystem_SynchronousRead(fs, readBuff, fi->startPosition+curPos+_CHUNKSIZE_, _CHUNKSIZE_, &bytesRead);
					if ( (result != SUCCESS) || (bytesRead!=_CHUNKSIZE_) ){
						// error
						error=1;
					} else {
						result = _FileSystem_SynchronousWrite(fs, readBuff, newstartPosition+curPos+_CHUNKSIZE_, _CHUNKSIZE_, &bytesWritten);
						if ( (result != SUCCESS) || (bytesWritten!=_CHUNKSIZE_) ){
							//error
							error=1;
						} else {
							bytesLeft -= _CHUNKSIZE_;
						    curPos    += _CHUNKSIZE_;
						}
					}
				}
				munmap(readBuff, _CHUNKSIZE_); // do this regardless of outcome...
				if (error) {
					return -EIO;
				} else {
					fi->startPosition = newstartPosition;
					fi->stopPosition  = newStopPosition;
					//fi->size          = newSize;
					return 0;
				}
			}
		}
		// if we got here,  then there were no legal placements
		return -ENOSPC; // case 4
	}
}

void readAheadWriteBuffer(FileSystem* fs,  MyFile* myfile){
	if(TRACE)fprintf(stderr, HLB("\nread ahead: top\n"));

	if (!myfile->buffer) {
		printf("LWAFS: Error: flush called on non-existant buffer.\n");
		return;
	}
	FileInfo * fi = &(fs->fileSystemHeaderData->fileInfo[myfile->index]);
	ssize_t bytesRead;
	if(TRACE)fprintf(stderr, HLB("\nread ahead: at %lu (%lu physical)\n"),myfile->buffer1stByte,fi->startPosition+myfile->buffer1stByte+_CHUNKSIZE_);
	int result = _FileSystem_SynchronousRead(fs, myfile->buffer, fi->startPosition+myfile->buffer1stByte+_CHUNKSIZE_, _CHUNKSIZE_, &bytesRead);
	if ( (result != SUCCESS) || (bytesRead!=_CHUNKSIZE_) ){
		printf("LWAFS: Error: flush remainder read failed.\n");
		assert(0);
		return;
	}
	myfile->contentSize       =  _CHUNKSIZE_;
}
void flushWriteBuffer(FileSystem* fs,  MyFile* myfile){
	if(TRACE)fprintf(stderr, HLB("\nwrite ahead: top\n"));
	//does no checking of file boundaries,  etc.
	// use w/ caution
	if (!myfile->buffer) {
		printf("LWAFS: Error: flush called on non-existant buffer.\n");
		return;
	}
	if (!myfile->contentSize) return;
	ssize_t bytesWritten;
	FileInfo * fi = &(fs->fileSystemHeaderData->fileInfo[myfile->index]);
	if(TRACE)fprintf(stderr, HLB("\nwrite flush: at %lu (%lu physical)\n"),myfile->buffer1stByte,fi->startPosition+myfile->buffer1stByte+_CHUNKSIZE_);
	int result = _FileSystem_SynchronousWrite(fs, myfile->buffer, fi->startPosition+myfile->buffer1stByte+_CHUNKSIZE_, _CHUNKSIZE_, &bytesWritten);
	if ( (result != SUCCESS) || (bytesWritten!=_CHUNKSIZE_) ){
		printf("LWAFS: Error: flush write failed.\n");
		assert(0);
	}
	myfile->contentSize       =  0;
}

size_t writeToBuffer(FileSystem* fs,  MyFile* myfile, char* buffer, size_t offset, size_t size){

	size_t requisite1stByte =  ((offset)/_CHUNKSIZE_)*_CHUNKSIZE_;
	size_t current1stByte   = myfile->buffer1stByte;
	size_t innerOffset     = offset - requisite1stByte;
	size_t bytesBeforeBoundary = _CHUNKSIZE_ - innerOffset;
	size_t bytesToWrite = (size<bytesBeforeBoundary) ? size: bytesBeforeBoundary;

	if (requisite1stByte!=current1stByte){
		if (myfile->contentSize) flushWriteBuffer(fs,myfile);
		myfile->buffer1stByte=requisite1stByte;
		if (!myfile->newFile  || (requisite1stByte < myfile->maxWrittenAddress-_CHUNKSIZE_))
			readAheadWriteBuffer(fs,myfile);
	}
	memcpy( &myfile->buffer[innerOffset], buffer, bytesToWrite);
	if (!myfile->contentSize) myfile->contentSize = innerOffset+bytesToWrite;
	return bytesToWrite;
}

int lwafs_write(const char * path,  const char * buf,  size_t size,  off_t offset,  struct fuse_file_info * fi){
	if(TRACE)fprintf(stderr, HLG("LWAFS: TRACE: lwafs_write(\"%s\",  %lu)    %lu bytes at offset 0x%lx \n"), path, size,  size,  offset);fflush(stderr);
	struct fuse_context* fc;
	FileSystem * fs;
	fc=fuse_get_context();
	fs = (FileSystem*) fc->private_data;
	if (fs==NULL){
		fprintf(stderr, "LWAFS: Error: filesystem user data missing in fuse context\n");fflush(stderr);
		return -ENODEV;
	}

	MyFile * myfile = (MyFile *) fi->fh;
	if (!myfile) return -EIO;

	FileInfo* fii=&fs->fileSystemHeaderData->fileInfo[myfile->index];

	size_t currSizeMax = (fii->stopPosition-fii->startPosition)-(2*_CHUNKSIZE_); // the current allocated size.
	if (size+offset>currSizeMax){
		int resizeResult=resize(fs, myfile, 2*currSizeMax);
		if (resizeResult) return resizeResult;
	}

	size_t lastAddress=offset+size;
	if (myfile->maxWrittenAddress < lastAddress)
		myfile->maxWrittenAddress = lastAddress;

	size_t pos=0;
	int error=0;
	size_t written=0;
	while (pos<size && !error){
		written=writeToBuffer(fs,myfile,(char*)&buf[pos],offset+pos,size-pos);
		pos+=written;
		if (written!=size-pos) error=1;
	}
	return pos;
}

int lwafs_truncate(const char * path,  off_t length){
	if(TRACE)fprintf(stderr, HLG("LWAFS: TRACE: lwafs_truncate(\"%s\",  %lu)    \n"), path, length);fflush(stderr);
	struct fuse_context* fc;
	FileSystem * fs;
	fc=fuse_get_context();
	fs = (FileSystem*) fc->private_data;
	if (fs==NULL){
		fprintf(stderr, "LWAFS: Error: filesystem user data missing in fuse context\n");fflush(stderr);
		return -ENODEV;
	}


	if (path[0]!='/') return -ENOENT;
	if (strchr(&path[1], '/')) return -ENOENT;
	if (!strlen(&path[1])) return -ENOENT;
	int findex=_FileSystem_GetFileIndex(fs, &path[1]);
	if (findex<0) return -ENOENT;

	MyFile myfile;
	myfile.index=-1;
	myfile.buffer = mmap(NULL,  _CHUNKSIZE_,  PROT_READ | PROT_WRITE,  MAP_PRIVATE | MAP_ANONYMOUS | MAP_LOCKED,  -1,  0);
	if (myfile.buffer == MAP_FAILED){
		fprintf(stderr, "LWAFS: Error: Cannot map memory for file\n");fflush(stderr);
		return -EIO;
	}
	myfile.buffer1stByte			= 0;
	myfile.contentSize			 	= 0;
	myfile.newFile    = 0;
	myfile.sizeAtOpen = 0;
	myfile.maxWrittenAddress = 0;

	StatusCode sc= FileSystem_OpenFile(fs, &myfile, &path[1], WRITE);
	if (sc!=SUCCESS) {
		munmap(myfile.buffer, _CHUNKSIZE_);
		return -EIO;
	}
	FileInfo* fi=&fs->fileSystemHeaderData->fileInfo[myfile.index];
	size_t cursize=fi->size;


	int res = resize(fs,&myfile,length);
	if (res){
		fi->currentPosition=cursize;
		FileSystem_CloseFile(&myfile);
		munmap(myfile.buffer, myfile._CHUNKSIZE_);
		return res;
	}
	if (FileSystem_CloseFile(&myfile)!=SUCCESS) return -EIO;
	munmap(myfile.buffer, myfile._CHUNKSIZE_);
	return 0;

}
int lwafs_ftruncate(const char * path,  off_t offset,  struct fuse_file_info * fi){
	if(TRACE)fprintf(stderr, HLG("LWAFS: TRACE: lwafs_ftruncate(\"%s\",  %lu)    \n"), path, offset);fflush(stderr);
	struct fuse_context* fc;
	FileSystem * fs;
	fc=fuse_get_context();
	fs = (FileSystem*) fc->private_data;
	if (fs==NULL){
		fprintf(stderr, "LWAFS: Error: filesystem user data missing in fuse context\n");fflush(stderr);
		return -ENODEV;
	}

	MyFile * myfile = (MyFile *) fi->fh;
	if (!myfile) return -EIO;
	if (myfile->contentSize) flushWriteBuffer(fs,myfile);
	return resize(fs,myfile,offset);

}

