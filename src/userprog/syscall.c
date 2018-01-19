#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "exception.h"
#include "process.h"
#include "pagedir.h"
#include "threads/vaddr.h"
#include "../filesys/filesys.h"
#include "../filesys/file.h"
#include "../devices/shutdown.h"
#include "filesys/off_t.h"

static void syscall_handler (struct intr_frame *);
bool is_valid_ (void *esp);
void halt_ (struct intr_frame *f UNUSED);
void exit_ (struct intr_frame *f);
void exec_ (struct intr_frame *f);
void wait_ (struct intr_frame *f);
void create_ (struct intr_frame *f);
void remove_ (struct intr_frame *f);
void open_ (struct intr_frame *f);
void get_file_size_ (struct intr_frame *f);
void read_ (struct intr_frame *f);
void write_ (struct intr_frame *f);
void seek_ (struct intr_frame *f);
void tell_ (struct intr_frame *f);
void close_ (struct intr_frame *f);

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f)
{

	int num = *(int *)f->esp;
  // printf ("system call! - %d\n", num);

	switch (num) {
		case SYS_HALT:
			halt_ (f);			              /* Halt the operating system. */
      break;
		case SYS_EXIT:
    	exit_ (f);                    /* Terminate this process. */
      break;
		case SYS_EXEC:
    	exec_ (f);                    /* Start another process. */
      break;
		case SYS_WAIT:
    	wait_ (f);                    /* Wait for a child process to die. */
      break;
		case SYS_CREATE:
    	create_ (f);                  /* Create a file. */
      break;
		case SYS_REMOVE:
    	remove_ (f);                  /* Delete a file. */
      break;
		case SYS_OPEN:
    	open_ (f);                    /* Open a file. */
      break;
		case SYS_FILESIZE:
    	get_file_size_ (f);           /* Obtain a file's size. */
      break;
		case SYS_READ:
    	read_ (f);                    /* Read from a file. */
      break;
		case SYS_WRITE:
    	write_ (f);                   /* Write to a file. */
      break;
		case SYS_SEEK:
    	seek_ (f);                    /* Change position in a file. */
      break;
		case SYS_TELL:
    	tell_ (f);                    /* Report current position in a file. */
      break;
		case SYS_CLOSE:
    	close_ (f);
      break;
		default :
			page_fault(f);
		}
}

//functions used in execution

void halt_sys_call ()
{
	shutdown_power_off ();
}

void exit_sys_call (int status)
{
	// thread_current()->exit_status = status;
  #ifdef USERPROG
    thread_current()->parent->child_exit_status = status;
    printf ("%s: exit(%d)\n", thread_current ()->exec_name, status);
  #endif
  thread_exit();
}

tid_t exec_sys_call (const char *file_name)
{
	return process_execute (file_name);
}

int wait_sys_call (tid_t child_tid)
{
	return process_wait (child_tid);
}

struct file *get_file (int fd)
{
	 struct list_elem *e;
 	 for (e = list_begin (&thread_current()->file_list); e != list_end (&thread_current()->file_list);
        e = list_next (e))
   {
     struct file *f = list_entry (e, struct file, file_elem);
     if(f->fd == fd) {
	 		return f;
	 	}
	 }
	return NULL;
}

bool create_sys_call (const char *name, unsigned initial_size)
{
	return filesys_create (name, initial_size);
}

bool remove_sys_call (const char *name)
{
	return filesys_remove (name);
}

int open_sys_call (const char *name)
{
	struct file *f = filesys_open (name);
	if(f == NULL)
		return -1;

	return f->fd;
}

int get_file_size_sys_call (int fd)
{
	struct file *f = get_file(fd);
	if(f != NULL)
		return file_length (f);

	return -1;
}

int read_sys_call (int fd, void *buffer, unsigned size)
{
	int val = -1;
	if(fd == 0) {
		return input_getc();
	} else {
			struct file *f = get_file(fd);
			if(f != NULL)
				val = (int) file_read (f, buffer, size);
	}
	return val;
}

int write_sys_call (int fd, const void *buffer, unsigned size)
{
	int val = -1;

	if(fd == 1) {
		putbuf ((char*)buffer, (size_t)size);
		return (int)size;
	} else {
			struct file *f = get_file(fd);
			if(f != NULL)
				val = (int) file_write (f, buffer, size);
  }
	return val;
}

void seek_sys_call (int fd, unsigned pos)
{
	struct file *f = get_file(fd);
	if(f != NULL)
	 file_seek (f, pos);
}

unsigned tell_sys_call (int fd)
{
	struct file *f = get_file(fd);
	if(f != NULL)
		return file_tell (f);
	return -1;
}

void close_sys_call (int fd)
{
	struct file *f = get_file(fd);
	if(f != NULL) {
		file_close (f);
		list_remove (&f->file_elem);
	}
}

/* Checks the validity of the stack pointer. */
bool is_valid_ (void *esp)
{
	if(esp == NULL || pagedir_get_page (thread_current()->pagedir, esp) == NULL){
		return false;
	}
	if(!is_user_vaddr(esp)) {
		return false;
	}
	return true;
}

void halt_ (struct intr_frame *f UNUSED)
{
	halt_sys_call();
}

void exit_ (struct intr_frame *f)
{
	void *pointer = f->esp;
	//incrementing pointer to skip system call number
	pointer += sizeof(int*);

	if(!is_valid_(pointer))
		page_fault(f);

 	int status = *(int *)pointer;
	exit_sys_call(status);

}

void exec_ (struct intr_frame *f)
{
	void *pointer = f->esp;
	//incrementing pointer to skip system call number
	pointer += sizeof(int*);

	if(!is_valid_(pointer))
		page_fault(f);

	char *file_name = *(char **)pointer;
	f->eax = exec_sys_call(file_name);

}

void wait_ (struct intr_frame *f)
{
	void *pointer = f->esp;
	//incrementing pointer to skip system call number
	pointer += sizeof(int*);

	if(!is_valid_(pointer))
		page_fault(f);

	tid_t child_tid = *(tid_t *)pointer;
	f->eax = wait_sys_call(child_tid);


}

void create_ (struct intr_frame *f)
{
	void *pointer = f->esp;
	//incrementing pointer to skip system call number
	pointer += sizeof(int*);

	if(!is_valid_(pointer))
		page_fault(f);

	char *file_name = *(char **)pointer;
	pointer += sizeof(char**);

	if(!is_valid_(pointer))
		page_fault(f);

	unsigned initial_size = *(unsigned *)pointer;
	f->eax = create_sys_call(file_name, initial_size);


}

void remove_ (struct intr_frame *f)
{
	void *pointer = f->esp;
	//incrementing pointer to skip system call number
	pointer += sizeof(int*);

	if(!is_valid_(pointer))
		page_fault(f);

	char *file_name = *(char **)pointer;
	f->eax = remove_sys_call(file_name);

}

void open_ (struct intr_frame *f)
{
	void *pointer = f->esp;
	//incrementing pointer to skip system call number
	pointer += sizeof(int*);

	if(!is_valid_(pointer))
		page_fault(f);

	char *file_name = *(char **)pointer;
	f->eax = open_sys_call(file_name);

}

void get_file_size_ (struct intr_frame *f)
{
	void *pointer = f->esp;
	//incrementing pointer to skip system call number
	pointer += sizeof(int*);

	if(!is_valid_(pointer))
		page_fault(f);

	int fd = *(int *)pointer;
	f->eax = get_file_size_sys_call(fd);

}

void read_ (struct intr_frame *f)
{
	void *pointer = f->esp;
	//incrementing pointer to skip system call number
	pointer += sizeof(int*);

	if(!is_valid_(pointer))
		page_fault(f);

	int fd = *(int *)pointer;
	pointer += sizeof(int*);

	if(!is_valid_(pointer))
		page_fault(f);

	char *buffer = *(char**)pointer;
	pointer += sizeof(char**);

	if(!is_valid_(pointer))
		page_fault(f);

	unsigned size = *(unsigned *)pointer;
	f->eax = read_sys_call(fd, buffer, size);
}

void write_ (struct intr_frame *f)
{
	void *pointer = f->esp;
	//incrementing pointer to skip system call number
	pointer += sizeof(int*);

	if(!is_valid_(pointer))
		page_fault(f);

	int fd = *(int *)pointer;
  // printf("FD: %d\n", fd);
	pointer += sizeof(int*);

	if(!is_valid_(pointer))
		page_fault(f);

	char *buffer = *(char**)pointer;
  // printf("Buffer: %s\n", buffer);
	pointer += sizeof(char**);

	if(!is_valid_(pointer))
		page_fault(f);

	unsigned size = *(unsigned *)pointer;
	f->eax = write_sys_call(fd, buffer, size);
  // printf("eax: %d", f->eax);

}

void seek_ (struct intr_frame *f)
{
	void *pointer = f->esp;
	//incrementing pointer to skip system call number
	pointer += sizeof(int*);

	if(!is_valid_(pointer))
		page_fault(f);

	int fd = *(int *)pointer;
	pointer += sizeof(int*);

	if(!is_valid_(pointer))
		page_fault(f);

	unsigned pos = *(unsigned *)pointer;
	seek_sys_call(fd, pos);

}

void tell_ (struct intr_frame *f)
{
	void *pointer = f->esp;
	//incrementing pointer to skip system call number
	pointer += sizeof(int*);

	if(!is_valid_(pointer))
		page_fault(f);

	int fd = *(int *)pointer;
	f->eax = tell_sys_call(fd);

}

void close_ (struct intr_frame *f)
{
	void *pointer = f->esp;
	//incrementing pointer to skip system call number
	pointer += sizeof(int*);
	if(!is_valid_(pointer))
		page_fault(f);

	int fd = *(int *)pointer;
	close_sys_call(fd);

}
