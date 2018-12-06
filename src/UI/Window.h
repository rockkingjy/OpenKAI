/*
 * Window.h
 *
 *  Created on: Dec 7, 2016
 *      Author: Kai Yan
 */

#ifndef OpenKAI_src_UI_Windows_H_
#define OpenKAI_src_UI_Windows_H_

#include "../Base/common.h"
#include "../Base/BASE.h"
#include "../Vision/Frame.h"

#define TAB_PIX 20
#define LINE_HEIGHT 20

namespace kai
{

class Window: public BASE
{
public:
	Window();
	virtual ~Window();

	bool init(void* pKiss);
	bool link(void);
	bool draw(void);
	Frame* getFrame(void);
	Point* getTextPos(void);

	void addMsg(const string& pMsg);
	void tabNext(void);
	void tabPrev(void);
	void tabReset(void);
	void lineNext(void);
	void lineReset(void);
	double textSize(void);
	Scalar textColor(void);

public:
	bool	m_bWindow;
	bool	m_bFullScreen;
	Frame	m_frame;
	vInt2	m_size;
	vInt2	m_textPos;
	vInt2	m_textStart;
	int		m_pixTab;
	int		m_lineHeight;
	Point	m_tPoint;
	double	m_textSize;
	Scalar	m_textCol;

	string	m_gstOutput;
	VideoWriter m_gst;

	string m_fileRec;
	VideoWriter m_VW;

	Frame	m_F;
	Frame	m_F2;
};

}

#endif
