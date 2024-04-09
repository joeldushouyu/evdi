#include <fstream>
#include <iostream>
#include <string>
#include <libusb-1.0/libusb.h>
#include "Card.h"
#include "Buffer.h"
#include <thread>
#include <cstdlib>
//TODO: load from file in the future
#define _FullHDAreaLimit 1920 * 1080
#define _4KAreaLimit 3840 * 2160
#define _60Hz 60

// another test
#define _30HZ 30

// #define DATA_SIZE 4 * (4096) //   4*4096 // 16384 *100  // 1024*4*4*2// 1024*4 *4   *2// Define the size of the data buffer

// #define TOTAL_DATA_ARRAY (480*640)/4096

#define delay_time_tolerance_ms  (1/_60Hz)*1000
libusb_device_handle *handle = NULL;

struct libusb_context *usb_context;

int initalize_usb(){
	    int r;
    // The context

    // Initialize libusb
    r = libusb_init(&usb_context);
    if (r < 0)
    {
        fprintf(stderr, "Error initializing libusb: %s\n", libusb_error_name(r));
        return 1;
    }

    // Open the device
    handle = libusb_open_device_with_vid_pid(NULL, VENDOR_ID, PRODUCT_ID);
    if (!handle)
    {
        fprintf(stderr, "Failed to open device\n");
        libusb_exit(NULL);
        return 1;
    }

    // Claim the interface
    r = libusb_claim_interface(handle, 0);
    if (r < 0)
    {
        fprintf(stderr, "Error claiming interface: %s\n", libusb_error_name(r));
        libusb_close(handle);
        libusb_exit(usb_context);
        return 1;
    }
	return 0;
}
 void read_callback(struct libusb_transfer *transfer)
{

    int res;
    if (transfer->status == LIBUSB_TRANSFER_COMPLETED)
    {
        // success
    }
    else
    {
        printf("ERROR: %s\n", libusb_error_name(transfer->status));
    }

    // resubmit the transfer object back to queue
    res = libusb_submit_transfer(transfer);
	
	printf("End~~~~\n");
 //throw std::invalid_argument( "received negative value" );
}



static struct libusb_transfer *create_transfer(libusb_device_handle *handle, size_t length)
{
    struct libusb_transfer *transfer;
    unsigned char *buf;

    // setup the transfer object
    buf = (unsigned char *)malloc(length);
    transfer = libusb_alloc_transfer(0); // Zero since is a bulk transfer
    // TODO: user data pass back to callback function

    //TODO: testing only for data initialization

    for(unsigned int i = 0; i < length; i++){
        buf[i] = 0xff;
    }


    libusb_fill_bulk_transfer(transfer, handle, ENDPOINT_OUT, buf, length, read_callback, NULL, 5000);
    return transfer;
}



 void CypressBulkCallback(struct libusb_transfer *transfer)
{
	// std::cout << "At the callback" << std::endl;
	// the private data of transfer should contan Card pointer
	Card *c = static_cast<Card *>(transfer->user_data);
	// acquire the queue lock
	c->usb_bulk_buffer_deque_mutex.lock();

	if (c->usb_bulk_buffer_deque.empty()) {
		// empty!!!
		std::cout << "ERROR: empty queue" << std::endl;
		// try recovery by continue submit the current

		libusb_submit_transfer(transfer);
	} else {
		// std::cout << "at relase case" << std::endl;
		//mark buffer as free
		// it should not in deque, but in the buffer
		bool hasFound = false;
		for (int i = 0; i < IMAGE_BUFFER_SIZE; i++) {
			std::shared_ptr<Buffer> p = c->buffers.at(i);
			if (p->buffer.buffer == transfer->buffer) {
				p->inUSBQueue = false;
				hasFound = true;
				// std::cout << "Trying to release" << std::endl;
				break;
			}
		}
		if (!hasFound) {
			std::cout << "***** DID not find" << std::endl;
		}

		// use next head in queue
		Buffer *ptr = c->usb_bulk_buffer_deque.front().get();

		c->usb_bulk_buffer_deque.pop_front();

		struct libusb_transfer *newTrans = ptr->transfer;
		ptr->inUSBQueue = true;
	

		libusb_submit_transfer(newTrans);
	}

	c->usb_bulk_buffer_deque_mutex.unlock();
}

std::string readEdidInfo(const std::string &fileName)
{
	std::ifstream in(fileName, std::ios_base::binary);
	//https://stackoverflow.com/questions/18398167/how-to-copy-a-txt-file-to-a-char-array-in-c
	std::string contents((std::istreambuf_iterator<char>(in)),
			     std::istreambuf_iterator<char>());
	return contents;
}

void usb_event_thread(Card *testGraphicCard)
{

	initalize_usb();
    // create the transfer
	while(testGraphicCard->buffers.size() != IMAGE_BUFFER_SIZE){
		;
	}
    for (unsigned int i = 0; i <IMAGE_BUFFER_SIZE; i++)
    {
        // struct libusb_transfer *transfer = create_transfer(handle, (640*480*4));

		Buffer* buf = testGraphicCard->buffers.at(i).get();
		buf->transfer = libusb_alloc_transfer(0);
		buf->transfer->buffer = static_cast<unsigned char *> (buf->buffer.buffer);
		libusb_fill_bulk_transfer(buf->transfer, handle, ENDPOINT_OUT,
		buf->transfer->buffer, buf->bufferSizeInByte,  CypressBulkCallback, testGraphicCard,  delay_time_tolerance_ms);

    }
	// start sending

	for(unsigned int i = 0; i < IMAGE_BUFFER_SIZE/4; i++){
		Buffer* buf = testGraphicCard->buffers.at(i).get();
		buf->inUSBQueue = true;
        libusb_submit_transfer(buf->transfer); // start the intitial transfer
	}
	while (true) {
		//TODO: handle usb event here
		int res = libusb_handle_events(usb_context);
	}
}

int main()
{

	Card testGraphicCard;
	// TODO: need to be from the analog of
	///https://github.com/linuxhw/EDID/tree/master/Analog
	std::string edidString = readEdidInfo("640-480-2.bin");
	//testGraphicCard.claimCypressUSBDevice();
	testGraphicCard.connect(edidString.data(), edidString.size(), 640 * 480,
				640 * 480 * 60);


	std::thread usb_thread{usb_event_thread, &testGraphicCard};

    int res;
    while (1)
    {
		testGraphicCard.handle_events(delay_time_tolerance_ms);
		usleep(  (delay_time_tolerance_ms/2) *1000);

    }
    return 0;
}

// int main()
// {
// 	std::cout << "Hello, Unix!" << std::endl;



// 	// test for libusblic




// 	// end test
// 	// Card testGraphicCard;
// 	// // TODO: need to be from the analog of
// 	// ///https://github.com/linuxhw/EDID/tree/master/Analog
// 	// std::string edidString = readEdidInfo("640-480-2.bin");
// 	// // testGraphicCard.claimCypressUSBDevice();
// 	// // testGraphicCard.connect(edidString.data(), edidString.size(), 640 * 480,
// 	// // 			640 * 480 * 60);

// 	// // before that, start all of it first
// 	// // unsigned int counter = 0;
// 	// // for(auto p : testGraphicCard.buffers){
// 	// // 	p->firsTimeInQueue = false;
// 	// // 	p->inUSBQueue = true;
// 	// // 	libusb_submit_transfer(p->transfer);

// 	// // 	counter++;
// 	// // 	if(counter > 60){
// 	// // 		break;
// 	// // 	}
// 	// // }

// 	// // std::thread usb_thread{usb_event_thread, &testGraphicCard};

// 	// // USBREADY=false;
// 	// // while(!USBREADY){
// 	// // 	;
// 	// // }
// 	// // for (int i = 0; i < testGraphicCard.buffers.size() / 2; i++) {
// 	// // 	Buffer *buffer = (testGraphicCard.buffers.at(i).get());

// 	// // 	buffer->transfer = libusb_alloc_transfer(0);
// 	// // 	libusb_fill_bulk_transfer(
// 	// // 		buffer->transfer, testGraphicCard.handle, ENDPOINT_OUT,
// 	// // 		(unsigned char *)buffer->buffer.buffer,
// 	// // 		buffer->bufferSizeInByte, CypressBulkCallback,
// 	// // 		&testGraphicCard, 5000);
// 	// // 	libusb_submit_transfer(buffer->transfer);
// 	// // }


// 	// while (true) {
// 	// 	// possible correct way
// 	// 	//testGraphicCard.handle_events(1000);
// 	// 	//testGraphicCard.handle_events(int((1 / _60Hz) * 1000)); // in unit of ms
// 	// 	//testGraphicCard.handle_events(500);
// 	// 	int res = libusb_handle_events(testGraphicCard.usb_context);
// 	// }

// 	// testGraphicCard.disconnect();
// 	// testGraphicCard.close();
// 	return 0;
// }
