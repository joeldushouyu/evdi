// Copyright (c) 2022 DisplayLink (UK) Ltd.
#include "../library/evdi_lib.h"
// #include <pybind11/pybind11.h>
#include "Card.h"
#include "Buffer.h"
#include <cstring>
#include <iostream>
#include "CImg.h"
#include <stdexcept>

// namespace py = pybind11;

static 	unsigned int RGBTo16Bit(unsigned char R, unsigned G, unsigned B){
	//RGB 565
    R = R>>3;
    G =G >> 2;
    B  = B>>3;
    // now, try to combine
    unsigned int returnResult;
    return returnResult =  (R << 11 & 0xF800) | (G << 5 & 0x7E0) | (B & 0x1F);

}



void default_update_ready_handler(int buffer_to_be_updated, void *user_data)
{
	Card *card_ptr = reinterpret_cast<Card *>(user_data);
	assert(buffer_to_be_updated == card_ptr->buffer_requested->buffer.id);
	card_ptr->grab_pixels();
}

void card_C_mode_handler(struct evdi_mode mode, void *user_data)
{
	// py::module logging = py::module::import("logging");
	// logging.attr("info")("Got mode_changed signal.");
	Card *card = reinterpret_cast<Card *>(user_data);
	//printf("width %d height %d bits_per_pixel %d", mode.width, mode.height, mode.bits_per_pixel);
	// std::cout << " width " << mode.width << "height " << mode.height
	// 	  << "bpp " << mode.bits_per_pixel << "refresh rate "
	// 	  << mode.refresh_rate << std::endl;
	assert(card);

	card->setMode(mode);
	card->makeBuffers(IMAGE_BUFFER_SIZE);//TODO: change later?

	// if (card->m_modeHandler != nullptr) {
	// 	card->m_modeHandler(mode);
	// }

	card->request_update();
}

void Card::setMode(struct evdi_mode mode)
{
	this->mode = mode;
}

void Card::makeBuffers(int count)
{
	clearBuffers();
	for (int i = 0; i < count; i++) {
		buffers.emplace_back(new Buffer(mode, evdiHandle));
		std::shared_ptr<Buffer> currentBuffer = buffers.at(i);
		// fill up the bulk transfer
		// libusb_fill_bulk_transfer(currentBuffer->transfer,
		// 	this->handle, ENDPOINT_OUT,  (unsigned char *) currentBuffer->buffer.buffer,
		// 	currentBuffer->bufferSizeInByte, CypressBulkCallback,

		// 	this, 1000
		// 	//TODO:

		// );

	}
}

void Card::clearBuffers()
{
	buffer_requested.reset();
	buffers.clear();
	usb_bulk_buffer_deque.clear();
}

void dpms_handler(int dpms_mode, void * /*user_data*/)
{
	// py::module logging = py::module::import("logging");
	// logging.attr("info")("Got dpms signal." + std::to_string(dpms_mode));
}
Card::Card()
	: evdiHandle(evdi_open_attached_to(NULL))
{
	memset(&eventContext, 0, sizeof(eventContext));

	//m_modeHandler = nullptr;
	// acquire_framebuffer_cb = nullptr;

	eventContext.mode_changed_handler = &card_C_mode_handler;
	eventContext.update_ready_handler = &default_update_ready_handler;
	eventContext.dpms_handler = dpms_handler;
	eventContext.user_data = this;

	memset(&mode, 0, sizeof(mode));

	// #ifdef DEBUG
	// 	img = cimg_library::CImg<>(mode.width, mode.height, 1, 3);
	// 	// main_window = cimg_library::CImgDisplay(img, "Random Data", 0);
	// 	frame = 0;
	// #endif
		}
Card::Card(int device)
	: evdiHandle(evdi_open(device))
{
	// if (evdiHandle == nullptr) {
	// 	throw py::value_error("Card /dev/dri/card" +
	// 			      std::to_string(device) +
	// 			      "does not exists!");
	// }
	
	memset(&eventContext, 0, sizeof(eventContext));

	//m_modeHandler = nullptr;
	// acquire_framebuffer_cb = nullptr;

	eventContext.mode_changed_handler = &card_C_mode_handler;
	eventContext.update_ready_handler = &default_update_ready_handler;
	eventContext.dpms_handler = dpms_handler;
	eventContext.user_data = this;

	memset(&mode, 0, sizeof(mode));
}

Card::~Card()
{
	close();
}

void Card::close()
{
	std::cout << "Closing" << std::endl;
	if (evdiHandle != nullptr) {
		clearBuffers();
		evdi_close(evdiHandle);
	}
	evdiHandle = nullptr;
}



int  Card:: claimCypressUSBDevice(){
	int r;


	r = libusb_init(& (this->usb_context));
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
        libusb_exit(this->usb_context);
        return 1;
    }

    // Claim the interface
    r = libusb_claim_interface(handle, 0);
    if (r < 0)
    {
        fprintf(stderr, "Error claiming interface: %s\n", libusb_error_name(r));
        libusb_close(handle);
        libusb_exit( this->usb_context);
        return 1;
    }
	return 0;
}

void Card::connect(const char *edid, const unsigned int edid_length,
		   const uint32_t pixel_area_limit,
		   const uint32_t pixel_per_second_limit)
{
	evdi_connect(evdiHandle, reinterpret_cast<const unsigned char *>(edid),
		     edid_length, pixel_area_limit);
	// evdi_connect2(evdiHandle, reinterpret_cast<const unsigned char *>(edid),
	// 	      edid_length, pixel_area_limit, pixel_per_second_limit);
}

void Card::disconnect()
{
	evdi_disconnect(evdiHandle);
}

struct evdi_mode Card::getMode() const
{
	return mode;
}

void Card::handle_events(int waiting_time)
{

	struct timeval tv;
	FD_ZERO(&rfds);
	int fd = evdi_get_event_ready(evdiHandle);
	FD_SET(fd, &rfds);
	tv.tv_sec = 0;
	tv.tv_usec = waiting_time * 1000;


	request_update();
	//std::cout <<"The return result is " << res <<" "<< std::endl;

	if ( select(fd + 1, &rfds, NULL, NULL, &tv)) {
		evdi_handle_events(evdiHandle, &eventContext);
	}
}

int Card::request_update()
{
	if (buffer_requested) {
		return -3;
	}

	// tru to get the lock
	// std::cout << "waiting for the lock" << std::endl;
	this->usb_bulk_buffer_deque_mutex.lock();
	// std::cout << "acquire for the lock" << std::endl;

	// grab the first in deque 

	for (auto &i : buffers) {
		//if (i.use_count() == 1 && i->inUSBQueue == false) {
		if ( i->inUSBQueue == false) {
			buffer_requested = i;
			break;
		}
	}
	this->usb_bulk_buffer_deque_mutex.unlock();

	// std::cout<< "release for the lock " << std::endl;
	if (!buffer_requested) {
		// std::cout <<"cannot find!" <<std::endl;
		if(buffers.size() == 0){
			return -2;
		}else {
			// std::cout <<"Buffer full" <<std::endl;
			// // repalce with the top
			this->usb_bulk_buffer_deque_mutex.lock();
			std::shared_ptr<Buffer> firstInQueue = usb_bulk_buffer_deque.front();
			usb_bulk_buffer_deque.pop_front();
			buffer_requested = firstInQueue;
			this->usb_bulk_buffer_deque_mutex.unlock();


			// return -1;
		}
	}



	// std::cout <<"start waiting for update" << std::endl;
	bool update_ready =
		evdi_request_update(evdiHandle, buffer_requested->buffer.id);
	// std::cout <<"after waiting for update" << std::endl;
	if (update_ready) {
		grab_pixels();
		return 0;
	}else{
		return -5;
	}

}

void Card::grab_pixels()
{
	if (!buffer_requested) {
		return;
	}

	evdi_grab_pixels(evdiHandle, buffer_requested->buffer.rects,
			 &buffer_requested->buffer.rect_count);
	
	// std::cout << " new frame:   width " << mode.width << "height "
	// 	  << mode.height << "bpp " << mode.bits_per_pixel
	// 	  << "refresh rate " << mode.refresh_rate << std::endl;

	// // now, process the data 
	// // only for 16 bit mode to compress the data
	unsigned int i,k;
	for (i = 0; i < mode.height; i++) {
		for (k = 0; k < mode.width; k++) {
			unsigned int offset = (i * mode.width + k) *
					      (mode.bits_per_pixel / 8);

			struct evdi_buffer b = buffer_requested->buffer;
			// unsigned char byte1 =
			// 	((char *)b.buffer)[offset];
			// unsigned char byte2 =
			// 	((char *)b.buffer)[offset + 1];
			// unsigned char byte3 =
			// 	((char *)b.buffer)[offset + 2];
			//std::cout << "offset " << offset << std::endl;
			if (mode.bits_per_pixel / 8 == 4) {
				const unsigned char color[4] = {
					((unsigned char *)b.buffer)[offset],
					((unsigned char *)b.buffer)[offset + 1],
					((unsigned char *)b.buffer)[offset + 2],
					((unsigned char *)b.buffer)[offset + 3],
				};
				// img.draw_point(k, i, color);
				//TODO: this is monitor specific
				// int CompressImage = RGBTo16Bit(color[2],color[1] ,color[0]);
				int CompressImage = RGBTo16Bit(((unsigned char *)b.buffer)[offset + 2],((unsigned char *)b.buffer)[offset + 1],((unsigned char *)b.buffer)[offset]);
				// ((unsigned char *)b.buffer)[offset+0]  = 0x0;
				// ((unsigned char *)b.buffer)[offset+1] = 0x0; 
				// int CompressImage;
				// if(i < mode.height/2){
				// 	CompressImage = RGBTo16Bit(255,0,0);
				// }else{
				// 	CompressImage = RGBTo16Bit(0,0,255);
				// }
				((unsigned char *)b.buffer)[offset+2] = (CompressImage >> 8) & 0xFF;
				((unsigned char *)b.buffer)[offset+3] = (CompressImage ) & 0xFF;


			}
		}
	}
	// add to queue
	// if(buffer_requested->firsTimeInQueue){
	// 	buffer_requested->firsTimeInQueue = false;
	// 	// send 
	// 	buffer_requested->inUSBQueue = true;
	// 	libusb_submit_transfer(buffer_requested->transfer);
	// }else{
	// 	// lets add to the buffer
	this->usb_bulk_buffer_deque_mutex.lock();
	// 	std::cout <<"adding to queue" << std::endl;
		buffer_requested->inUSBQueue = true;
		this->usb_bulk_buffer_deque.push_back(buffer_requested); // should also increase the count
	this->usb_bulk_buffer_deque_mutex.unlock();
	// }
#ifdef DEBUG
	//std::cout << "draw image" << std::endl;
	frame++;
	frame = frame % 15;
	int i = 0;
	int k = 0;
	cimg_library::CImg<unsigned char> img(mode.width, mode.height, 1,
					      mode.bits_per_pixel / 8);
	for (i = 0; i < mode.height; i++) {
		for (k = 0; k < mode.width; k++) {
			unsigned int offset = (i * mode.width + k) *
					      (mode.bits_per_pixel / 8);

			struct evdi_buffer b = buffer_requested->buffer;
			// unsigned char byte1 =
			// 	((char *)b.buffer)[offset];
			// unsigned char byte2 =
			// 	((char *)b.buffer)[offset + 1];
			// unsigned char byte3 =
			// 	((char *)b.buffer)[offset + 2];
			//std::cout << "offset " << offset << std::endl;
			if (mode.bits_per_pixel / 8 == 4) {
				const unsigned char color[4] = {
					((unsigned char *)b.buffer)[offset], // r?
					((unsigned char *)b.buffer)[offset + 1], // green
					0,//((unsigned char *)b.buffer)[offset + 2],// b
					0//((unsigned char *)b.buffer)[offset + 3],//
				};
				unsigned char fixedColor[4] = {color[3], color[2], color[1], color[0]};
				img.draw_point(k, i, color);
			}
		}
		// if (i == 1) {
		// 	std::cout << "i " << std::to_string(i) << " k "
		// 		  << std::to_string(k) << std::endl;
		// 	std::cout << "offset " << (i * mode.width + k) * 4
		// 		  << std::endl;
		// 	std::string name =
		// 		"file" + std::to_string(frame) + ".png";
		// 	img.save_png(name.c_str());
		// 	throw std::invalid_argument("received negative value");
		// }

		// std::cout << "line  " << i << std::endl;
	}

	std::cout << "i " << std::to_string(i) << " k " << std::to_string(k)
		  << std::endl;
	std::string name = "file" + std::to_string(frame) + ".png";
	img.save_png(name.c_str());

#endif
	//TODO:
	// if (acquire_framebuffer_cb)
	// 	acquire_framebuffer_cb(std::move(buffer_requested));
	buffer_requested = nullptr;

	request_update();
}
