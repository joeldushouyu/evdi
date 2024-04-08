// Copyright (c) 2022 DisplayLink (UK) Ltd.
#include "Buffer.h"
#include "../library/evdi_lib.h"
#include <cstdlib>
#include <cstdio>
#include <iostream>

int Buffer::numerator = 0;

Buffer::Buffer(evdi_mode mode, evdi_handle evdiHandle)
{
	int id = numerator++;
	this->bufferSizeInByte = mode.width * mode.height * (mode.bits_per_pixel / 8);
	this->evdiHandle = evdiHandle;
	int stride = mode.width;
	int pitch_mask = 63;

	stride += pitch_mask;
	stride &= ~pitch_mask;
	stride *= 4;

	
	buffer.id = id;
	buffer.width = mode.width;
	buffer.height = mode.height;
	buffer.stride = stride;
	buffer.rect_count = 16;
	buffer.rects = reinterpret_cast<evdi_rect *>(
		calloc(buffer.rect_count, sizeof(struct evdi_rect)));
	buffer.buffer =
		calloc(mode.width * mode.height, mode.bits_per_pixel / 8);
	std::cout << " Buffer width " << mode.width << "height " << mode.height << "bpp "
		  << mode.bits_per_pixel << "refresh rate " << mode.refresh_rate
		  << std::endl;

	evdi_register_buffer(evdiHandle, buffer);

	this->inUSBQueue = false;// indicate read for usb send queue
	this->firsTimeInQueue = true;
	// initalize the usb_transfer buffer
	// this->transfer = libusb_alloc_transfer(0);

}

Buffer::~Buffer()
{
	evdi_unregister_buffer(evdiHandle, buffer.id);
	free(buffer.buffer);
	free(buffer.rects);
}
