# Disk Addressing System
In this project, we followed a global absolute addressing system.
The addresses generated by the processes correspond to physical addresses in the disk.
To read or write to a specific address, the process reuest the buffer manager to read or write to the specific address.
The buffer manager first checks if the address is in the buffer.
If it is not, it generates block read requests to the disk manager to read the block containing the address.
The disk manager then reads the block from the disk and returns it to the buffer manager.
If the request is for writing, the buffer manager writes the data to the buffer and marks the block as dirty.

# Buffer Management
The buffer manager maintains a buffer pool of fixed-size frames.
Each frame holds a page of data.
The page size, frame size and block size are same.
The information about frame to page mapping is maintained in a global page table.
The buffer manager supports two page replacement policies, Least Recently Used (LRU) and Most Recently Used (MRU).
If no free frames are available for a request, the buffer manager uses the page replacement policy to select a frame to evict.
The evicted frame is written back to the disk if it is dirty.
The buffer manager writes all dirty frames back to the disk before shutting down to ensure data integrity.