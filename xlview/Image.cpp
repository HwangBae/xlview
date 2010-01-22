#include <assert.h>
#include "Image.h"

//////////////////////////////////////////////////////////////////////////
// CImage::BitmapAndDelay
CImage::BitmapAndDelay::BitmapAndDelay () {
	bitmap = NULL;
	delay = 0;
}

CImage::BitmapAndDelay::~BitmapAndDelay () {
	if (bitmap) {
		::DeleteObject(bitmap);
		bitmap = NULL;
	}
	delay = 0;
}


//////////////////////////////////////////////////////////////////////////
// CImage
CImage::CImage () {

}

CImage::~CImage () {

}

xl::uint CImage::getImageCount () {
	return m_bads.size();
}

xl::uint CImage::getImageDelay (xl::uint index) {
	assert(index < getImageCount());
	return m_bads[index]->delay;
}

HBITMAP CImage::getImage (xl::uint index) {
	assert(index < getImageCount());
	return m_bads[index]->bitmap;
}

void CImage::insertImage (HBITMAP bitmap, xl::uint delay) {
	assert(bitmap != NULL);

	_BADPtr bad(new _BAD());
	bad->bitmap = bitmap;
	bad->delay = delay;
	m_bads.push_back(bad);
}