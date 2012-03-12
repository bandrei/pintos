#ifndef MMAP_H
#define MMAP_H

#include <stdint.h>


struct mmap_entry
{
	/* when used by the lazy loading of the executable
	 * this will represent the offset in the elf file
	 * of the code that needs to be read (see load_segment
	 * for more details on the workings).
	 * In all other cases this will be uses as it defined
	 * (as a pointer to a file)
	 */
	struct file *file_ptr;

	/* To be use primarily with the elf entries for
	 * keeping a record of the number of bytes that need
	 * to be read from the elf entry into a page
	 */
	uint32_t bytes;
};

#endif
