/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#endif
#include <linux/kernel.h>  // For printk
#include "aesd-circular-buffer.h"
#include "stdio.h"
// for debugging
void print_buffer(struct aesd_circular_buffer *buffer) {
    size_t i;
    printf("Buffer contents (in_offs = %d, out_offs = %d, full = %d):\n",
           buffer->in_offs, buffer->out_offs, buffer->full);
    
    for (i = 0; i < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; i++) {
        size_t index = (buffer->out_offs + i) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
        if (i >= (buffer->in_offs - buffer->out_offs + AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED) {
            break; // No more valid entries
        }
        printf("Entry %ld: %s (size: %ld)\n", index, buffer->entry[index].buffptr, buffer->entry[index].size);
    }
}

/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer.
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
            size_t char_offset, size_t *entry_offset_byte_rtn )
{
    /**
    * TODO: implement per description
    */
    size_t current_offset = 0;  // Tracks the cumulative character count
    size_t i;
    struct aesd_buffer_entry *entry;

    // Iterate through the circular buffer entries
    for (i = 0; i < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; i++) {
        size_t entry_size;
        size_t index = (buffer->out_offs + i) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;



        entry = &buffer->entry[index];
        entry_size = entry->size;

        if (char_offset < (current_offset + entry_size)) {
            // The requested character offset falls within this entry
            *entry_offset_byte_rtn = char_offset - current_offset;
            
            // Print the entry found
            printf("Found entry at offset %zu (entry_offset_byte_rtn = %zu):\n", char_offset, *entry_offset_byte_rtn);
            printf("Entry data: %.*s\n", (int)entry->size, entry->buffptr);
            return entry;
        }

        // Update the cumulative character offset tracker
        current_offset += entry_size;
    }
    printf("null\n\r\n\r");
    // If we get here, the requested offset is beyond the total buffer size
    return NULL;
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
void aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    /**
    * TODO: implement per description
    */
    // debug
    // Print the buffer before adding the entry
    printf("Before adding entry:\n");
    print_buffer(buffer);
        // If the buffer is full, we need to advance the out_offs pointer
    if (buffer->full) {
        // Overwriting oldest entry, move out_offs to the next entry
        buffer->out_offs = (buffer->out_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
        buffer->full = 0;
    }

    // Copy the new entry into the buffer at the in_offs position
    buffer->entry[buffer->in_offs] = *add_entry;

    // Move the in_offs pointer to the next position in the circular buffer
    buffer->in_offs = (buffer->in_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;

    // If the buffer was previously not full but now wrapped around, mark it as full
    if (buffer->in_offs == buffer->out_offs) {
        buffer->full = 1;  // Buffer is now full
    }
    // Print the buffer after adding the entry
    printf("After adding entry:\n");
    print_buffer(buffer);
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}
