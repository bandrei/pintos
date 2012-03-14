#ifndef MMAP_H
#define MMAP_H

#include <stdint.h>


struct mmap_entry
{
	/* when used by the lazy loading of the executable
	 * this will represent the offset in the elf file
	 * of the code that needs to be read (see load_segment
	 * for more details on the workings). We don't need
	 * to store a pointer to a file in this case because
	 * the thread already has this information.
	 * In all other cases this will be uses as it is defined
	 * (as a pointer to a file)
	 */
	struct file *file_ptr;

	/* To be used for keeping record of what the page offset
	 * is relative to the start of the file.
	 *
	 * In the special case of the exe (elf) mappings it will
	 * be used to store the number of bytes to be read
	 */
	uint32_t page_offset;
};

#endif
