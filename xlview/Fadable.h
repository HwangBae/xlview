#ifndef XL_VIEW_FADABLE_H
#define XL_VIEW_FADABLE_H

template <class T>
class CFadableT {
	int m_opacityFadeOut;
	int m_opacityFadeIn;
	int m_stepLength;
	int m_fadeOutDelay;
	int m_timeInterval;
	int m_opacity;
	bool m_testFadeOut;

	void _DoFadeOut () {
		this->m_opacity = m_opacityFadeOut;
		process();
	}
public:
	CFadableT (int fadeout = 0, int fadein = 50, int step = 20, int fadeoutdelay = 1000, int timeinterval = 150)
		: m_opacityFadeOut(fadeout)
		, m_opacityFadeIn(fadein)
		, m_stepLength(step)
		, m_fadeOutDelay(fadeoutdelay)
		, m_timeInterval(timeinterval)
		, m_testFadeOut(false)
	{
		T *p = (T *)this;
		assert(m_opacityFadeIn > m_opacityFadeOut);
		assert(m_opacityFadeIn <= 100);
		assert(m_opacityFadeOut >= 0);
		assert(m_stepLength > 0);
		assert(m_fadeOutDelay >= 0);
		assert(m_timeInterval > 0);
		p->setOpacity(m_opacityFadeOut);
	}

	void fadeIn () {
		T *p = (T *)this;
		if (p->disable) {
			return;
		}
		m_testFadeOut = false;
		this->m_opacity = m_opacityFadeIn;
		process();
	}

	void fadeOut () {
		T *p = (T *)this;
		if (p->disable) {
			return;
		}
		if (m_fadeOutDelay > 0 && !m_testFadeOut) {
			m_testFadeOut = true;
			p->_SetTimer(m_fadeOutDelay, (xl::uint)this);
			return;
		}
		_DoFadeOut();
	}

	void process () {
		T *p = (T *)this;
		if (p->disable) {
			return;
		}

		if (m_testFadeOut) {
			m_testFadeOut = false;
			if (!p->isCursorIn()) {
				_DoFadeOut();
			}
			return;
		}

		int opacity = p->opacity;
		if (opacity == m_opacity) {
			return;
		} else if (opacity < m_opacity) {
			if ((m_opacity - opacity) % m_stepLength > 0) {
				opacity += ((m_opacity - opacity) % m_stepLength);
			} else {
				opacity += m_stepLength;
			}
			if (opacity > m_opacity) {
				opacity = m_opacity;
			}
		} else {
			if ((opacity - m_opacity) % m_stepLength > 0) {
				opacity -= ((opacity - m_opacity) % m_stepLength);
			} else {
				opacity -= m_stepLength;
			}
			if (opacity < m_opacity) {
				opacity = m_opacity;
			}
		}

		p->setOpacity(opacity);

		if (opacity != m_opacity) {
			p->_SetTimer(m_timeInterval, (xl::uint)this);
		}
	}

};

#endif
