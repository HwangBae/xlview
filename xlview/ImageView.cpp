#include "ImageView.h"

CImageView::CImageView(void)
	: xl::ui::CControl(ID_VIEW)
{
	setStyle(_T("px:left;py:top;width:fill;height:fill;"));
	setStyle(_T("background-color:#808080;"));
}

CImageView::~CImageView(void)
{
}
