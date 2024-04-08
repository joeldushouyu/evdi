1. At creation of each buffer, create buffer and libusb_transfer object
2. have a thread safe queue

3. A new thread that is responsible for sending data out by getting the lock
    1. if no more thing in queue, add current event_handle into queue 
    2. if have, fetch, mark current buffer UNUSED
4. a lock need to be held everytime access the queue

5. at a new image
    1. get the lock for queue
    2. mark the buffer be in used

//TODO buffer size? 60 or 120?
