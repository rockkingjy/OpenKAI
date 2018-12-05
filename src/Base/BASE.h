/*
 * BASE.h
 *
 *  Created on: Sep 15, 2016
 *      Author: Kai Yan
 */

#ifndef OpenKAI_src_Base_BASE_H_
#define OpenKAI_src_Base_BASE_H_

#include "common.h"
#include "../Vision/Frame.h"

using namespace std;

namespace kai
{

class BASE
{
public:
	BASE();
	virtual ~BASE();

	virtual bool init(void* pKiss);
	virtual bool start(void);
	virtual bool draw(void);
	virtual bool console(int& iY);
	virtual int check(void);
	virtual int serialize(uint8_t* pB, int nB);
	virtual int deSerialize(uint8_t* pB, int nB);

	string* getName(void);
	string* getClass(void);

public:
	void*	m_pKiss;
	void*	m_pWindow;
	bool	m_bLog;
	bool	m_bDraw;

	string	m_consoleMsg;
	int8_t m_consoleMsgLevel;
};

}

#endif
