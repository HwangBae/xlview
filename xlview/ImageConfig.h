#ifndef XL_VIEW_IMAGE_CONFIG_H
#define XL_VIEW_IMAGE_CONFIG_H
#include <atltypes.h>

// when zooming, we don't generate image that is smaller than that
static const int MIN_VIEW_WIDTH = 160;
static const int MIN_VIEW_HEIGHT = 120;

inline void CHECK_VIEW_SIZE (CSize &sz) {
	if (sz.cx < MIN_VIEW_WIDTH) {
		sz.cx = MIN_VIEW_WIDTH;
	}
	if (sz.cy < MIN_VIEW_HEIGHT) {
		sz.cy = MIN_VIEW_HEIGHT;
	}
}

// the thumbnail size
static const int THUMBNAIL_WIDTH = 80;
static const int THUMBNAIL_HEIGHT = 60;


#endif
