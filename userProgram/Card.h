// Copyright (c) 2022 DisplayLink (UK) Ltd.
#ifndef CARD_H
#define CARD_H

#include "Buffer.h"
#include <list>
#include <memory>
#include <stdio.h> /* printf */
#include <assert.h> /* assert */
#include <iostream>
#include <libusb-1.0/libusb.h>
#include <deque>
#include "CImg.h"
#include <mutex>
#include <vector>


//USB device specific
#define VENDOR_ID 0x04b4    
#define PRODUCT_ID 0x00f1    
#define ENDPOINT_OUT 0x01
#define  IMAGE_BUFFER_SIZE 60
//#define DEBUG 0
class Card {
	evdi_handle evdiHandle;
	evdi_event_context eventContext;
	evdi_mode mode;


	std::shared_ptr<Buffer> buffer_requested;

	void setMode(struct evdi_mode mode);
	void makeBuffers(int count);
	void clearBuffers();

	void grab_pixels();

	friend void default_update_ready_handler(int buffer_to_be_updated,
						 void *user_data);
	friend void card_C_mode_handler(struct evdi_mode mode, void *user_data);

    public:
	std::vector<std::shared_ptr<Buffer> > buffers;
	// std::function<void(struct evdi_mode)> m_modeHandler;
	// std::function<void(std::shared_ptr<Buffer> buffer)>
	// 	acquire_framebuffer_cb;
	explicit Card();
	explicit Card(int device);
	~Card();
	void close();
	void connect(const char *edid, const unsigned int edid_length,
		     const uint32_t pixel_area_limit,
		     const uint32_t pixel_per_second_limit);
	void disconnect();

	struct evdi_mode getMode() const;
	void request_update();
	void handle_events(int waiting_time);


	// for USB stuff
	libusb_device_handle *handle=NULL;
	struct libusb_context *usb_context = NULL;

	int  claimCypressUSBDevice();

	
	// queue of Buffer pointers
	std::deque<std::shared_ptr<Buffer> > usb_bulk_buffer_deque;
	std::mutex usb_bulk_buffer_deque_mutex;

	// FOR debug only
#ifdef DEBUG
	// cimg_library::CImg <unsigned char> img;
	// cimg_library::CImgDisplay main_window;
	int frame = 0;
#endif
};

#endif
