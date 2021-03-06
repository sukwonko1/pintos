#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include <devices/shutdown.h> /* Added to use 'shutdown_power_off' function */
#include <filesys/filesys.h> /* Added to use filesystem related function */
#include <filesys/file.h> /* Added to use filesystem related function */
#include <devices/input.h> /* Added to use input_getc() function */
#include "userprog/process.h" /* Added to use process_execute() */
#include "threads/synch.h" /* Added to use lock */

static void syscall_handler (struct intr_frame *);

/* This file structure will be used for file descriptor table. Copied from file.c */
struct file 
{
	struct inode *inode;        /* File's inode. */
	off_t pos;                  /* Current position. */
	bool deny_write;            /* Has file_deny_write() been called? */
};

struct lock filesys_lock; /* Added to use filesystem lock to prevent unexpected situation. */

void check_address(void *addr);
void get_argument(unsigned int *esp, unsigned int *arg[5], int count);

void halt(void);
void exit(int status);
bool create(const char *file, unsigned int initial_size);
bool remove(const char *file);
tid_t exec(char *exec_filename);
int wait(tid_t tid);
int open(const char *open_filename);
int filesize(int fd);
int read(int fd, char *buffer, unsigned int size);
int write(int fd, char *buffer, unsigned int size);
void seek(int fd, unsigned int position);
unsigned int tell(int fd);
void close(int fd);

void
syscall_init (void) 
{
	lock_init(&filesys_lock);
	intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
  	unsigned int *esp = (unsigned int*)(f->esp); //Stack pointer from interrupt frame
	check_address(esp);

	unsigned int *argument[5]; //Arguments for system call will be stored temporary.
	int system_call_number = *(int*)esp; //Recognize system call number. This will be used for switch case block.

	esp = esp + 1; //Increase stack pointer value.
	check_address(esp); /* Check again */

	switch(system_call_number)
	{
		case SYS_HALT:
			halt();
			break;

		case SYS_EXIT:
			{
				get_argument(esp, argument, 1); //One argument will be used.

				/* Argument type casting section. */
				int exit_status = (int)*(argument[0]); //Type casting.

				exit(exit_status);
			}
			break;

		case SYS_EXEC:
			{
				get_argument(esp, argument, 1); //One argument will be used.

				/* Argument type casting section. */
				char *exec_filename = (char*)*(argument[0]); //Type casting.

				f->eax = exec(exec_filename); //Store return value to eax.
			}
			break;
			
		case SYS_WAIT:
			{
				get_argument(esp, argument, 1); //One argument will be used.

				/* Argument type casting section. */
				int wait_tid = (int)*(argument[0]); //Type casting.

				f->eax = wait(wait_tid); //Store return value to eax.
			}
			break;

		case SYS_CREATE:
			{
				get_argument(esp, argument, 2); //Two arguments will be used.

				/* Argument type casting section. */
				char *create_filename = (char*)*(argument[0]);
				unsigned int initial_size = (int)*(argument[1]);

				f->eax = create(create_filename, initial_size); //Store return value to eax.
			}
			break;
			
		case SYS_REMOVE:
			{
				get_argument(esp, argument, 1); //One argument will be used.

				/* Argument type casting section. */
				char *remove_filename = (char*)*(argument[0]); //Type casting.

				f->eax = remove(remove_filename); //Store return value to eax.
			}
			break;

		case SYS_OPEN:
			{
				get_argument(esp, argument, 1); //One argument will be used.

				/* Argument type casting section. */
				char *open_filename = (char*)*(argument[0]);

				f->eax = open(open_filename); //Store return value to eax.
			}
			break;
			
		case SYS_FILESIZE:
			{
				get_argument(esp, argument, 1); //One argument will be used.

				/* Argument type casting section. */
				int fd = (int)*(argument[0]);

				f->eax = filesize(fd); //Store return value to eax.
			}
			break;

		case SYS_READ:
			{
				get_argument(esp, argument, 3); //Three arguments will be used.

				/* Argument type casting section. */
				int fd = (int)*(argument[0]);
				char *buffer = (char*)*(argument[1]);
				unsigned int size = (unsigned int)*(argument[2]);

				f->eax = read(fd, buffer, size); //Store return value to eax.
			}
			break;

		case SYS_WRITE:
			{
				get_argument(esp, argument, 3); //Three arguments will be used.

				/* Argument type casting section. */
				int fd = (int)*(argument[0]);
				char *buffer = (char*)*(argument[1]);
				unsigned int size = (unsigned int)*(argument[2]);

				f->eax = write(fd, buffer, size); //Store return value to eax.
			}
			break;

		case SYS_SEEK:
			{
				get_argument(esp, argument, 2); //Two arguments will be used.

				/* Argument type casting section. */
				int fd = (int)*(argument[0]);
				unsigned int position = (unsigned int)*(argument[1]);

				seek(fd, position); //Store return value to eax.
			}
			break;
			
		case SYS_TELL:
			{
				get_argument(esp, argument, 1); //Three arguments will be used.

				/* Argument type casting section. */
				int fd = (int)*(argument[0]);

				f->eax = tell(fd); //Store return value to eax.
			}
			break;
			
		case SYS_CLOSE:
			{
				get_argument(esp, argument, 1); //Three arguments will be used.

				/* Argument type casting section. */
				int fd = (int)*(argument[0]);

				close(fd); //Store return value to eax.
			}
			break;
			
		default:
			NOT_REACHED (); //If handler is correctly implemented, this line should not be executed.
			break;
	}
}

/* Check address if it is valid address */
void
check_address(void *addr)
{
	/* Check address and if address value is out of range, exit process. */
	if((unsigned int)addr <= 0x8048000 || (unsigned int)addr >= 0xc0000000) exit(-1);
}

/* Get argument from esp and store them into kernel stack */
void
get_argument(unsigned int *esp, unsigned int *arg[5], int count)
{
	int i;
	for(i = 0; i < count; i++)
	{
		/* Before store arguments from esp to kernel stack, check every esp pointer value. */
		check_address((void*)esp);
		arg[i] = esp; /* Insert each esp address into kernel stack */
		esp++;
	}
}

/* Shutdown system */
void
halt(void)
{
	shutdown_power_off();
}

/* Exit current process */
void
exit(int status)
{
	struct thread *current_thread = thread_current(); //Get current thread information. This will be used to get thread name.
	printf("%s: exit(%d)\n", current_thread->name, status); //Display exit task information.
	current_thread->exit_status = status; //Store exit status into child_process descriptor.
	
	thread_exit();
}

/* Create file */
bool
create(const char *file, unsigned int initial_size)
{
	/* If argument is pointer value, check this if it is out of range. */
	check_address((void*)file);

	lock_acquire(&filesys_lock); //lock for atomic file operation.
	bool result = filesys_create(file, initial_size);
	lock_release(&filesys_lock); //Unlock for atomic file operation.

	return result; //If success, return true. Else, return false.
}

/* Remove file */
bool
remove(const char *file)
{
	/* If argument is pointer value, check this if it is out of range. */
	check_address((void*)file);

	lock_acquire(&filesys_lock); //lock for atomic file operation.
	bool result = filesys_remove(file);
	lock_release(&filesys_lock); //Unlock for atomic file operation.

	return result; //If success, return true. Else, return false.return result;
}

/* Execute child process */
tid_t
exec(char *exec_filename)
{
	tid_t executed_process_tid = process_execute(exec_filename); //Get tid of executed process.
	struct thread *executed_process_desc = get_child_process(executed_process_tid); //Get object of correspond tid.

	if(executed_process_desc) //If tid exists, then...
	{
		sema_down(&executed_process_desc->load_sema); //Block parent process.

		if(executed_process_desc->is_load) //If successfully load
		{
			return executed_process_tid;
		}
		else //If failed to load
		{
			return -1;
		}
	}
	else //If load fail, return -1.
	{
		return -1;
	}
}

/* Wait for child process to exit */
int
wait(tid_t tid)
{
	return process_wait(tid);
}

/* Open file */
int
open(const char *open_filename)
{
	/* If argument is pointer value, check this if it is out of range. */
	check_address((void*)open_filename);

	lock_acquire(&filesys_lock); //lock for atomic file operation.
	struct file *open_file = filesys_open(open_filename); //Get file object
	if(!open_file)
	{
		lock_release(&filesys_lock); //Unlock for atomic file operation.
		return -1;
	}

	int open_file_fd = process_add_file(open_file);
	lock_release(&filesys_lock); //Unlock for atomic file operation.

	return open_file_fd;
}

/* Get filesize of correspond file descriptor */
int
filesize(int fd)
{
	lock_acquire(&filesys_lock); //lock for atomic file operation.
	struct file *target_file = process_get_file(fd); //Get file object
	if(!target_file)
	{
		lock_release(&filesys_lock); //Unlock for atomic file operation.
		return -1;
	}

	int file_size = file_length(target_file);
	lock_release(&filesys_lock); //Unlock for atomic file operation.

	return file_size;
}

/* Get data from input buffer. */
int
read(int fd, char *buffer, unsigned int size)
{
	/* If argument is pointer value, check this if it is out of range. */
	check_address((void*)buffer);
	lock_acquire(&filesys_lock); //lock for atomic file operation.

	if(fd == 0) //STDIN
	{
		unsigned int i;
		for(i = 0; i < size; i++)
		{
			buffer[i] = input_getc();
		}
		lock_release(&filesys_lock); //Unlock for atomic file operation.

		return size;
	}

	
	struct file *read_file = process_get_file(fd); //Get file object
	if(!read_file)
	{
		lock_release(&filesys_lock); //Unlock for atomic file operation.
		return -1;
	}

	int read_size = file_read(read_file, buffer, size);
	lock_release(&filesys_lock); //Unlock for atomic file operation.

	return read_size;
}

/* Put data into output buffer. */
int
write(int fd, char *buffer, unsigned int size)
{
	/* If argument is pointer value, check this if it is out of range. */
	check_address((void*)buffer);

	lock_acquire(&filesys_lock); //lock for atomic file operation.
	
	if(fd == 1) //STDOUT
	{
		putbuf(buffer, size);
		lock_release(&filesys_lock); //Unlock for atomic file operation.

		return size;
	}

	struct file *write_file = process_get_file(fd); //Get file object
	if(!write_file)
	{
		lock_release(&filesys_lock); //Unlock for atomic file operation.
		return -1;
	}

	int write_size = file_write(write_file, buffer, size);
	lock_release(&filesys_lock); //Unlock for atomic file operation.

	return write_size;
}

/* Move offset of file */
void
seek(int fd, unsigned int position)
{
	lock_acquire(&filesys_lock); //lock for atomic file operation.

	struct file *seek_file = process_get_file(fd);
	if(!seek_file)
	{
		lock_release(&filesys_lock); //Unlock for atomic file operation.
		return;
	}

	file_seek(seek_file, (off_t)position);
	lock_release(&filesys_lock); //Unlock for atomic file operation.
}

/* Get current offset of file. */
unsigned int
tell(int fd)
{
	lock_acquire(&filesys_lock); //lock for atomic file operation.

	struct file *tell_file = process_get_file(fd); //Get file object
	if(!tell_file)
	{
		lock_release(&filesys_lock); //Unlock for atomic file operation.
		return -1;
	}

	off_t offset = file_tell(tell_file);
	lock_release(&filesys_lock); //Unlock for atomic file operation.

	return offset;
}

/* Close file */
void
close(int fd)
{
	lock_acquire(&filesys_lock); //lock for atomic file operation.
	process_close_file(fd);
	lock_release(&filesys_lock); //Unlock for atomic file operation.
}

