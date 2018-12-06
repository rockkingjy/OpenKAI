/*
 * _TCPclient.cpp
 *
 *  Created on: August 8, 2016
 *      Author: yankai
 */

#include "_TCPclient.h"

namespace kai
{

_TCPclient::_TCPclient()
{
	m_strAddr = "";
	m_port = 0;
	m_bClient = true;
	m_socket = 0;
	m_ioType = io_tcp;
	m_ioStatus = io_unknown;
}

_TCPclient::~_TCPclient()
{
	close();
}

bool _TCPclient::init(void* pKiss)
{
	IF_F(!this->_IOBase::init(pKiss));
	Kiss* pK = (Kiss*) pKiss;

	F_INFO(pK->v("addr", &m_strAddr));
	F_INFO(pK->v("port", (int* )&m_port));

	m_bClient = true;
	return true;
}

bool _TCPclient::open(void)
{
	m_socket = socket(AF_INET, SOCK_STREAM, 0);
	IF_F(m_socket < 0);

	struct sockaddr_in server;
	server.sin_addr.s_addr = inet_addr(m_strAddr.c_str());
	server.sin_family = AF_INET;
	server.sin_port = htons(m_port);
	LOG_I("connecting");

	int ret = ::connect(m_socket, (struct sockaddr *) &server, sizeof(server));
	if (ret < 0)
	{
		close();
		LOG_E("connect failed");
		return false;
	}

	m_ioStatus = io_opened;
	LOG_I("connected");

	return true;
}

void _TCPclient::close(void)
{
	IF_(m_ioStatus!=io_opened);
	::close(m_socket);
	this->_IOBase::close();
}

bool _TCPclient::start(void)
{
	int retCode;

	if(!m_bThreadON)
	{
		m_bThreadON = true;
		retCode = pthread_create(&m_threadID, 0, getUpdateThreadW, this);
		if (retCode != 0)
		{
			LOG_E(retCode);
			m_bThreadON = false;
			return false;
		}
	}

	if(!m_bRThreadON)
	{
		m_bRThreadON = true;
		retCode = pthread_create(&m_rThreadID, 0, getUpdateThreadR, this);
		if (retCode != 0)
		{
			LOG_E(retCode);
			m_bRThreadON = false;
			return false;
		}
	}

	return true;
}

void _TCPclient::updateW(void)
{
	while (m_bThreadON)
	{
		if (!isOpen())
		{
			if (!open())
			{
				this->sleepTime(USEC_1SEC);
				continue;
			}
		}

		this->autoFPSfrom();

		static uint8_t pBw[N_IO_BUF];
		int nB;
		while((nB = m_fifoW.output(pBw, N_IO_BUF)) > 0)
		{
			int nSend = ::send(m_socket, pBw, nB, 0);
			if (nSend == -1)
			{
				if(errno == EAGAIN)break;
				if(errno == EWOULDBLOCK)break;
				LOG_E("send error: " + i2str(errno));
				close();
				break;
			}

			LOG_I("send: " + i2str(nSend) + " bytes");
		}

		this->autoFPSto();
	}
}

void _TCPclient::updateR(void)
{
	while (m_bRThreadON)
	{
		if (!isOpen())
		{
			::sleep(1);
			continue;
		}

		//blocking mode, no FPS control
		static uint8_t pBr[N_IO_BUF];
		int nR = ::recv(m_socket, pBr, N_IO_BUF, 0);
		if (nR <= 0)
		{
			LOG_E("recv error: " + i2str(errno));
			close();
			continue;
		}

		m_fifoR.input(pBr,nR);
		this->wakeUpLinked();

		LOG_I("received: " + i2str(nR) + " bytes");
	}
}

bool _TCPclient::draw(void)
{
	IF_F(!this->_ThreadBase::draw());
	Window* pWin = (Window*)this->m_pWindow;
	Mat* pMat = pWin->getFrame()->m();

	string msg = "Peer IP: " + m_strAddr + ":" + i2str(m_port) + ((m_bClient) ? "; Client" : "; Server");
	pWin->addMsg(msg);

	return true;
}

}
