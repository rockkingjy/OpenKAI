#include "_Canbus.h"

namespace kai
{
_Canbus::_Canbus()
{
	m_pSerialPort = NULL;
	m_nCanData = 0;
}

_Canbus::~_Canbus()
{
	close();
}

bool _Canbus::init(void* pKiss)
{
	IF_F(!this->_ThreadBase::init(pKiss));
	Kiss* pK = (Kiss*)pKiss;

	Kiss** pItr = pK->getChildItr();
	m_nCanData = 0;
	while (pItr[m_nCanData])
	{
		Kiss* pAddr = pItr[m_nCanData];
		IF_F(m_nCanData >= N_CANDATA);

		m_pCanData[m_nCanData].init();
		F_ERROR_F(pAddr->v("addr", (int*)&(m_pCanData[m_nCanData].m_addr)));
		m_nCanData++;
	}

	//link
	string iName;
	iName = "";
	F_ERROR_F(pK->v("_IOBase", &iName));
	m_pSerialPort = (_SerialPort*) (pK->root()->getChildInst(iName));
	NULL_Fl(m_pSerialPort,"_IOBase not found");

	return true;
}

void _Canbus::close()
{
	if (m_pSerialPort)
	{
		m_pSerialPort->close();
		delete m_pSerialPort;
	}

	LOG_I("Closed");
}

bool _Canbus::start(void)
{
	//Start thread
	m_bThreadON = true;
	int retCode = pthread_create(&m_threadID, 0, getUpdateThread, this);
	if (retCode != 0)
	{
		LOG_E(retCode);
		m_bThreadON = false;
		return false;
	}

	return true;
}

void _Canbus::update(void)
{
	while (m_bThreadON)
	{
		if(!m_pSerialPort)
		{
			this->sleepTime(USEC_1SEC);
			continue;
		}

		if(!m_pSerialPort->isOpen())
		{
			if(!m_pSerialPort->open())
			{
				this->sleepTime(USEC_1SEC);
				continue;
			}
		}

		//Regular update loop
		this->autoFPSfrom();

		//Handling incoming messages
		recv();

		this->autoFPSto();
	}
}

bool _Canbus::recv()
{
	unsigned char inByte;
	int byteRead;

	while ((byteRead = m_pSerialPort->read(&inByte, 1)) > 0)
	{
		if (m_recvMsg.m_cmd != 0)
		{
			m_recvMsg.m_pBuf[m_recvMsg.m_iByte] = inByte;
			m_recvMsg.m_iByte++;

			if (m_recvMsg.m_iByte == 2)	//Payload length
			{
				m_recvMsg.m_payloadLen = inByte;
			}
			else if (m_recvMsg.m_iByte
					== m_recvMsg.m_payloadLen + MAVLINK_HEADDER_LEN + 1)
			{
				//decode the command
				recvMsg();
				m_recvMsg.m_cmd = 0;

				return true; //Execute one command at a time
			}
			else if (m_recvMsg.m_iByte >= CAN_BUF)
			{
				m_recvMsg.m_cmd = 0;
				return false;
			}
		}
		else if (inByte == MAVLINK_BEGIN)
		{
			m_recvMsg.m_cmd = inByte;
			m_recvMsg.m_pBuf[0] = inByte;
			m_recvMsg.m_iByte = 1;
			m_recvMsg.m_payloadLen = 0;
		}
	}

	return false;
}

void _Canbus::recvMsg(void)
{
	uint8_t cmd = m_recvMsg.m_pBuf[2];

	if(cmd == CMD_CAN_SEND)
	{
		uint32_t* pAddr = (uint32_t*)(&m_recvMsg.m_pBuf[3]);
		uint32_t addr = *pAddr;

		for(int i=0; i<m_nCanData; i++)
		{
			CAN_DATA* pCan = &m_pCanData[i];
			IF_CONT(pCan->m_addr != addr);

			pCan->m_len = m_recvMsg.m_pBuf[7];
			memcpy (pCan->m_pData, &m_recvMsg.m_pBuf[8], 8);

			LOG_I("Updated data for CANID:" + i2str(addr));
			return;
		}

		LOG_I("CANID:" + i2str(addr));
	}
}

uint8_t* _Canbus::get(unsigned long addr)
{
	for(int i=0; i<m_nCanData; i++)
	{
		CAN_DATA* pCan = &m_pCanData[i];
		IF_CONT(pCan->m_addr != addr);

		return pCan->m_pData;
	}

	return NULL;
}

void _Canbus::send(unsigned long addr, unsigned char len, unsigned char* pData)
{
	IF_(len+8 > CAN_BUF);
	IF_(!m_pSerialPort->isOpen());

	unsigned char pBuf[CAN_BUF];

	//Link header
	pBuf[0] = 0xFE; //Mavlink begin
	pBuf[1] = 5 + len;	//Payload Length
	pBuf[2] = CMD_CAN_SEND;

	//Payload from here
	//CAN addr
	pBuf[3] = (uint8_t)addr;
	pBuf[4] = (uint8_t)(addr >> 8);
	pBuf[5] = (uint8_t)(addr >> 16);
	pBuf[6] = (uint8_t)(addr >> 24);

	//CAN data len
	pBuf[7] = len;

	for (int i = 0; i < len; i++)
	{
		pBuf[i + 8] = pData[i];
	}
	//Payload to here

	m_pSerialPort->write(pBuf, len + 8);
}

void _Canbus::pinOut(uint8_t pin, uint8_t output)
{
	IF_(!m_pSerialPort->isOpen());

	unsigned char pBuf[CAN_BUF];

	//Link header
	pBuf[0] = 0xFE; //Mavlink begin
	pBuf[1] = 2;	//Payload Length
	pBuf[2] = CMD_PIN_OUTPUT;

	//Payload from here
	//CAN addr
	pBuf[3] = pin;
	pBuf[4] = output;
	//Payload to here

	m_pSerialPort->write(pBuf, 5);
}

bool _Canbus::draw(void)
{
	IF_F(!this->BASE::draw());
	Window* pWin = (Window*)this->m_pWindow;

	string msg = *this->getName();
	if (m_pSerialPort->isOpen())
		msg += ": CONNECTED";
	else
		msg += ": Not connected";

	pWin->addMsg(msg);

	return true;
}

}
