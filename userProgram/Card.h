// Copyright (c) 2022 DisplayLink (UK) Ltd.
#ifndef CARD_H
#define CARD_H

#include "Buffer.h"
#include <list>
#include <memory>
#include <stdio.h> /* printf */
#include <assert.h> /* assert */
#include <iostream>


#include "CImg.h"

#define DEBUG 1
class Card {
	evdi_handle evdiHandle;
	evdi_event_context eventContext;
	evdi_mode mode;

	std::list<std::shared_ptr<Buffer> > buffers;
	std::shared_ptr<Buffer> buffer_requested;

	void setMode(struct evdi_mode mode);
	void makeBuffers(int count);
	void clearBuffers();

	void grab_pixels();

	friend void default_update_ready_handler(int buffer_to_be_updated,
						 void *user_data);
	friend void card_C_mode_handler(struct evdi_mode mode, void *user_data);

    public:
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

	// FOR debug only
#ifdef DEBUG
	// cimg_library::CImg <unsigned char> img;
	// cimg_library::CImgDisplay main_window;
	int frame = 0;
#endif
};

#endif
