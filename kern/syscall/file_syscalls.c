/*
 * File-related system call implementations.
 */

#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <kern/limits.h>
#include <kern/seek.h>
#include <kern/stat.h>
#include <lib.h>
#include <uio.h>
#include <proc.h>
#include <current.h>
#include <synch.h>
#include <copyinout.h>
#include <vfs.h>
#include <vnode.h>
#include <openfile.h>
#include <filetable.h>
#include <syscall.h>

/*
 * open() - get the path with copyinstr, then use openfile_open and
 * filetable_place to do the real work.
 */
int
sys_open(const_userptr_t upath, int flags, mode_t mode, int *retval)
{
	const int allflags = O_ACCMODE | O_CREAT | O_EXCL | O_TRUNC | O_APPEND | O_NOCTTY;

	//Allocate NAME_MAX blocks of contigous char sized memory.
	char *kpath = kmalloc(sizeof(char)*NAME_MAX);
	
	struct openfile *file;
	int result = 0;
	size_t actual = 0;

	//We need to make sure provided flags are valid by using allflags
	//and bitwise & operator. Since the flags are powers of two
	//a bitwise & will produce 1 

	if(flags & allflags){

	//COPYFILENAME FROM USERPTR TO CHAR ARRAY
	result = copyinstr(upath, kpath, NAME_MAX, &actual);
	if(result){
		return result;
	}


	if (flags > O_NOCTTY) {
        *retval = EINVAL;
        return -1;
    }

	result = openfile_open(kpath, flags, mode, &file);
	if(result){ // 0 indicates success
		return result;
	}


	/* 
	 * Your implementation of system call open starts here.  
	 *
	 * Check the design document design/filesyscall.txt for the steps
	 */
	/*
	(void) upath; // suppress compilation warning until code gets written
	(void) flags; // suppress compilation warning until code gets written
	(void) mode; // suppress compilation warning until code gets written
	(void) retval; // suppress compilation warning until code gets written
	(void) allflags; // suppress compilation warning until code gets written
	(void) kpath; // suppress compilation warning until code gets written
	(void) file; // suppress compilation warning until code gets written
	*/
	result = filetable_place(curproc->p_filetable, file, retval);
	if(result)
		return result;
	}	
	else
		result = ENOFLG;

	return result;
}

/*
 * read() - read data from a file
 */
int
sys_read(int fd, userptr_t buf, size_t size, int *retval)
{
	   struct iovec iov;
	   struct uio userio;
	   struct openfile *file;
	   int result = 0;

	   if(!filetable_okfd(curproc->p_filetable, fd)){
	   		return EBADF;
	   	}

	   	result = filetable_get(curproc->p_filetable, fd, &file);

	   	if(result){
	   		return result;
	   	}

	   	lock_acquire(file->of_offsetlock);

	   	if(file->of_accmode == O_WRONLY){
	   		lock_release(file->of_offsetlock);
	   		return EBADF;
	   	}

	   	uio_uinit(&iov, &userio, buf, size, 0, UIO_READ);

	   	result = VOP_READ(file->of_vnode, &userio);
	   	if(result){
	   		lock_release(file->of_offsetlock);
	   		return result;
	   	}

	   	file->of_offset = userio.uio_offset;

	   	lock_release(file->of_offsetlock);

       /* 
        * Your implementation of system call read starts here.  
        *
        * Check the design document design/filesyscall.txt for the steps
        */
       /*
       (void) fd; // suppress compilation warning until code gets written
       (void) buf; // suppress compilation warning until code gets written
       (void) size; // suppress compilation warning until code gets written
       (void) retval; // suppress compilation warning until code gets written
		*/

		*retval = size - userio.uio_resid;

        return 0;
}

/*
 * write() - write data to a file
 */

int 
sys_write(int fd, userptr_t buf, size_t size, int *retval){
	   struct iovec iov;
	   struct uio userio;
	   struct openfile *file;
	   int result = 0;

	   if(!filetable_okfd(curproc->p_filetable, fd)){
	   		return EBADF;
	   	}

	   	result = filetable_get(curproc->p_filetable, fd, &file);

	   	if(result){
	   		return result;
	   	}

	   	lock_acquire(file->of_offsetlock);

	   	if(file->of_accmode == O_RDONLY){
	   		lock_release(file->of_offsetlock);
	   		return EBADF;
	   	}

	   	uio_uinit(&iov, &userio, buf, size, 0, UIO_WRITE);

	   	result = VOP_WRITE(file->of_vnode, &userio);
	   	if(result){
	   		lock_release(file->of_offsetlock);
	   		return result;
	   	}

	   	file->of_offset = userio.uio_offset;

	   	lock_release(file->of_offsetlock);

       /* 
        * Your implementation of system call read starts here.  
        *
        * Check the design document design/filesyscall.txt for the steps
        */
       /*
       (void) fd; // suppress compilation warning until code gets written
       (void) buf; // suppress compilation warning until code gets written
       (void) size; // suppress compilation warning until code gets written
       (void) retval; // suppress compilation warning until code gets written
		*/

		*retval = userio.uio_resid;

        return 0;
}

int
sys_close(int fd){
	int result = 0;
	struct openfile *oldfile;
	//validate fd number
	if(!filetable_okfd(curproc->p_filetable, fd)){
		return EBADF;
	}

	filetable_placeat(curproc->p_filetable, NULL, fd, &oldfile);

	if(result){
		return result;
	}

	KASSERT(oldfile == NULL);

	openfile_decref(oldfile);

	return 0;
}

/*
int 
sys_meld(userptr_t pn1, userptr_t pn2, userptr_t pn3){
	char pn1arr[NAME_MAX];
	char pn2arr[NAME_MAX];
	char pn3arr[NAME_MAX];

	struct openfile *file1;
	struct openfile *file2;
	struct openfile *file3;

	int fd1;
	int fd2;
	int fd3;


	int result = 0;


	copyinstr(pn1, pn1arr, sizeof(pn1arr), NULL);
	copyinstr(pn2, pn2arr, sizeof(pn2arr), NULL);
	copyinstr(pn3, pn3arr, sizeof(pn3arr), NULL);

	result = openfile_open(pn1arr, O_RDONLY, NULL, &file1);
	if(result){ // 0 indicates success
		return result;
	}

	else{
		return ENOFLG; //My custom error code.
	}

	result = openfile_open(kpath, O_RDONLY, NULL, &file2);
	if(result){ // 0 indicates success
		return result;
	}
	else{
		return ENOFLG; //My custom error code.
	}

	result = openfile_open(kpath, O_WRONLY, NULL, &file3);
	if(result){ // 0 indicates success
		return result;
	}
	else{
		return ENOFLG; //My custom error code.
	}

	result = filetable_place(curproc->p_filetable, file1, &fd1);
	if(result)
		return result;

	result = filetable_place(curproc->p_filetable, file2, &fd2);
	if(result)
		return result;

	result = filetable_place(curproc->p_filetable, file3, &fd3);
	if(rseult)
		return result;


*/
/*
 * close() - remove from the file table.
 */

/* 
* meld () - combine the content of two files word by word into a new file
*/
