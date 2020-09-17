//  This is an altered source version of TPCircularBuffer available at:
//
//  https://github.com/tokyovigilante/TPCircularBuffer
//
//  The original version is available and distributed as follows:
//
//---------------------------------------------------------------------------
//
//  TPCircularBuffer.c
//  Circular/Ring buffer implementation
//
//  https://github.com/michaeltyson/TPCircularBuffer
//
//  Created by Michael Tyson on 10/12/2011.
//
//  Copyright (C) 2012-2013 A Tasty Pixel
//
//  This software is provided 'as-is', without any express or implied
//  warranty.  In no event will the authors be held liable for any damages
//  arising from the use of this software.
//
//  Permission is granted to anyone to use this software for any purpose,
//  including commercial applications, and to alter it and redistribute it
//  freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not
//     claim that you wrote the original software. If you use this software
//     in a product, an acknowledgment in the product documentation would be
//     appreciated but is not required.
//
//  2. Altered source versions must be plainly marked as such, and must not be
//     misrepresented as being the original software.
//
//  3. This notice may not be removed or altered from any source distribution.
//

#include "include/TPCircularBuffer.h"
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

static inline int round_page(int length) {
    int pagesize = getpagesize();
    return length + pagesize - 1 - (length - 1) % pagesize;
}

int memfd_create (const char *name, unsigned int flags) {
    return syscall(__NR_memfd_create, name, flags);
}

bool _TPCircularBufferInit(TPCircularBuffer *buffer, uint32_t length, size_t structSize) {

    assert(length > 0);

    if ( structSize != sizeof(TPCircularBuffer) ) {
        fprintf(stderr, "TPCircularBuffer: Header version mismatch. Check for old versions of TPCircularBuffer in your project\n");
        abort();
    }

    buffer->length = (int32_t)round_page(length);    // We need whole page sizes

    // Create a temporary file to identify the memory block, that we immediatly unlink
    int fd = memfd_create("queue_buffer", 0);
    if ( fd < 0 ) {
        return false;
    }

    // Grow the file, to allow mmap, but don't actually allocate space
    if (ftruncate(fd, buffer->length*2) < 0) {
        close(fd);
        return false;
    }

    // Temporarily allocate twice the length, so we have the contiguous address space to
    // support a second instance of the buffer directly after
    void* bufferAddress = mmap(NULL, buffer->length*2, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if ( bufferAddress == (void*) -1 ) {
        close(fd);
        return false;
    }

    buffer->buffer = bufferAddress;
    buffer->fillCount = 0;
    buffer->head = buffer->tail = 0;

    // Now replace the second half of the allocation with a virtual copy of the first half
    void* virtualAddress = mmap(bufferAddress + buffer->length, buffer->length,
                                PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FIXED, fd, 0);

    // Cleanup, as we don't need the descriptor anymore
    close(fd);

    if ( virtualAddress != bufferAddress+buffer->length ) {
        munmap(buffer->buffer, buffer->length * 2);
        return false;
    }

    return true;
}

void TPCircularBufferCleanup(TPCircularBuffer *buffer) {
    munmap(buffer->buffer, buffer->length * 2);
    memset(buffer, 0, sizeof(TPCircularBuffer));
}

void TPCircularBufferClear(TPCircularBuffer *buffer) {
    uint32_t fillCount;
    if ( TPCircularBufferTail(buffer, &fillCount) ) {
        TPCircularBufferConsume(buffer, fillCount);
    }
}

void  TPCircularBufferSetAtomic(TPCircularBuffer *buffer, bool atomic) {
    buffer->atomic = atomic;
}
